//Includes.
#include "system/util/nvs_usage.h"

#if DEF_NVS_USAGE_API_ENABLE
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

#define NVS_USAGE_X				(float)(300)		//X offset for NVS usage info in px.
#define NVS_USAGE_Y				(float)(155)		//Y offset for NVS usage info in px.
#define NVS_USAGE_WIDTH			(float)(100)		//Element width for NVS usage info in px.
#define NVS_USAGE_HEIGHT		(float)(10)			//Element height for NVS usage info in px.

#define FONT_SIZE_NVS_USAGE		(float)(12.00)		//Font size for NVS usage info in px.

//Typedefs.
//N/A.

//Prototypes.
extern Result __real_FSUSER_CreateFile(FS_Archive archive, FS_Path path, u32 attributes, u64 fileSize);
extern Result __real_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes);
extern Result __real_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes);
extern Result __real_FSUSER_OpenFileDirectly(Handle* out, FS_ArchiveID archiveId, FS_Path archivePath, FS_Path filePath, u32 openFlags, u32 attributes);
extern Result __real_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path);
extern Result __real_FSUSER_OpenArchive(FS_Archive* archive, FS_ArchiveID id, FS_Path path);
extern Result __real_FSFILE_OpenSubFile(Handle handle, Handle* subFile, u64 offset, u64 size);
extern Result __real_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
extern Result __real_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
extern Result __real_FSUSER_DeleteFile(FS_Archive archive, FS_Path path);
extern Result __real_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path);
extern Result __real_FSUSER_DeleteDirectoryRecursively(FS_Archive archive, FS_Path path);
extern Result __real_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size);
extern Result __real_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries);
extern Result __real_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags);
extern Result __real_FSFILE_GetSize(Handle handle, u64* size);
extern Result __real_FSFILE_SetSize(Handle handle, u64 size);
extern Result __real_FSFILE_GetAttributes(Handle handle, u32* attributes);
extern Result __real_FSFILE_SetAttributes(Handle handle, u32 attributes);
extern Result __real_FSFILE_SetPriority(Handle handle, u32 priority);
extern Result __real_FSFILE_GetPriority(Handle handle, u32* priority);
extern Result __real_FSFILE_Flush(Handle handle);
extern Result __real_FSUSER_CloseArchive(FS_Archive archive);
extern Result __real_FSFILE_Close(Handle handle);
extern Result __real_FSDIR_Close(Handle handle);
Result __wrap_FSUSER_CreateFile(FS_Archive archive, FS_Path path, u32 attributes, u64 fileSize);
Result __wrap_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes);
Result __wrap_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes);
Result __wrap_FSUSER_OpenFileDirectly(Handle* out, FS_ArchiveID archiveId, FS_Path archivePath, FS_Path filePath, u32 openFlags, u32 attributes);
Result __wrap_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path);
Result __wrap_FSUSER_OpenArchive(FS_Archive* archive, FS_ArchiveID id, FS_Path path);
Result __wrap_FSFILE_OpenSubFile(Handle handle, Handle* subFile, u64 offset, u64 size);
Result __wrap_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
Result __wrap_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
Result __wrap_FSUSER_DeleteFile(FS_Archive archive, FS_Path path);
Result __wrap_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path);
Result __wrap_FSUSER_DeleteDirectoryRecursively(FS_Archive archive, FS_Path path);
Result __wrap_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size);
Result __wrap_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries);
Result __wrap_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags);
Result __wrap_FSFILE_GetSize(Handle handle, u64* size);
Result __wrap_FSFILE_SetSize(Handle handle, u64 size);
Result __wrap_FSFILE_GetAttributes(Handle handle, u32* attributes);
Result __wrap_FSFILE_SetAttributes(Handle handle, u32 attributes);
Result __wrap_FSFILE_SetPriority(Handle handle, u32 priority);
Result __wrap_FSFILE_GetPriority(Handle handle, u32* priority);
Result __wrap_FSFILE_Flush(Handle handle);
Result __wrap_FSUSER_CloseArchive(FS_Archive archive);
Result __wrap_FSFILE_Close(Handle handle);
Result __wrap_FSDIR_Close(Handle handle);
static void Util_nvs_usage_increment_pending_count(void);
static void Util_nvs_usage_decrement_pending_count_and_increment_speed(uint32_t result, uint32_t* size, bool is_read);
void Util_nvs_usage_calculate_thread(void* arg);

