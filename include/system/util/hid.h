#if !defined(DEF_HID_H)
#define DEF_HID_H
#include <stdbool.h>
#include <stdint.h>
#include "system/draw/draw.h"
#include "system/util/hid_types.h"

/**
 * @brief Initialize a hid api.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_hid_init(void);

/**
 * @brief Uninitialize a hid API.
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
bool Util_hid_is_pressed(Hid_info hid_state, Draw_image_data image);

/**
 * @brief Check whether image is held.
 * Always return false if hid api is not initialized.
 * @param hid_state (in) Key info returned by Util_hid_query_key_state().
 * @param image (in) Image data.
 * @return Whether image is held.
 * @note Thread safe
*/
bool Util_hid_is_held(Hid_info hid_state, Draw_image_data image);

/**
 * @brief Check whether image is released.
 * Always return false if hid api is not initialized.
 * @param hid_state (in) Key info returned by Util_hid_query_key_state().
 * @param image (in) Image data.
 * @return Whether image is released.
 * @note Thread safe
*/
bool Util_hid_is_released(Hid_info hid_state, Draw_image_data image);

/**
 * @brief Query current key state.
 * @param out_key_state (out) Pointer for key info.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Util_hid_query_key_state(Hid_info* out_key_state);

/**
 * @brief Reset afk time.
 * Do nothing if hid api is not initialized.
 * @note Thread safe
*/
void Util_hid_reset_afk_time(void);

/**
 * @brief Reset key state to NOT_PRESSED.
 * Do nothing if hid api is not initialized.
 * @param keys (in) Key to reset.
 * @note Thread safe
*/
void Util_hid_reset_key_state(Hid_key_bit keys);

/**
 * @brief Add hid callback.
 * Always return false if hid api is not initialized.
 * @param callback (in) Pointer for callback function.
 * @return On success true,
 * on failure false.
 * @note Thread safe
*/
bool Util_hid_add_callback(void (*const callback)(void));

/**
 * @brief Remove hid callback.
 * Do nothing if hid api is not initialized.
 * @param callback (in) Pointer for callback function.
 * @note Thread safe
*/
void Util_hid_remove_callback(void (*const callback)(void));

#endif //!defined(DEF_HID_H)
