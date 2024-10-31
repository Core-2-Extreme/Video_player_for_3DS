//Includes.
#include "system/util/util.h"

#include <malloc.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mbedtls/base64.h"
#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/file.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/thread_types.h"

//Defines.
#define DEF_UTIL_LINEAR_THRESHOLD_SIZE		(uint32_t)(1000 * 32)
#define DEF_UTIL_IS_LINEAR_RAM(ptr)			(bool)((ptr >= (void*)OS_FCRAM_VADDR && ptr <= (void*)(OS_FCRAM_VADDR + OS_FCRAM_SIZE)) \
											|| (ptr >= (void*)OS_OLD_FCRAM_VADDR && ptr <= (void*)(OS_OLD_FCRAM_VADDR + OS_OLD_FCRAM_SIZE)))

//Typedefs.
//N/A.

//Prototypes.
extern void memcpy_asm(uint8_t*, uint8_t*, uint32_t);
extern void* __real_malloc(size_t size);
extern void* __real_calloc(size_t items, size_t size);
extern void* __real_realloc(void* ptr, size_t size);
extern void __real_free(void* ptr);
extern void __real__free_r(struct _reent* r, void* ptr);
extern void* __real_memalign(size_t alignment, size_t size);
extern void* __real_linearAlloc(size_t size);
extern void* __real_linearMemAlign(size_t size, size_t alignment);
extern void* __real_linearRealloc(void* mem, size_t size);
extern size_t __real_linearGetSize(void* mem);
extern void __real_linearFree(void* mem);
extern uint32_t __real_linearSpaceFree(void);
static inline bool Util_is_heap_low(void);
static void* Util_realloc_heap_to_linear(void* ptr, size_t size);
static void* Util_realloc_linear_to_heap(void* ptr, size_t size);
static void* memalign_heap_only(size_t align, size_t size);
static void* malloc_heap_only(size_t size);
void* __wrap_malloc(size_t size);
void* __wrap_calloc(size_t items, size_t size);
void* __wrap_realloc(void* ptr, size_t size);
void __wrap_free(void* ptr);
void __wrap__free_r(struct _reent* r, void* ptr);
void* __wrap_memalign(size_t alignment, size_t size);
void* __wrap_linearAlloc(size_t size);
void* __wrap_linearMemAlign(size_t size, size_t alignment);
void* __wrap_linearRealloc(void* mem, size_t size);
size_t __wrap_linearGetSize(void* mem);
void __wrap_linearFree(void* mem);
uint32_t __wrap_linearSpaceFree(void);
void Util_check_core_thread(void* arg);

//Variables.
//Set heap size, rest of RAM will be linear RAM, it should be (1024 * 1024 * n).
uint32_t __ctru_heap_size = (1024 * 1024 * 5);
void* (*memalign_heap)(size_t align, size_t size) = memalign_heap_only;
void* (*malloc_heap)(size_t size) = malloc_heap_only;

static bool util_init = false;
static bool util_is_core_available[4] = { 0, };
static LightLock util_linear_alloc_mutex = 1;//Initially unlocked state.
static LightLock util_malloc_mutex = 1;//Initially unlocked state.

//Code.
static inline bool Util_is_heap_low(void)
{
	bool is_low = true;
	void* ptr = NULL;

	LightLock_Lock(&util_malloc_mutex);
	ptr = __real_malloc(DEF_UTIL_LOW_HEAP_THRESHOLD);
	if(ptr)
		is_low = false;

	__real_free(ptr);
	LightLock_Unlock(&util_malloc_mutex);

	return is_low;
}

