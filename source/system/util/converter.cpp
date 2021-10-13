#include "system/headers.hpp"

extern "C" void memcpy_asm(u8*, u8*, int);
extern "C" void yuv420p_to_bgr565_asm(u8* yuv420p, u8* bgr565, int width, int height);
extern "C" void yuv420p_to_bgr888_asm(u8* yuv420p, u8* bgr888, int width, int height);

extern "C" {
#include "libswscale/swscale.h"
}

#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)
// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )

#define YUV2R(Y, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)

Result_with_string Util_converter_yuv422_to_bgr565(u8* yuv422, u8** bgr565, int width, int height)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;
	
	sws_context = sws_getContext(width, height, AV_PIX_FMT_YUYV422,
	width, height, AV_PIX_FMT_RGB565LE, 0, 0, 0, 0);
	if(sws_context == NULL)
	{
		result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
		result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
		return result;
	}

	*bgr565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(*bgr565 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	src_data[0] = yuv422;
	src_line_size[0] = width * 2;
	dst_data[0] = *bgr565;
	dst_line_size[0] = width * 2;
	
	sws_scale(sws_context, src_data, src_line_size, 0, 
	height, dst_data, dst_line_size);

	sws_freeContext(sws_context);
	return result;
}

Result_with_string Util_converter_yuv422_to_yuv420p(u8* yuv422, u8** yuv420p, int width, int height)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;
	
	sws_context = sws_getContext(width, height, AV_PIX_FMT_YUYV422,
	width, height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	if(sws_context == NULL)
	{
		result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
		result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
		return result;
	}

	*yuv420p = (u8*)Util_safe_linear_alloc(width * height * 1.5);
	if(*yuv420p == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	src_data[0] = yuv422;
	src_line_size[0] = width * 2;
	dst_data[0] = *yuv420p;
	dst_data[1] = *yuv420p + (width * height);
	dst_data[2] = *yuv420p + (width * height) + (width * height / 4);
	dst_line_size[0] = width;
	dst_line_size[1] = width / 2;
	dst_line_size[2] = width / 2;
	
	sws_scale(sws_context, src_data, src_line_size, 0, 
	height, dst_data, dst_line_size);

	sws_freeContext(sws_context);
	return result;
}

Result_with_string Util_converter_yuv420p_to_bgr565(u8* yuv420p, u8** bgr565, int width, int height)
{
    int index = 0;
    u8* ybase = yuv420p;
    u8* ubase = yuv420p + width * height;
    u8* vbase = yuv420p + width * height + width * height / 4;
	Result_with_string result;

	*bgr565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(*bgr565 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}
	
	u8 Y[4], U, V, r[4], g[4], b[4];
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			//YYYYYYYYUUVV
			Y[0] = ybase[x + y * width];
			U = ubase[y / 2 * width / 2 + (x / 2)];
			V = vbase[y / 2 * width / 2 + (x / 2)];
			b[0] = YUV2B(Y[0], U);
			g[0] = YUV2G(Y[0], U, V);
			r[0] = YUV2R(Y[0], V);
			b[0] = b[0] >> 3;
			g[0] = g[0] >> 2;
			r[0] = r[0] >> 3;
			*(*bgr565 + index++) = (g[0] & 0b00000111) << 5 | b[0];
			*(*bgr565 + index++) = (g[0] & 0b00111000) >> 3 | (r[0] & 0b00011111) << 3;
		}
	}
	return result;
}

Result_with_string Util_converter_yuv420p_to_bgr565_asm(u8* yuv420p, u8** bgr565, int width, int height)
{
	Result_with_string result;

	*bgr565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(*bgr565 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	yuv420p_to_bgr565_asm(yuv420p, *bgr565, width, height);
	return result;
}

Result_with_string Util_converter_yuv420p_to_bgr888(u8* yuv420p, u8** bgr888, int width, int height)
{
    int index = 0;
    u8* ybase = yuv420p;
    u8* ubase = yuv420p + width * height;
    u8* vbase = yuv420p + width * height + width * height / 4;
	Result_with_string result;

	*bgr888 = (u8*)Util_safe_linear_alloc(width * height * 3);
	if(*bgr888 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}
	
	u8 Y[4], U, V;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			//YYYYYYYYUUVV
			Y[0] = *ybase++;
			U = ubase[y / 2 * width / 2 + (x / 2)];
			V = vbase[y / 2 * width / 2 + (x / 2)];
			
			*(*bgr888 + index++) = YUV2B(Y[0], U);
			*(*bgr888 + index++) = YUV2G(Y[0], U, V);
			*(*bgr888 + index++) = YUV2R(Y[0], V);
		}
	}
	return result;
}

