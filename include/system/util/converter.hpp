#pragma once

Result_with_string Util_converter_yuv422_to_bgr565(u8* yuv422, u8** bgr565, int width, int height);

Result_with_string Util_converter_yuv420p_to_bgr565(u8* yuv420p, u8** bgr565, int width, int height);

void Util_converter_rgb888_to_bgr888(u8* buf, int width, int height);

Result_with_string Util_converter_bgr888_rotate_90_degree(u8* bgr888, u8** rotated_bgr888, int width, int height, int* rotated_width, int* rotated_height);

Result_with_string Util_converter_bgr888_to_yuv420p(u8* bgr888, u8** yuv420p, int width, int height);

Result_with_string Util_converter_y2r_init(void);

Result_with_string Util_converter_y2r_yuv420p_to_bgr565(u8* yuv420p, u8** bgr565, int width, int height);

void Util_converter_y2r_exit(void);
