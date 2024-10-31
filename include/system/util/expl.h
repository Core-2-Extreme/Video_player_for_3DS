#if !defined(DEF_EXPL_H)
#define DEF_EXPL_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/expl_types.h"
#include "system/util/hid_types.h"
#include "system/util/str_types.h"

#if DEF_EXPL_API_ENABLE

/**
 * @brief Initialize a explorer api.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_expl_init(void);

/**
 * @brief Uninitialize a explorer API.
 * Do nothing if explorer api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_expl_exit(void);

/**
 * @brief Query current directory path.
 * @param dir_name (out) current directory name.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_expl_query_current_dir(Str_data* dir_name);

/**
 * @brief Query num of file in current directory.
 * Always return 0 if explorer api is not initialized.
 * @return Num of file in current directory.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_expl_query_num_of_file(void);

/**
 * @brief Query current file index.
 * Always return DEF_EXPL_INVALID_INDEX if explorer api is not initialized.
 * @return Current (selected) file index.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_expl_query_current_file_index(void);

/**
 * @brief Query file name.
 * @param index (in) File index.
 * @param file_name (out) current directory name.
 * @return File name.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_expl_query_file_name(uint32_t index, Str_data* file_name);

/**
 * @brief Query file size.
 * Always return 0 if explorer api is not initialized.
 * @param index (in) File index.
 * @return File size.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_expl_query_size(uint32_t index);

/**
 * @brief Query file type.
 * Always return EXPL_FILE_TYPE_NONE if explorer api is not initialized.
 * @param index (in) File index.
 * @return File type.
 * @warning Thread dangerous (untested)
*/
Expl_file_type Util_expl_query_type(uint32_t index);

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
void Util_expl_set_callback(void (*const callback)(Str_data* file, Str_data* dir));

/**
 * @brief Set call back for cancellation.
 * Do nothing if explorer api is not initialized.
 * @param callback (in) Call back for cancellation.
 * @warning Thread dangerous (untested)
*/
void Util_expl_set_cancel_callback(void (*const callback)(void));

/**
 * @brief Set current directory.
 * Do nothing if explorer api is not initialized.
 * @param dir_name (in) Directory name.
 * @warning Thread dangerous (untested)
*/
void Util_expl_set_current_dir(const Str_data* dir_name);

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
 * @warning Call it only from rendering thread.
*/
void Util_expl_draw(void);

/**
 * @brief Process user input for Util_expl_draw().
 * @param key (in) key info returned by Util_hid_query_key_state().
 * @param scroll_speed (in) Scroll sensitivity.
 * @warning Thread dangerous (untested)
*/
void Util_expl_main(Hid_info key, double scroll_speed);

#else

#define Util_expl_init() DEF_ERR_DISABLED
#define Util_expl_exit()
#define Util_expl_query_current_dir() NULL
#define Util_expl_query_num_of_file() 0
#define Util_expl_query_current_file_index() UINT32_MAX
#define Util_expl_query_file_name(...) NULL
#define Util_expl_query_size(...) 0
#define Util_expl_query_type(...) EXPL_FILE_TYPE_NONE
#define Util_expl_query_show_flag() false
#define Util_expl_set_callback(...)
#define Util_expl_set_cancel_callback(...)
#define Util_expl_set_current_dir(...)
#define Util_expl_set_show_flag(...)
#define Util_expl_draw()
#define Util_expl_main(...)

#endif //DEF_EXPL_API_ENABLE

#endif //!defined(DEF_EXPL_H)
