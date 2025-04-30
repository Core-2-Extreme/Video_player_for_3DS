//Includes.
#include "system/util/sync.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/util.h"

//Defines.
//N/A.

//Typedefs.
typedef struct
{
	LightLock lock;		//Lock object.
} Sync_internal_non_recursive_lock;

typedef struct
{
	uint32_t count;		//Lock count.
	RecursiveLock lock;	//Lock object.
} Sync_internal_recursive_lock;

typedef struct
{
	void* owner;		//Owner for this sync object.
	Sync_type type;		//Type for this sync object.
	union
	{
		Sync_internal_non_recursive_lock non_recursive;
		Sync_internal_recursive_lock recursive;
	} u;
} Sync_internal_data;

//Prototypes.
static void* Util_sync_get_tls(void);
static bool Util_sync_is_valid(const Sync_data* sync_object);

//Variables.
static LightLock util_sync_mutex = 1;//Initially unlocked state.

//Code.
uint32_t Util_sync_create(Sync_data* sync_object, Sync_type type)
{
	Sync_internal_data* data = NULL;

	if(!sync_object || type <= SYNC_TYPE_INVALID || type >= SYNC_TYPE_MAX)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(Util_sync_is_valid(sync_object))
		goto already_inited;

	memset(sync_object, 0x0, sizeof(Sync_data));
	sync_object->internal_object = (Sync_internal_data*)malloc(sizeof(Sync_internal_data));
	if(!sync_object->internal_object)
		goto out_of_memory;

	//Init fields based on object type.
	data = (Sync_internal_data*)sync_object->internal_object;
	data->owner = NULL;
	data->type = type;

	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
		LightLock_Init(&data->u.non_recursive.lock);
	else if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
	{
		RecursiveLock_Init(&data->u.recursive.lock);
		data->u.recursive.count = 0;
	}
	LightLock_Unlock(&util_sync_mutex);

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_ALREADY_INITIALIZED;

	out_of_memory:
	free(sync_object->internal_object);
	sync_object->internal_object = NULL;
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Util_sync_lock(Sync_data* sync_object, uint64_t wait_us)
{
	uint64_t end_ms = 0;
	Sync_internal_data* data = NULL;

	if(!sync_object)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid(sync_object))
		goto not_inited;

	data = (Sync_internal_data*)sync_object->internal_object;

	//Set timeout then try to lock it.
	if(wait_us == 0)
		end_ms = 0;
	else
		end_ms = (osGetTime() + ((wait_us + 999) / 1000));

	if(data->type == SYNC_TYPE_RECURSIVE_MUTEX
	&& data->owner == Util_sync_get_tls()
	&& data->u.recursive.count == UINT32_MAX)
	{
		//We can't lock it anymore.
		DEF_LOG_FORMAT("Lock count has overflowed!!!!!");
		goto out_of_memory;
	}

	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX
	|| data->type == SYNC_TYPE_RECURSIVE_MUTEX)
	{
		int32_t lock_result = -1;

		while(true)
		{
			if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
			{
				if(data->owner == Util_sync_get_tls())
				{
					DEF_LOG_FORMAT("Already locked by the same owner (0x%08" PRIXPTR ")!!!!!", (uintptr_t)Util_sync_get_tls());
					goto already_locked;
				}

				lock_result = LightLock_TryLock(&data->u.non_recursive.lock);
			}
			else if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
				lock_result = RecursiveLock_TryLock(&data->u.recursive.lock);

			if(lock_result == 0)
				break;

			if(end_ms > osGetTime())
			{
				//Let other sync operations execute while we are sleeping.
				LightLock_Unlock(&util_sync_mutex);
				Util_sleep(1000);
				LightLock_Lock(&util_sync_mutex);

				//Check if sync object is still valid before continuing.
				if(!Util_sync_is_valid(sync_object))
					goto no_longer_valid;
			}
			else
				goto try_again;//We couldn't lock it within the timeout.
		}
	}

	//Set the owner.
	data->owner = Util_sync_get_tls();
	if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
		data->u.recursive.count++;

	LightLock_Unlock(&util_sync_mutex);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	out_of_memory:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_OUT_OF_MEMORY;

	already_locked:
	no_longer_valid:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_OTHER;

	try_again:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_TRY_AGAIN;
}

uint32_t Util_sync_unlock(Sync_data* sync_object)
{
	Sync_internal_data* data = NULL;

	if(!sync_object)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid(sync_object))
		goto not_inited;

	data = (Sync_internal_data*)sync_object->internal_object;
	if(data->owner != Util_sync_get_tls())
	{
		DEF_LOG_FORMAT("Wrong owner (current : 0x%08" PRIXPTR ") != (requested : 0x%08 " PRIXPTR ")!!!!!", (uintptr_t)data->owner, (uintptr_t)Util_sync_get_tls());
		goto wrong_owner;
	}

	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
	{
		//Clear owner info.
		data->owner = NULL;

		LightLock_Unlock(&data->u.non_recursive.lock);
	}
	else if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
	{
		if(data->u.recursive.count == 0)
		{
			DEF_LOG_FORMAT("Lock count is 0!!!!!");
			//We still need to clear owner and unlock mutex so let's continue.
		}
		else
			data->u.recursive.count--;

		if(data->u.recursive.count == 0)
		{
			//Clear owner info.
			data->owner = NULL;
		}

		RecursiveLock_Unlock(&data->u.recursive.lock);
	}

	LightLock_Unlock(&util_sync_mutex);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	wrong_owner:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_OTHER;
}

void Util_sync_destroy(Sync_data* sync_object)
{
	Sync_internal_data* data = NULL;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid(sync_object))
		goto not_inited;

	data = (Sync_internal_data*)sync_object->internal_object;

	//Delete it.
	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX
	|| data->type == SYNC_TYPE_RECURSIVE_MUTEX)
	{
		free(sync_object->internal_object);
		sync_object->internal_object = NULL;
	}

	LightLock_Unlock(&util_sync_mutex);
	return;

	not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return;
}

static void* Util_sync_get_tls(void)
{
	void* ret;
	__asm__ ("mrc p15, 0, %[data], c13, c0, 3" : [data] "=r" (ret));
	return ret;
}

static bool Util_sync_is_valid(const Sync_data* sync_object)
{
	Sync_internal_data* data = NULL;

	if(!sync_object || !sync_object->internal_object)
		return false;

	data = (Sync_internal_data*)sync_object->internal_object;
	if(data->type <= SYNC_TYPE_INVALID || data->type >= SYNC_TYPE_MAX)
		return false;
	else
		return true;
}
