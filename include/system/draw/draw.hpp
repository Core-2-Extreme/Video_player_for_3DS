#ifndef DRAW_HPP
#define DRAW_HPP

#include "system/types.hpp"

/**
 * @brief Initialize a draw api.
 * It is NOT possible to enable 800px mode and 3D mode at the same time.
 * @param wide (in) When true, enable 800px mode.
 * @param _3d (in) When true, enable 3D mode.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
Result_with_string Draw_init(bool wide, bool _3d);

/**
 * @brief Reinitialize a draw api.
 * It is NOT possible to enable 800px mode and 3D mode at the same time.
 * @param wide (in) When true, enable 800px mode.
 * @param _3d (in) When true, enable 3D mode.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
Result_with_string Draw_reinit(bool wide, bool _3d);

/**
 * @brief Uninitialize a draw api.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_exit(void);

/**
 * @brief Get if currently 800px mode is enabled.
 * Always return false if draw api is not initialized.
 * @return true if 800px mode is enabled, otherwise false.
 * @warning Thread dangerous (untested)
*/
bool Draw_is_800px_mode(void);

/**
 * @brief Get if currently 3d mode is enabled.
 * Always return false if draw api is not initialized.
 * @return true if 3d mode is enabled, otherwise false.
 * @warning Thread dangerous (untested)
*/
bool Draw_is_3d_mode(void);

/**
 * @brief Query frametime.
 * Always return 0 if draw api is not initialized.
 * @return Frametime (in ms).
 * @warning Thread dangerous (untested)
*/
double Draw_query_frametime(void);

/**
 * @brief Query framerate.
 * Always return 0 if draw api is not initialized.
 * @return Framerate (average of 10 frames).
 * @warning Thread dangerous (untested)
*/
double Draw_query_fps(void);

/**
 * @brief Initialize a texture.
 * @param image (out) Pointer for texture.
 * @param tex_size_x (in) Texture width (must be power of 2).
 * @param tex_size_y (in) Texture height (must be power of 2).
 * @param color_format (in) Color format (PIXEL_FORMAT_ABGR8888, PIXEL_FORMAT_BGR888 or PIXEL_FORMAT_RGB565LE).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Draw_texture_init(Image_data* image, int tex_size_x, int tex_size_y, Pixel_format color_format);

/**
 * @brief Uninitialize a texture.
 * Do nothing if draw api is not initialized.
 * @param image (in&out) Pointer for texture.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
void Draw_texture_free(Image_data* image);

/**
 * @brief Set raw image data to texture data.
 * @param image (in&out) Pointer for texture data.
 * @param buf (in) Pointer for raw image data (must be texture format).
 * @param pic_width (in) Raw image width (up to 1024).
 * @param pic_height (in) Raw image height (up to 1024).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Draw_set_texture_data_direct(Image_data* image, u8* buf, int pic_width, int pic_height);

/**
 * @brief Convert raw image data to texture format and set it.
 * @param image (in&out) Pointer for texture data.
 * @param buf (in) Pointer for raw image data.
 * @param pic_width (in) Raw image width.
 * @param pic_height (in) Raw image height.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Draw_set_texture_data(Image_data* image, u8* buf, int pic_width, int pic_height);

/**
 * @brief Convert raw image data to texture format and set it.
 * @param image (in&out) Pointer for texture data.
 * @param buf (in) Pointer for raw image data.
 * @param pic_width (in) Raw image width.
 * @param pic_height (in) Raw image height.
 * @param width_offset (in) Width offset.
 * @param height_offset (in) Height offset.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Draw_set_texture_data(Image_data* image, u8* buf, int pic_width, int pic_height, int width_offset, int height_offset);

/**
 * @brief Set texture filter.
 * Do nothing if draw api is not initialized.
 * @param image (in&out) Texture data.
 * @param filter (in) When true, LINEAR filter will be applied, otherwise NEAREST filter will be applied.
 * @note Thread safe
*/
void Draw_set_texture_filter(Image_data* image, bool filter);

