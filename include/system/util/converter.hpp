#ifndef CONVERTER_HPP
#define CONVERTER_HPP

#if (defined(DEF_ENABLE_SW_FFMPEG_COLOR_CONVERTER_API) || defined(DEF_ENABLE_SW_FFMPEG_AUDIO_CONVERTER_API) \
|| defined(DEF_ENABLE_SW_CONVERTER_API) || defined(DEF_ENABLE_SW_ASM_CONVERTER_API) || defined(DEF_ENABLE_HW_CONVERTER_API))
#include "system/types.hpp"
#endif

#if DEF_ENABLE_SW_FFMPEG_COLOR_CONVERTER_API

/**
 * @brief Convert color format and/or size.
 * @param paraeters (in) Pointer for parameters. (See types.hpp for explanation).
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_convert_color(Color_converter_parameters* paraeters);

#else

#define Util_converter_convert_color(...) Util_return_result_with_string(var_disabled_result)

#endif

#if DEF_ENABLE_SW_FFMPEG_AUDIO_CONVERTER_API

/**
 * @brief Convert audio format and/or sample rate.
 * @param paraeters (in) Pointer for parameters. (See types.hpp for explanation).
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_convert_audio(Audio_converter_parameters* parameters);

#else

#define Util_converter_convert_audio(...) Util_return_result_with_string(var_disabled_result)

#endif

#if DEF_ENABLE_SW_CONVERTER_API

/**
 * @brief Convert YUV420P to RGB565LE.
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb565 (out) Pointer for rgb565 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_yuv420p_to_rgb565le(u8* yuv420p, u8** rgb565, int width, int height);

/**
 * @brief Convert YUV420P to RGB888LE.
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb888 (out) Pointer for rgb888 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_yuv420p_to_rgb888le(u8* yuv420p, u8** rgb888, int width, int height);

/**
 * @brief Convert RGB8888BE to RGB8888LE.
 * @param rgb8888 (in&out) Pointer for rgb888 data.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_rgba8888be_to_rgba8888le(u8* rgba8888, int width, int height);

/**
 * @brief Convert RGB888BE to RGB888LE.
 * @param rgb888 (in&out) Pointer for rgb888 data.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_rgb888be_to_rgb888le(u8* rgb888, int width, int height);

/**
 * @brief Rotate (any) RGB888 90 degree.
 * @param rgb888 (in) Pointer for rgb888 data.
 * @param rotated_rgb888 (out) Pointer for rotated_rgb888 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @param rotated_width (out) new(after rotated) picture width.
 * @param rotated_height (out) new(after rotated) picture height.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_rgb888_rotate_90_degree(u8* rgb888, u8** rotated_rgb888, int width, int height, int* rotated_width, int* rotated_height);

#else

#define Util_converter_yuv422_to_rgb565le(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_yuv422_to_yuv420p(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_yuv420p_to_rgb565le(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_yuv420p_to_rgb888le(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_rgba8888be_to_rgba8888le(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_rgb888be_to_rgb888le(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_rgb888_rotate_90_degree(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_rgb888le_to_yuv420p(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_rgb565le_to_rgb888le(...) Util_return_result_with_string(var_disabled_result)

#endif

#if DEF_ENABLE_SW_ASM_CONVERTER_API

/**
 * @brief Convert YUV420P to RGB888LE (assembly optimized).
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb888 (out) Pointer for rgb888 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_yuv420p_to_rgb888le_asm(u8* yuv420p, u8** rgb888, int width, int height);

/**
 * @brief Convert YUV420P to RGB565LE (assembly optimized).
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb565 (out) Pointer for rgb565 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_converter_yuv420p_to_rgb565le_asm(u8* yuv420p, u8** rgb565, int width, int height);

#else

#define Util_converter_yuv420p_to_rgb888le_asm(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_yuv420p_to_rgb565le_asm(...) Util_return_result_with_string(var_disabled_result)

#endif

#if DEF_ENABLE_HW_CONVERTER_API

/**
 * @brief Initialize a y2r(hardware color converter).
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_converter_y2r_init(void);

/**
 * @brief Convert YUV420P to RGB565LE (hardware conversion).
 * @param yuv420p (in) Pointer for yuv420p data.
 * @param rgb565 (out) Pointer for rgb565 data, the pointer will be allocated inside of function.
 * @param width (in) Picture width.
 * @param height (in) Picture height.
 * @param texture_format (in) When true, rgb565 data will be outputted as texture format that
 * can be used for Draw_set_texture_data_direct().
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_converter_y2r_yuv420p_to_rgb565le(u8* yuv420p, u8** rgb565, int width, int height, bool texture_format);

/**
 * @brief Uninitialize a y2r(hardware color converter).
 * Do nothing if y2r api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_converter_y2r_exit(void);

#else

#define Util_converter_y2r_init() Util_return_result_with_string(var_disabled_result)
#define Util_converter_y2r_yuv420p_to_rgb565le(...) Util_return_result_with_string(var_disabled_result)
#define Util_converter_y2r_exit()

#endif

#endif