static void* Util_realloc_heap_to_linear(void* ptr, size_t size)
{
	//Not portable, but efficient.

	//Previous pointer was on heap, new pointer will be
	//on linear RAM, use linearAlloc() to allocate new pointer
	//then copy old data manually, finally free previous pointer.
	void* new_ptr = linearAlloc(size);
	if(new_ptr)
	{
		uint32_t pointer_size = malloc_usable_size(ptr);

		if(size > pointer_size)
			memcpy_asm((uint8_t*)new_ptr, (uint8_t*)ptr, pointer_size);
		else
			memcpy_asm((uint8_t*)new_ptr, (uint8_t*)ptr, size);

		free(ptr);
	}

	return new_ptr;

	//Portable but need to realloc on heap onec
	//so that it may fail if we are running low on heap.

	/*
	//Previous pointer was on heap, new pointer
	//will be on linear RAM, use realloc() to allocate new pointer
	//then use linearAlloc() to allocate linear RAM, copy old data
	//manually, finally free unnecessary pointers.
	void* new_ptr[2] = { NULL, NULL, };

	//Move onto linear RAM.
	LightLock_Lock(&util_malloc_mutex);
	new_ptr[0] = __real_realloc(ptr, size);
	LightLock_Unlock(&util_malloc_mutex);

	if(new_ptr[0])
	{
		new_ptr[1] = linearAlloc(size);
		if(new_ptr[1])
		{
			memcpy(new_ptr[1], new_ptr[0], size);
			free(new_ptr[0]);
			return new_ptr[1];
		}
		else
			return new_ptr[0];
	}
	else
	{
		//Previous pointer was on heap, new pointer
		//will be on heap, just use realloc().
		LightLock_Lock(&util_malloc_mutex);
		new_ptr[0] = __real_realloc(ptr, size);
		LightLock_Unlock(&util_malloc_mutex);

		return new_ptr[0];
	}
	*/
}

static void* Util_realloc_linear_to_heap(void* ptr, size_t size)
{
	//Previous pointer was on linear RAM, new pointer will be
	//on heap, use malloc() to allocate new pointer
	//then copy old data manually, finally free previous pointer.
	void* new_ptr = NULL;

	LightLock_Lock(&util_malloc_mutex);
	new_ptr = __real_malloc(size);
	LightLock_Unlock(&util_malloc_mutex);

	if(new_ptr)
	{
		uint32_t pointer_size = linearGetSize(ptr);

		if(size > pointer_size)
			memcpy_asm((uint8_t*)new_ptr, (uint8_t*)ptr, pointer_size);
		else
			memcpy_asm((uint8_t*)new_ptr, (uint8_t*)ptr, size);

		free(ptr);
	}

	return new_ptr;
}

static void* memalign_heap_only(size_t align, size_t size)
{
	void* ptr = NULL;

	//Alloc memory on heap for some libctru functions (precisely svcCreateMemoryBlock()).
	//Only heap is acceptable here.
	LightLock_Lock(&util_malloc_mutex);
	ptr = __real_memalign(align, size);
	LightLock_Unlock(&util_malloc_mutex);

	return ptr;
}

static void* malloc_heap_only(size_t size)
{
	void* ptr = NULL;

	//Alloc memory on heap for some libctru functions (precisely svcCreateMemoryBlock()).
	//Only heap is acceptable here.
	LightLock_Lock(&util_malloc_mutex);
	ptr = __real_malloc(size);
	LightLock_Unlock(&util_malloc_mutex);

	return ptr;
}

int posix_memalign(void** memptr, size_t alignment, size_t size)
{
	*memptr = memalign(alignment, size);
	if(!*memptr)
		return -1;
	else
		return 0;
}

long sysconf(int name)
{
	if(name == _SC_NPROCESSORS_CONF || name == _SC_NPROCESSORS_ONLN)
	{
		uint8_t model = 0;
		CFGU_GetSystemModel(&model);

		if(name == _SC_NPROCESSORS_CONF)
		{
			if(model == CFG_MODEL_N2DSXL || model == CFG_MODEL_N3DS || model == CFG_MODEL_N3DSXL)
				return 4;
			else
				return 2;
		}
		else if(name == _SC_NPROCESSORS_ONLN)
		{
			if(model == CFG_MODEL_N2DSXL || model == CFG_MODEL_N3DS || model == CFG_MODEL_N3DSXL)
				return 2 + Util_is_core_available(2) + Util_is_core_available(3);
			else
				return 2;
		}
		else
			return -1;
	}
	else
		return -1;
}

void* __wrap_malloc(size_t size)
{
	bool is_heap_low = Util_is_heap_low();
	void* ptr = NULL;

	//Alloc memory on linear ram if requested size is greater than threshold
	//or running low on heap to prevent slow down (linear alloc is slow).
	//If allocation failed, try different memory before giving up.
	if(size > DEF_UTIL_LINEAR_THRESHOLD_SIZE || is_heap_low)
	{
		ptr = linearAlloc(size);
		if(!ptr)
		{
			LightLock_Lock(&util_malloc_mutex);
			ptr = __real_malloc(size);
			LightLock_Unlock(&util_malloc_mutex);
		}
	}
	else
	{
		LightLock_Lock(&util_malloc_mutex);
		ptr = __real_malloc(size);
		LightLock_Unlock(&util_malloc_mutex);

		if(!ptr)
			ptr = linearAlloc(size);
	}
	return ptr;
}

