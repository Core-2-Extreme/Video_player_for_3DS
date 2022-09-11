#pragma once

#if DEF_ENABLE_EXPL_API

/**
 * @brief Initialize a explorer api.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_expl_init(void);

/**
 * @brief Uninitialize a explorer API.
 * Do nothing if explorer api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_expl_exit(void);

/**
 * @brief Query current directory path.
 * Always return empty string if explorer api is not initialized.
 * @return Current directory name.
 * @warning Thread dangerous (untested)
*/
std::string Util_expl_query_current_dir(void);

/**
 * @brief Query num of file in current directory.
 * Always return 0 if explorer api is not initialized.
 * @return Num of file in current directory.
 * @warning Thread dangerous (untested)
*/
int Util_expl_query_num_of_file(void);

/**
 * @brief Query current file index.
 * Always return -1 if explorer api is not initialized.
 * @return Current (selected) file index.
 * @warning Thread dangerous (untested)
*/
int Util_expl_query_current_file_index(void);

/**
 * @brief Query file name.
 * Always return empty string if explorer api is not initialized.
 * @param index (in) File index.
 * @return File name.
 * @warning Thread dangerous (untested)
*/
std::string Util_expl_query_file_name(int index);

/**
 * @brief Query file size.
 * Always return 0 if explorer api is not initialized.
 * @param index (in) File index.
 * @return File size.
 * @warning Thread dangerous (untested)
*/
int Util_expl_query_size(int index);

/**
 * @brief Query file type.
 * Always return DEF_EXPL_TYPE_UNKNOWN if explorer api is not initialized.
 * @param index (in) File index.
 * @return File type (combination of DEF_EXPL_TYPE_*).
 * @warning Thread dangerous (untested)
*/
int Util_expl_query_type(int index);

/**
 * @brief Query explorer show flag.
 * Always return false if explorer api is not initialized.
 * @return Internal explorer show flag.
 * @warning Thread dangerous (untested)
*/
bool Util_expl_query_show_flag(void);

/**
 * @brief Set call back for file selection.
 * Do nothing if explorer api is not initialized.
 * @param callback (in) Call back for file selection.
 * @warning Thread dangerous (untested)
*/
void Util_expl_set_callback(void (*callback)(std::string file, std::string dir));

/**
 * @brief Set call back for cancellation.
 * Do nothing if explorer api is not initialized.
 * @param callback (in) Call back for cancellation.
 * @warning Thread dangerous (untested)
*/
void Util_expl_set_cancel_callback(void (*callback)(void));

/**
 * @brief Set current directory.
 * Do nothing if explorer api is not initialized.
 * @param dir (in) Directory name.
 * @warning Thread dangerous (untested)
*/
void Util_expl_set_current_dir(std::string dir);

/**
 * @brief Set explorer show flag.
 * Do nothing if explorer api is not initialized.
 * @param flag (in) When true, internal explorer show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested)
*/
void Util_expl_set_show_flag(bool flag);

/**
 * @brief Draw explorer.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Util_expl_draw(void);

/**
 * @brief Process user input for Util_expl_draw().
 * @param key (in) key info returned by Util_hid_query_key_state().
 * @warning Thread dangerous (untested)
*/
void Util_expl_main(Hid_info key);

#else

#define Util_expl_init(...) Util_return_result_with_string(var_disabled_result)
#define Util_expl_exit(...) 
#define Util_expl_query_current_dir(...) Util_return_string("")
#define Util_expl_query_num_of_file(...) Util_return_int(0)
#define Util_expl_query_current_file_index(...) Util_return_int(-1)
#define Util_expl_query_file_name(...) Util_return_string("")
#define Util_expl_query_size(...) Util_return_int(0)
#define Util_expl_query_type(...) Util_return_int(DEF_EXPL_TYPE_UNKNOWN)
#define Util_expl_query_show_flag(...) Util_return_bool(false)
#define Util_expl_set_callback(...) 
#define Util_expl_set_cancel_callback(...) 
#define Util_expl_set_current_dir(...) 
#define Util_expl_set_show_flag(...) 
#define Util_expl_draw(...) 
#define Util_expl_main(...) 

#endif
