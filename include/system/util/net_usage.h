#if !defined(DEF_NET_USAGE_H)
#define DEF_NET_USAGE_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/net_usage_types.h"

#if DEF_NET_USAGE_API_ENABLE

/**
 * @brief Initialize NET usage API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_net_usage_init(void);

/**
 * @brief Uninitialize NET usage API.
 * Do nothing if NET usage API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_net_usage_exit(void);

/**
 * @brief Get NET download speed.
 * Always return 0 if NET usage API is not initialized.
 * @return Network download speed in bytes.
 * @note Thread safe.
*/
uint32_t Util_net_usage_get_net_download_speed(void);

/**
 * @brief Get NET upload speed.
 * Always return 0 if NET usage API is not initialized.
 * @return Network upload speed in bytes.
 * @note Thread safe.
*/
uint32_t Util_net_usage_get_net_upload_speed(void);

/**
 * @brief Query NET usage show flag.
 * Always return false if NET usage API is not initialized.
 * @return Internal NET usage show flag.
 * @warning Thread dangerous (untested).
*/
bool Util_net_usage_query_show_flag(void);

/**
 * @brief Set NET usage show flag.
 * Do nothing if NET usage API is not initialized.
 * @param flag (in) When true, internal NET usage show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested).
*/
void Util_net_usage_set_show_flag(bool flag);

/**
 * @brief Draw NET usage.
 * @warning Thread dangerous (untested).
 * @warning Call it only from rendering thread.
*/
void Util_net_usage_draw(void);

#else

#define Util_net_usage_init() DEF_ERR_DISABLED
#define Util_net_usage_exit()
#define Util_net_usage_get_net_download_speed() 0
#define Util_net_usage_get_net_upload_speed() 0
#define Util_net_usage_query_show_flag() false
#define Util_net_usage_set_show_flag(...)
#define Util_net_usage_draw()

#endif //DEF_NET_USAGE_API_ENABLE

#endif //!defined(DEF_NET_USAGE_H)