void* __wrap_calloc(size_t items, size_t size)
{
	bool is_heap_low = Util_is_heap_low();
	void* ptr = NULL;

	//Alloc memory on linear ram if requested size is greater than threshold
	//or running low on heap to prevent slow down (linear alloc is slow).
	//If allocation failed, try different memory before giving up.
	if((size * items) > DEF_UTIL_LINEAR_THRESHOLD_SIZE || is_heap_low)
	{
		ptr = linearAlloc(size * items);
		if(!ptr)
		{
			LightLock_Lock(&util_malloc_mutex);
			ptr = __real_calloc(items, size);
			LightLock_Unlock(&util_malloc_mutex);
		}
		else
			memset(ptr, 0x00, (size * items));
	}
	else
	{
		LightLock_Lock(&util_malloc_mutex);
		ptr = __real_calloc(items, size);
		LightLock_Unlock(&util_malloc_mutex);

		if(!ptr)
		{
			ptr = linearAlloc(size * items);
			if(ptr)
				memset(ptr, 0x00, (size * items));
		}
	}
	return ptr;
}

void* __wrap_realloc(void* ptr, size_t size)
{
	bool is_heap_low = Util_is_heap_low();
	void* new_ptr = NULL;

	//Alloc memory on linear ram if requested size is greater than threshold,
	//running low on heap or previous pointer is allocated on
	//linear ram to prevent slow down (linear alloc is slow).
	//If allocation failed, try different memory before giving up.
	if(size > DEF_UTIL_LINEAR_THRESHOLD_SIZE || is_heap_low || DEF_UTIL_IS_LINEAR_RAM(ptr))
	{
		if(!ptr || DEF_UTIL_IS_LINEAR_RAM(ptr))
		{
			//Previous pointer was on linear RAM, new pointer will be on linear RAM, just use linearRealloc().
			new_ptr = linearRealloc(ptr, size);
			if(!new_ptr)
				new_ptr = Util_realloc_linear_to_heap(ptr, size);
		}
		else
		{
			new_ptr = Util_realloc_heap_to_linear(ptr, size);
			if(!new_ptr)
			{
				//Previous pointer was on heap, new pointer will be on heap, just use realloc().
				LightLock_Lock(&util_malloc_mutex);
				new_ptr = __real_realloc(ptr, size);
				LightLock_Unlock(&util_malloc_mutex);
			}
		}
	}
	else
	{
		//Previous pointer was on heap, new pointer will be on heap, just use realloc().
		LightLock_Lock(&util_malloc_mutex);
		new_ptr = __real_realloc(ptr, size);
		LightLock_Unlock(&util_malloc_mutex);
		if(!new_ptr)
			new_ptr = Util_realloc_heap_to_linear(ptr, size);
	}

	return new_ptr;
}

void __wrap_free(void* ptr)
{
	if(DEF_UTIL_IS_LINEAR_RAM(ptr))
		linearFree(ptr);
	else
	{
		LightLock_Lock(&util_malloc_mutex);
		__real_free(ptr);
		LightLock_Unlock(&util_malloc_mutex);
	}
}

void __wrap__free_r(struct _reent* r, void* ptr)
{
	if(DEF_UTIL_IS_LINEAR_RAM(ptr))
		linearFree(ptr);
	else
		__real__free_r(r, ptr);
}

void* __wrap_memalign(size_t alignment, size_t size)
{
	bool is_heap_low = Util_is_heap_low();
	void* ptr = NULL;

	//Alloc memory on linear ram if requested size is greater than threshold
	//or running low on heap to prevent slow down (linear alloc is slow).
	//If allocation failed, try different memory before giving up.
	if(size > DEF_UTIL_LINEAR_THRESHOLD_SIZE || is_heap_low)
	{
		ptr = linearMemAlign(size, alignment);
		if(!ptr)
		{
			LightLock_Lock(&util_malloc_mutex);
			ptr = __real_memalign(alignment, size);
			LightLock_Unlock(&util_malloc_mutex);
		}
	}
	else
	{
		LightLock_Lock(&util_malloc_mutex);
		ptr = __real_memalign(alignment, size);
		LightLock_Unlock(&util_malloc_mutex);

		if(!ptr)
			ptr = linearMemAlign(size, alignment);
	}
	return ptr;
}

