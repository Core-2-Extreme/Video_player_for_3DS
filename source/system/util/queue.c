//Includes.
#include "system/util/queue.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/util.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
static LightLock util_queue_mutex = 1;//Initially unlocked state.

//Code.
uint32_t Util_queue_create(Queue_data* queue, uint32_t max_items)
{
	if(!queue || max_items == 0 || max_items == UINT32_MAX)
		goto invalid_arg;

	LightLock_Lock(&util_queue_mutex);

	if(queue->data || queue->event_id)
		goto already_inited;

	memset(queue, 0x0, sizeof(Queue_data));

	queue->data = (void**)malloc(max_items * sizeof(void*));
	queue->event_id = (uint32_t*)malloc(max_items * sizeof(uint32_t));
	if(!queue->data || !queue->event_id)
		goto out_of_memory;

	memset(queue->data, 0x0, (max_items * sizeof(void*)));
	memset(queue->event_id, 0x0, (max_items * sizeof(uint32_t)));
	queue->max_items = max_items;
	queue->next_index = 0;
	queue->reference_count = 0;

	LightEvent_Init(&queue->receive_wait_event, RESET_ONESHOT);
	LightEvent_Init(&queue->send_wait_event, RESET_ONESHOT);

	LightLock_Unlock(&util_queue_mutex);

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_ALREADY_INITIALIZED;

	out_of_memory:
	free(queue->data);
	free(queue->event_id);
	queue->data = NULL;
	queue->event_id = NULL;
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Util_queue_add(Queue_data* queue, uint32_t event_id, void* data, int64_t wait_us, Queue_option option)
{
	if(!queue)
		goto invalid_arg;

	LightLock_Lock(&util_queue_mutex);

	if(!queue->data || !queue->event_id || queue->deleting)
		goto not_inited;

	queue->reference_count++;

	if(queue->next_index >= queue->max_items && wait_us > 0)
	{
		//No spaces are available, wait for a space.
		LightLock_Unlock(&util_queue_mutex);
		LightEvent_WaitTimeout(&queue->send_wait_event, wait_us * 1000);
		LightLock_Lock(&util_queue_mutex);

		//It may possible to start deleting this queue on different thread while waiting.
		if(queue->deleting)
			goto deleting;
	}

	if(queue->next_index >= queue->max_items)
	{
		//Queue is full.
		goto out_of_memory;
	}
	else
	{
		//Queue is not full.
		uint32_t index = 0;

		if(option & QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
		{
			//Don't add the event if the same event exist.
			for(uint32_t i = 0; i < queue->next_index; i++)
			{
				if(queue->event_id[i] == event_id)
					goto already_exist;
			}
		}

		if(option & QUEUE_OPTION_SEND_TO_FRONT)
		{
			//Move other data to back.
			for(int64_t i = ((int64_t)queue->next_index - 1); i > -1; i--)
			{
				queue->data[i + 1] = queue->data[i];
				queue->event_id[i + 1] = queue->event_id[i];
			}

			index = 0;
		}
		else
			index = queue->next_index;

		queue->data[index] = data;
		queue->event_id[index] = event_id;
		queue->next_index++;

		LightEvent_Clear(&queue->send_wait_event);
		LightEvent_Signal(&queue->receive_wait_event);
	}

	queue->reference_count--;
	LightLock_Unlock(&util_queue_mutex);

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	deleting:
	queue->reference_count--;
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	out_of_memory:
	queue->reference_count--;
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_OUT_OF_MEMORY;

	already_exist://Treat it as success.
	queue->reference_count--;
	LightLock_Unlock(&util_queue_mutex);
	return DEF_SUCCESS;
}

uint32_t Util_queue_get(Queue_data* queue, uint32_t* event_id, void** data, int64_t wait_us)
{
	if(!queue || !event_id)
		goto invalid_arg;

	LightLock_Lock(&util_queue_mutex);

	if(!queue->data || !queue->event_id || queue->deleting)
		goto not_inited;

	queue->reference_count++;

	if(queue->next_index == 0 && wait_us > 0)
	{
		//No messages are available, wait for a message.
		LightLock_Unlock(&util_queue_mutex);
		LightEvent_WaitTimeout(&queue->receive_wait_event, wait_us * 1000);
		LightLock_Lock(&util_queue_mutex);

		//It may possible to start delting this queue on different thread while waiting.
		if(queue->deleting)
			goto deleting;
	}

	if(queue->next_index == 0)
	{
		//Queue is empty.
		goto try_again;
	}
	else
	{
		//Queue is not empty.
		*event_id = queue->event_id[0];
		if(queue->data[0])
		{
			if(data)
				*data = queue->data[0];
			else
				free(queue->data[0]);
		}

		//Delete old data as it is no longer necessary.
		queue->data[0] = NULL;
		queue->event_id[0] = 0;

		//Move rest of the data to front.
		for(uint32_t i = 0; i < (queue->next_index - 1); i++)
		{
			queue->data[i] = queue->data[i + 1];
			queue->event_id[i] = queue->event_id[i + 1];
		}

		//Delete the last data.
		queue->data[queue->next_index - 1] = NULL;
		queue->event_id[queue->next_index - 1] = 0;

		queue->next_index--;

		LightEvent_Clear(&queue->receive_wait_event);
		LightEvent_Signal(&queue->send_wait_event);
	}

	queue->reference_count--;
	LightLock_Unlock(&util_queue_mutex);

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	deleting:
	queue->reference_count--;
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	queue->reference_count--;
	LightLock_Unlock(&util_queue_mutex);
	return DEF_ERR_TRY_AGAIN;
}

bool Util_queue_check_event_exist(const Queue_data* queue, uint32_t event_id)
{
	bool exist = false;

	if(!queue)
		return false;

	LightLock_Lock(&util_queue_mutex);

	if(!queue->data || !queue->event_id || queue->deleting)
	{
		LightLock_Unlock(&util_queue_mutex);
		return false;
	}

	for(uint32_t i = 0; i < queue->next_index; i++)
	{
		if(queue->event_id[i] == event_id)
		{
			exist = true;
			break;
		}
	}

	LightLock_Unlock(&util_queue_mutex);

	return exist;
}

uint32_t Util_queue_get_free_space(const Queue_data* queue)
{
	uint32_t free = 0;

	if(!queue)
		return 0;

	LightLock_Lock(&util_queue_mutex);

	if(!queue->data || !queue->event_id || queue->deleting)
	{
		LightLock_Unlock(&util_queue_mutex);
		return 0;
	}

	free = queue->max_items - queue->next_index;

	LightLock_Unlock(&util_queue_mutex);

	return free;
}

void Util_queue_delete(Queue_data* queue)
{
	if(!queue || !queue->data || !queue->event_id || queue->deleting)
		return;

	LightLock_Lock(&util_queue_mutex);
	queue->deleting = true;

	//Wait for all threads to exit Util_queue*() functions.
	while(queue->reference_count > 0)
	{
		LightLock_Unlock(&util_queue_mutex);
		LightEvent_Signal(&queue->receive_wait_event);
		LightEvent_Signal(&queue->send_wait_event);
		Util_sleep(1000);
		LightLock_Lock(&util_queue_mutex);
	}

	for(uint32_t i = 0; i < queue->max_items; i++)
	{
		free(queue->data[i]);
		queue->data[i] = NULL;
		queue->event_id[i] = 0;
	}

	free(queue->data);
	free(queue->event_id);
	queue->data = NULL;
	queue->event_id = NULL;
	memset(queue, 0x0, sizeof(Queue_data));

	LightLock_Unlock(&util_queue_mutex);
}
