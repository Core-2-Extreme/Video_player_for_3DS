#pragma once

/**
 * @brief Initialize a hid api.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_hid_init(void);

/**
 * @brief Uninitialize a error API.
 * Do nothing if hid api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_hid_exit(void);

/**
 * @brief Check whether image is pressed.
 * Always return false if hid api is not initialized.
 * @param hid_state (in) Key info returned by Util_hid_query_key_state().
 * @param image (in) Image data.
 * @return Whether image is pressed.
 * @note Thread safe
*/
bool Util_hid_is_pressed(Hid_info hid_state, Image_data image);

/**
 * @brief Check whether image is held.
 * Always return false if hid api is not initialized.
 * @param hid_state (in) Key info returned by Util_hid_query_key_state().
 * @param image (in) Image data.
 * @return Whether image is held.
 * @note Thread safe
*/
bool Util_hid_is_held(Hid_info hid_state, Image_data image);

/**
 * @brief Check whether image is released.
 * Always return false if hid api is not initialized.
 * @param hid_state (in) Key info returned by Util_hid_query_key_state().
 * @param image (in) Image data.
 * @return Whether image is released.
 * @note Thread safe
*/
bool Util_hid_is_released(Hid_info hid_state, Image_data image);

/**
 * @brief Query current key state.
 * @param out_key_state (out) Pointer for key info.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_hid_query_key_state(Hid_info* out_key_state);
