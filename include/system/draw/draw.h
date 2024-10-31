#if !defined(DEF_DRAW_H)
#define DEF_DRAW_H
#include <stdbool.h>
#include <stdint.h>
#include "system/draw/draw_types.h"
#include "system/util/cpu_usage_types.h"
#include "system/util/raw_types.h"
#include "system/util/str_types.h"

/**
 * @brief Initialize a draw api.
 * It is NOT possible to enable 800px mode and 3D mode at the same time.
 * @param wide (in) When true, enable 800px mode.
 * @param _3d (in) When true, enable 3D mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
uint32_t Draw_init(bool wide, bool _3d);

/**
 * @brief Reinitialize a draw api.
 * It is NOT possible to enable 800px mode and 3D mode at the same time.
 * @param wide (in) When true, enable 800px mode.
 * @param _3d (in) When true, enable 3D mode.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
uint32_t Draw_reinit(bool wide, bool _3d);

/**
 * @brief Uninitialize a draw api.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
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
 * @brief Get if screen needs to be refreshed (re-rendered).
 * Always return false if draw api is not initialized.
 * @return true if screen refresh is needed, otherwise false.
 * @note Thread safe
*/
bool Draw_is_refresh_needed(void);

/**
 * @brief Set if screen needs to be refreshed (re-rendered).
 * Do nothing if draw api is not initialized.
 * @param is_refresh_needed (in) Whether screen needs to be refreshed.
 * @note Thread safe
*/
void Draw_set_refresh_needed(bool is_refresh_needed);

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
 * @param color_format (in) Color format (RAW_PIXEL_ABGR8888, RAW_PIXEL_BGR888 or RAW_PIXEL_RGB565LE).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Draw_texture_init(Draw_image_data* image, uint16_t tex_size_x, uint16_t tex_size_y, Raw_pixel color_format);

/**
 * @brief Uninitialize a texture.
 * Do nothing if draw api is not initialized.
 * @param image (in&out) Pointer for texture.
 * @note Thread safe
*/
void Draw_texture_free(Draw_image_data* image);

/**
 * @brief Set raw image data to texture data.
 * @param image (in&out) Pointer for texture data.
 * @param buf (in) Pointer for raw image data (must be texture format).
 * @param pic_width (in) Raw image width (up to 1024).
 * @param pic_height (in) Raw image height (up to 1024).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Draw_set_texture_data_direct(Draw_image_data* image, uint8_t* buf, uint16_t pic_width, uint16_t pic_height);

/**
 * @brief Convert raw image data to texture format and set it.
 * @param image (in&out) Pointer for texture data.
 * @param buf (in) Pointer for raw image data.
 * @param pic_width (in) Raw image width.
 * @param pic_height (in) Raw image height.
 * @param width_offset (in) Width offset.
 * @param height_offset (in) Height offset.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Draw_set_texture_data(Draw_image_data* image, uint8_t* buf, uint32_t pic_width, uint32_t pic_height, uint32_t width_offset, uint32_t height_offset);

/**
 * @brief Set texture filter.
 * Do nothing if draw api is not initialized.
 * @param image (in&out) Texture data.
 * @param filter (in) When true, LINEAR filter will be applied, otherwise NEAREST filter will be applied.
 * @note Thread safe
*/
void Draw_set_texture_filter(Draw_image_data* image, bool filter);

/**
 * @brief Get empty (1x1 image that can be used to draw square) image.
 * @note Thread safe
*/
Draw_image_data Draw_get_empty_image(void);

/**
 * @brief Get text size.
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param out_text_size_x (out) Text width (in px).
 * @param out_text_size_y (out) Text height (in px).
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_get_text_size(const char* text, float text_size_x, float text_size_y, double* out_text_size_x, double* out_text_size_y);

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
 * @warning Call it only from rendering thread.
*/
void Draw_c(const char* text, float x, float y, float text_size_x, float text_size_y, uint32_t abgr8888);
void Draw(const Str_data* text, float x, float y, float text_size_x, float text_size_y, uint32_t abgr8888);

