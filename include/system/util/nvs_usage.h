#if !defined(DEF_NVS_USAGE_H)
#define DEF_NVS_USAGE_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/nvs_usage_types.h"

#if DEF_NVS_USAGE_API_ENABLE

/**
 * @brief Initialize NVS usage API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_nvs_usage_init(void);

/**
 * @brief Uninitialize NVS usage API.
 * Do nothing if NVS usage API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_nvs_usage_exit(void);

/**
 * @brief Get NVS usage.
 * Always return NAN if NVS usage API is not initialized.
 * @return NVS usage in %.
 * @note Thread safe.
*/
float Util_nvs_usage_get_nvs_usage(void);

/**
 * @brief Get NVS read speed.
 * Always return 0 if NVS usage API is not initialized.
 * @return NVS read speed in bytes.
 * @note Thread safe.
*/
uint32_t Util_nvs_usage_get_nvs_read_speed(void);

/**
 * @brief Get NVS write speed.
 * Always return 0 if NVS usage API is not initialized.
 * @return NVS write speed in bytes.
 * @note Thread safe.
*/
uint32_t Util_nvs_usage_get_nvs_write_speed(void);

/**
 * @brief Query NVS usage show flag.
 * Always return false if NVS usage API is not initialized.
 * @return Internal NVS usage show flag.
 * @warning Thread dangerous (untested).
*/
bool Util_nvs_usage_query_show_flag(void);

/**
 * @brief Set NVS usage show flag.
 * Do nothing if NVS usage API is not initialized.
 * @param flag (in) When true, internal NVS usage show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested).
*/
void Util_nvs_usage_set_show_flag(bool flag);

/**
 * @brief Draw NVS usage.
 * @warning Thread dangerous (untested).
 * @warning Call it only from rendering thread.
*/
void Util_nvs_usage_draw(void);

#else

#define Util_nvs_usage_init() DEF_ERR_DISABLED
#define Util_nvs_usage_exit()
#define Util_nvs_usage_get_nvs_usage() NAN
#define Util_nvs_usage_get_nvs_read_speed() 0
#define Util_nvs_usage_get_nvs_write_speed() 0
#define Util_nvs_usage_query_show_flag() false
#define Util_nvs_usage_set_show_flag(...)
#define Util_nvs_usage_draw()

#endif //DEF_NVS_USAGE_API_ENABLE

#endif //!defined(DEF_NVS_USAGE_H)
