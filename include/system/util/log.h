#if !defined(DEF_LOG_H)
#define DEF_LOG_H
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "system/util/hid_types.h"
#include "system/util/log_enum_types.h"
#include "system/util/log_types.h"

/**
 * @brief Initialize a log API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_log_init(void);

/**
 * @brief Uninitialize a log API.
 * Do nothing if log API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_log_exit(void);

/**
 * @brief Dump log data to a file.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_log_dump(const char* file_name, const char* dir_path);

/**
 * @brief Query log show flag.
 * Always return false if log API is not initialized.
 * @return Internal log show flag.
 * @warning Thread dangerous (untested).
*/
bool Util_log_query_show_flag(void);

/**
 * @brief Set log show flag.
 * Do nothing if log API is not initialized.
 * @param flag (in) When true, internal log show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested).
*/
void Util_log_set_show_flag(bool flag);

/**
 * @brief Save a new log.
 * It's recommended to use DEF_LOG_FORMAT() or DEF_LOG_VFORMAT().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param caller (in) Caller name, can be NULL.
 * @param format_string (in) String format.
 * @param additional args (in) Additional args if any.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_format(const char* caller, const char* format_string, ...);
uint32_t Util_log_vformat(const char* caller, const char* format_string, va_list args);

/**
 * @brief Append a log.
 * It's recommended to use DEF_LOG_FORMAT_APPEND() or DEF_LOG_VFORMAT_APPEND().
 * Calling this function with log_index == DEF_LOG_INDEX_AUTO
 * is same as calling Util_log_(v)format.
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param log_index (in) Log index to append log to.
 * @param append_elapsed_time (in) Whether append elapsed time since first call to Util_log_(v)format() (with the same log_index).
 * @param format_string (in) String format.
 * @param additional args (in) Additional args if any.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_format_append(uint32_t log_index, bool append_elapsed_time, const char* format_string, ...);
uint32_t Util_log_vformat_append(uint32_t log_index, bool append_elapsed_time, const char* format_string, va_list args);

/**
 * @brief Save a function result.
 * It's recommended to use DEF_LOG_RESULT().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param caller (in) Caller name, can be NULL.
 * @param function_name (in) Human friendly function name, can be NULL.
 * @param is_success (in) Whether result is success.
 * @param result (in) Result code.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_result(const char* caller, const char* function_name, bool is_success, uint32_t result);

/**
 * @brief Save a start-of-function-call.
 * It's recommended to use DEF_LOG_RESULT_SMART or DEF_LOG_RESULT_START().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param caller (in) Caller name, can be NULL.
 * @param function_name (in) Human friendly function name, can be NULL.
 * @param is_smart_macro (in) Used by DEF_LOG_RESULT_SMART(), should be false.
 * @param omit_args (in) Whether omit args from function_name.
 * @return Internal log index that can be used for Util_log_save_result_end().
 * @note Thread safe.
*/
uint32_t Util_log_save_result_start(const char* caller, const char* function_name, bool is_smart_macro, bool omit_args);

