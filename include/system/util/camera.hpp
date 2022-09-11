#pragma once

#if DEF_ENABLE_CAM_API

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
 * @brief Set framerate.
 * @param fps_mode (in) Framerate mode (DEF_CAM_FPS_*).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_fps(int fps_mode);

/**
 * @brief Set contrast.
 * @param contrast_mode (in) Contrast mode (DEF_CAM_CONTRAST_*).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_contrast(int contrast_mode);

/**
 * @brief Set white balance.
 * @param white_balance_mode (in) White balance mode (DEF_CAM_WHITE_BALANCE_*).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_white_balance(int white_balance_mode);

/**
 * @brief Set lens correction.
 * @param lens_correction_mode (in) Lens correction mode (DEF_CAM_LENS_CORRECTION_*).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_lens_correction(int lens_correction_mode);

/**
 * @brief Set camera.
 * @param camera_mode (in) Camera mode (DEF_CAM_*).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_camera(int camera_mode);

/**
 * @brief Set exposure.
 * @param exposure_mode (in) Exposure mode (0 ~ 5).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_exposure(int exposure_mode);

/**
 * @brief Set noise filter.
 * @param noise_filter_mode (in) When true, noise filter will be turned on otherwise off.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_noise_filter(bool noise_filter_mode);

/**
 * @brief Uninitialize a camera.
 * Do nothing if camera api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_cam_exit(void);

#else

#define Util_cam_init(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_take_a_picture(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_resolution(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_fps(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_contrast(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_white_balance(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_lens_correction(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_camera(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_exposure(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_set_noise_filter(...) Util_return_result_with_string(var_disabled_result)
#define Util_cam_exit(...)

#endif
