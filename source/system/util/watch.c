//Includes.
#include "system/util/watch.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
static bool util_watch_init = false;
static uint32_t util_watch_num_of_active[WATCH_HANDLE_MAX] = { 0, };
static LightLock util_watch_variables_mutex = 1;//Initially unlocked state.
static Watch_data util_watch_data[DEF_WATCH_MAX_VARIABLES] = { 0, };

//Code.
uint32_t Util_watch_init(void)
{
	if(util_watch_init)
		goto already_inited;

	LightLock_Init(&util_watch_variables_mutex);

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
}

void Util_watch_exit(void)
{
	if(!util_watch_init)
		return;

	util_watch_init = false;

	LightLock_Lock(&util_watch_variables_mutex);
	for(uint32_t i = 0; i < DEF_WATCH_MAX_VARIABLES; i++)
	{
		free(util_watch_data[i].previous_data);
		util_watch_data[i].previous_data = NULL;
	}

	LightLock_Unlock(&util_watch_variables_mutex);
}

uint32_t Util_watch_get_usage(Watch_handle handle)
{
	uint32_t used = 0;

	if(!util_watch_init)
		return 0;

	if(handle <= WATCH_HANDLE_INVALID || handle >= WATCH_HANDLE_MAX)
		return 0;

	LightLock_Lock(&util_watch_variables_mutex);
	used = util_watch_num_of_active[handle];
	LightLock_Unlock(&util_watch_variables_mutex);

	return used;
}

uint32_t Util_watch_get_total_usage(void)
{
	uint32_t used = 0;

	if(!util_watch_init)
		return 0;

	LightLock_Lock(&util_watch_variables_mutex);
	for(uint16_t i = 0; i < (uint16_t)WATCH_HANDLE_MAX; i++)
		used += util_watch_num_of_active[i];

	LightLock_Unlock(&util_watch_variables_mutex);

	return used;
}

uint32_t Util_watch_add(Watch_handle handle, const void* variable, uint32_t length)
{
	uint32_t used = 0;

	if(!util_watch_init)
		goto not_inited;

	if(handle <= WATCH_HANDLE_INVALID || handle >= WATCH_HANDLE_MAX || !variable || length == 0)
		goto invalid_arg;

	LightLock_Lock(&util_watch_variables_mutex);

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

	LightLock_Unlock(&util_watch_variables_mutex);
	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	LightLock_Unlock(&util_watch_variables_mutex);
	return DEF_ERR_OUT_OF_MEMORY;
}

void Util_watch_remove(Watch_handle handle, const void* variable)
{
	if(!util_watch_init)
		return;

	if(handle <= WATCH_HANDLE_INVALID || handle >= WATCH_HANDLE_MAX || !variable)
		return;

	LightLock_Lock(&util_watch_variables_mutex);

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

	LightLock_Unlock(&util_watch_variables_mutex);
}

bool Util_watch_is_changed(Watch_handle_bit handles)
{
	bool is_changed = false;

	if(!util_watch_init)
		return false;

	if(handles == DEF_WATCH_HANDLE_BIT_NONE)
		return false;

	LightLock_Lock(&util_watch_variables_mutex);

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

	LightLock_Unlock(&util_watch_variables_mutex);

	return is_changed;
}
