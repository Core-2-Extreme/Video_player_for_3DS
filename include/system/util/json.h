#if !defined(DEF_JSON_HPP)
#define DEF_JSON_HPP
#include <stdbool.h>
#include <stdint.h>
#include "system/util/json_types.h"

#if DEF_JSON_API_ENABLE

/**
 * @brief Parse json data for later use.
 * @param raw_json (in) Pointer for NULL terminated json string.
 * @param parsed_json_data (out) Pointer for parsed JSON data, the user
 * MUST free it by Util_json_free() when it's no longer needed.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_json_parse(const char* raw_json, Json_data* parsed_json_data);

/**
 * @brief Extract data with a key.
 * Two or more dimensional arrays (such as "msg[0][0].gnu") are not supported.
 * @param key (in) Key name.
 * @param parsed_json_data (in) Pointer for parsed JSON data from Util_json_parse().
 * @param extracted_data (out) Pointer for extracted value.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_json_get_data(const char* key, const Json_data* json_data, Json_extracted_data* extracted_data);

/**
 * @brief Free parsed json data.
 * @param parsed_json_data Pointer for parsed JSON data.
 * @note Thread safe.
*/
void Util_json_free(Json_data* parsed_json_data);

#else

#define Util_json_parse(...) DEF_ERR_DISABLED
#define Util_json_get_data(...) DEF_ERR_DISABLED
#define Util_json_free(...)

#endif //DEF_JSON_API_ENABLE

#endif //!defined(DEF_JSON_HPP)
