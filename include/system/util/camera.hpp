#pragma once

/**
 * @brief Initialize a camera.
 * @param color_format (in) Output color format (DEF_CAM_OUT_*).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_init(int color_format);

/**
 * @brief Take a picture.
 * @param raw_data (out) Pointer for picture data, the pointer will be allocated inside of function.
 * @param width (out) Picture width.
 * @param height (out) Picture height.
 * @param shutter_sound (in) When true, shutter sound will be played after taking a picture.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_take_a_picture(u8** raw_data, int* width, int* height, bool shutter_sound);

/**
 * @brief Set picture resolution.
 * Available resolutions are : 640*480, 512*384, 400*240, 352*288, 320*240, 256*192, 176*144 and 160*120
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_resolution(int width, int height);

/**
 * @brief Uninitialize a camera.
 * Do nothing if camera api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_cam_exit(void);
