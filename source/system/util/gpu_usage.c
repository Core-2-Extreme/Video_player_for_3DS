//Includes.
#include "system/util/gpu_usage.h"

#if DEF_GPU_USAGE_API_ENABLE
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"
#include "citro3d.h"

#include "system/draw/draw.h"
#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/sync.h"
#include "system/util/thread_types.h"
#include "system/util/util.h"

//Defines.
#define STACK_SIZE				(uint32_t)(2048)	//Stack size for worker threads.

#define GPU_USAGE_X				(float)(300)		//X offset for GPU usage info in px.
#define GPU_USAGE_Y				(float)(77.5)		//Y offset for GPU usage info in px.
#define GPU_USAGE_WIDTH			(float)(100)		//Element width for GPU usage info in px.
#define GPU_USAGE_HEIGHT		(float)(10)			//Element height for GPU usage info in px.

#define FONT_SIZE_GPU_USAGE		(float)(11.00)		//Font size for GPU usage info in px.

//Typedefs.
//N/A.

//Prototypes.
extern void __real_C3D_FrameEnd(u8 flags);
extern Result __real_GSPGPU_FlushDataCache(const void* adr, u32 size);
extern Result __real_GSPGPU_InvalidateDataCache(const void* adr, u32 size);
void __wrap_C3D_FrameEnd(u8 flags);
Result __wrap_GSPGPU_FlushDataCache(const void* adr, u32 size);
Result __wrap_GSPGPU_InvalidateDataCache(const void* adr, u32 size);
static void Util_gpu_usage_increment_pending_count(void);
static void Util_gpu_usage_decrement_pending_count_and_increment_speed(void);
void Util_gpu_usage_calculate_thread(void* arg);

//Variables.
static bool util_gpu_usage_init = false;
static bool util_gpu_usage_show_flag = false;
static uint32_t util_gpu_pending_count = 0;
static uint64_t util_gpu_usage_busy_start_ts = 0;
static float util_gpu_usage = NAN;
static double util_gpu_usage_ms_cache = 0;
static Sync_data util_gpu_usage_mutex = { 0, };
static Thread util_gpu_usage_thread_handle = 0;
static Handle util_gpu_usage_timer_handle = 0;

