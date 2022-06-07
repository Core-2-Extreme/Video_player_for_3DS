#include "system/headers.hpp"

Result_with_string Util_file_save_to_file(std::string file_name, std::string dir_path, u8* write_data, int size, bool delete_old_file)
{
	u16* utf16_dir_path = NULL;
	u16* utf16_path = NULL;
	u32 written_size = 0;
	u64 offset = 0;
	std::string path = "";
	Handle handle = 0;
	FS_Archive archive = 0;
	Result_with_string result;

	if(file_name == "" || dir_path == "" || !write_data || size <= 0)
		goto invalid_arg;

	path = dir_path + file_name;
	utf16_dir_path = (u16*)malloc(4096);
	utf16_path = (u16*)malloc(4096);
	if(!utf16_dir_path || !utf16_path)
		goto out_of_memory;

	memset(utf16_dir_path, 0x0, 4096);
	memset(utf16_path, 0x0, 4096);
	utf8_to_utf16(utf16_dir_path, (u8*)dir_path.c_str(), 2048);
	utf8_to_utf16(utf16_path, (u8*)path.c_str(), 2048);
	
	result.code = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if(result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenArchive() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSUSER_CreateDirectory(archive, fsMakePath(PATH_UTF16, utf16_dir_path), FS_ATTRIBUTE_DIRECTORY);
	if (result.code != 0 && result.code != 0xC82044BE)//#0xC82044BE directory already exist
	{
		result.error_description = "[Error] FSUSER_CreateDirectory() failed. ";
		goto nintendo_api_failed;
	}

	if (delete_old_file)
		FSUSER_DeleteFile(archive, fsMakePath(PATH_UTF16, utf16_path));

	result.code = FSUSER_CreateFile(archive, fsMakePath(PATH_UTF16, utf16_path), FS_ATTRIBUTE_ARCHIVE, 0);
	if (result.code != 0 && result.code != 0xC82044BE)//#0xC82044BE file already exist
	{
		result.error_description = "[Error] FSUSER_CreateFile() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_WRITE, FS_ATTRIBUTE_ARCHIVE);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenFile() failed. ";
		goto nintendo_api_failed;
	}

	if (!delete_old_file)
	{
		result.code = FSFILE_GetSize(handle, &offset);
		if (result.code != 0)
		{
			result.error_description = "[Error] FSFILE_GetSize() failed. ";
			goto nintendo_api_failed;
		}
	}
	
	result.code = FSFILE_Write(handle, &written_size, offset, write_data, size, FS_WRITE_FLUSH);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSFILE_Write() failed. ";
		goto nintendo_api_failed;
	}

	free(utf16_dir_path);
	free(utf16_path);
	utf16_dir_path = NULL;
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	free(utf16_dir_path);
	free(utf16_path);
	utf16_dir_path = NULL;
	utf16_path = NULL;
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	free(utf16_dir_path);
	free(utf16_path);
	utf16_dir_path = NULL;
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_file_load_from_file(std::string file_name, std::string dir_path, u8** read_data, int max_size)
{
	u32 read_size = 0;
	return Util_file_load_from_file_with_range(file_name, dir_path, read_data, max_size, 0, &read_size);
}

Result_with_string Util_file_load_from_file(std::string file_name, std::string dir_path, u8** read_data, int max_size, u32* read_size)
{
	return Util_file_load_from_file_with_range(file_name, dir_path, read_data, max_size, 0, read_size);
}

Result_with_string Util_file_load_from_file_with_range(std::string file_name, std::string dir_path, u8** read_data, int max_size, u64 read_offset, u32* read_size)
{
	u16* utf16_path = NULL;
	u32 max_read_size = 0;
	u64 file_size = 0;
	std::string path = "";
	Handle handle = 0;
	FS_Archive archive = 0;
	Result_with_string result;

	if(file_name == "" || dir_path == "" || !read_data || max_size <= 0 || read_offset < 0 || !read_size)
		goto invalid_arg;

	path = dir_path + file_name;
	utf16_path = (u16*)malloc(4096);
	if(!utf16_path)
		goto out_of_memory;
	
	memset(utf16_path, 0x0, 4096);
	utf8_to_utf16(utf16_path, (u8*)path.c_str(), 2048);

	result.code = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenArchive() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_READ, FS_ATTRIBUTE_ARCHIVE);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenFile() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSFILE_GetSize(handle, &file_size);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSFILE_GetSize() failed. ";
		goto nintendo_api_failed;
	}

	max_read_size = (file_size - read_offset) > (u64)max_size ? max_size : (file_size - read_offset);
	Util_safe_linear_free(*read_data);
	*read_data = (u8*)Util_safe_linear_alloc(max_read_size + 1);
	if(!*read_data)
		goto out_of_memory;

	memset(*read_data, 0x0, max_read_size + 1);

	result.code = FSFILE_Read(handle, read_size, read_offset, *read_data, max_read_size);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSFILE_Read() failed. ";
		goto nintendo_api_failed;
	}

	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	free(utf16_path);
	Util_safe_linear_free(*read_data);
	utf16_path = NULL;
	*read_data = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	free(utf16_path);
	Util_safe_linear_free(*read_data);
	utf16_path = NULL;
	*read_data = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_file_load_from_rom(std::string file_name, std::string dir_path, u8** read_data, int max_size)
{
	u32 read_size = 0;
	return Util_file_load_from_rom(file_name, dir_path, read_data, max_size, &read_size);
}

