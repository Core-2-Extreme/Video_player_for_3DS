#include "system/headers.hpp"

int util_fake_pthread_core_offset = 0;
int util_fake_pthread_enabled_core_list[4] = { 0, 1, -3, -3, };
int util_fake_pthread_enabled_cores = 2;

void Util_fake_pthread_set_enabled_core(bool enabled_core[4])
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

int	pthread_mutex_lock(pthread_mutex_t *__mutex)
{
    uint result = 0;

    while(true)
    {
        result = svcWaitSynchronization((Handle)*__mutex, U64_MAX);
        if(result == 0)
            return 0;

        if(result == 0xD8E007F7)
        {
            result = pthread_mutex_init(__mutex, NULL);
            if(result != 0)
                return -1;
        }
    }
}

int	pthread_mutex_unlock(pthread_mutex_t *__mutex)
{
    return svcReleaseMutex((Handle)*__mutex);
}

int	pthread_mutex_init(pthread_mutex_t *__mutex, const pthread_mutexattr_t *__attr)
{
    return svcCreateMutex((Handle*)__mutex, false);
}

int	pthread_mutex_destroy(pthread_mutex_t *__mutex)
{
    return svcCloseHandle((Handle)*__mutex);
}

int	pthread_once(pthread_once_t *__once_control, void (*__init_routine)(void))
{
    if(__once_control->init_executed == 0)
    {
        __once_control->is_initialized = 1;
        __once_control->init_executed = 1;
        __init_routine();
    }
    return 0;
}

int	pthread_cond_wait(pthread_cond_t *__cond, pthread_mutex_t *__mutex)
{
    uint result = 0;
    pthread_mutex_unlock(__mutex);

    while(true)
    {
        result = svcWaitSynchronization((Handle)*__cond, U64_MAX);
        if(result == 0)
            return 0;

        if(result == 0xD8E007F7)
        {
            result = pthread_cond_init(__cond, NULL);
            if(result != 0)
                return -1;
        }
    }
}

int	pthread_cond_signal(pthread_cond_t *__cond)
{
    return svcSignalEvent((Handle)*__cond);
}

int	pthread_cond_broadcast(pthread_cond_t *__cond)
{
    uint result = 0;
    
    while(true)
    {
        result = svcSignalEvent((Handle)*__cond);
        if(result == 0xD8E007F7)
        {
            result = pthread_cond_init(__cond, NULL);
            if(result != 0)
                return -1;
            else
                continue;
        }

        result = svcWaitSynchronization((Handle)*__cond, 0);
        if(result == 0)
            return 0;
    }
}

int	pthread_cond_init(pthread_cond_t *__cond, const pthread_condattr_t *__attr)
{
    return svcCreateEvent((Handle*)__cond, RESET_ONESHOT);
}

int	pthread_cond_destroy(pthread_cond_t *__mutex)
{
    return svcCloseHandle((Handle)*__mutex);
}

int	pthread_create(pthread_t *__pthread, const pthread_attr_t  *__attr, void *(*__start_routine)(void *), void *__arg)
{
    Thread handle = 0;

    if(util_fake_pthread_enabled_cores == 0)
        return -1;

    handle = threadCreate((ThreadFunc)__start_routine, __arg, DEF_STACKSIZE, DEF_THREAD_PRIORITY_LOW, util_fake_pthread_enabled_core_list[util_fake_pthread_core_offset], true);
    *__pthread = (pthread_t)handle;

    if(util_fake_pthread_core_offset + 1 < util_fake_pthread_enabled_cores)
        util_fake_pthread_core_offset++;
    else
        util_fake_pthread_core_offset = 0;

    if(!handle)
        return -1;
    else
        return 0;
}

int	pthread_join(pthread_t __pthread, void **__value_ptr)
{
    int result = -1;
    while(true)
    {
        result = threadJoin((Thread)__pthread, U64_MAX);
        if(result == 0)
            return 0;
    }
}

int pthread_attr_init(pthread_attr_t *attr)
{
    if(!attr)
        return -1;

    attr->is_initialized = true;
    attr->stackaddr = NULL;
    attr->stacksize = DEF_STACKSIZE;
    attr->contentionscope = PTHREAD_SCOPE_SYSTEM;
    attr->inheritsched = PTHREAD_INHERIT_SCHED;
    attr->schedpolicy = SCHED_FIFO;
    attr->schedparam.sched_priority = DEF_THREAD_PRIORITY_LOW;
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
    if(!attr)
        return -1;

    attr->is_initialized = false;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if(!attr || stacksize < 16384)
        return -1;

    attr->stacksize = stacksize;
    return 0;
}

int posix_memalign(void **memptr, size_t alignment, size_t size)
{
    *memptr = memalign(alignment, size);
    if(!*memptr)
        return -1;
    else
        return 0;
}

long sysconf(int name)
{
    if(name == _SC_NPROCESSORS_CONF)
    {
        if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
            return 4;
        else
            return 2;
    }
    else if(name == _SC_NPROCESSORS_ONLN)
    {
        if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
            return 2 + var_core_2_available + var_core_3_available;
        else
            return 2;
    }
    else
        return -1;
}