//Variables.
static bool util_nvs_usage_init = false;
static bool util_nvs_usage_show_flag = false;
static uint32_t util_nvs_usage_read_speed = 0;
static uint32_t util_nvs_usage_read_speed_cache = 0;
static uint32_t util_nvs_usage_write_speed = 0;
static uint32_t util_nvs_usage_write_speed_cache = 0;
static uint32_t util_nvs_pending_count = 0;
static uint64_t util_nvs_usage_busy_start_ts = 0;
static float util_nvs_usage = NAN;
static double util_nvs_usage_ms_cache = 0;
static Sync_data util_nvs_usage_mutex = { 0, };
static Thread util_nvs_usage_thread_handle = 0;
static Handle util_nvs_usage_timer_handle = 0;

//Code.
uint32_t Util_nvs_usage_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_nvs_usage_init)
		goto already_inited;

	util_nvs_usage_show_flag = false;

	result = Util_sync_create(&util_nvs_usage_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	util_nvs_pending_count = 0;
	util_nvs_usage_init = true;
	util_nvs_usage_thread_handle = threadCreate(Util_nvs_usage_calculate_thread, NULL, STACK_SIZE, DEF_THREAD_SYSTEM_PRIORITY_REALTIME, 0, false);
	if(!util_nvs_usage_thread_handle)
	{
		DEF_LOG_RESULT(threadCreate, false, DEF_ERR_OTHER);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed:
	Util_sync_destroy(&util_nvs_usage_mutex);
	util_nvs_usage_init = false;
	return DEF_ERR_OTHER;

	error_other:
	return DEF_ERR_OTHER;
}

void Util_nvs_usage_exit(void)
{
	if(!util_nvs_usage_init)
		return;

	util_nvs_usage_init = false;
	svcSignalEvent(util_nvs_usage_timer_handle);
	if(util_nvs_usage_thread_handle)
	{
		threadJoin(util_nvs_usage_thread_handle, DEF_THREAD_WAIT_TIME);
		threadFree(util_nvs_usage_thread_handle);
	}
	Util_sync_destroy(&util_nvs_usage_mutex);
}

float Util_nvs_usage_get_nvs_usage(void)
{
	float usage = 0;

	if(!util_nvs_usage_init)
		return NAN;

	Util_sync_lock(&util_nvs_usage_mutex, UINT64_MAX);
	usage = util_nvs_usage;
	Util_sync_unlock(&util_nvs_usage_mutex);

	return usage;
}

uint32_t Util_nvs_usage_get_nvs_read_speed(void)
{
	uint32_t speed = 0;

	if(!util_nvs_usage_init)
		return 0;

	Util_sync_lock(&util_nvs_usage_mutex, UINT64_MAX);
	speed = util_nvs_usage_read_speed;
	Util_sync_unlock(&util_nvs_usage_mutex);

	return speed;
}

uint32_t Util_nvs_usage_get_nvs_write_speed(void)
{
	uint32_t speed = 0;

	if(!util_nvs_usage_init)
		return 0;

	Util_sync_lock(&util_nvs_usage_mutex, UINT64_MAX);
	speed = util_nvs_usage_write_speed;
	Util_sync_unlock(&util_nvs_usage_mutex);

	return speed;
}

bool Util_nvs_usage_query_show_flag(void)
{
	if(!util_nvs_usage_init)
		return false;

	return util_nvs_usage_show_flag;
}

void Util_nvs_usage_set_show_flag(bool flag)
{
	if(!util_nvs_usage_init)
		return;

	util_nvs_usage_show_flag = flag;
}

void Util_nvs_usage_draw(void)
{
	uint32_t char_length = 0;
	char msg_cache[64] = { 0, };
	Draw_image_data background = Draw_get_empty_image();
	Str_data read_size_string = { 0, };
	Str_data write_size_string = { 0, };

	if(!util_nvs_usage_init)
		return;

	Util_format_size(Util_nvs_usage_get_nvs_read_speed(), &read_size_string);
	Util_format_size(Util_nvs_usage_get_nvs_write_speed(), &write_size_string);

	//%f expects double.
	char_length = snprintf(msg_cache, sizeof(msg_cache), "NVS: %.1f%%", (double)Util_nvs_usage_get_nvs_usage());
	char_length += snprintf((msg_cache + char_length), (sizeof(msg_cache) - char_length), "\nR: %s/s", DEF_STR_NEVER_NULL(&read_size_string));
	char_length += snprintf((msg_cache + char_length), (sizeof(msg_cache) - char_length), "\nW: %s/s", DEF_STR_NEVER_NULL(&write_size_string));

	Util_str_free(&read_size_string);
	Util_str_free(&write_size_string);

	Draw_with_background_c(msg_cache, NVS_USAGE_X, NVS_USAGE_Y, FONT_SIZE_NVS_USAGE, DEF_DRAW_BLACK, DRAW_X_ALIGN_RIGHT,
	DRAW_Y_ALIGN_TOP, NVS_USAGE_WIDTH, NVS_USAGE_HEIGHT, DRAW_BACKGROUND_UNDER_TEXT, &background, 0x80FFFFFF);
}

Result __wrap_FSUSER_CreateFile(FS_Archive archive, FS_Path path, u32 attributes, u64 fileSize)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_CreateFile(archive, path, attributes, fileSize);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_CreateFile(archive, path, attributes, fileSize);
}

