#if !defined(DEF_CURL_H)
#define DEF_CURL_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/curl_types.h"
#include "system/util/net_types.h"

#if DEF_CURL_API_ENABLE

/**
 * @brief Initialize curl api.
 * @param buffer_size Internal buffer size used by post request.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_curl_init(uint32_t buffer_size);

/**
 * @brief Uninitialize curl API.
 * Do nothing if curl api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_curl_exit(void);

/**
 * @brief Get default user agent.
 * Always return empty string if curl api is not initialized.
 * @return Default user agent.
 * @note Thread safe
*/
const char* Util_curl_get_default_user_agent(void);

/**
 * @brief Make a HTTP get request.
 * @param parameters (in) Parameters.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Util_curl_dl_data(Net_dl_parameters* parameters);

/**
 * @brief Make a HTTP get request and save response to SD card.
 * @param parameters (in) Parameters.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Util_curl_save_data(Net_save_parameters* parameters);

/**
 * @brief Make a HTTP post request.
 * @param parameters (in) Parameters.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Util_curl_post_and_dl_data(Net_post_dl_parameters* parameters);

/**
 * @brief Make a HTTP post request and save response to SD card.
 * @param parameters (in) Parameters.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Util_curl_post_and_save_data(Net_post_save_parameters* parameters);

#else

#define Util_curl_init(...) DEF_ERR_DISABLED
#define Util_curl_exit()
#define Util_curl_get_default_user_agent() ""
#define Util_curl_dl_data(...) DEF_ERR_DISABLED
#define Util_curl_save_data(...) DEF_ERR_DISABLED
#define Util_curl_post_and_dl_data(...) DEF_ERR_DISABLED
#define Util_curl_post_and_save_data(...) DEF_ERR_DISABLED

#endif //DEF_CURL_API_ENABLE

#endif //!defined(DEF_CURL_H)
