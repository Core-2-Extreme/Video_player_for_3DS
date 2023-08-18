#include "definitions.hpp"
#include "system/types.hpp"

#include "system/util/file.hpp"

#include "base64/base64.h"

//Include myself.
#include "system/util/util.hpp"

bool util_safe_linear_alloc_init = false, util_init = false;
int util_draw_num_of_watch_bool = 0, util_draw_num_of_watch_int = 0, util_draw_num_of_watch_double = 0, util_draw_num_of_watch_string = 0;
u32 util_max_core_1 = 0;
LightLock util_safe_linear_alloc_mutex = 1, util_watch_variables_mutex = 1;//Initially unlocked state.
Watch_bool util_draw_watch_bool[DEF_DRAW_MAX_WATCH_BOOL_VARIABLES];
Watch_int util_draw_watch_int[DEF_DRAW_MAX_WATCH_INT_VARIABLES];
Watch_double util_draw_watch_double[DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES];
Watch_string util_draw_watch_string[DEF_DRAW_MAX_WATCH_STRING_VARIABLES];

extern "C" void memcpy_asm(u8*, u8*, int);

extern "C" void* __wrap_malloc(size_t size)
{
	void* ptr = NULL;
	//Alloc memory on linear ram if requested size is greater than 32KB to prevent slow down (linear alloc is slow).
	//If allocation failed, try different memory before giving up.
	if(size > 1024 * 32)
	{
		ptr = Util_safe_linear_alloc(size);
		if(!ptr)
			ptr = __real_malloc(size);
	}
	else
	{
		ptr = __real_malloc(size);
		if(!ptr)
			ptr = Util_safe_linear_alloc(size);
	}
	return ptr;
}

extern "C" void* __wrap_realloc(void* ptr, size_t size)
{
	void* new_ptr[2] = { NULL, NULL, };

	//Alloc memory on linear ram if requested size is greater than 32KB
	//or previous pointer is allocated on linear ram to prevent slow down (linear alloc is slow).
	if(size > 1024 * 32 || (ptr >= (void*)OS_FCRAM_VADDR && ptr <= (void*)(OS_FCRAM_VADDR + OS_FCRAM_SIZE))
		|| (ptr >= (void*)OS_OLD_FCRAM_VADDR && ptr <= (void*)(OS_OLD_FCRAM_VADDR + OS_OLD_FCRAM_SIZE)))
	{
		if(!ptr || (ptr >= (void*)OS_FCRAM_VADDR && ptr <= (void*)(OS_FCRAM_VADDR + OS_FCRAM_SIZE))
		|| (ptr >= (void*)OS_OLD_FCRAM_VADDR && ptr <= (void*)(OS_OLD_FCRAM_VADDR + OS_OLD_FCRAM_SIZE)))
			return Util_safe_linear_realloc(ptr, size);
		else
		{
			//move onto linear ram
			new_ptr[0] = __real_realloc(ptr, size);
			if(new_ptr[0])
			{
				new_ptr[1] = Util_safe_linear_alloc(size);
				if(new_ptr[1])
					memcpy(new_ptr[1], new_ptr[0], size);

				free(new_ptr[0]);
				return new_ptr[1];
			}
			else
				return new_ptr[0];
		}
	}
	else
		return __real_realloc(ptr, size);
}

extern "C" void __wrap_free(void* ptr)
{
	if((ptr >= (void*)OS_FCRAM_VADDR && ptr <= (void*)(OS_FCRAM_VADDR + OS_FCRAM_SIZE))
	|| (ptr >= (void*)OS_OLD_FCRAM_VADDR && ptr <= (void*)(OS_OLD_FCRAM_VADDR + OS_OLD_FCRAM_SIZE)))
		Util_safe_linear_free(ptr);
	else
		__real_free(ptr);
}

extern "C" void __wrap__free_r(struct _reent *r, void* ptr)
{
	if((ptr >= (void*)OS_FCRAM_VADDR && ptr <= (void*)(OS_FCRAM_VADDR + OS_FCRAM_SIZE))
	|| (ptr >= (void*)OS_OLD_FCRAM_VADDR && ptr <= (void*)(OS_OLD_FCRAM_VADDR + OS_OLD_FCRAM_SIZE)))
		Util_safe_linear_free(ptr);
	else
		__real__free_r(r, ptr);
}

