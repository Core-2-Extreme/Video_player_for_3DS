#if !defined(DEF_CPU_USAGE_TYPES_H)
#define DEF_CPU_USAGE_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_CPU_USAGE_API_ENABLE			/*(bool)(*/true/*)*/	//Enable CPU usage monitor API.

#define DEF_CPU_USAGE_MAX_CORES				(uint8_t)(4)			//Maximum number of CPU cores.
#define DEF_CPU_USAGE_ALL_CORES				(uint8_t)(UINT8_MAX)	//All cores (for Util_cpu_usage_get_cpu_usage()).

#endif //!defined(DEF_CPU_USAGE_TYPES_H)