void* __wrap_linearAlloc(size_t size)
{
	void* pointer = NULL;

	LightLock_Lock(&util_linear_alloc_mutex);
	pointer = __real_linearAlloc(size);
	LightLock_Unlock(&util_linear_alloc_mutex);

	return pointer;
}

void* __wrap_linearMemAlign(size_t size, size_t alignment)
{
	void* pointer = NULL;

	LightLock_Lock(&util_linear_alloc_mutex);
	pointer = __real_linearMemAlign(size, alignment);
	LightLock_Unlock(&util_linear_alloc_mutex);

	return pointer;
}

void* __wrap_linearRealloc(void* mem, size_t size)
{
	void* new_ptr = NULL;

	LightLock_Lock(&util_linear_alloc_mutex);
	if(size == 0)
	{
		__real_linearFree(mem);
		new_ptr = mem;
	}
	else if(!mem)
		new_ptr = __real_linearAlloc(size);
	else
	{
		new_ptr = __real_linearAlloc(size);
		if(new_ptr)
		{
			uint32_t pointer_size = __real_linearGetSize(mem);

			if(size > pointer_size)
				memcpy_asm((uint8_t*)new_ptr, (uint8_t*)mem, pointer_size);
			else
				memcpy_asm((uint8_t*)new_ptr, (uint8_t*)mem, size);

			__real_linearFree(mem);
		}
	}
	LightLock_Unlock(&util_linear_alloc_mutex);

	return new_ptr;
}

size_t __wrap_linearGetSize(void* mem)
{
	size_t size = 0;

	LightLock_Lock(&util_linear_alloc_mutex);
	size = __real_linearGetSize(mem);
	LightLock_Unlock(&util_linear_alloc_mutex);

	return size;
}

void __wrap_linearFree(void* mem)
{
	LightLock_Lock(&util_linear_alloc_mutex);
	__real_linearFree(mem);
	LightLock_Unlock(&util_linear_alloc_mutex);
}

uint32_t __wrap_linearSpaceFree(void)
{
	uint32_t space = 0;

	LightLock_Lock(&util_linear_alloc_mutex);
	space = __real_linearSpaceFree();
	LightLock_Unlock(&util_linear_alloc_mutex);

	return space;
}

uint32_t Util_init(void)
{
	uint8_t model = 0;
	uint8_t loop = 0;

	if(util_init)
		goto already_inited;

	for(uint8_t i = 0; i < 4; i++)
		util_is_core_available[i] = false;

	CFGU_GetSystemModel(&model);

	//NEW3DS have 4 cores, OLD3DS have 2 cores.
	if(model == CFG_MODEL_N2DSXL || model == CFG_MODEL_N3DS || model == CFG_MODEL_N3DSXL)
		loop = 4;
	else
		loop = 2;

	//Check for core availability.
	for(uint8_t i = 0; i < loop; i++)
	{
		Thread thread = threadCreate(Util_check_core_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, i, false);

		if(!thread)
			util_is_core_available[i] = false;
		else
		{
			threadJoin(thread, U64_MAX);
			util_is_core_available[i] = true;
		}

		threadFree(thread);
	}

	util_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;
}

void Util_exit(void)
{
	if(!util_init)
		return;

	util_init = false;
}

