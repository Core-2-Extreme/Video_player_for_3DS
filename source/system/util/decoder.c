//Includes.
#include "system/util/decoder.h"

#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"

#include "system/util/converter_types.h"
#include "system/util/err_types.h"
#include "system/util/fake_pthread.h"
#include "system/util/log.h"
#include "system/util/media_types.h"
#include "system/util/util.h"

#if DEF_DECODER_VIDEO_AUDIO_API_ENABLE
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#endif //DEF_DECODER_VIDEO_AUDIO_API_ENABLE

#if DEF_DECODER_IMAGE_API_ENABLE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif //DEF_DECODER_IMAGE_API_ENABLE

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
#if DEF_DECODER_VIDEO_AUDIO_API_ENABLE
extern void memcpy_asm(uint8_t*, uint8_t*, uint32_t);
static void Util_decoder_video_free(void *opaque, uint8_t *data);
static int Util_decoder_video_allocate_buffer(AVCodecContext *avctx, AVFrame *frame, int flags);
static void Util_decoder_audio_exit(uint8_t session);
static void Util_decoder_video_exit(uint8_t session);
static void Util_decoder_mvd_exit(uint8_t session);
static void Util_decoder_subtitle_exit(uint8_t session);
#endif //DEF_DECODER_VIDEO_AUDIO_API_ENABLE

//Variables.
#if DEF_DECODER_VIDEO_AUDIO_API_ENABLE
static bool util_audio_decoder_init[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static bool util_audio_decoder_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static bool util_audio_decoder_cache_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static uint8_t util_audio_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static AVPacket* util_audio_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static AVPacket* util_audio_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static AVFrame* util_audio_decoder_raw_data[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static AVCodecContext* util_audio_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };
static const AVCodec* util_audio_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS] = { 0, };

static bool util_video_decoder_init[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static bool util_video_decoder_frame_cores[4] = { 0, };
static bool util_video_decoder_slice_cores[4] = { 0, };
static bool util_video_decoder_changeable_buffer_size[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static bool util_video_decoder_cache_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static bool util_video_decoder_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static uint8_t util_video_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static uint16_t util_video_decoder_available_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static uint16_t util_video_decoder_raw_image_ready_index[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static uint16_t util_video_decoder_raw_image_current_index[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static uint16_t util_video_decoder_max_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static LightLock util_video_decoder_raw_image_mutex[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static AVPacket* util_video_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static AVPacket* util_video_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static AVFrame* util_video_decoder_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS][DEF_DECODER_MAX_RAW_IMAGE] = { 0, };
static AVCodecContext* util_video_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };
static const AVCodec* util_video_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, };

static bool util_mvd_video_decoder_init = false;
static bool util_mvd_video_decoder_changeable_buffer_size = false;
static bool util_mvd_video_decoder_first = false;
static bool util_mvd_video_decoder_should_skip_process_nal_unit = false;
static uint8_t util_mvd_video_decoder_current_cached_pts_index = 0;
static uint8_t util_mvd_video_decoder_next_cached_pts_index = 0;
static uint8_t* util_mvd_video_decoder_packet = NULL;
static uint16_t util_mvd_video_decoder_available_raw_image[DEF_DECODER_MAX_SESSIONS] = { 0, };
static uint16_t util_mvd_video_decoder_raw_image_ready_index[DEF_DECODER_MAX_SESSIONS] = { 0, };
static uint16_t util_mvd_video_decoder_raw_image_current_index[DEF_DECODER_MAX_SESSIONS] = { 0, };
static uint16_t util_mvd_video_decoder_max_raw_image[DEF_DECODER_MAX_SESSIONS] = { 0, };
static uint32_t util_mvd_video_decoder_packet_size = 0;
static int64_t util_mvd_video_decoder_cached_pts[32] = { 0, };
static LightLock util_mvd_video_decoder_raw_image_mutex[DEF_DECODER_MAX_SESSIONS] = { 0, };
static AVFrame* util_mvd_video_decoder_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_RAW_IMAGE] = { 0, };

static bool util_subtitle_decoder_init[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static bool util_subtitle_decoder_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static bool util_subtitle_decoder_cache_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static uint8_t util_subtitle_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static AVPacket* util_subtitle_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static AVPacket* util_subtitle_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static AVSubtitle* util_subtitle_decoder_raw_data[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static AVCodecContext* util_subtitle_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };
static const AVCodec* util_subtitle_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS] = { 0, };

static bool util_decoder_opened_file[DEF_DECODER_MAX_SESSIONS] = { 0, };
static uint16_t util_decoder_available_cache_packet[DEF_DECODER_MAX_SESSIONS] = { 0, };
static uint16_t util_decoder_cache_packet_ready_index[DEF_DECODER_MAX_SESSIONS] = { 0, };
static uint16_t util_decoder_cache_packet_current_index[DEF_DECODER_MAX_SESSIONS] = { 0, };
static LightLock util_decoder_cache_packet_mutex[DEF_DECODER_MAX_SESSIONS] = { 0, };
static MVDSTD_Config util_decoder_mvd_config = { .input_type = MVD_INPUT_H264, .output_type = MVD_OUTPUT_RGB565, 0, };
static AVPacket* util_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_CACHE_PACKETS] = { 0, };
static AVFormatContext* util_decoder_format_context[DEF_DECODER_MAX_SESSIONS] = { 0, };

//Translation table for AVPixelFormat -> Raw_pixel.
static Raw_pixel util_video_decoder_pixel_format_table[AV_PIX_FMT_NB] =
{
	RAW_PIXEL_YUV420P,
	RAW_PIXEL_YUYV422,
	RAW_PIXEL_RGB888,
	RAW_PIXEL_BGR888,
	RAW_PIXEL_YUV422P,
	RAW_PIXEL_YUV444P,
	RAW_PIXEL_YUV410P,
	RAW_PIXEL_YUV411P,
	RAW_PIXEL_GRAY8,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_YUVJ420P,
	RAW_PIXEL_YUVJ422P,
	RAW_PIXEL_YUVJ444P,
	RAW_PIXEL_UYVY422,
	RAW_PIXEL_UYYVYY411,
	RAW_PIXEL_BGR332,
	RAW_PIXEL_BGR121,
	RAW_PIXEL_BGR121_BYTE,
	RAW_PIXEL_RGB332,
	RAW_PIXEL_RGB121,
	RAW_PIXEL_RGB121_BYTE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_ARGB8888,
	RAW_PIXEL_RGBA8888,
	RAW_PIXEL_ABGR8888,
	RAW_PIXEL_BGRA8888,
	RAW_PIXEL_GRAY16BE,
	RAW_PIXEL_GRAY16LE,
	RAW_PIXEL_YUV440P,
	RAW_PIXEL_YUVJ440P,
	RAW_PIXEL_YUVA420P,
	RAW_PIXEL_RGB161616BE,
	RAW_PIXEL_RGB161616LE,
	RAW_PIXEL_RGB565BE,
	RAW_PIXEL_RGB565LE,
	RAW_PIXEL_RGB555BE,
	RAW_PIXEL_RGB555LE,
	RAW_PIXEL_BGR565BE,
	RAW_PIXEL_BGR565LE,
	RAW_PIXEL_BGR555BE,
	RAW_PIXEL_BGR555LE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_YUV420P16LE,
	RAW_PIXEL_YUV420P16BE,
	RAW_PIXEL_YUV422P16LE,
	RAW_PIXEL_YUV422P16BE,
	RAW_PIXEL_YUV444P16LE,
	RAW_PIXEL_YUV444P16BE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_RGB444LE,
	RAW_PIXEL_RGB444BE,
	RAW_PIXEL_BGR444LE,
	RAW_PIXEL_BGR444BE,
	RAW_PIXEL_GRAYALPHA88,
	RAW_PIXEL_BGR161616BE,
	RAW_PIXEL_BGR161616LE,
	RAW_PIXEL_YUV420P9BE,
	RAW_PIXEL_YUV420P9LE,
	RAW_PIXEL_YUV420P10BE,
	RAW_PIXEL_YUV420P10LE,
	RAW_PIXEL_YUV422P10BE,
	RAW_PIXEL_YUV422P10LE,
	RAW_PIXEL_YUV444P9BE,
	RAW_PIXEL_YUV444P9LE,
	RAW_PIXEL_YUV444P10BE,
	RAW_PIXEL_YUV444P10LE,
	RAW_PIXEL_YUV422P9BE,
	RAW_PIXEL_YUV422P9LE,
	RAW_PIXEL_GBR888P,
	RAW_PIXEL_GBR999PBE,
	RAW_PIXEL_GBR999PLE,
	RAW_PIXEL_GBR101010PBE,
	RAW_PIXEL_GBR101010PLE,
	RAW_PIXEL_GBR161616PBE,
	RAW_PIXEL_GBR161616PLE,
	RAW_PIXEL_YUVA422P,
	RAW_PIXEL_YUVA444P,
	RAW_PIXEL_YUVA420P9BE,
	RAW_PIXEL_YUVA420P9LE,
	RAW_PIXEL_YUVA422P9BE,
	RAW_PIXEL_YUVA422P9LE,
	RAW_PIXEL_YUVA444P9BE,
	RAW_PIXEL_YUVA444P9LE,
	RAW_PIXEL_YUVA420P10BE,
	RAW_PIXEL_YUVA420P10LE,
	RAW_PIXEL_YUVA422P10BE,
	RAW_PIXEL_YUVA422P10LE,
	RAW_PIXEL_YUVA444P10BE,
	RAW_PIXEL_YUVA444P10LE,
	RAW_PIXEL_YUVA420P16BE,
	RAW_PIXEL_YUVA420P16LE,
	RAW_PIXEL_YUVA422P16BE,
	RAW_PIXEL_YUVA422P16LE,
	RAW_PIXEL_YUVA444P16BE,
	RAW_PIXEL_YUVA444P16LE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_RGBA16161616BE,
	RAW_PIXEL_RGBA16161616LE,
	RAW_PIXEL_BGRA16161616BE,
	RAW_PIXEL_BGRA16161616LE,
	RAW_PIXEL_YVYU422,
	RAW_PIXEL_GRAYALPHA1616BE,
	RAW_PIXEL_GRAYALPHA1616LE,
	RAW_PIXEL_GBRA8888P,
	RAW_PIXEL_GBRA16161616PBE,
	RAW_PIXEL_GBRA16161616PLE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_YUV420P12BE,
	RAW_PIXEL_YUV420P12LE,
	RAW_PIXEL_YUV420P14BE,
	RAW_PIXEL_YUV420P14LE,
	RAW_PIXEL_YUV422P12BE,
	RAW_PIXEL_YUV422P12LE,
	RAW_PIXEL_YUV422P14BE,
	RAW_PIXEL_YUV422P14LE,
	RAW_PIXEL_YUV444P12BE,
	RAW_PIXEL_YUV444P12LE,
	RAW_PIXEL_YUV444P14BE,
	RAW_PIXEL_YUV444P14LE,
	RAW_PIXEL_GBR121212PBE,
	RAW_PIXEL_GBR121212PLE,
	RAW_PIXEL_GBR141414PBE,
	RAW_PIXEL_GBR141414PLE,
	RAW_PIXEL_YUVJ411P,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_YUV440P10LE,
	RAW_PIXEL_YUV440P10BE,
	RAW_PIXEL_YUV440P12LE,
	RAW_PIXEL_YUV440P12BE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_GBRA12121212PBE,
	RAW_PIXEL_GBRA12121212PLE,
	RAW_PIXEL_GBRA10101010PBE,
	RAW_PIXEL_GBRA10101010PLE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_GRAY12BE,
	RAW_PIXEL_GRAY12LE,
	RAW_PIXEL_GRAY10BE,
	RAW_PIXEL_GRAY10LE,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
	RAW_PIXEL_INVALID,
};