Result __wrap_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_CreateDirectory(archive, path, attributes);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_CreateDirectory(archive, path, attributes);
}

Result __wrap_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_OpenFile(out, archive, path, openFlags, attributes);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_OpenFile(out, archive, path, openFlags, attributes);
}

Result __wrap_FSUSER_OpenFileDirectly(Handle* out, FS_ArchiveID archiveId, FS_Path archivePath, FS_Path filePath, u32 openFlags, u32 attributes)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_OpenFileDirectly(out, archiveId, archivePath, filePath, openFlags, attributes);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_OpenFileDirectly(out, archiveId, archivePath, filePath, openFlags, attributes);
}

Result __wrap_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_OpenDirectory(out, archive, path);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_OpenDirectory(out, archive, path);
}

Result __wrap_FSUSER_OpenArchive(FS_Archive* archive, FS_ArchiveID id, FS_Path path)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_OpenArchive(archive, id, path);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_OpenArchive(archive, id, path);
}

Result __wrap_FSFILE_OpenSubFile(Handle handle, Handle* subFile, u64 offset, u64 size)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_OpenSubFile(handle, subFile, offset, size);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSFILE_OpenSubFile(handle, subFile, offset, size);
}

Result __wrap_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_RenameFile(srcArchive, srcPath, dstArchive, dstPath);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_RenameFile(srcArchive, srcPath, dstArchive, dstPath);
}

Result __wrap_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_RenameDirectory(srcArchive, srcPath, dstArchive, dstPath);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_RenameDirectory(srcArchive, srcPath, dstArchive, dstPath);
}

Result __wrap_FSUSER_DeleteFile(FS_Archive archive, FS_Path path)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_DeleteFile(archive, path);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_DeleteFile(archive, path);
}

Result __wrap_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_DeleteDirectory(archive, path);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_DeleteDirectory(archive, path);
}

Result __wrap_FSUSER_DeleteDirectoryRecursively(FS_Archive archive, FS_Path path)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_DeleteDirectoryRecursively(archive, path);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_DeleteDirectoryRecursively(archive, path);
}

Result __wrap_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_SUCCESS;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_Read(handle, bytesRead, offset, buffer, size);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, bytesRead, true);

		return result;
	}
	else
		return __real_FSFILE_Read(handle, bytesRead, offset, buffer, size);
}

Result __wrap_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;
		uint32_t read_size = 0;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSDIR_Read(handle, entriesRead, entryCount, entries);

		if(entriesRead && result == DEF_SUCCESS)
		{
			//Calculate size read from SD.
			for(uint32_t i = 0; i < *entriesRead; i++)
			{
				read_size += utf16_to_utf8(NULL, entries[i].name, 0);
				read_size += sizeof(uint32_t) + sizeof(uint64_t);//attributes, fileSize.
			}
		}

		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, &read_size, true);

		return result;
	}
	else
		return __real_FSDIR_Read(handle, entriesRead, entryCount, entries);
}

Result __wrap_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_SUCCESS;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_Write(handle, bytesWritten, offset, buffer, size, flags);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, bytesWritten, false);

		return result;
	}
	else
		return __real_FSFILE_Write(handle, bytesWritten, offset, buffer, size, flags);
}

Result __wrap_FSFILE_GetSize(Handle handle, u64* size)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;
		uint32_t read_size = sizeof(uint64_t);//size.

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_GetSize(handle, size);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, &read_size, true);

		return result;
	}
	else
		return __real_FSFILE_GetSize(handle, size);
}

