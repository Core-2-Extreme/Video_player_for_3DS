#ifndef ERROR_HPP
#define ERROR_HPP

#include "system/types.hpp"

/**
 * @brief Initialize a error api.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_err_init(void);

/**
 * @brief Uninitialize a error API.
 * Do nothing if error api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_err_exit(void);

/**
 * @brief Query error show flag.
 * Always return false if error api is not initialized.
 * @return Internal error show flag.
 * @warning Thread dangerous (untested)
*/
bool Util_err_query_error_show_flag(void);

/**
 * @brief Set error message.
 * Do nothing if error api is not initialized.
 * @param summary (in) Error summary.
 * @param description (in) Error description.
 * @param place (in) Error place.
 * @warning Thread dangerous (untested)
*/
void Util_err_set_error_message(std::string summary, std::string description, std::string place);

/**
 * @brief Set error message.
 * Do nothing if error api is not initialized.
 * @param summary (in) Error summary.
 * @param description (in) Error description.
 * @param place (in) Error place.
 * @param error_code (in) Error code.
 * @warning Thread dangerous (untested)
*/
void Util_err_set_error_message(std::string summary, std::string description, std::string place, int error_code);

/**
 * @brief Set error show flag.
 * Do nothing if error api is not initialized.
 * @param flag (in) When true, internal error show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested)
*/
void Util_err_set_error_show_flag(bool flag);

/**
 * @brief Clear error message.
 * Do nothing if error api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_err_clear_error_message(void);

/**
 * @brief Save error message to SD card.
 * After saving error message, error show flag will be set to false.
 * Do nothing if error api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_err_save_error(void);

/**
 * @brief Process user input for Util_err_draw().
 * @param key (in) key info returned by Util_hid_query_key_state().
 * @warning Thread dangerous (untested)
*/
void Util_err_main(Hid_info key);

/**
 * @brief Draw error message.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Util_err_draw(void);

#endif