//Translation table for AVSampleFormat -> Raw_sample.
static Raw_sample util_audio_decoder_sample_format_table[AV_SAMPLE_FMT_NB] =
{
	RAW_SAMPLE_U8,
	RAW_SAMPLE_S16,
	RAW_SAMPLE_S32,
	RAW_SAMPLE_FLOAT32,
	RAW_SAMPLE_DOUBLE64,
	RAW_SAMPLE_U8P,
	RAW_SAMPLE_S16P,
	RAW_SAMPLE_S32P,
	RAW_SAMPLE_FLOAT32P,
	RAW_SAMPLE_DOUBLE64P,
	RAW_SAMPLE_S64,
	RAW_SAMPLE_S64P,
};

static uint8_t util_audio_decoder_sample_format_size_table[] =
{
	sizeof(uint8_t),
	sizeof(int16_t),
	sizeof(int32_t),
	sizeof(float),
	sizeof(double),
	sizeof(uint8_t),
	sizeof(int16_t),
	sizeof(int32_t),
	sizeof(float),
	sizeof(double),
	sizeof(int64_t),
	sizeof(int64_t),
};
#endif //DEF_DECODER_VIDEO_AUDIO_API_ENABLE

//Code.
#if DEF_DECODER_VIDEO_AUDIO_API_ENABLE
//We can't get rid of this "int" because library uses "int" type as args.
// static void Util_decoder_video_log_callback(void *avcl, int level, const char *fmt, va_list list)
// {
// 	if(level > AV_LOG_TRACE)
// 		return;

// 	DEF_LOG_VFORMAT(fmt, list);
// }

static void Util_decoder_video_free(void *opaque, uint8_t *data)
{
	(void)opaque;
	free(data);
}

//We can't get rid of this "int" because library uses "int" type as args.
static int Util_decoder_video_allocate_buffer(AVCodecContext *avctx, AVFrame *frame, int flags)
{
	(void)flags;

	if(avctx->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		uint32_t width = 0;
		uint32_t height = 0;
		int32_t buffer_size = 0;
		uint8_t* buffer = NULL;

		for (uint8_t i = 0; i < AV_NUM_DATA_POINTERS; i++)
		{
			frame->data[i] = NULL;
			frame->linesize[i] = 0;
			frame->buf[i] = NULL;
		}
		width = frame->width;
		height = frame->height;

		if(avctx->codec_id == AV_CODEC_ID_AV1)
		{
			if(width % 128 != 0)
				width += 128 - width % 128;
			if(height % 128 != 0)
				height += 128 - height % 128;
		}
		else
		{
			if(width % 16 != 0)
				width += 16 - width % 16;
			if(height % 16 != 0)
				height += 16 - height % 16;
		}

		buffer_size = av_image_get_buffer_size(avctx->pix_fmt, width, height, 1);
		if(buffer_size <= 0)
			return -1;

		buffer = (uint8_t*)linearAlloc(buffer_size);
		if(!buffer)
			return -1;

		if(av_image_fill_arrays(frame->data, frame->linesize, buffer, avctx->pix_fmt, width, height, 1) <= 0)
		{
			free(buffer);
			buffer = NULL;
			return -1;
		}

		for(uint8_t i = 0; i < AV_NUM_DATA_POINTERS; i++)
		{
			if(frame->data[i])
			{
				frame->buf[i] = av_buffer_create(frame->data[i], 0, Util_decoder_video_free, NULL, 0);
				if(!frame->buf[i])
				{
					for(uint8_t k = 0; k < AV_NUM_DATA_POINTERS; k++)
						frame->buf[k] = NULL;

					free(buffer);
					buffer = NULL;
					return -1;
				}
			}
		}

		return 0;
	}
	else
		return -1;
}

