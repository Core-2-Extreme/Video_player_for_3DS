//Includes.
#include "system/util/sync.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/util.h"

//Defines.
#define DEF_SYNC_SPIN_MS		(uint16_t)(1000)

//Typedefs.
typedef struct
{
	LightLock lock;		//Lock object.
	void* owner;		//Owner for this sync object.
} Sync_internal_non_recursive_lock;

typedef struct
{
	uint32_t count;		//Lock count.
	void* owner;		//Owner for this sync object.
	RecursiveLock lock;	//Lock object.
} Sync_internal_recursive_lock;

typedef struct
{
	CondVar cond;		//Condition object.
} Sync_internal_cond;

typedef struct
{
	uint32_t ref_count;	//Reference count for this object.
	Sync_type type;		//Type for this sync object.
	union
	{
		Sync_internal_non_recursive_lock non_recursive;
		Sync_internal_recursive_lock recursive;
		Sync_internal_cond cond;
	} u;
} Sync_internal_data;

//Prototypes.
static inline bool Util_sync_is_valid(const Sync_data* sync_object);
static inline bool Util_sync_is_valid_lock_object(const Sync_data* lock_object);
static inline bool Util_sync_is_valid_cond_object(const Sync_data* cond_object);

//Variables.
static bool util_sync_init = false;
static LightLock util_sync_mutex = { 0, };

//Code.
uint32_t Util_sync_init(void)
{
	if(util_sync_init)
		goto api_already_inited;

	LightLock_Init(&util_sync_mutex);

	util_sync_init = true;
	return DEF_SUCCESS;

	api_already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;
}

void Util_sync_exit(void)
{
	if(!util_sync_init)
		return;

	util_sync_init = false;
}

