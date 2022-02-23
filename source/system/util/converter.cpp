#include "system/headers.hpp"

extern "C" void memcpy_asm(u8*, u8*, int);
extern "C" void yuv420p_to_rgb565le_asm(u8* yuv420p, u8* rgb565, int width, int height);
extern "C" void yuv420p_to_rgb888le_asm(u8* yuv420p, u8* rgb888, int width, int height);

extern "C" 
{
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

bool util_y2r_init = false;

Result_with_string Util_converter_yuv422_to_rgb565le(u8* yuv422, u8** rgb565, int width, int height)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	int ffmpeg_result = 0;
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;

	if(!yuv422 || !rgb565 || width <= 0 || height <= 0)
		goto invalid_arg;

	Util_safe_linear_free(*rgb565);
	*rgb565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(!*rgb565)
		goto out_of_memory;

	sws_context = sws_getContext(width, height, AV_PIX_FMT_YUYV422,
	width, height, AV_PIX_FMT_RGB565LE, 0, 0, 0, 0);
	if(!sws_context)
	{
		result.error_description = "[Error] sws_getContext() failed. ";
		goto ffmpeg_api_failed;
	}

	src_data[0] = yuv422;
	src_line_size[0] = width * 2;
	dst_data[0] = *rgb565;
	dst_line_size[0] = width * 2;
	
	ffmpeg_result = sws_scale(sws_context, src_data, src_line_size, 0, height, dst_data, dst_line_size);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] sws_scale() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	sws_freeContext(sws_context);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	ffmpeg_api_failed:
	Util_safe_linear_free(*rgb565);
	*rgb565 = NULL;
	sws_freeContext(sws_context);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_converter_yuv422_to_yuv420p(u8* yuv422, u8** yuv420p, int width, int height)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	int ffmpeg_result = 0;
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;

	if(!yuv422 || !yuv420p || width <= 0 || height <= 0)
		goto invalid_arg;

	Util_safe_linear_free(*yuv420p);
	*yuv420p = (u8*)Util_safe_linear_alloc(width * height * 1.5);
	if(!*yuv420p)
		goto out_of_memory;
		
	sws_context = sws_getContext(width, height, AV_PIX_FMT_YUYV422,
	width, height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	if(!sws_context)
		goto ffmpeg_api_failed;

	src_data[0] = yuv422;
	src_line_size[0] = width * 2;
	dst_data[0] = *yuv420p;
	dst_data[1] = *yuv420p + (width * height);
	dst_data[2] = *yuv420p + (width * height) + (width * height / 4);
	dst_line_size[0] = width;
	dst_line_size[1] = width / 2;
	dst_line_size[2] = width / 2;
	
	ffmpeg_result = sws_scale(sws_context, src_data, src_line_size, 0, height, dst_data, dst_line_size);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] sws_scale() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	sws_freeContext(sws_context);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	ffmpeg_api_failed:
	Util_safe_linear_free(*yuv420p);
	*yuv420p = NULL;
	sws_freeContext(sws_context);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_converter_yuv420p_to_rgb565le(u8* yuv420p, u8** rgb565, int width, int height)
{
    int index = 0;
	u8 Y[4], U, V, r[4], g[4], b[4];
    u8* ybase = yuv420p;
    u8* ubase = yuv420p + width * height;
    u8* vbase = yuv420p + width * height + width * height / 4;
	Result_with_string result;

	if(!yuv420p || !rgb565 || width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	Util_safe_linear_free(*rgb565);
	*rgb565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(!*rgb565)
		goto out_of_memory;
	
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
			*(*rgb565 + index++) = (g[0] & 0b00000111) << 5 | b[0];
			*(*rgb565 + index++) = (g[0] & 0b00111000) >> 3 | (r[0] & 0b00011111) << 3;
		}
	}
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

