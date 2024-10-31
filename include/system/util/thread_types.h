#if !defined(DEF_THREAD_TYPES_H)
#define DEF_THREAD_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_THREAD_STACKSIZE				(uint32_t)(64 * 1024)
#define DEF_THREAD_INACTIVE_SLEEP_TIME		(uint64_t)(100000)
#define DEF_THREAD_ACTIVE_SLEEP_TIME		(uint64_t)(50000)
#define DEF_THREAD_WAIT_TIME				(uint64_t)(10000000000)	//10s.

//0x18~0x3F.
#define DEF_THREAD_PRIORITY_IDLE			(int32_t)(0x36)	//Lowest priority for user thread.
#define DEF_THREAD_PRIORITY_LOW				(int32_t)(0x2E)
#define DEF_THREAD_PRIORITY_BELOW_NORMAL	(int32_t)(0x2D)
#define DEF_THREAD_PRIORITY_NORMAL			(int32_t)(0x2C)
#define DEF_THREAD_PRIORITY_ABOVE_NORMAL	(int32_t)(0x2B)
#define DEF_THREAD_PRIORITY_HIGH			(int32_t)(0x2A)
#define DEF_THREAD_PRIORITY_REALTIME		(int32_t)(0x22)	//Highest priority for user thread.

#define DEF_THREAD_SYSTEM_PRIORITY_IDLE		(int32_t)(0x3F)	//Lowest priority for system thread, user thread must not use this priority.
#define DEF_THREAD_SYSTEM_PRIORITY_REALTIME	(int32_t)(0x18)	//Highest priority for system thread, user thread must not use this priority.

#endif //!defined(DEF_THREAD_TYPES_H)
