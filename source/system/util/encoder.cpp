#include "system/headers.hpp"

extern "C" 
{
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
}

#include "stb_image/stb_image_write.h"

extern "C" void memcpy_asm(u8*, u8*, int);

bool util_audio_encoder_init[DEF_ENCODER_MAX_SESSIONS];
int util_audio_stream_index[DEF_ENCODER_MAX_SESSIONS];
int util_audio_pos[DEF_ENCODER_MAX_SESSIONS];
int util_audio_increase_pts[DEF_ENCODER_MAX_SESSIONS];
int util_audio_encoder_cache_size[DEF_ENCODER_MAX_SESSIONS];
double util_audio_encoder_conversion_size_rate[DEF_ENCODER_MAX_SESSIONS];
u8* util_audio_encoder_cache[DEF_ENCODER_MAX_SESSIONS];
AVPacket* util_audio_encoder_packet[DEF_ENCODER_MAX_SESSIONS];
AVFrame* util_audio_encoder_raw_data[DEF_ENCODER_MAX_SESSIONS];
AVCodecContext* util_audio_encoder_context[DEF_ENCODER_MAX_SESSIONS];
const AVCodec* util_audio_encoder_codec[DEF_ENCODER_MAX_SESSIONS];
SwrContext* util_audio_encoder_swr_context[DEF_ENCODER_MAX_SESSIONS];
AVStream* util_audio_encoder_stream[DEF_ENCODER_MAX_SESSIONS];

bool util_video_encoder_init[DEF_ENCODER_MAX_SESSIONS];
int util_video_stream_index[DEF_ENCODER_MAX_SESSIONS];
int util_video_pos[DEF_ENCODER_MAX_SESSIONS];
int util_video_increase_pts[DEF_ENCODER_MAX_SESSIONS];
AVPacket* util_video_encoder_packet[DEF_ENCODER_MAX_SESSIONS];
AVFrame* util_video_encoder_raw_data[DEF_ENCODER_MAX_SESSIONS];
AVCodecContext* util_video_encoder_context[DEF_ENCODER_MAX_SESSIONS];
const AVCodec* util_video_encoder_codec[DEF_ENCODER_MAX_SESSIONS];
AVStream* util_video_encoder_stream[DEF_ENCODER_MAX_SESSIONS];

bool util_encoder_created_file[DEF_ENCODER_MAX_SESSIONS];
bool util_encoder_wrote_header[DEF_ENCODER_MAX_SESSIONS];
bool util_encoder_init = false;
int util_encoder_next_stream_index[DEF_ENCODER_MAX_SESSIONS];
AVFormatContext* util_encoder_format_context[DEF_ENCODER_MAX_SESSIONS];


void Util_audio_encoder_exit(int session);
void Util_video_encoder_exit(int session);

void Util_encoder_init_variables(void)
{
	if(util_encoder_init)
		return;
	
	for(int i = 0; i < DEF_ENCODER_MAX_SESSIONS; i++)
	{
		util_audio_encoder_init[i] = false;
		util_audio_stream_index[i] = 0;
		util_audio_pos[i] = 0;
		util_audio_increase_pts[i] = 0;
		util_audio_encoder_cache_size[i] = 0;
		util_audio_encoder_conversion_size_rate[i] = 0;
		util_audio_encoder_cache[i] = NULL;
		util_audio_encoder_packet[i] = NULL;
		util_audio_encoder_raw_data[i] = NULL;
		util_audio_encoder_context[i] = NULL;
		util_audio_encoder_codec[i] = NULL;
		util_audio_encoder_swr_context[i] = NULL;
		util_audio_encoder_stream[i] = NULL;

		util_video_encoder_init[i] = false;
		util_video_stream_index[i] = 0;
		util_video_pos[i] = 0;
		util_video_increase_pts[i] = 0;
		util_video_encoder_packet[i] = NULL;
		util_video_encoder_raw_data[i] = NULL;
		util_video_encoder_context[i] = NULL;
		util_video_encoder_codec[i] = NULL;
		util_video_encoder_stream[i] = NULL;

		util_encoder_created_file[i] = false;
		util_encoder_wrote_header[i] = false;
		util_encoder_next_stream_index[i] = 0;
		util_encoder_format_context[i] = NULL;
	}

	util_encoder_init = true;
	return;
}