Result __wrap_FSFILE_SetSize(Handle handle, u64 size)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;
		uint32_t written_size = sizeof(uint64_t);//size.

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_SetSize(handle, size);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, &written_size, false);

		return result;
	}
	else
		return __real_FSFILE_SetSize(handle, size);
}

Result __wrap_FSFILE_GetAttributes(Handle handle, u32* attributes)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;
		uint32_t read_size = sizeof(uint32_t);//attributes.

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_GetAttributes(handle, attributes);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, &read_size, true);

		return result;
	}
	else
		return __real_FSFILE_GetAttributes(handle, attributes);
}

Result __wrap_FSFILE_SetAttributes(Handle handle, u32 attributes)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;
		uint32_t written_size = sizeof(uint32_t);//attributes.

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_SetAttributes(handle, attributes);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, &written_size, false);

		return result;
	}
	else
		return __real_FSFILE_SetAttributes(handle, attributes);
}

Result __wrap_FSFILE_GetPriority(Handle handle, u32* priority)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;
		uint32_t read_size = sizeof(uint32_t);//priority.

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_GetPriority(handle, priority);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, &read_size, true);

		return result;
	}
	else
		return __real_FSFILE_GetPriority(handle, priority);
}

Result __wrap_FSFILE_SetPriority(Handle handle, u32 priority)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;
		uint32_t written_size = sizeof(uint32_t);//priority.

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_SetPriority(handle, priority);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, &written_size, false);

		return result;
	}
	else
		return __real_FSFILE_SetPriority(handle, priority);
}

Result __wrap_FSFILE_Flush(Handle handle)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_Flush(handle);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSFILE_Flush(handle);
}

Result __wrap_FSUSER_CloseArchive(FS_Archive archive)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSUSER_CloseArchive(archive);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSUSER_CloseArchive(archive);
}

Result __wrap_FSFILE_Close(Handle handle)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSFILE_Close(handle);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSFILE_Close(handle);
}

Result __wrap_FSDIR_Close(Handle handle)
{
	if(util_nvs_usage_init)
	{
		uint32_t result = DEF_ERR_OTHER;

		Util_nvs_usage_increment_pending_count();
		result = __real_FSDIR_Close(handle);
		Util_nvs_usage_decrement_pending_count_and_increment_speed(result, NULL, false);

		return result;
	}
	else
		return __real_FSDIR_Close(handle);
}

static void Util_nvs_usage_increment_pending_count(void)
{
	Util_sync_lock(&util_nvs_usage_mutex, UINT64_MAX);
	if(util_nvs_pending_count == 0)
		util_nvs_usage_busy_start_ts = osGetTime();

	util_nvs_pending_count++;
	Util_sync_unlock(&util_nvs_usage_mutex);
}

static void Util_nvs_usage_decrement_pending_count_and_increment_speed(uint32_t result, uint32_t* size, bool is_read)
{
	Util_sync_lock(&util_nvs_usage_mutex, UINT64_MAX);
	util_nvs_pending_count--;
	//Only increment bytes read/written if it was successful.
	if(result == DEF_SUCCESS && size)
	{
		if(is_read)
			util_nvs_usage_read_speed_cache += *size;
		else
			util_nvs_usage_write_speed_cache += *size;
	}

	//Increment time consumed if pending count is 0.
	if(util_nvs_pending_count == 0)
	{
		util_nvs_usage_ms_cache += Util_get_diff(osGetTime(), util_nvs_usage_busy_start_ts, UINT64_MAX);
		util_nvs_usage_busy_start_ts = 0;
	}
	Util_sync_unlock(&util_nvs_usage_mutex);
}

