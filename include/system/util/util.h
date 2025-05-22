#if !defined(DEF_UTIL_H)
#define DEF_UTIL_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/str_types.h"

#define DEF_UTIL_LOW_HEAP_THRESHOLD		(uint32_t)(1000 * 1000 * 2.5)

/**
 * @brief Initialize util API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_init(void);

/**
 * @brief Uninitialize util API.
 * Do nothing if util API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_exit(void);

/**
 * @brief Parse a file.
 * @param source_data (in) Text data.
 * @param expected_items (in) Expected elements.
 * @param out_data (out) Array for parsed data.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_parse_file(const char* source_data, uint32_t expected_items, Str_data* out_data);

/**
 * @brief Convert seconds to time (hh:mm:ss.ms).
 * @param input_seconds (in) Seconds.
 * @param time_string (out) Formatted string (hh:mm:ss.ms).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_convert_seconds_to_time(double input_seconds, Str_data* time_string);

/**
 * @brief Load a message.
 * @param file_name (in) File name in romfs:/gfx/msg/.
 * @param out_msg (out) Array for parsed message.
 * @param num_of_msg (in) Number of message.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_load_msg(const char* file_name, Str_data* out_msg, uint32_t num_of_msg);

/**
 * @brief Convert [\n](new line), ["](double quote) and [\](backslash) to escape expression.
 * @param text (in) Input text.
 * @param escaped_text (out) Escaped text.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_encode_to_escape(const char* text, Str_data* escaped_text);

/**
 * @brief Encode text with base64.
 * @param text (in) Text to be encoded.
 * @param encoded_text (out) Encoded text.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_base64_encode(const char* text, Str_data* encoded_text);

/**
 * @brief Decode base64.
 * @param encoded_text (in) Encoded text to be decoded.
 * @param text (out) Decoded text.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_base64_decode(const char* encoded_text, Str_data* text);

/**
 * @brief Check free linear memory size.
 * @return Free linear memory size.
 * @note Thread safe.
*/
uint32_t Util_check_free_linear_space(void);

/**
 * @brief Check free memory size.
 * @return Free memory size.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_check_free_ram(void);

/**
 * @brief Check if specified core is available (can make a thread).
 * Always return false if util API is not initialized.
 * @return True if core is available, otherwise false.
 * @note Thread safe.
*/
bool Util_is_core_available(uint8_t core_id);

/**
 * @brief Sleep thread.
 * @param us (in) Time to sleep in us.
 * @note Thread safe.
*/
void Util_sleep(uint64_t us);

/**
 * @brief Compare values and return minimum value.
 * @param value_0 (in) Value to compare.
 * @param value_1 (in) Value to compare.
 * @return Minimum value.
 * @note Thread safe.
*/
int64_t Util_min(int64_t value_0, int64_t value_1);

/**
 * @brief Compare values and return maximum value.
 * @param value_0 (in) Value to compare.
 * @param value_1 (in) Value to compare.
 * @return Maximum value.
 * @note Thread safe.
*/
int64_t Util_max(int64_t value_0, int64_t value_1);

/**
 * @brief Compare double values and return minimum value.
 * @param value_0 (in) Value to compare.
 * @param value_1 (in) Value to compare.
 * @return Minimum value.
 * @note Thread safe.
*/
double Util_min_d(double value_0, double value_1);

/**
 * @brief Compare double values and return maximum value.
 * @param value_0 (in) Value to compare.
 * @param value_1 (in) Value to compare.
 * @return Maximum value.
 * @note Thread safe.
*/
double Util_max_d(double value_0, double value_1);

/**
 * @brief Get difference (distance) between new value and old value.
 * @param new_value (in) New value to use.
 * @param old_value (in) Old value to use.
 * @param max_value (in) Maximum value before wrapping to 0 (e.g. if max_value == 213, value range is 0-213).
 * @return Difference (distance between new and old).
 * @note Thread safe.
*/
uint64_t Util_get_diff(uint64_t new_value, uint64_t old_value, uint64_t max_value);

#endif //!defined(DEF_UTIL_H)
