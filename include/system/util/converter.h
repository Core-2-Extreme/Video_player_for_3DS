#if !defined(DEF_CONVERTER_H)
#define DEF_CONVERTER_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/converter_types.h"

#if DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE

/**
 * @brief Convert color format and/or size.
 * @param parameters (in) Pointer for parameters.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_convert_color(Converter_color_parameters* parameters);

#else

#define Util_converter_convert_color(...) DEF_ERR_DISABLED

#endif //DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE

#if DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE

/**
 * @brief Convert audio format and/or sample rate.
 * @param parameters (in) Pointer for parameters.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_convert_audio(Converter_audio_parameters* parameters);

#else

#define Util_converter_convert_audio(...) DEF_ERR_DISABLED

#endif //DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE

#if DEF_CONVERTER_SW_API_ENABLE

/**
 * @brief Convert YUV420P to RGB565LE.
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb565 (out) Pointer for rgb565 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_yuv420p_to_rgb565le(const uint8_t* yuv420p, uint8_t** rgb565, uint32_t width, uint32_t height);

/**
 * @brief Convert YUV420P to RGB888LE.
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb888 (out) Pointer for rgb888 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_yuv420p_to_rgb888le(const uint8_t* yuv420p, uint8_t** rgb888, uint32_t width, uint32_t height);

/**
 * @brief Convert RGBA8888BE to RGBA8888LE.
 * @param rgba8888 (in/out) Pointer for rgba888 data.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_rgba8888be_to_rgba8888le(uint8_t* rgba8888, uint32_t width, uint32_t height);

/**
 * @brief Convert RGB888BE to RGB888LE.
 * @param rgb888 (in/out) Pointer for rgb888 data.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_rgb888be_to_rgb888le(uint8_t* rgb888, uint32_t width, uint32_t height);

/**
 * @brief Rotate (any) RGB888 90 degree.
 * @param rgb888 (in) Pointer for rgb888 data.
 * @param rotated_rgb888 (out) Pointer for rotated_rgb888 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @param rotated_width (out) new (after rotated) picture width.
 * @param rotated_height (out) new (after rotated) picture height.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_rgb888_rotate_90_degree(const uint8_t* rgb888, uint8_t** rotated_rgb888, uint32_t width, uint32_t height, uint32_t* rotated_width, uint32_t* rotated_height);

#else

#define Util_converter_yuv422_to_rgb565le(...) DEF_ERR_DISABLED
#define Util_converter_yuv422_to_yuv420p(...) DEF_ERR_DISABLED
#define Util_converter_yuv420p_to_rgb565le(...) DEF_ERR_DISABLED
#define Util_converter_yuv420p_to_rgb888le(...) DEF_ERR_DISABLED
#define Util_converter_rgba8888be_to_rgba8888le(...) DEF_ERR_DISABLED
#define Util_converter_rgb888be_to_rgb888le(...) DEF_ERR_DISABLED
#define Util_converter_rgb888_rotate_90_degree(...) DEF_ERR_DISABLED
#define Util_converter_rgb888le_to_yuv420p(...) DEF_ERR_DISABLED
#define Util_converter_rgb565le_to_rgb888le(...) DEF_ERR_DISABLED

#endif //DEF_CONVERTER_SW_API_ENABLE

#if DEF_CONVERTER_SW_ASM_API_ENABLE

/**
 * @brief Convert YUV420P to RGB888LE (assembly optimized).
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb888 (out) Pointer for rgb888 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_yuv420p_to_rgb888le_asm(const uint8_t* yuv420p, uint8_t** rgb888, uint32_t width, uint32_t height);

/**
 * @brief Convert YUV420P to RGB565LE (assembly optimized).
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb565 (out) Pointer for rgb565 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_converter_yuv420p_to_rgb565le_asm(const uint8_t* yuv420p, uint8_t** rgb565, uint32_t width, uint32_t height);

#else

#define Util_converter_yuv420p_to_rgb888le_asm(...) DEF_ERR_DISABLED
#define Util_converter_yuv420p_to_rgb565le_asm(...) DEF_ERR_DISABLED

#endif //DEF_CONVERTER_SW_ASM_API_ENABLE

#if DEF_CONVERTER_HW_API_ENABLE

/**
 * @brief Initialize a y2r(hardware color converter).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_converter_y2r_init(void);

/**
 * @brief Convert YUV420P to RGB565LE (hardware conversion).
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb565 (out) Pointer for rgb565 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @param texture_format (in) When true, rgb565 data will be outputted as texture format that
 * can be used for Draw_set_texture_data_direct().
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_converter_y2r_yuv420p_to_rgb565le(const uint8_t* yuv420p, uint8_t** rgb565, uint16_t width, uint16_t height, bool texture_format);

/**
 * @brief Uninitialize a y2r(hardware color converter).
 * Do nothing if y2r API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_converter_y2r_exit(void);

#else

#define Util_converter_y2r_init() DEF_ERR_DISABLED
#define Util_converter_y2r_yuv420p_to_rgb565le(...) DEF_ERR_DISABLED
#define Util_converter_y2r_exit()

#endif //DEF_CONVERTER_HW_API_ENABLE

#endif //!defined(DEF_CONVERTER_H)
