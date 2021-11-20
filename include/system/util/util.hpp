#pragma once

/**
 * @brief Parse a file.
 * @param source_data (in) Text data.
 * @param expected_items (in) Expected elements.
 * @param out_data (out) Array for parsed data.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_parse_file(std::string source_data, int expected_items, std::string out_data[]);

/**
 * @brief Convert seconds to time (hh:mm:ss.ms).
 * @param input_sseconds (in) Seconds.
 * @return Converted time (hh:mm:ss.ms).
 * @note Thread safe
*/
std::string Util_convert_seconds_to_time(double input_seconds);

/**
 * @brief Convert [\\n], ["] and [\\\\] to escape expression.
 * @param in_data (in) Input text.
 * @return Converted text.
 * @note Thread safe
*/
std::string Util_encode_to_escape(std::string in_data);

/**
 * @brief Load a message.
 * @param file_name (in) File name in romfs:/gfx/msg/.
 * @param out_msg (out) Array for parsed message.
 * @param num_of_msg (in) Number of message.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_load_msg(std::string file_name, std::string out_msg[], int num_of_msg);

/**
 * @brief Initialize a safe linear alloc API.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_safe_linear_alloc_init(void);

/**
 * @brief Uninitialize a safe linear alloc API.
 * Do nothing if safe linear alloc api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_safe_linear_alloc_exit(void);

/**
 * @brief Linear alloc.
 * Always return NULL if safe linear alloc api is not initialized.
 * @param size (in) Memory size (in byte).
 * @return On success pointer, on failure NULL.
 * @note Thread safe
*/
void* Util_safe_linear_alloc(size_t size);

/**
 * @brief Linear realloc.
 * Always return NULL if safe linear alloc api is not initialized.
 * @param pointer (in) Old pointer.
 * @param size (in) New memory size (in byte).
 * @return On success pointer, on failure NULL.
 * @note Thread safe
*/
void* Util_safe_linear_realloc(void* pointer, size_t size);

/**
 * @brief Free linear memory.
 * Do nothing if safe linear alloc api is not initialized.
 * @note Thread safe
*/
void Util_safe_linear_free(void* pointer);

/**
 * @brief Check free linear memory size.
 * Always return 0 if safe linear alloc api is not initialized.
 * @return Free linear memory size.
 * @note Thread safe
*/
u32 Util_check_free_linear_space(void);

/**
 * @brief Check free memory size.
 * @return Free memory size.
 * @warning Thread dangerous (untested)
*/
u32 Util_check_free_ram(void);
