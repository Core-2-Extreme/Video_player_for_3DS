//Includes.
#include "system/util/file.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/expl.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/util.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
static uint32_t Util_file_make_path(const char* file_name, const char* dir_path, uint16_t** utf16_path, uint16_t** utf16_dir_path);

//Variables.
//N/A.

//Code.
uint32_t Util_file_save_to_file(const char* file_name, const char* dir_path, const uint8_t* write_data, uint32_t size, bool delete_old_file)
{
	uint16_t* utf16_dir_path = NULL;
	uint16_t* utf16_path = NULL;
	uint32_t written_size = 0;
	uint32_t result = DEF_ERR_OTHER;
	uint64_t offset = 0;
	Handle handle = 0;
	FS_Archive archive = 0;

	if(!file_name || !dir_path || !write_data || size == 0)
		goto invalid_arg;

	result = Util_file_make_path(file_name, dir_path, &utf16_path, &utf16_dir_path);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_file_make_path, false, result);
		goto error_other;
	}

	result = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenArchive, false, result);
		goto nintendo_api_failed;
	}

	result = FSUSER_CreateDirectory(archive, fsMakePath(PATH_UTF16, utf16_dir_path), FS_ATTRIBUTE_DIRECTORY);
	if(result != DEF_SUCCESS && result != 0xC82044BE)//0xC82044BE means directory already exist.
	{
		DEF_LOG_RESULT(FSUSER_CreateDirectory, false, result);
		goto nintendo_api_failed;
	}

	if (delete_old_file)
		FSUSER_DeleteFile(archive, fsMakePath(PATH_UTF16, utf16_path));

	result = FSUSER_CreateFile(archive, fsMakePath(PATH_UTF16, utf16_path), FS_ATTRIBUTE_ARCHIVE, 0);
	if (result != DEF_SUCCESS && result != 0xC82044BE)//0xC82044BE means file already exist.
	{
		DEF_LOG_RESULT(FSUSER_CreateFile, false, result);
		goto nintendo_api_failed;
	}

	result = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_WRITE, FS_ATTRIBUTE_ARCHIVE);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenFile, false, result);
		goto nintendo_api_failed;
	}

	if (!delete_old_file)
	{
		result = FSFILE_GetSize(handle, &offset);
		if (result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(FSFILE_GetSize, false, result);
			goto nintendo_api_failed;
		}
	}

	result = FSFILE_Write(handle, &written_size, offset, write_data, size, FS_WRITE_FLUSH);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSFILE_Write, false, result);
		goto nintendo_api_failed;
	}

	free(utf16_dir_path);
	free(utf16_path);
	utf16_dir_path = NULL;
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error_other:
	return result;

	nintendo_api_failed:
	free(utf16_dir_path);
	free(utf16_path);
	utf16_dir_path = NULL;
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return result;
}

uint32_t Util_file_load_from_file(const char* file_name, const char* dir_path, uint8_t** read_data, uint32_t max_size, uint64_t read_offset, uint32_t* read_size)
{
	uint16_t* utf16_path = NULL;
	uint32_t max_read_size = 0;
	uint32_t result = DEF_ERR_OTHER;
	uint64_t file_size = 0;
	Handle handle = 0;
	FS_Archive archive = 0;

	if(!file_name || !dir_path || !read_data || max_size == 0 || !read_size)
		goto invalid_arg;

	result = Util_file_make_path(file_name, dir_path, &utf16_path, NULL);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_file_make_path, false, result);
		goto error_other;
	}

	result = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenArchive, false, result);
		goto nintendo_api_failed;
	}

	result = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_READ, FS_ATTRIBUTE_ARCHIVE);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenFile, false, result);
		goto nintendo_api_failed;
	}

	result = FSFILE_GetSize(handle, &file_size);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSFILE_GetSize, false, result);
		goto nintendo_api_failed;
	}

	max_read_size = (((file_size - read_offset) > (uint64_t)max_size) ? max_size : (file_size - read_offset));
	free(*read_data);
	*read_data = (uint8_t*)linearAlloc(max_read_size + 1);
	if(!*read_data)
		goto out_of_memory;

	result = FSFILE_Read(handle, read_size, read_offset, *read_data, max_read_size);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSFILE_Read, false, result);
		goto nintendo_api_failed;
	}

	(*read_data)[*read_size] = 0x00;//Add a null terminator.

	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error_other:
	return result;

	out_of_memory:
	free(utf16_path);
	free(*read_data);
	utf16_path = NULL;
	*read_data = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed:
	free(utf16_path);
	free(*read_data);
	utf16_path = NULL;
	*read_data = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return result;
}

