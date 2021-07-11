#pragma once

double Draw_query_frametime(void);

double Draw_query_fps(void);

Result_with_string Draw_set_texture_data(Image_data* c2d_image, u8* buf, int pic_width, int pic_height, int tex_size_x, int tex_size_y, GPU_TEXCOLOR color_format);

Result_with_string Draw_set_texture_data(Image_data* c2d_image, u8* buf, int pic_width, int pic_height, int parse_start_width, int parse_start_height, int tex_size_x, int tex_size_y, GPU_TEXCOLOR color_format);

void Draw_c2d_image_set_filter(Image_data* c2d_image, bool filter);

Result_with_string Draw_c2d_image_init(Image_data* c2d_image,int tex_size_x, int tex_size_y, GPU_TEXCOLOR color_format);

void Draw_c2d_image_free(Image_data c2d_image);

void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888);

Result_with_string Draw_load_texture(std::string file_name, int sheet_map_num, C2D_Image return_image[], int start_num, int num_of_array);

void Draw_touch_pos(void);

void Draw_top_ui(void);

void Draw_bot_ui(void);

void Draw_texture(C2D_Image image, float x, float y, float x_size, float y_size);

void Draw_texture(C2D_Image image, int abgr8888, float x, float y, float x_size, float y_size);

void Draw_line(float x_0, float y_0, int abgr8888_0, float x_1, float y_1, int abgr8888_1, float width);

void Draw_debug_info(void);

Result_with_string Draw_init(bool wide, bool _3d);

void Draw_reinit(bool wide, bool _3d);

Result_with_string Draw_load_system_font(int system_font_num);

void Draw_free_system_font(int system_font_num);

void Draw_free_texture(int sheet_map_num);

void Draw_exit(void);

void Draw_frame_ready(void);

void Draw_screen_ready(int screen_num, int abgr8888);

void Draw_apply_draw(void);
