//Includes.
#include "system/util/ram_usage.h"

#if DEF_RAM_USAGE_API_ENABLE
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"

#include "system/draw/draw.h"
#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/sync.h"
#include "system/util/thread_types.h"
#include "system/util/util.h"

//Defines.
#define STACK_SIZE				(uint32_t)(2048)	//Stack size for worker threads.

#define RAM_USAGE_X				(float)(300)		//X offset for RAM usage info in px.
#define RAM_USAGE_Y				(float)(150)		//Y offset for RAM usage info in px.
#define RAM_USAGE_WIDTH			(float)(100)		//Element width for RAM usage info in px.
#define RAM_USAGE_HEIGHT		(float)(10)			//Element height for RAM usage info in px.

#define FONT_SIZE_RAM_USAGE		(float)(11.00)		//Font size for RAM usage info in px.

//Typedefs.
//N/A.

//Prototypes.
void Util_ram_usage_calculate_thread(void* arg);

//Variables.
static bool util_ram_usage_init = false;
static bool util_ram_usage_show_flag = false;
static uint32_t util_ram_free_size = 0;
static uint32_t util_ram_free_linear_size = 0;
static uint32_t util_ram_used_size = 0;
static uint32_t util_ram_used_linear_size = 0;
static uint32_t util_ram_total_size = 0;
static uint32_t util_ram_total_linear_size = 0;
static float util_ram_usage = NAN;
static float util_ram_usage_linear = NAN;
static Sync_data util_ram_usage_mutex = { 0, };
static Thread util_ram_usage_thread_handle = 0;
static Handle util_ram_usage_timer_handle = 0;