uint32_t Util_parse_file(const char* source_data, uint32_t expected_items, Str_data* out_data)
{
	uint32_t result = DEF_ERR_OTHER;
	Str_data start_text = { 0, };
	Str_data end_text = { 0, };

	if(!out_data || expected_items == 0)
		goto invalid_arg;

	for(uint32_t i = 0; i < expected_items; i++)
	{
		result = Util_str_init(&out_data[i]);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto api_failed;
		}
	}

	result = Util_str_init(&start_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
	}

	result = Util_str_init(&end_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
	}

	for (uint32_t i = 0; i < expected_items; i++)
	{
		char* start_pos = NULL;
		char* end_pos = NULL;
		uint32_t copy_length = 0;

		result = Util_str_format(&start_text, "<%" PRIu32 ">", i);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_format, false, result);
			goto api_failed;
		}

		result = Util_str_format(&end_text, "</%" PRIu32 ">", i);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_format, false, result);
			goto api_failed;
		}

		start_pos = strstr(source_data, start_text.buffer);
		end_pos = strstr(source_data, end_text.buffer);

		if (!start_pos || !end_pos || (start_pos + start_text.length) > end_pos)
		{
			DEF_LOG_FORMAT("Failed to parse file. Error pos : %s%s", start_text.buffer, end_text.buffer);
			result = DEF_ERR_OTHER;
			goto error_other;
		}

		copy_length = (end_pos - (start_pos + start_text.length));
		copy_length = (uint32_t)Util_min(copy_length, INT_MAX);
		//We can't get rid of this "int" because "*" specifier expects "int".
		result = Util_str_format(&out_data[i], "%.*s", (int)copy_length, (start_pos + start_text.length));
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_format, false, result);
			goto api_failed;
		}
	}

	Util_str_free(&start_text);
	Util_str_free(&end_text);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	error_other:
	for(uint32_t i = 0; i < expected_items; i++)
		Util_str_free(&out_data[i]);

	Util_str_free(&start_text);
	Util_str_free(&end_text);
	return result;
}

uint32_t Util_convert_seconds_to_time(double input_seconds, Str_data* time_string)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t hours = 0;
	uint32_t minutes = 0;
	double seconds = 0;

	if(!time_string)
		goto invalid_arg;

	if(isinf(input_seconds) || isnan(input_seconds))
		input_seconds = 0;

	result = Util_str_init(time_string);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
	}

	hours = ((uint32_t)input_seconds / 3600);
	minutes = (((uint32_t)input_seconds % 3600) / 60);
	seconds = ((uint32_t)input_seconds % 60);
	seconds += (input_seconds - (uint32_t)input_seconds);//ms.

	result = Util_str_format(time_string, "%02" PRIu32 ":%02" PRIu32 ":%04.1f", hours, minutes, seconds);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_format, false, result);
		goto api_failed;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	return result;
}

uint32_t Util_load_msg(const char* file_name, Str_data* out_msg, uint32_t num_of_msg)
{
	uint8_t* fs_buffer = NULL;
	uint32_t read_size = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(!file_name || !out_msg || num_of_msg == 0)
		goto invalid_arg;

	result = Util_file_load_from_rom(file_name, "romfs:/gfx/msg/", &fs_buffer, 0x2000, &read_size);
	if (result != DEF_SUCCESS)
		goto api_failed;

	result = Util_parse_file((char*)fs_buffer, num_of_msg, out_msg);
	if (result != DEF_SUCCESS)
		goto api_failed;

	free(fs_buffer);
	fs_buffer = NULL;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	free(fs_buffer);
	fs_buffer = NULL;
	return result;
}

uint32_t Util_encode_to_escape(const char* text, Str_data* escaped_text)
{
	uint32_t in_text_length = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(!text || !escaped_text)
		goto invalid_arg;

	result = Util_str_init(escaped_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
	}

	in_text_length = strlen(text);
	for(uint32_t i = 0; i < in_text_length; i++)
	{
		if(text[i] == '\n')
			result = Util_str_add(escaped_text, "\\n");
		else if(text[i] == '"')
			result = Util_str_add(escaped_text, "\\\"");
		else if(text[i] == '\\')
			result = Util_str_add(escaped_text, "\\\\");
		else
		{
			const char one_char[2] = { text[i], 0x00 };
			result = Util_str_add(escaped_text, one_char);
		}

		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_add, false, result);
			goto api_failed;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	Util_str_free(escaped_text);
	return result;
}

uint32_t Util_base64_encode(const char* text, Str_data* encoded_text)
{
	uint32_t in_text_length = 0;
	uint32_t result = DEF_ERR_OTHER;
	uint32_t out_text_length = 0;
	size_t written = 0;
	char* encoded = NULL;

	if(!text || !encoded_text)
		goto invalid_arg;

	//Calc output size then allocate output buffer.
	in_text_length = strlen(text);
	out_text_length = ((((4 * in_text_length) / 3) + 3) & ~0x03);
	out_text_length++;//+1 for NULL terminator.
	encoded = (char*)malloc(out_text_length);
	if(!encoded)
		goto out_of_memory;

	result = Util_str_init(encoded_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
	}

	result = mbedtls_base64_encode((unsigned char*)encoded, out_text_length, &written, (const unsigned char*)text, in_text_length);
	if(result != 0)
	{
		DEF_LOG_RESULT(mbedtls_base64_encode, false, result);
		goto mbed_tls_api_failed;
	}

	//Add NULL terminator.
	if(out_text_length <= written)
		encoded[out_text_length - 1] = 0x00;
	else
		encoded[written] = 0x00;

	result = Util_str_set(encoded_text, encoded);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_failed;
	}

	free(encoded);
	encoded = NULL;

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	api_failed:
	Util_str_free(encoded_text);
	free(encoded);
	encoded = NULL;
	return result;

	mbed_tls_api_failed:
	Util_str_free(encoded_text);
	free(encoded);
	encoded = NULL;
	return DEF_ERR_MBEDTLS_RETURNED_NOT_SUCCESS;
}