uint32_t Util_file_load_from_rom(const char* file_name, const char* dir_path, uint8_t** read_data, uint32_t max_size, uint32_t* read_size)
{
	uint32_t result = DEF_ERR_OTHER;
	uint64_t file_size = 0;
	size_t max_read_size = 0;
	FILE* handle = 0;
	Str_data path = { 0, };

	if(!file_name || !dir_path || !read_data || max_size == 0 || !read_size)
		goto invalid_arg;

	result = Util_str_init(&path);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_format(&path, "%s%s", dir_path, file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_format, false, result);
		goto error;
	}

	handle = fopen(path.buffer, "rb");
	Util_str_free(&path);
	if (!handle)
	{
		DEF_LOG_RESULT(fopen, false, DEF_ERR_OTHER);
		goto io_failed;
	}

	if(fseek(handle, 0, SEEK_END) != 0)
	{
		DEF_LOG_RESULT(fseek, false, DEF_ERR_OTHER);
		goto io_failed;
	}

	file_size = ftell(handle);
	if(file_size == 0)
	{
		DEF_LOG_RESULT(ftell, false, DEF_ERR_OTHER);
		goto io_failed;
	}

	rewind(handle);

	max_read_size = ((file_size > (uint64_t)max_size) ? max_size : file_size);
	free(*read_data);
	*read_data = (uint8_t*)linearAlloc(max_read_size + 1);
	if(!*read_data)
		goto out_of_memory;

	*read_size = fread(*read_data, 1, max_read_size, handle);
	if(*read_size != max_read_size)
	{
		DEF_LOG_RESULT(fread, false, DEF_ERR_OTHER);
		goto io_failed;
	}

	(*read_data)[*read_size] = 0x00;//Add a null terminator.
	fclose(handle);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	fclose(handle);
	return DEF_ERR_OUT_OF_MEMORY;

	error:
	Util_str_free(&path);
	return result;

	io_failed:
	free(*read_data);
	*read_data = NULL;
	if(handle)
		fclose(handle);

	return DEF_ERR_OTHER;
}

uint32_t Util_file_delete_file(const char* file_name, const char* dir_path)
{
	uint16_t* utf16_path = NULL;
	uint32_t result = DEF_ERR_OTHER;
	FS_Archive archive = 0;

	if(!file_name || !dir_path)
		goto invalid_arg;

	result = Util_file_make_path(file_name, dir_path, &utf16_path, NULL);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_file_make_path, false, result);
		goto error_other;
	}

	result = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenArchive, false, result);
		goto nintendo_api_failed;
	}

	result = FSUSER_DeleteFile(archive, fsMakePath(PATH_UTF16, utf16_path));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_DeleteFile, false, result);
		goto nintendo_api_failed;
	}

	free(utf16_path);
	utf16_path = NULL;
	FSUSER_CloseArchive(archive);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error_other:
	return result;

	nintendo_api_failed:
	free(utf16_path);
	utf16_path = NULL;
	FSUSER_CloseArchive(archive);
	return result;
}

uint32_t Util_file_check_file_size(const char* file_name, const char* dir_path, uint64_t* file_size)
{
	uint16_t* utf16_path = NULL;
	uint32_t result = DEF_ERR_OTHER;
	Handle handle = 0;
	FS_Archive archive = 0;

	if(!file_name || !dir_path || !file_size)
		goto invalid_arg;

	result = Util_file_make_path(file_name, dir_path, &utf16_path, NULL);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_file_make_path, false, result);
		goto error_other;
	}

	result = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenArchive, false, result);
		goto nintendo_api_failed;
	}

	result = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_READ, FS_ATTRIBUTE_ARCHIVE);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenFile, false, result);
		goto nintendo_api_failed;
	}

	result = FSFILE_GetSize(handle, file_size);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSFILE_GetSize, false, result);
		goto nintendo_api_failed;
	}

	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error_other:
	return result;

	nintendo_api_failed:
	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return result;
}

uint32_t Util_file_check_file_exist(const char* file_name, const char* dir_path)
{
	uint16_t* utf16_path = NULL;
	uint32_t result = DEF_ERR_OTHER;
	Handle handle = 0;
	FS_Archive archive = 0;

	if(!file_name || !dir_path)
		goto invalid_arg;

	result = Util_file_make_path(file_name, dir_path, &utf16_path, NULL);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_file_make_path, false, result);
		goto error_other;
	}

	result = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenArchive, false, result);
		goto nintendo_api_failed;
	}

	result = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_READ, FS_ATTRIBUTE_ARCHIVE);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenFile, false, result);
		goto nintendo_api_failed;
	}

	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	free(utf16_path);
	utf16_path = NULL;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error_other:
	return result;

	nintendo_api_failed:
	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	return result;
}

