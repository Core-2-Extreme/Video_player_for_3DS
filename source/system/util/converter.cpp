#include "headers.hpp"

extern "C" void memcpy_asm(u8*, u8*, int);

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

	*bgr565 = (u8*)malloc(width * height * 2);
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

Result_with_string Util_converter_yuv420p_to_bgr565(u8* yuv420p, u8** bgr565, int width, int height)
{
    int index = 0;
	/*int uv_pos = 0;
	int y_pos = 0;*/
    u8* ybase = yuv420p;
    u8* ubase = yuv420p + width * height;
    u8* vbase = yuv420p + width * height + width * height / 4;
	Result_with_string result;

	*bgr565 = (u8*)malloc(width * height * 2);
	if(*bgr565 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}
	
	u8 Y[4], U, V, r[4], g[4], b[4];
	if(true)
	{
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
	}
	else
	{
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x += 4)
			{
				//YYYYYYYYUUVV
				/*uv_pos = y / 2 * width / 2 + (x / 2);
				y_pos = x + y * width;*/
				U = *ubase++;
				V = *vbase++;
				Y[0] = *ybase++;
				Y[1] = *ybase++;
				Y[2] = *ybase++;
				Y[3] = *ybase++;

				/*U = ubase[uv_pos];
				V = vbase[uv_pos];
				Y[0] = ybase[y_pos];
				Y[1] = ybase[y_pos + 1];
				Y[2] = ybase[y_pos + 2];
				Y[3] = ybase[y_pos + 3];*/

				b[0] = YUV2B(Y[0], U);
				g[0] = YUV2G(Y[0], U, V);
				r[0] = YUV2R(Y[0], V);
				b[1] = YUV2B(Y[1], U);
				g[1] = YUV2G(Y[1], U, V);
				r[1] = YUV2R(Y[1], V);
				b[2] = YUV2B(Y[2], U);
				g[2] = YUV2G(Y[2], U, V);
				r[2] = YUV2R(Y[2], V);
				b[3] = YUV2B(Y[3], U);
				g[3] = YUV2G(Y[3], U, V);
				r[3] = YUV2R(Y[3], V);

				b[0] = b[0] >> 3;
				g[0] = g[0] >> 2;
				r[0] = r[0] >> 3;
				b[1] = b[1] >> 3;
				g[1] = g[1] >> 2;
				r[1] = r[1] >> 3;
				b[2] = b[2] >> 3;
				g[2] = g[2] >> 2;
				r[2] = r[2] >> 3;
				b[3] = b[3] >> 3;
				g[3] = g[3] >> 2;
				r[3] = r[3] >> 3;

				*bgr565[index++] = (g[0] & 0b00000111) << 5 | b[0];
				*bgr565[index++] = (g[0] & 0b00111000) >> 3 | (r[0] & 0b00011111) << 3;
				*bgr565[index++] = (g[1] & 0b00000111) << 5 | b[1];
				*bgr565[index++] = (g[1] & 0b00111000) >> 3 | (r[1] & 0b00011111) << 3;
				*bgr565[index++] = (g[2] & 0b00000111) << 5 | b[2];
				*bgr565[index++] = (g[2] & 0b00111000) >> 3 | (r[2] & 0b00011111) << 3;
				*bgr565[index++] = (g[3] & 0b00000111) << 5 | b[3];
				*bgr565[index++] = (g[3] & 0b00111000) >> 3 | (r[3] & 0b00011111) << 3;
			}
		}
	}
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

	*rotated_bgr888 = (u8*)malloc(width * height * 3);
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

	*yuv420p = (u8*)malloc(width * height * 1.5);
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

	/*Result_with_string result;
	u8* y_pos = NULL;
	u8* u_pos = NULL;
	u8* v_pos = NULL;
	u8 b, g, r, y, u, v;

	*yuv420p = (u8*)malloc(width * height * 1.5);
	if(*yuv420p == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}
	y_pos = *yuv420p;
	u_pos = *yuv420p + (width * height);
	v_pos = *yuv420p + (width * height) + (width * height / 4);

	for(int i = 0; i < height; i++)
	{
		for(int k = 0; k < width; k += 4)
		{
			b = *bgr888++;
			g = *bgr888++;
			r = *bgr888++;
			y = ((66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
            u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128 ;
            v = (( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
			//y = (77 * r + 150 * g + 29 * b) >> 8;
			//u = ((128 * b - 43 * r -85 * g) >> 8) + 128;
			//v = ((128 * r - 107 * g -21 * b) >> 8 ) + 128;
			if(y > 255)
				y = 255;
			else if(y < 0)
				y = 0;
			if(u > 255)
				u = 255;
			else if(u < 0)
				u = 0;
			if(v > 255)
				v = 255;
			else if(v < 0)
				v = 0;
			
			*y_pos++ = y;
			*u_pos++ = u;
			*v_pos++ = v;

			b = *bgr888++;
			g = *bgr888++;
			r = *bgr888++;
			y = ((66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
			//y = (77 * r + 150 * g + 29 * b) >> 8;
			if(y > 255)
				y = 255;
			else if(y < 0)
				y = 0;
			*y_pos++ = y;

			b = *bgr888++;
			g = *bgr888++;
			r = *bgr888++;
			y = ((66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
			//y = (77 * r + 150 * g + 29 * b) >> 8;
			if(y > 255)
				y = 255;
			else if(y < 0)
				y = 0;
			*y_pos++ = y;

			b = *bgr888++;
			g = *bgr888++;
			r = *bgr888++;
			y = ((66 * r + 129 * g +  25 * b + 128) >> 8) + 16;
			//y = (77 * r + 150 * g + 29 * b) >> 8;
			if(y > 255)
				y = 255;
			else if(y < 0)
				y = 0;
			*y_pos++ = y;
		}
	}
	
	return result;*/
}

Result_with_string Util_converter_y2r_init(void)
{
	Result_with_string result;

	result.code = y2rInit();
	if(result.code != 0)
		result.string = "[Error] y2rInit() failed. ";
	
	return result;
}

Result_with_string Util_converter_y2r_yuv420p_to_bgr565(u8* yuv420p, u8** bgr565, int width, int height)
{
	bool finished = false;
	Y2RU_ConversionParams y2r_parameters;
	Result_with_string result;

	*bgr565 = (u8*)malloc(width * height * 2);
	if(*bgr565 == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	y2r_parameters.input_format = INPUT_YUV420_INDIV_8;
	y2r_parameters.output_format = OUTPUT_RGB_16_565;
	y2r_parameters.rotation = ROTATION_NONE;
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

	/*result.code = Y2RU_SetInputFormat(INPUT_YUV420_INDIV_8);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetInputFormat() failed. ";
		return result;
	}

	result.code = Y2RU_SetOutputFormat(OUTPUT_RGB_16_565);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetOutputFormat() failed. ";
		return result;
	}

	result.code = Y2RU_SetInputLineWidth(width);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetInputLineWidth() failed. ";
		return result;
	}

	result.code = Y2RU_SetInputLines(height);
	if(result.code != 0)
	{
		result.string = "[Error] Y2RU_SetInputLines() failed. ";
		return result;
	}*/

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