//Code.
uint32_t Util_gpu_usage_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_gpu_usage_init)
		goto already_inited;

	util_gpu_usage_show_flag = false;

	result = Util_sync_create(&util_gpu_usage_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	util_gpu_pending_count = 0;
	util_gpu_usage_init = true;
	util_gpu_usage_thread_handle = threadCreate(Util_gpu_usage_calculate_thread, NULL, STACK_SIZE, DEF_THREAD_SYSTEM_PRIORITY_REALTIME, 0, false);
	if(!util_gpu_usage_thread_handle)
	{
		DEF_LOG_RESULT(threadCreate, false, DEF_ERR_OTHER);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed:
	Util_sync_destroy(&util_gpu_usage_mutex);
	util_gpu_usage_init = false;
	return DEF_ERR_OTHER;

	error_other:
	return DEF_ERR_OTHER;
}

void Util_gpu_usage_exit(void)
{
	if(!util_gpu_usage_init)
		return;

	util_gpu_usage_init = false;
	svcSignalEvent(util_gpu_usage_timer_handle);
	if(util_gpu_usage_thread_handle)
	{
		threadJoin(util_gpu_usage_thread_handle, DEF_THREAD_WAIT_TIME);
		threadFree(util_gpu_usage_thread_handle);
	}
	Util_sync_destroy(&util_gpu_usage_mutex);
}

float Util_gpu_usage_get_gpu_usage(void)
{
	float usage = 0;

	if(!util_gpu_usage_init)
		return NAN;

	Util_sync_lock(&util_gpu_usage_mutex, UINT64_MAX);
	usage = util_gpu_usage;
	Util_sync_unlock(&util_gpu_usage_mutex);

	return usage;
}

bool Util_gpu_usage_query_show_flag(void)
{
	if(!util_gpu_usage_init)
		return false;

	return util_gpu_usage_show_flag;
}

void Util_gpu_usage_set_show_flag(bool flag)
{
	if(!util_gpu_usage_init)
		return;

	util_gpu_usage_show_flag = flag;
}

void Util_gpu_usage_draw(void)
{
	char msg_cache[32] = { 0, };
	Draw_image_data background = Draw_get_empty_image();

	if(!util_gpu_usage_init)
		return;

	//%f expects double.
	snprintf(msg_cache, sizeof(msg_cache), "GPU: %.1f%%", (double)Util_gpu_usage_get_gpu_usage());

	Draw_with_scale_c(msg_cache, GPU_USAGE_X, GPU_USAGE_Y, FONT_SIZE_GPU_USAGE, DEF_DRAW_NORMAL_SCALE_AND_COMPACT, DEF_DRAW_BLACK,
	DRAW_X_ALIGN_RIGHT, DRAW_Y_ALIGN_TOP, GPU_USAGE_WIDTH, GPU_USAGE_HEIGHT, DRAW_BACKGROUND_UNDER_TEXT, &background, 0x80FFFFFF);
}

void __wrap_C3D_FrameEnd(u8 flags)
{
	if(util_gpu_usage_init)
	{
		Util_sync_lock(&util_gpu_usage_mutex, UINT64_MAX);
		if(util_gpu_pending_count == 0)
			util_gpu_usage_busy_start_ts = osGetTime();

		Util_sync_unlock(&util_gpu_usage_mutex);

		__real_C3D_FrameEnd(flags);

		Util_sync_lock(&util_gpu_usage_mutex, UINT64_MAX);
		//Increment time consumed.
		util_gpu_usage_ms_cache += (double)C3D_GetDrawingTime();
		//Reset busy timestamp if pending count is 0.
		if(util_gpu_pending_count == 0)
			util_gpu_usage_busy_start_ts = 0;

		Util_sync_unlock(&util_gpu_usage_mutex);
	}
	else
		__real_C3D_FrameEnd(flags);
}

Result __wrap_GSPGPU_FlushDataCache(const void* adr, u32 size)
{
	if(util_gpu_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_gpu_usage_increment_pending_count();
		result = __real_GSPGPU_FlushDataCache(adr, size);
		Util_gpu_usage_decrement_pending_count_and_increment_speed();

		return result;
	}
	else
		return __real_GSPGPU_FlushDataCache(adr, size);
}

Result __wrap_GSPGPU_InvalidateDataCache(const void* adr, u32 size)
{
	if(util_gpu_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_gpu_usage_increment_pending_count();
		result = __real_GSPGPU_InvalidateDataCache(adr, size);
		Util_gpu_usage_decrement_pending_count_and_increment_speed();

		return result;
	}
	else
		return __real_GSPGPU_InvalidateDataCache(adr, size);
}

static void Util_gpu_usage_increment_pending_count(void)
{
	Util_sync_lock(&util_gpu_usage_mutex, UINT64_MAX);
	if(util_gpu_pending_count == 0)
		util_gpu_usage_busy_start_ts = osGetTime();

	util_gpu_pending_count++;
	Util_sync_unlock(&util_gpu_usage_mutex);
}

static void Util_gpu_usage_decrement_pending_count_and_increment_speed(void)
{
	Util_sync_lock(&util_gpu_usage_mutex, UINT64_MAX);
	util_gpu_pending_count--;

	//Increment time consumed if pending count is 0.
	if(util_gpu_pending_count == 0)
	{
		util_gpu_usage_ms_cache += Util_get_diff(osGetTime(), util_gpu_usage_busy_start_ts, UINT64_MAX);
		util_gpu_usage_busy_start_ts = 0;
	}
	Util_sync_unlock(&util_gpu_usage_mutex);
}

void Util_gpu_usage_calculate_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");

	svcCreateTimer(&util_gpu_usage_timer_handle, RESET_PULSE);
	svcSetTimer(util_gpu_usage_timer_handle, 0, DEF_UTIL_MS_TO_NS(1000));

	while(util_gpu_usage_init)
	{
		uint64_t busy_start_ts = 0;
		double busy_time_ms = 0;

		//Update GPU usage every 1000ms.
		svcWaitSynchronization(util_gpu_usage_timer_handle, U64_MAX);

		Util_sync_lock(&util_gpu_usage_mutex, UINT64_MAX);
		busy_start_ts = util_gpu_usage_busy_start_ts;
		busy_time_ms = util_gpu_usage_ms_cache;

		if(busy_start_ts != 0)//GPU is currently busy, add time consumed since it started processing.
			busy_time_ms += (osGetTime() - busy_start_ts);

		busy_time_ms = Util_min_d(busy_time_ms, 1000);

		util_gpu_usage = ((busy_time_ms / 1000) * 100);
		util_gpu_usage_ms_cache = Util_max_d((util_gpu_usage_ms_cache - 1000), 0);
		Util_sync_unlock(&util_gpu_usage_mutex);
	}

	svcCancelTimer(util_gpu_usage_timer_handle);
	svcCloseHandle(util_gpu_usage_timer_handle);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

#else
#include "3ds.h"
#include "citro3d.h"

extern void __real_C3D_FrameEnd(u8 flags);
extern Result __real_GSPGPU_FlushDataCache(const void* adr, u32 size);
extern Result __real_GSPGPU_InvalidateDataCache(const void* adr, u32 size);
void __wrap_C3D_FrameEnd(u8 flags);
Result __wrap_GSPGPU_FlushDataCache(const void* adr, u32 size);
Result __wrap_GSPGPU_InvalidateDataCache(const void* adr, u32 size);

void __wrap_C3D_FrameEnd(u8 flags) { __real_C3D_FrameEnd(flags); }
Result __wrap_GSPGPU_FlushDataCache(const void* adr, u32 size) { return __real_GSPGPU_FlushDataCache(adr, size); }
Result __wrap_GSPGPU_InvalidateDataCache(const void* adr, u32 size) { return __real_GSPGPU_InvalidateDataCache(adr, size); }

#endif //DEF_GPU_USAGE_API_ENABLE