uint32_t Util_file_read_dir(const char* dir_path, uint32_t* detected, Str_data* file_name, Expl_file_type* type, uint32_t array_length)
{
	uint16_t* utf16_dir_path = NULL;
	uint32_t count = 0;
	uint32_t read_entry = 0;
	uint32_t read_entry_count = 1;
	uint32_t result = DEF_ERR_OTHER;
	char* utf8_file_name = NULL;
	ssize_t utf_out_size = 0;
	FS_DirectoryEntry fs_entry = { 0,};
	Handle handle = 0;
	FS_Archive archive = 0;

	if(!dir_path || !detected || !file_name || !type || array_length == 0)
		goto invalid_arg;

	for(uint32_t i = 0; i < array_length; i++)
	{
		if(Util_str_init(&file_name[i]) != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto out_of_memory;
		}

		type[i] = EXPL_FILE_TYPE_NONE;
	}
	*detected = 0;

	utf16_dir_path = (uint16_t*)malloc(4096 + 2);
	utf8_file_name = (char*)malloc(256 + 1);
	if(!utf16_dir_path || !utf8_file_name)
		goto out_of_memory;

	utf_out_size = utf8_to_utf16(utf16_dir_path, (const uint8_t*)dir_path, 2048);
	utf16_dir_path[(utf_out_size < 0 ? 0 : utf_out_size)] = 0x00;//Add a null terminator.

	result = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenArchive, false, result);
		goto nintendo_api_failed;
	}

	result = FSUSER_OpenDirectory(&handle, archive, fsMakePath(PATH_UTF16, utf16_dir_path));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(FSUSER_OpenDirectory, false, result);
		goto nintendo_api_failed;
	}

	while (true)
	{
		if(count >= array_length)
			goto out_of_memory;

		result = FSDIR_Read(handle, &read_entry, read_entry_count, &fs_entry);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(FSDIR_Read, false, result);
			goto nintendo_api_failed;
		}

		if (read_entry == 0)
			break;

		utf_out_size = utf16_to_utf8((uint8_t*)utf8_file_name, fs_entry.name, 256);
		utf8_file_name[(utf_out_size < 0 ? 0 : utf_out_size)] = 0x00;//Add a null terminator.

		if(Util_str_set(&file_name[count], utf8_file_name) != DEF_SUCCESS)
			goto out_of_memory;

		if (fs_entry.attributes & FS_ATTRIBUTE_HIDDEN)
			type[count] = (type[count] | EXPL_FILE_TYPE_HIDDEN);
		if (fs_entry.attributes & FS_ATTRIBUTE_DIRECTORY)
			type[count] = (type[count] | EXPL_FILE_TYPE_DIR);
		if (fs_entry.attributes & FS_ATTRIBUTE_ARCHIVE)
			type[count] = (type[count] | EXPL_FILE_TYPE_FILE);
		if (fs_entry.attributes & FS_ATTRIBUTE_READ_ONLY)
			type[count] = (type[count] | EXPL_FILE_TYPE_READ_ONLY);

		count++;
		*detected = count;
	}

	free(utf8_file_name);
	free(utf16_dir_path);
	utf8_file_name = NULL;
	utf16_dir_path = NULL;
	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	free(utf8_file_name);
	free(utf16_dir_path);
	utf8_file_name = NULL;
	utf16_dir_path = NULL;
	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	for(uint32_t i = 0; i < array_length; i++)
		Util_str_free(&file_name[i]);

	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed:
	free(utf8_file_name);
	free(utf16_dir_path);
	utf8_file_name = NULL;
	utf16_dir_path = NULL;
	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	for(uint32_t i = 0; i < array_length; i++)
		Util_str_free(&file_name[i]);

	return result;
}

static uint32_t Util_file_make_path(const char* file_name, const char* dir_path, uint16_t** utf16_path, uint16_t** utf16_dir_path)
{
	uint32_t result = DEF_ERR_OTHER;
	ssize_t utf_out_size = 0;
	Str_data path = { 0, };

	result = Util_str_init(&path);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_format(&path, "%s%s", dir_path, file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_format, false, result);
		goto error;
	}

	if(utf16_dir_path)
	{
		uint32_t dir_length = strlen(dir_path);

		*utf16_dir_path = (uint16_t*)malloc((dir_length + 1) * sizeof(uint16_t));
		if(!*utf16_dir_path)
		{
			result = DEF_ERR_OUT_OF_MEMORY;
			goto error;
		}

		utf_out_size = utf8_to_utf16(*utf16_dir_path, (const uint8_t*)dir_path, dir_length);
		(*utf16_dir_path)[(utf_out_size < 0 ? 0 : utf_out_size)] = 0x00;//Add a NULL terminator.
	}
	if(utf16_path)
	{
		*utf16_path = (uint16_t*)malloc((path.length + 1) * sizeof(uint16_t));
		if(!*utf16_path)
		{
			result = DEF_ERR_OUT_OF_MEMORY;
			goto error;
		}

		utf_out_size = utf8_to_utf16(*utf16_path, (uint8_t*)path.buffer, path.length);
		(*utf16_path)[(utf_out_size < 0 ? 0 : utf_out_size)] = 0x00;//Add a NULL terminator.
	}

	Util_str_free(&path);
	return DEF_SUCCESS;

	error:
	Util_str_free(&path);
	if(utf16_path)
	{
		free(*utf16_path);
		*utf16_path = NULL;
	}
	if(utf16_dir_path)
	{
		free(*utf16_dir_path);
		*utf16_dir_path = NULL;
	}
	return result;
}