extern "C" void* __wrap_memalign(size_t alignment, size_t size)
{
	void* ptr = NULL;
	//Alloc memory on linear ram if requested size is greater than 32KB to prevent slow down (linear alloc is slow).
	//If allocation failed, try different memory before giving up.
	if(size > 1024 * 32)
	{
		ptr = Util_safe_linear_align(alignment, size);
		if(!ptr)
			ptr = __real_memalign(alignment, size);
	}
	else
	{
		ptr = __real_memalign(alignment, size);
		if(!ptr)
			ptr = Util_safe_linear_align(alignment, size);
	}
	return ptr;
}

extern "C" Result __wrap_APT_SetAppCpuTimeLimit(u32 percent)
{
	Result code = __real_APT_SetAppCpuTimeLimit(percent);
	if(code == 0)
		util_max_core_1 = percent;

	return code;
}

extern "C" Result __wrap_APT_GetAppCpuTimeLimit(u32* percent)
{
	Result code = __real_APT_GetAppCpuTimeLimit(percent);
	if(percent && code == 0)
		util_max_core_1 = *percent;

	return code;
}

Result_with_string Util_init(void)
{
	Result_with_string result;

	if(util_init)
		goto already_inited;

	LightLock_Init(&util_watch_variables_mutex);

	util_draw_num_of_watch_bool = 0;
	util_draw_num_of_watch_int = 0;
	util_draw_num_of_watch_double = 0;
	util_draw_num_of_watch_string = 0;
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_BOOL_VARIABLES; i++)
	{
		util_draw_watch_bool[i].address = NULL;
		util_draw_watch_bool[i].previous_value = false;
	}
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_INT_VARIABLES; i++)
	{
		util_draw_watch_int[i].address = NULL;
		util_draw_watch_int[i].previous_value = INT32_MAX;
	}
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES; i++)
	{
		util_draw_watch_double[i].address = NULL;
		util_draw_watch_double[i].previous_value = INT32_MAX;
	}
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_STRING_VARIABLES; i++)
	{
		util_draw_watch_string[i].address = NULL;
		util_draw_watch_string[i].previous_value = "";
	}

	util_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;
}

void Util_exit(void)
{
	if(!util_init)
		return;

	util_init = false;
	util_draw_num_of_watch_bool = 0;
	util_draw_num_of_watch_int = 0;
	util_draw_num_of_watch_double = 0;
	util_draw_num_of_watch_string = 0;
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_BOOL_VARIABLES; i++)
	{
		util_draw_watch_bool[i].address = NULL;
		util_draw_watch_bool[i].previous_value = false;
	}
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_INT_VARIABLES; i++)
	{
		util_draw_watch_int[i].address = NULL;
		util_draw_watch_int[i].previous_value = INT32_MAX;
	}
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES; i++)
	{
		util_draw_watch_double[i].address = NULL;
		util_draw_watch_double[i].previous_value = INTMAX_MAX;
	}
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_STRING_VARIABLES; i++)
	{
		util_draw_watch_string[i].address = NULL;
		util_draw_watch_string[i].previous_value = "";
	}
}

int Util_get_watch_bool_usage(void)
{
	return util_draw_num_of_watch_bool;
}

int Util_get_watch_int_usage(void)
{
	return util_draw_num_of_watch_int;
}

int Util_get_watch_double_usage(void)
{
	return util_draw_num_of_watch_double;
}

int Util_get_watch_string_usage(void)
{
	return util_draw_num_of_watch_string;
}

bool Util_add_watch(bool* variable)
{
	if(!variable)
		return false;

	if(!util_init)
		return false;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_bool + 1 >= DEF_DRAW_MAX_WATCH_BOOL_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return false;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_BOOL_VARIABLES; i++)
	{
		if(!util_draw_watch_bool[i].address)
		{
			util_draw_watch_bool[i].address = variable;
			util_draw_watch_bool[i].previous_value = *variable;
			util_draw_num_of_watch_bool++;
			break;
		}
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return true;
}

bool Util_add_watch(int* variable)
{
	if(!variable)
		return false;

	if(!util_init)
		return false;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_int + 1 >= DEF_DRAW_MAX_WATCH_INT_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return false;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_INT_VARIABLES; i++)
	{
		if(!util_draw_watch_int[i].address)
		{
			util_draw_watch_int[i].address = variable;
			util_draw_watch_int[i].previous_value = *variable;
			util_draw_num_of_watch_int++;
			break;
		}
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return true;
}

