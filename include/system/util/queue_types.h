#if !defined(DEF_QUEUE_TYPES_H)
#define DEF_QUEUE_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "3ds.h"

typedef uint8_t Queue_option;
#define QUEUE_OPTION_NONE					(Queue_option)(0 << 0)	//Default.
#define QUEUE_OPTION_DO_NOT_ADD_IF_EXIST	(Queue_option)(1 << 0)	//Do not add the event if the same event ID exist.
#define QUEUE_OPTION_SEND_TO_FRONT			(Queue_option)(1 << 1)	//Send an event to the front of the queue, use it for high priority event.

typedef struct
{
	bool deleting;					//Whether this queue is being deleted.
	void** data;					//Data list.
	uint32_t* event_id;				//Event ID list.
	uint32_t max_items;				//Queue capacity.
	uint32_t next_index;			//Next free index.
	uint32_t reference_count;		//Reference count for this queue.
	LightEvent receive_wait_event;	//If timeout is not 0, this is used to wait for new message from this queue.
	LightEvent send_wait_event;		//If timeout is not 0, this is used to wait for available space to send data to this queue.
} Queue_data;

#endif //!defined(DEF_QUEUE_TYPES_H)