/**
 * @brief Get text size.
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param out_text_size_x (out) Text width (in px).
 * @param out_text_size_y (out) Text height (in px).
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_get_text_size(std::string text, float text_size_x, float text_size_y, double* out_text_size_x, double* out_text_size_y);

/**
 * @brief Draw text.
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param x (in) X position (in px).
 * @param y (in) Y position (in px).
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param abgr8888 (in) Font color.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888);

/**
 * @brief Draw text.
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param x (in) X position (in px).
 * @param y (in) Y position (in px).
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param abgr8888 (in) Font color.
 * @param x_align (in) Text align for x direction.
 * @param y_align (in) Text align for y direction.
 * @param x_size (in) If align is X_ALIGN_LEFT, this is ignored otherwise virtual box width (in px).
 * @param y_size (in) If align is Y_ALIGN_TOP, this is ignored otherwise virtual box height (in px).
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888, Text_align_x x_align,
Text_align_y y_align, float box_size_x, float box_size_y);

/**
 * @brief Draw text.
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param x (in) X position (in px).
 * @param y (in) Y position (in px).
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param abgr8888 (in) Font color.
 * @param x_align (in) Text align for x direction.
 * @param y_align (in) Text align for y direction.
 * @param x_size (in) If align is X_ALIGN_LEFT, this is ignored otherwise virtual box width (in px).
 * @param y_size (in) If align is Y_ALIGN_TOP, this is ignored otherwise virtual box height (in px).
 * @param texture_position (in) Background texture position.
 * @param background_image (in) C2D Texture data.
 * @param texture_abgr8888 (in) Texture color.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888, Text_align_x x_align,
Text_align_y y_align, float box_size_x, float box_size_y, Background texture_position, C2D_Image background_image, int texture_abgr8888);

/**
 * @brief Draw text.
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param x (in) X position (in px).
 * @param y (in) Y position (in px).
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param abgr8888 (in) Font color.
 * @param x_align (in) Text align for x direction.
 * @param y_align (in) Text align for y direction.
 * @param x_size (in) If align is X_ALIGN_LEFT, this is ignored otherwise virtual box width (in px).
 * @param y_size (in) If align is Y_ALIGN_TOP, this is ignored otherwise virtual box height (in px).
 * @param texture_position (in) Background texture position.
 * @param background_image (in&out) Image data.
 * @param texture_abgr8888 (in) Texture color.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888, Text_align_x x_align,
Text_align_y y_align, float box_size_x, float box_size_y, Background texture_position, Image_data* background_image, int texture_abgr8888);

/**
 * @brief Get free sheet index that can be used for Draw_load_texture().
 * Always return -1 if draw api is not initialized.
 * @return Free sheet index.
 * @warning Thread dangerous (untested)
*/
int Draw_get_free_sheet_num(void);

/**
 * @brief Load texture data from .t3x file.
 * @param file_path (in) File path.
 * @param sheet_map_num (in) Internal sheet index.
 * @param return_image (out) Array for texture data, the array will be allocated inside of function.
 * @param start_num (in) Array offset.
 * @param num_of_array (in) Array size.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Draw_load_texture(std::string file_path, int sheet_map_num, C2D_Image return_image[], int start_num, int num_of_array);

/**
 * @brief Unload texture data.
 * Do nothing if draw api is not initialized.
 * @param sheet_map_num (in) Internal sheet index.
 * @warning Thread dangerous (untested)
*/
void Draw_free_texture(int sheet_map_num);

/**
 * @brief Draw top UI.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_top_ui(void);

/**
 * @brief Draw bottom UI.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_bot_ui(void);

/**
 * @brief Get bottom UI's button.
 * @return Bottom UI's button.
 * @warning Thread dangerous (untested)
*/
Image_data* Draw_get_bot_ui_button(void);