Result_with_string Util_converter_yuv420p_to_bgr888_asm(u8* yuv420p, u8** bgr888, int width, int height)
{
	Result_with_string result;

	*bgr888 = (u8*)Util_safe_linear_alloc(width * height * 3);
	if(*bgr888 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	yuv420p_to_bgr888_asm(yuv420p, *bgr888, width, height);
	return result;
}

void Util_converter_rgb888_to_bgr888(u8* buf, int width, int height)
{
	int offset = 0;
	for (int x = 0; x < width; x++) 
	{
		for (int y = 0; y < height; y++) 
		{
			u8 r = *(u8*)(buf + offset);
			u8 g = *(u8*)(buf + offset + 1);
			u8 b = *(u8*)(buf + offset + 2);

			*(buf + offset) = b;
			*(buf + offset + 1) = g;
			*(buf + offset + 2) = r;
			offset += 3;
		}
	}
}

Result_with_string Util_converter_bgr888_rotate_90_degree(u8* bgr888, u8** rotated_bgr888, int width, int height, int* rotated_width, int* rotated_height)
{
	Result_with_string result;
	int offset;
	int rotated_offset = 0;

	*rotated_bgr888 = (u8*)Util_safe_linear_alloc(width * height * 3);
	if(*rotated_bgr888 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	*rotated_width = height;
	*rotated_height = width;

	for(int i = width - 1; i >= 0; i--)
	{
		offset = i * 3;
		for(int k = 0; k < height; k++)
		{
			memcpy(*rotated_bgr888 + rotated_offset, bgr888 + offset, 0x3);
			rotated_offset += 3;
			offset += width * 3;
		}
	}

	return result;
}

Result_with_string Util_converter_bgr888_to_yuv420p(u8* bgr888, u8** yuv420p, int width, int height)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;

	sws_context = sws_getContext(width, height, AV_PIX_FMT_BGR24,
	width, height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	if(sws_context == NULL)
	{
		result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
		result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
		return result;
	}

	*yuv420p = (u8*)Util_safe_linear_alloc(width * height * 1.5);
	if(*yuv420p == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	src_data[0] = bgr888;
	src_line_size[0] = width * 3;
	dst_data[0] = *yuv420p;
	dst_data[1] = *yuv420p + (width * height);
	dst_data[2] = *yuv420p + (width * height) + (width * height / 4);
	dst_line_size[0] = width;
	dst_line_size[1] = width / 2;
	dst_line_size[2] = width / 2;
	
	sws_scale(sws_context, src_data, src_line_size, 0, 
	height, dst_data, dst_line_size);

	sws_freeContext(sws_context);

	return result;
}

Result_with_string Util_converter_y2r_init(void)
{
	Result_with_string result;

	result.code = y2rInit();
	if(result.code != 0)
		result.string = "[Error] y2rInit() failed. ";
	
	return result;
}

Result_with_string Util_converter_y2r_yuv420p_to_bgr565(u8* yuv420p, u8** bgr565, int width, int height, bool texture_format)
{
	bool finished = false;
	Y2RU_ConversionParams y2r_parameters;
	Result_with_string result;

	*bgr565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(*bgr565 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	y2r_parameters.input_format = INPUT_YUV420_INDIV_8;
	y2r_parameters.output_format = OUTPUT_RGB_16_565;
	y2r_parameters.rotation = ROTATION_NONE;
	if(texture_format)
		y2r_parameters.block_alignment = BLOCK_8_BY_8;
	else
		y2r_parameters.block_alignment = BLOCK_LINE;
	y2r_parameters.input_line_width = width;
	y2r_parameters.input_lines = height;
	y2r_parameters.standard_coefficient = COEFFICIENT_ITU_R_BT_709_SCALING;
	y2r_parameters.alpha = 0xFF;

	result.code = Y2RU_SetConversionParams(&y2r_parameters);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetConversionParams() failed. ";
		return result;
	}

	result.code = Y2RU_SetSendingY(yuv420p, width * height, width, 0);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetSendingY() failed. ";
		return result;
	}

	result.code = Y2RU_SetSendingU(yuv420p + (width * height), width * height / 4, width / 2, 0);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetSendingU() failed. ";
		return result;
	}

	result.code = Y2RU_SetSendingV(yuv420p + ((width * height) + (width * height / 4)), width * height / 4, width / 2, 0);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetSendingV() failed. ";
		return result;
	}

	result.code = Y2RU_SetReceiving(*bgr565, width * height * 2, width * 2 * 4, 0);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetReceiving() failed. ";
		return result;
	}

	result.code = Y2RU_StartConversion();
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_StartConversion() failed. ";
		return result;
	}

	while(!finished)
	{
		Y2RU_IsDoneReceiving(&finished);
		usleep(1000);
	}

	return result;
}

void Util_converter_y2r_exit(void)
{
	y2rExit();
}