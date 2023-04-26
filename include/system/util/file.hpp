#ifndef FILE_HPP
#define FILE_HPP

#include "system/types.hpp"

/**
 * @brief Save data to a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param write_data (in) Pointer for write data.
 * @param size (in) Write data size (in byte).
 * @param delete_old_file (in) When true, old file will be deleted if exist otherwise data will be written end of the file.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_save_to_file(std::string file_name, std::string dir_path, u8* write_data, int size, bool delete_old_file);

/**
 * @brief Load data from a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param read_data (out) Pointer for read data, the pointer will be allocated up to max_size+1 
 * (for null terminator) depends on file size inside of function.
 * @param max_size (in) Max read size (in byte).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_load_from_file(std::string file_name, std::string dir_path, u8** read_data, int max_size);

/**
 * @brief Load data from a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param read_data (out) Pointer for read data, the pointer will be allocated up to max_size+1 
 * (for null terminator) depends on file size inside of function.
 * @param max_size (in) Max read size (in byte).
 * @param read_size (out) Actuall read size (in byte).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_load_from_file(std::string file_name, std::string dir_path, u8** read_data, int max_size, u32* read_size);

/**
 * @brief Load data from a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param read_data (out) Pointer for read data, the pointer will be allocated up to max_size+1 
 * (for null terminator) depends on file size inside of function.
 * @param max_size (in) Max read size (in byte).
 * @param read_offset (in) Read offset (in byte).
 * @param read_size (out) Actuall read size (in byte).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_load_from_file_with_range(std::string file_name, std::string dir_path, u8** read_data, int max_size, u64 read_offset, u32* read_size);

/**
 * @brief Load data from a file in romfs:/.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param read_data (out) Pointer for read data, the pointer will be allocated up to max_size+1 
 * (for null terminator) depends on file size inside of function.
 * @param max_size (in) Max read size (in byte).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_file_load_from_rom(std::string file_name, std::string dir_path, u8** read_data, int max_size);

/**
 * @brief Load data from a file in romfs:/.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param read_data (out) Pointer for read data, the pointer will be allocated up to max_size+1 
 * (for null terminator) depends on file size inside of function.
 * @param max_size (in) Max read size (in byte).
 * @param read_size (out) Actuall read size (in byte).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_file_load_from_rom(std::string file_name, std::string dir_path, u8** read_data, int max_size, u32* read_size);

/**
 * @brief Delete a file.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_delete_file(std::string file_name, std::string dir_path);

/**
 * @brief Check file size.
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @param file_size (out) File size.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_check_file_size(std::string file_name, std::string dir_path, u64* file_size);

/**
 * @brief Check if a file exist.
 * If file exist, return DEF_SUCCESS
 * @param file_name (in) File name.
 * @param dir_path (in) Directory path.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_check_file_exist(std::string file_name, std::string dir_path);

/**
 * @brief Read all file's name and type in directory.
 * @param dir_path (in) Directory path.
 * @param detected (out) Number of file in directory.
 * @param file_name (out) Array for file name.
 * @param type (out) Array for file type.
 * @param array_length (in) Array length for file name and file type.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe
*/
Result_with_string Util_file_read_dir(std::string dir_path, int* detected, std::string file_name[], File_type type[], int array_length);

#endif