/**
 * @brief Draw text with specified alignment.
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param x (in) X position (in px).
 * @param y (in) Y position (in px).
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param abgr8888 (in) Font color.
 * @param x_align (in) Text align for x direction.
 * @param y_align (in) Text align for y direction.
 * @param x_size (in) If align is DRAW_X_ALIGN_LEFT, this is ignored otherwise virtual box width (in px).
 * @param y_size (in) If align is DRAW_Y_ALIGN_TOP, this is ignored otherwise virtual box height (in px).
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_align_c(const char* text, float x, float y, float text_size_x, float text_size_y, uint32_t abgr8888,
Draw_text_align_x x_align, Draw_text_align_y y_align, float box_size_x, float box_size_y);
void Draw_align(const Str_data* text, float x, float y, float text_size_x, float text_size_y, uint32_t abgr8888,
Draw_text_align_x x_align, Draw_text_align_y y_align, float box_size_x, float box_size_y);

/**
 * @brief Draw text with background (and specified alignment).
 * Do nothing if draw api is not initialized.
 * @param text (in) Text.
 * @param x (in) X position (in px).
 * @param y (in) Y position (in px).
 * @param text_size_x (in) Font size for X direction.
 * @param text_size_y (in) Font size for Y direction.
 * @param abgr8888 (in) Font color.
 * @param x_align (in) Text align for x direction.
 * @param y_align (in) Text align for y direction.
 * @param x_size (in) If align is DRAW_X_ALIGN_LEFT, this is ignored otherwise virtual box width (in px).
 * @param y_size (in) If align is DRAW_Y_ALIGN_TOP, this is ignored otherwise virtual box height (in px).
 * @param texture_position (in) Background texture position.
 * @param background_image (in&out) Image data.
 * @param texture_abgr8888 (in) Texture color.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_with_background_c(const char* text, float x, float y, float text_size_x, float text_size_y, uint32_t abgr8888, Draw_text_align_x x_align,
Draw_text_align_y y_align, float box_size_x, float box_size_y, Draw_background texture_position, Draw_image_data* background_image, uint32_t texture_abgr8888);
void Draw_with_background(const Str_data* text, float x, float y, float text_size_x, float text_size_y, uint32_t abgr8888, Draw_text_align_x x_align,
Draw_text_align_y y_align, float box_size_x, float box_size_y, Draw_background texture_position, Draw_image_data* background_image, uint32_t texture_abgr8888);

/**
 * @brief Get free sheet index that can be used for Draw_load_texture().
 * Always return UINT32_MAX if draw api is not initialized.
 * @return Free sheet index.
 * @warning Thread dangerous (untested)
*/
uint32_t Draw_get_free_sheet_num(void);

/**
 * @brief Load texture data from .t3x file.
 * @param file_path (in) File path.
 * @param sheet_map_num (in) Internal sheet index.
 * @param return_image (out) Array for texture data, the array will be allocated inside of function.
 * @param start_num (in) Array offset.
 * @param num_of_array (in) Array size.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
uint32_t Draw_load_texture(const char* file_path, uint32_t sheet_map_num, C2D_Image return_image[], uint32_t start_num, uint32_t num_of_array);

/**
 * @brief Unload texture data.
 * Do nothing if draw api is not initialized.
 * @param sheet_map_num (in) Internal sheet index.
 * @warning Thread dangerous (untested)
*/
void Draw_free_texture(uint32_t sheet_map_num);

/**
 * @brief Draw top UI.
 * Do nothing if draw api is not initialized.
 * @param is_eco (in) Whether eco mode is enabled
 * @param is_charging (in) Whether charger is active.
 * @param wifi_signal (in) Wifi signal strength.
 * @param battery_level (in) Battery level in %.
 * @param message (in) Optional message, can be NULL.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_top_ui(bool is_eco, bool is_charging, uint8_t wifi_signal, uint8_t battery_level, const char* message);

/**
 * @brief Draw bottom UI.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_bot_ui(void);

/**
 * @brief Get bottom UI's button.
 * @return Bottom UI's button.
 * @warning Thread dangerous (untested)
*/
Draw_image_data* Draw_get_bot_ui_button(void);

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
 * @warning Call it only from rendering thread.
*/
void Draw_texture(Draw_image_data* image, uint32_t abgr8888, float x, float y, float x_size, float y_size);

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
 * @warning Call it only from rendering thread.
*/
void Draw_texture_with_rotation(Draw_image_data* image, uint32_t abgr8888, float x, float y, float x_size, float y_size, float angle, float center_x, float center_y);

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
 * @warning Call it only from rendering thread.
*/
void Draw_line(float x_0, float y_0, uint32_t abgr8888_0, float x_1, float y_1, uint32_t abgr8888_1, float width);

/**
 * @brief Draw debug info.
 * Do nothing if draw api is not initialized.
 * @param is_night Whether night mode is enabled.
 * @param free_ram Free heap size in bytes.
 * @param free_linear_ram Free linear RAM size in bytes.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_debug_info(bool is_night, uint32_t free_ram, uint32_t free_linear_ram);

/**
 * @brief Ready frame for drawing.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_frame_ready(void);

/**
 * @brief Ready screen and fill with abgr8888.
 * Do nothing if draw api is not initialized.
 * @param screen_num (in) Target screen.
 * @param abgr8888 (in) Fill color.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_screen_ready(Draw_screen screen, uint32_t abgr8888);

/**
 * @brief Apply drawing.
 * Do nothing if draw api is not initialized.
 * @warning Thread dangerous (untested)
 * @warning Call it only from rendering thread.
*/
void Draw_apply_draw(void);

#endif //!defined(DEF_DRAW_H)