uint32_t Util_decoder_open_file(const char* path, uint8_t* num_of_audio_tracks, uint8_t* num_of_video_tracks, uint8_t* num_of_subtitle_tracks, uint8_t session)
{
	int32_t ffmpeg_result = 0;
	uint8_t audio_index = 0;
	uint8_t video_index = 0;
	uint8_t subtitle_index = 0;

	if(!path || !num_of_audio_tracks || !num_of_video_tracks || !num_of_subtitle_tracks || session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_decoder_opened_file[session])
		goto already_inited;

	*num_of_video_tracks = 0;
	*num_of_audio_tracks = 0;
	*num_of_subtitle_tracks = 0;
	util_decoder_cache_packet_ready_index[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_available_cache_packet[session] = 0;
	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		util_audio_decoder_stream_num[session][i] = UINT8_MAX;

	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
		util_video_decoder_stream_num[session][i] = UINT8_MAX;

	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		util_subtitle_decoder_stream_num[session][i] = UINT8_MAX;

	util_decoder_format_context[session] = avformat_alloc_context();
	if(!util_decoder_format_context[session])
	{
		DEF_LOG_RESULT(avformat_alloc_context, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_open_input(&util_decoder_format_context[session], path, NULL, NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avformat_open_input, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_find_stream_info(util_decoder_format_context[session], NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avformat_find_stream_info, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	LightLock_Init(&util_decoder_cache_packet_mutex[session]);

	for(uint8_t i = 0; i < util_decoder_format_context[session]->nb_streams; i++)
	{
		if(util_decoder_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_index < DEF_DECODER_MAX_AUDIO_TRACKS)
		{
			util_audio_decoder_stream_num[session][audio_index] = i;
			audio_index++;
		}
		else if(util_decoder_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_index < DEF_DECODER_MAX_VIDEO_TRACKS)
		{
			util_video_decoder_stream_num[session][video_index] = i;
			video_index++;
		}
		else if(util_decoder_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE && subtitle_index < DEF_DECODER_MAX_SUBTITLE_TRACKS)
		{
			util_subtitle_decoder_stream_num[session][subtitle_index] = i;
			subtitle_index++;
		}
	}

	if(audio_index == 0 && video_index == 0 && subtitle_index == 0)
	{
		DEF_LOG_STRING("No audio and video were found.");
		goto other;
	}

	*num_of_audio_tracks = audio_index;
	*num_of_video_tracks = video_index;
	*num_of_subtitle_tracks = subtitle_index;
	util_decoder_opened_file[session] = true;

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	avformat_free_context(util_decoder_format_context[session]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;

	other:
	avformat_free_context(util_decoder_format_context[session]);
	return DEF_ERR_OTHER;
}

uint32_t Util_decoder_audio_init(uint8_t num_of_audio_tracks, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(num_of_audio_tracks == 0 || num_of_audio_tracks > DEF_DECODER_MAX_AUDIO_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;

	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		if(util_audio_decoder_init[session][i])
			goto already_inited;
	}

	for(uint8_t i = 0; i < num_of_audio_tracks; i++)
	{
		if(util_audio_decoder_stream_num[session][i] == UINT8_MAX)
			goto invalid_arg;
	}

	for(uint8_t i = 0; i < num_of_audio_tracks; i++)
	{
		util_audio_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_audio_decoder_codec[session][i])
		{
			DEF_LOG_RESULT(avcodec_find_decoder, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
			goto ffmpeg_api_failed;
		}

		util_audio_decoder_context[session][i] = avcodec_alloc_context3(util_audio_decoder_codec[session][i]);
		if(!util_audio_decoder_context[session][i])
		{
			DEF_LOG_RESULT(avcodec_alloc_context3, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_audio_decoder_context[session][i], util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avcodec_parameters_to_context, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}

		util_audio_decoder_context[session][i]->flags = AV_CODEC_FLAG_OUTPUT_CORRUPT;
		ffmpeg_result = avcodec_open2(util_audio_decoder_context[session][i], util_audio_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avcodec_open2, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}

		util_audio_decoder_init[session][i] = true;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		util_audio_decoder_init[session][i] = false;
		avcodec_free_context(&util_audio_decoder_context[session][i]);
	}
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

void Util_decoder_video_set_enabled_cores(const bool frame_threading_cores[4], const bool slice_threading_cores[4])
{
	if(!frame_threading_cores[0] && !frame_threading_cores[1] && !frame_threading_cores[2] && !frame_threading_cores[3]
	&& !slice_threading_cores[0] && !slice_threading_cores[1] && !slice_threading_cores[2] && !slice_threading_cores[3])
		return;

	for(uint8_t i = 0; i < 4; i++)
	{
		util_video_decoder_frame_cores[i] = frame_threading_cores[i];
		util_video_decoder_slice_cores[i] = slice_threading_cores[i];
	}
}

uint32_t Util_decoder_video_init(uint8_t low_resolution, uint8_t num_of_video_tracks, uint8_t num_of_threads, Media_thread_type thread_type, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(num_of_video_tracks == 0 || thread_type <= MEDIA_THREAD_TYPE_INVALID || thread_type >= MEDIA_THREAD_TYPE_MAX
	|| num_of_video_tracks > DEF_DECODER_MAX_VIDEO_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;

	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		if(util_video_decoder_init[session][i])
			goto already_inited;
	}

	for(uint8_t i = 0; i < num_of_video_tracks; i++)
	{
		if(util_video_decoder_stream_num[session][i] == UINT8_MAX)
			goto invalid_arg;
	}

	for (uint8_t i = 0; i < num_of_video_tracks; i++)
	{
		util_video_decoder_raw_image_current_index[session][i] = 0;
		util_video_decoder_raw_image_ready_index[session][i] = 0;
		util_video_decoder_available_raw_image[session][i] = 0;
		util_video_decoder_max_raw_image[session][i] = 0;
	}

	for(uint8_t i = 0; i < num_of_video_tracks; i++)
	{
		util_video_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_video_decoder_codec[session][i])
		{
			DEF_LOG_RESULT(avcodec_find_decoder, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
			goto ffmpeg_api_failed;
		}

		util_video_decoder_context[session][i] = avcodec_alloc_context3(util_video_decoder_codec[session][i]);
		if(!util_video_decoder_context[session][i])
		{
			DEF_LOG_RESULT(avcodec_alloc_context3, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_video_decoder_context[session][i], util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avcodec_parameters_to_context, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}

		if(util_video_decoder_codec[session][i]->max_lowres < low_resolution)
			util_video_decoder_context[session][i]->lowres = util_video_decoder_codec[session][i]->max_lowres;
		else
			util_video_decoder_context[session][i]->lowres = low_resolution;

		util_video_decoder_context[session][i]->flags = AV_CODEC_FLAG_OUTPUT_CORRUPT;

		//If there is more than 1 video tracks, limit number of threads to avoid poor performance due to too many threads.
		if(num_of_video_tracks >= 2)
			util_video_decoder_context[session][i]->thread_count = (num_of_threads > 4 ? 4 : num_of_threads);
		else
			util_video_decoder_context[session][i]->thread_count = num_of_threads;

		if(thread_type == MEDIA_THREAD_TYPE_AUTO)
		{
			if(util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_FRAME_THREADS)
				util_video_decoder_context[session][i]->thread_type = FF_THREAD_FRAME;
			else if(util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_SLICE_THREADS)
				util_video_decoder_context[session][i]->thread_type = FF_THREAD_SLICE;
			else
				util_video_decoder_context[session][i]->thread_type = FF_THREAD_FRAME;
		}
		else if(thread_type == MEDIA_THREAD_TYPE_SLICE && util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_SLICE_THREADS)
			util_video_decoder_context[session][i]->thread_type = FF_THREAD_SLICE;
		else if(thread_type == MEDIA_THREAD_TYPE_FRAME && util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_FRAME_THREADS)
			util_video_decoder_context[session][i]->thread_type = FF_THREAD_FRAME;
		else
		{
			util_video_decoder_context[session][i]->thread_type = 0;
			util_video_decoder_context[session][i]->thread_count = 1;
		}

		if(util_video_decoder_context[session][i]->thread_type == FF_THREAD_FRAME)
			Util_fake_pthread_set_enabled_core(util_video_decoder_frame_cores);
		else if(util_video_decoder_context[session][i]->thread_type == FF_THREAD_SLICE)
			Util_fake_pthread_set_enabled_core(util_video_decoder_slice_cores);

		util_video_decoder_context[session][i]->get_buffer2 = Util_decoder_video_allocate_buffer;

		ffmpeg_result = avcodec_open2(util_video_decoder_context[session][i], util_video_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avcodec_open2, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}

		LightLock_Init(&util_video_decoder_raw_image_mutex[session][i]);

		util_video_decoder_max_raw_image[session][i] = DEF_DECODER_MAX_RAW_IMAGE;
		util_video_decoder_changeable_buffer_size[session][i] = true;
		util_video_decoder_init[session][i] = true;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		util_video_decoder_init[session][i] = false;
		avcodec_free_context(&util_video_decoder_context[session][i]);
	}

	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_mvd_init(uint8_t session)
{
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t size = 0;
	uint32_t result = DEF_ERR_OTHER;
	MVDSTD_CalculateWorkBufSizeConfig config = { 0, };

	if(session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0])
		goto not_inited;

	if(util_mvd_video_decoder_init)
		goto already_inited;

	width = util_video_decoder_context[session][0]->width;
	height = util_video_decoder_context[session][0]->height;

	for(uint8_t i = 0; i < 32; i++)
		util_mvd_video_decoder_cached_pts[i] = 0;

	util_mvd_video_decoder_current_cached_pts_index = 0;
	util_mvd_video_decoder_next_cached_pts_index = 0;

	util_mvd_video_decoder_raw_image_current_index[session] = 0;
	util_mvd_video_decoder_raw_image_ready_index[session] = 0;
	util_mvd_video_decoder_available_raw_image[session] = 0;
	util_mvd_video_decoder_should_skip_process_nal_unit = false;

	util_mvd_video_decoder_packet_size = (1000 * 256);
	util_mvd_video_decoder_packet = (uint8_t*)linearAlloc(util_mvd_video_decoder_packet_size);
	if(!util_mvd_video_decoder_packet)
		goto out_of_linear_memory;

	config.level.enable = true;
	config.level.flag = (MVD_CALC_WITH_LEVEL_FLAG_ENABLE_CALC | MVD_CALC_WITH_LEVEL_FLAG_ENABLE_EXTRA_OP | MVD_CALC_WITH_LEVEL_FLAG_UNK);
	config.level.level = 0xFF;
	config.width = width;
	config.height = height;

	switch(util_video_decoder_context[session][0]->level)
	{
		case 10: config.level.level = MVD_H264_LEVEL_1_0; break; //Level 1.0.
		case 9: config.level.level = MVD_H264_LEVEL_1_0B; break; //Level 1.0b.
		case 11: config.level.level = MVD_H264_LEVEL_1_1; break; //Level 1.1.
		case 12: config.level.level = MVD_H264_LEVEL_1_2; break; //Level 1.2.
		case 13: config.level.level = MVD_H264_LEVEL_1_3; break; //Level 1.3.
		case 20: config.level.level = MVD_H264_LEVEL_2_0; break; //Level 2.0.
		case 21: config.level.level = MVD_H264_LEVEL_2_1; break; //Level 2.1.
		case 22: config.level.level = MVD_H264_LEVEL_2_2; break; //Level 2.2.
		case 30: config.level.level = MVD_H264_LEVEL_3_0; break; //Level 3.0.
		case 31: config.level.level = MVD_H264_LEVEL_3_1; break; //Level 3.1.
		case 32: config.level.level = MVD_H264_LEVEL_3_2; break; //Level 3.2.
		case 40: config.level.level = MVD_H264_LEVEL_4_0; break; //Level 4.0.
		case 41: config.level.level = MVD_H264_LEVEL_4_1; break; //Level 4.1.
		case 42: config.level.level = MVD_H264_LEVEL_4_2; break; //Level 4.2.
		case 50: config.level.level = MVD_H264_LEVEL_5_0; break; //Level 5.0.
		case 51: config.level.level = MVD_H264_LEVEL_5_1; break; //Level 5.1.
		case 52: config.level.level = MVD_H264_LEVEL_5_2; break; //Level 5.2.

		case 60: DEF_LOG_STRING("Level 6.0 is NOT supported!!!!!"); break;
		case 61: DEF_LOG_STRING("Level 6.1 is NOT supported!!!!!"); break;
		case 62: DEF_LOG_STRING("Level 6.2 is NOT supported!!!!!"); break;

		default:
		{
			DEF_LOG_STRING("Unknown level!!!!!");
			DEF_LOG_INT(util_video_decoder_context[session][0]->level);
			break;
		}
	}

	if(config.level.level == 0xFF)
		goto unsupported_levels;

	DEF_LOG_FORMAT("%" PRIu8 "(%" PRIi32 ") %" PRIu32 "x%" PRIu32, config.level.level, util_video_decoder_context[session][0]->level, config.width, config.height);
	result = mvdstdCalculateBufferSize(&config, &size);

	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(mvdstdCalculateBufferSize, false, result);
		goto nintendo_api_failed;
	}

	DEF_LOG_FORMAT("%fMB (%" PRIu32 "B)", (size / 1000.0 / 1000.0), size);
	result = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264, MVD_OUTPUT_BGR565, size, NULL);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(mvdstdInit, false, result);
		goto nintendo_api_failed_1;
	}

	LightLock_Init(&util_mvd_video_decoder_raw_image_mutex[session]);

	util_mvd_video_decoder_max_raw_image[session] = DEF_DECODER_MAX_RAW_IMAGE;
	util_mvd_video_decoder_first = true;
	util_mvd_video_decoder_changeable_buffer_size = true;
	util_mvd_video_decoder_init = true;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	out_of_linear_memory:
	return DEF_ERR_OUT_OF_LINEAR_MEMORY;

	unsupported_levels:
	free(util_mvd_video_decoder_packet);
	util_mvd_video_decoder_packet = NULL;
	return DEF_ERR_OTHER;

	nintendo_api_failed:
	free(util_mvd_video_decoder_packet);
	util_mvd_video_decoder_packet = NULL;
	return result;

	nintendo_api_failed_1:
	free(util_mvd_video_decoder_packet);
	util_mvd_video_decoder_packet = NULL;
	mvdstdExit();
	return result;
}

uint32_t Util_decoder_subtitle_init(uint8_t num_of_subtitle_tracks, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(num_of_subtitle_tracks == 0 || num_of_subtitle_tracks > DEF_DECODER_MAX_SUBTITLE_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;

	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
	{
		if(util_subtitle_decoder_init[session][i])
			goto already_inited;
	}

	for(uint8_t i = 0; i < num_of_subtitle_tracks; i++)
	{
		if(util_subtitle_decoder_stream_num[session][i] == UINT8_MAX)
			goto invalid_arg;
	}

	for(uint8_t i = 0; i < num_of_subtitle_tracks; i++)
	{
		util_subtitle_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_subtitle_decoder_codec[session][i])
		{
			DEF_LOG_RESULT(avcodec_find_decoder, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
			goto ffmpeg_api_failed;
		}

		util_subtitle_decoder_context[session][i] = avcodec_alloc_context3(util_subtitle_decoder_codec[session][i]);
		if(!util_subtitle_decoder_context[session][i])
		{
			DEF_LOG_RESULT(avcodec_alloc_context3, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_subtitle_decoder_context[session][i], util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avcodec_parameters_to_context, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_open2(util_subtitle_decoder_context[session][i], util_subtitle_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avcodec_open2, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}

		util_subtitle_decoder_init[session][i] = true;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
	{
		util_subtitle_decoder_init[session][i] = false;
		avcodec_free_context(&util_subtitle_decoder_context[session][i]);
	}
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

void Util_decoder_audio_get_info(Media_a_info* audio_info, uint8_t audio_index, uint8_t session)
{
	uint32_t size = 0;
	const char lang_und[] = "und";
	AVDictionaryEntry *data = NULL;

	if(!audio_info || audio_index >= DEF_DECODER_MAX_AUDIO_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_audio_decoder_init[session][audio_index])
		return;

	audio_info->bitrate = util_audio_decoder_context[session][audio_index]->bit_rate;
	audio_info->sample_rate = util_audio_decoder_context[session][audio_index]->sample_rate;
	audio_info->ch = util_audio_decoder_context[session][audio_index]->ch_layout.nb_channels;
	audio_info->duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
	if(util_audio_decoder_context[session][audio_index]->sample_fmt < 0 || util_audio_decoder_context[session][audio_index]->sample_fmt >= AV_SAMPLE_FMT_NB)
		audio_info->sample_format = RAW_SAMPLE_INVALID;
	else
		audio_info->sample_format = util_audio_decoder_sample_format_table[util_audio_decoder_context[session][audio_index]->sample_fmt];

	if(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][audio_index]]->metadata)
		data = av_dict_get(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][audio_index]]->metadata, "language", data, AV_DICT_IGNORE_SUFFIX);

	size = (data ? strlen(data->value) : strlen(lang_und));
	size = Util_min(size, (sizeof(audio_info->track_lang) - 1));
	if(data)
		memcpy(audio_info->track_lang, data->value, size);
	else
		memcpy(audio_info->track_lang, lang_und, size);

	audio_info->track_lang[size] = 0x00;

	size = (util_audio_decoder_codec[session][audio_index]->long_name ? strlen(util_audio_decoder_codec[session][audio_index]->long_name) : 0);
	if(size > 0)
	{
		size = Util_min(size, (sizeof(audio_info->format_name) - 1));
		memcpy(audio_info->format_name, util_audio_decoder_codec[session][audio_index]->long_name, size);
	}
	audio_info->format_name[size] = 0x00;

	size = (util_audio_decoder_codec[session][audio_index]->name ? strlen(util_audio_decoder_codec[session][audio_index]->name) : 0);
	if(size > 0)
	{
		size = Util_min(size, (sizeof(audio_info->short_format_name) - 1));
		memcpy(audio_info->short_format_name, util_audio_decoder_codec[session][audio_index]->name, size);
	}
	audio_info->short_format_name[size] = 0x00;
}

void Util_decoder_video_get_info(Media_v_info* video_info, uint8_t video_index, uint8_t session)
{
	uint16_t multiple_of = 0;
	uint32_t size = 0;
	AVRational sar = { 0, };

	if(!video_info || video_index >= DEF_DECODER_MAX_VIDEO_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][video_index])
		return;

	sar = av_guess_sample_aspect_ratio(util_decoder_format_context[session], util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]], NULL);
	if(sar.num == 0)
	{
		video_info->sar_width = 1;
		video_info->sar_height = 1;
	}
	else
	{
		video_info->sar_width = 1;
		video_info->sar_height = (double)sar.den / sar.num;
	}

	video_info->width = util_video_decoder_context[session][video_index]->width;
	video_info->height = util_video_decoder_context[session][video_index]->height;

	if(util_video_decoder_context[session][video_index]->codec_id == AV_CODEC_ID_AV1)
		multiple_of = 128;//AV1 buffer size is multiple of 128.
	else
		multiple_of = 16;//Other codecs are multiple of 16.

	if(video_info->width % multiple_of != 0)
		video_info->codec_width = video_info->width + (multiple_of - (video_info->width % multiple_of));
	else
		video_info->codec_width = video_info->width;

	if(video_info->height % multiple_of != 0)
		video_info->codec_height = video_info->height + (multiple_of - (video_info->height % multiple_of));
	else
		video_info->codec_height = video_info->height;

	if(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.den == 0)
		video_info->framerate = 0;
	else
		video_info->framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.den;

	video_info->duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
	if(util_video_decoder_context[session][video_index]->thread_type == FF_THREAD_FRAME)
		video_info->thread_type = MEDIA_THREAD_TYPE_FRAME;
	else if(util_video_decoder_context[session][video_index]->thread_type == FF_THREAD_SLICE)
		video_info->thread_type = MEDIA_THREAD_TYPE_SLICE;
	else
		video_info->thread_type = MEDIA_THREAD_TYPE_NONE;

	if(util_video_decoder_context[session][video_index]->pix_fmt < 0 || util_video_decoder_context[session][video_index]->pix_fmt >= AV_PIX_FMT_NB)
		video_info->pixel_format = RAW_PIXEL_INVALID;
	else
		video_info->pixel_format = util_video_decoder_pixel_format_table[util_video_decoder_context[session][video_index]->pix_fmt];

	size = (util_video_decoder_codec[session][video_index]->long_name ? strlen(util_video_decoder_codec[session][video_index]->long_name) : 0);
	if(size > 0)
	{
		size = Util_min(size, (sizeof(video_info->format_name) - 1));
		memcpy(video_info->format_name, util_video_decoder_codec[session][video_index]->long_name, size);
	}
	video_info->format_name[size] = 0x00;

	size = (util_video_decoder_codec[session][video_index]->name ? strlen(util_video_decoder_codec[session][video_index]->name) : 0);
	if(size > 0)
	{
		size = Util_min(size, (sizeof(video_info->short_format_name) - 1));
		memcpy(video_info->short_format_name, util_video_decoder_codec[session][video_index]->name, size);
	}
	video_info->short_format_name[size] = 0x00;
}

void Util_decoder_subtitle_get_info(Media_s_info* subtitle_info, uint8_t subtitle_index, uint8_t session)
{
	uint32_t size = 0;
	const char lang_und[] = "und";
	AVDictionaryEntry *data = NULL;

	if(!subtitle_info || subtitle_index >= DEF_DECODER_MAX_SUBTITLE_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_subtitle_decoder_init[session][subtitle_index])
		return;

	if(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][subtitle_index]]->metadata)
		data = av_dict_get(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][subtitle_index]]->metadata, "language", data, AV_DICT_IGNORE_SUFFIX);

	size = (data ? strlen(data->value) : strlen(lang_und));
	size = Util_min(size, (sizeof(subtitle_info->track_lang) - 1));
	if(data)
		memcpy(subtitle_info->track_lang, data->value, size);
	else
		memcpy(subtitle_info->track_lang, lang_und, size);

	subtitle_info->track_lang[size] = 0x00;

	size = (util_subtitle_decoder_codec[session][subtitle_index]->long_name ? strlen(util_subtitle_decoder_codec[session][subtitle_index]->long_name) : 0);
	if(size > 0)
	{
		size = Util_min(size, (sizeof(subtitle_info->format_name) - 1));
		memcpy(subtitle_info->format_name, util_subtitle_decoder_codec[session][subtitle_index]->long_name, size);
	}
	subtitle_info->format_name[size] = 0x00;

	size = (util_subtitle_decoder_codec[session][subtitle_index]->name ? strlen(util_subtitle_decoder_codec[session][subtitle_index]->name) : 0);
	if(size > 0)
	{
		size = Util_min(size, (sizeof(subtitle_info->short_format_name) - 1));
		memcpy(subtitle_info->short_format_name, util_subtitle_decoder_codec[session][subtitle_index]->name, size);
	}
	subtitle_info->short_format_name[size] = 0x00;
}