bool Util_add_watch(double* variable)
{
	if(!variable)
		return false;

	if(!util_init)
		return false;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_double + 1 >= DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return false;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES; i++)
	{
		if(!util_draw_watch_double[i].address)
		{
			util_draw_watch_double[i].address = variable;
			util_draw_watch_double[i].previous_value = *variable;
			util_draw_num_of_watch_double++;
			break;
		}
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return true;
}

bool Util_add_watch(std::string* variable)
{
	if(!variable)
		return false;

	if(!util_init)
		return false;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_string + 1 >= DEF_DRAW_MAX_WATCH_STRING_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return false;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_STRING_VARIABLES; i++)
	{
		if(!util_draw_watch_string[i].address)
		{
			util_draw_watch_string[i].address = variable;
			util_draw_watch_string[i].previous_value = *variable;
			util_draw_num_of_watch_string++;
			break;
		}
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return true;
}

void Util_remove_watch(int* variable)
{
	int count = 0;
	if(!variable)
		return;

	if(!util_init)
		return;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_int + 1 >= DEF_DRAW_MAX_WATCH_INT_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_INT_VARIABLES; i++)
	{
		if(util_draw_watch_int[i].address == variable)
		{
			util_draw_watch_int[i].address = NULL;
			util_draw_watch_int[i].previous_value = INT32_MAX;
			util_draw_num_of_watch_int--;
			break;
		}
		else if(util_draw_watch_int[i].address)
			count++;

		if(count >= util_draw_num_of_watch_int)
			break;
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return;
}

void Util_remove_watch(bool* variable)
{
	int count = 0;
	if(!variable)
		return;

	if(!util_init)
		return;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_bool + 1 >= DEF_DRAW_MAX_WATCH_BOOL_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_BOOL_VARIABLES; i++)
	{
		if(util_draw_watch_bool[i].address == variable)
		{
			util_draw_watch_bool[i].address = NULL;
			util_draw_watch_bool[i].previous_value = INTMAX_MAX;
			util_draw_num_of_watch_bool--;
			break;
		}
		else if(util_draw_watch_bool[i].address)
			count++;

		if(count >= util_draw_num_of_watch_bool)
			break;
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return;
}

void Util_remove_watch(double* variable)
{
	int count = 0;
	if(!variable)
		return;

	if(!util_init)
		return;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_double + 1 >= DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES; i++)
	{
		if(util_draw_watch_double[i].address == variable)
		{
			util_draw_watch_double[i].address = NULL;
			util_draw_watch_double[i].previous_value = INTMAX_MAX;
			util_draw_num_of_watch_double--;
			break;
		}
		else if(util_draw_watch_double[i].address)
			count++;

		if(count >= util_draw_num_of_watch_double)
			break;
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return;
}

void Util_remove_watch(std::string* variable)
{
	int count = 0;
	if(!variable)
		return;

	if(!util_init)
		return;

	LightLock_Lock(&util_watch_variables_mutex);
	if(util_draw_num_of_watch_string + 1 >= DEF_DRAW_MAX_WATCH_STRING_VARIABLES)
	{
		LightLock_Unlock(&util_watch_variables_mutex);
		return;
	}

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_STRING_VARIABLES; i++)
	{
		if(util_draw_watch_string[i].address == variable)
		{
			util_draw_watch_string[i].address = NULL;
			util_draw_watch_string[i].previous_value = "";
			util_draw_num_of_watch_string--;
			break;
		}
		else if(util_draw_watch_string[i].address)
			count++;

		if(count >= util_draw_num_of_watch_string)
			break;
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return;
}