Result_with_string Util_file_load_from_rom(std::string file_name, std::string dir_path, u8** read_data, int max_size, u32* read_size)
{
	size_t max_read_size = 0;
	u64 file_size = 0;
	std::string path = "";
	FILE* handle = 0;
	Result_with_string result;

	if(file_name == "" || dir_path == "" || !read_data || max_size <= 0 || !read_size)
		goto invalid_arg;

	path = dir_path + file_name;
	handle = fopen(path.c_str(), "rb");
	if (!handle)
	{
		result.error_description = "[Error] fopen() failed. ";
		goto fopen_failed;
	}

	if(fseek(handle, 0, SEEK_END) != 0)
	{
		result.error_description = "[Error] fseek() failed. ";
		goto fopen_failed;
	}

	file_size = ftell(handle);
	if(file_size <= 0)
	{
		result.error_description = "[Error] ftell() failed. ";
		goto fopen_failed;
	}

	rewind(handle);

	max_read_size = file_size > (u64)max_size ? max_size : file_size;
	Util_safe_linear_free(*read_data);
	*read_data = (u8*)Util_safe_linear_alloc(max_read_size + 1);
	if(!*read_data)
		goto out_of_memory;

	memset(*read_data, 0x0, max_read_size + 1);

	*read_size = fread(*read_data, 1, max_read_size, handle);
	if(*read_size != max_read_size)
	{
		result.error_description = "[Error] fread() failed. ";
		goto fopen_failed;
	}

	fclose(handle);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	fclose(handle);
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	fopen_failed:
	Util_safe_linear_free(*read_data);
	fclose(handle);
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

Result_with_string Util_file_delete_file(std::string file_name, std::string dir_path)
{
	u16* utf16_path = NULL;
	std::string path = "";
	FS_Archive archive = 0;
	Result_with_string result;

	if(file_name == "" || dir_path == "")
		goto invalid_arg;

	path = dir_path + file_name;
	utf16_path = (u16*)malloc(4096);
	if(!utf16_path)
		goto out_of_memory;
	
	memset(utf16_path, 0x0, 4096);
	utf8_to_utf16(utf16_path, (u8*)path.c_str(), 2048);
	
	result.code = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenArchive() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSUSER_DeleteFile(archive, fsMakePath(PATH_UTF16, utf16_path));
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_DeleteFile() failed. ";
		goto nintendo_api_failed;
	}

	free(utf16_path);
	utf16_path = NULL;
	FSUSER_CloseArchive(archive);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	free(utf16_path);
	utf16_path = NULL;
	FSUSER_CloseArchive(archive);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_file_check_file_size(std::string file_name, std::string dir_path, u64* file_size)
{
	u16* utf16_path = NULL;
	std::string path = "";
	Handle handle = 0;
	FS_Archive archive = 0;
	Result_with_string result;

	if(file_name == "" || dir_path == "" || !file_size)
		goto invalid_arg;

	path = dir_path + file_name;
	utf16_path = (u16*)malloc(4096);
	if(!utf16_path)
		goto out_of_memory;
	
	memset(utf16_path, 0x0, 4096);
	utf8_to_utf16(utf16_path, (u8*)path.c_str(), 2048);

	result.code = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenArchive() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_READ, FS_ATTRIBUTE_ARCHIVE);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenFile() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSFILE_GetSize(handle, file_size);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSFILE_GetSize() failed. ";
		goto nintendo_api_failed;
	}

	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_file_check_file_exist(std::string file_name, std::string dir_path)
{
	u16* utf16_path = NULL;
	std::string path = "";
	Handle handle = 0;
	FS_Archive archive = 0;
	Result_with_string result;

	if(file_name == "" || dir_path == "")
		goto invalid_arg;

	path = dir_path + file_name;
	utf16_path = (u16*)malloc(4096);
	if(!utf16_path)
		goto out_of_memory;
	
	memset(utf16_path, 0x0, 4096);
	utf8_to_utf16(utf16_path, (u8*)path.c_str(), 2048);
	
	result.code = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenArchive() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSUSER_OpenFile(&handle, archive, fsMakePath(PATH_UTF16, utf16_path), FS_OPEN_READ, FS_ATTRIBUTE_ARCHIVE);
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenFile() failed. ";
		goto nintendo_api_failed;
	}

	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);

	free(utf16_path);
	utf16_path = NULL;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	free(utf16_path);
	utf16_path = NULL;
	FSFILE_Close(handle);
	FSUSER_CloseArchive(archive);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_file_read_dir(std::string dir_path, int* detected, std::string file_name[], int type[], int array_length)
{
	int count = 0;
	u16* utf16_dir_path = NULL;
	u32 read_entry = 0;
	u32 read_entry_count = 1;
	char* utf8_file_name = NULL;
	FS_DirectoryEntry fs_entry;
	Handle handle = 0;
	FS_Archive archive = 0;
	Result_with_string result;

	if(dir_path == "" || !detected || !file_name || !type || array_length <= 0)
		goto invalid_arg;

	for(int i = 0; i < array_length; i++)
	{
		file_name[i] = "";
		type[i] = 0;
	}
	*detected = 0;

	utf16_dir_path = (u16*)malloc(4096);
	utf8_file_name = (char*)malloc(256);
	if(!utf16_dir_path || !utf8_file_name)
		goto out_of_memory;
	
	memset(utf16_dir_path, 0x0, 4096);
	utf8_to_utf16(utf16_dir_path, (u8*)dir_path.c_str(), 2048);

	result.code = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenArchive() failed. ";
		goto nintendo_api_failed;
	}

	result.code = FSUSER_OpenDirectory(&handle, archive, fsMakePath(PATH_UTF16, utf16_dir_path));
	if (result.code != 0)
	{
		result.error_description = "[Error] FSUSER_OpenDirectory() failed. ";
		goto nintendo_api_failed;
	}

	while (true)
	{
		if(count >= array_length)
			goto out_of_memory;

		result.code = FSDIR_Read(handle, &read_entry, read_entry_count, (FS_DirectoryEntry*)&fs_entry);
		if(result.code != 0)
		{
			result.error_description = "[Error] FSDIR_Read() failed. ";
			goto nintendo_api_failed;
		}

		if (read_entry == 0)
			break;
		
		memset(utf8_file_name, 0x0, 256);
		utf16_to_utf8((u8*)utf8_file_name, fs_entry.name, 256);
		file_name[count] = utf8_file_name;

		type[count] = 0;
		if (fs_entry.attributes & FS_ATTRIBUTE_HIDDEN)
			type[count] |= DEF_FILE_TYPE_HIDDEN;
		if (fs_entry.attributes & FS_ATTRIBUTE_DIRECTORY)
			type[count] |= DEF_FILE_TYPE_DIR;
		if (fs_entry.attributes & FS_ATTRIBUTE_ARCHIVE)
			type[count] |= DEF_FILE_TYPE_FILE;
		if (fs_entry.attributes & FS_ATTRIBUTE_READ_ONLY)
			type[count] |= DEF_FILE_TYPE_READ_ONLY;

		count++;
		*detected = count;
	}

	free(utf8_file_name);
	free(utf16_dir_path);
	utf8_file_name = NULL;
	utf16_dir_path = NULL;
	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	free(utf8_file_name);
	free(utf16_dir_path);
	utf8_file_name = NULL;
	utf16_dir_path = NULL;
	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	free(utf8_file_name);
	free(utf16_dir_path);
	utf8_file_name = NULL;
	utf16_dir_path = NULL;
	FSDIR_Close(handle);
	FSUSER_CloseArchive(archive);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}
