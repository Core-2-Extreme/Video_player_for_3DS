#if !defined(DEF_CAM_H)
#define DEF_CAM_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/cam_types.h"
#include "system/util/raw_types.h"

#if defined(DEF_CAM_API_ENABLE)

/**
 * @brief Initialize a camera.
 * @param color_format (in) Output color format (RAW_PIXEL_RGB565LE or RAW_PIXEL_YUV422P).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_init(Raw_pixel color_format);

/**
 * @brief Take a picture.
 * @param raw_data (out) Pointer for picture data, the pointer will be allocated inside of function.
 * @param width (out) Picture width.
 * @param height (out) Picture height.
 * @param shutter_sound (in) When true, shutter sound will be played after taking a picture.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_take_a_picture(uint8_t** raw_data, uint16_t* width, uint16_t* height, bool shutter_sound);

/**
 * @brief Set picture resolution.
 * @param resolution_mode (in) Resolution mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_resolution(Cam_resolution resolution_mode);

/**
 * @brief Set framerate.
 * @param fps_mode (in) Framerate mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_fps(Cam_framerate fps_mode);

/**
 * @brief Set contrast.
 * @param contrast_mode (in) Contrast mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_contrast(Cam_contrast contrast_mode);

/**
 * @brief Set white balance.
 * @param white_balance_mode (in) White balance mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_white_balance(Cam_white_balance white_balance_mode);

/**
 * @brief Set lens correction.
 * @param lens_correction_mode (in) Lens correction mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_lens_correction(Cam_lens_correction lens_correction_mode);

/**
 * @brief Set camera port.
 * @param camera_port (in) Camera port.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_camera(Cam_port camera_port);

/**
 * @brief Set exposure.
 * @param exposure_mode (in) Exposure mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_exposure(Cam_exposure exposure_mode);

/**
 * @brief Set noise filter.
 * @param noise_filter_mode (in) When true, noise filter will be turned on otherwise off.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_cam_set_noise_filter(bool noise_filter_mode);

/**
 * @brief Uninitialize a camera.
 * Do nothing if camera API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_cam_exit(void);

#else

#define Util_cam_init(...) DEF_ERR_DISABLED
#define Util_cam_take_a_picture(...) DEF_ERR_DISABLED
#define Util_cam_set_resolution(...) DEF_ERR_DISABLED
#define Util_cam_set_fps(...) DEF_ERR_DISABLED
#define Util_cam_set_contrast(...) DEF_ERR_DISABLED
#define Util_cam_set_white_balance(...) DEF_ERR_DISABLED
#define Util_cam_set_lens_correction(...) DEF_ERR_DISABLED
#define Util_cam_set_camera(...) DEF_ERR_DISABLED
#define Util_cam_set_exposure(...) DEF_ERR_DISABLED
#define Util_cam_set_noise_filter(...) DEF_ERR_DISABLED
#define Util_cam_exit()

#endif //defined(DEF_CAM_API_ENABLE)

#endif //!defined(DEF_CAM_H)