/**
 * @brief Draw texture.
 * Do nothing if draw api is not initialized.
 * @param image (in) C2D Texture data.
 * @param x (in) X position.
 * @param y (in) Y position.
 * @param x_size (in) Texture size for X direction.
 * @param y_size (in) Texture size for Y direction.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_texture(C2D_Image image, float x, float y, float x_size, float y_size);

/**
 * @brief Draw texture.
 * Do nothing if draw api is not initialized.
 * @param image (in) C2D Texture data.
 * @param abgr8888 (in) Texture color.
 * @param x (in) X position.
 * @param y (in) Y position.
 * @param x_size (in) Texture size for X direction.
 * @param y_size (in) Texture size for Y direction.
 * @param angle (in) Texture angle.
 * @param center_x (in) Center of texture for X direction (used to rotate texture).
 * @param center_y (in) Center of texture for Y direction (used to rotate texture).
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_texture(C2D_Image image, int abgr8888, float x, float y, float x_size, float y_size, float angle, float center_x, float center_y);

/**
 * @brief Draw texture.
 * Do nothing if draw api is not initialized.
 * @param image (in) C2D Texture data.
 * @param abgr8888 (in) Texture color.
 * @param x (in) X position.
 * @param y (in) Y position.
 * @param x_size (in) Texture size for X direction.
 * @param y_size (in) Texture size for Y direction.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_texture(C2D_Image image, int abgr8888, float x, float y, float x_size, float y_size);

/**
 * @brief Draw texture.
 * Do nothing if draw api is not initialized.
 * @param image (in&out) Texture data.
 * @param x (in) X position.
 * @param y (in) Y position.
 * @param x_size (in) Texture size for X direction.
 * @param y_size (in) Texture size for Y direction.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_texture(Image_data* image, float x, float y, float x_size, float y_size);

/**
 * @brief Draw texture.
 * Do nothing if draw api is not initialized.
 * @param image (in&out) Texture data.
 * @param abgr8888 (in) Texture color.
 * @param x (in) X position.
 * @param y (in) Y position.
 * @param x_size (in) Texture size for X direction.
 * @param y_size (in) Texture size for Y direction.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_texture(Image_data* image, int abgr8888, float x, float y, float x_size, float y_size);

/**
 * @brief Draw texture.
 * Do nothing if draw api is not initialized.
 * @param image (in&out) Texture data.
 * @param abgr8888 (in) Texture color.
 * @param x (in) X position.
 * @param y (in) Y position.
 * @param x_size (in) Texture size for X direction.
 * @param y_size (in) Texture size for Y direction.
 * @param angle (in) Texture angle.
 * @param center_x (in) Center of texture for X direction (used to rotate texture).
 * @param center_y (in) Center of texture for Y direction (used to rotate texture).
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_texture(Image_data* image, int abgr8888, float x, float y, float x_size, float y_size, float angle, float center_x, float center_y);

/**
 * @brief Draw line.
 * Do nothing if draw api is not initialized.
 * @param x_0 (in) Initial x position.
 * @param y_0 (in) Initial y position.
 * @param abgr8888_0 (in) Initial line color.
 * @param x_1 (in) Final x position.
 * @param y_1 (in) Final y position.
 * @param abgr8888_1 (in) Final line color.
 * @param width (in) Line width.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_line(float x_0, float y_0, int abgr8888_0, float x_1, float y_1, int abgr8888_1, float width);

#if DEF_ENABLE_CPU_MONITOR_API

/**
 * @brief Draw cpu usage.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_cpu_usage_info(void);

#else

#define Draw_cpu_usage_info()

#endif

/**
 * @brief Load system font.
 * @param system_font_num (in) System font index.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Draw_load_system_font(int system_font_num);

/**
 * @brief Unload system font.
 * Do nothing if draw api is not initialized.
 * @param system_font_num (in) System font index.
 * @warning Thread dangerous (untested)
*/
void Draw_free_system_font(int system_font_num);

/**
 * @brief Ready frame for drawing.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_frame_ready(void);

/**
 * @brief Ready screen and fill with abgr8888.
 * Do nothing if draw api is not initialized.
 * @param screen_num (in) Target screen.
 * @param abgr8888 (in) Fill color.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_screen_ready(Screen screen, int abgr8888);

/**
 * @brief Apply drawing.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Draw_apply_draw(void);

#endif
