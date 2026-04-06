//Includes.
#include "system/util/net_usage.h"

#if DEF_NET_USAGE_API_ENABLE
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

#define NET_USAGE_X				(float)(300)		//X offset for NET usage info in px.
#define NET_USAGE_Y				(float)(115)		//Y offset for NET usage info in px.
#define NET_USAGE_WIDTH			(float)(100)		//Element width for NET usage info in px.
#define NET_USAGE_HEIGHT		(float)(10)			//Element height for NET usage info in px.

#define FONT_SIZE_NET_USAGE		(float)(12.00)		//Font size for NET usage info in px.

//Typedefs.
//N/A.

//Prototypes.
extern ssize_t __real_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern ssize_t __real_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern Result __real_httpcDownloadData(httpcContext *context, u8* buffer, u32 size, u32 *downloadedsize);
ssize_t __wrap_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t __wrap_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
Result __wrap_httpcDownloadData(httpcContext *context, u8* buffer, u32 size, u32 *downloadedsize);
static void Util_net_usage_increment_speed(uint32_t size, bool is_download);
void Util_net_usage_calculate_thread(void* arg);

//Variables.
static bool util_net_usage_init = false;
static bool util_net_usage_show_flag = false;
static uint32_t util_net_usage_download_speed = 0;
static uint32_t util_net_usage_download_speed_cache = 0;
static uint32_t util_net_usage_upload_speed = 0;
static uint32_t util_net_usage_upload_speed_cache = 0;
static Sync_data util_net_usage_mutex = { 0, };
static Thread util_net_usage_thread_handle = 0;
static Handle util_net_usage_timer_handle = 0;