Result_with_string Util_converter_yuv420p_to_rgb565le_asm(u8* yuv420p, u8** rgb565, int width, int height)
{
	Result_with_string result;

	if(!yuv420p || !rgb565 || width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	Util_safe_linear_free(*rgb565);
	*rgb565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(!*rgb565)
		goto out_of_memory;

	yuv420p_to_rgb565le_asm(yuv420p, *rgb565, width, height);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

Result_with_string Util_converter_yuv420p_to_rgb888le(u8* yuv420p, u8** rgb888, int width, int height)
{
    int index = 0;
    u8* ybase = yuv420p;
    u8* ubase = yuv420p + width * height;
    u8* vbase = yuv420p + width * height + width * height / 4;
	Result_with_string result;

	if(!yuv420p || !rgb888 || width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	Util_safe_linear_free(*rgb888);
	*rgb888 = (u8*)Util_safe_linear_alloc(width * height * 3);
	if(!*rgb888)
		goto out_of_memory;
	
	u8 Y[4], U, V;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			//YYYYYYYYUUVV
			Y[0] = *ybase++;
			U = ubase[y / 2 * width / 2 + (x / 2)];
			V = vbase[y / 2 * width / 2 + (x / 2)];
			
			*(*rgb888 + index++) = YUV2B(Y[0], U);
			*(*rgb888 + index++) = YUV2G(Y[0], U, V);
			*(*rgb888 + index++) = YUV2R(Y[0], V);
		}
	}
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

Result_with_string Util_converter_yuv420p_to_rgb888le_asm(u8* yuv420p, u8** rgb888, int width, int height)
{
	Result_with_string result;

	if(!yuv420p || !rgb888 || width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	Util_safe_linear_free(*rgb888);
	*rgb888 = (u8*)Util_safe_linear_alloc(width * height * 3);
	if(!*rgb888)
		goto out_of_memory;

	yuv420p_to_rgb888le_asm(yuv420p, *rgb888, width, height);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

Result_with_string Util_converter_rgb888be_to_rgb888le(u8* rgb888, int width, int height)
{
	int offset = 0;
	Result_with_string result;

	if(!rgb888 || width <= 0 || height <= 0)
		goto invalid_arg;

	for (int x = 0; x < width; x++) 
	{
		for (int y = 0; y < height; y++) 
		{
			u8 r = *(u8*)(rgb888 + offset);
			u8 g = *(u8*)(rgb888 + offset + 1);
			u8 b = *(u8*)(rgb888 + offset + 2);

			*(rgb888 + offset) = b;
			*(rgb888 + offset + 1) = g;
			*(rgb888 + offset + 2) = r;
			offset += 3;
		}
	}
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
}

Result_with_string Util_converter_rgb888_rotate_90_degree(u8* rgb888, u8** rotated_rgb888, int width, int height, int* rotated_width, int* rotated_height)
{
	Result_with_string result;
	int offset;
	int rotated_offset = 0;

	if(!rgb888 || !rotated_rgb888 || width <= 0 || height <= 0 || !rotated_width || !rotated_height)
		goto invalid_arg;

	Util_safe_linear_free(*rotated_rgb888);
	*rotated_rgb888 = (u8*)Util_safe_linear_alloc(width * height * 3);
	if(!*rotated_rgb888)
		goto out_of_memory;

	*rotated_width = height;
	*rotated_height = width;

	for(int i = width - 1; i >= 0; i--)
	{
		offset = i * 3;
		for(int k = 0; k < height; k++)
		{
			memcpy(*rotated_rgb888 + rotated_offset, rgb888 + offset, 0x3);
			rotated_offset += 3;
			offset += width * 3;
		}
	}

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

Result_with_string Util_converter_rgb888le_to_yuv420p(u8* rgb888, u8** yuv420p, int width, int height)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	int ffmpeg_result = 0;
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;

	if(!rgb888 || !yuv420p || width <= 0 || height <= 0)
		goto invalid_arg;

	Util_safe_linear_free(*yuv420p);
	*yuv420p = (u8*)Util_safe_linear_alloc(width * height * 1.5);
	if(!*yuv420p)
		goto out_of_memory;
	
	sws_context = sws_getContext(width, height, AV_PIX_FMT_BGR24,
	width, height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	if(!sws_context)
	{
		result.error_description = "[Error] sws_getContext() failed. ";
		goto ffmpeg_api_failed;
	}

	src_data[0] = rgb888;
	src_line_size[0] = width * 3;
	dst_data[0] = *yuv420p;
	dst_data[1] = *yuv420p + (width * height);
	dst_data[2] = *yuv420p + (width * height) + (width * height / 4);
	dst_line_size[0] = width;
	dst_line_size[1] = width / 2;
	dst_line_size[2] = width / 2;
	
	ffmpeg_result = sws_scale(sws_context, src_data, src_line_size, 0, height, dst_data, dst_line_size);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] sws_scale() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	sws_freeContext(sws_context);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	ffmpeg_api_failed:
	Util_safe_linear_free(*yuv420p);
	*yuv420p = NULL;
	sws_freeContext(sws_context);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_converter_rgb565le_to_rgb888le(u8* rgb565, u8** rgb888, int width, int height)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	int ffmpeg_result = 0;
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;

	if(!rgb565 || !rgb888 || width <= 0 || height <= 0)
		goto invalid_arg;

	Util_safe_linear_free(*rgb888);
	*rgb888 = (u8*)Util_safe_linear_alloc(width * height * 3);
	if(!*rgb888)
		goto out_of_memory;
	
	sws_context = sws_getContext(width, height, AV_PIX_FMT_BGR565LE,
	width, height, AV_PIX_FMT_BGR24, 0, 0, 0, 0);
	if(!sws_context)
	{
		result.error_description = "[Error] sws_getContext() failed. ";
		goto ffmpeg_api_failed;
	}

	src_data[0] = rgb565;
	src_line_size[0] = width * 2;
	dst_data[0] = *rgb888;
	dst_line_size[0] = width * 3;
	
	ffmpeg_result = sws_scale(sws_context, src_data, src_line_size, 0, height, dst_data, dst_line_size);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] sws_scale() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	sws_freeContext(sws_context);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	ffmpeg_api_failed:
	Util_safe_linear_free(*rgb888);
	*rgb888 = NULL;
	sws_freeContext(sws_context);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_converter_y2r_init(void)
{
	Result_with_string result;

	if(util_y2r_init)
		goto already_inited;

	result.code = y2rInit();
	if(result.code != 0)
	{
		result.error_description = "[Error] y2rInit() failed. ";
		goto nintendo_api_failed;
	}

	util_y2r_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_converter_y2r_yuv420p_to_rgb565le(u8* yuv420p, u8** rgb565, int width, int height, bool texture_format)
{
	bool finished = false;
	Y2RU_ConversionParams y2r_parameters;
	Result_with_string result;

	if(!util_y2r_init)
		goto not_inited;
	
	if(!yuv420p || !rgb565 || width <= 0 || height <= 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	Util_safe_linear_free(*rgb565);
	*rgb565 = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(!*rgb565)
		goto out_of_memory;
	
	y2r_parameters.input_format = INPUT_YUV420_INDIV_8;
	y2r_parameters.output_format = OUTPUT_RGB_16_565;
	y2r_parameters.rotation = ROTATION_NONE;
	y2r_parameters.block_alignment = texture_format ? BLOCK_8_BY_8 : BLOCK_LINE;
	y2r_parameters.input_line_width = width;
	y2r_parameters.input_lines = height;
	y2r_parameters.standard_coefficient = COEFFICIENT_ITU_R_BT_709_SCALING;
	y2r_parameters.alpha = 0xFF;

	result.code = Y2RU_SetConversionParams(&y2r_parameters);
	if(result.code != 0)
	{
		result.error_description = "[Error] Y2RU_SetConversionParams() failed. ";
		goto nintendo_api_failed;
	}

	result.code = Y2RU_SetSendingY(yuv420p, width * height, width, 0);
	if(result.code != 0)
	{
		result.error_description = "[Error] Y2RU_SetSendingY() failed. ";
		goto nintendo_api_failed;
	}

	result.code = Y2RU_SetSendingU(yuv420p + (width * height), width * height / 4, width / 2, 0);
	if(result.code != 0)
	{
		result.error_description = "[Error] Y2RU_SetSendingU() failed. ";
		goto nintendo_api_failed;
	}

	result.code = Y2RU_SetSendingV(yuv420p + ((width * height) + (width * height / 4)), width * height / 4, width / 2, 0);
	if(result.code != 0)
	{
		result.error_description = "[Error] Y2RU_SetSendingV() failed. ";
		goto nintendo_api_failed;
	}

	result.code = Y2RU_SetReceiving(*rgb565, width * height * 2, width * 2 * 4, 0);
	if(result.code != 0)
	{
		result.error_description = "[Error] Y2RU_SetReceiving() failed. ";
		goto nintendo_api_failed;
	}

	result.code = Y2RU_StartConversion();
	if(result.code != 0)
	{
		result.error_description = "[Error] Y2RU_StartConversion() failed. ";
		goto nintendo_api_failed;
	}

	while(!finished)
	{
		result.code = Y2RU_IsDoneReceiving(&finished);
		if(result.code != 0)
		{
			result.error_description = "[Error] Y2RU_IsDoneReceiving() failed. ";
			goto nintendo_api_failed;
		}
		usleep(500);
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_converter_y2r_exit(void)
{
	if(util_y2r_init)
		y2rExit();
	
	util_y2r_init = false;
}