bool Util_is_watch_changed(void)
{
	bool changed = false;
	int count = 0;

	if(!util_init)
		return false;

	LightLock_Lock(&util_watch_variables_mutex);

	for(int i = 0; i < DEF_DRAW_MAX_WATCH_BOOL_VARIABLES; i++)
	{
		if(util_draw_watch_bool[i].address)
		{
			if(util_draw_watch_bool[i].previous_value != *util_draw_watch_bool[i].address)
			{
				util_draw_watch_bool[i].previous_value = *util_draw_watch_bool[i].address;
				changed = true;
			}
			count++;
		}

		if(count >= util_draw_num_of_watch_bool)
			break;
	}

	count = 0;
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_INT_VARIABLES; i++)
	{
		if(util_draw_watch_int[i].address)
		{
			if(util_draw_watch_int[i].previous_value != *util_draw_watch_int[i].address)
			{
				util_draw_watch_int[i].previous_value = *util_draw_watch_int[i].address;
				changed = true;
			}
			count++;
		}

		if(count >= util_draw_num_of_watch_int)
			break;
	}

	count = 0;
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES; i++)
	{
		if(util_draw_watch_double[i].address)
		{
			if(util_draw_watch_double[i].previous_value != *util_draw_watch_double[i].address)
			{
				util_draw_watch_double[i].previous_value = *util_draw_watch_double[i].address;
				changed = true;
			}
			count++;
		}

		if(count >= util_draw_num_of_watch_double)
			break;
	}

	count = 0;
	for(int i = 0; i < DEF_DRAW_MAX_WATCH_STRING_VARIABLES; i++)
	{
		if(util_draw_watch_string[i].address)
		{
			if(util_draw_watch_string[i].previous_value.length() != util_draw_watch_string[i].address->length()
			|| util_draw_watch_string[i].previous_value != *util_draw_watch_string[i].address)
			{
				util_draw_watch_string[i].previous_value = *util_draw_watch_string[i].address;
				changed = true;
			}
			count++;
		}

		if(count >= util_draw_num_of_watch_string)
			break;
	}

	LightLock_Unlock(&util_watch_variables_mutex);
	return changed;
}