/**
 * @brief Save a end-of-function-call (with elapsed time).
 * It's recommended to use DEF_LOG_RESULT_SMART or DEF_LOG_RESULT_END().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param log_index (in) Log index to append log to, usually returned by Util_log_save_result_start().
 * @param is_success (in) Whether result is success.
 * @param result (in) Result code.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_result_end(uint32_t log_index, bool is_success, uint32_t result);

/**
 * @brief Save a bool value.
 * It's recommended to use DEF_LOG_BOOL().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param caller (in) Caller name, can be NULL.
 * @param symbol_name (in) Human friendly symbol name, can be NULL.
 * @param value (in) Value to log.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_bool(const char* caller, const char* symbol_name, bool value);

/**
 * @brief Save a int/int8_t/int16_t/int32_t/int64_t value.
 * It's recommended to use DEF_LOG_INT().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param caller (in) Caller name, can be NULL.
 * @param symbol_name (in) Human friendly symbol name, can be NULL.
 * @param value (in) Value to log.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_int(const char* caller, const char* symbol_name, int64_t value);

/**
 * @brief Save a unsigned int/uint8_t/uint16_t/uint32_t/uint64_t value.
 * It's recommended to use DEF_LOG_UINT().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param caller (in) Caller name, can be NULL.
 * @param symbol_name (in) Human friendly symbol name, can be NULL.
 * @param value (in) Value to log.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_uint(const char* caller, const char* symbol_name, uint64_t value);

/**
 * @brief Save a unsigned int/uint8_t/uint16_t/uint32_t/uint64_t value as a hex format.
 * It's recommended to use DEF_LOG_HEX().
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * @param caller (in) Caller name, can be NULL.
 * @param symbol_name (in) Human friendly symbol name, can be NULL.
 * @param value (in) Value to log.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_hex(const char* caller, const char* symbol_name, uint64_t value);

/**
 * @brief Save a float/double value.
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * It's recommended to use DEF_LOG_DOUBLE().
 * @param caller (in) Caller name, can be NULL.
 * @param symbol_name (in) Human friendly symbol name, can be NULL.
 * @param value (in) Value to log.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_double(const char* caller, const char* symbol_name, double value);

/**
 * @brief Save a string.
 * Always return DEF_LOG_INDEX_AUTO if log API is not initialized.
 * It's recommended to use DEF_LOG_STRING().
 * @param caller (in) Caller name, can be NULL.
 * @param symbol_name (in) Human friendly symbol name, can be NULL.
 * @param text (in) Text to log.
 * @return Internal log index that can be used for Util_log_(v)format_append().
 * @note Thread safe.
*/
uint32_t Util_log_save_string(const char* caller, const char* symbol_name, const char* text);

//Useful log macros.
#define DEF_LOG_FORMAT(...) Util_log_format(__func__, __VA_ARGS__)
#define DEF_LOG_VFORMAT(format, va_arg_list) Util_log_vformat(__func__, format, va_arg_list)
#define DEF_LOG_FORMAT_APPEND(log_index, append_time, ...) Util_log_format_append(log_index, append_time, __VA_ARGS__)
#define DEF_LOG_VFORMAT_APPEND(log_index, append_time, format, va_arg_list) Util_log_vformat_append(log_index, append_time, format, va_arg_list)
#define DEF_LOG_RESULT(function_name, is_success, result) Util_log_save_result(__func__, #function_name, is_success, result)
#define DEF_LOG_RESULT_START(function_name) Util_log_save_result_start(__func__, #function_name, false, true)
#define DEF_LOG_RESULT_END(log_index, is_success, result) Util_log_save_result_end(log_index, is_success, result)
#define DEF_LOG_RESULT_SMART(result_out, function_call, is_success, log_api_result) \
{ \
	uint32_t _macro_log_index_ = Util_log_save_result_start(__func__, #function_call, true, true); \
	result_out = function_call; \
	Util_log_save_result_end(_macro_log_index_, is_success, log_api_result); \
}
#define DEF_LOG_RESULT_SMART_FULL(result_out, function_call, is_success, log_api_result) \
{ \
	uint32_t _macro_log_index_ = Util_log_save_result_start(__func__, #function_call, true, false); \
	result_out = function_call; \
	Util_log_save_result_end(_macro_log_index_, is_success, log_api_result); \
}
#define DEF_LOG_BOOL(value) Util_log_save_bool(__func__, #value, value)
#define DEF_LOG_INT(value) Util_log_save_int(__func__, #value, value)
#define DEF_LOG_UINT(value) Util_log_save_uint(__func__, #value, value)
#define DEF_LOG_HEX(value) Util_log_save_hex(__func__, #value, value)
#define DEF_LOG_DOUBLE(value) Util_log_save_double(__func__, #value, value)
#define DEF_LOG_STRING(text) Util_log_save_string(__func__, #text, text)

/**
 * @brief Process user input for Util_log_draw().
 * @param key (in) key info returned by Util_hid_query_key_state().
 * @warning Thread dangerous (untested).
*/
void Util_log_main(Hid_info key);

/**
 * @brief Draw logs.
 * @warning Thread dangerous (untested).
 * @warning Call it only from rendering thread.
*/
void Util_log_draw(void);

#endif //!defined(DEF_LOG_H)