//Code.
uint32_t Util_ram_usage_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_ram_usage_init)
		goto already_inited;

	util_ram_usage_show_flag = false;

	result = Util_sync_create(&util_ram_usage_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	util_ram_total_size = envGetHeapSize();
	util_ram_total_linear_size = envGetLinearHeapSize();

	util_ram_usage_init = true;
	util_ram_usage_thread_handle = threadCreate(Util_ram_usage_calculate_thread, NULL, STACK_SIZE, DEF_THREAD_SYSTEM_PRIORITY_REALTIME, 0, false);
	if(!util_ram_usage_thread_handle)
	{
		DEF_LOG_RESULT(threadCreate, false, DEF_ERR_OTHER);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed:
	Util_sync_destroy(&util_ram_usage_mutex);
	util_ram_usage_init = false;
	return DEF_ERR_OTHER;

	error_other:
	return result;
}

void Util_ram_usage_exit(void)
{
	if(!util_ram_usage_init)
		return;

	util_ram_usage_init = false;
	svcSignalEvent(util_ram_usage_timer_handle);
	if(util_ram_usage_thread_handle)
	{
		threadJoin(util_ram_usage_thread_handle, DEF_THREAD_WAIT_TIME);
		threadFree(util_ram_usage_thread_handle);
	}
	Util_sync_destroy(&util_ram_usage_mutex);
}

float Util_ram_usage_get_ram_usage(bool is_linear)
{
	float usage = 0;

	if(!util_ram_usage_init)
		return NAN;

	Util_sync_lock(&util_ram_usage_mutex, UINT64_MAX);
	if(is_linear)
		usage = util_ram_usage_linear;
	else
		usage = util_ram_usage;
	Util_sync_unlock(&util_ram_usage_mutex);

	return usage;
}

uint32_t Util_ram_usage_get_free_ram(bool is_linear)
{
	uint32_t size = 0;

	if(!util_ram_usage_init)
		return 0;

	Util_sync_lock(&util_ram_usage_mutex, UINT64_MAX);
	if(is_linear)
		size = util_ram_free_linear_size;
	else
		size = util_ram_free_size;
	Util_sync_unlock(&util_ram_usage_mutex);

	return size;
}

uint32_t Util_ram_usage_get_used_ram(bool is_linear)
{
	uint32_t size = 0;

	if(!util_ram_usage_init)
		return 0;

	Util_sync_lock(&util_ram_usage_mutex, UINT64_MAX);
	if(is_linear)
		size = util_ram_used_linear_size;
	else
		size = util_ram_used_size;
	Util_sync_unlock(&util_ram_usage_mutex);

	return size;
}

uint32_t Util_ram_usage_get_total_ram(bool is_linear)
{
	uint32_t size = 0;

	if(!util_ram_usage_init)
		return 0;

	Util_sync_lock(&util_ram_usage_mutex, UINT64_MAX);
	if(is_linear)
		size = util_ram_total_linear_size;
	else
		size = util_ram_total_size;
	Util_sync_unlock(&util_ram_usage_mutex);

	return size;
}

bool Util_ram_usage_query_show_flag(void)
{
	if(!util_ram_usage_init)
		return false;

	return util_ram_usage_show_flag;
}

void Util_ram_usage_set_show_flag(bool flag)
{
	if(!util_ram_usage_init)
		return;

	util_ram_usage_show_flag = flag;
}

void Util_ram_usage_draw(void)
{
	uint32_t char_length = 0;
	char msg_cache[96] = { 0, };
	Draw_image_data background = Draw_get_empty_image();
	Str_data used_size_string = { 0, };
	Str_data used_linear_size_string = { 0, };
	Str_data total_size_string = { 0, };
	Str_data total_linear_size_string = { 0, };

	if(!util_ram_usage_init)
		return;

	Util_format_size(Util_ram_usage_get_used_ram(false), &used_size_string);
	Util_format_size(Util_ram_usage_get_used_ram(true), &used_linear_size_string);
	Util_format_size(Util_ram_usage_get_total_ram(false), &total_size_string);
	Util_format_size(Util_ram_usage_get_total_ram(true), &total_linear_size_string);

	//%f expects double.
	char_length = snprintf(msg_cache, sizeof(msg_cache), "RAM: %.1f%%", (double)Util_ram_usage_get_ram_usage(false));
	char_length += snprintf((msg_cache + char_length), (sizeof(msg_cache) - char_length), "\n(%s/%s)", DEF_STR_NEVER_NULL(&used_size_string), DEF_STR_NEVER_NULL(&total_size_string));
	char_length += snprintf((msg_cache + char_length), (sizeof(msg_cache) - char_length), "\nLinear RAM: %.1f%%", (double)Util_ram_usage_get_ram_usage(true));
	char_length += snprintf((msg_cache + char_length), (sizeof(msg_cache) - char_length), "\n(%s/%s)", DEF_STR_NEVER_NULL(&used_linear_size_string), DEF_STR_NEVER_NULL(&total_linear_size_string));

	Util_str_free(&used_size_string);
	Util_str_free(&used_linear_size_string);
	Util_str_free(&total_size_string);
	Util_str_free(&total_linear_size_string);

	Draw_with_scale_c(msg_cache, RAM_USAGE_X, RAM_USAGE_Y, FONT_SIZE_RAM_USAGE, DEF_DRAW_NORMAL_SCALE_AND_COMPACT, DEF_DRAW_BLACK,
	DRAW_X_ALIGN_RIGHT, DRAW_Y_ALIGN_TOP, RAM_USAGE_WIDTH, RAM_USAGE_HEIGHT, DRAW_BACKGROUND_UNDER_TEXT, &background, 0x80FFFFFF);
}

void Util_ram_usage_calculate_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");

	svcCreateTimer(&util_ram_usage_timer_handle, RESET_PULSE);
	svcSetTimer(util_ram_usage_timer_handle, 0, DEF_UTIL_MS_TO_NS(1000));

	while(util_ram_usage_init)
	{
		//Update RAM usage every 1000ms.
		svcWaitSynchronization(util_ram_usage_timer_handle, U64_MAX);

		Util_sync_lock(&util_ram_usage_mutex, UINT64_MAX);
		util_ram_free_size = Util_check_free_ram();
		util_ram_free_linear_size = Util_check_free_linear_space();
		util_ram_used_size = (util_ram_total_size - util_ram_free_size);
		util_ram_used_linear_size = (util_ram_total_linear_size - util_ram_free_linear_size);

		//Update usage.
		if(util_ram_total_size != 0)
			util_ram_usage = (((float)util_ram_used_size / util_ram_total_size) * 100);
		if(util_ram_total_linear_size != 0)
			util_ram_usage_linear = (((float)util_ram_used_linear_size / util_ram_total_linear_size) * 100);

		Util_sync_unlock(&util_ram_usage_mutex);
	}

	svcCancelTimer(util_ram_usage_timer_handle);
	svcCloseHandle(util_ram_usage_timer_handle);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

#endif //DEF_RAM_USAGE_API_ENABLE
