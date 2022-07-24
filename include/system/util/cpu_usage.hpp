#pragma once

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
