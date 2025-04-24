#if !defined(DEF_STR_H)
#define DEF_STR_H
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "system/util/str_types.h"

/**
 * @brief Initialize a string struct.
 * @param string (in/out) Pointer for target struct.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_init(Str_data* string);

/**
 * @brief Free a string struct.
 * @param string (in/out) Pointer for target struct.
 * @note Thread safe.
*/
void Util_str_free(Str_data* string);

/**
 * @brief Clear string data.
 * @param string (in/out) Pointer for target struct.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_clear(Str_data* string);

/**
 * @brief Set string data.
 * @param string (in/out) Pointer for target struct.
 * @param source_string (in) Pointer for source string.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_set(Str_data* string, const char* source_string);

/**
 * @brief Add (append) string data.
 * @param string (in/out) Pointer for target struct.
 * @param source_string (in) Pointer for source string to add.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_add(Str_data* string, const char* source_string);

/**
 * @brief Set string data with format.
 * @param string (in/out) Pointer for target struct.
 * @param format_string (in) Pointer for format string.
 * @param additional_parameters (in) Additional parameters for format_string.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_format(Str_data* string, const char* format_string, ...) DEF_STR_GCC_FMT_CHECK;

/**
 * @brief va_list version of Util_str_format().
 * @param string (in/out) Pointer for target struct.
 * @param format_string (in) Pointer for format string.
 * @param args (in) Additional parameters for format_string.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_vformat(Str_data* string, const char* format_string, va_list args);

/**
 * @brief Same as Util_str_format() except this will append text instead of overwrite old one.
 * @param string (in/out) Pointer for target struct.
 * @param format_string (in) Pointer for format string.
 * @param additional_parameters (in) Additional parameters for format_string.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_format_append(Str_data* string, const char* format_string, ...) DEF_STR_GCC_FMT_CHECK;

/**
 * @brief va_list version of Util_str_format_append().
 * @param string (in/out) Pointer for target struct.
 * @param format_string (in) Pointer for format string.
 * @param args (in) Additional parameters for format_string.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_vformat_append(Str_data* string, const char* format_string, va_list args);

/**
 * @brief Resize string buffer size.
 * @note If new_capacity is less than current string length,
 * string data WILL BE TRUNCATED to length of new_capacity.
 * @param string (in/out) Pointer for target struct.
 * @param new_capacity (in) New buffer capacity.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_str_resize(Str_data* string, uint32_t new_capacity);

/**
 * @brief Check if struct is valid (so that safe to access the buffer).
 * @note If string is empty (buffer is allocated but string length is 0)
 * this will return true.
 * @param string (in) Pointer for target struct.
 * @return True if struct is valid, otherwise false.
 * @note Thread safe.
*/
bool Util_str_is_valid(const Str_data* string);

/**
 * @brief Check if struct is valid and contains at least 1 character.
 * @note If string is empty (buffer is allocated but string length is 0)
 * this will return false.
 * @param string (in) Pointer for target struct.
 * @return True if struct is valid AND contains at least 1 character, otherwise false.
 * @note Thread safe.
*/
bool Util_str_has_data(const Str_data* string);

#endif //!defined(DEF_STR_H)
