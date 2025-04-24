//Includes.
#include "system/util/encoder.h"

#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/media_types.h"
#include "system/util/util.h"

#if DEF_ENCODER_VIDEO_AUDIO_API_ENABLE
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#endif //DEF_ENCODER_VIDEO_AUDIO_API_ENABLE

#if DEF_ENCODER_IMAGE_API_ENABLE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif //DEF_ENCODER_IMAGE_API_ENABLE

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
#if DEF_ENCODER_VIDEO_AUDIO_API_ENABLE
static void Util_encoder_audio_exit(uint8_t session);
static void Util_encoder_video_exit(uint8_t session);
#endif //DEF_ENCODER_VIDEO_AUDIO_API_ENABLE

//Variables.
#if DEF_ENCODER_VIDEO_AUDIO_API_ENABLE
static bool util_audio_encoder_init[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static uint8_t util_audio_stream_index[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static uint8_t* util_audio_encoder_cache[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static uint32_t util_audio_encoder_cache_size[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static int64_t util_audio_pos[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static int64_t util_audio_increase_pts[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static double util_audio_encoder_conversion_size_rate[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVPacket* util_audio_encoder_packet[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVFrame* util_audio_encoder_raw_data[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVCodecContext* util_audio_encoder_context[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static const AVCodec* util_audio_encoder_codec[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static SwrContext* util_audio_encoder_swr_context[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVStream* util_audio_encoder_stream[DEF_ENCODER_MAX_SESSIONS] = { 0, };

static bool util_video_encoder_init[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static uint8_t util_video_stream_index[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static int64_t util_video_pos[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static int64_t util_video_increase_pts[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVPacket* util_video_encoder_packet[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVFrame* util_video_encoder_raw_data[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVCodecContext* util_video_encoder_context[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static const AVCodec* util_video_encoder_codec[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVStream* util_video_encoder_stream[DEF_ENCODER_MAX_SESSIONS] = { 0, };

static bool util_encoder_created_file[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static bool util_encoder_wrote_header[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static uint8_t util_encoder_next_stream_index[DEF_ENCODER_MAX_SESSIONS] = { 0, };
static AVFormatContext* util_encoder_format_context[DEF_ENCODER_MAX_SESSIONS] = { 0, };
#endif //DEF_ENCODER_VIDEO_AUDIO_API_ENABLE

//Code.
#if DEF_ENCODER_VIDEO_AUDIO_API_ENABLE
uint32_t Util_encoder_create_output_file(const char* path, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(session >= DEF_ENCODER_MAX_SESSIONS || !path)
		goto invalid_arg;

	if(util_encoder_created_file[session])
		goto already_inited;

	util_encoder_format_context[session] = avformat_alloc_context();
	if(!util_encoder_format_context[session])
	{
		DEF_LOG_RESULT(avformat_alloc_context, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed_0;
	}

	util_encoder_format_context[session]->oformat = av_guess_format(NULL, path, NULL);
	if(!util_encoder_format_context[session]->oformat)
	{
		DEF_LOG_RESULT(av_guess_format, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed_0;
	}

	ffmpeg_result = avio_open(&util_encoder_format_context[session]->pb, path, AVIO_FLAG_READ_WRITE);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avio_open, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_encoder_created_file[session] = true;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	avio_close(util_encoder_format_context[session]->pb);
	//Fallthrough.

	ffmpeg_api_failed_0:
	avformat_free_context(util_encoder_format_context[session]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_encoder_audio_init(Media_a_codec codec, uint32_t original_sample_rate, uint32_t encode_sample_rate, uint32_t bitrate, uint8_t session)
{
	int32_t ffmpeg_result = 0;
	enum AVCodecID codec_id = AV_CODEC_ID_NONE;

	if(session >= DEF_ENCODER_MAX_SESSIONS || original_sample_rate == 0 || encode_sample_rate == 0
	|| bitrate == 0 || codec <= MEDIA_A_CODEC_INVALID || codec >= MEDIA_A_CODEC_MAX)
		goto invalid_arg;

	if(!util_encoder_created_file[session])
		goto not_inited;

	if(util_audio_encoder_init[session] || util_encoder_wrote_header[session])
		goto already_inited;

	if(codec == MEDIA_A_CODEC_AAC)
		codec_id = AV_CODEC_ID_AAC;
	else if(codec == MEDIA_A_CODEC_AC3)
		codec_id = AV_CODEC_ID_AC3;
	else if(codec == MEDIA_A_CODEC_MP2)
		codec_id = AV_CODEC_ID_MP2;
	else if(codec == MEDIA_A_CODEC_MP3)
		codec_id = AV_CODEC_ID_MP3;

	util_audio_pos[session] = 0;
	util_audio_increase_pts[session] = 0;

	util_audio_encoder_codec[session] = avcodec_find_encoder(codec_id);
	if(!util_audio_encoder_codec[session])
	{
		DEF_LOG_RESULT(avcodec_find_encoder, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_audio_encoder_context[session] = avcodec_alloc_context3(util_audio_encoder_codec[session]);
	if(!util_audio_encoder_context[session])
	{
		DEF_LOG_RESULT(avcodec_alloc_context3, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	if(codec_id == AV_CODEC_ID_MP2)
		util_audio_encoder_context[session]->sample_fmt = AV_SAMPLE_FMT_S16;
	else if(codec_id == AV_CODEC_ID_MP3)
		util_audio_encoder_context[session]->sample_fmt = AV_SAMPLE_FMT_S16P;
	else
		util_audio_encoder_context[session]->sample_fmt = AV_SAMPLE_FMT_FLT;

	util_audio_encoder_context[session]->bit_rate = bitrate;
	util_audio_encoder_context[session]->sample_rate = encode_sample_rate;
	util_audio_encoder_context[session]->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
	util_audio_encoder_context[session]->codec_type = AVMEDIA_TYPE_AUDIO;
	util_audio_encoder_context[session]->time_base = (AVRational){ 1, (int32_t)encode_sample_rate };
	if(codec_id == AV_CODEC_ID_AAC)
		util_audio_encoder_context[session]->profile = FF_PROFILE_AAC_LOW;

	ffmpeg_result = avcodec_open2(util_audio_encoder_context[session], util_audio_encoder_codec[session], NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_open2, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_audio_increase_pts[session] = util_audio_encoder_context[session]->frame_size;
	util_audio_encoder_packet[session] = av_packet_alloc();
	util_audio_encoder_raw_data[session] = av_frame_alloc();
	if(!util_audio_encoder_raw_data[session] || !util_audio_encoder_packet[session])
	{
		if(!util_audio_encoder_packet[session])
			DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		if(!util_audio_encoder_raw_data[session])
			DEF_LOG_RESULT(av_frame_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);

		goto ffmpeg_api_failed;
	}

	util_audio_encoder_raw_data[session]->nb_samples = util_audio_encoder_context[session]->frame_size;
	util_audio_encoder_raw_data[session]->format = util_audio_encoder_context[session]->sample_fmt;

	av_channel_layout_copy(&util_audio_encoder_raw_data[session]->ch_layout, &util_audio_encoder_context[session]->ch_layout);

	ffmpeg_result = av_frame_get_buffer(util_audio_encoder_raw_data[session], 0);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(av_frame_get_buffer, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_frame_make_writable(util_audio_encoder_raw_data[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(av_frame_make_writable, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = swr_alloc_set_opts2(&util_audio_encoder_swr_context[session], &util_audio_encoder_context[session]->ch_layout, util_audio_encoder_context[session]->sample_fmt, util_audio_encoder_context[session]->sample_rate,
	&util_audio_encoder_context[session]->ch_layout, AV_SAMPLE_FMT_S16, original_sample_rate, 0, NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(swr_alloc_set_opts2, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = swr_init(util_audio_encoder_swr_context[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(swr_init, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}
	util_audio_encoder_conversion_size_rate[session] = (double)encode_sample_rate / original_sample_rate;
	util_audio_encoder_conversion_size_rate[session] += 0.1;

	util_audio_encoder_stream[session] = avformat_new_stream(util_encoder_format_context[session], util_audio_encoder_codec[session]);
	if(!util_audio_encoder_stream[session])
	{
		DEF_LOG_RESULT(avformat_new_stream, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	if (util_encoder_format_context[session]->oformat->flags & AVFMT_GLOBALHEADER)
		util_encoder_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ffmpeg_result = avcodec_parameters_from_context(util_audio_encoder_stream[session]->codecpar, util_audio_encoder_context[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_parameters_from_context, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_audio_stream_index[session] = util_encoder_next_stream_index[session];
	util_encoder_next_stream_index[session]++;
	util_audio_encoder_cache_size[session] = 0;

	util_audio_encoder_init[session] = true;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	Util_encoder_audio_exit(session);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_encoder_video_init(Media_v_codec codec, uint32_t width, uint32_t height, uint32_t bitrate, uint32_t fps, uint8_t session)
{
	int32_t ffmpeg_result = 0;
	enum AVCodecID video_codec = AV_CODEC_ID_NONE;

	if(session >= DEF_ENCODER_MAX_SESSIONS || width == 0 || height == 0
	|| codec <= MEDIA_V_CODEC_INVALID || codec >= MEDIA_V_CODEC_MAX)
		goto invalid_arg;

	if(!util_encoder_created_file[session])
		goto not_inited;

	if(util_video_encoder_init[session] || util_encoder_wrote_header[session])
		goto already_inited;

	if(codec == MEDIA_V_CODEC_MJPEG)
		video_codec = AV_CODEC_ID_MJPEG;
	else if(codec == MEDIA_V_CODEC_H264)
		video_codec = AV_CODEC_ID_H264;
	else if(codec == MEDIA_V_CODEC_MPEG4)
		video_codec = AV_CODEC_ID_MPEG4;
	else if(codec == MEDIA_V_CODEC_MPEG2VIDEO)
		video_codec = AV_CODEC_ID_MPEG2VIDEO;

	util_video_pos[session] = 0;
	util_video_increase_pts[session] = 0;
	util_video_encoder_codec[session] = avcodec_find_encoder(video_codec);
	if(!util_video_encoder_codec[session])
	{
		DEF_LOG_RESULT(avcodec_find_encoder, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_video_encoder_context[session] = avcodec_alloc_context3(util_video_encoder_codec[session]);
	if(!util_video_encoder_context[session])
	{
		DEF_LOG_RESULT(avcodec_alloc_context3, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_video_encoder_context[session]->bit_rate = bitrate;
	util_video_encoder_context[session]->width = width;
	util_video_encoder_context[session]->height = height;
	util_video_encoder_context[session]->time_base = (AVRational){ 1, (int32_t)fps };
	util_video_encoder_context[session]->gop_size = fps;
	if(video_codec == AV_CODEC_ID_MJPEG)
		util_video_encoder_context[session]->pix_fmt = AV_PIX_FMT_YUVJ420P;
	else
		util_video_encoder_context[session]->pix_fmt = AV_PIX_FMT_YUV420P;

	if(video_codec == AV_CODEC_ID_H264)
	{
		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "crf", "32", 0);
		if(ffmpeg_result < 0)
		{
			DEF_LOG_RESULT(av_opt_set, false, ffmpeg_result);
			DEF_LOG_STRING("crf : 32");
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "profile", "baseline", 0);
		if(ffmpeg_result < 0)
		{
			DEF_LOG_RESULT(av_opt_set, false, ffmpeg_result);
			DEF_LOG_STRING("profile : baseline");
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "preset", "ultrafast", 0);
		if(ffmpeg_result < 0)
		{
			DEF_LOG_RESULT(av_opt_set, false, ffmpeg_result);
			DEF_LOG_STRING("preset : ultrafast");
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "me_method", "dia", 0);
		if(ffmpeg_result < 0)
		{
			DEF_LOG_RESULT(av_opt_set, false, ffmpeg_result);
			DEF_LOG_STRING("me_method : dia");
			goto ffmpeg_api_failed;
		}
	}

	ffmpeg_result = avcodec_open2(util_video_encoder_context[session], util_video_encoder_codec[session], NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_open2, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_video_increase_pts[session] = 90000 / fps;

	util_video_encoder_packet[session] = av_packet_alloc();
	util_video_encoder_raw_data[session] = av_frame_alloc();
	if(!util_video_encoder_raw_data[session] || !util_video_encoder_packet[session])
	{
		if(!util_video_encoder_packet[session])
			DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		if(!util_video_encoder_raw_data[session])
			DEF_LOG_RESULT(av_frame_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);

		goto ffmpeg_api_failed;
	}

	util_video_encoder_raw_data[session]->format = util_video_encoder_context[session]->pix_fmt;
	util_video_encoder_raw_data[session]->width = util_video_encoder_context[session]->width;
	util_video_encoder_raw_data[session]->height = util_video_encoder_context[session]->height;
	ffmpeg_result = av_frame_get_buffer(util_video_encoder_raw_data[session], 0);
	if(ffmpeg_result < 0)
	{
		DEF_LOG_RESULT(av_frame_get_buffer, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_frame_make_writable(util_video_encoder_raw_data[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(av_frame_make_writable, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_video_encoder_stream[session] = avformat_new_stream(util_encoder_format_context[session], util_video_encoder_codec[session]);
	if(!util_video_encoder_stream[session])
	{
		DEF_LOG_RESULT(avformat_new_stream, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_from_context(util_video_encoder_stream[session]->codecpar, util_video_encoder_context[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_parameters_from_context, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_video_stream_index[session] = util_encoder_next_stream_index[session];
	util_encoder_next_stream_index[session]++;

	util_video_encoder_init[session] = true;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	Util_encoder_video_exit(session);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_encoder_write_header(uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(session >= DEF_ENCODER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_encoder_wrote_header[session])
		goto already_inited;

	if(!util_encoder_created_file[session])
		goto not_inited;

	util_encoder_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ffmpeg_result = avformat_write_header(util_encoder_format_context[session], NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avformat_write_header, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_encoder_wrote_header[session] = true;

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	ffmpeg_api_failed:
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_encoder_audio_encode(uint32_t size, const uint8_t* raw_data, uint8_t session)
{
	uint8_t bytes_per_sample = 0;
	const uint8_t* swr_in_cache[1] = { NULL, };
	uint8_t* swr_out_cache[1] = { NULL, };
	uint8_t* raw_audio = NULL;
	int32_t out_samples = 0;
	int32_t ffmpeg_result = 0;
	uint32_t encode_offset = 0;
	uint32_t one_frame_size = 0;
	uint32_t max_out_samples = 0;
	uint32_t out_size = 0;
	uint32_t in_samples = 0;

	if(session >= DEF_ENCODER_MAX_SESSIONS || size == 0 || !raw_data)
		goto invalid_arg;

	if(!util_audio_encoder_init[session] || !util_encoder_wrote_header[session])
		goto not_inited;

	one_frame_size = av_samples_get_buffer_size(NULL, util_audio_encoder_context[session]->ch_layout.nb_channels, util_audio_encoder_context[session]->frame_size, util_audio_encoder_context[session]->sample_fmt, 0);
	bytes_per_sample = av_get_bytes_per_sample(util_audio_encoder_context[session]->sample_fmt);

	in_samples = size / 2;
	swr_in_cache[0] = raw_data;
	max_out_samples = in_samples * util_audio_encoder_conversion_size_rate[session];
	raw_audio = (uint8_t*)linearAlloc((max_out_samples * bytes_per_sample) + util_audio_encoder_cache_size[session]);
	if(!raw_audio)
		goto out_of_memory;

	if(util_audio_encoder_cache_size[session] > 0)
	{
		memcpy(raw_audio, util_audio_encoder_cache[session], util_audio_encoder_cache_size[session]);
		swr_out_cache[0] = (raw_audio + util_audio_encoder_cache_size[session]);
	}
	else
		swr_out_cache[0] = raw_audio;

	out_samples = swr_convert(util_audio_encoder_swr_context[session], swr_out_cache, max_out_samples, (const uint8_t* const*)swr_in_cache, in_samples);
	if(out_samples < 0)
	{
		DEF_LOG_RESULT(swr_convert, false, out_samples);
		goto ffmpeg_api_failed;
	}
	out_size = out_samples * bytes_per_sample;
	out_size += util_audio_encoder_cache_size[session];

	while(true)
	{
		memcpy(util_audio_encoder_raw_data[session]->data[0], raw_audio + encode_offset, one_frame_size);
		//Set pts.
		util_audio_encoder_raw_data[session]->pts = util_audio_pos[session];
		util_audio_pos[session] += util_audio_increase_pts[session];

		ffmpeg_result = avcodec_send_frame(util_audio_encoder_context[session], util_audio_encoder_raw_data[session]);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avcodec_send_frame, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_receive_packet(util_audio_encoder_context[session], util_audio_encoder_packet[session]);
		if(ffmpeg_result == 0)
		{
			util_audio_encoder_packet[session]->stream_index = util_audio_stream_index[session];
			ffmpeg_result = av_interleaved_write_frame(util_encoder_format_context[session], util_audio_encoder_packet[session]);
			av_packet_unref(util_audio_encoder_packet[session]);
			if(ffmpeg_result != 0)
			{
				DEF_LOG_RESULT(av_interleaved_write_frame, false, ffmpeg_result);
				goto ffmpeg_api_failed;
			}
		}
		else
		{
			// DEF_LOG_RESULT(avcodec_receive_packet, false, ffmpeg_result);
			av_packet_unref(util_audio_encoder_packet[session]);
		}

		out_size -= one_frame_size;
		encode_offset += one_frame_size;
		if(one_frame_size > out_size)
			break;
	}

	free(util_audio_encoder_cache[session]);
	util_audio_encoder_cache[session] = NULL;
	util_audio_encoder_cache[session] = (uint8_t*)linearAlloc(out_size);
	if(!util_audio_encoder_cache[session])
		goto out_of_memory;

	memcpy(util_audio_encoder_cache[session], raw_audio + encode_offset, out_size);
	util_audio_encoder_cache_size[session] = out_size;

	free(raw_audio);
	raw_audio = NULL;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	out_of_memory:
	util_audio_encoder_cache_size[session] = 0;
	free(util_audio_encoder_cache[session]);
	free(raw_audio);
	util_audio_encoder_cache[session] = NULL;
	raw_audio = NULL;
	return DEF_ERR_OUT_OF_MEMORY;

	ffmpeg_api_failed:
	util_audio_encoder_cache_size[session] = 0;
	free(util_audio_encoder_cache[session]);
	free(raw_audio);
	util_audio_encoder_cache[session] = NULL;
	raw_audio = NULL;
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

uint32_t Util_encoder_video_encode(const uint8_t* raw_data, uint8_t session)
{
	int32_t ffmpeg_result = 0;
	uint32_t width = 0;
	uint32_t height = 0;

	if(session >= DEF_ENCODER_MAX_SESSIONS || !raw_data)
		goto invalid_arg;

	if(!util_video_encoder_init[session] || !util_encoder_wrote_header[session])
		goto not_inited;

	width = util_video_encoder_raw_data[session]->width;
	height = util_video_encoder_raw_data[session]->height;

	memcpy(util_video_encoder_raw_data[session]->data[0], raw_data, width * height);
	memcpy(util_video_encoder_raw_data[session]->data[1], raw_data + width * height, width * height / 4);
	memcpy(util_video_encoder_raw_data[session]->data[2], raw_data + width * height + width * height / 4, width * height / 4);
	util_video_encoder_raw_data[session]->linesize[0] = width;
	util_video_encoder_raw_data[session]->linesize[1] = width / 2;
	util_video_encoder_raw_data[session]->linesize[2] = width / 2;

	//Set pts.
	util_video_encoder_raw_data[session]->pts = util_video_pos[session];
	util_video_pos[session] += util_video_increase_pts[session];

	ffmpeg_result = avcodec_send_frame(util_video_encoder_context[session], util_video_encoder_raw_data[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_send_frame, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_receive_packet(util_video_encoder_context[session], util_video_encoder_packet[session]);
	if(ffmpeg_result == 0)
	{
		util_video_encoder_packet[session]->stream_index = util_video_stream_index[session];
		ffmpeg_result = av_interleaved_write_frame(util_encoder_format_context[session], util_video_encoder_packet[session]);
		av_packet_unref(util_video_encoder_packet[session]);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(av_interleaved_write_frame, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}
	}
	else
	{
		// DEF_LOG_RESULT(avcodec_receive_packet, false, ffmpeg_result);
		av_packet_unref(util_video_encoder_packet[session]);
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	ffmpeg_api_failed:
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

void Util_encoder_close_output_file(uint8_t session)
{
	if(session >= DEF_ENCODER_MAX_SESSIONS)
		return;

	if(util_encoder_created_file[session])
	{
		Util_encoder_audio_exit(session);
		Util_encoder_video_exit(session);

		if(util_encoder_wrote_header[session])
			av_write_trailer(util_encoder_format_context[session]);

		avio_close(util_encoder_format_context[session]->pb);
		avformat_free_context(util_encoder_format_context[session]);
		util_encoder_created_file[session] = false;
		util_encoder_wrote_header[session] = false;
		util_encoder_next_stream_index[session] = 0;
	}
}

static void Util_encoder_audio_exit(uint8_t session)
{
	if(util_audio_encoder_init[session])
	{
		avcodec_free_context(&util_audio_encoder_context[session]);
		av_packet_free(&util_audio_encoder_packet[session]);
		av_frame_free(&util_audio_encoder_raw_data[session]);
		swr_free(&util_audio_encoder_swr_context[session]);
		free(util_audio_encoder_cache[session]);
		util_audio_encoder_cache[session] = NULL;
	}
	util_audio_encoder_init[session] = false;
}

static void Util_encoder_video_exit(uint8_t session)
{
	if(util_video_encoder_init[session])
	{
		avcodec_free_context(&util_video_encoder_context[session]);
		av_packet_free(&util_video_encoder_packet[session]);
		av_frame_free(&util_video_encoder_raw_data[session]);
	}
	util_video_encoder_init[session] = false;
}
#endif //DEF_ENCODER_VIDEO_AUDIO_API_ENABLE

#if DEF_ENCODER_IMAGE_API_ENABLE
uint32_t Util_encoder_image_encode(const char* path, const uint8_t* raw_data, uint32_t width, uint32_t height, Media_i_codec codec, uint8_t quality)
{
	int32_t stbi_result = 0;

	if(!path || !raw_data || width == 0 || height == 0 || codec <= MEDIA_I_CODEC_INVALID
	|| codec >= MEDIA_I_CODEC_MAX || (codec == MEDIA_I_CODEC_JPG && (quality < 1 || quality > 100)))
		goto invalid_arg;

	if(codec == MEDIA_I_CODEC_PNG)
		stbi_result = stbi_write_png(path, width, height, 3, raw_data, 0);
	else if(codec == MEDIA_I_CODEC_JPG)
		stbi_result = stbi_write_jpg(path, width, height, 3, raw_data, quality);
	else if(codec == MEDIA_I_CODEC_BMP)
		stbi_result = stbi_write_bmp(path, width, height, 3, raw_data);
	else if(codec == MEDIA_I_CODEC_TGA)
		stbi_result = stbi_write_tga(path, width, height, 3, raw_data);

	if(stbi_result == 0)
	{
		if(codec == MEDIA_I_CODEC_PNG)
			DEF_LOG_RESULT(stbi_write_png, false, stbi_result);
		else if(codec == MEDIA_I_CODEC_JPG)
			DEF_LOG_RESULT(stbi_write_jpg, false, stbi_result);
		else if(codec == MEDIA_I_CODEC_BMP)
			DEF_LOG_RESULT(stbi_write_bmp, false, stbi_result);
		else if(codec == MEDIA_I_CODEC_TGA)
			DEF_LOG_RESULT(stbi_write_tga, false, stbi_result);

		goto stbi_api_failed;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	stbi_api_failed:
	return DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS;
}
#endif //DEF_ENCODER_IMAGE_API_ENABLE
