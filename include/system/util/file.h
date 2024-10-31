#if !defined(DEF_FILE_H)
#define DEF_FILE_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/expl_types.h"
#include "system/util/str_types.h"

/**
 * @brief Save data to a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param write_data (in) Pointer for write data.
 * @param size (in) Write data size (in byte).
 * @param delete_old_file (in) When true, old file will be deleted if exist otherwise data will be written end of the file.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
uint32_t Util_file_save_to_file(const char* file_name, const char* dir_path, const uint8_t* write_data, uint32_t size, bool delete_old_file);

/**
 * @brief Load data from a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param read_data (out) Pointer for read data, the pointer will be allocated up to max_size+1
 * (for null terminator) depends on file size inside of function.
 * @param max_size (in) Max read size (in byte).
 * @param read_offset (in) Read offset (in byte).
 * @param read_size (out) Actuall read size (in byte).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
uint32_t Util_file_load_from_file(const char* file_name, const char* dir_path, uint8_t** read_data, uint32_t max_size, uint64_t read_offset, uint32_t* read_size);

/**
 * @brief Load data from a file in romfs:/.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param read_data (out) Pointer for read data, the pointer will be allocated up to max_size+1
 * (for null terminator) depends on file size inside of function.
 * @param max_size (in) Max read size (in byte).
 * @param read_size (out) Actuall read size (in byte).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Util_file_load_from_rom(const char* file_name, const char* dir_path, uint8_t** read_data, uint32_t max_size, uint32_t* read_size);

/**
 * @brief Delete a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
uint32_t Util_file_delete_file(const char* file_name, const char* dir_path);

/**
 * @brief Check file size.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param file_size (out) File size.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
uint32_t Util_file_check_file_size(const char* file_name, const char* dir_path, uint64_t* file_size);

/**
 * @brief Check if a file exist.
 * If file exist, return DEF_SUCCESS
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
uint32_t Util_file_check_file_exist(const char* file_name, const char* dir_path);

/**
 * @brief Read all files name and type in directory.
 * @param dir_path (in) Directory path.
 * @param detected (out) Number of files in directory.
 * @param file_name (out) Array for file name.
 * @param type (out) Array for file type.
 * @param array_length (in) Array length for file name and file type.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
uint32_t Util_file_read_dir(const char* dir_path, uint32_t* detected, Str_data* file_name, Expl_file_type* type, uint32_t array_length);

#endif //!defined(DEF_FILE_H)
