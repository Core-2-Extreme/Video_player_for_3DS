//Includes.
#include "system/util/watch.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/sync.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
static bool util_watch_init = false;
static uint32_t util_watch_num_of_active[WATCH_HANDLE_MAX] = { 0, };
static Sync_data util_watch_variables_mutex = { 0, };
static Watch_data util_watch_data[DEF_WATCH_MAX_VARIABLES] = { 0, };

//Code.
uint32_t Util_watch_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_watch_init)
		goto already_inited;

	result = Util_sync_create(&util_watch_variables_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	for(uint16_t i = 0; i < (uint16_t)WATCH_HANDLE_MAX; i++)
		util_watch_num_of_active[i] = 0;

	for(uint32_t i = 0; i < DEF_WATCH_MAX_VARIABLES; i++)
	{
		util_watch_data[i].original_address = NULL;
		util_watch_data[i].previous_data = NULL;
		util_watch_data[i].data_length = 0;
		util_watch_data[i].handle = WATCH_HANDLE_INVALID;
	}

	util_watch_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	error_other:
	return result;
}

void Util_watch_exit(void)
{
	if(!util_watch_init)
		return;

	util_watch_init = false;

	Util_sync_lock(&util_watch_variables_mutex, UINT64_MAX);
	for(uint32_t i = 0; i < DEF_WATCH_MAX_VARIABLES; i++)
	{
		free(util_watch_data[i].previous_data);
		util_watch_data[i].previous_data = NULL;
	}

	Util_sync_unlock(&util_watch_variables_mutex);
	Util_sync_destroy(&util_watch_variables_mutex);
}

uint32_t Util_watch_get_usage(Watch_handle handle)
{
	uint32_t used = 0;

	if(!util_watch_init)
		return 0;

	if(handle <= WATCH_HANDLE_INVALID || handle >= WATCH_HANDLE_MAX)
		return 0;

	Util_sync_lock(&util_watch_variables_mutex, UINT64_MAX);
	used = util_watch_num_of_active[handle];
	Util_sync_unlock(&util_watch_variables_mutex);

	return used;
}

uint32_t Util_watch_get_total_usage(void)
{
	uint32_t used = 0;

	if(!util_watch_init)
		return 0;

	Util_sync_lock(&util_watch_variables_mutex, UINT64_MAX);
	for(uint16_t i = 0; i < (uint16_t)WATCH_HANDLE_MAX; i++)
		used += util_watch_num_of_active[i];

	Util_sync_unlock(&util_watch_variables_mutex);

	return used;
}

uint32_t Util_watch_add(Watch_handle handle, const void* variable, uint32_t length)
{
	uint32_t used = 0;

	if(!util_watch_init)
		goto not_inited;

	if(handle <= WATCH_HANDLE_INVALID || handle >= WATCH_HANDLE_MAX || !variable || length == 0)
		goto invalid_arg;

	Util_sync_lock(&util_watch_variables_mutex, UINT64_MAX);

	for(uint16_t i = 0; i < (uint16_t)WATCH_HANDLE_MAX; i++)
		used += util_watch_num_of_active[i];

	if(used >= DEF_WATCH_MAX_VARIABLES)
		goto out_of_memory;

	//Search for free space and register it.
	for(uint32_t i = 0; i < DEF_WATCH_MAX_VARIABLES; i++)
	{
		if(!util_watch_data[i].original_address)
		{
			util_watch_data[i].previous_data = (void*)malloc(length);
			if(!util_watch_data[i].previous_data)
				goto out_of_memory;

			util_watch_data[i].original_address = variable;
			util_watch_data[i].data_length = length;
			util_watch_data[i].handle = handle;
			util_watch_num_of_active[handle]++;

			memcpy(util_watch_data[i].previous_data, util_watch_data[i].original_address, util_watch_data[i].data_length);
			break;
		}
	}

	Util_sync_unlock(&util_watch_variables_mutex);
	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	Util_sync_unlock(&util_watch_variables_mutex);
	return DEF_ERR_OUT_OF_MEMORY;
}

void Util_watch_remove(Watch_handle handle, const void* variable)
{
	if(!util_watch_init)
		return;

	if(handle <= WATCH_HANDLE_INVALID || handle >= WATCH_HANDLE_MAX || !variable)
		return;

	Util_sync_lock(&util_watch_variables_mutex, UINT64_MAX);

	//Search for specified address and remove it if exists.
	for(uint32_t i = 0; i < DEF_WATCH_MAX_VARIABLES; i++)
	{
		if(util_watch_data[i].original_address == variable && util_watch_data[i].handle == handle)
		{
			free(util_watch_data[i].previous_data);

			util_watch_data[i].previous_data = NULL;
			util_watch_data[i].original_address = NULL;
			util_watch_data[i].data_length = 0;
			util_watch_data[i].handle = WATCH_HANDLE_INVALID;
			util_watch_num_of_active[handle]--;
			break;
		}
	}

	Util_sync_unlock(&util_watch_variables_mutex);
}

bool Util_watch_is_changed(Watch_handle_bit handles)
{
	bool is_changed = false;

	if(!util_watch_init)
		return false;

	if(handles == DEF_WATCH_HANDLE_BIT_NONE)
		return false;

	Util_sync_lock(&util_watch_variables_mutex, UINT64_MAX);

	//Check if any data that is linked with specified handle were changed.
	for(uint32_t i = 0; i < DEF_WATCH_MAX_VARIABLES; i++)
	{
		Watch_handle_bit handle_bit = DEF_WATCH_HANDLE_BIT_NONE;

		if(util_watch_data[i].handle != WATCH_HANDLE_INVALID)
			handle_bit = (Watch_handle_bit)(1 << (uint16_t)util_watch_data[i].handle);

		if(util_watch_data[i].original_address && (handles & handle_bit))
		{
			//This data is linked with specified handle.
			if(memcmp(util_watch_data[i].previous_data, util_watch_data[i].original_address, util_watch_data[i].data_length) != 0)
			{
				//Data was changed, update it.
				memcpy(util_watch_data[i].previous_data, util_watch_data[i].original_address, util_watch_data[i].data_length);
				is_changed = true;
			}
		}
	}

	Util_sync_unlock(&util_watch_variables_mutex);

	return is_changed;
}