void Util_decoder_clear_cache_packet(uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session])
		return;

	for(uint16_t i = 0; i < DEF_DECODER_MAX_CACHE_PACKETS; i++)
		av_packet_free(&util_decoder_cache_packet[session][i]);

	util_decoder_available_cache_packet[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_cache_packet_ready_index[session] = 0;
}

uint16_t Util_decoder_get_available_packet_num(uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS)
		return 0;

	if(!util_decoder_opened_file[session])
		return 0;
	else
		return util_decoder_available_cache_packet[session];
}

uint32_t Util_decoder_read_packet(uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;

	LightLock_Lock(&util_decoder_cache_packet_mutex[session]);
	if(util_decoder_available_cache_packet[session] + 1 >= DEF_DECODER_MAX_CACHE_PACKETS)
	{
		//DEF_LOG_STRING("Queues are full!!!!!");
		goto try_again;
	}
	LightLock_Unlock(&util_decoder_cache_packet_mutex[session]);

	util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]] = av_packet_alloc();
	if(!util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]])
	{
		DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_read_frame(util_decoder_format_context[session], util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(av_read_frame, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	if(util_decoder_cache_packet_ready_index[session] + 1 < DEF_DECODER_MAX_CACHE_PACKETS)
		util_decoder_cache_packet_ready_index[session]++;
	else
		util_decoder_cache_packet_ready_index[session] = 0;

	LightLock_Lock(&util_decoder_cache_packet_mutex[session]);
	util_decoder_available_cache_packet[session]++;
	LightLock_Unlock(&util_decoder_cache_packet_mutex[session]);

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	LightLock_Unlock(&util_decoder_cache_packet_mutex[session]);
	return DEF_ERR_TRY_AGAIN;

	ffmpeg_api_failed:
	av_packet_free(&util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_parse_packet(Media_packet_type* type, uint8_t* packet_index, bool* key_frame, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS || !type || !packet_index || !key_frame)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;

	*key_frame = false;
	*packet_index = UINT8_MAX;
	*type = MEDIA_PACKET_TYPE_UNKNOWN;

	LightLock_Lock(&util_decoder_cache_packet_mutex[session]);
	if(util_decoder_available_cache_packet[session] == 0)
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}
	LightLock_Unlock(&util_decoder_cache_packet_mutex[session]);

	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		if(util_audio_decoder_stream_num[session][i] == UINT8_MAX)
			continue;

		if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_audio_decoder_stream_num[session][i])//audio packet
		{
			util_audio_decoder_cache_packet[session][i] = av_packet_alloc();
			if(!util_audio_decoder_cache_packet[session][i])
			{
				DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
				goto ffmpeg_api_failed;
			}

			av_packet_unref(util_audio_decoder_cache_packet[session][i]);
			ffmpeg_result = av_packet_ref(util_audio_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
			if(ffmpeg_result != 0)
			{
				av_packet_free(&util_audio_decoder_cache_packet[session][i]);
				DEF_LOG_RESULT(av_packet_ref, false, ffmpeg_result);
				goto ffmpeg_api_failed;
			}
			*packet_index = i;
			*type = MEDIA_PACKET_TYPE_AUDIO;
			util_audio_decoder_cache_packet_ready[session][i] = true;
			break;
		}
	}

	if(*type == MEDIA_PACKET_TYPE_UNKNOWN)
	{
		for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
		{
			if(util_video_decoder_stream_num[session][i] == UINT8_MAX)
				continue;

			if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_video_decoder_stream_num[session][i])//video packet
			{
				util_video_decoder_cache_packet[session][i] = av_packet_alloc();
				if(!util_video_decoder_cache_packet[session][i])
				{
					DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
					goto ffmpeg_api_failed;
				}

				av_packet_unref(util_video_decoder_cache_packet[session][i]);
				ffmpeg_result = av_packet_ref(util_video_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
				if(ffmpeg_result != 0)
				{
					av_packet_free(&util_video_decoder_cache_packet[session][i]);
					DEF_LOG_RESULT(av_packet_ref, false, ffmpeg_result);
					goto ffmpeg_api_failed;
				}
				*packet_index = i;
				*type = MEDIA_PACKET_TYPE_VIDEO;
				*key_frame = util_video_decoder_cache_packet[session][i]->flags & AV_PKT_FLAG_KEY;
				util_video_decoder_cache_packet_ready[session][i] = true;
				break;
			}
		}
	}

	if(*type == MEDIA_PACKET_TYPE_UNKNOWN)
	{
		for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		{
			if(util_subtitle_decoder_stream_num[session][i] == UINT8_MAX)
				continue;

			if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_subtitle_decoder_stream_num[session][i])//subtitle packet
			{
				util_subtitle_decoder_cache_packet[session][i] = av_packet_alloc();
				if(!util_subtitle_decoder_cache_packet[session][i])
				{
					DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
					goto ffmpeg_api_failed;
				}

				av_packet_unref(util_subtitle_decoder_cache_packet[session][i]);
				ffmpeg_result = av_packet_ref(util_subtitle_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
				if(ffmpeg_result != 0)
				{
					av_packet_free(&util_subtitle_decoder_cache_packet[session][i]);
					DEF_LOG_RESULT(av_packet_ref, false, ffmpeg_result);
					goto ffmpeg_api_failed;
				}
				*packet_index = i;
				*type = MEDIA_PACKET_TYPE_SUBTITLE;
				util_subtitle_decoder_cache_packet_ready[session][i] = true;
				break;
			}
		}
	}

	av_packet_free(&util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
	if(util_decoder_cache_packet_current_index[session] + 1 < DEF_DECODER_MAX_CACHE_PACKETS)
		util_decoder_cache_packet_current_index[session]++;
	else
		util_decoder_cache_packet_current_index[session] = 0;

	LightLock_Lock(&util_decoder_cache_packet_mutex[session]);
	util_decoder_available_cache_packet[session]--;
	LightLock_Unlock(&util_decoder_cache_packet_mutex[session]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	LightLock_Unlock(&util_decoder_cache_packet_mutex[session]);
	return DEF_ERR_TRY_AGAIN;

	ffmpeg_api_failed:
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_ready_audio_packet(uint8_t packet_index, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_AUDIO_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_audio_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_audio_decoder_cache_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	if(util_audio_decoder_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("Queues are full!!!!!");
		goto try_again;
	}

	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	util_audio_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_audio_decoder_packet[session][packet_index])
	{
		DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	av_packet_unref(util_audio_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_audio_decoder_packet[session][packet_index], util_audio_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(av_packet_ref, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_audio_decoder_cache_packet_ready[session][packet_index] = false;
	util_audio_decoder_packet_ready[session][packet_index] = true;
	av_packet_free(&util_audio_decoder_cache_packet[session][packet_index]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	ffmpeg_api_failed:
	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_ready_video_packet(int8_t packet_index, int8_t session)
{
	int32_t ffmpeg_result = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_video_decoder_cache_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	if(util_video_decoder_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("Queues are full!!!!!");
		goto try_again;
	}

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	util_video_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_video_decoder_packet[session][packet_index])
	{
		DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	av_packet_unref(util_video_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_video_decoder_packet[session][packet_index], util_video_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(av_packet_ref, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_video_decoder_cache_packet_ready[session][packet_index] = false;
	util_video_decoder_packet_ready[session][packet_index] = true;
	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	ffmpeg_api_failed:
	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_ready_subtitle_packet(uint8_t packet_index, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_SUBTITLE_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_subtitle_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_subtitle_decoder_cache_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	if(util_subtitle_decoder_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("Queues are full!!!!!");
		goto try_again;
	}

	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	util_subtitle_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_subtitle_decoder_packet[session][packet_index])
	{
		DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	av_packet_unref(util_subtitle_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_subtitle_decoder_packet[session][packet_index], util_subtitle_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(av_packet_ref, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_subtitle_decoder_cache_packet_ready[session][packet_index] = false;
	util_subtitle_decoder_packet_ready[session][packet_index] = true;
	av_packet_free(&util_subtitle_decoder_cache_packet[session][packet_index]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	ffmpeg_api_failed:
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

void Util_decoder_skip_audio_packet(uint8_t packet_index, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_AUDIO_TRACKS)
		return;

	if(!util_decoder_opened_file[session])
		return;

	if(!util_audio_decoder_cache_packet_ready[session][packet_index])
		return;

	av_packet_free(&util_audio_decoder_cache_packet[session][packet_index]);
	util_audio_decoder_cache_packet_ready[session][packet_index] = false;
}

void Util_decoder_skip_video_packet(uint8_t packet_index, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS)
		return;

	if(!util_decoder_opened_file[session])
		return;

	if(!util_video_decoder_cache_packet_ready[session][packet_index])
		return;

	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
	util_video_decoder_cache_packet_ready[session][packet_index] = false;
}

void Util_decoder_skip_subtitle_packet(uint8_t packet_index, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_SUBTITLE_TRACKS)
		return;

	if(!util_decoder_opened_file[session])
		return;

	if(!util_subtitle_decoder_cache_packet_ready[session][packet_index])
		return;

	av_packet_free(&util_subtitle_decoder_cache_packet[session][packet_index]);
	util_subtitle_decoder_cache_packet_ready[session][packet_index] = false;
}

void Util_decoder_video_set_raw_image_buffer_size(uint32_t max_num_of_buffer, uint8_t packet_index, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS
	|| max_num_of_buffer < 3 || max_num_of_buffer > DEF_DECODER_MAX_RAW_IMAGE)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index]
	|| !util_video_decoder_changeable_buffer_size[session][packet_index])
		return;

	util_video_decoder_max_raw_image[session][packet_index] = max_num_of_buffer;
}

void Util_decoder_mvd_set_raw_image_buffer_size(uint32_t max_num_of_buffer, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || max_num_of_buffer < 3 || max_num_of_buffer > DEF_DECODER_MAX_RAW_IMAGE)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init
	|| !util_mvd_video_decoder_changeable_buffer_size)
		return;

	util_mvd_video_decoder_max_raw_image[session] = max_num_of_buffer;
}

uint32_t Util_decoder_video_get_raw_image_buffer_size(uint8_t packet_index, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return 0;

	return util_video_decoder_max_raw_image[session][packet_index];
}

uint32_t Util_decoder_mvd_get_raw_image_buffer_size(uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return 0;

	return util_mvd_video_decoder_max_raw_image[session];
}

uint32_t Util_decoder_audio_decode(uint32_t* samples, uint8_t** raw_data, double* current_pos, uint8_t packet_index, uint8_t session)
{
	int32_t ffmpeg_result = 0;
	uint32_t copy_size_per_ch = 0;
	double current_frame = 0;
	double timebase = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_AUDIO_TRACKS || !samples || !raw_data || !current_pos)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_audio_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_audio_decoder_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	if(util_audio_decoder_packet[session][packet_index]->duration != 0)
		current_frame = (double)util_audio_decoder_packet[session][packet_index]->dts / util_audio_decoder_packet[session][packet_index]->duration;

	*samples = 0;
	*current_pos = 0;

	util_audio_decoder_raw_data[session][packet_index] = av_frame_alloc();
	if(!util_audio_decoder_raw_data[session][packet_index])
	{
		DEF_LOG_RESULT(av_frame_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_send_packet(util_audio_decoder_context[session][packet_index], util_audio_decoder_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_send_packet, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_receive_frame(util_audio_decoder_context[session][packet_index], util_audio_decoder_raw_data[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_receive_frame, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][packet_index]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_audio_decoder_packet[session][packet_index]->dts * timebase * 1000;//calc pos
	else
		*current_pos = current_frame * ((1000.0 / util_audio_decoder_raw_data[session][packet_index]->sample_rate) * util_audio_decoder_raw_data[session][packet_index]->nb_samples);//calc pos

	copy_size_per_ch = util_audio_decoder_raw_data[session][packet_index]->nb_samples * util_audio_decoder_sample_format_size_table[util_audio_decoder_context[session][packet_index]->sample_fmt];
	free(*raw_data);
	*raw_data = NULL;
	*raw_data = (uint8_t*)linearAlloc(copy_size_per_ch * util_audio_decoder_context[session][packet_index]->ch_layout.nb_channels);
	if(!*raw_data)
		goto out_of_memory;

	if(util_audio_decoder_context[session][packet_index]->sample_fmt == AV_SAMPLE_FMT_U8P || util_audio_decoder_context[session][packet_index]->sample_fmt == AV_SAMPLE_FMT_S16P
	|| util_audio_decoder_context[session][packet_index]->sample_fmt == AV_SAMPLE_FMT_S32P || util_audio_decoder_context[session][packet_index]->sample_fmt == AV_SAMPLE_FMT_S64P
	|| util_audio_decoder_context[session][packet_index]->sample_fmt == AV_SAMPLE_FMT_FLTP || util_audio_decoder_context[session][packet_index]->sample_fmt == AV_SAMPLE_FMT_DBLP)
	{
		for(uint8_t i = 0; i < util_audio_decoder_context[session][packet_index]->ch_layout.nb_channels; i++)
			memcpy(((*raw_data) + (copy_size_per_ch * i)), util_audio_decoder_raw_data[session][packet_index]->data[i], copy_size_per_ch);
	}
	else
		memcpy(*raw_data, util_audio_decoder_raw_data[session][packet_index]->data[0], copy_size_per_ch * util_audio_decoder_context[session][packet_index]->ch_layout.nb_channels);

	*samples = util_audio_decoder_raw_data[session][packet_index]->nb_samples;

	util_audio_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_frame_free(&util_audio_decoder_raw_data[session][packet_index]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	out_of_memory:
	util_audio_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_frame_free(&util_audio_decoder_raw_data[session][packet_index]);
	return DEF_ERR_OUT_OF_MEMORY;

	ffmpeg_api_failed:
	util_audio_decoder_packet_ready[session][packet_index] = false;
	free(*raw_data);
	*raw_data = NULL;
	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_frame_free(&util_audio_decoder_raw_data[session][packet_index]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_video_decode(uint8_t packet_index, uint8_t session)
{
	int32_t send_ffmpeg_result = 0;
	int32_t receive_ffmpeg_result = 0;
	uint16_t buffer_num = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_video_decoder_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	util_video_decoder_changeable_buffer_size[session][packet_index] = false;
	if(util_video_decoder_available_raw_image[session][packet_index] + 1 >= util_video_decoder_max_raw_image[session][packet_index])
	{
		//DEF_LOG_STRING("Queues are full!!!!!");
		goto try_again;
	}

	buffer_num = util_video_decoder_raw_image_current_index[session][packet_index];

	util_video_decoder_raw_image[session][packet_index][buffer_num] = av_frame_alloc();
	if(!util_video_decoder_raw_image[session][packet_index][buffer_num])
	{
		DEF_LOG_RESULT(av_frame_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	send_ffmpeg_result = avcodec_send_packet(util_video_decoder_context[session][packet_index], util_video_decoder_packet[session][packet_index]);
	//Some decoders (such as av1 and vp9) may return EAGAIN if so, ignore it and call avcodec_receive_frame().
	if(send_ffmpeg_result != 0 && send_ffmpeg_result != AVERROR(EAGAIN))
	{
		DEF_LOG_RESULT(avcodec_send_packet, false, send_ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	receive_ffmpeg_result = avcodec_receive_frame(util_video_decoder_context[session][packet_index], util_video_decoder_raw_image[session][packet_index][buffer_num]);
	if(receive_ffmpeg_result != 0)
	{
		if(send_ffmpeg_result == AVERROR(EAGAIN))
			goto try_again_no_output;
		else
		{
			DEF_LOG_RESULT(avcodec_receive_frame, false, receive_ffmpeg_result);
			goto ffmpeg_api_failed;
		}
	}

	if(buffer_num + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_current_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_current_index[session][packet_index] = 0;

	LightLock_Lock(&util_video_decoder_raw_image_mutex[session][packet_index]);
	util_video_decoder_available_raw_image[session][packet_index]++;
	LightLock_Unlock(&util_video_decoder_raw_image_mutex[session][packet_index]);

	if(send_ffmpeg_result == AVERROR(EAGAIN))
		goto try_again_with_output;

	util_video_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	try_again_no_output:
	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);
	return DEF_ERR_DECODER_TRY_AGAIN_NO_OUTPUT;

	try_again_with_output:
	return DEF_ERR_DECODER_TRY_AGAIN;

	ffmpeg_api_failed:
	util_video_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_mvd_decode(uint8_t session)
{
	bool got_a_frame = false;
	bool got_a_frame_after_processing_nal_unit = false;
	uint32_t offset = 0;
	uint32_t source_offset = 0;
	uint32_t size = 0;
	uint16_t buffer_num = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		goto not_inited;

	if(!util_video_decoder_packet_ready[session][0])
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	util_mvd_video_decoder_changeable_buffer_size = false;
	if(util_mvd_video_decoder_available_raw_image[session] + 1 >= util_mvd_video_decoder_max_raw_image[session])
	{
		//DEF_LOG_STRING("Queues are full!!!!!");
		goto try_again;
	}

	buffer_num = util_mvd_video_decoder_raw_image_current_index[session];
	width = util_video_decoder_context[session][0]->width;
	height = util_video_decoder_context[session][0]->height;
	if(width % 16 != 0)
		width += 16 - width % 16;
	if(height % 16 != 0)
		height += 16 - height % 16;

	util_mvd_video_decoder_raw_image[session][buffer_num] = av_frame_alloc();
	if(!util_mvd_video_decoder_raw_image[session][buffer_num])
	{
		DEF_LOG_RESULT(av_frame_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = (uint8_t*)linearAlloc(width * height * 2);
	if(!util_mvd_video_decoder_raw_image[session][buffer_num]->data[0])
		goto out_of_linear_memory;

	if(util_video_decoder_packet[session][0]->size > (int32_t)util_mvd_video_decoder_packet_size)
	{
		util_mvd_video_decoder_packet_size = util_video_decoder_packet[session][0]->size;
		free(util_mvd_video_decoder_packet);
		util_mvd_video_decoder_packet = NULL;
		util_mvd_video_decoder_packet = (uint8_t*)linearAlloc(util_mvd_video_decoder_packet_size);
		if(!util_mvd_video_decoder_packet)
		{
			util_mvd_video_decoder_packet_size = 0;
			goto out_of_linear_memory;
		}
	}

	if(util_mvd_video_decoder_first)
	{
		mvdstdGenerateDefaultConfig(&util_decoder_mvd_config, width, height, width, height, NULL, NULL, NULL);

		//Set extra data.
		offset = 0;
		memset(util_mvd_video_decoder_packet, 0x0, 0x2);
		offset += 2;
		memset(util_mvd_video_decoder_packet + offset, 0x1, 0x1);
		offset += 1;
		memcpy(util_mvd_video_decoder_packet + offset, util_video_decoder_context[session][0]->extradata + 8, *(util_video_decoder_context[session][0]->extradata + 7));
		offset += *(util_video_decoder_context[session][0]->extradata + 7);

		mvdstdProcessVideoFrame(util_mvd_video_decoder_packet, offset, 0, NULL);

		offset = 0;
		memset(util_mvd_video_decoder_packet, 0x0, 0x2);
		offset += 2;
		memset(util_mvd_video_decoder_packet + offset, 0x1, 0x1);
		offset += 1;
		memcpy(util_mvd_video_decoder_packet + offset, util_video_decoder_context[session][0]->extradata + 11 + *(util_video_decoder_context[session][0]->extradata + 7), *(util_video_decoder_context[session][0]->extradata + 10 + *(util_video_decoder_context[session][0]->extradata + 7)));
		offset += *(util_video_decoder_context[session][0]->extradata + 10 + *(util_video_decoder_context[session][0]->extradata + 7));

		mvdstdProcessVideoFrame(util_mvd_video_decoder_packet, offset, 0, NULL);
	}
	util_decoder_mvd_config.physaddr_outdata0 = osConvertVirtToPhys(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);

	offset = 0;
	source_offset = 0;

	while((source_offset + 4) < (uint32_t)util_video_decoder_packet[session][0]->size)
	{
		//Get nal size.
		size = *((uint32_t*)(util_video_decoder_packet[session][0]->data + source_offset));
		size = __builtin_bswap32(size);
		source_offset += 4;
		if((source_offset + size) > (uint32_t)util_video_decoder_packet[session][0]->size || size == 0)
		{
			if(size == 0)
				DEF_LOG_FORMAT("unexpected nal size : %" PRIu32, size);
			else
			{
				DEF_LOG_FORMAT("unexpected nal size : %" PRIu32 " (%" PRIu32 " > %" PRIu32 ")", size,
				(source_offset + size), (uint32_t)util_video_decoder_packet[session][0]->size);
			}

			goto ffmpeg_api_failed;
		}

		//Set nal prefix 0x0 0x0 0x1.
		memset(util_mvd_video_decoder_packet + offset, 0x0, 0x2);
		offset += 2;
		memset(util_mvd_video_decoder_packet + offset, 0x1, 0x1);
		offset += 1;

		//Copy raw nal data.
		memcpy(util_mvd_video_decoder_packet + offset, (util_video_decoder_packet[session][0]->data + source_offset), size);
		offset += size;
		source_offset += size;
	}

	//Set 0x11 to top-left, top-right, bottom-left and bottom-right then check them later.
	//For more information, see : https://gbatemp.net/threads/release-video-player-for-3ds.586094/page-20#post-9915780
	*util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = 0x11;
	*(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * 2 - 1)) = 0x11;
	*(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + ((width * height * 2) - (width * 2))) = 0x11;
	*(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * height * 2 - 1)) = 0x11;

	// DEF_LOG_STRING("-------------------------------");

	MVDSTD_SetConfig(&util_decoder_mvd_config);

	if(!util_mvd_video_decoder_should_skip_process_nal_unit)
	{
		// DEF_LOG_STRING("util_mvd_video_decoder_should_skip_process_nal_unit is not set, so call mvdstdProcessVideoFrame()");
		result = mvdstdProcessVideoFrame(util_mvd_video_decoder_packet, offset, 0, NULL);

		//Save pts
		util_mvd_video_decoder_cached_pts[util_mvd_video_decoder_next_cached_pts_index] = util_video_decoder_packet[session][0]->dts;
		if(util_mvd_video_decoder_next_cached_pts_index + 1 < 32)
			util_mvd_video_decoder_next_cached_pts_index++;
		else
			util_mvd_video_decoder_next_cached_pts_index = 0;

		if(util_mvd_video_decoder_first)
		{
			//Do I need to send same nal data at first frame?
			result = mvdstdProcessVideoFrame(util_mvd_video_decoder_packet, offset, 0, NULL);
			util_mvd_video_decoder_first = false;
		}

		//If any of them got changed, it means MVD service wrote the frame data to the buffer.
		if(*util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] != 0x11
		|| *(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * 2 - 1)) != 0x11
		|| *(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + ((width * height * 2) - (width * 2))) != 0x11
		|| *(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * height * 2 - 1)) != 0x11)
		{
			// DEF_LOG_STRING("got a frame after mvdstdProcessVideoFrame()");
			got_a_frame = true;
			util_mvd_video_decoder_should_skip_process_nal_unit = true;
			got_a_frame_after_processing_nal_unit = true;
		}
		// else
		// 	DEF_LOG_STRING("no frames after mvdstdProcessVideoFrame()");

		if(result != MVD_STATUS_FRAMEREADY && result != MVD_STATUS_PARAMSET)
		{
			DEF_LOG_RESULT(mvdstdProcessVideoFrame, false, result);
			goto nintendo_api_failed;
		}
	}
	// else
	// 	DEF_LOG_STRING("util_mvd_video_decoder_should_skip_process_nal_unit is set, so skip mvdstdProcessVideoFrame()");

	if(!got_a_frame)
	{
		// DEF_LOG_STRING("got_a_frame is not set, so call mvdstdRenderVideoFrame()");
		while(true)
		{
			//You need to use a custom libctru to use NULL here. https://github.com/Core-2-Extreme/libctru_custom/tree/3ds
			result = mvdstdRenderVideoFrame(NULL, false);

			//If any of them got changed, it means MVD service wrote the frame data to the buffer.
			if(*util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] != 0x11
			|| *(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * 2 - 1)) != 0x11
			|| *(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + ((width * height * 2) - (width * 2))) != 0x11
			|| *(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * height * 2 - 1)) != 0x11)
			{
				// DEF_LOG_STRING("got a frame after mvdstdRenderVideoFrame()");
				result = MVD_STATUS_OK;
				got_a_frame = true;
			}
			// else
			// 	DEF_LOG_STRING("no frames after mvdstdRenderVideoFrame()");

			if(result != MVD_STATUS_BUSY || got_a_frame)
				break;
			// else
			// 	DEF_LOG_STRING("mvdstdRenderVideoFrame() returned MVD_STATUS_BUSY, so try again");
		}

		if(result != MVD_STATUS_OK)
		{
			DEF_LOG_RESULT(mvdstdRenderVideoFrame, false, result);
			goto nintendo_api_failed;
		}
	}
	// else
	// 	DEF_LOG_STRING("got_a_frame is set, so skip mvdstdRenderVideoFrame()");

	if(!got_a_frame && util_mvd_video_decoder_should_skip_process_nal_unit)
	{
		// DEF_LOG_STRING("util_mvd_video_decoder_should_skip_process_nal_unit is set, and got no frames");
		util_mvd_video_decoder_should_skip_process_nal_unit = false;
		goto try_again_no_output;
	}
	else if(!got_a_frame)
	{
		// DEF_LOG_STRING("Got no frames");
		goto need_more_packet;
	}

	//Restore cached pts.
	util_mvd_video_decoder_raw_image[session][buffer_num]->pts = util_mvd_video_decoder_cached_pts[util_mvd_video_decoder_current_cached_pts_index];
	util_mvd_video_decoder_raw_image[session][buffer_num]->duration = util_video_decoder_packet[session][0]->duration;
	if(util_mvd_video_decoder_current_cached_pts_index + 1 < 32)
		util_mvd_video_decoder_current_cached_pts_index++;
	else
		util_mvd_video_decoder_current_cached_pts_index = 0;

	if(buffer_num + 1 < util_mvd_video_decoder_max_raw_image[session])
		util_mvd_video_decoder_raw_image_current_index[session]++;
	else
		util_mvd_video_decoder_raw_image_current_index[session] = 0;

	LightLock_Lock(&util_mvd_video_decoder_raw_image_mutex[session]);
	util_mvd_video_decoder_available_raw_image[session]++;
	LightLock_Unlock(&util_mvd_video_decoder_raw_image_mutex[session]);

	if(util_mvd_video_decoder_should_skip_process_nal_unit && !got_a_frame_after_processing_nal_unit)
	{
		// DEF_LOG_STRING("util_mvd_video_decoder_should_skip_process_nal_unit is set, and got a frame");
		goto try_again_with_output;
	}

	util_video_decoder_packet_ready[session][0] = false;
	av_packet_free(&util_video_decoder_packet[session][0]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	try_again_no_output:
	free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	return DEF_ERR_DECODER_TRY_AGAIN_NO_OUTPUT;

	try_again_with_output:
	return DEF_ERR_DECODER_TRY_AGAIN;

	need_more_packet:
	util_video_decoder_packet_ready[session][0] = false;
	free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	return DEF_ERR_NEED_MORE_INPUT;

	out_of_linear_memory:
	util_video_decoder_packet_ready[session][0] = false;
	free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	return DEF_ERR_OUT_OF_LINEAR_MEMORY;

	ffmpeg_api_failed:
	util_video_decoder_packet_ready[session][0] = false;
	free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;

	nintendo_api_failed:
	util_video_decoder_packet_ready[session][0] = false;
	free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	return result;
}

uint32_t Util_decoder_subtitle_decode(Media_s_data* subtitle_data, uint8_t packet_index, uint8_t session)
{
	int32_t ffmpeg_result = 0;
	//We can't get rid of this "int" because library uses "int" type as args.
	int decoded = 0;
	double timebase = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_SUBTITLE_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_subtitle_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_subtitle_decoder_packet_ready[session][packet_index])
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	subtitle_data->text = NULL;
	subtitle_data->start_time = 0;
	subtitle_data->end_time = 0;

	util_subtitle_decoder_raw_data[session][packet_index] = (AVSubtitle*)linearAlloc(sizeof(AVSubtitle));
	if(!util_subtitle_decoder_raw_data[session][packet_index])
	{
		DEF_LOG_RESULT(linearAlloc, false, DEF_ERR_OUT_OF_MEMORY);
		goto out_of_memory;
	}

	ffmpeg_result = avcodec_decode_subtitle2(util_subtitle_decoder_context[session][packet_index], util_subtitle_decoder_raw_data[session][packet_index], &decoded, util_subtitle_decoder_packet[session][packet_index]);
	if(ffmpeg_result < 0)
	{
		DEF_LOG_RESULT(avcodec_decode_subtitle2, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	if(decoded > 0)
	{
		timebase = av_q2d(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][packet_index]]->time_base);
		if(timebase != 0)
		{
			subtitle_data->start_time = (double)util_subtitle_decoder_packet[session][packet_index]->dts * timebase * 1000;//calc pos
			subtitle_data->end_time = subtitle_data->start_time + (util_subtitle_decoder_packet[session][packet_index]->duration * timebase * 1000);
		}

		for(uint32_t i = 0; i < util_subtitle_decoder_raw_data[session][packet_index]->num_rects; i++)
		{
			free(subtitle_data->bitmap);
			subtitle_data->bitmap = NULL;
			subtitle_data->bitmap_x = 0;
			subtitle_data->bitmap_y = 0;
			subtitle_data->bitmap_width = 0;
			subtitle_data->bitmap_height = 0;
			free(subtitle_data->text);
			subtitle_data->text = NULL;

			if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->type == SUBTITLE_BITMAP)
			{
				uint8_t* index_table = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->data[0];
				uint32_t* color_table = (uint32_t*)util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->data[1];
				uint32_t index = 0;
				uint32_t size = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->w * util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->h * 4;

				if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->nb_colors != 4)
				{
					DEF_LOG_STRING("Unsupported format.");
					goto unsupported;
				}

				subtitle_data->bitmap = (uint8_t*)malloc(size);
				if(!subtitle_data->bitmap)
					goto out_of_memory;

				subtitle_data->bitmap_x = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->x;
				subtitle_data->bitmap_y = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->y;
				subtitle_data->bitmap_width = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->w;
				subtitle_data->bitmap_height = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->h;

				//Convert to ABGR8888.
				for(uint32_t k = 0; k < subtitle_data->bitmap_height; k++)
				{
					for(uint32_t s = 0; s < subtitle_data->bitmap_width; s++)
						((uint32_t*)subtitle_data->bitmap)[index++] = __builtin_bswap32(color_table[index_table[(subtitle_data->bitmap_width * k) + s]]);
				}
			}
			else if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->type == SUBTITLE_TEXT)
			{
				uint32_t size = strlen(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->text);

				subtitle_data->text = (char*)malloc(size + 1);
				if(!subtitle_data->text)
					goto out_of_memory;

				memcpy(subtitle_data->text, util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->text, size);
				subtitle_data->text[size] = 0x00;
			}
			else if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->type == SUBTITLE_ASS)
			{
				uint32_t size = 0;
				char* text = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->ass;

				for(uint8_t k = 0; k < 8; k++)
				{
					//Make sure subtitle data is formatted correctly.
					char* found = strstr(text, ",");

					if(!found)
						break;

					text = (found + 1);

					if(k == 7)
					{
						char* source = text;
						char* destination = NULL;
						uint32_t remaining = 0;

						size = strlen(source);
						subtitle_data->text = (char*)malloc(size + 1);
						if(!subtitle_data->text)
							goto out_of_memory;

						memset(subtitle_data->text, 0x0, (size + 1));
						destination = subtitle_data->text;
						remaining = size;

						while(true)
						{
							const char* new_line = strstr(source, "\\N");
							uint32_t copy_size = 0;

							if(!new_line)
							{
								//No new line codes were found, copy all of remaining data.
								copy_size = remaining;
								memcpy(destination, source, copy_size);
								break;
							}

							//Copy until (\N).
							copy_size = (new_line - source);
							memcpy(destination, source, copy_size);
							destination += copy_size;
							source += copy_size;
							remaining -= copy_size;

							//Replace string (\N) to new line code (\n).
							destination[0] = '\n';
							//(\n) is 1 byte.
							destination++;
							//(\N) is 2 bytes.
							source += 2;
							remaining -= 2;
						}
					}
				}

				if(!subtitle_data->text)
				{
					DEF_LOG_STRING("Corrupted data.");
					goto unsupported;
				}
			}
		}
	}

	util_subtitle_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	avsubtitle_free(util_subtitle_decoder_raw_data[session][packet_index]);
	free(util_subtitle_decoder_raw_data[session][packet_index]);
	util_subtitle_decoder_raw_data[session][packet_index] = NULL;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	ffmpeg_api_failed:
	util_subtitle_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	avsubtitle_free(util_subtitle_decoder_raw_data[session][packet_index]);
	free(util_subtitle_decoder_raw_data[session][packet_index]);
	util_subtitle_decoder_raw_data[session][packet_index] = NULL;
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;

	out_of_memory:
	util_subtitle_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	avsubtitle_free(util_subtitle_decoder_raw_data[session][packet_index]);
	free(util_subtitle_decoder_raw_data[session][packet_index]);
	util_subtitle_decoder_raw_data[session][packet_index] = NULL;
	return DEF_ERR_OUT_OF_MEMORY;

	unsupported:
	util_subtitle_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	avsubtitle_free(util_subtitle_decoder_raw_data[session][packet_index]);
	free(util_subtitle_decoder_raw_data[session][packet_index]);
	util_subtitle_decoder_raw_data[session][packet_index] = NULL;
	return DEF_ERR_OTHER;
}

void Util_decoder_video_clear_raw_image(uint8_t packet_index, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return;

	for(uint16_t k = 0; k < util_video_decoder_max_raw_image[session][packet_index]; k++)
		av_frame_free(&util_video_decoder_raw_image[session][packet_index][k]);

	util_video_decoder_available_raw_image[session][packet_index] = 0;
	util_video_decoder_raw_image_ready_index[session][packet_index] = 0;
	util_video_decoder_raw_image_current_index[session][packet_index] = 0;
}

void Util_decoder_mvd_clear_raw_image(uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return;

	for(uint16_t i = 0; i < util_mvd_video_decoder_max_raw_image[session]; i++)
	{
		if(util_mvd_video_decoder_raw_image[session][i])
		{
			free(util_mvd_video_decoder_raw_image[session][i]->data[0]);
			for(uint8_t k = 0; k < AV_NUM_DATA_POINTERS; k++)
				util_mvd_video_decoder_raw_image[session][i]->data[k] = NULL;
		}
		av_frame_free(&util_mvd_video_decoder_raw_image[session][i]);
	}

	util_mvd_video_decoder_available_raw_image[session] = 0;
	util_mvd_video_decoder_raw_image_ready_index[session] = 0;
	util_mvd_video_decoder_raw_image_current_index[session] = 0;
}

uint16_t Util_decoder_video_get_available_raw_image_num(uint8_t packet_index, uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return 0;
	else
		return util_video_decoder_available_raw_image[session][packet_index];
}

uint16_t Util_decoder_mvd_get_available_raw_image_num(uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return 0;
	else
		return util_mvd_video_decoder_available_raw_image[session];
}

uint32_t Util_decoder_video_get_image(uint8_t** raw_data, double* current_pos, uint32_t width, uint32_t height, uint8_t packet_index, uint8_t session)
{
	bool is_linear = true;
	uint32_t cpy_size = 0;
	uint16_t buffer_num = 0;
	uint32_t y_offset = 0;
	uint32_t u_offset = 0;
	uint32_t v_offset = 0;
	uint32_t y_size = width * height;
	uint32_t uv_size = width * height / 4;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;

#if DEF_DECODER_DMA_ENABLE
	uint32_t dma_result[3] = { 0, 0, 0, };
	Handle dma_handle[3] = { 0, 0, 0, };
	DmaConfig dma_config;
#endif //DEF_DECODER_DMA_ENABLE

	if(!raw_data || !current_pos || width == 0 || height == 0
	|| packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		goto not_inited;

	if(util_video_decoder_available_raw_image[session][packet_index] == 0)
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	cpy_size = av_image_get_buffer_size(util_video_decoder_context[session][packet_index]->pix_fmt, width, height, 1);
	*current_pos = 0;
	free(*raw_data);
	*raw_data = NULL;
	*raw_data = (uint8_t*)linearAlloc(cpy_size);
	if(!*raw_data)
		goto out_of_memory;

	buffer_num = util_video_decoder_raw_image_ready_index[session][packet_index];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.den;
	if(util_video_decoder_raw_image[session][packet_index][buffer_num]->duration != 0)
		current_frame = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts / util_video_decoder_raw_image[session][packet_index][buffer_num]->duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->time_base);
	if(timebase != 0)
	{
		if(util_video_decoder_raw_image[session][packet_index][buffer_num]->pts == AV_NOPTS_VALUE)//If pts is not available, use dts instead.
			*current_pos = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pkt_dts * timebase * 1000;//Calc pos.
		else
			*current_pos = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts * timebase * 1000;//Calc pos.
	}
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//Calc frame pos.

	y_offset = (uint32_t)util_video_decoder_raw_image[session][packet_index][buffer_num]->data[0];
	u_offset = (uint32_t)util_video_decoder_raw_image[session][packet_index][buffer_num]->data[1];
	v_offset = (uint32_t)util_video_decoder_raw_image[session][packet_index][buffer_num]->data[2];

	//Check if the decoded data is in linear format (some decoder return in not linear format).
	//Currently, only check for YUV420P because it only occurs in h263p afaik.
	if(util_video_decoder_context[session][packet_index]->pix_fmt == AV_PIX_FMT_YUV420P
	&& (y_offset + y_size != u_offset || u_offset + uv_size != v_offset))
		is_linear = false;

#if DEF_DECODER_DMA_ENABLE
	if(is_linear)
	{
		svcFlushProcessDataCache(CUR_PROCESS_HANDLE, y_offset, cpy_size);

		dmaConfigInitDefault(&dma_config);
		dma_result[0] = svcStartInterProcessDma(&dma_handle[0], CUR_PROCESS_HANDLE, (uint32_t)*raw_data, CUR_PROCESS_HANDLE, (uint32_t)util_video_decoder_raw_image[session][packet_index][buffer_num]->data[0], cpy_size, &dma_config);

		if(dma_result[0] == DEF_SUCCESS)
			svcWaitSynchronization(dma_handle[0], INT64_MAX);

		svcCloseHandle(dma_handle[0]);

		svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (uint32_t)*raw_data, cpy_size);
	}
	else
	{
		svcFlushProcessDataCache(CUR_PROCESS_HANDLE, y_offset, y_size);
		svcFlushProcessDataCache(CUR_PROCESS_HANDLE, u_offset, uv_size);
		svcFlushProcessDataCache(CUR_PROCESS_HANDLE, v_offset, uv_size);

		dmaConfigInitDefault(&dma_config);
		dma_result[0] = svcStartInterProcessDma(&dma_handle[0], CUR_PROCESS_HANDLE, (uint32_t)*raw_data, CUR_PROCESS_HANDLE, (uint32_t)util_video_decoder_raw_image[session][packet_index][buffer_num]->data[0], y_size, &dma_config);
		dma_result[1] = svcStartInterProcessDma(&dma_handle[1], CUR_PROCESS_HANDLE, (uint32_t)(*raw_data) + y_size, CUR_PROCESS_HANDLE, (uint32_t)util_video_decoder_raw_image[session][packet_index][buffer_num]->data[1], uv_size, &dma_config);
		dma_result[2] = svcStartInterProcessDma(&dma_handle[2], CUR_PROCESS_HANDLE, (uint32_t)(*raw_data) + y_size + uv_size, CUR_PROCESS_HANDLE, (uint32_t)util_video_decoder_raw_image[session][packet_index][buffer_num]->data[2], uv_size, &dma_config);

		for(uint8_t i = 0; i < 3; i++)
		{
			if(dma_result[i] == DEF_SUCCESS)
				svcWaitSynchronization(dma_handle[i], INT64_MAX);

			svcCloseHandle(dma_handle[i]);
		}

		svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (uint32_t)*raw_data, cpy_size);
	}
#else
	if(is_linear)
		memcpy_asm(*raw_data, util_video_decoder_raw_image[session][packet_index][buffer_num]->data[0], cpy_size);
	else
	{
		memcpy_asm(*raw_data, util_video_decoder_raw_image[session][packet_index][buffer_num]->data[0], y_size);
		memcpy_asm((*raw_data) + y_size, util_video_decoder_raw_image[session][packet_index][buffer_num]->data[1], uv_size);
		memcpy_asm((*raw_data) + y_size + uv_size, util_video_decoder_raw_image[session][packet_index][buffer_num]->data[2], uv_size);
	}
#endif //DEF_DECODER_DMA_ENABLE

	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);

	if(util_video_decoder_raw_image_ready_index[session][packet_index] + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_ready_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_ready_index[session][packet_index] = 0;

	LightLock_Lock(&util_video_decoder_raw_image_mutex[session][packet_index]);
	util_video_decoder_available_raw_image[session][packet_index]--;
	LightLock_Unlock(&util_video_decoder_raw_image_mutex[session][packet_index]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Util_decoder_mvd_get_image(uint8_t** raw_data, double* current_pos, uint32_t width, uint32_t height, uint8_t session)
{
	uint32_t cpy_size = 0;
	uint16_t buffer_num = 0;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;

#if DEF_DECODER_DMA_ENABLE
	uint32_t dma_result = 0;
	Handle dma_handle = 0;
	DmaConfig dma_config;
#endif //DEF_DECODER_DMA_ENABLE

	if(!raw_data || !current_pos || width == 0 || height == 0 || session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		goto not_inited;

	if(util_mvd_video_decoder_available_raw_image[session] == 0)
	{
		//DEF_LOG_STRING("No packets are available!!!!!");
		goto try_again;
	}

	free(*raw_data);
	*raw_data = NULL;
	*raw_data = (uint8_t*)linearAlloc(width * height * 2);
	if(!*raw_data)
		goto out_of_memory;

	*current_pos = 0;
	buffer_num = util_mvd_video_decoder_raw_image_ready_index[session];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	if(util_mvd_video_decoder_raw_image[session][buffer_num]->duration != 0)
		current_frame = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts / util_mvd_video_decoder_raw_image[session][buffer_num]->duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts * timebase * 1000;//Calc pos.
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//Calc frame pos.

	cpy_size = (width * height * 2);

#if DEF_DECODER_DMA_ENABLE
	svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (uint32_t)util_mvd_video_decoder_raw_image[session][buffer_num]->data[0], cpy_size);

	dmaConfigInitDefault(&dma_config);
	dma_result = svcStartInterProcessDma(&dma_handle, CUR_PROCESS_HANDLE, (uint32_t)*raw_data, CUR_PROCESS_HANDLE, (uint32_t)util_mvd_video_decoder_raw_image[session][buffer_num]->data[0], cpy_size, &dma_config);

	if(dma_result == DEF_SUCCESS)
		svcWaitSynchronization(dma_handle, INT64_MAX);

	svcCloseHandle(dma_handle);
	svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (uint32_t)*raw_data, cpy_size);
#else
	memcpy_asm(*raw_data, util_mvd_video_decoder_raw_image[session][buffer_num]->data[0], cpy_size);
#endif //DEF_DECODER_DMA_ENABLE

	buffer_num = util_mvd_video_decoder_raw_image_ready_index[session];

	if(util_mvd_video_decoder_raw_image[session][buffer_num])
	{
		free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
		for(uint8_t i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_mvd_video_decoder_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);

	if(util_mvd_video_decoder_raw_image_ready_index[session] + 1 < util_mvd_video_decoder_max_raw_image[session])
		util_mvd_video_decoder_raw_image_ready_index[session]++;
	else
		util_mvd_video_decoder_raw_image_ready_index[session] = 0;

	LightLock_Lock(&util_mvd_video_decoder_raw_image_mutex[session]);
	util_mvd_video_decoder_available_raw_image[session]--;
	LightLock_Unlock(&util_mvd_video_decoder_raw_image_mutex[session]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

void Util_decoder_video_skip_image(double* current_pos, uint8_t packet_index, uint8_t session)
{
	uint16_t buffer_num = 0;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;

	if(!current_pos || packet_index >= DEF_DECODER_MAX_VIDEO_TRACKS || session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return;

	if(util_video_decoder_available_raw_image[session][packet_index] == 0)
		return;

	*current_pos = 0;
	buffer_num = util_video_decoder_raw_image_ready_index[session][packet_index];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.den;
	if(util_video_decoder_raw_image[session][packet_index][buffer_num]->duration != 0)
		current_frame = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts / util_video_decoder_raw_image[session][packet_index][buffer_num]->duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->time_base);
	if(timebase != 0)
	{
		if(util_video_decoder_raw_image[session][packet_index][buffer_num]->pts == AV_NOPTS_VALUE)//If pts is not available, use dts instead.
			*current_pos = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pkt_dts * timebase * 1000;//Calc pos.
		else
			*current_pos = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts * timebase * 1000;//Calc pos.
	}
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//Calc frame pos.

	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);

	if(util_video_decoder_raw_image_ready_index[session][packet_index] + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_ready_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_ready_index[session][packet_index] = 0;

	LightLock_Lock(&util_video_decoder_raw_image_mutex[session][packet_index]);
	util_video_decoder_available_raw_image[session][packet_index]--;
	LightLock_Unlock(&util_video_decoder_raw_image_mutex[session][packet_index]);
}

void Util_decoder_mvd_skip_image(double* current_pos, uint8_t session)
{
	uint16_t buffer_num = 0;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;

	if(!current_pos || session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return;

	if(util_mvd_video_decoder_available_raw_image[session] == 0)
		return;

	*current_pos = 0;
	buffer_num = util_mvd_video_decoder_raw_image_ready_index[session];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	if(util_mvd_video_decoder_raw_image[session][buffer_num]->duration != 0)
		current_frame = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts / util_mvd_video_decoder_raw_image[session][buffer_num]->duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts * timebase * 1000;//Calc pos.
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//Calc frame pos.

	if(util_mvd_video_decoder_raw_image[session][buffer_num])
	{
		free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
		for(uint8_t i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_mvd_video_decoder_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);

	if(util_mvd_video_decoder_raw_image_ready_index[session] + 1 < util_mvd_video_decoder_max_raw_image[session])
		util_mvd_video_decoder_raw_image_ready_index[session]++;
	else
		util_mvd_video_decoder_raw_image_ready_index[session] = 0;

	LightLock_Lock(&util_mvd_video_decoder_raw_image_mutex[session]);
	util_mvd_video_decoder_available_raw_image[session]--;
	LightLock_Unlock(&util_mvd_video_decoder_raw_image_mutex[session]);
}

uint32_t Util_decoder_seek(uint64_t seek_pos, Media_seek_flag flag, uint8_t session)
{
	int32_t ffmpeg_result = 0;
	int32_t ffmpeg_seek_flag = 0;

	if(session >= DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;

	if(flag & MEDIA_SEEK_FLAG_BACKWARD)
		ffmpeg_seek_flag |= AVSEEK_FLAG_BACKWARD;
	if(flag & MEDIA_SEEK_FLAG_BYTE)
		ffmpeg_seek_flag |= AVSEEK_FLAG_BYTE;
	if(flag & MEDIA_SEEK_FLAG_ANY)
		ffmpeg_seek_flag |= AVSEEK_FLAG_ANY;
	if(flag & MEDIA_SEEK_FLAG_FRAME)
		ffmpeg_seek_flag |= AVSEEK_FLAG_FRAME;

	ffmpeg_result = avformat_seek_file(util_decoder_format_context[session], -1, INT64_MIN, seek_pos * 1000, INT64_MAX, ffmpeg_seek_flag);
	if(ffmpeg_result < 0)
	{
		DEF_LOG_RESULT(avformat_seek_file, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	ffmpeg_api_failed:
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

void Util_decoder_close_file(uint8_t session)
{
	if(session >= DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session])
		return;

	util_decoder_opened_file[session] = false;
	Util_decoder_audio_exit(session);
	Util_decoder_video_exit(session);
	Util_decoder_mvd_exit(session);
	Util_decoder_subtitle_exit(session);
	for(uint16_t i = 0; i < DEF_DECODER_MAX_CACHE_PACKETS; i++)
		av_packet_free(&util_decoder_cache_packet[session][i]);

	util_decoder_available_cache_packet[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_cache_packet_ready_index[session] = 0;
	avformat_close_input(&util_decoder_format_context[session]);
}

static void Util_decoder_audio_exit(uint8_t session)
{
	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		if(util_audio_decoder_init[session][i])
		{
			util_audio_decoder_init[session][i] = false;
			util_audio_decoder_cache_packet_ready[session][i] = false;
			util_audio_decoder_packet_ready[session][i] = false;
			avcodec_free_context(&util_audio_decoder_context[session][i]);
			av_packet_free(&util_audio_decoder_packet[session][i]);
			av_packet_free(&util_audio_decoder_cache_packet[session][i]);
			av_frame_free(&util_audio_decoder_raw_data[session][i]);
		}
	}
}

static void Util_decoder_video_exit(uint8_t session)
{
	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		if(util_video_decoder_init[session][i])
		{
			util_video_decoder_init[session][i] = false;
			util_video_decoder_cache_packet_ready[session][i] = false;
			util_video_decoder_packet_ready[session][i] = false;
			avcodec_free_context(&util_video_decoder_context[session][i]);
			av_packet_free(&util_video_decoder_packet[session][i]);
			av_packet_free(&util_video_decoder_cache_packet[session][i]);
			util_video_decoder_available_raw_image[session][i] = 0;
			util_video_decoder_raw_image_ready_index[session][i] = 0;
			util_video_decoder_raw_image_current_index[session][i] = 0;
			for(uint16_t k = 0; k < util_video_decoder_max_raw_image[session][i]; k++)
				av_frame_free(&util_video_decoder_raw_image[session][i][k]);
		}
	}
}

static void Util_decoder_mvd_exit(uint8_t session)
{
	if(!util_mvd_video_decoder_init)
		return;

	util_mvd_video_decoder_init = false;
	mvdstdExit();
	free(util_mvd_video_decoder_packet);
	util_mvd_video_decoder_packet = NULL;
	util_mvd_video_decoder_available_raw_image[session] = 0;
	util_mvd_video_decoder_raw_image_ready_index[session] = 0;
	util_mvd_video_decoder_raw_image_current_index[session] = 0;
	for(uint16_t i = 0; i < util_mvd_video_decoder_max_raw_image[session]; i++)
	{
		if(util_mvd_video_decoder_raw_image[session][i])
		{
			free(util_mvd_video_decoder_raw_image[session][i]->data[0]);
			for(uint8_t k = 0; k < AV_NUM_DATA_POINTERS; k++)
				util_mvd_video_decoder_raw_image[session][i]->data[k] = NULL;
		}
		av_frame_free(&util_mvd_video_decoder_raw_image[session][i]);
	}
}

static void Util_decoder_subtitle_exit(uint8_t session)
{
	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
	{
		if(util_subtitle_decoder_init[session][i])
		{
			util_subtitle_decoder_init[session][i] = false;
			util_subtitle_decoder_packet_ready[session][i] = false;
			util_subtitle_decoder_cache_packet_ready[session][i] = false;
			avcodec_free_context(&util_subtitle_decoder_context[session][i]);
			av_packet_free(&util_subtitle_decoder_packet[session][i]);
			av_packet_free(&util_subtitle_decoder_cache_packet[session][i]);
			if(util_subtitle_decoder_raw_data[session][i])
			{
				avsubtitle_free(util_subtitle_decoder_raw_data[session][i]);
				free(util_subtitle_decoder_raw_data[session][i]);
				util_subtitle_decoder_raw_data[session][i] = NULL;
			}
		}
	}
}
#endif //DEF_DECODER_VIDEO_AUDIO_API_ENABLE

#if DEF_DECODER_IMAGE_API_ENABLE
uint32_t Util_decoder_image_decode(const char* path, uint8_t** raw_data, uint32_t* width, uint32_t* height, Raw_pixel* format)
{
	//We can't get rid of this "int" because library uses "int" type as args.
	int ch = 0, w = 0, h = 0;

	if(!path || !raw_data || !width || !height || !format)
		goto invalid_arg;

	*raw_data = stbi_load(path, &w, &h, &ch, STBI_default);
	if(!*raw_data)
	{
		DEF_LOG_RESULT(stbi_load, false, DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS);
		DEF_LOG_STRING(stbi_failure_reason());
		goto stbi_api_failed;
	}
	*width = w;
	*height = h;

	if(ch == 4)
		*format = RAW_PIXEL_RGBA8888;
	else if(ch == 3)
		*format = RAW_PIXEL_RGB888;
	else if(ch == 2)
		*format = RAW_PIXEL_GRAYALPHA88;
	else
		*format = RAW_PIXEL_GRAY8;

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	stbi_api_failed:
	return DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_decoder_image_decode_data(const uint8_t* compressed_data, uint32_t compressed_buffer_size, uint8_t** raw_data, uint32_t* width, uint32_t* height, Raw_pixel* format)
{
	//We can't get rid of this "int" because library uses "int" type as args.
	int ch = 0, w = 0, h = 0;

	if(!compressed_data || compressed_buffer_size == 0 || !raw_data || !width || !height || !format)
		goto invalid_arg;

	*raw_data = stbi_load_from_memory(compressed_data, compressed_buffer_size, &w, &h, &ch, STBI_default);
	if(!*raw_data)
	{
		DEF_LOG_RESULT(stbi_load_from_memory, false, DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS);
		DEF_LOG_STRING(stbi_failure_reason());
		goto stbi_api_failed;
	}
	*width = w;
	*height = h;

	if(ch == 4)
		*format = RAW_PIXEL_RGBA8888;
	else if(ch == 3)
		*format = RAW_PIXEL_RGB888;
	else if(ch == 2)
		*format = RAW_PIXEL_GRAYALPHA88;
	else
		*format = RAW_PIXEL_GRAY8;

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	stbi_api_failed:
	return DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS;
}
#endif //DEF_DECODER_IMAGE_API_ENABLE
