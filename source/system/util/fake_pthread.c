//Includes.
#include "system/util/fake_pthread.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/sync.h"
#include "system/util/thread_types.h"
#include "system/util/util.h"

//Defines.
#define STACK_SIZE_MIN		(uint32_t)(16384)

//Typedefs.
//N/A.

//Prototypes.
int	__wrap_pthread_mutex_init(pthread_mutex_t* __mutex, const pthread_mutexattr_t* __attr);
int	__wrap_pthread_mutex_lock(pthread_mutex_t* __mutex);
int	__wrap_pthread_mutex_unlock(pthread_mutex_t* __mutex);
int	__wrap_pthread_mutex_destroy(pthread_mutex_t* __mutex);
int	__wrap_pthread_once(pthread_once_t* __once_control, void (*__init_routine)(void));
int	__wrap_pthread_cond_init(pthread_cond_t* __cond, const pthread_condattr_t* __attr);
int	__wrap_pthread_cond_wait(pthread_cond_t* __cond, pthread_mutex_t* __mutex);
int	__wrap_pthread_cond_signal(pthread_cond_t* __cond);
int	__wrap_pthread_cond_broadcast(pthread_cond_t* __cond);
int	__wrap_pthread_cond_destroy(pthread_cond_t* __cond);
int	__wrap_pthread_create(pthread_t* __pthread, const pthread_attr_t * __attr, void* (*__start_routine)(void*), void* __arg);
int	__wrap_pthread_join(pthread_t __pthread, void** __value_ptr);
int __wrap_pthread_attr_init(pthread_attr_t* attr);
int __wrap_pthread_attr_destroy(pthread_attr_t* attr);
int __wrap_pthread_attr_setstacksize(pthread_attr_t* attr, size_t stacksize);

//Variables.
static bool util_fake_pthread_init = false;
static int util_fake_pthread_core_offset = 0;
static int util_fake_pthread_enabled_core_list[4] = { 0, 1, -3, -3, };
static int util_fake_pthread_enabled_cores = 2;
static Sync_data util_fake_pthread_mutex = { 0, };

//Code.
uint32_t Util_fake_pthread_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_fake_pthread_init)
		goto already_inited;

	result = Util_sync_create(&util_fake_pthread_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	util_fake_pthread_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	error_other:
	return result;
}

void Util_fake_pthread_exit(void)
{
	if(!util_fake_pthread_init)
		return;

	util_fake_pthread_init = false;
	Util_sync_destroy(&util_fake_pthread_mutex);
}

void Util_fake_pthread_set_enabled_core(const bool enabled_core[4])
{
	int num_of_core = 0;

	if(!util_fake_pthread_init)
		return;

	if(!enabled_core[0] && !enabled_core[1] && !enabled_core[2] && !enabled_core[3])
		return;

	for(int i = 0; i < 4; i++)
		util_fake_pthread_enabled_core_list[i] = -3;

	for(int i = 0; i < 4; i++)
	{
		if(enabled_core[i])
		{
			util_fake_pthread_enabled_core_list[num_of_core] = i;
			num_of_core++;
		}
	}
	util_fake_pthread_enabled_cores = num_of_core;
	util_fake_pthread_core_offset = 0;
}

int	__wrap_pthread_mutex_init(pthread_mutex_t* __mutex, const pthread_mutexattr_t* __attr)
{
	(void)__attr;
	uint32_t result = DEF_ERR_OTHER;
	Sync_data* sync_data = NULL;

	__mutex->normal = 0;

	if(!util_fake_pthread_init)
		return ENOSYS;

	sync_data = (Sync_data*)malloc(sizeof(Sync_data));
	if(!sync_data)
		return ENOMEM;

	memset(sync_data, 0x00, sizeof(Sync_data));
	result = Util_sync_create(sync_data, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		free(sync_data);
		return EINVAL;
	}

	__mutex->normal = (_LOCK_T)sync_data;
	return 0;
}

int	__wrap_pthread_mutex_lock(pthread_mutex_t* __mutex)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!__mutex->normal)
		return EINVAL;

	result = Util_sync_lock((Sync_data*)__mutex->normal, UINT64_MAX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_lock, false, result);
		return EINVAL;
	}

	return 0;
}

int	__wrap_pthread_mutex_unlock(pthread_mutex_t* __mutex)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!__mutex->normal)
		return EINVAL;

	result = Util_sync_unlock((Sync_data*)__mutex->normal);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_unlock, false, result);
		return EINVAL;
	}

	return 0;
}

int	__wrap_pthread_mutex_destroy(pthread_mutex_t* __mutex)
{
	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!__mutex->normal)
		return EINVAL;

	Util_sync_destroy((Sync_data*)__mutex->normal);
	free((void*)__mutex->normal);
	__mutex->normal = 0;

	return 0;
}