uint32_t Util_base64_decode(const char* encoded_text, Str_data* text)
{
	uint32_t in_text_length = 0;
	uint32_t result = DEF_ERR_OTHER;
	uint32_t out_text_length = 0;
	size_t written = 0;
	char* decoded = NULL;

	if(!encoded_text || !text)
		goto invalid_arg;

	//Calc output size then allocate output buffer.
	in_text_length = strlen(encoded_text);
	out_text_length = ((in_text_length / 4) * 3);
	out_text_length++;//+1 for NULL terminator.
	decoded = (char*)malloc(out_text_length);
	if(!decoded)
		goto out_of_memory;

	result = Util_str_init(text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
	}

	result = mbedtls_base64_decode((unsigned char*)decoded, out_text_length, &written, (const unsigned char*)encoded_text, in_text_length);
	if(result != 0)
	{
		DEF_LOG_RESULT(mbedtls_base64_decode, false, result);
		goto mbed_tls_api_failed;
	}

	//Add NULL terminator.
	if(out_text_length <= written)
		decoded[out_text_length - 1] = 0x00;
	else
		decoded[written] = 0x00;

	result = Util_str_set(text, decoded);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_failed;
	}

	free(decoded);
	decoded = NULL;

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	api_failed:
	Util_str_free(text);
	free(decoded);
	decoded = NULL;
	return result;

	mbed_tls_api_failed:
	Util_str_free(text);
	free(decoded);
	decoded = NULL;
	return DEF_ERR_MBEDTLS_RETURNED_NOT_SUCCESS;
}

uint32_t Util_check_free_linear_space(void)
{
	return linearSpaceFree();
}

uint32_t Util_check_free_ram(void)
{
	//Put this array on static area so that we don't need to fly our stack.
	static uint8_t* malloc_check[1024] = { 0, };
	uint32_t index = 0;
	uint32_t alloc_size = (1000 * 16000);
	uint32_t alloced_size = 0;

	for (uint32_t i = 0; i < 1024; i++)
		malloc_check[i] = NULL;

	//Block malloc opetations while we are checking for available heap size.
	LightLock_Lock(&util_malloc_mutex);
	while(true)
	{
		if(index == 1024)
			break;

		malloc_check[index] = (uint8_t*)__real_malloc(alloc_size);
		if (malloc_check[index])
		{
			alloced_size += alloc_size;
			index++;
		}
		else
		{
			if(alloc_size == 1)
				break;//Out of memory.

			alloc_size /= 2;
		}
	}

	for (uint32_t i = 0; i < index; i++)
		__real_free(malloc_check[i]);

	LightLock_Unlock(&util_malloc_mutex);

	return alloced_size;//Return free bytes.
}

bool Util_is_core_available(uint8_t core_id)
{
	if(!util_init)
		return false;
	if(core_id > 3)
		return false;
	else
		return util_is_core_available[core_id];
}

void Util_sleep(uint64_t us)
{
	svcSleepThread(us * 1000);
}

int64_t Util_min(int64_t value_0, int64_t value_1)
{
	return (value_0 > value_1 ? value_1 : value_0);
}

int64_t Util_max(int64_t value_0, int64_t value_1)
{
	return (value_0 > value_1 ? value_0 : value_1);
}

double Util_min_d(double value_0, double value_1)
{
	return (value_0 > value_1 ? value_1 : value_0);
}

double Util_max_d(double value_0, double value_1)
{
	return (value_0 > value_1 ? value_0 : value_1);
}

void Util_check_core_thread(void* arg)
{
	(void)arg;
	threadExit(0);
}
