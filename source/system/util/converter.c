//Includes.
#include "system/util/converter.h"

#if (DEF_CONVERTER_HW_API_ENABLE || DEF_CONVERTER_SW_ASM_API_ENABLE || DEF_CONVERTER_SW_API_ENABLE || DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE || DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE)
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#if DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#endif //DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE

#if DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE
#include "libswresample/swresample.h"
#endif //DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/util.h"

//Defines.
#if DEF_CONVERTER_SW_API_ENABLE
#define CLIP(x) ((x) > 255 ? 255 : (x) < 0 ? 0 : x)
// YUV -> RGB
#define C(y) ((y) - 16)
#define D(u) ((u) - 128)
#define E(v) ((v) - 128)

#define YUV2R(y, v) 	CLIP((298 * C(y) + 409 * E(v) + 128) >> 8)
#define YUV2G(y, u, v) 	CLIP((298 * C(y) - 100 * D(u) - 208 * E(v) + 128) >> 8)
#define YUV2B(y, u) 	CLIP((298 * C(y) + 516 * D(u) + 128) >> 8)
#endif //DEF_CONVERTER_SW_API_ENABLE

//Typedefs.
//N/A.

//Prototypes.
#if DEF_CONVERTER_SW_ASM_API_ENABLE
extern void yuv420p_to_rgb565le_asm(const uint8_t* yuv420p, uint8_t* rgb565, uint32_t width, uint32_t height);
extern void yuv420p_to_rgb888le_asm(const uint8_t* yuv420p, uint8_t* rgb888, uint32_t width, uint32_t height);
#endif //DEF_CONVERTER_SW_ASM_API_ENABLE

//Variables.
#if DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE
//Translation table for Raw_pixel -> AVPixelFormat.
static enum AVPixelFormat util_converter_pixel_format_table[RAW_PIXEL_MAX] =
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
#endif //DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE

#if DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE
//Translation table for Raw_sample -> AVSampleFormat.
static enum AVSampleFormat util_converter_sample_format_table[RAW_SAMPLE_MAX] =
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
static uint8_t util_converter_sample_format_size_table[] =
{
	sizeof(uint8_t),
	sizeof(uint8_t),
	sizeof(int16_t),
	sizeof(int16_t),
	sizeof(int32_t),
	sizeof(int32_t),
	sizeof(int64_t),
	sizeof(int64_t),
	sizeof(float),
	sizeof(float),
	sizeof(double),
	sizeof(double),
};
#endif //DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE

#if DEF_CONVERTER_HW_API_ENABLE
static bool util_y2r_init = false;
#endif //DEF_CONVERTER_HW_API_ENABLE

