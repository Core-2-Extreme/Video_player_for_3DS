//Includes.
#include "system/util/cpu_usage.h"

#if DEF_CPU_USAGE_API_ENABLE
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"

#include "system/draw/draw.h"
#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/thread_types.h"
#include "system/util/util.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
extern Result __real_APT_SetAppCpuTimeLimit(uint32_t percent);
extern Result __real_APT_GetAppCpuTimeLimit(uint32_t* percent);
Result __wrap_APT_SetAppCpuTimeLimit(uint32_t percent);
Result __wrap_APT_GetAppCpuTimeLimit(uint32_t* percent);
void Util_cpu_usage_counter_thread(void* arg);
void Util_cpu_usage_calculate_thread(void* arg);

//Variables.
static bool util_cpu_usage_init = false;
static bool util_cpu_usage_show_flag = false;
static bool util_cpu_usage_reset_counter_request[4] = { 0, };
static uint8_t util_cpu_usage_core_1_limit = 0;
static uint8_t util_cpu_usage_core_id[4] = { 0, };
static uint16_t util_cpu_usage_counter_cache[4] = { 0, };
static float util_cpu_usage_per_core[4] = { 0, };
static float util_cpu_usage = NAN;
static Thread util_cpu_usage_thread_handle[5] = { 0, };
static Handle util_cpu_usage_timer_handle = 0;

