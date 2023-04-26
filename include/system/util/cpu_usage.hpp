#ifndef CPU_USAGE_HPP
#define CPU_USAGE_HPP

#if DEF_ENABLE_CPU_MONITOR_API
#include "system/types.hpp"

/**
 * @brief Initialize cpu usage monitor API.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cpu_usage_monitor_init(void);

/**
 * @brief Uninitialize cpu usage monitor API.
 * Do nothing if cpu usage monitor api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_cpu_usage_monitor_exit(void);

/**
 * @brief Get cpu usage.
 * Always return NAN if cpu usage monitor api is not initialized.
 * @param core_id (in) CPU core, -1 means all CPU.
 * @return Counter interval in ms.
 * @warning Thread dangerous (untested)
*/
float Util_cpu_usage_monitor_get_cpu_usage(s8 core_id);

#else

#define Util_cpu_usage_monitor_init() Util_return_result_with_string(var_disabled_result)
#define Util_cpu_usage_monitor_exit()
#define Util_cpu_usage_monitor_get_cpu_usage(...) Util_return_double(NAN)

#endif

#endif