Result_with_string Util_encoder_create_output_file(std::string file_path, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	if(!util_encoder_init)
		Util_encoder_init_variables();

	if(session < 0 || session >= DEF_ENCODER_MAX_SESSIONS || file_path == "")
		goto invalid_arg;
	
	if(util_encoder_created_file[session])
		goto already_inited;

	util_encoder_format_context[session] = avformat_alloc_context();
	if(!util_encoder_format_context[session])
	{
		result.error_description = "[Error] avformat_alloc_context() failed. ";
		goto ffmpeg_api_failed_0;
	}

	util_encoder_format_context[session]->oformat = av_guess_format(NULL, file_path.c_str(), NULL);
	if(!util_encoder_format_context[session]->oformat)
	{
		result.error_description = "[Error] av_guess_format() failed. ";
		goto ffmpeg_api_failed_0;
	}

	ffmpeg_result = avio_open(&util_encoder_format_context[session]->pb, file_path.c_str(), AVIO_FLAG_READ_WRITE);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avio_open() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_encoder_created_file[session] = true;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;
	
	ffmpeg_api_failed:
	avio_close(util_encoder_format_context[session]->pb);

	ffmpeg_api_failed_0:
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	avformat_free_context(util_encoder_format_context[session]);
	return result;
}

Result_with_string Util_audio_encoder_init(int codec, int original_sample_rate, int encode_sample_rate, int bitrate, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;
	AVCodecID codec_id = AV_CODEC_ID_NONE;
	
	if(!util_encoder_init)
		Util_encoder_init_variables();

	if(session < 0 || session >= DEF_ENCODER_MAX_SESSIONS || original_sample_rate <= 0 || encode_sample_rate <= 0 || bitrate <= 0
	|| (codec != DEF_ENCODER_AUDIO_CODEC_AAC && codec != DEF_ENCODER_AUDIO_CODEC_AC3 && codec != DEF_ENCODER_AUDIO_CODEC_MP2 && codec != DEF_ENCODER_AUDIO_CODEC_MP3))
		goto invalid_arg;

	if(!util_encoder_created_file[session])
		goto not_inited;

	if(util_audio_encoder_init[session] || util_encoder_wrote_header[session])
		goto already_inited;
	
	if(codec == DEF_ENCODER_AUDIO_CODEC_AAC)
		codec_id = AV_CODEC_ID_AAC;
	else if(codec == DEF_ENCODER_AUDIO_CODEC_AC3)
		codec_id = AV_CODEC_ID_AC3;
	else if(codec == DEF_ENCODER_AUDIO_CODEC_MP2)
		codec_id = AV_CODEC_ID_MP2;
	else if(codec == DEF_ENCODER_AUDIO_CODEC_MP3)
		codec_id = AV_CODEC_ID_MP3;

	util_audio_pos[session] = 0;
	util_audio_increase_pts[session] = 0;

	util_audio_encoder_codec[session] = avcodec_find_encoder(codec_id);
	if(!util_audio_encoder_codec[session])
	{
		result.error_description = "[Error] avcodec_find_encoder() failed. ";
		goto ffmpeg_api_failed;
	}

	util_audio_encoder_context[session] = avcodec_alloc_context3(util_audio_encoder_codec[session]);
	if(!util_audio_encoder_context[session])
	{
		result.error_description = "[Error] avcodec_alloc_context3() failed. ";
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
	util_audio_encoder_context[session]->channel_layout = AV_CH_LAYOUT_MONO;
	util_audio_encoder_context[session]->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);
	util_audio_encoder_context[session]->codec_type = AVMEDIA_TYPE_AUDIO;
	util_audio_encoder_context[session]->time_base = (AVRational){ 1, encode_sample_rate };
	if(codec_id == AV_CODEC_ID_AAC)
		util_audio_encoder_context[session]->profile = FF_PROFILE_AAC_LOW;

	ffmpeg_result = avcodec_open2(util_audio_encoder_context[session], util_audio_encoder_codec[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_open2() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}
	
	util_audio_increase_pts[session] = util_audio_encoder_context[session]->frame_size;
	util_audio_encoder_packet[session] = av_packet_alloc();
	util_audio_encoder_raw_data[session] = av_frame_alloc();
	if(!util_audio_encoder_raw_data[session] || !util_audio_encoder_packet)
	{
		result.error_description = "[Error] av_packet_alloc() / av_frame_alloc() failed. ";
		goto ffmpeg_api_failed;
	}
	
	util_audio_encoder_raw_data[session]->nb_samples = util_audio_encoder_context[session]->frame_size;
	util_audio_encoder_raw_data[session]->format = util_audio_encoder_context[session]->sample_fmt;
	util_audio_encoder_raw_data[session]->channel_layout = util_audio_encoder_context[session]->channel_layout;

	ffmpeg_result = av_frame_get_buffer(util_audio_encoder_raw_data[session], 0);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] av_frame_get_buffer() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_frame_make_writable(util_audio_encoder_raw_data[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] av_frame_make_writable() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_audio_encoder_swr_context[session] = swr_alloc_set_opts(NULL, av_get_default_channel_layout(util_audio_encoder_context[session]->channels), util_audio_encoder_context[session]->sample_fmt, util_audio_encoder_context[session]->sample_rate,
	av_get_default_channel_layout(util_audio_encoder_context[session]->channels), AV_SAMPLE_FMT_S16, original_sample_rate, 0, NULL);
	if(!util_audio_encoder_swr_context[session])
	{
		result.error_description = "[Error] swr_alloc_set_opts() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = swr_init(util_audio_encoder_swr_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] swr_init() failed. ";
		goto ffmpeg_api_failed;
	}
	util_audio_encoder_conversion_size_rate[session] = (double)encode_sample_rate / original_sample_rate;
	util_audio_encoder_conversion_size_rate[session] += 0.1;

	util_audio_encoder_stream[session] = avformat_new_stream(util_encoder_format_context[session], util_audio_encoder_codec[session]);
	if(!util_audio_encoder_stream[session])
	{
		result.error_description = "[Error] avformat_new_stream() failed. ";
		goto ffmpeg_api_failed;
	}

	if (util_encoder_format_context[session]->oformat->flags & AVFMT_GLOBALHEADER)
		util_encoder_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ffmpeg_result = avcodec_parameters_from_context(util_audio_encoder_stream[session]->codecpar, util_audio_encoder_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_parameters_from_context() failed. ";
		goto ffmpeg_api_failed;
	}

	util_audio_stream_index[session] = util_encoder_next_stream_index[session];
	util_encoder_next_stream_index[session]++;
	util_audio_encoder_cache_size[session] = 0;

	util_audio_encoder_init[session] = true;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;
	
	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;
	
	ffmpeg_api_failed:
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	Util_audio_encoder_exit(session);
	return result;
}

Result_with_string Util_video_encoder_init(int codec, int width, int height, int bitrate, int fps, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;
	AVCodecID video_codec = AV_CODEC_ID_NONE;

	if(!util_encoder_init)
		Util_encoder_init_variables();

	if(session < 0 || session >= DEF_ENCODER_MAX_SESSIONS || width <= 0 || height <= 0
	|| (codec != DEF_ENCODER_VIDEO_CODEC_MJPEG && codec != DEF_ENCODER_VIDEO_CODEC_H264 
	&& codec != DEF_ENCODER_VIDEO_CODEC_MPEG4 && codec != DEF_ENCODER_VIDEO_CODEC_MPEG2VIDEO))
		goto invalid_arg;

	if(!util_encoder_created_file[session])
		goto not_inited;

	if(util_video_encoder_init[session] || util_encoder_wrote_header[session])
		goto already_inited;

	if(codec == DEF_ENCODER_VIDEO_CODEC_MJPEG)
		video_codec = AV_CODEC_ID_MJPEG;
	else if(codec == DEF_ENCODER_VIDEO_CODEC_H264)
		video_codec = AV_CODEC_ID_H264;
	else if(codec == DEF_ENCODER_VIDEO_CODEC_MPEG4)
		video_codec = AV_CODEC_ID_MPEG4;
	else if(codec == DEF_ENCODER_VIDEO_CODEC_MPEG2VIDEO)
		video_codec = AV_CODEC_ID_MPEG2VIDEO;

	util_video_pos[session] = 0;
	util_video_increase_pts[session] = 0;
	util_video_encoder_codec[session] = avcodec_find_encoder(video_codec);
	if(!util_video_encoder_codec[session])
	{
		result.error_description = "[Error] avcodec_find_encoder() failed. ";
		goto ffmpeg_api_failed;
	}

	util_video_encoder_context[session] = avcodec_alloc_context3(util_video_encoder_codec[session]);
	if(!util_video_encoder_context[session])
	{
		result.error_description = "[Error] avcodec_alloc_context3() failed. ";
		goto ffmpeg_api_failed;
	}
	
	util_video_encoder_context[session]->bit_rate = bitrate;
	util_video_encoder_context[session]->width = width;
	util_video_encoder_context[session]->height = height;
	util_video_encoder_context[session]->time_base = (AVRational){ 1, fps };
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
			result.error_description = "[Error] av_opt_set() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "profile", "baseline", 0);
		if(ffmpeg_result < 0)
		{
			result.error_description = "[Error] av_opt_set() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "preset", "ultrafast", 0);
		if(ffmpeg_result < 0)
		{
			result.error_description = "[Error] av_opt_set() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "me_method", "dia", 0);
		if(ffmpeg_result < 0)
		{
			result.error_description = "[Error] av_opt_set() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}
	}

	ffmpeg_result = avcodec_open2(util_video_encoder_context[session], util_video_encoder_codec[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_open2() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}
	
	util_video_increase_pts[session] = 90000 / fps;

	util_video_encoder_packet[session] = av_packet_alloc();
	util_video_encoder_raw_data[session] = av_frame_alloc();
	if(!util_video_encoder_raw_data[session] || !util_video_encoder_packet[session])
	{
		result.error_description = "[Error] av_packet_alloc() / av_frame_alloc() failed. ";
		goto ffmpeg_api_failed;
	}
	
	util_video_encoder_raw_data[session]->format = util_video_encoder_context[session]->pix_fmt;
	util_video_encoder_raw_data[session]->width = util_video_encoder_context[session]->width;
	util_video_encoder_raw_data[session]->height = util_video_encoder_context[session]->height;
	ffmpeg_result = av_frame_get_buffer(util_video_encoder_raw_data[session], 0);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error]  av_image_alloc() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_frame_make_writable(util_video_encoder_raw_data[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] av_frame_make_writable() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_video_encoder_stream[session] = avformat_new_stream(util_encoder_format_context[session], util_video_encoder_codec[session]);
	if(!util_video_encoder_stream[session])
	{
		result.error_description = "[Error] avformat_new_stream() failed. ";
		goto ffmpeg_api_failed;
	}
	
	ffmpeg_result = avcodec_parameters_from_context(util_video_encoder_stream[session]->codecpar, util_video_encoder_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_parameters_from_context() failed. ";
		goto ffmpeg_api_failed;
	}

	util_video_stream_index[session] = util_encoder_next_stream_index[session];
	util_encoder_next_stream_index[session]++;

	util_video_encoder_init[session] = true;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;
	
	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;
	
	ffmpeg_api_failed:
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	Util_video_encoder_exit(session);
	return result;
}

Result_with_string Util_encoder_write_header(int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	if(!util_encoder_init)
		Util_encoder_init_variables();

	if(session < 0 || session >= DEF_ENCODER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_encoder_wrote_header[session])
		goto already_inited;
	
	if(!util_encoder_created_file[session])
		goto not_inited;
	

	util_encoder_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ffmpeg_result = avformat_write_header(util_encoder_format_context[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avformat_write_header() failed. ";
		goto ffmpeg_api_failed;
	}

	util_encoder_wrote_header[session] = true;

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	ffmpeg_api_failed:
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_audio_encoder_encode(int size, u8* raw_data, int session)
{
	int encode_offset = 0;
	int ffmpeg_result = 0;
	int one_frame_size = 0;
	int max_out_samples = 0;
	int out_samples = 0;
	int out_size = 0;
	int in_samples = 0;
	int bytes_per_sample = 0;
	u8* swr_in_cache[1] = { NULL, };
	u8* swr_out_cache[1] = { NULL, };
	u8* raw_audio = NULL;
	Result_with_string result;

	if(!util_encoder_init)
		Util_encoder_init_variables();

	if(session < 0 || session >= DEF_ENCODER_MAX_SESSIONS || size <= 0 || !raw_data)
		goto invalid_arg;

	if(!util_audio_encoder_init[session] || !util_encoder_wrote_header[session])
		goto not_inited;

	one_frame_size = av_samples_get_buffer_size(NULL, util_audio_encoder_context[session]->channels, util_audio_encoder_context[session]->frame_size, util_audio_encoder_context[session]->sample_fmt, 0);
	bytes_per_sample = av_get_bytes_per_sample(util_audio_encoder_context[session]->sample_fmt);
	
	in_samples = size / 2;
	swr_in_cache[0] = raw_data;
	max_out_samples = in_samples * util_audio_encoder_conversion_size_rate[session];
	raw_audio = (u8*)Util_safe_linear_alloc((max_out_samples * bytes_per_sample) + util_audio_encoder_cache_size[session]);
	if(!raw_audio)
		goto out_of_memory;

	if(util_audio_encoder_cache_size[session] > 0)
	{
		memcpy(raw_audio, util_audio_encoder_cache[session], util_audio_encoder_cache_size[session]);
		swr_out_cache[0] = (raw_audio + util_audio_encoder_cache_size[session]);
	}
	else
		swr_out_cache[0] = raw_audio;
	
	out_samples = swr_convert(util_audio_encoder_swr_context[session], (uint8_t**)swr_out_cache, max_out_samples, (const uint8_t**)swr_in_cache, in_samples);
	if(out_samples < 0)
	{
		result.error_description = "[Error] swr_convert() failed. " + std::to_string(ffmpeg_result);
		goto ffmpeg_api_failed;
	}
	out_size = out_samples * bytes_per_sample;
	out_size += util_audio_encoder_cache_size[session];

	while(true)
	{
		memcpy(util_audio_encoder_raw_data[session]->data[0], raw_audio + encode_offset, one_frame_size);
		//set pts
		util_audio_encoder_raw_data[session]->pts = util_audio_pos[session];
		util_audio_pos[session] += util_audio_increase_pts[session];

		ffmpeg_result = avcodec_send_frame(util_audio_encoder_context[session], util_audio_encoder_raw_data[session]);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_send_frame() failed. " + std::to_string(ffmpeg_result);
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
				result.error_description = "[Error] av_interleaved_write_frame() failed. " + std::to_string(ffmpeg_result) + " ";
				goto ffmpeg_api_failed;
			}
		}
		else
		{
			//Util_log_save("debug", "avcodec_receive_packet()...", ffmpeg_result);
			av_packet_unref(util_audio_encoder_packet[session]);
		}

		out_size -= one_frame_size;
		encode_offset += one_frame_size;
		if(one_frame_size > out_size)
			break;
	}

	Util_safe_linear_free(util_audio_encoder_cache[session]);
	util_audio_encoder_cache[session] = NULL;
	util_audio_encoder_cache[session] = (u8*)Util_safe_linear_alloc(out_size);
	if(!util_audio_encoder_cache[session])
		goto out_of_memory;
	
	memcpy(util_audio_encoder_cache[session], raw_audio + encode_offset, out_size);
	util_audio_encoder_cache_size[session] = out_size;

	Util_safe_linear_free(raw_audio);
	raw_audio = NULL;

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	out_of_memory:
	util_audio_encoder_cache_size[session] = 0;
	Util_safe_linear_free(util_audio_encoder_cache[session]);
	Util_safe_linear_free(raw_audio);
	util_audio_encoder_cache[session] = NULL;
	raw_audio = NULL;
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
	
	ffmpeg_api_failed:
	util_audio_encoder_cache_size[session] = 0;
	Util_safe_linear_free(util_audio_encoder_cache[session]);
	Util_safe_linear_free(raw_audio);
	util_audio_encoder_cache[session] = NULL;
	raw_audio = NULL;
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_video_encoder_encode(u8* raw_image, int session)
{
	int ffmpeg_result = 0;
	int width = 0;
	int height = 0;
	Result_with_string result;

	if(!util_encoder_init)
		Util_encoder_init_variables();

	if(session < 0 || session >= DEF_ENCODER_MAX_SESSIONS || !raw_image)
		goto invalid_arg;

	if(!util_video_encoder_init[session] || !util_encoder_wrote_header[session])
		goto not_inited;
	
	width = util_video_encoder_raw_data[session]->width;
	height = util_video_encoder_raw_data[session]->height;
	
	memcpy(util_video_encoder_raw_data[session]->data[0], raw_image, width * height);
	memcpy(util_video_encoder_raw_data[session]->data[1], raw_image + width * height, width * height / 4);
	memcpy(util_video_encoder_raw_data[session]->data[2], raw_image + width * height + width * height / 4, width * height / 4);
	util_video_encoder_raw_data[session]->linesize[0] = width;
	util_video_encoder_raw_data[session]->linesize[1] = width / 2;
	util_video_encoder_raw_data[session]->linesize[2] = width / 2;

	//set pts
	util_video_encoder_raw_data[session]->pts = util_video_pos[session];
	util_video_pos[session] += util_video_increase_pts[session];

	ffmpeg_result = avcodec_send_frame(util_video_encoder_context[session], util_video_encoder_raw_data[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_send_frame() failed " + std::to_string(ffmpeg_result);
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
			result.error_description = "av_interleaved_write_frame() failed " + std::to_string(ffmpeg_result);
			goto ffmpeg_api_failed;
		}
	}
	else
	{
		//Util_log_save("debug", "avcodec_receive_packet()...", ffmpeg_result);
		av_packet_unref(util_video_encoder_packet[session]);
	}

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;
	
	ffmpeg_api_failed:
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_encoder_close_output_file(int session)
{
	if(!util_encoder_init)
		Util_encoder_init_variables();
	
	if(session < 0 || session >= DEF_ENCODER_MAX_SESSIONS)
		return;

	if(util_encoder_created_file[session])
	{
		Util_audio_encoder_exit(session);
		Util_video_encoder_exit(session);

		if(util_encoder_wrote_header[session])
			av_write_trailer(util_encoder_format_context[session]);
		
		avio_close(util_encoder_format_context[session]->pb);
		avformat_free_context(util_encoder_format_context[session]);
		util_encoder_created_file[session] = false;
		util_encoder_wrote_header[session] = false;
		util_encoder_next_stream_index[session] = 0;
	}
}

void Util_audio_encoder_exit(int session)
{
	if(util_audio_encoder_init[session])
	{
		avcodec_free_context(&util_audio_encoder_context[session]);
		av_packet_free(&util_audio_encoder_packet[session]);
		av_frame_free(&util_audio_encoder_raw_data[session]);
		swr_free(&util_audio_encoder_swr_context[session]);
		Util_safe_linear_free(util_audio_encoder_cache[session]);
		util_audio_encoder_cache[session] = NULL;
	}
	util_audio_encoder_init[session] = false;
}

void Util_video_encoder_exit(int session)
{
	if(util_video_encoder_init[session])
	{
		avcodec_free_context(&util_video_encoder_context[session]);
		av_packet_free(&util_video_encoder_packet[session]);
		av_frame_free(&util_video_encoder_raw_data[session]);
	}
	util_video_encoder_init[session] = false;
}

Result_with_string Util_image_encoder_encode(std::string file_path, u8* raw_data, int width, int height, int format, int quality)
{
	Result_with_string result;
	if(!raw_data || width <= 0 || height <= 0 || (format == DEF_ENCODER_IMAGE_CODEC_JPG && (quality < 0 || quality > 100))
	|| (format != DEF_ENCODER_IMAGE_CODEC_PNG && format != DEF_ENCODER_IMAGE_CODEC_JPG && format != DEF_ENCODER_IMAGE_CODEC_BMP
	&& format != DEF_ENCODER_IMAGE_CODEC_TGA))
		goto invalid_arg;

	if(format == DEF_ENCODER_IMAGE_CODEC_PNG)
		result.code = stbi_write_png(file_path.c_str(), width, height, 3, raw_data, 0);
	else if(format == DEF_ENCODER_IMAGE_CODEC_JPG)
		result.code = stbi_write_jpg(file_path.c_str(), width, height, 3, raw_data, quality);
	else if(format == DEF_ENCODER_IMAGE_CODEC_BMP)
		result.code = stbi_write_bmp(file_path.c_str(), width, height, 3, raw_data);
	else if(format == DEF_ENCODER_IMAGE_CODEC_TGA)
		result.code = stbi_write_tga(file_path.c_str(), width, height, 3, raw_data);

	if(result.code == 0)
	{
		if(format == DEF_ENCODER_IMAGE_CODEC_PNG)
			result.error_description = "[Error] stbi_write_png() failed. ";
		else if(format == DEF_ENCODER_IMAGE_CODEC_JPG)
			result.error_description = "[Error] stbi_write_jpg() failed. ";
		else if(format == DEF_ENCODER_IMAGE_CODEC_BMP)
			result.error_description = "[Error] stbi_write_bmp() failed. ";
		else if(format == DEF_ENCODER_IMAGE_CODEC_TGA)
			result.error_description = "[Error] stbi_write_tga() failed. ";

		goto stbi_api_failed;
	}
	else
		result.code = 0;

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	stbi_api_failed:
	result.code = DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS_STR;
	return result;
}