//Code.
uint32_t Util_cpu_usage_init(void)
{
	if(util_cpu_usage_init)
		goto already_inited;

	util_cpu_usage_show_flag = false;
	util_cpu_usage_init = true;
	for(uint8_t i = 0; i < 4; i++)
	{
		util_cpu_usage_core_id[i] = i;
		util_cpu_usage_per_core[i] = NAN;
	}

	for(uint8_t i = 0; i < 4; i++)
	{
		//This may fail depending on core availability.
		util_cpu_usage_thread_handle[i] = threadCreate(Util_cpu_usage_counter_thread, &util_cpu_usage_core_id[i], 2048, DEF_THREAD_SYSTEM_PRIORITY_IDLE, i, false);
	}

	util_cpu_usage_thread_handle[4] = threadCreate(Util_cpu_usage_calculate_thread, NULL, 2048, DEF_THREAD_SYSTEM_PRIORITY_REALTIME, 0, false);
	if(!util_cpu_usage_thread_handle[4])
	{
		DEF_LOG_RESULT(threadCreate, false, DEF_ERR_OTHER);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	nintendo_api_failed:
	util_cpu_usage_init = false;
	return DEF_ERR_OTHER;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;
}

void Util_cpu_usage_exit(void)
{
	if(!util_cpu_usage_init)
		return;

	util_cpu_usage_init = false;
	svcSignalEvent(util_cpu_usage_timer_handle);
	for(uint8_t i = 0; i < 5; i++)
	{
		if(util_cpu_usage_thread_handle[i])
		{
			//Make sure thread can exit as quick as possible.
			svcSetThreadPriority(threadGetHandle(util_cpu_usage_thread_handle[i]), DEF_THREAD_SYSTEM_PRIORITY_REALTIME);

			threadJoin(util_cpu_usage_thread_handle[i], DEF_THREAD_WAIT_TIME);
			threadFree(util_cpu_usage_thread_handle[i]);
		}
	}
}

float Util_cpu_usage_get_cpu_usage(int8_t core_id)
{
	if(!util_cpu_usage_init)
		return NAN;

	if(core_id < -1 || core_id > 3)
		return NAN;

	if(core_id == -1)
		return util_cpu_usage;
	else
		return util_cpu_usage_per_core[core_id];
}

uint8_t Util_cpu_usage_get_core_1_limit(void)
{
	if(!util_cpu_usage_init)
		return 0;

	return util_cpu_usage_core_1_limit;
}

bool Util_cpu_usage_query_show_flag(void)
{
	if(!util_cpu_usage_init)
		return false;

	return util_cpu_usage_show_flag;
}

void Util_cpu_usage_set_show_flag(bool flag)
{
	if(!util_cpu_usage_init)
		return;

	util_cpu_usage_show_flag = flag;
}

void Util_cpu_usage_draw(void)
{
	uint32_t char_length = 0;
	char msg_cache[128] = { 0, };
	Draw_image_data background = Draw_get_empty_image();

	//%f expects double.
	char_length = snprintf(msg_cache, 128, "CPU : %.1f%%", (double)Util_cpu_usage_get_cpu_usage(-1));
	for(uint8_t i = 0; i < 4; i++)//%f expects double.
		char_length += snprintf((msg_cache + char_length), 128 - char_length, "\nCore #%" PRIu8 " : %.1f%%", i, (double)Util_cpu_usage_get_cpu_usage(i));

	snprintf((msg_cache + char_length), 128 - char_length, "\n(#1 max : %" PRIu8 "%%)", Util_cpu_usage_get_core_1_limit());

	Draw_with_background_c(msg_cache, 300, 25, 0.4, 0.4, DEF_DRAW_BLACK, DRAW_X_ALIGN_RIGHT, DRAW_Y_ALIGN_CENTER,
	100, 60, DRAW_BACKGROUND_UNDER_TEXT, &background, 0x80FFFFFF);
}

Result __wrap_APT_SetAppCpuTimeLimit(uint32_t percent)
{
	Result code = __real_APT_SetAppCpuTimeLimit(percent);
	if(code == DEF_SUCCESS)
		util_cpu_usage_core_1_limit = percent;

	return code;
}

Result __wrap_APT_GetAppCpuTimeLimit(uint32_t* percent)
{
	Result code = __real_APT_GetAppCpuTimeLimit(percent);
	if(percent && code == DEF_SUCCESS)
		util_cpu_usage_core_1_limit = *percent;

	return code;
}

void Util_cpu_usage_counter_thread(void* arg)
{
	uint8_t core_id = 0;

	if(!arg || *(uint8_t*)arg > 3)
	{
		DEF_LOG_STRING("Invalid arg!!!!!");
		DEF_LOG_STRING("Thread exit.");
		threadExit(0);
	}

	core_id = *(uint8_t*)arg;
	DEF_LOG_FORMAT("#%" PRIu8 " thread started.", core_id);

	//This thread will run at the lowest priority.
	while(util_cpu_usage_init)
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

	DEF_LOG_FORMAT("#%" PRIu8 " thread exit.", core_id);
	threadExit(0);
}

void Util_cpu_usage_calculate_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");

	svcCreateTimer(&util_cpu_usage_timer_handle, RESET_PULSE);
	svcSetTimer(util_cpu_usage_timer_handle, 0, 1000000000);//1000ms

	while(util_cpu_usage_init)
	{
		uint8_t div = 0;
		float total_cpu_usage = 0;

		//Update CPU usage every 1000ms.
		svcWaitSynchronization(util_cpu_usage_timer_handle, U64_MAX);

		for(uint8_t i = 0; i < 4; i++)
		{
			//Core is not available
			if(!util_cpu_usage_thread_handle[i])
				util_cpu_usage_per_core[i] = NAN;
			else
			{
				float cpu_usage_cache = 0;

				//If this flag is not cleared here, it means core usage is kept 100% for a second so that it couldn't reset counter.
				if(util_cpu_usage_reset_counter_request[i])
					cpu_usage_cache = 100;
				else//Estimated CPU usage (%) = (1000 - util_cpu_usage_counter_cache) / 10.0
					cpu_usage_cache = util_cpu_usage_per_core[i] = (1000 - (util_cpu_usage_counter_cache[i] > 1000 ? 1000 : util_cpu_usage_counter_cache[i])) / 10.0;

				if(i == 1)
				{
					uint8_t core_1_limit = Util_cpu_usage_get_core_1_limit();

					if(core_1_limit != 0)
						util_cpu_usage_per_core[i] = (cpu_usage_cache / (100.0f / core_1_limit));
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

	svcCancelTimer(util_cpu_usage_timer_handle);
	svcCloseHandle(util_cpu_usage_timer_handle);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}
#endif //DEF_CPU_USAGE_API_ENABLE
