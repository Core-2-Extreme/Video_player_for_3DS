#if !defined(DEF_KEYBOARD_H)
#define DEF_KEYBOARD_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/str_types.h"
#include "system/util/keyboard_types.h"

#if DEF_KEYBOARD_API_ENABLE

/**
 * @brief Initialize a software keyboard.
 * @param type (in) Software keyboard type.
 * @param valid_type (in) Accepted input type.
 * @param button_type (in) Buttons to display.
 * @param max_length (in) Max input length in number of characters, NOT in bytes.
 * @param hint_text (in) Hint text.
 * @param init_text (in) Initial text.
 * @param password_mode (in) Password mode.
 * @param feature (in) Software keyboard feature.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_keyboard_init(Keyboard_type type, Keyboard_acceptable_input valid_type, Keyboard_display_button button_type,
uint32_t max_length, const Str_data* hint_text, const Str_data* init_text, Keyboard_password_mode password_mode, Keyboard_features_bit features);

/**
 * @brief Set dictionary word.
 * @param first_spell (in) Array for word's spell.
 * @param full_spell (in) Array for word's full spell.
 * @param num_of_word (in) Number of words.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_keyboard_set_dic_word(Str_data* first_spell, Str_data* full_spell, uint16_t num_of_word);

/**
 * @brief Launch software keyboard.
 * @param out_data (out) Pointer for user input text.
 * @param pressed_button (out) Pressed button, can be NULL.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Call it only from rendering thread.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_keyboard_launch(Str_data* out_data, Keyboard_button* pressed_button);

/**
 * @brief Uninitialize a software keyboard.
 * Do nothing if keyboard API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_keyboard_exit(void);

#else

#define Util_keyboard_init(...) DEF_ERR_DISABLED
#define Util_keyboard_set_dic_word(...) DEF_ERR_DISABLED
#define Util_keyboard_launch(...) DEF_ERR_DISABLED
#define Util_keyboard_exit()

#endif //DEF_KEYBOARD_API_ENABLE

#endif //!defined(DEF_KEYBOARD_H)