Result_with_string Util_parse_file(std::string source_data, int expected_items, std::string out_data[])
{
	size_t start_num = 0;
	size_t end_num = 0;
	std::string start_text = "";
	std::string end_text = "";
	Result_with_string result;

	if(!out_data || expected_items <= 0)
		goto invalid_arg;

	for (int i = 0; i < expected_items; i++)
	{
		start_text = "<" + std::to_string(i) + ">";
		start_num = source_data.find(start_text);
		end_text = "</" + std::to_string(i) + ">";
		end_num = source_data.find(end_text);

		if (end_num == std::string::npos || start_num == std::string::npos)
		{
			result.error_description = "[Error] Failed to parse file. error pos : " + std::to_string(i) + " ";
			goto other;
		}

		start_num += start_text.length();
		end_num -= start_num;
		out_data[i] = source_data.substr(start_num, end_num);
	}

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	other:
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

std::string Util_convert_seconds_to_time(double input_seconds)
{
	int hours = 0;
	int minutes = 0;
	int seconds = 0;
	std::string time = "";

	if(std::isnan(input_seconds) || std::isinf(input_seconds))
		input_seconds = 0;

	hours = (int)input_seconds / 3600;
	minutes = ((int)input_seconds % 3600) / 60;
	seconds = ((int)input_seconds % 60);

	if(hours < 10)
		time += "0" + std::to_string(hours) + ":";
	else
		time += std::to_string(hours) + ":";

	if(minutes < 10)
		time += "0" + std::to_string(minutes) + ":";
	else
		time += std::to_string(minutes) + ":";

	if(seconds < 10)
		time += "0" + std::to_string(seconds);
	else
		time += std::to_string(seconds);

	time += std::to_string(input_seconds - (int)input_seconds).substr(1, 2);
	return time;
}

std::string Util_encode_to_escape(std::string in_data)
{
	int string_length = in_data.length();
	std::string check = "";
	std::string return_data = "";

	for(int i = 0; i < string_length; i++)
	{
		check = in_data.substr(i, 1);
		if(check == "\n")
			return_data += "\\n";
		else if(check == "\u0022")
			return_data += "\\\u0022";
		else if(check == "\u005c")
			return_data += "\\\u005c";
		else
			return_data += in_data.substr(i, 1);
	}

	return return_data;
}

Result_with_string Util_load_msg(std::string file_name, std::string out_msg[], int num_of_msg)
{
	u8* fs_buffer = NULL;
	u32 read_size = 0;
	Result_with_string result;
	fs_buffer = NULL;

	if(file_name == "" || !out_msg || num_of_msg <= 0)
		goto invalid_arg;

	result = Util_file_load_from_rom(file_name, "romfs:/gfx/msg/", &fs_buffer, 0x2000, &read_size);
	if (result.code != 0)
		goto api_failed;

	result = Util_parse_file((char*)fs_buffer, num_of_msg, out_msg);
	if (result.code != 0)
		goto api_failed;

	Util_safe_linear_free(fs_buffer);
	fs_buffer = NULL;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	api_failed:
	Util_safe_linear_free(fs_buffer);
	fs_buffer = NULL;
	return result;
}

std::string Util_encode_to_base64(char* source, int size)
{
	return base64_encode((const unsigned char*)source, size);
}

std::string Util_decode_from_base64(std::string source)
{
	return base64_decode(source);
}

Result_with_string Util_safe_linear_alloc_init(void)
{
	Result_with_string result;
	if(util_safe_linear_alloc_init)
		goto already_inited;

	LightLock_Init(&util_safe_linear_alloc_mutex);

	util_safe_linear_alloc_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;
}

void Util_safe_linear_alloc_exit(void)
{
	if(!util_safe_linear_alloc_init)
		return;

	util_safe_linear_alloc_init = false;
}

void* Util_safe_linear_alloc(size_t size)
{
	void* pointer = NULL;
	if(!util_safe_linear_alloc_init)
		return NULL;

	LightLock_Lock(&util_safe_linear_alloc_mutex);
	pointer = linearAlloc(size);
	LightLock_Unlock(&util_safe_linear_alloc_mutex);

	return pointer;
}

void* Util_safe_linear_align(size_t alignment, size_t size)
{
	void* pointer = NULL;
	if(!util_safe_linear_alloc_init)
		return NULL;

	LightLock_Lock(&util_safe_linear_alloc_mutex);
	pointer = linearMemAlign(size, alignment);
	LightLock_Unlock(&util_safe_linear_alloc_mutex);

	return pointer;
}

void* __attribute__((optimize("O0"))) Util_safe_linear_realloc(void* pointer, size_t size)
{
	void* new_ptr = NULL;
	u32 pointer_size = 0;
	if(!util_safe_linear_alloc_init)
		return NULL;

	if(size == 0)
	{
		Util_safe_linear_free(pointer);
		return pointer;
	}
	if(!pointer)
		return Util_safe_linear_alloc(size);

	new_ptr = Util_safe_linear_alloc(size);
	if(new_ptr)
	{
		LightLock_Lock(&util_safe_linear_alloc_mutex);
		pointer_size = linearGetSize(pointer);
		LightLock_Unlock(&util_safe_linear_alloc_mutex);

		if(size > pointer_size)
			memcpy_asm((u8*)new_ptr, (u8*)pointer, pointer_size);
		else
			memcpy_asm((u8*)new_ptr, (u8*)pointer, size);

		Util_safe_linear_free(pointer);
	}
	return new_ptr;
}

void Util_safe_linear_free(void* pointer)
{
	if(!util_safe_linear_alloc_init)
		return;

	LightLock_Lock(&util_safe_linear_alloc_mutex);
	linearFree(pointer);
	LightLock_Unlock(&util_safe_linear_alloc_mutex);
}

u32 Util_check_free_linear_space(void)
{
	u32 space = 0;
	if(!util_safe_linear_alloc_init)
		return 0;

	LightLock_Lock(&util_safe_linear_alloc_mutex);
	space = linearSpaceFree();
	LightLock_Unlock(&util_safe_linear_alloc_mutex);
	return space;
}

u32 Util_check_free_ram(void)
{
	u8* malloc_check[2000];
	u32 count;

	for (int i = 0; i < 2000; i++)
		malloc_check[i] = NULL;

	for (count = 0; count < 2000; count++)
	{
		malloc_check[count] = (u8*)__real_malloc(0x186A0);// 100KB
		if (malloc_check[count] == NULL)
			break;
	}

	for (u32 i = 0; i <= count; i++)
		__real_free(malloc_check[i]);

	return count * 100 * 1024;//return free B
}

u32 Util_get_core_1_max(void)
{
	return util_max_core_1;
}

void Util_sleep(s64 us)
{
	svcSleepThread(us * 1000);
}

long Util_min(long value_0, long value_1)
{
	return (value_0 > value_1 ? value_1 : value_0);
}

long Util_max(long value_0, long value_1)
{
	return (value_0 > value_1 ? value_0 : value_1);
}

bool Util_return_bool(bool value)
{
	return value;
}

int Util_return_int(int value)
{
	return value;
}

double Util_return_double(double value)
{
	return value;
}

std::string Util_return_string(std::string string)
{
	return string;
}

Result_with_string Util_return_result_with_string(Result_with_string value)
{
	return value;
}