uint32_t Util_sync_create(Sync_data* sync_object, Sync_type type)
{
	Sync_internal_data* data = NULL;

	if(!util_sync_init)
		goto api_not_inited;

	if(!sync_object || type <= SYNC_TYPE_INVALID || type >= SYNC_TYPE_MAX)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(Util_sync_is_valid(sync_object))
		goto object_already_inited;

	memset(sync_object, 0x0, sizeof(Sync_data));
	sync_object->internal_object = (Sync_internal_data*)malloc(sizeof(Sync_internal_data));
	if(!sync_object->internal_object)
		goto out_of_memory;

	//Init fields based on object type.
	data = (Sync_internal_data*)sync_object->internal_object;
	data->ref_count = 0;
	data->type = type;

	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
	{
		LightLock_Init(&data->u.non_recursive.lock);
		data->u.non_recursive.owner = NULL;
	}
	else if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
	{
		RecursiveLock_Init(&data->u.recursive.lock);
		data->u.recursive.count = 0;
		data->u.recursive.owner = NULL;
	}
	else if(data->type == SYNC_TYPE_CONDITION)
	{
		CondVar_Init(&data->u.cond.cond);
	}
	LightLock_Unlock(&util_sync_mutex);

	return DEF_SUCCESS;

	api_not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	object_already_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_ALREADY_INITIALIZED;

	out_of_memory:
	free(sync_object->internal_object);
	sync_object->internal_object = NULL;
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Util_sync_lock(Sync_data* lock_object, uint64_t wait_us)
{
	Sync_internal_data* data = NULL;

	if(!util_sync_init)
		goto api_not_inited;

	if(!lock_object)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid_lock_object(lock_object))
		goto object_not_inited;

	data = (Sync_internal_data*)lock_object->internal_object;
	if(data->ref_count == UINT32_MAX)
	{
		//Too many ref counts.
		goto out_of_memory;
	}

	if(data->type == SYNC_TYPE_RECURSIVE_MUTEX
	&& data->u.recursive.owner == getThreadLocalStorage()
	&& data->u.recursive.count == UINT32_MAX)
	{
		//We can't lock it anymore.
		goto out_of_memory;
	}

	data->ref_count++;

	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX
	|| data->type == SYNC_TYPE_RECURSIVE_MUTEX)
	{
		int32_t lock_result = -1;
		Sync_type type = data->type;

		if(type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
		{
			if(data->u.non_recursive.owner == getThreadLocalStorage())
				goto already_locked;
		}

		LightLock_Unlock(&util_sync_mutex);
		if(wait_us == UINT64_MAX)
		{
			if(type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
			{
				LightLock_Lock(&data->u.non_recursive.lock);
				lock_result = 0;
			}
			else if(type == SYNC_TYPE_RECURSIVE_MUTEX)
			{
				RecursiveLock_Lock(&data->u.recursive.lock);
				lock_result = 0;
			}
		}
		else
		{
			uint64_t remaining_us = wait_us;
			uint32_t sleep_us = 0;

			do
			{
				if(remaining_us > (DEF_SYNC_SPIN_MS * 1000))
				{
					sleep_us = (DEF_SYNC_SPIN_MS * 1000);
					remaining_us -= (DEF_SYNC_SPIN_MS * 1000);
				}
				else
				{
					sleep_us = remaining_us;
					remaining_us = 0;
				}

				if(type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
					lock_result = LightLock_TryLock(&data->u.non_recursive.lock);
				else if(type == SYNC_TYPE_RECURSIVE_MUTEX)
					lock_result = RecursiveLock_TryLock(&data->u.recursive.lock);

				if(lock_result == 0)
					break;//Success.

				Util_sleep(sleep_us);
			}
			while(sleep_us > 0);
		}
		LightLock_Lock(&util_sync_mutex);

		if(lock_result != 0)
			goto try_again;
	}
	else
		goto invalid_type;

	//Set the owner.
	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
		data->u.non_recursive.owner = getThreadLocalStorage();
	else if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
		data->u.recursive.owner = getThreadLocalStorage();

	if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
		data->u.recursive.count++;

	data->ref_count--;

	LightLock_Unlock(&util_sync_mutex);
	return DEF_SUCCESS;

	api_not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	object_not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	out_of_memory:
	LightLock_Unlock(&util_sync_mutex);
	//Log API may use sync API, so log it after unlocking mutex.
	DEF_LOG_FORMAT("Lock/ref count has overflowed!!!!!");
	return DEF_ERR_OUT_OF_MEMORY;

	already_locked:
	data->ref_count--;
	LightLock_Unlock(&util_sync_mutex);
	//Log API may use sync API, so log it after unlocking mutex.
	DEF_LOG_FORMAT("Already locked by the same owner (0x%08" PRIXPTR ")!!!!!", (uintptr_t)getThreadLocalStorage());
	return DEF_ERR_OTHER;

	try_again:
	data->ref_count--;
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_TRY_AGAIN;

	invalid_type:
	data->ref_count--;
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_sync_unlock(Sync_data* lock_object)
{
	void* owner = NULL;
	Sync_internal_data* data = NULL;

	if(!util_sync_init)
		goto api_not_inited;

	if(!lock_object)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid_lock_object(lock_object))
		goto object_not_inited;

	data = (Sync_internal_data*)lock_object->internal_object;

	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
		owner = data->u.non_recursive.owner;
	else if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
		owner = data->u.recursive.owner;

	if(owner != getThreadLocalStorage())
		goto wrong_owner;

	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
	{
		//Clear owner info.
		data->u.non_recursive.owner = NULL;

		LightLock_Unlock(&data->u.non_recursive.lock);
	}
	else if(data->type == SYNC_TYPE_RECURSIVE_MUTEX)
	{
		if(data->u.recursive.count == 0)
			goto count_underflow;

		data->u.recursive.count--;

		if(data->u.recursive.count == 0)
		{
			//Clear owner info.
			data->u.recursive.owner = NULL;
		}

		RecursiveLock_Unlock(&data->u.recursive.lock);
	}
	else
		goto invalid_type;

	LightLock_Unlock(&util_sync_mutex);
	return DEF_SUCCESS;

	api_not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	object_not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	wrong_owner:
	LightLock_Unlock(&util_sync_mutex);
	//Log API may use sync API, so log it after unlocking mutex.
	DEF_LOG_FORMAT("Wrong owner (current : 0x%08" PRIXPTR ") != (requested : 0x%08 " PRIXPTR ")!!!!!", (uintptr_t)owner, (uintptr_t)getThreadLocalStorage());
	return DEF_ERR_OTHER;

	count_underflow:
	LightLock_Unlock(&util_sync_mutex);
	//Log API may use sync API, so call it after unlocking mutex.
	DEF_LOG_FORMAT("Lock count is 0!!!!!");
	return DEF_ERR_OTHER;

	invalid_type:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_sync_cond_wait(Sync_data* cond_object, Sync_data* lock_object, uint64_t wait_us)
{
	int32_t result = 0;
	void* owner = NULL;
	Sync_internal_data* cond_data = NULL;
	Sync_internal_data* lock_data = NULL;

	if(!util_sync_init)
		goto api_not_inited;

	if(!cond_object || !lock_object)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid_cond_object(cond_object) || !Util_sync_is_valid_lock_object(lock_object))
		goto object_not_inited;

	cond_data = (Sync_internal_data*)cond_object->internal_object;
	lock_data = (Sync_internal_data*)lock_object->internal_object;

	if(cond_data->ref_count == UINT32_MAX
	|| lock_data->ref_count == UINT32_MAX)
	{
		//Too many ref counts.
		goto out_of_memory;
	}

	cond_data->ref_count++;
	lock_data->ref_count++;

	if(cond_data->type == SYNC_TYPE_CONDITION && lock_data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX)
	{
		owner = lock_data->u.non_recursive.owner;
		if(owner != getThreadLocalStorage())
			goto wrong_owner;

		//Mutex will be unlocked by CondVar_WaitTimeout().
		lock_data->u.non_recursive.owner = NULL;

		LightLock_Unlock(&util_sync_mutex);
		if(wait_us == UINT64_MAX)
		{
			CondVar_Wait(&cond_data->u.cond.cond, &lock_data->u.non_recursive.lock);
			result = 0;
		}
		else
		{
			int64_t wait_ns = 0;
			uint64_t remaining_us = wait_us;

			do
			{
				//Set timeout.
				if(remaining_us > (INT64_MAX / 1000))
				{
					wait_ns = INT64_MAX;
					remaining_us -= (INT64_MAX / 1000);
				}
				else
				{
					wait_ns = (remaining_us * 1000);
					remaining_us = 0;
				}

				result = CondVar_WaitTimeout(&cond_data->u.cond.cond, &lock_data->u.non_recursive.lock, wait_ns);
				if(result == 0)
					break;//Success.
			}
			while(remaining_us > 0);
		}
		LightLock_Lock(&util_sync_mutex);

		//Mutex was relocked by CondVar_WaitTimeout().
		lock_data->u.non_recursive.owner = owner;

		if(result != 0)
			goto try_again;
	}
	else
		goto invalid_type;

	cond_data->ref_count--;
	lock_data->ref_count--;
	LightLock_Unlock(&util_sync_mutex);
	return DEF_SUCCESS;

	api_not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	object_not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	out_of_memory:
	LightLock_Unlock(&util_sync_mutex);
	//Log API may use sync API, so log it after unlocking mutex.
	DEF_LOG_FORMAT("Ref count(s) have overflowed!!!!!");
	return DEF_ERR_OUT_OF_MEMORY;

	wrong_owner:
	LightLock_Unlock(&util_sync_mutex);
	//Log API may use sync API, so log it after unlocking mutex.
	DEF_LOG_FORMAT("Wrong owner (current : 0x%08" PRIXPTR ") != (requested : 0x%08 " PRIXPTR ")!!!!!", (uintptr_t)owner, (uintptr_t)getThreadLocalStorage());
	return DEF_ERR_OTHER;

	try_again:
	cond_data->ref_count--;
	lock_data->ref_count--;
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_TRY_AGAIN;

	invalid_type:
	cond_data->ref_count--;
	lock_data->ref_count--;
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_sync_cond_signal(Sync_data* cond_object, bool is_broadcast)
{
	Sync_internal_data* data = NULL;

	if(!util_sync_init)
		goto api_not_inited;

	if(!cond_object)
		goto invalid_arg;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid_cond_object(cond_object))
		goto object_not_inited;

	data = (Sync_internal_data*)cond_object->internal_object;

	if(data->type == SYNC_TYPE_CONDITION)
	{
		if(is_broadcast)
			CondVar_Broadcast(&data->u.cond.cond);
		else
			CondVar_Signal(&data->u.cond.cond);
	}
	else
		goto invalid_type;

	LightLock_Unlock(&util_sync_mutex);
	return DEF_SUCCESS;

	api_not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	object_not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	invalid_type:
	LightLock_Unlock(&util_sync_mutex);
	return DEF_ERR_INVALID_ARG;
}

void Util_sync_destroy(Sync_data* sync_object)
{
	Sync_internal_data* data = NULL;

	if(!util_sync_init)
		goto api_not_inited;

	LightLock_Lock(&util_sync_mutex);
	if(!Util_sync_is_valid(sync_object))
		goto object_not_inited;

	data = (Sync_internal_data*)sync_object->internal_object;

	//Wait for all references to be removed.
	while(data->ref_count > 0)
	{
		LightLock_Unlock(&util_sync_mutex);
		Util_sleep(1000);
		LightLock_Lock(&util_sync_mutex);
	}

	//Delete it.
	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX
	|| data->type == SYNC_TYPE_RECURSIVE_MUTEX
	|| data->type == SYNC_TYPE_CONDITION)
	{
		free(sync_object->internal_object);
		sync_object->internal_object = NULL;
	}

	LightLock_Unlock(&util_sync_mutex);
	return;

	api_not_inited:
	return;

	object_not_inited:
	LightLock_Unlock(&util_sync_mutex);
	return;
}

static inline bool Util_sync_is_valid(const Sync_data* sync_object)
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

static inline bool Util_sync_is_valid_lock_object(const Sync_data* lock_object)
{
	Sync_internal_data* data = NULL;

	if(!lock_object || !lock_object->internal_object)
		return false;

	data = (Sync_internal_data*)lock_object->internal_object;
	if(data->type == SYNC_TYPE_NON_RECURSIVE_MUTEX || data->type == SYNC_TYPE_RECURSIVE_MUTEX)
		return true;
	else
		return false;
}

static inline bool Util_sync_is_valid_cond_object(const Sync_data* cond_object)
{
	Sync_internal_data* data = NULL;

	if(!cond_object || !cond_object->internal_object)
		return false;

	data = (Sync_internal_data*)cond_object->internal_object;
	if(data->type == SYNC_TYPE_CONDITION)
		return true;
	else
		return false;
}
