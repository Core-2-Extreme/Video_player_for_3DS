//Includes.
#include "system/util/fake_pthread.h"

#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/thread_types.h"
#include "system/util/util.h"

//Defines.
#define DEF_FAKE_PTHREAD_STACK_SIZE_MIN		(uint32_t)(16384)

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
static int util_fake_pthread_core_offset = 0;
static int util_fake_pthread_enabled_core_list[4] = { 0, 1, -3, -3, };
static int util_fake_pthread_enabled_cores = 2;
static LightLock util_fake_pthread_mutex = 1;//Initially unlocked state.

//Code.
void Util_fake_pthread_set_enabled_core(const bool enabled_core[4])
{
	int num_of_core = 0;

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
	LightLock_Init((LightLock*)__mutex);
	return 0;
}

int	__wrap_pthread_mutex_lock(pthread_mutex_t* __mutex)
{
	LightLock_Lock((LightLock*)__mutex);
	return 0;
}

int	__wrap_pthread_mutex_unlock(pthread_mutex_t* __mutex)
{
	LightLock_Unlock((LightLock*)__mutex);
	return 0;
}

int	__wrap_pthread_mutex_destroy(pthread_mutex_t* __mutex)
{
	(void)__mutex;
	return 0;
}

int	__wrap_pthread_once(pthread_once_t* __once_control, void (*__init_routine)(void))
{
	LightLock_Lock(&util_fake_pthread_mutex);
	if(__once_control->status != 0)
	{
		LightLock_Unlock(&util_fake_pthread_mutex);
		return 0;
	}

	__once_control->status = 1;
	LightLock_Unlock(&util_fake_pthread_mutex);

	__init_routine();

	//Only 1 thread can reach here, so we don't need to lock mutex.
	__once_control->status = 2;

	return 0;
}

int	__wrap_pthread_cond_init(pthread_cond_t* __cond, const pthread_condattr_t* __attr)
{
	(void)__attr;
	CondVar_Init((CondVar*)__cond);
	return 0;
}

int	__wrap_pthread_cond_wait(pthread_cond_t* __cond, pthread_mutex_t* __mutex)
{
	CondVar_Wait((CondVar*)__cond, (LightLock*)__mutex);
	return 0;
}

int	__wrap_pthread_cond_signal(pthread_cond_t* __cond)
{
	CondVar_Signal((CondVar*)__cond);
	return 0;
}

int	__wrap_pthread_cond_broadcast(pthread_cond_t* __cond)
{
	CondVar_Broadcast((CondVar*)__cond);
	return 0;
}

int	__wrap_pthread_cond_destroy(pthread_cond_t* __cond)
{
	(void)__cond;
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

	if(util_fake_pthread_enabled_cores == 0)
		return -1;

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
		return -1;
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
	while(true)
	{
		int result = threadJoin((Thread)__pthread, U64_MAX);
		if(result == DEF_SUCCESS)
			return 0;
	}
}

int __wrap_pthread_attr_init(pthread_attr_t* attr)
{
	if(!attr)
		return -1;

	attr->stackaddr = NULL;
	attr->stacksize = DEF_THREAD_STACKSIZE;
	attr->schedparam.sched_priority = DEF_THREAD_PRIORITY_LOW;
	attr->detachstate = PTHREAD_CREATE_JOINABLE;
	return 0;
}

int __wrap_pthread_attr_destroy(pthread_attr_t* attr)
{
	if(!attr)
		return -1;

	memset(attr, 0x0, sizeof(pthread_attr_t));
	return 0;
}

int __wrap_pthread_attr_setstacksize(pthread_attr_t* attr, size_t stacksize)
{
	if(!attr || stacksize < DEF_FAKE_PTHREAD_STACK_SIZE_MIN)
		return -1;

	attr->stacksize = stacksize;
	return 0;
}
