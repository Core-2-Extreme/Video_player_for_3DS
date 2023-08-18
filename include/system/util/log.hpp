#ifndef LOG_HPP
#define LOG_HPP

#include "system/types.hpp"

/**
 * @brief Initialize a log api.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_log_init(void);

/**
 * @brief Uninitialize a log API.
 * Do nothing if log api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_log_exit(void);

/**
 * @brief Dump log data to a file.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_log_dump(std::string file_name, std::string dir_path);

/**
 * @brief Query log show flag.
 * Always return false if log api is not initialized.
 * @return Internal log show flag.
 * @warning Thread dangerous (untested)
*/
bool Util_log_query_log_show_flag(void);

/**
 * @brief Set log show flag.
 * Do nothing if log api is not initialized.
 * @param flag (in) When true, internal log show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested)
*/
void Util_log_set_log_show_flag(bool flag);

/**
 * @brief Save new log.
 * Always return -1 if log api is not initialized.
 * @param place (in) Place where this log written from.
 * @param text (in) Log text.
 * @return Internal log index that can be used for Util_log_add().
 * @note Thread safe
*/
int Util_log_save(std::string place, std::string text);

/**
 * @brief Save new log.
 * Always return -1 if log api is not initialized.
 * @param place (in) Place where this log written from.
 * @param text (in) Log text.
 * @param result (in) Result code.
 * @return Internal log index that can be used for Util_log_add().
 * @note Thread safe
*/
int Util_log_save(std::string place, std::string text, int result);

/**
 * @brief Add to existing log.
 * Do nothing if log api is not initialized.
 * @param log_index (in) Log index returned by Util_log_save().
 * @param text (in) Log text.
 * @param result (in) Result code.
 * @note Thread safe
*/
void Util_log_add(int log_index, std::string text);

/**
 * @brief Add to existing log.
 * Do nothing if log api is not initialized.
 * @param log_index (in) Log index returned by Util_log_save().
 * @param text (in) Log text.
 * @param result (in) Result code.
 * @note Thread safe
*/
void Util_log_add(int log_index, std::string text, int result);

/**
 * @brief Process user input for Util_log_draw().
 * @param key (in) key info returned by Util_hid_query_key_state().
 * @warning Thread dangerous (untested)
*/
void Util_log_main(Hid_info key);

/**
 * @brief Draw logs.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Util_log_draw(void);

#endif
