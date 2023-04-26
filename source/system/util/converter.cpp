#include "definitions.hpp"

#if DEF_ENABLE_SW_FFMPEG_COLOR_CONVERTER_API
#include "system/types.hpp"

#include "system/util/util.hpp"

extern "C" 
{
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

//Include myself.
#include "system/util/converter.hpp"

//Translation table for Pixel_format -> AVPixelFormat.
AVPixelFormat util_converter_pixel_format_table[PIXEL_FORMAT_MAX] = 
{
	//YUV*
	AV_PIX_FMT_YUV410P,
	AV_PIX_FMT_YUV411P,
	AV_PIX_FMT_YUV420P,
	AV_PIX_FMT_YUV420P9BE,
	AV_PIX_FMT_YUV420P9LE,
	AV_PIX_FMT_YUV420P10BE,
	AV_PIX_FMT_YUV420P10LE,
	AV_PIX_FMT_YUV420P12BE,
	AV_PIX_FMT_YUV420P12LE,
	AV_PIX_FMT_YUV420P14BE,
	AV_PIX_FMT_YUV420P14LE,
	AV_PIX_FMT_YUV420P16BE,
	AV_PIX_FMT_YUV420P16LE,
	AV_PIX_FMT_YUV422P,
	AV_PIX_FMT_YUV422P9BE,
	AV_PIX_FMT_YUV422P9LE,
	AV_PIX_FMT_YUV422P10BE,
	AV_PIX_FMT_YUV422P10LE,
	AV_PIX_FMT_YUV422P12BE,
	AV_PIX_FMT_YUV422P12LE,
	AV_PIX_FMT_YUV422P14BE,
	AV_PIX_FMT_YUV422P14LE,
	AV_PIX_FMT_YUV422P16BE,
	AV_PIX_FMT_YUV422P16LE,
	AV_PIX_FMT_YUV440P,
	AV_PIX_FMT_YUV440P10BE,
	AV_PIX_FMT_YUV440P10LE,
	AV_PIX_FMT_YUV440P12BE,
	AV_PIX_FMT_YUV440P12LE,
	AV_PIX_FMT_YUV444P,
	AV_PIX_FMT_YUV444P9BE,
	AV_PIX_FMT_YUV444P9LE,
	AV_PIX_FMT_YUV444P10BE,
	AV_PIX_FMT_YUV444P10LE,
	AV_PIX_FMT_YUV444P12BE,
	AV_PIX_FMT_YUV444P12LE,
	AV_PIX_FMT_YUV444P14BE,
	AV_PIX_FMT_YUV444P14LE,
	AV_PIX_FMT_YUV444P16BE,
	AV_PIX_FMT_YUV444P16LE,
	//YUVJ*
	AV_PIX_FMT_YUVJ411P,
	AV_PIX_FMT_YUVJ420P,
	AV_PIX_FMT_YUVJ422P,
	AV_PIX_FMT_YUVJ440P,
	AV_PIX_FMT_YUVJ444P,
	//YUVA*
	AV_PIX_FMT_YUVA420P,
	AV_PIX_FMT_YUVA420P9BE,
	AV_PIX_FMT_YUVA420P9LE,
	AV_PIX_FMT_YUVA420P10BE,
	AV_PIX_FMT_YUVA420P10LE,
	AV_PIX_FMT_YUVA420P16BE,
	AV_PIX_FMT_YUVA420P16LE,
	AV_PIX_FMT_YUVA422P,
	AV_PIX_FMT_YUVA422P9BE,
	AV_PIX_FMT_YUVA422P9LE,
	AV_PIX_FMT_YUVA422P10BE,
	AV_PIX_FMT_YUVA422P10LE,
	AV_PIX_FMT_YUVA422P16BE,
	AV_PIX_FMT_YUVA422P16LE,
	AV_PIX_FMT_YUVA444P,
	AV_PIX_FMT_YUVA444P9BE,
	AV_PIX_FMT_YUVA444P9LE,
	AV_PIX_FMT_YUVA444P10BE,
	AV_PIX_FMT_YUVA444P10LE,
	AV_PIX_FMT_YUVA444P16BE,
	AV_PIX_FMT_YUVA444P16LE,
	//UYVY*
	AV_PIX_FMT_UYVY422,
	//YUYV*
	AV_PIX_FMT_YUYV422,
	//YVYU*
	AV_PIX_FMT_YVYU422,
	//UYYVYY*
	AV_PIX_FMT_UYYVYY411,
	//RGB* (exclude RGBA)
	AV_PIX_FMT_RGB4,
	AV_PIX_FMT_RGB4_BYTE,
	AV_PIX_FMT_RGB8,
	AV_PIX_FMT_RGB444BE,
	AV_PIX_FMT_RGB444LE,
	AV_PIX_FMT_RGB555BE,
	AV_PIX_FMT_RGB555LE,
	AV_PIX_FMT_RGB565BE,
	AV_PIX_FMT_RGB565LE,
	AV_PIX_FMT_RGB24,
	AV_PIX_FMT_RGB48BE,
	AV_PIX_FMT_RGB48LE,
	//BGR* (exclude BGRA)
	AV_PIX_FMT_BGR4,
	AV_PIX_FMT_BGR4_BYTE,
	AV_PIX_FMT_BGR8,
	AV_PIX_FMT_BGR444BE,
	AV_PIX_FMT_BGR444LE,
	AV_PIX_FMT_BGR555BE,
	AV_PIX_FMT_BGR555LE,
	AV_PIX_FMT_BGR565BE,
	AV_PIX_FMT_BGR565LE,
	AV_PIX_FMT_BGR24,
	AV_PIX_FMT_BGR48BE,
	AV_PIX_FMT_BGR48LE,
	//GRB* (exclude GRBA)
	AV_PIX_FMT_GBRP,
	AV_PIX_FMT_GBRP9BE,
	AV_PIX_FMT_GBRP9LE,
	AV_PIX_FMT_GBRP10BE,
	AV_PIX_FMT_GBRP10LE,
	AV_PIX_FMT_GBRP12BE,
	AV_PIX_FMT_GBRP12LE,
	AV_PIX_FMT_GBRP14BE,
	AV_PIX_FMT_GBRP14LE,
	AV_PIX_FMT_GBRP16BE,
	AV_PIX_FMT_GBRP16LE,
	//ARGB*
	AV_PIX_FMT_ARGB,
	//ABGR*
	AV_PIX_FMT_ABGR,
	//RGBA*
	AV_PIX_FMT_RGBA,
	AV_PIX_FMT_RGBA64BE,
	AV_PIX_FMT_RGBA64LE,
	//BGRA*
	AV_PIX_FMT_BGRA,
	AV_PIX_FMT_BGRA64BE,
	AV_PIX_FMT_BGRA64LE,
	//GBRA*
	AV_PIX_FMT_GBRAP,
	AV_PIX_FMT_GBRAP10BE,
	AV_PIX_FMT_GBRAP10LE,
	AV_PIX_FMT_GBRAP12BE,
	AV_PIX_FMT_GBRAP12LE,
	AV_PIX_FMT_GBRAP16BE,
	AV_PIX_FMT_GBRAP16LE,
	//GRAY*
	AV_PIX_FMT_GRAY8,
	AV_PIX_FMT_GRAY10BE,
	AV_PIX_FMT_GRAY10LE,
	AV_PIX_FMT_GRAY12BE,
	AV_PIX_FMT_GRAY12LE,
	AV_PIX_FMT_GRAY16BE,
	AV_PIX_FMT_GRAY16LE,
	//GRAYALPHA*
	AV_PIX_FMT_YA8,
	AV_PIX_FMT_YA16BE,
	AV_PIX_FMT_YA16LE,
};

#endif

#if DEF_ENABLE_SW_FFMPEG_AUDIO_CONVERTER_API

extern "C" 
{
#include "libswresample/swresample.h"
}

//Translation table for Sample_format -> AVSampleFormat.
AVSampleFormat util_converter_sample_format_table[SAMPLE_FORMAT_MAX] = 
{
    AV_SAMPLE_FMT_U8,
    AV_SAMPLE_FMT_U8P,
    AV_SAMPLE_FMT_S16,
    AV_SAMPLE_FMT_S16P,
    AV_SAMPLE_FMT_S32,
    AV_SAMPLE_FMT_S32P,
    AV_SAMPLE_FMT_S64,
    AV_SAMPLE_FMT_S64P,
    AV_SAMPLE_FMT_FLT,
    AV_SAMPLE_FMT_FLTP,
    AV_SAMPLE_FMT_DBL,
    AV_SAMPLE_FMT_DBLP,
};

u8 util_converter_sample_format_size_table[] = 
{
    sizeof(u8),
    sizeof(u8),
    sizeof(s16),
    sizeof(s16),
    sizeof(s32),
    sizeof(s32),
    sizeof(s64),
    sizeof(s64),
    sizeof(float),
    sizeof(float),
    sizeof(double),
    sizeof(double),
};

#endif

#if DEF_ENABLE_SW_CONVERTER_API

#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)
// YUV -> RGB
#define C(Y) ( (Y) - 16  )
#define D(U) ( (U) - 128 )
#define E(V) ( (V) - 128 )

#define YUV2R(Y, V) CLIP(( 298 * C(Y)              + 409 * E(V) + 128) >> 8)
#define YUV2G(Y, U, V) CLIP(( 298 * C(Y) - 100 * D(U) - 208 * E(V) + 128) >> 8)
#define YUV2B(Y, U) CLIP(( 298 * C(Y) + 516 * D(U)              + 128) >> 8)

#endif

#if DEF_ENABLE_SW_ASM_CONVERTER_API

extern "C" void yuv420p_to_rgb565le_asm(u8* yuv420p, u8* rgb565, int width, int height);
extern "C" void yuv420p_to_rgb888le_asm(u8* yuv420p, u8* rgb888, int width, int height);

#endif

#if DEF_ENABLE_HW_CONVERTER_API

bool util_y2r_init = false;

#endif

#if DEF_ENABLE_SW_FFMPEG_COLOR_CONVERTER_API

Result_with_string Util_converter_convert_color(Color_converter_parameters* parameters)
{
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	int converted_image_size = 0;
	int ffmpeg_result = 0;
	u8* src_data[4] = { NULL, NULL, NULL, NULL, };
	u8* dst_data[4] = { NULL, NULL, NULL, NULL, };
	Result_with_string result;
	SwsContext* sws_context = NULL;

	if(!parameters || !parameters->source || parameters->in_width <= 0 || parameters->in_height <= 0 || parameters->out_width <= 0 || parameters->out_height <= 0
	|| parameters->in_color_format <= PIXEL_FORMAT_INVALID || parameters->in_color_format >= PIXEL_FORMAT_MAX || parameters->out_color_format <= PIXEL_FORMAT_INVALID || parameters->out_color_format >= PIXEL_FORMAT_MAX)
		goto invalid_arg;

	//Get required buffer size for output data.
	converted_image_size = av_image_get_buffer_size(util_converter_pixel_format_table[parameters->out_color_format], parameters->out_width, parameters->out_height, 1);
	if(converted_image_size <= 0)
	{
		result.error_description = "[Error] av_image_get_buffer_size() failed. " + std::to_string(converted_image_size) + " ";
		goto ffmpeg_api_failed;
	}

	Util_safe_linear_free(parameters->converted);
	parameters->converted = (u8*)Util_safe_linear_alloc(converted_image_size);
	if(!parameters->converted)
		goto out_of_memory;
	
	sws_context = sws_getContext(parameters->in_width, parameters->in_height, util_converter_pixel_format_table[parameters->in_color_format],
	parameters->out_width, parameters->out_height, util_converter_pixel_format_table[parameters->out_color_format], 0, 0, 0, 0);
	if(!sws_context)
	{
		result.error_description = "[Error] sws_getContext() failed. ";
		goto ffmpeg_api_failed;
	}

	//Set linesizes and pointers.
	ffmpeg_result = av_image_fill_arrays(src_data, src_line_size, parameters->source, util_converter_pixel_format_table[parameters->in_color_format], parameters->in_width, parameters->in_height, 1);
	if(ffmpeg_result <= 0)
	{
		result.error_description = "[Error] av_image_fill_arrays() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_image_fill_arrays(dst_data, dst_line_size, parameters->converted, util_converter_pixel_format_table[parameters->out_color_format], parameters->out_width, parameters->out_height, 1);
	if(ffmpeg_result <= 0)
	{
		result.error_description = "[Error] av_image_fill_arrays() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = sws_scale(sws_context, src_data, src_line_size, 0, parameters->in_height, dst_data, dst_line_size);
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
	Util_safe_linear_free(parameters->converted);
	parameters->converted = NULL;
	sws_freeContext(sws_context);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

#endif

#if DEF_ENABLE_SW_FFMPEG_AUDIO_CONVERTER_API

Result_with_string Util_converter_convert_audio(Audio_converter_parameters* parameters)
{
	u8* src_data[AV_NUM_DATA_POINTERS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, };
	u8* dst_data[AV_NUM_DATA_POINTERS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, };
	int ffmpeg_result = 0;
	Result_with_string result;
	AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
	AVChannelLayout in_ch_layout = AV_CHANNEL_LAYOUT_MONO;
	SwrContext* swr_context = NULL;

	if(!parameters || !parameters->source || parameters->in_ch <= 0 || parameters->in_ch > 8 || parameters->in_sample_rate <= 0 || parameters->in_samples <= 0
	|| parameters->in_sample_format <= SAMPLE_FORMAT_INVALID || parameters->in_sample_format >= SAMPLE_FORMAT_MAX || parameters->out_ch <= 0 || parameters->out_ch > 8
	|| parameters->out_sample_rate <= 0 || parameters->out_sample_format <= SAMPLE_FORMAT_INVALID || parameters->out_sample_format >= SAMPLE_FORMAT_MAX)
		goto invalid_arg;

	parameters->out_samples = 0;

	Util_safe_linear_free(parameters->converted);
	parameters->converted = NULL;
	parameters->converted = (u8*)Util_safe_linear_alloc(parameters->in_samples * util_converter_sample_format_size_table[parameters->out_sample_format] * parameters->out_ch);
	if(!parameters->converted)
		goto out_of_memory;

	parameters->out_samples = parameters->in_samples;

	av_channel_layout_default(&out_ch_layout, parameters->out_ch);
	av_channel_layout_default(&in_ch_layout, parameters->in_ch);

	ffmpeg_result = swr_alloc_set_opts2(&swr_context, &out_ch_layout, util_converter_sample_format_table[parameters->out_sample_format], parameters->out_sample_rate,
	&in_ch_layout, util_converter_sample_format_table[parameters->in_sample_format], parameters->in_sample_rate, 0, NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] swr_alloc_set_opts2() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = swr_init(swr_context);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] swr_init() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	src_data[0] = parameters->source;
	dst_data[0] = parameters->converted;

	if(parameters->in_sample_format == SAMPLE_FORMAT_U8P || parameters->in_sample_format == SAMPLE_FORMAT_S16P
	|| parameters->in_sample_format == SAMPLE_FORMAT_S32P || parameters->in_sample_format == SAMPLE_FORMAT_S64P
	|| parameters->in_sample_format == SAMPLE_FORMAT_FLOAT32P || parameters->in_sample_format == SAMPLE_FORMAT_DOUBLE64P)
	{
		for(int i = 1; i < parameters->in_ch; i++)
			src_data[i] = parameters->source + (parameters->in_samples * util_converter_sample_format_size_table[parameters->in_sample_format] * i);
	}
	if(parameters->out_sample_format == SAMPLE_FORMAT_U8P || parameters->out_sample_format == SAMPLE_FORMAT_S16P
	|| parameters->out_sample_format == SAMPLE_FORMAT_S32P || parameters->out_sample_format == SAMPLE_FORMAT_S64P
	|| parameters->out_sample_format == SAMPLE_FORMAT_FLOAT32P || parameters->out_sample_format == SAMPLE_FORMAT_DOUBLE64P)
	{
		for(int i = 1; i < parameters->out_ch; i++)
			dst_data[i] = parameters->converted + (parameters->out_samples * util_converter_sample_format_size_table[parameters->out_sample_format] * i);
	}

	ffmpeg_result = swr_convert(swr_context, dst_data, parameters->out_samples, (const u8**)src_data, parameters->in_samples);
	if(ffmpeg_result <= 0)
	{
		result.error_description = "[Error] swr_convert() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}
	parameters->out_samples = ffmpeg_result;

	av_channel_layout_uninit(&out_ch_layout);
	av_channel_layout_uninit(&in_ch_layout);
	swr_free(&swr_context);

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
	Util_safe_linear_free(parameters->converted);
	parameters->converted = NULL;
	swr_free(&swr_context);
	av_channel_layout_uninit(&out_ch_layout);
	av_channel_layout_uninit(&in_ch_layout);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

#endif

#if DEF_ENABLE_SW_CONVERTER_API

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

Result_with_string Util_converter_rgba8888be_to_rgba8888le(u8* rgba8888, int width, int height)
{
	int offset = 0;
	Result_with_string result;

	if(!rgba8888 || width <= 0 || height <= 0)
		goto invalid_arg;

	for (int x = 0; x < width; x++) 
	{
		for (int y = 0; y < height; y++) 
		{
			u8 r = *(u8*)(rgba8888 + offset);
			u8 g = *(u8*)(rgba8888 + offset + 1);
			u8 b = *(u8*)(rgba8888 + offset + 2);
			u8 a = *(u8*)(rgba8888 + offset + 3);

			*(rgba8888 + offset) = a;
			*(rgba8888 + offset + 1) = b;
			*(rgba8888 + offset + 2) = g;
			*(rgba8888 + offset + 3) = r;
			offset += 4;
		}
	}
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
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

#endif

#if DEF_ENABLE_SW_ASM_CONVERTER_API

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

#endif

#if DEF_ENABLE_HW_CONVERTER_API

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
		Util_sleep(500);
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

#endif
