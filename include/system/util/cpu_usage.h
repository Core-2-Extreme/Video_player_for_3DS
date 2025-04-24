#if !defined(DEF_CPU_USAGE_H)
#define DEF_CPU_USAGE_H
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "system/util/cpu_usage_types.h"

#if DEF_CPU_USAGE_API_ENABLE

/**
 * @brief Initialize CPU usage API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cpu_usage_init(void);

/**
 * @brief Uninitialize CPU usage API.
 * Do nothing if CPU usage API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_cpu_usage_exit(void);

/**
 * @brief Get CPU usage.
 * Always return NAN if CPU usage API is not initialized.
 * @param core_id (in) CPU core, -1 means all CPUs.
 * @return CPU usage in %.
 * @warning Thread dangerous (untested).
*/
float Util_cpu_usage_get_cpu_usage(int8_t core_id);

/**
 * @brief Check max allowed core #1 usage.
 * Always return 0 if CPU usage API is not initialized.
 * @return Max allowed core #1 usage.
 * @warning Thread dangerous (untested).
*/
uint8_t Util_cpu_usage_get_core_1_limit(void);

/**
 * @brief Query CPU usage show flag.
 * Always return false if CPU usage API is not initialized.
 * @return Internal CPU usage show flag.
 * @warning Thread dangerous (untested).
*/
bool Util_cpu_usage_query_show_flag(void);

/**
 * @brief Set CPU usage show flag.
 * Do nothing if CPU usage API is not initialized.
 * @param flag (in) When true, internal CPU usage show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested).
*/
void Util_cpu_usage_set_show_flag(bool flag);

/**
 * @brief Draw CPU usage.
 * @warning Thread dangerous (untested).
 * @warning Call it only from rendering thread.
*/
void Util_cpu_usage_draw(void);

#else

#define Util_cpu_usage_init() DEF_ERR_DISABLED
#define Util_cpu_usage_exit()
#define Util_cpu_usage_get_cpu_usage(...) NAN
#define Util_cpu_usage_get_core_1_limit() 0
#define Util_cpu_usage_query_show_flag() false
#define Util_cpu_usage_set_show_flag(...)
#define Util_cpu_usage_draw()

#endif //DEF_CPU_USAGE_API_ENABLE

#endif //!defined(DEF_CPU_USAGE_H)
