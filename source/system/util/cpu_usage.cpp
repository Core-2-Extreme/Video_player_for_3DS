#include "definitions.hpp"

#if DEF_ENABLE_CPU_MONITOR_API
#include "system/types.hpp"

#include "system/variables.hpp"

#include "system/util/log.hpp"
#include "system/util/util.hpp"

//Include myself.
#include "system/util/cpu_usage.hpp"

bool util_cpu_usage_monitor_init = false;
bool util_cpu_usage_reset_counter_request[4] = { false, false, false, false, };
u8 util_cpu_usage_core_id[4] = { 0, 1, 2, 3, };
u16 util_cpu_usage_counter_cache[4] = { 0, 0, 0, 0, };
u32 util_cpu_usage_max_core_1 = 0;
float util_cpu_usage_per_core[4] = { NAN, NAN, NAN, NAN, };
float util_cpu_usage = NAN;
Thread util_cpu_usage_thread_handle[5] = { 0, 0, 0, 0, };
Handle timer_handle = 0;

void Util_cpu_usage_counter_thread(void* arg);
void Util_cpu_usage_calculate_thread(void* arg);

Result_with_string Util_cpu_usage_monitor_init(void)
{
	Result_with_string result;

	if(util_cpu_usage_monitor_init)
		goto already_inited;

	util_cpu_usage_monitor_init = true;
	for(int i = 0; i < 4; i++)
	{
		if(i == 2 && !var_core_2_available)
			continue;
		if(i == 3 && !var_core_3_available)
			continue;

		//This may fail depending on core availability.
		util_cpu_usage_thread_handle[i] = threadCreate(Util_cpu_usage_counter_thread, &util_cpu_usage_core_id[i], 2048, DEF_SYSTEM_THREAD_PRIORITY_IDLE, i, false);
	}

	util_cpu_usage_thread_handle[4] = threadCreate(Util_cpu_usage_calculate_thread, NULL, 2048, DEF_SYSTEM_THREAD_PRIORITY_REALTIME, 0, false);
	if(!util_cpu_usage_thread_handle[4])
	{
		result.code = DEF_ERR_OTHER;
		result.error_description = "[Error] threadCreate() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	nintendo_api_failed:
	util_cpu_usage_monitor_init = false;
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;
}

void Util_cpu_usage_monitor_exit(void)
{
	if(!util_cpu_usage_monitor_init)
		return;

	util_cpu_usage_monitor_init = false;
	svcSignalEvent(timer_handle);
	for(int i = 0; i < 5; i++)
	{
		if(util_cpu_usage_thread_handle[i])
		{
			threadJoin(util_cpu_usage_thread_handle[i], DEF_THREAD_WAIT_TIME);
			threadFree(util_cpu_usage_thread_handle[i]);
		}
	}
}

float Util_cpu_usage_monitor_get_cpu_usage(s8 core_id)
{
	if(!util_cpu_usage_monitor_init)
		return NAN;

	if(core_id < -1 || core_id > 3)
		return NAN;

	if(core_id == -1)
		return util_cpu_usage;
	else
		return util_cpu_usage_per_core[core_id];
}


void Util_cpu_usage_counter_thread(void* arg)
{
	u8 core_id = 0;

	if(!arg || *(u8*)arg < 0 || *(u8*)arg > 3)
	{
		Util_log_save(DEF_CPU_COUNTER_THREAD_STR, "Invalid arg!!!!!");
		threadExit(0);
	}

	core_id = *(u8*)arg;
	Util_log_save(DEF_CPU_COUNTER_THREAD_STR + "#" + std::to_string(core_id), "Thread started.");

	//This thread will run at the lowest priority.
	while(util_cpu_usage_monitor_init)
	{
		//1ms
		Util_sleep(1000);
		//In ideal condition (CPU usage is 0%), it should be 1000 in 1000ms.
		util_cpu_usage_counter_cache[core_id]++;

		if(util_cpu_usage_reset_counter_request[core_id])
		{
			util_cpu_usage_counter_cache[core_id] = 0;
			util_cpu_usage_reset_counter_request[core_id] = false;
		}
	}

	Util_log_save(DEF_CPU_COUNTER_THREAD_STR + "#" + std::to_string(core_id), "Thread exit.");
	threadExit(0);
}

void Util_cpu_usage_calculate_thread(void* arg)
{
	Util_log_save(DEF_CPU_CALCULATE_THREAD_STR, "Thread started.");

	u8 div = 0;
	float total_cpu_usage = 0;
	float cpu_usage_cache = 0;

	svcCreateTimer(&timer_handle, RESET_PULSE);
	svcSetTimer(timer_handle, 0, 1000000000);//1000ms

	while(util_cpu_usage_monitor_init)
	{
		total_cpu_usage = 0;
		div = 0;

		//Update cpu usage every 1000ms.
		svcWaitSynchronization(timer_handle, U64_MAX);

		for(int i = 0; i < 4; i++)
		{
			//Core is not available
			if(!util_cpu_usage_thread_handle[i])
				util_cpu_usage_per_core[i] = NAN;
			else
			{
				//If this flag is not cleared here, it means core usage is kept 100% for a second so that it couldn't reset counter.
				if(util_cpu_usage_reset_counter_request[i])
					cpu_usage_cache = 100;
				else//Estimated CPU usage (%) = (1000 - util_cpu_usage_counter_cache) / 10.0
					cpu_usage_cache = util_cpu_usage_per_core[i] = (1000 - (util_cpu_usage_counter_cache[i] > 1000 ? 1000 : util_cpu_usage_counter_cache[i])) / 10.0;

				if(i == 1)
				{
					if(Util_get_core_1_max() != 0)
						util_cpu_usage_per_core[i] = cpu_usage_cache / (100.0 / Util_get_core_1_max());
					else
						util_cpu_usage_per_core[i] = 0;
				}
				else
					util_cpu_usage_per_core[i] = cpu_usage_cache;

				total_cpu_usage += util_cpu_usage_per_core[i];
				div++;
				util_cpu_usage_reset_counter_request[i] = true;
			}
		}

		if(div != 0)
			util_cpu_usage = total_cpu_usage / div;
	}

	svcCancelTimer(timer_handle);
	svcCloseHandle(timer_handle);

	Util_log_save(DEF_CPU_CALCULATE_THREAD_STR, "Thread exit.");
	threadExit(0);
}

#endif