void Util_nvs_usage_calculate_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");

	svcCreateTimer(&util_nvs_usage_timer_handle, RESET_PULSE);
	svcSetTimer(util_nvs_usage_timer_handle, 0, DEF_UTIL_MS_TO_NS(1000));

	while(util_nvs_usage_init)
	{
		uint64_t busy_start_ts = 0;
		double busy_time_ms = 0;

		//Update NVS usage every 1000ms.
		svcWaitSynchronization(util_nvs_usage_timer_handle, U64_MAX);

		Util_sync_lock(&util_nvs_usage_mutex, UINT64_MAX);
		busy_start_ts = util_nvs_usage_busy_start_ts;
		busy_time_ms = util_nvs_usage_ms_cache;

		if(busy_start_ts != 0)//NVS is currently busy, add time consumed since it started processing.
			busy_time_ms += (osGetTime() - busy_start_ts);

		busy_time_ms = Util_min_d(busy_time_ms, 1000);

		util_nvs_usage = ((busy_time_ms / 1000) * 100);
		util_nvs_usage_ms_cache = Util_max_d((util_nvs_usage_ms_cache - 1000), 0);

		//Update RW speed.
		util_nvs_usage_read_speed = util_nvs_usage_read_speed_cache;
		util_nvs_usage_write_speed = util_nvs_usage_write_speed_cache;
		util_nvs_usage_read_speed_cache = 0;
		util_nvs_usage_write_speed_cache = 0;
		Util_sync_unlock(&util_nvs_usage_mutex);
	}

	svcCancelTimer(util_nvs_usage_timer_handle);
	svcCloseHandle(util_nvs_usage_timer_handle);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

#else
#include "3ds.h"

extern Result __real_FSUSER_CreateFile(FS_Archive archive, FS_Path path, u32 attributes, u64 fileSize);
extern Result __real_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes);
extern Result __real_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes);
extern Result __real_FSUSER_OpenFileDirectly(Handle* out, FS_ArchiveID archiveId, FS_Path archivePath, FS_Path filePath, u32 openFlags, u32 attributes);
extern Result __real_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path);
extern Result __real_FSUSER_OpenArchive(FS_Archive* archive, FS_ArchiveID id, FS_Path path);
extern Result __real_FSFILE_OpenSubFile(Handle handle, Handle* subFile, u64 offset, u64 size);
extern Result __real_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
extern Result __real_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
extern Result __real_FSUSER_DeleteFile(FS_Archive archive, FS_Path path);
extern Result __real_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path);
extern Result __real_FSUSER_DeleteDirectoryRecursively(FS_Archive archive, FS_Path path);
extern Result __real_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size);
extern Result __real_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries);
extern Result __real_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags);
extern Result __real_FSFILE_GetSize(Handle handle, u64* size);
extern Result __real_FSFILE_SetSize(Handle handle, u64 size);
extern Result __real_FSFILE_GetAttributes(Handle handle, u32* attributes);
extern Result __real_FSFILE_SetAttributes(Handle handle, u32 attributes);
extern Result __real_FSFILE_SetPriority(Handle handle, u32 priority);
extern Result __real_FSFILE_GetPriority(Handle handle, u32* priority);
extern Result __real_FSFILE_Flush(Handle handle);
extern Result __real_FSUSER_CloseArchive(FS_Archive archive);
extern Result __real_FSFILE_Close(Handle handle);
extern Result __real_FSDIR_Close(Handle handle);
Result __wrap_FSUSER_CreateFile(FS_Archive archive, FS_Path path, u32 attributes, u64 fileSize);
Result __wrap_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes);
Result __wrap_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes);
Result __wrap_FSUSER_OpenFileDirectly(Handle* out, FS_ArchiveID archiveId, FS_Path archivePath, FS_Path filePath, u32 openFlags, u32 attributes);
Result __wrap_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path);
Result __wrap_FSUSER_OpenArchive(FS_Archive* archive, FS_ArchiveID id, FS_Path path);
Result __wrap_FSFILE_OpenSubFile(Handle handle, Handle* subFile, u64 offset, u64 size);
Result __wrap_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
Result __wrap_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath);
Result __wrap_FSUSER_DeleteFile(FS_Archive archive, FS_Path path);
Result __wrap_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path);
Result __wrap_FSUSER_DeleteDirectoryRecursively(FS_Archive archive, FS_Path path);
Result __wrap_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size);
Result __wrap_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries);
Result __wrap_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags);
Result __wrap_FSFILE_GetSize(Handle handle, u64* size);
Result __wrap_FSFILE_SetSize(Handle handle, u64 size);
Result __wrap_FSFILE_GetAttributes(Handle handle, u32* attributes);
Result __wrap_FSFILE_SetAttributes(Handle handle, u32 attributes);
Result __wrap_FSFILE_SetPriority(Handle handle, u32 priority);
Result __wrap_FSFILE_GetPriority(Handle handle, u32* priority);
Result __wrap_FSFILE_Flush(Handle handle);
Result __wrap_FSUSER_CloseArchive(FS_Archive archive);
Result __wrap_FSFILE_Close(Handle handle);
Result __wrap_FSDIR_Close(Handle handle);

