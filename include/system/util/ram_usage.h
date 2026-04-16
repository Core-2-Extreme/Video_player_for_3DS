#if !defined(DEF_RAM_USAGE_H)
#define DEF_RAM_USAGE_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/ram_usage_types.h"

#if DEF_RAM_USAGE_API_ENABLE

/**
 * @brief Initialize RAM usage API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_ram_usage_init(void);

/**
 * @brief Uninitialize RAM usage API.
 * Do nothing if RAM usage API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_ram_usage_exit(void);

/**
 * @brief Get RAM usage.
 * Always return NAN if RAM usage API is not initialized.
 * @param is_linear (in) Whether check for linear RAM.
 * @return RAM usage in %.
 * @note Thread safe.
*/
float Util_ram_usage_get_ram_usage(bool is_linear);

/**
 * @brief Get free RAM size.
 * Always return 0 if RAM usage API is not initialized.
 * @param is_linear (in) Whether check for linear RAM.
 * @return Free RAM in bytes.
 * @note Thread safe.
*/
uint32_t Util_ram_usage_get_free_ram(bool is_linear);

/**
 * @brief Get allocated RAM size.
 * Always return 0 if RAM usage API is not initialized.
 * @param is_linear (in) Whether check for linear RAM.
 * @return Allocated RAM in bytes.
 * @note Thread safe.
*/
uint32_t Util_ram_usage_get_used_ram(bool is_linear);

/**
 * @brief Get total RAM size.
 * Always return 0 if RAM usage API is not initialized.
 * @param is_linear (in) Whether check for linear RAM.
 * @return Total RAM in bytes.
 * @note Thread safe.
*/
uint32_t Util_ram_usage_get_total_ram(bool is_linear);

/**
 * @brief Query RAM usage show flag.
 * Always return false if RAM usage API is not initialized.
 * @return Internal RAM usage show flag.
 * @warning Thread dangerous (untested).
*/
bool Util_ram_usage_query_show_flag(void);

/**
 * @brief Set RAM usage show flag.
 * Do nothing if RAM usage API is not initialized.
 * @param flag (in) When true, internal RAM usage show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested).
*/
void Util_ram_usage_set_show_flag(bool flag);

/**
 * @brief Draw RAM usage.
 * @warning Thread dangerous (untested).
 * @warning Call it only from rendering thread.
*/
void Util_ram_usage_draw(void);

#else

#define Util_ram_usage_init() DEF_ERR_DISABLED
#define Util_ram_usage_exit()
#define Util_ram_usage_get_ram_usage(...) NAN
#define Util_ram_usage_get_free_ram(...) 0
#define Util_ram_usage_get_used_ram(...) 0
#define Util_ram_usage_get_total_ram(...) 0
#define Util_ram_usage_query_show_flag() false
#define Util_ram_usage_set_show_flag(...)
#define Util_ram_usage_draw()

#endif //DEF_RAM_USAGE_API_ENABLE

#endif //!defined(DEF_RAM_USAGE_H)
