#ifndef SWKBD_HPP
#define SWKBD_HPP

#if DEF_ENABLE_SWKBD_API
#include "system/types.hpp"

/**
 * @brief Initialize a software keyboard.
 * @param type (in) Software keyboard type.
 * @param valid_type (in) Accepted input type.
 * @param num_of_button (in) Number of button (1 ~ 3).
 * @param max_length (in) Max input length.
 * @param hint_text (in) Hint text.
 * @param init_text (in) Initial text.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text);

/**
 * @brief Initialize a software keyboard.
 * @param type (in) Software keyboard type.
 * @param valid_type (in) Accepted input type.
 * @param num_of_button (in) Number of button (1 ~ 3).
 * @param max_length (in) Max input length.
 * @param hint_text (in) Hint text.
 * @param init_text (in) Initial text.
 * @param feature (in) Software keyboard feature.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text,
u32 feature);

/**
 * @brief Initialize a software keyboard.
 * @param type (in) Software keyboard type.
 * @param valid_type (in) Accepted input type.
 * @param num_of_button (in) Number of button (1 ~ 3).
 * @param max_length (in) Max input length.
 * @param hint_text (in) Hint text.
 * @param init_text (in) Initial text.
 * @param password_mode (in) Password mode.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text,
SwkbdPasswordMode password_mode);

/**
 * @brief Initialize a software keyboard.
 * @param type (in) Software keyboard type.
 * @param valid_type (in) Accepted input type.
 * @param num_of_button (in) Number of button (1 ~ 3).
 * @param max_length (in) Max input length.
 * @param hint_text (in) Hint text.
 * @param init_text (in) Initial text.
 * @param password_mode (in) Password mode.
 * @param feature (in) Software keyboard feature.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text,
SwkbdPasswordMode password_mode, u32 feature);

/**
 * @brief Set dictionary word.
 * @param first_spell (in) Array for word's spell.
 * @param full_spell (in) Array for word's full spell.
 * @param num_of_word (in) Number of word.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_swkbd_set_dic_word(std::string first_spell[], std::string full_spell[], int num_of_word);

/**
 * @brief Launch software keyboard.
 * @param out_data (out) Pointer for user input text.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Call it from only drawing thread.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_swkbd_launch(std::string* out_data);

/**
 * @brief Launch software keyboard.
 * @param out_data (out) Pointer for user input text.
 * @param pressed_button (out) Pressed button.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Call it from only drawing thread.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_swkbd_launch(std::string* out_data, Keyboard_button* pressed_button);

/**
 * @brief Uninitialize a software keyboard.
 * Do nothing if swkbd api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_swkbd_exit(void);

#else

#define Util_swkbd_init(...) Util_return_result_with_string(var_disabled_result)
#define Util_swkbd_set_dic_word(...) Util_return_result_with_string(var_disabled_result)
#define Util_swkbd_launch(...) Util_return_result_with_string(var_disabled_result)
#define Util_swkbd_exit()

#endif

#endif
