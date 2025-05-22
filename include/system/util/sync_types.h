#if !defined(DEF_SYNC_TYPES_H)
#define DEF_SYNC_TYPES_H
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	SYNC_TYPE_INVALID = -1,

	SYNC_TYPE_NON_RECURSIVE_MUTEX,		//Non-recursive mutex.
	SYNC_TYPE_RECURSIVE_MUTEX,			//Recursive mutex.
	SYNC_TYPE_CONDITION,				//Condition variable.

	SYNC_TYPE_MAX,
} Sync_type;

typedef struct
{
	void* internal_object;			//Internal object, do not tamper with.
} Sync_data;

#endif //!defined(DEF_SYNC_TYPES_H)
