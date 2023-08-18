#ifndef CAMERA_HPP
#define CAMERA_HPP

#if DEF_ENABLE_CAM_API
#include "system/types.hpp"

/**
 * @brief Initialize a camera.
 * @param color_format (in) Output color format (PIXEL_FORMAT_RGB565LE or PIXEL_FORMAT_YUV422P).
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_init(Pixel_format color_format);

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
 * @param resolution_mode (in) Resolution mode.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_resolution(Camera_resolution resolution_mode);

/**
 * @brief Set framerate.
 * @param fps_mode (in) Framerate mode.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_fps(Camera_framerate fps_mode);

/**
 * @brief Set contrast.
 * @param contrast_mode (in) Contrast mode.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_contrast(Camera_contrast contrast_mode);

/**
 * @brief Set white balance.
 * @param white_balance_mode (in) White balance mode.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_white_balance(Camera_white_balance white_balance_mode);

/**
 * @brief Set lens correction.
 * @param lens_correction_mode (in) Lens correction mode.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_lens_correction(Camera_lens_correction lens_correction_mode);

/**
 * @brief Set camera port.
 * @param camera_mode (in) Camera port.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_camera(Camera_port camera_port);

/**
 * @brief Set exposure.
 * @param exposure_mode (in) Exposure mode.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cam_set_exposure(Camera_exposure exposure_mode);

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
#define Util_cam_exit()

#endif

#endif