Result __wrap_FSUSER_CreateFile(FS_Archive archive, FS_Path path, u32 attributes, u64 fileSize) { return __real_FSUSER_CreateFile(archive, path, attributes, fileSize); }
Result __wrap_FSUSER_CreateDirectory(FS_Archive archive, FS_Path path, u32 attributes) { return __real_FSUSER_CreateDirectory(archive, path, attributes); }
Result __wrap_FSUSER_OpenFile(Handle* out, FS_Archive archive, FS_Path path, u32 openFlags, u32 attributes) { return __real_FSUSER_OpenFile(out, archive, path, openFlags, attributes); }
Result __wrap_FSUSER_OpenFileDirectly(Handle* out, FS_ArchiveID archiveId, FS_Path archivePath, FS_Path filePath, u32 openFlags, u32 attributes) { return __real_FSUSER_OpenFileDirectly(out, archiveId, archivePath, filePath, openFlags, attributes); }
Result __wrap_FSUSER_OpenDirectory(Handle *out, FS_Archive archive, FS_Path path) { return __real_FSUSER_OpenDirectory(out, archive, path); }
Result __wrap_FSUSER_OpenArchive(FS_Archive* archive, FS_ArchiveID id, FS_Path path) { return __real_FSUSER_OpenArchive(archive, id, path); }
Result __wrap_FSFILE_OpenSubFile(Handle handle, Handle* subFile, u64 offset, u64 size) { return __real_FSFILE_OpenSubFile(handle, subFile, offset, size); }
Result __wrap_FSUSER_RenameFile(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath) { return __real_FSUSER_RenameFile(srcArchive, srcPath, dstArchive, dstPath); }
Result __wrap_FSUSER_RenameDirectory(FS_Archive srcArchive, FS_Path srcPath, FS_Archive dstArchive, FS_Path dstPath) { return __real_FSUSER_RenameDirectory(srcArchive, srcPath, dstArchive, dstPath); }
Result __wrap_FSUSER_DeleteFile(FS_Archive archive, FS_Path path) { return __real_FSUSER_DeleteFile(archive, path); }
Result __wrap_FSUSER_DeleteDirectory(FS_Archive archive, FS_Path path) { return __real_FSUSER_DeleteDirectory(archive, path); }
Result __wrap_FSUSER_DeleteDirectoryRecursively(FS_Archive archive, FS_Path path) { return __real_FSUSER_DeleteDirectoryRecursively(archive, path); }
Result __wrap_FSFILE_Read(Handle handle, u32* bytesRead, u64 offset, void* buffer, u32 size) { return __real_FSFILE_Read(handle, bytesRead, offset, buffer, size); }
Result __wrap_FSDIR_Read(Handle handle, u32* entriesRead, u32 entryCount, FS_DirectoryEntry* entries) { return __real_FSDIR_Read(handle, entriesRead, entryCount, entries); }
Result __wrap_FSFILE_Write(Handle handle, u32* bytesWritten, u64 offset, const void* buffer, u32 size, u32 flags) { return __real_FSFILE_Write(handle, bytesWritten, offset, buffer, size, flags); }
Result __wrap_FSFILE_GetSize(Handle handle, u64* size) { return __real_FSFILE_GetSize(handle, size); }
Result __wrap_FSFILE_SetSize(Handle handle, u64 size) { return __real_FSFILE_SetSize(handle, size); }
Result __wrap_FSFILE_GetAttributes(Handle handle, u32* attributes) { return __real_FSFILE_GetAttributes(handle, attributes); }
Result __wrap_FSFILE_SetAttributes(Handle handle, u32 attributes) { return __real_FSFILE_SetAttributes(handle, attributes); }
Result __wrap_FSFILE_GetPriority(Handle handle, u32* priority) { return __real_FSFILE_GetPriority(handle, priority); }
Result __wrap_FSFILE_SetPriority(Handle handle, u32 priority) { return __real_FSFILE_SetPriority(handle, priority); }
Result __wrap_FSFILE_Flush(Handle handle) { return __real_FSFILE_Flush(handle); }
Result __wrap_FSUSER_CloseArchive(FS_Archive archive) { return __real_FSUSER_CloseArchive(archive); }
Result __wrap_FSFILE_Close(Handle handle) { return __real_FSFILE_Close(handle); }
Result __wrap_FSDIR_Close(Handle handle) { return __real_FSDIR_Close(handle); }

#endif //DEF_NVS_USAGE_API_ENABLE