//Code.
#if DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE
uint32_t Util_converter_convert_color(Converter_color_parameters* parameters)
{
	//We can't get rid of this "int" because library uses "int" type as args.
	int src_line_size[4] = { 0, 0, 0, 0, };
	int dst_line_size[4] = { 0, 0, 0, 0, };
	uint8_t* src_data[4] = { NULL, NULL, NULL, NULL, };
	uint8_t* dst_data[4] = { NULL, NULL, NULL, NULL, };
	int32_t converted_image_size = 0;
	int32_t ffmpeg_result = 0;
	struct SwsContext* sws_context = NULL;

	if(!parameters || !parameters->source || parameters->in_width == 0 || parameters->in_height == 0 || parameters->out_width == 0 || parameters->out_height == 0
	|| parameters->in_color_format <= RAW_PIXEL_INVALID || parameters->in_color_format >= RAW_PIXEL_MAX || parameters->out_color_format <= RAW_PIXEL_INVALID || parameters->out_color_format >= RAW_PIXEL_MAX)
		goto invalid_arg;

	//Get required buffer size for output data.
	converted_image_size = av_image_get_buffer_size(util_converter_pixel_format_table[parameters->out_color_format], parameters->out_width, parameters->out_height, 1);
	if(converted_image_size <= 0)
	{
		DEF_LOG_RESULT(av_image_get_buffer_size, false, converted_image_size);
		goto ffmpeg_api_failed;
	}

	free(parameters->converted);
	parameters->converted = (uint8_t*)linearAlloc(converted_image_size);
	if(!parameters->converted)
		goto out_of_memory;

	sws_context = sws_getContext(parameters->in_width, parameters->in_height, util_converter_pixel_format_table[parameters->in_color_format],
	parameters->out_width, parameters->out_height, util_converter_pixel_format_table[parameters->out_color_format], 0, 0, 0, 0);
	if(!sws_context)
	{
		DEF_LOG_RESULT(sws_getContext, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	//Set linesizes and pointers.
	ffmpeg_result = av_image_fill_arrays(src_data, src_line_size, parameters->source, util_converter_pixel_format_table[parameters->in_color_format], parameters->in_width, parameters->in_height, 1);
	if(ffmpeg_result <= 0)
	{
		DEF_LOG_RESULT(av_image_fill_arrays, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_image_fill_arrays(dst_data, dst_line_size, parameters->converted, util_converter_pixel_format_table[parameters->out_color_format], parameters->out_width, parameters->out_height, 1);
	if(ffmpeg_result <= 0)
	{
		DEF_LOG_RESULT(av_image_fill_arrays, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = sws_scale(sws_context, (const uint8_t* const*)src_data, src_line_size, 0, parameters->in_height, dst_data, dst_line_size);
	if(ffmpeg_result < 0)
	{
		DEF_LOG_RESULT(sws_scale, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	sws_freeContext(sws_context);

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	ffmpeg_api_failed:
	free(parameters->converted);
	parameters->converted = NULL;
	sws_freeContext(sws_context);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}
#endif //DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE

#if DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE
uint32_t Util_converter_convert_audio(Converter_audio_parameters* parameters)
{
	uint8_t* src_data[AV_NUM_DATA_POINTERS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, };
	uint8_t* dst_data[AV_NUM_DATA_POINTERS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, };
	int32_t ffmpeg_result = 0;
	AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
	AVChannelLayout in_ch_layout = AV_CHANNEL_LAYOUT_MONO;
	SwrContext* swr_context = NULL;

	if(!parameters || !parameters->source || parameters->in_ch == 0 || parameters->in_ch > 8 || parameters->in_sample_rate == 0 || parameters->in_samples == 0
	|| parameters->in_sample_format <= RAW_SAMPLE_INVALID || parameters->in_sample_format >= RAW_SAMPLE_MAX || parameters->out_ch == 0 || parameters->out_ch > 8
	|| parameters->out_sample_rate == 0 || parameters->out_sample_format <= RAW_SAMPLE_INVALID || parameters->out_sample_format >= RAW_SAMPLE_MAX)
		goto invalid_arg;

	parameters->out_samples = 0;

	free(parameters->converted);
	parameters->converted = NULL;
	parameters->converted = (uint8_t*)linearAlloc(parameters->in_samples * util_converter_sample_format_size_table[parameters->out_sample_format] * parameters->out_ch);
	if(!parameters->converted)
		goto out_of_memory;

	parameters->out_samples = parameters->in_samples;

	av_channel_layout_default(&out_ch_layout, parameters->out_ch);
	av_channel_layout_default(&in_ch_layout, parameters->in_ch);

	ffmpeg_result = swr_alloc_set_opts2(&swr_context, &out_ch_layout, util_converter_sample_format_table[parameters->out_sample_format], parameters->out_sample_rate,
	&in_ch_layout, util_converter_sample_format_table[parameters->in_sample_format], parameters->in_sample_rate, 0, NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(swr_alloc_set_opts2, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = swr_init(swr_context);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(swr_init, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	src_data[0] = parameters->source;
	dst_data[0] = parameters->converted;

	if(parameters->in_sample_format == RAW_SAMPLE_U8P || parameters->in_sample_format == RAW_SAMPLE_S16P
	|| parameters->in_sample_format == RAW_SAMPLE_S32P || parameters->in_sample_format == RAW_SAMPLE_S64P
	|| parameters->in_sample_format == RAW_SAMPLE_FLOAT32P || parameters->in_sample_format == RAW_SAMPLE_DOUBLE64P)
	{
		for(uint8_t i = 1; i < parameters->in_ch; i++)
			src_data[i] = parameters->source + (parameters->in_samples * util_converter_sample_format_size_table[parameters->in_sample_format] * i);
	}
	if(parameters->out_sample_format == RAW_SAMPLE_U8P || parameters->out_sample_format == RAW_SAMPLE_S16P
	|| parameters->out_sample_format == RAW_SAMPLE_S32P || parameters->out_sample_format == RAW_SAMPLE_S64P
	|| parameters->out_sample_format == RAW_SAMPLE_FLOAT32P || parameters->out_sample_format == RAW_SAMPLE_DOUBLE64P)
	{
		for(uint8_t i = 1; i < parameters->out_ch; i++)
			dst_data[i] = parameters->converted + (parameters->out_samples * util_converter_sample_format_size_table[parameters->out_sample_format] * i);
	}

	ffmpeg_result = swr_convert(swr_context, dst_data, parameters->out_samples, (const uint8_t* const*)src_data, parameters->in_samples);
	if(ffmpeg_result <= 0)
	{
		DEF_LOG_RESULT(swr_convert, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}
	parameters->out_samples = ffmpeg_result;

	av_channel_layout_uninit(&out_ch_layout);
	av_channel_layout_uninit(&in_ch_layout);
	swr_free(&swr_context);

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	ffmpeg_api_failed:
	free(parameters->converted);
	parameters->converted = NULL;
	swr_free(&swr_context);
	av_channel_layout_uninit(&out_ch_layout);
	av_channel_layout_uninit(&in_ch_layout);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}
#endif //DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE

#if DEF_CONVERTER_SW_API_ENABLE
uint32_t Util_converter_yuv420p_to_rgb565le(const uint8_t* yuv420p, uint8_t** rgb565, uint32_t width, uint32_t height)
{
	uint32_t index = 0;
	uint8_t y[4] = { 0, }, u = 0, v = 0, r[4] = { 0, }, g[4] = { 0, }, b[4] = { 0, };
	const uint8_t* ybase = NULL;
	const uint8_t* ubase = NULL;
	const uint8_t* vbase = NULL;

	if(!yuv420p || !rgb565 || width == 0 || height == 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	free(*rgb565);
	*rgb565 = (uint8_t*)linearAlloc(width * height * 2);
	if(!*rgb565)
		goto out_of_memory;

	ybase = yuv420p;
	ubase = yuv420p + width * height;
	vbase = yuv420p + width * height + width * height / 4;

	for (uint32_t h = 0; h < height; h++)
	{
		for (uint32_t w = 0; w < width; w++)
		{
			//YYYYYYYYUUVV
			y[0] = ybase[w + h * width];
			u = ubase[h / 2 * width / 2 + (w / 2)];
			v = vbase[h / 2 * width / 2 + (w / 2)];
			b[0] = YUV2B(y[0], u);
			g[0] = YUV2G(y[0], u, v);
			r[0] = YUV2R(y[0], v);
			b[0] = b[0] >> 3;
			g[0] = g[0] >> 2;
			r[0] = r[0] >> 3;
			*(*rgb565 + index++) = (g[0] & 0x07) << 5 | b[0];
			*(*rgb565 + index++) = (g[0] & 0x38) >> 3 | (r[0] & 0x1F) << 3;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Util_converter_yuv420p_to_rgb888le(const uint8_t* yuv420p, uint8_t** rgb888, uint32_t width, uint32_t height)
{
	uint32_t index = 0;
	uint8_t y[4] = { 0, }, u = 0, v = 0;
	const uint8_t* ybase = NULL;
	const uint8_t* ubase = NULL;
	const uint8_t* vbase = NULL;

	if(!yuv420p || !rgb888 || width == 0 || height == 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	free(*rgb888);
	*rgb888 = (uint8_t*)linearAlloc(width * height * 3);
	if(!*rgb888)
		goto out_of_memory;

	ybase = yuv420p;
	ubase = yuv420p + width * height;
	vbase = yuv420p + width * height + width * height / 4;

	for (uint32_t h = 0; h < height; h++)
	{
		for (uint32_t w = 0; w < width; w++)
		{
			//YYYYYYYYUUVV
			y[0] = *ybase++;
			u = ubase[h / 2 * width / 2 + (w / 2)];
			v = vbase[h / 2 * width / 2 + (w / 2)];

			*(*rgb888 + index++) = YUV2B(y[0], u);
			*(*rgb888 + index++) = YUV2G(y[0], u, v);
			*(*rgb888 + index++) = YUV2R(y[0], v);
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Util_converter_rgba8888be_to_rgba8888le(uint8_t* rgba8888, uint32_t width, uint32_t height)
{
	uint32_t offset = 0;

	if(!rgba8888 || width == 0 || height == 0)
		goto invalid_arg;

	for (uint32_t w = 0; w < width; w++)
	{
		for (uint32_t h = 0; h < height; h++)
		{
			uint8_t r = *(rgba8888 + offset);
			uint8_t g = *(rgba8888 + offset + 1);
			uint8_t b = *(rgba8888 + offset + 2);
			uint8_t a = *(rgba8888 + offset + 3);

			*(rgba8888 + offset) = a;
			*(rgba8888 + offset + 1) = b;
			*(rgba8888 + offset + 2) = g;
			*(rgba8888 + offset + 3) = r;
			offset += 4;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_converter_rgb888be_to_rgb888le(uint8_t* rgb888, uint32_t width, uint32_t height)
{
	uint32_t offset = 0;

	if(!rgb888 || width == 0 || height == 0)
		goto invalid_arg;

	for (uint32_t w = 0; w < width; w++)
	{
		for (uint32_t h = 0; h < height; h++)
		{
			uint8_t r = *(rgb888 + offset);
			uint8_t g = *(rgb888 + offset + 1);
			uint8_t b = *(rgb888 + offset + 2);

			*(rgb888 + offset) = b;
			*(rgb888 + offset + 1) = g;
			*(rgb888 + offset + 2) = r;
			offset += 3;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_converter_rgb888_rotate_90_degree(const uint8_t* rgb888, uint8_t** rotated_rgb888, uint32_t width, uint32_t height, uint32_t* rotated_width, uint32_t* rotated_height)
{
	uint32_t offset = 0;
	uint32_t rotated_offset = 0;

	if(!rgb888 || !rotated_rgb888 || width == 0 || height == 0 || !rotated_width || !rotated_height)
		goto invalid_arg;

	free(*rotated_rgb888);
	*rotated_rgb888 = (uint8_t*)linearAlloc(width * height * 3);
	if(!*rotated_rgb888)
		goto out_of_memory;

	*rotated_width = height;
	*rotated_height = width;

	for(int32_t i = width - 1; i >= 0; i--)
	{
		offset = i * 3;
		for(uint32_t k = 0; k < height; k++)
		{
			memcpy(*rotated_rgb888 + rotated_offset, rgb888 + offset, 0x3);
			rotated_offset += 3;
			offset += width * 3;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}
#endif //DEF_CONVERTER_SW_API_ENABLE

#if DEF_CONVERTER_SW_ASM_API_ENABLE
uint32_t Util_converter_yuv420p_to_rgb565le_asm(const uint8_t* yuv420p, uint8_t** rgb565, uint32_t width, uint32_t height)
{
	if(!yuv420p || !rgb565 || width == 0 || height == 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	free(*rgb565);
	*rgb565 = (uint8_t*)linearAlloc(width * height * 2);
	if(!*rgb565)
		goto out_of_memory;

	yuv420p_to_rgb565le_asm(yuv420p, *rgb565, width, height);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Util_converter_yuv420p_to_rgb888le_asm(const uint8_t* yuv420p, uint8_t** rgb888, uint32_t width, uint32_t height)
{
	if(!yuv420p || !rgb888 || width == 0 || height == 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	free(*rgb888);
	*rgb888 = (uint8_t*)linearAlloc(width * height * 3);
	if(!*rgb888)
		goto out_of_memory;

	yuv420p_to_rgb888le_asm(yuv420p, *rgb888, width, height);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}
#endif //DEF_CONVERTER_SW_ASM_API_ENABLE

#if DEF_CONVERTER_HW_API_ENABLE
uint32_t Util_converter_y2r_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_y2r_init)
		goto already_inited;

	result = y2rInit();
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(y2rInit, false, result);
		goto nintendo_api_failed;
	}

	util_y2r_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed:
	return result;
}

uint32_t Util_converter_y2r_yuv420p_to_rgb565le(const uint8_t* yuv420p, uint8_t** rgb565, uint16_t width, uint16_t height, bool texture_format)
{
	uint32_t result = DEF_ERR_OTHER;
	Y2RU_ConversionParams y2r_parameters;
	Handle conversion_finish_event_handle = 0;

	if(!util_y2r_init)
		goto not_inited;

	if(!yuv420p || !rgb565 || width == 0 || height == 0 || width % 2 != 0 || height % 2 != 0)
		goto invalid_arg;

	free(*rgb565);
	*rgb565 = (uint8_t*)linearAlloc(width * height * 2);
	if(!*rgb565)
		goto out_of_memory;

	memset(&y2r_parameters, 0x0, sizeof(y2r_parameters));
	y2r_parameters.input_format = INPUT_YUV420_INDIV_8;
	y2r_parameters.output_format = OUTPUT_RGB_16_565;
	y2r_parameters.rotation = ROTATION_NONE;
	y2r_parameters.block_alignment = texture_format ? BLOCK_8_BY_8 : BLOCK_LINE;
	y2r_parameters.input_line_width = width;
	y2r_parameters.input_lines = height;
	y2r_parameters.standard_coefficient = COEFFICIENT_ITU_R_BT_709_SCALING;
	y2r_parameters.alpha = 0xFF;

	result = Y2RU_SetConversionParams(&y2r_parameters);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Y2RU_SetConversionParams, false, result);
		goto nintendo_api_failed;
	}

	result = Y2RU_SetSendingY(yuv420p, width * height, width, 0);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Y2RU_SetSendingY, false, result);
		goto nintendo_api_failed;
	}

	result = Y2RU_SetSendingU(yuv420p + (width * height), width * height / 4, width / 2, 0);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Y2RU_SetSendingU, false, result);
		goto nintendo_api_failed;
	}

	result = Y2RU_SetSendingV(yuv420p + ((width * height) + (width * height / 4)), width * height / 4, width / 2, 0);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Y2RU_SetSendingV, false, result);
		goto nintendo_api_failed;
	}

	result = Y2RU_SetReceiving(*rgb565, width * height * 2, width * 2 * 4, 0);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Y2RU_SetReceiving, false, result);
		goto nintendo_api_failed;
	}

	result = Y2RU_StartConversion();
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Y2RU_StartConversion, false, result);
		goto nintendo_api_failed;
	}

	result = Y2RU_GetTransferEndEvent(&conversion_finish_event_handle);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Y2RU_GetTransferEndEvent, false, result);
		goto nintendo_api_failed;
	}

	svcWaitSynchronization(conversion_finish_event_handle, 200000000);//Wait up to 200ms.
	svcCloseHandle(conversion_finish_event_handle);

	return result;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed:
	return result;
}

void Util_converter_y2r_exit(void)
{
	if(util_y2r_init)
		y2rExit();

	util_y2r_init = false;
}
#endif //DEF_CONVERTER_HW_API_ENABLE

#endif //(DEF_CONVERTER_HW_API_ENABLE || DEF_CONVERTER_SW_ASM_API_ENABLE || DEF_CONVERTER_SW_API_ENABLE || DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE || DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE)
