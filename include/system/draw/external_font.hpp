#pragma once

/**
 * @brief Initialize a external font api.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Exfont_init(void);

/**
 * @brief Uninitialize a external font API.
 * Do nothing if external font api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Exfont_exit(void);

/**
 * @brief Query external font name.
 * Always return empty string if external font api is not initialized.
 * @param exfont_num (in) External font index.
 * @return External font name.
 * @warning Thread dangerous (untested)
*/
std::string Exfont_query_external_font_name(int exfont_num);

/**
 * @brief Check whether external font is loaded (ready).
 * Always return false if external font api is not initialized.
 * @param exfont_num (in) External font index.
 * @return True if external font is loaded, otherwise false. 
 * @warning Thread dangerous (untested)
*/
bool Exfont_is_loaded_external_font(int exfont_num);

/**
 * @brief Check whether external font is loading.
 * Always return false if external font api is not initialized.
 * @return True if external font is loading right now, otherwise false. 
 * @warning Thread dangerous (untested)
*/
bool Exfont_is_loading_external_font(void);

/**
 * @brief Check whether external font is unloading.
 * Always return false if external font api is not initialized.
 * @return True if external font is unloading right now, otherwise false. 
 * @warning Thread dangerous (untested)
*/
bool Exfont_is_unloading_external_font(void);

/**
 * @brief Check whether system font is loaded (ready).
 * Always return false if external font api is not initialized.
 * @param system_font_num (in) System font index.
 * @return True if system font is loaded, otherwise false. 
 * @warning Thread dangerous (untested)
*/
bool Exfont_is_loaded_system_font(int system_font_num);

/**
 * @brief Check whether system font is loading.
 * Always return false if external font api is not initialized.
 * @return True if system font is loading right now, otherwise false. 
 * @warning Thread dangerous (untested)
*/
bool Exfont_is_loading_system_font(void);

/**
 * @brief Check whether system font is unloading.
 * Always return false if external font api is not initialized.
 * @return True if system font is unloading right now, otherwise false. 
 * @warning Thread dangerous (untested)
*/
bool Exfont_is_unloading_system_font(void);

/**
 * @brief Set external font request state.
 * Do nothing if external font api is not initialized.
 * Actuall loading/unloading won't happen until you call 
 * Exfont_request_load_external_font()/Exfont_request_unload_external_font().
 * @param exfont_num (in) External font index.
 * @param flag (in) When true, external font request state will be set to true, otherwise set to false.
 * @warning Thread dangerous (untested)
*/
void Exfont_set_external_font_request_state(int exfont_num, bool flag);

/**
 * @brief Load external font.
 * Do nothing if external font api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Exfont_request_load_external_font(void);

/**
 * @brief Unload external font.
 * Do nothing if external font api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Exfont_request_unload_external_font(void);

/**
 * @brief Set system font request state.
 * Do nothing if external font api is not initialized.
 * Actuall loading/unloading won't happen until you call 
 * Exfont_request_load_system_font()/Exfont_request_unload_system_font().
 * @param system_font_num (in) System font index.
 * @param flag (in) When true, system font request state will be set to true, otherwise set to false.
 * @warning Thread dangerous (untested)
*/
void Exfont_set_system_font_request_state(int system_font_num, bool flag);

/**
 * @brief Load system font.
 * Do nothing if external font api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Exfont_request_load_system_font(void);

/**
 * @brief Unload system font.
 * Do nothing if external font api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Exfont_request_unload_system_font(void);

/**
 * @brief Sort right-to-left text data (partial support).
 * Always return empty string if external font api is not initialized.
 * @param sorce_part_string (in) Array for unsorted text.
 * @param max_loop (in) Number of array.
 * @return Sorted text data.
 * @note Thread safe
*/
std::string Exfont_text_sort(std::string sorce_part_string[], int max_loop);

/**
 * @brief Parse text data.
 * @param sorce_string (in) Array for text.
 * @param part_string (out) Array for parsed text.
 * @param max_loop (in) Number of array for part_string.
 * @param out_element (out) Parsed number of characters.
 * @note Thread safe
*/
void Exfont_text_parse(std::string sorce_string, std::string part_string[], int max_loop, int* out_element);

/**
 * @brief Draw external font.
 * Do nothing if external font api is not initialized.
 * @param in_string (in) Text.
 * @param texture_x (in) X position (in px).
 * @param texture_y (in) Y position (in px).
 * @param texture_size_x (in) Font size for X direction.
 * @param texture_size_y (in) Font size for Y direction.
 * @param abgr8888 (in) Font color.
 * @param out_width (out) Total font width (in px).
 * @param out_height (out) Total font height (in px).
 * @param out_height (out) Total font height (in px).
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Exfont_draw_external_fonts(std::string in_string, float texture_x, float texture_y, float texture_size_x,
 float texture_size_y, int abgr8888, float* out_width, float* out_height);

/**
 * @brief Get text size.
 * Do nothing if external font api is not initialized.
 * @param in_string (in) Text.
 * @param texture_size_x (in) Font size for X direction.
 * @param texture_size_y (in) Font size for Y direction.
 * @param out_width (out) Text width (in px).
 * @param out_height (out) Text height (in px).
 * @warning Thread dangerous (untested)
 * @warning Call it from only drawing thread.
*/
void Exfont_draw_get_text_size(std::string in_string, float texture_size_x, float texture_size_y, float* out_width, float* out_height);