//Code.
uint32_t Util_net_usage_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_net_usage_init)
		goto already_inited;

	util_net_usage_show_flag = false;

	result = Util_sync_create(&util_net_usage_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	util_net_usage_init = true;
	util_net_usage_thread_handle = threadCreate(Util_net_usage_calculate_thread, NULL, STACK_SIZE, DEF_THREAD_SYSTEM_PRIORITY_REALTIME, 0, false);
	if(!util_net_usage_thread_handle)
	{
		DEF_LOG_RESULT(threadCreate, false, DEF_ERR_OTHER);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed:
	Util_sync_destroy(&util_net_usage_mutex);
	util_net_usage_init = false;
	return DEF_ERR_OTHER;

	error_other:
	return DEF_ERR_OTHER;
}

void Util_net_usage_exit(void)
{
	if(!util_net_usage_init)
		return;

	util_net_usage_init = false;
	svcSignalEvent(util_net_usage_timer_handle);
	if(util_net_usage_thread_handle)
	{
		threadJoin(util_net_usage_thread_handle, DEF_THREAD_WAIT_TIME);
		threadFree(util_net_usage_thread_handle);
	}
	Util_sync_destroy(&util_net_usage_mutex);
}

uint32_t Util_net_usage_get_net_download_speed(void)
{
	uint32_t speed = 0;

	if(!util_net_usage_init)
		return 0;

	Util_sync_lock(&util_net_usage_mutex, UINT64_MAX);
	speed = util_net_usage_download_speed;
	Util_sync_unlock(&util_net_usage_mutex);

	return speed;
}

uint32_t Util_net_usage_get_net_upload_speed(void)
{
	uint32_t speed = 0;

	if(!util_net_usage_init)
		return 0;

	Util_sync_lock(&util_net_usage_mutex, UINT64_MAX);
	speed = util_net_usage_upload_speed;
	Util_sync_unlock(&util_net_usage_mutex);

	return speed;
}

bool Util_net_usage_query_show_flag(void)
{
	if(!util_net_usage_init)
		return false;

	return util_net_usage_show_flag;
}

void Util_net_usage_set_show_flag(bool flag)
{
	if(!util_net_usage_init)
		return;

	util_net_usage_show_flag = flag;
}

void Util_net_usage_draw(void)
{
	uint32_t char_length = 0;
	char msg_cache[64] = { 0, };
	Draw_image_data background = Draw_get_empty_image();
	Str_data download_size_string = { 0, };
	Str_data upload_size_string = { 0, };

	if(!util_net_usage_init)
		return;

	Util_format_size(Util_net_usage_get_net_download_speed(), &download_size_string);
	Util_format_size(Util_net_usage_get_net_upload_speed(), &upload_size_string);

	//%f expects double.
	char_length = snprintf(msg_cache, sizeof(msg_cache), "NET:");
	char_length += snprintf((msg_cache + char_length), (sizeof(msg_cache) - char_length), "\nDL: %s/s", DEF_STR_NEVER_NULL(&download_size_string));
	char_length += snprintf((msg_cache + char_length), (sizeof(msg_cache) - char_length), "\nUL: %s/s", DEF_STR_NEVER_NULL(&upload_size_string));

	Util_str_free(&download_size_string);
	Util_str_free(&upload_size_string);

	Draw_with_background_c(msg_cache, NET_USAGE_X, NET_USAGE_Y, FONT_SIZE_NET_USAGE, DEF_DRAW_BLACK, DRAW_X_ALIGN_RIGHT,
	DRAW_Y_ALIGN_TOP, NET_USAGE_WIDTH, NET_USAGE_HEIGHT, DRAW_BACKGROUND_UNDER_TEXT, &background, 0x80FFFFFF);
}

ssize_t __wrap_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
	if(util_net_usage_init)
	{
		int32_t result = __real_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

		if(result > 0)
			Util_net_usage_increment_speed(result, true);

		return result;
	}
	else
		return __real_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

ssize_t __wrap_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
	if(util_net_usage_init)
	{
		int32_t result = __real_sendto(sockfd, buf, len, flags, dest_addr, addrlen);

		if(result > 0)
			Util_net_usage_increment_speed(result, false);

		return result;
	}
	else
		return __real_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

Result __wrap_httpcDownloadData(httpcContext *context, u8* buffer, u32 size, u32 *downloadedsize)
{
	if(util_net_usage_init)
	{
		uint32_t result = __real_httpcDownloadData(context, buffer, size, downloadedsize);

		if((result == DEF_SUCCESS || result == HTTPC_RESULTCODE_DOWNLOADPENDING) && downloadedsize)
			Util_net_usage_increment_speed(*downloadedsize, true);

		return result;
	}
	else
		return __real_httpcDownloadData(context, buffer, size, downloadedsize);
}

static void Util_net_usage_increment_speed(uint32_t size, bool is_download)
{
	Util_sync_lock(&util_net_usage_mutex, UINT64_MAX);
	//Increment bytes downloaded/uploaded.
	if(is_download)
		util_net_usage_download_speed_cache += size;
	else
		util_net_usage_upload_speed_cache += size;

	Util_sync_unlock(&util_net_usage_mutex);
}

void Util_net_usage_calculate_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");

	svcCreateTimer(&util_net_usage_timer_handle, RESET_PULSE);
	svcSetTimer(util_net_usage_timer_handle, 0, DEF_UTIL_MS_TO_NS(1000));

	while(util_net_usage_init)
	{
		//Update NET usage every 1000ms.
		svcWaitSynchronization(util_net_usage_timer_handle, U64_MAX);

		Util_sync_lock(&util_net_usage_mutex, UINT64_MAX);

		//Update DL/UL speed.
		util_net_usage_download_speed = util_net_usage_download_speed_cache;
		util_net_usage_upload_speed = util_net_usage_upload_speed_cache;
		util_net_usage_download_speed_cache = 0;
		util_net_usage_upload_speed_cache = 0;
		Util_sync_unlock(&util_net_usage_mutex);
	}

	svcCancelTimer(util_net_usage_timer_handle);
	svcCloseHandle(util_net_usage_timer_handle);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

#else
#include "3ds.h"

extern ssize_t __real_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern ssize_t __real_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern Result __real_httpcDownloadData(httpcContext *context, u8* buffer, u32 size, u32 *downloadedsize);
ssize_t __wrap_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t __wrap_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
Result __wrap_httpcDownloadData(httpcContext *context, u8* buffer, u32 size, u32 *downloadedsize);

ssize_t __wrap_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) { return __real_recvfrom(sockfd, buf, len, flags, src_addr, addrlen); }
ssize_t __wrap_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) { return __real_sendto(sockfd, buf, len, flags, dest_addr, addrlen); }
Result __wrap_httpcDownloadData(httpcContext *context, u8* buffer, u32 size, u32 *downloadedsize) { return __real_httpcDownloadData(context, buffer, size, downloadedsize); }

#endif //DEF_NET_USAGE_API_ENABLE