int	__wrap_pthread_once(pthread_once_t* __once_control, void (*__init_routine)(void))
{
	if(!util_fake_pthread_init)
		return ENOSYS;

	Util_sync_lock(&util_fake_pthread_mutex, UINT64_MAX);
	if(__once_control->status != 0)
	{
		Util_sync_unlock(&util_fake_pthread_mutex);
		return 0;
	}

	__once_control->status = 1;
	Util_sync_unlock(&util_fake_pthread_mutex);

	__init_routine();

	//Only 1 thread can reach here, so we don't need to lock mutex.
	__once_control->status = 2;

	return 0;
}

int	__wrap_pthread_cond_init(pthread_cond_t* __cond, const pthread_condattr_t* __attr)
{
	(void)__attr;
	uint32_t result = DEF_ERR_OTHER;
	Sync_data* sync_data = NULL;

	if(!util_fake_pthread_init)
		return ENOSYS;

	__cond->cond = 0;
	sync_data = (Sync_data*)malloc(sizeof(Sync_data));
	if(!sync_data)
		return ENOMEM;

	memset(sync_data, 0x00, sizeof(Sync_data));
	result = Util_sync_create(sync_data, SYNC_TYPE_CONDITION);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		free(sync_data);
		return EINVAL;
	}

	__cond->cond = (_COND_T)sync_data;
	return 0;
}

int	__wrap_pthread_cond_wait(pthread_cond_t* __cond, pthread_mutex_t* __mutex)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!__cond->cond || !__mutex->normal)
		return EINVAL;

	result = Util_sync_cond_wait((Sync_data*)__cond->cond, (Sync_data*)__mutex->normal, UINT64_MAX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_cond_wait, false, result);
		return EINVAL;
	}

	return 0;
}

int	__wrap_pthread_cond_signal(pthread_cond_t* __cond)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!__cond->cond)
		return EINVAL;

	result = Util_sync_cond_signal((Sync_data*)__cond->cond, false);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_cond_signal, false, result);
		return EINVAL;
	}

	return 0;
}

int	__wrap_pthread_cond_broadcast(pthread_cond_t* __cond)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!__cond->cond)
		return EINVAL;

	result = Util_sync_cond_signal((Sync_data*)__cond->cond, true);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_cond_signal, false, result);
		return EINVAL;
	}

	return 0;
}

int	__wrap_pthread_cond_destroy(pthread_cond_t* __cond)
{
	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!__cond->cond)
		return EINVAL;

	Util_sync_destroy((Sync_data*)__cond->cond);
	free((void*)__cond->cond);
	__cond->cond = 0;

	return 0;
}

int	__wrap_pthread_create(pthread_t* __pthread, const pthread_attr_t * __attr, void* (*__start_routine)(void*), void* __arg)
{
	Thread handle = 0;

#if __GNUC__
	//We can't avoid "-Wcast-function-type".
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
	ThreadFunc entry_point = (ThreadFunc)__start_routine;
#pragma GCC diagnostic pop
#else
	ThreadFunc entry_point = (ThreadFunc)__start_routine;
#endif //__GNUC__

	if(!util_fake_pthread_init)
		return ENOSYS;

	if(util_fake_pthread_enabled_cores == 0)
		return ENOSYS;

	if(__attr)
		handle = threadCreate(entry_point, __arg, __attr->stacksize, DEF_THREAD_PRIORITY_BELOW_NORMAL, util_fake_pthread_enabled_core_list[util_fake_pthread_core_offset], true);
	else
		handle = threadCreate(entry_point, __arg, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_BELOW_NORMAL, util_fake_pthread_enabled_core_list[util_fake_pthread_core_offset], true);

	if(util_fake_pthread_core_offset + 1 < util_fake_pthread_enabled_cores)
		util_fake_pthread_core_offset++;
	else
		util_fake_pthread_core_offset = 0;

	if(!handle)
	{
		DEF_LOG_RESULT(threadCreate, false, 0);
		return ENOMEM;
	}
	else
	{
		*__pthread = (pthread_t)handle;
		return 0;
	}
}

int	__wrap_pthread_join(pthread_t __pthread, void** __value_ptr)
{
	(void)__value_ptr;

	if(!util_fake_pthread_init)
		return ENOSYS;

	while(true)
	{
		int result = threadJoin((Thread)__pthread, U64_MAX);
		if(result == DEF_SUCCESS)
			return 0;
	}
}

int __wrap_pthread_attr_init(pthread_attr_t* attr)
{
	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!attr)
		return EINVAL;

	attr->stackaddr = NULL;
	attr->stacksize = DEF_THREAD_STACKSIZE;
	attr->schedparam.sched_priority = DEF_THREAD_PRIORITY_LOW;
	attr->detachstate = PTHREAD_CREATE_JOINABLE;
	return 0;
}

int __wrap_pthread_attr_destroy(pthread_attr_t* attr)
{
	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!attr)
		return EINVAL;

	memset(attr, 0x0, sizeof(pthread_attr_t));
	return 0;
}

int __wrap_pthread_attr_setstacksize(pthread_attr_t* attr, size_t stacksize)
{
	if(!util_fake_pthread_init)
		return ENOSYS;

	if(!attr || stacksize < STACK_SIZE_MIN)
		return EINVAL;

	attr->stacksize = stacksize;
	return 0;
}
