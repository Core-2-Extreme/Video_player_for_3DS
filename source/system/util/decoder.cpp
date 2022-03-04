#include "system/headers.hpp"

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

#include "stb_image/stb_image.h"

extern "C" void memcpy_asm(u8*, u8*, int);

bool util_audio_decoder_init[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
bool util_audio_decoder_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
bool util_audio_decoder_cache_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
int util_audio_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVPacket* util_audio_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVPacket* util_audio_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVFrame* util_audio_decoder_raw_data[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVCodecContext* util_audio_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
const AVCodec* util_audio_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
SwrContext* util_audio_decoder_swr_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];

bool util_video_decoder_init[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
bool util_video_decoder_frame_cores[4] = { true, true, false, false, };
bool util_video_decoder_slice_cores[4] = { false, true, false, false, };
bool util_video_decoder_changeable_buffer_size[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
bool util_video_decoder_cache_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
bool util_video_decoder_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
//int util_video_decoder_previous_pts[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
//int util_video_decoder_increase_per_pts[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_available_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_raw_image_ready_index[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_raw_image_current_index[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_max_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
Handle util_video_decoder_raw_image_mutex[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
AVPacket* util_video_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
AVPacket* util_video_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
AVFrame* util_video_decoder_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS][DEF_DECODER_MAX_RAW_IMAGE];
AVCodecContext* util_video_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
const AVCodec* util_video_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];

bool util_mvd_video_decoder_init = false;
bool util_mvd_video_decoder_changeable_buffer_size = false;
bool util_mvd_video_decoder_first = false;
bool util_mvd_video_decoder_workarounds = false;
int util_mvd_video_decoder_available_raw_image[DEF_DECODER_MAX_SESSIONS];
int util_mvd_video_decoder_raw_image_ready_index[DEF_DECODER_MAX_SESSIONS];
int util_mvd_video_decoder_raw_image_current_index[DEF_DECODER_MAX_SESSIONS];
int util_mvd_video_decoder_packet_size = 0;
int util_mvd_video_decoder_max_raw_image[DEF_DECODER_MAX_SESSIONS];
u8* util_mvd_video_decoder_packet = NULL;
Handle util_mvd_video_decoder_raw_image_mutex[DEF_DECODER_MAX_SESSIONS];
AVFrame* util_mvd_video_decoder_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_RAW_IMAGE];

bool util_subtitle_decoder_init[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
bool util_subtitle_decoder_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
bool util_subtitle_decoder_cache_packet_ready[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
int util_subtitle_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
AVPacket* util_subtitle_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
AVPacket* util_subtitle_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
AVSubtitle* util_subtitle_decoder_raw_data[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
AVCodecContext* util_subtitle_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];
const AVCodec* util_subtitle_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_SUBTITLE_TRACKS];


bool util_decoder_opened_file[DEF_DECODER_MAX_SESSIONS];
bool util_decoder_init = false;
int util_decoder_available_cache_packet[DEF_DECODER_MAX_SESSIONS];
int util_decoder_cache_packet_ready_index[DEF_DECODER_MAX_SESSIONS];
int util_decoder_cache_packet_current_index[DEF_DECODER_MAX_SESSIONS];
Handle util_decoder_cache_packet_mutex[DEF_DECODER_MAX_SESSIONS];
MVDSTD_Config util_decoder_mvd_config;
AVPacket* util_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_CACHE_PACKETS];
AVFormatContext* util_decoder_format_context[DEF_DECODER_MAX_SESSIONS];

void Util_video_decoder_free(void *opaque, uint8_t *data)
{
	Util_safe_linear_free(data);
}

int Util_video_decoder_allocate_yuv420p_buffer(AVCodecContext *avctx, AVFrame *frame, int flags)
{
	int width = 0;
	int height = 0;
	if(avctx->codec_type == AVMEDIA_TYPE_VIDEO)
	{
	    for (int i = 0; i < AV_NUM_DATA_POINTERS; i++) 
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

		frame->linesize[0] = width;
		frame->linesize[1] = width / 2;
		frame->linesize[2] = width / 2;
		
        frame->data[0] = (u8*)Util_safe_linear_alloc(width * height);
        frame->data[1] = (u8*)Util_safe_linear_alloc(width * height / 4);
        frame->data[2] = (u8*)Util_safe_linear_alloc(width * height / 4);

        frame->buf[0] = av_buffer_create(frame->data[0], frame->linesize[0] * frame->height, Util_video_decoder_free, NULL, 0);
        frame->buf[1] = av_buffer_create(frame->data[1], frame->linesize[1] * frame->height / 4, Util_video_decoder_free, NULL, 0);
        frame->buf[2] = av_buffer_create(frame->data[2], frame->linesize[2] * frame->height / 4, Util_video_decoder_free, NULL, 0);
		return 0;
	}
	else
		return -1;
}

void Util_decoder_init_variables(void)
{
	if(util_decoder_init)
		return;

	for(int i = 0; i < DEF_DECODER_MAX_SESSIONS; i++)
	{
		util_decoder_opened_file[i] = false;
		util_decoder_available_cache_packet[i] = 0;
		util_decoder_cache_packet_ready_index[i] = 0;
		util_decoder_cache_packet_current_index[i] = 0;
		util_decoder_cache_packet_mutex[i] = 0;
		memset(&util_decoder_mvd_config, 0x0, sizeof(util_decoder_mvd_config));
		util_decoder_format_context[i] = NULL;

		for(int k = 0; k < DEF_DECODER_MAX_CACHE_PACKETS; k++)
			util_decoder_cache_packet[i][k] = NULL;

		for(int k = 0; k < DEF_DECODER_MAX_AUDIO_TRACKS; k++)
		{
			util_audio_decoder_init[i][k] = false;
			util_audio_decoder_packet_ready[i][k] = false;
			util_audio_decoder_cache_packet_ready[i][k] = false;
			util_audio_decoder_stream_num[i][k] = -1;
			util_audio_decoder_packet[i][k] = NULL;
			util_audio_decoder_cache_packet[i][k] = NULL;
			util_audio_decoder_raw_data[i][k] = NULL;
			util_audio_decoder_context[i][k] = NULL;
			util_audio_decoder_codec[i][k] = NULL;
			util_audio_decoder_swr_context[i][k] = NULL;
		}

		for(int k = 0; k < DEF_DECODER_MAX_VIDEO_TRACKS; k++)
		{
			util_video_decoder_init[i][k] = false;
			util_video_decoder_changeable_buffer_size[i][k] = false;
			util_video_decoder_cache_packet_ready[i][k] = false;
			util_video_decoder_packet_ready[i][k] = false;
			//util_video_decoder_previous_pts[i][k] = 0;
			//util_video_decoder_increase_per_pts[i][k] = 0;
			util_video_decoder_available_raw_image[i][k] = 0;
			util_video_decoder_raw_image_ready_index[i][k] = 0;
			util_video_decoder_raw_image_current_index[i][k] = 0;
			util_video_decoder_stream_num[i][k] = -1;
			util_video_decoder_max_raw_image[i][k] = 0;
			util_video_decoder_raw_image_mutex[i][k] = 0;
			util_video_decoder_packet[i][k] = NULL;
			util_video_decoder_cache_packet[i][k] = NULL;
			util_video_decoder_context[i][k] = NULL;
			util_video_decoder_codec[i][k] = NULL;

			for(int s = 0; s < DEF_DECODER_MAX_RAW_IMAGE; s++)
				util_video_decoder_raw_image[i][k][s] = NULL;
		}

		util_mvd_video_decoder_available_raw_image[i] = 0;
		util_mvd_video_decoder_raw_image_ready_index[i] = 0;
		util_mvd_video_decoder_raw_image_current_index[i] = 0;
		util_mvd_video_decoder_max_raw_image[i] = 0;
		util_mvd_video_decoder_raw_image_mutex[i] = 0;

		for(int s = 0; s < DEF_DECODER_MAX_RAW_IMAGE; s++)
			util_mvd_video_decoder_raw_image[i][s] = NULL;

		for(int k = 0; k < DEF_DECODER_MAX_SUBTITLE_TRACKS; k++)
		{
			util_subtitle_decoder_init[i][k] = false;
			util_subtitle_decoder_packet_ready[i][k] = false;
			util_subtitle_decoder_cache_packet_ready[i][k] = false;
			util_subtitle_decoder_stream_num[i][k] = 0;
			util_subtitle_decoder_packet[i][k] = NULL;
			util_subtitle_decoder_cache_packet[i][k] = NULL;
			util_subtitle_decoder_raw_data[i][k] = NULL;
			util_subtitle_decoder_context[i][k] = NULL;
			util_subtitle_decoder_codec[i][k] = NULL;
		}
	}

	util_mvd_video_decoder_init = false;
	util_mvd_video_decoder_changeable_buffer_size = false;
	util_mvd_video_decoder_first = false;
	util_mvd_video_decoder_workarounds = false;
	util_mvd_video_decoder_packet_size = 0;
	util_mvd_video_decoder_packet = NULL;

	util_decoder_init = true;
}

Result_with_string Util_decoder_open_file(std::string file_path, int* num_of_audio_tracks, int* num_of_video_tracks, int* num_of_subtitle_tracks, int session)
{
	int ffmpeg_result = 0;
	int audio_index = 0;
	int video_index = 0;
	int subtitle_index = 0;
	Result_with_string result;

	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(file_path == "" || !num_of_audio_tracks || !num_of_video_tracks || !num_of_subtitle_tracks || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_decoder_opened_file[session])
		goto already_inited;

	*num_of_video_tracks = 0;
	*num_of_audio_tracks = 0;
	*num_of_subtitle_tracks = 0;
	util_decoder_cache_packet_ready_index[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_available_cache_packet[session] = 0;
	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		util_audio_decoder_stream_num[session][i] = -1;

	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
		util_video_decoder_stream_num[session][i] = -1;
	
	util_decoder_format_context[session] = avformat_alloc_context();
	if(!util_decoder_format_context[session])
	{
		result.error_description = "[Error] avformat_alloc_context() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_open_input(&util_decoder_format_context[session], file_path.c_str(), NULL, NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avformat_open_input() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}
	
	ffmpeg_result = avformat_find_stream_info(util_decoder_format_context[session], NULL);
	if(!util_decoder_format_context[session])
	{
		result.error_description = "[Error] avformat_find_stream_info() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	result.code = svcCreateMutex(&util_decoder_cache_packet_mutex[session], false);
	if(result.code != 0)
	{
		result.error_description = "[Error] svcCreateMutex() failed. ";
		goto nintendo_api_failed;
	}

	for(int i = 0; i < (int)util_decoder_format_context[session]->nb_streams; i++)
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
		result.error_description = "[Error] No audio and video was found. ";
		goto other;
	}

	*num_of_audio_tracks = audio_index;
	*num_of_video_tracks = video_index;
	*num_of_subtitle_tracks = subtitle_index;
	util_decoder_opened_file[session] = true;

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
	avformat_free_context(util_decoder_format_context[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;

	nintendo_api_failed:
	svcCloseHandle(util_decoder_cache_packet_mutex[session]);
	avformat_free_context(util_decoder_format_context[session]);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	other:
	svcCloseHandle(util_decoder_cache_packet_mutex[session]);
	avformat_free_context(util_decoder_format_context[session]);
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

Result_with_string Util_audio_decoder_init(int num_of_audio_tracks, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(num_of_audio_tracks <= 0 || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;
	
	if(!util_decoder_opened_file[session])
		goto not_inited;

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		if(util_audio_decoder_init[session][i])
			goto already_inited;
	}

	for(int i = 0; i < num_of_audio_tracks; i++)
	{
		util_audio_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_audio_decoder_codec[session][i])
		{
			result.error_description = "[Error] avcodec_find_decoder() failed. ";
			goto ffmpeg_api_failed;
		}

		util_audio_decoder_context[session][i] = avcodec_alloc_context3(util_audio_decoder_codec[session][i]);
		if(!util_audio_decoder_context[session][i])
		{
			result.error_description = "[Error] avcodec_alloc_context3() failed. ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_audio_decoder_context[session][i], util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_parameters_to_context() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		util_audio_decoder_context[session][i]->flags = AV_CODEC_FLAG_OUTPUT_CORRUPT;
		ffmpeg_result = avcodec_open2(util_audio_decoder_context[session][i], util_audio_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_open2() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		util_audio_decoder_swr_context[session][i] = swr_alloc_set_opts(NULL, av_get_default_channel_layout(util_audio_decoder_context[session][i]->channels), AV_SAMPLE_FMT_S16, util_audio_decoder_context[session][i]->sample_rate,
			av_get_default_channel_layout(util_audio_decoder_context[session][i]->channels), util_audio_decoder_context[session][i]->sample_fmt, util_audio_decoder_context[session][i]->sample_rate, 0, NULL);
		if(!util_audio_decoder_swr_context[session][i])
		{
			result.error_description = "[Error] swr_alloc_set_opts() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = swr_init(util_audio_decoder_swr_context[session][i]);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] swr_init() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		util_audio_decoder_init[session][i] = true;
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

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	ffmpeg_api_failed:
	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		util_audio_decoder_init[session][i] = false;
		avcodec_free_context(&util_audio_decoder_context[session][i]);
		swr_free(&util_audio_decoder_swr_context[session][i]);
	}
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_video_decoder_set_enabled_cores(bool frame_threading_cores[4], bool slice_threading_cores[4])
{
	if(!frame_threading_cores[0] && !frame_threading_cores[1] && !frame_threading_cores[2] && !frame_threading_cores[3]
	&& !slice_threading_cores[0] && !slice_threading_cores[1] && !slice_threading_cores[2] && !slice_threading_cores[3])
		return;

	for(int i = 0; i < 4; i++)
	{
		util_video_decoder_frame_cores[i] = frame_threading_cores[i];
		util_video_decoder_slice_cores[i] = slice_threading_cores[i];
	}
}

Result_with_string Util_video_decoder_init(int low_resolution, int num_of_video_tracks, int num_of_threads, int thread_type, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(num_of_video_tracks <= 0 || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;
	
	if(!util_decoder_opened_file[session])
		goto not_inited;

	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		if(util_video_decoder_init[session][i])
			goto already_inited;
	}

	for (int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		util_video_decoder_raw_image_current_index[session][i] = 0;
		util_video_decoder_raw_image_ready_index[session][i] = 0;
		util_video_decoder_available_raw_image[session][i] = 0;
		util_video_decoder_max_raw_image[session][i] = 0;
	}

	for(int i = 0; i < num_of_video_tracks; i++)
	{
		util_video_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_video_decoder_codec[session][i])
		{
			result.error_description = "[Error] avcodec_find_decoder() failed. ";
			goto ffmpeg_api_failed;
		}

		util_video_decoder_context[session][i] = avcodec_alloc_context3(util_video_decoder_codec[session][i]);
		if(!util_video_decoder_context[session][i])
		{
			result.error_description = "[Error] avcodec_alloc_context3() failed. ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_video_decoder_context[session][i], util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_parameters_to_context() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		if(util_video_decoder_codec[session][i]->max_lowres < low_resolution)
			util_video_decoder_context[session][i]->lowres = util_video_decoder_codec[session][i]->max_lowres;
		else
			util_video_decoder_context[session][i]->lowres = low_resolution;

		util_video_decoder_context[session][i]->flags = AV_CODEC_FLAG_OUTPUT_CORRUPT;
		util_video_decoder_context[session][i]->thread_count = num_of_threads;
		if(thread_type == DEF_DECODER_THREAD_TYPE_AUTO)
		{
			if(util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_FRAME_THREADS)
				util_video_decoder_context[session][i]->thread_type = FF_THREAD_FRAME;
			else if(util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_SLICE_THREADS)
				util_video_decoder_context[session][i]->thread_type = FF_THREAD_SLICE;
			else
			{
				util_video_decoder_context[session][i]->thread_type = FF_THREAD_FRAME;
				//util_video_decoder_context[session][i]->thread_type = 0;
				//util_video_decoder_context[session][i]->thread_count = 1;
			}
		}
		else if(thread_type == DEF_DECODER_THREAD_TYPE_SLICE && util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_SLICE_THREADS)
			util_video_decoder_context[session][i]->thread_type = FF_THREAD_SLICE;
		else if(thread_type == DEF_DECODER_THREAD_TYPE_FRAME && util_video_decoder_codec[session][i]->capabilities & AV_CODEC_CAP_FRAME_THREADS)
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

		util_video_decoder_context[session][i]->thread_safe_callbacks = 1;
		util_video_decoder_context[session][i]->get_buffer2 = Util_video_decoder_allocate_yuv420p_buffer;

		if(util_video_decoder_context[session][i]->pix_fmt != AV_PIX_FMT_YUV420P && util_video_decoder_context[session][i]->pix_fmt != AV_PIX_FMT_YUVJ420P)
		{
			result.error_description = "[Error] Unsupported pixel format. ";
			goto other;
		}

		ffmpeg_result = avcodec_open2(util_video_decoder_context[session][i], util_video_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_open2() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		result.code = svcCreateMutex(&util_video_decoder_raw_image_mutex[session][i], false);
		if(result.code != 0)
		{
			result.error_description = "[Error] svcCreateMutex() failed. ";
			goto nintendo_api_failed;
		}

		util_video_decoder_max_raw_image[session][i] = DEF_DECODER_MAX_RAW_IMAGE;
		util_video_decoder_changeable_buffer_size[session][i] = true;
		util_video_decoder_init[session][i] = true;
		//util_video_decoder_previous_pts[session][i] = 0;
		//util_video_decoder_increase_per_pts[session][i] = 0;
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

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	ffmpeg_api_failed:
	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		avcodec_free_context(&util_video_decoder_context[session][i]);
		svcCloseHandle(util_video_decoder_raw_image_mutex[session][i]);
	}
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;

	nintendo_api_failed:
	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		avcodec_free_context(&util_video_decoder_context[session][i]);
		svcCloseHandle(util_video_decoder_raw_image_mutex[session][i]);
	}
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	other:
	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		avcodec_free_context(&util_video_decoder_context[session][i]);
		svcCloseHandle(util_video_decoder_raw_image_mutex[session][i]);
	}
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

Result_with_string Util_mvd_video_decoder_init(int session)
{
	int width = 0;
	int height = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;
	
	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0])
		goto not_inited;

	if(util_mvd_video_decoder_init)
		goto already_inited;
	
	width = util_video_decoder_context[session][0]->width;
	height = util_video_decoder_context[session][0]->height;

	util_mvd_video_decoder_raw_image_current_index[session] = 0;
	util_mvd_video_decoder_raw_image_ready_index[session] = 0;
	util_mvd_video_decoder_available_raw_image[session] = 0;
	util_mvd_video_decoder_workarounds = false;

	util_mvd_video_decoder_packet_size = 1024 * 256;
	util_mvd_video_decoder_packet = (u8*)Util_safe_linear_alloc(util_mvd_video_decoder_packet_size);
	if(!util_mvd_video_decoder_packet)
		goto out_of_linear_memory;

	result.code = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264, MVD_OUTPUT_BGR565, MVD_DEFAULT_WORKBUF_SIZE * 1.5, NULL);
	//result.code = mvdstdInit(MVDMODE_VIDEOPROCESSING, (MVDSTD_InputFormat)0x00180001, MVD_OUTPUT_BGR565, width * height * 9, NULL);
	if(result.code != 0)
	{
		result.error_description = "[Error] mvdstdInit() failed. ";
		goto nintendo_api_failed;
	}

	if(width % 16 != 0)
		width += 16 - width % 16;
	if(height % 16 != 0)
		height += 16 - height % 16;

	result.code = svcCreateMutex(&util_mvd_video_decoder_raw_image_mutex[session], false);
	if(result.code != 0)
	{
		result.error_description = "[Error] svcCreateMutex() failed. ";
		goto nintendo_api_failed;
	}

	//Util_log_save("debug", util_video_decoder_context[session][0]->has_b_frames ? "has_b_frames" : "does_not_have_b_frames");

	if(util_video_decoder_context[session][0]->has_b_frames)
		util_mvd_video_decoder_workarounds = true;

	util_mvd_video_decoder_max_raw_image[session] = DEF_DECODER_MAX_RAW_IMAGE;
	util_mvd_video_decoder_first = true;
	util_mvd_video_decoder_changeable_buffer_size = true;
	util_mvd_video_decoder_init = true;
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

	out_of_linear_memory:
	result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
	result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
	return result;

	nintendo_api_failed:
	mvdstdExit();
	svcCloseHandle(util_mvd_video_decoder_raw_image_mutex[session]);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_subtitle_decoder_init(int num_of_subtitle_tracks, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(num_of_subtitle_tracks <= 0 || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;
	
	if(!util_decoder_opened_file[session])
		goto not_inited;

	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
	{
		if(util_subtitle_decoder_init[session][i])
			goto already_inited;
	}

	for(int i = 0; i < num_of_subtitle_tracks; i++)
	{
		util_subtitle_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_subtitle_decoder_codec[session][i])
		{
			result.error_description = "[Error] avcodec_find_decoder() failed. ";
			goto ffmpeg_api_failed;
		}

		util_subtitle_decoder_context[session][i] = avcodec_alloc_context3(util_subtitle_decoder_codec[session][i]);
		if(!util_subtitle_decoder_context[session][i])
		{
			result.error_description = "[Error] avcodec_alloc_context3() failed. ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_subtitle_decoder_context[session][i], util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_parameters_to_context() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		ffmpeg_result = avcodec_open2(util_subtitle_decoder_context[session][i], util_subtitle_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_open2() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}

		util_subtitle_decoder_init[session][i] = true;
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

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	ffmpeg_api_failed:
	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
	{
		util_subtitle_decoder_init[session][i] = false;
		avcodec_free_context(&util_subtitle_decoder_context[session][i]);
	}
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_audio_decoder_get_info(Audio_info* audio_info, int audio_index, int session)
{
	AVDictionaryEntry *data = NULL;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(!audio_info || audio_index < 0 || audio_index > DEF_DECODER_MAX_AUDIO_TRACKS || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_audio_decoder_init[session][audio_index])
		return;
		
	audio_info->track_lang = "language:und";

	if(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][audio_index]]->metadata)
	{
		data = av_dict_get(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][audio_index]]->metadata, "language", data, AV_DICT_IGNORE_SUFFIX);
		if(data)
			audio_info->track_lang = (std::string)data->key + ":" + data->value;
	}
		
	audio_info->bitrate = util_audio_decoder_context[session][audio_index]->bit_rate;
	audio_info->sample_rate = util_audio_decoder_context[session][audio_index]->sample_rate;
	audio_info->ch = util_audio_decoder_context[session][audio_index]->channels;
	audio_info->format_name = util_audio_decoder_codec[session][audio_index]->long_name;
	audio_info->duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
}

void Util_video_decoder_get_info(Video_info* video_info, int video_index, int session)
{
	AVRational sar;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(!video_info || video_index < 0 || video_index > DEF_DECODER_MAX_VIDEO_TRACKS || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][video_index])
		return;

	//Util_log_save("debug", std::to_string(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->sample_aspect_ratio.num) + ":" + std::to_string(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->sample_aspect_ratio.den));
	sar = av_guess_sample_aspect_ratio(util_decoder_format_context[session], util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]], NULL);
	//Util_log_save("debug", std::to_string(sar.num) + ":" + std::to_string(sar.den));
	if(sar.num == 0 && sar.den == 1)
	{
		video_info->sar_width = 1;
		video_info->sar_height = 1;
	}
	else
	{
		video_info->sar_width = sar.num;
		video_info->sar_height = sar.den;
	}

	video_info->width = util_video_decoder_context[session][video_index]->width;
	video_info->height = util_video_decoder_context[session][video_index]->height;
	if(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.den == 0)
		video_info->framerate = 0;
	else
		video_info->framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.den;

	video_info->format_name = util_video_decoder_codec[session][video_index]->long_name;
	video_info->duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
	if(util_video_decoder_context[session][video_index]->thread_type == FF_THREAD_FRAME)
		video_info->thread_type = DEF_DECODER_THREAD_TYPE_FRAME;
	else if(util_video_decoder_context[session][video_index]->thread_type == FF_THREAD_SLICE)
		video_info->thread_type = DEF_DECODER_THREAD_TYPE_SLICE;
	else
		video_info->thread_type = DEF_DECODER_THREAD_TYPE_NONE;
}

void Util_subtitle_decoder_get_info(Subtitle_info* subtitle_info, int subtitle_index, int session)
{
	AVDictionaryEntry *data = NULL;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(!subtitle_info || subtitle_index < 0 || subtitle_index > DEF_DECODER_MAX_SUBTITLE_TRACKS || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_subtitle_decoder_init[session][subtitle_index])
		return;
		
	subtitle_info->track_lang = "language:und";

	if(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][subtitle_index]]->metadata)
	{
		data = av_dict_get(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][subtitle_index]]->metadata, "language", data, AV_DICT_IGNORE_SUFFIX);
		if(data)
			subtitle_info->track_lang = (std::string)data->key + ":" + data->value;
	}
		
	subtitle_info->format_name = util_subtitle_decoder_codec[session][subtitle_index]->long_name;
}

void Util_decoder_clear_cache_packet(int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;
	
	if(!util_decoder_opened_file[session])
		return;
	
	for(int i = 0; i < DEF_DECODER_MAX_CACHE_PACKETS; i++)
		av_packet_free(&util_decoder_cache_packet[session][i]);

	util_decoder_available_cache_packet[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_cache_packet_ready_index[session] = 0;
}

int Util_decoder_get_available_packet_num(int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return 0;
	
	if(!util_decoder_opened_file[session])
		return 0;
	else
		return util_decoder_available_cache_packet[session];
}

Result_with_string Util_decoder_read_packet(int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;
	
	if(!util_decoder_opened_file[session])
		goto not_inited;

	svcWaitSynchronization(util_decoder_cache_packet_mutex[session], U64_MAX);
	if(util_decoder_available_cache_packet[session] + 1 >= DEF_DECODER_MAX_CACHE_PACKETS)
	{
		result.error_description = "[Error] Queues are full. ";
		goto try_again;
	}
	svcReleaseMutex(util_decoder_cache_packet_mutex[session]);

	util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]] = av_packet_alloc();
	if(!util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]])
	{
		result.error_description = "[Error] av_packet_alloc() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = av_read_frame(util_decoder_format_context[session], util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] av_read_frame() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	if(util_decoder_cache_packet_ready_index[session] + 1 < DEF_DECODER_MAX_CACHE_PACKETS)
		util_decoder_cache_packet_ready_index[session]++;
	else
		util_decoder_cache_packet_ready_index[session] = 0;

	svcWaitSynchronization(util_decoder_cache_packet_mutex[session], U64_MAX);
	util_decoder_available_cache_packet[session]++;
	svcReleaseMutex(util_decoder_cache_packet_mutex[session]);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	svcReleaseMutex(util_decoder_cache_packet_mutex[session]);
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	av_packet_free(&util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_parse_packet(int* type, int* packet_index, bool* key_frame, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || !type || !packet_index || !key_frame)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;
	
	*key_frame = false;
	*packet_index = -1;
	*type = DEF_DECODER_PACKET_TYPE_UNKNOWN;

	svcWaitSynchronization(util_decoder_cache_packet_mutex[session], U64_MAX);
	if(util_decoder_available_cache_packet[session] <= 0)
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}
	svcReleaseMutex(util_decoder_cache_packet_mutex[session]);

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_audio_decoder_stream_num[session][i])//audio packet
		{
			util_audio_decoder_cache_packet[session][i] = av_packet_alloc();
			if(!util_audio_decoder_cache_packet[session][i])
			{
				result.error_description = "[Error] av_packet_alloc() failed. ";
				goto ffmpeg_api_failed;
			}

			av_packet_unref(util_audio_decoder_cache_packet[session][i]);
			ffmpeg_result = av_packet_ref(util_audio_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
			if(ffmpeg_result != 0)
			{
				av_packet_free(&util_audio_decoder_cache_packet[session][i]);
				result.error_description = "[Error] av_packet_ref() failed. " + std::to_string(ffmpeg_result) + " ";
				goto ffmpeg_api_failed;
			}
			*packet_index = i;
			*type = DEF_DECODER_PACKET_TYPE_AUDIO;
			util_audio_decoder_cache_packet_ready[session][i] = true;
			break;
		}
	}

	if(*type == DEF_DECODER_PACKET_TYPE_UNKNOWN)
	{
		for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
		{
			if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_video_decoder_stream_num[session][i])//video packet
			{
				util_video_decoder_cache_packet[session][i] = av_packet_alloc();
				if(!util_video_decoder_cache_packet[session][i])
				{
					result.error_description = "[Error] av_packet_alloc() failed. ";
					goto ffmpeg_api_failed;
				}

				av_packet_unref(util_video_decoder_cache_packet[session][i]);
				ffmpeg_result = av_packet_ref(util_video_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
				if(ffmpeg_result != 0)
				{
					av_packet_free(&util_video_decoder_cache_packet[session][i]);
					result.error_description = "[Error] av_packet_ref() failed. " + std::to_string(ffmpeg_result) + " ";
					goto ffmpeg_api_failed;
				}
				*packet_index = i;
				*type = DEF_DECODER_PACKET_TYPE_VIDEO;
				*key_frame = util_video_decoder_cache_packet[session][i]->flags & AV_PKT_FLAG_KEY;
				util_video_decoder_cache_packet_ready[session][i] = true;
				break;
			}
		}
	}

	if(*type == DEF_DECODER_PACKET_TYPE_UNKNOWN)
	{
		for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		{
			if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_subtitle_decoder_stream_num[session][i])//subtitle packet
			{
				util_subtitle_decoder_cache_packet[session][i] = av_packet_alloc();
				if(!util_subtitle_decoder_cache_packet[session][i])
				{
					result.error_description = "[Error] av_packet_alloc() failed. ";
					goto ffmpeg_api_failed;
				}

				av_packet_unref(util_subtitle_decoder_cache_packet[session][i]);
				ffmpeg_result = av_packet_ref(util_subtitle_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
				if(ffmpeg_result != 0)
				{
					av_packet_free(&util_subtitle_decoder_cache_packet[session][i]);
					result.error_description = "[Error] av_packet_ref() failed. " + std::to_string(ffmpeg_result) + " ";
					goto ffmpeg_api_failed;
				}
				*packet_index = i;
				*type = DEF_DECODER_PACKET_TYPE_SUBTITLE;
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
	
	svcWaitSynchronization(util_decoder_cache_packet_mutex[session], U64_MAX);
	util_decoder_available_cache_packet[session]--;
	svcReleaseMutex(util_decoder_cache_packet_mutex[session]);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	svcReleaseMutex(util_decoder_cache_packet_mutex[session]);
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_ready_audio_packet(int packet_index, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_AUDIO_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_audio_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_audio_decoder_cache_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}

	if(util_audio_decoder_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] Queues are full. ";
		goto try_again;
	}

	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	util_audio_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_audio_decoder_packet[session][packet_index])
	{
		result.error_description = "[Error] av_packet_alloc() failed. ";
		goto ffmpeg_api_failed;
	}

	av_packet_unref(util_audio_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_audio_decoder_packet[session][packet_index], util_audio_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[error] av_packet_ref() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_audio_decoder_cache_packet_ready[session][packet_index] = false;
	util_audio_decoder_packet_ready[session][packet_index] = true;
	av_packet_free(&util_audio_decoder_cache_packet[session][packet_index]);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_ready_video_packet(int packet_index, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_video_decoder_cache_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}

	if(util_video_decoder_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] Queues are full. ";
		goto try_again;
	}

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	util_video_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_video_decoder_packet[session][packet_index])
	{
		result.error_description = "[Error] av_packet_alloc() failed. ";
		goto ffmpeg_api_failed;
	}

	av_packet_unref(util_video_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_video_decoder_packet[session][packet_index], util_video_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] av_packet_ref() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_video_decoder_cache_packet_ready[session][packet_index] = false;
	util_video_decoder_packet_ready[session][packet_index] = true;
	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_ready_subtitle_packet(int packet_index, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_SUBTITLE_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_subtitle_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_subtitle_decoder_cache_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}

	if(util_subtitle_decoder_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] Queues are full. ";
		goto try_again;
	}

	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	util_subtitle_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_subtitle_decoder_packet[session][packet_index])
	{
		result.error_description = "[Error] av_packet_alloc() failed. ";
		goto ffmpeg_api_failed;
	}

	av_packet_unref(util_subtitle_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_subtitle_decoder_packet[session][packet_index], util_subtitle_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[error] av_packet_ref() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_subtitle_decoder_cache_packet_ready[session][packet_index] = false;
	util_subtitle_decoder_packet_ready[session][packet_index] = true;
	av_packet_free(&util_subtitle_decoder_cache_packet[session][packet_index]);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_decoder_skip_audio_packet(int packet_index, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_AUDIO_TRACKS)
		return;

	if(!util_decoder_opened_file[session] || !util_audio_decoder_init[session][packet_index])
		return;

	if(!util_audio_decoder_cache_packet_ready[session][packet_index])
		return;
	
	av_packet_free(&util_audio_decoder_cache_packet[session][packet_index]);
	util_audio_decoder_cache_packet_ready[session][packet_index] = false;
}

void Util_decoder_skip_video_packet(int packet_index, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return;

	if(!util_video_decoder_cache_packet_ready[session][packet_index])
		return;

	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
	util_video_decoder_cache_packet_ready[session][packet_index] = false;
}

void Util_decoder_skip_subtitle_packet(int packet_index, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_SUBTITLE_TRACKS)
		return;

	if(!util_decoder_opened_file[session] || !util_subtitle_decoder_init[session][packet_index])
		return;

	if(!util_subtitle_decoder_cache_packet_ready[session][packet_index])
		return;
	
	av_packet_free(&util_subtitle_decoder_cache_packet[session][packet_index]);
	util_subtitle_decoder_cache_packet_ready[session][packet_index] = false;
}

void Util_video_decoder_set_raw_image_buffer_size(int max_num_of_buffer, int packet_index, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS
	|| max_num_of_buffer < 3 || max_num_of_buffer > DEF_DECODER_MAX_RAW_IMAGE)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index]
	|| !util_video_decoder_changeable_buffer_size[session][packet_index])
		return;

	util_video_decoder_max_raw_image[session][packet_index] = max_num_of_buffer;
}

void Util_mvd_video_decoder_set_raw_image_buffer_size(int max_num_of_buffer, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || max_num_of_buffer < 3 || max_num_of_buffer > DEF_DECODER_MAX_RAW_IMAGE)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init
	|| !util_mvd_video_decoder_changeable_buffer_size)
		return;

	util_mvd_video_decoder_max_raw_image[session] = max_num_of_buffer;
}

int Util_video_decoder_get_raw_image_buffer_size( int packet_index, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return 0;

	return util_video_decoder_max_raw_image[session][packet_index];
}

int Util_mvd_video_decoder_get_raw_image_buffer_size(int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return 0;

	return util_mvd_video_decoder_max_raw_image[session];
}

Result_with_string Util_audio_decoder_decode(int* size, u8** raw_data, double* current_pos, int packet_index, int session)
{
	int ffmpeg_result = 0;
	int swr_size = 0;
	double current_frame = 0;
	double timebase = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_AUDIO_TRACKS
	|| !size || !raw_data || !current_pos)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_audio_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_audio_decoder_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}

	if(util_audio_decoder_packet[session][packet_index]->duration != 0)
		current_frame = (double)util_audio_decoder_packet[session][packet_index]->dts / util_audio_decoder_packet[session][packet_index]->duration;

	*size = 0;
	*current_pos = 0;
	
	util_audio_decoder_raw_data[session][packet_index] = av_frame_alloc();
	if(!util_audio_decoder_raw_data[session][packet_index])
	{
		result.error_description = "[Error] av_frame_alloc() failed. ";
		goto ffmpeg_api_failed;
	}
	
	ffmpeg_result = avcodec_send_packet(util_audio_decoder_context[session][packet_index], util_audio_decoder_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_send_packet() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_receive_frame(util_audio_decoder_context[session][packet_index], util_audio_decoder_raw_data[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_receive_frame() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][packet_index]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_audio_decoder_packet[session][packet_index]->dts * timebase * 1000;//calc pos
	else
		*current_pos = current_frame * ((1000.0 / util_audio_decoder_raw_data[session][packet_index]->sample_rate) * util_audio_decoder_raw_data[session][packet_index]->nb_samples);//calc pos

	Util_safe_linear_free(*raw_data);
	*raw_data = NULL;
	*raw_data = (u8*)Util_safe_linear_alloc(util_audio_decoder_raw_data[session][packet_index]->nb_samples * 2 * util_audio_decoder_context[session][packet_index]->channels);
	swr_size = swr_convert(util_audio_decoder_swr_context[session][packet_index], raw_data, util_audio_decoder_raw_data[session][packet_index]->nb_samples, (const uint8_t**)util_audio_decoder_raw_data[session][packet_index]->data, util_audio_decoder_raw_data[session][packet_index]->nb_samples);
	if(swr_size <= 0)
	{
		result.error_description = "[Error] swr_convert() failed. " + std::to_string(swr_size) + " ";
		goto ffmpeg_api_failed;
	}
	*size = swr_size * 2 * util_audio_decoder_context[session][packet_index]->channels;

	util_audio_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_frame_free(&util_audio_decoder_raw_data[session][packet_index]);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	util_audio_decoder_packet_ready[session][packet_index] = false;
	Util_safe_linear_free(*raw_data);
	*raw_data = NULL;
	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_frame_free(&util_audio_decoder_raw_data[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_video_decoder_decode(int packet_index, int session)
{
	int ffmpeg_result = 0;
	int buffer_num = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_video_decoder_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}

	util_video_decoder_changeable_buffer_size[session][packet_index] = false;
	if(util_video_decoder_available_raw_image[session][packet_index] + 1 >= util_video_decoder_max_raw_image[session][packet_index])
	{
		result.error_description = "[Error] Queues are full. ";
		goto try_again;
	}
	
	buffer_num = util_video_decoder_raw_image_current_index[session][packet_index];
	
	util_video_decoder_raw_image[session][packet_index][buffer_num] = av_frame_alloc();
	if(!util_video_decoder_raw_image[session][packet_index][buffer_num])
	{
		result.error_description = "[Error] av_frame_alloc() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_send_packet(util_video_decoder_context[session][packet_index], util_video_decoder_packet[session][packet_index]);
	//dav1d decoder sometimes return EAGAIN if so, ignore it and call avcodec_receive_frame()
	if(ffmpeg_result != 0 && ffmpeg_result != AVERROR(EAGAIN))
	{
		result.error_description = "[Error] avcodec_send_packet() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_receive_frame(util_video_decoder_context[session][packet_index], util_video_decoder_raw_image[session][packet_index][buffer_num]);
	if(ffmpeg_result != 0)
	{
		//dav1d decoder sometimes doesn't return image at first call
		if(ffmpeg_result == AVERROR(EAGAIN))
			ffmpeg_result = avcodec_receive_frame(util_video_decoder_context[session][packet_index], util_video_decoder_raw_image[session][packet_index][buffer_num]);

		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avcodec_receive_frame() failed. " + std::to_string(ffmpeg_result) + " ";
			goto ffmpeg_api_failed;
		}
	}

	if(buffer_num + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_current_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_current_index[session][packet_index] = 0;

	svcWaitSynchronization(util_video_decoder_raw_image_mutex[session][packet_index], U64_MAX);
	util_video_decoder_available_raw_image[session][packet_index]++;
	svcReleaseMutex(util_video_decoder_raw_image_mutex[session][packet_index]);

	util_video_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	util_video_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_mvd_video_decoder_decode(int session)
{
	int offset = 0;
	int source_offset = 0;
	int size = 0;
	int buffer_num = 0;
	int width = 0;
	int height = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		goto not_inited;

	if(!util_video_decoder_packet_ready[session][0])
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}

	util_mvd_video_decoder_changeable_buffer_size = false;
	if(util_mvd_video_decoder_available_raw_image[session] + 1 >= util_mvd_video_decoder_max_raw_image[session])
	{
		result.error_description = "[Error] Queues are full. ";
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
		result.error_description = "[Error] av_frame_alloc() failed. ";
		goto ffmpeg_api_failed;
	}

	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(!util_mvd_video_decoder_raw_image[session][buffer_num]->data[0])
		goto out_of_linear_memory;

	if(util_video_decoder_packet[session][0]->size > util_mvd_video_decoder_packet_size)
	{
		util_mvd_video_decoder_packet_size = util_video_decoder_packet[session][0]->size;
		Util_safe_linear_free(util_mvd_video_decoder_packet);
		util_mvd_video_decoder_packet = NULL;
		util_mvd_video_decoder_packet = (u8*)Util_safe_linear_alloc(util_mvd_video_decoder_packet_size);
		if(!util_mvd_video_decoder_packet)
		{
			util_mvd_video_decoder_packet_size = 0;
			goto out_of_linear_memory;
		}
	}

	if(util_mvd_video_decoder_workarounds)
	{
		//to detect all blue image if mvd service won't write anything to the buffer, fill the buffer by 0x8
		memset((util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * (height / 2)) * 2), 0x8, width * 2);
	}
	
	if(util_mvd_video_decoder_first)
	{
		mvdstdGenerateDefaultConfig(&util_decoder_mvd_config, width, height, width, height, NULL, NULL, NULL);

		//set extra data
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

	while(source_offset + 4 < util_video_decoder_packet[session][0]->size)
	{
		//get nal size
		size = *((int*)(util_video_decoder_packet[session][0]->data + source_offset));
		size = __builtin_bswap32(size);
		source_offset += 4;
		if(source_offset + size > util_video_decoder_packet[session][0]->size || size < 0)
		{
			Util_log_save("debug", "unexpected nal size : " + std::to_string(size));
			break;
		}
		
		//set nal prefix 0x0 0x0 0x1
		memset(util_mvd_video_decoder_packet + offset, 0x0, 0x2);
		offset += 2;
		memset(util_mvd_video_decoder_packet + offset, 0x1, 0x1);
		offset += 1;

		//copy raw nal data
		memcpy(util_mvd_video_decoder_packet + offset, (util_video_decoder_packet[session][0]->data + source_offset), size);
		offset += size;
		source_offset += size;
	}

	result.code = mvdstdProcessVideoFrame(util_mvd_video_decoder_packet, offset, 0, NULL);

	if(util_mvd_video_decoder_first)
	{
		//Do I need to send same nal data at first frame?
		result.code = mvdstdProcessVideoFrame(util_mvd_video_decoder_packet, offset, 0, NULL);
		util_mvd_video_decoder_first = false;
	}

	if(result.code != MVD_STATUS_FRAMEREADY)
	{
		result.error_description = "[Error] mvdstdProcessVideoFrame() failed. ";
		goto nintendo_api_failed;
	}
	result.code = mvdstdRenderVideoFrame(&util_decoder_mvd_config, true);
	
	if(result.code != MVD_STATUS_OK)
	{
		result.error_description = "[Error] mvdstdRenderVideoFrame() failed. ";
		goto nintendo_api_failed;
	}

	util_mvd_video_decoder_raw_image[session][buffer_num]->pts = util_video_decoder_packet[session][0]->pts;
	util_mvd_video_decoder_raw_image[session][buffer_num]->pkt_duration = util_video_decoder_packet[session][0]->duration;
	result.code = 0;

	if(buffer_num + 1 < util_mvd_video_decoder_max_raw_image[session])
		util_mvd_video_decoder_raw_image_current_index[session]++;
	else
		util_mvd_video_decoder_raw_image_current_index[session] = 0;

	svcWaitSynchronization(util_mvd_video_decoder_raw_image_mutex[session], U64_MAX);
	util_mvd_video_decoder_available_raw_image[session]++;
	svcReleaseMutex(util_mvd_video_decoder_raw_image_mutex[session]);

	util_video_decoder_packet_ready[session][0] = false;
	av_packet_free(&util_video_decoder_packet[session][0]);
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	out_of_linear_memory:
	util_video_decoder_packet_ready[session][0] = false;
	Util_safe_linear_free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
	result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
	return result;

	ffmpeg_api_failed:
	util_video_decoder_packet_ready[session][0] = false;
	Util_safe_linear_free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;

	nintendo_api_failed:
	util_video_decoder_packet_ready[session][0] = false;
	Util_safe_linear_free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
	util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_subtitle_decoder_decode(Subtitle_data* subtitle_data, int packet_index, int session)
{
	int ffmpeg_result = 0;
	int decoded = 0;
	size_t cut_pos;
	double timebase = 0;
	std::string text = "";
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_SUBTITLE_TRACKS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_subtitle_decoder_init[session][packet_index])
		goto not_inited;

	if(!util_subtitle_decoder_packet_ready[session][packet_index])
	{
		result.error_description = "[Error] No packet available. ";
		goto try_again;
	}

	subtitle_data->text = "";
	subtitle_data->start_time = 0;
	subtitle_data->end_time = 0;
	
	util_subtitle_decoder_raw_data[session][packet_index] = (AVSubtitle*)Util_safe_linear_alloc(sizeof(AVSubtitle));
	if(!util_subtitle_decoder_raw_data[session][packet_index])
	{
		result.error_description = "[Error] av_subtitle_alloc() failed. ";
		goto ffmpeg_api_failed;
	}
	
	ffmpeg_result = avcodec_decode_subtitle2(util_subtitle_decoder_context[session][packet_index], util_subtitle_decoder_raw_data[session][packet_index], &decoded, util_subtitle_decoder_packet[session][packet_index]);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] avcodec_decode_subtitle2() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	//Util_log_save("debug", "decoded " + std::to_string(decoded));
	if(decoded > 0)
	{
		timebase = av_q2d(util_decoder_format_context[session]->streams[util_subtitle_decoder_stream_num[session][packet_index]]->time_base);
		if(timebase != 0)
		{
			subtitle_data->start_time = (double)util_subtitle_decoder_packet[session][packet_index]->dts * timebase * 1000;//calc pos
			subtitle_data->end_time = subtitle_data->start_time + (util_subtitle_decoder_packet[session][packet_index]->duration * timebase * 1000);
		}

		//Util_log_save("debug", "duration : " + std::to_string(util_subtitle_decoder_packet[session][packet_index]->duration * timebase * 1000));
		
		//Util_log_save("debug", "dts : " + std::to_string(util_subtitle_decoder_packet[session][packet_index]->dts));
		for(uint i = 0; i < util_subtitle_decoder_raw_data[session][packet_index]->num_rects; i++)
		{
			//Util_log_save("debug", "from : " + std::to_string(subtitle_data->start_time) + " to : " + std::to_string(subtitle_data->end_time));

			if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->type == SUBTITLE_NONE)
			{
				//Util_log_save("debug", "type : NONE");
			}
			if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->type == SUBTITLE_BITMAP)
			{
				//Util_log_save("debug", "type : BITMAP");
			}
			if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->type == SUBTITLE_TEXT)
			{
				//Util_log_save("debug", "type : TEXT");
				//Util_log_save("debug", util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->text);
				subtitle_data->text = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->text;
			}
			if(util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->type == SUBTITLE_ASS)
			{
				//Util_log_save("debug", "type : ASS");
				//Util_log_save("debug", util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->ass);

				text = util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->ass;
				cut_pos = 0;
				for(int i = 0; i < 8; i++)
				{
					if(text.length() >= cut_pos + 1)
						cut_pos = text.find(",", cut_pos + 1);
					else
						cut_pos = std::string::npos;

					if(cut_pos == std::string::npos)
						break;
					else if(i == 7)
					{
						text = text.substr(cut_pos + 1);
						while(true)
						{
							cut_pos = text.find("\\N");
							if(cut_pos != std::string::npos)
								text.replace(cut_pos, 2, "\n");
							else
								break;
						}
					}
				}
				/*text_length = text.length();
					for(int i = 1; i <= text_length / 20; i++)
						text.insert(i * 20 + i, "\n");*/

				subtitle_data->text = text;
			}
			//Util_log_save("debug", util_subtitle_decoder_raw_data[session][packet_index]->rects[i]->text);
		}
	}

	util_subtitle_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	avsubtitle_free(util_subtitle_decoder_raw_data[session][packet_index]);
	Util_safe_linear_free(util_subtitle_decoder_raw_data[session][packet_index]);
	util_subtitle_decoder_raw_data[session][packet_index] = NULL;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	ffmpeg_api_failed:
	util_subtitle_decoder_packet_ready[session][packet_index] = false;
	av_packet_free(&util_subtitle_decoder_packet[session][packet_index]);
	avsubtitle_free(util_subtitle_decoder_raw_data[session][packet_index]);
	Util_safe_linear_free(util_subtitle_decoder_raw_data[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_video_decoder_clear_raw_image(int packet_index, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return;
	
	for(int k = 0; k < util_video_decoder_max_raw_image[session][packet_index]; k++)
		av_frame_free(&util_video_decoder_raw_image[session][packet_index][k]);

	util_video_decoder_available_raw_image[session][packet_index] = 0;
	util_video_decoder_raw_image_ready_index[session][packet_index] = 0;
	util_video_decoder_raw_image_current_index[session][packet_index] = 0;
}

void Util_mvd_video_decoder_clear_raw_image(int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return;
	
	for(int i = 0; i < util_mvd_video_decoder_max_raw_image[session]; i++)
	{
		if(util_mvd_video_decoder_raw_image[session][i])
		{
			Util_safe_linear_free(util_mvd_video_decoder_raw_image[session][i]->data[0]);
			for(int k = 0; k < AV_NUM_DATA_POINTERS; k++)
				util_mvd_video_decoder_raw_image[session][i]->data[k] = NULL;
		}
		av_frame_free(&util_mvd_video_decoder_raw_image[session][i]);
	}

	util_mvd_video_decoder_available_raw_image[session] = 0;
	util_mvd_video_decoder_raw_image_ready_index[session] = 0;
	util_mvd_video_decoder_raw_image_current_index[session] = 0;
}

int Util_video_decoder_get_available_raw_image_num(int packet_index, int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return 0;
	else
		return util_video_decoder_available_raw_image[session][packet_index];
}

int Util_mvd_video_decoder_get_available_raw_image_num(int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return 0;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return 0;
	else
		return util_mvd_video_decoder_available_raw_image[session];
}

Result_with_string Util_video_decoder_get_image(u8** raw_data, double* current_pos, int width, int height, int packet_index, int session)
{
	int cpy_size[2] = { 0, 0, };
	int buffer_num = 0;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(!raw_data || !current_pos || width <= 0 || height <= 0 || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS
	|| session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		goto not_inited;
	
	if(util_video_decoder_available_raw_image[session][packet_index] <= 0)
	{
		result.error_description = "[Error] No raw image available. ";
		goto try_again;
	}

	*current_pos = 0;
	Util_safe_linear_free(*raw_data);
	*raw_data = NULL;
	*raw_data = (u8*)Util_safe_linear_alloc(width * height * 1.5);
	if(!*raw_data)
		goto out_of_memory;

	cpy_size[0] = (width * height);
	cpy_size[1] = cpy_size[0] / 4;
	buffer_num = util_video_decoder_raw_image_ready_index[session][packet_index];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.den;
	if(util_video_decoder_raw_image[session][packet_index][buffer_num]->pkt_duration != 0)
		current_frame = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts / util_video_decoder_raw_image[session][packet_index][buffer_num]->pkt_duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts * timebase * 1000;//calc pos
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos

	memcpy_asm(*raw_data, util_video_decoder_raw_image[session][packet_index][buffer_num]->data[0], cpy_size[0]);
	memcpy_asm(*raw_data + (width * height), util_video_decoder_raw_image[session][packet_index][buffer_num]->data[1], cpy_size[1]);
	memcpy_asm(*raw_data + (width * height) + (width * height / 4), util_video_decoder_raw_image[session][packet_index][buffer_num]->data[2], cpy_size[1]);

	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);

	if(util_video_decoder_raw_image_ready_index[session][packet_index] + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_ready_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_ready_index[session][packet_index] = 0;

	svcWaitSynchronization(util_video_decoder_raw_image_mutex[session][packet_index], U64_MAX);
	util_video_decoder_available_raw_image[session][packet_index]--;
	svcReleaseMutex(util_video_decoder_raw_image_mutex[session][packet_index]);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

Result_with_string Util_mvd_video_decoder_get_image(u8** raw_data, double* current_pos, int width, int height, int session)
{
	bool all_blue = true;
	int cpy_size = 0;
	int buffer_num = 0;
	int available_raw_image = 0;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(!raw_data || !current_pos || width <= 0 || height <= 0 || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		goto not_inited;
	
	if(util_mvd_video_decoder_available_raw_image[session] <= 0)
	{
		result.error_description = "[Error] No raw image available. ";
		goto try_again;
	}

	Util_safe_linear_free(*raw_data);
	*raw_data = NULL;
	*raw_data = (u8*)Util_safe_linear_alloc(width * height * 2);
	if(!*raw_data)
		goto out_of_memory;
	
	*current_pos = 0;
	buffer_num = util_mvd_video_decoder_raw_image_ready_index[session];
	available_raw_image = util_mvd_video_decoder_available_raw_image[session];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	if(util_mvd_video_decoder_raw_image[session][buffer_num]->pkt_duration != 0)
		current_frame = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts / util_mvd_video_decoder_raw_image[session][buffer_num]->pkt_duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts * timebase * 1000;//calc pos
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos

	cpy_size = (width * height * 2);

	if(util_mvd_video_decoder_workarounds)
	{
		for(int s = 0; s < available_raw_image; s++)
		{
			for(int i = 0; i + 4 < width; i+= 4)//scan for 0x0808 color horizontally
			{
				if(*(u16*)(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0] + (width * (height / 2) * 2) + i) != 0x0808)
				{
					all_blue = false;
					break;
				}
			}

			if(all_blue && s + 1 < available_raw_image)
			{
				if(buffer_num + 1 < util_mvd_video_decoder_max_raw_image[session])
					buffer_num++;
				else
					buffer_num = 0;
			}
			else
				break;
		}

		memcpy_asm(*raw_data, util_mvd_video_decoder_raw_image[session][buffer_num]->data[0], cpy_size);
	}
	else
		memcpy_asm(*raw_data, util_mvd_video_decoder_raw_image[session][buffer_num]->data[0], cpy_size);

	buffer_num = util_mvd_video_decoder_raw_image_ready_index[session];

	if(util_mvd_video_decoder_raw_image[session][buffer_num])
	{
		Util_safe_linear_free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
		for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_mvd_video_decoder_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);

	if(util_mvd_video_decoder_raw_image_ready_index[session] + 1 < util_mvd_video_decoder_max_raw_image[session])
		util_mvd_video_decoder_raw_image_ready_index[session]++;
	else
		util_mvd_video_decoder_raw_image_ready_index[session] = 0;

	svcWaitSynchronization(util_mvd_video_decoder_raw_image_mutex[session], U64_MAX);
	util_mvd_video_decoder_available_raw_image[session]--;
	svcReleaseMutex(util_mvd_video_decoder_raw_image_mutex[session]);

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
	
	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

void Util_video_decoder_skip_image(double* current_pos, int packet_index, int session)
{
	int buffer_num = 0;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(!current_pos || packet_index < 0 || packet_index > DEF_DECODER_MAX_VIDEO_TRACKS
	|| session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][packet_index])
		return;
	
	if(util_video_decoder_available_raw_image[session][packet_index] <= 0)
		return;

	*current_pos = 0;
	buffer_num = util_video_decoder_raw_image_ready_index[session][packet_index];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.den;
	if(util_video_decoder_raw_image[session][packet_index][buffer_num]->pkt_duration != 0)
		current_frame = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts / util_video_decoder_raw_image[session][packet_index][buffer_num]->pkt_duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_video_decoder_raw_image[session][packet_index][buffer_num]->pts * timebase * 1000;//calc pos
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos

	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);

	if(util_video_decoder_raw_image_ready_index[session][packet_index] + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_ready_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_ready_index[session][packet_index] = 0;

	svcWaitSynchronization(util_video_decoder_raw_image_mutex[session][packet_index], U64_MAX);
	util_video_decoder_available_raw_image[session][packet_index]--;
	svcReleaseMutex(util_video_decoder_raw_image_mutex[session][packet_index]);
}

void Util_mvd_video_decoder_skip_image(double* current_pos, int session)
{
	int buffer_num = 0;
	double framerate = 0;
	double current_frame = 0;
	double timebase = 0;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(!current_pos || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session] || !util_video_decoder_init[session][0] || !util_mvd_video_decoder_init)
		return;
	
	if(util_mvd_video_decoder_available_raw_image[session] <= 0)
		return;
	
	*current_pos = 0;
	buffer_num = util_mvd_video_decoder_raw_image_ready_index[session];
	framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	if(util_mvd_video_decoder_raw_image[session][buffer_num]->pkt_duration != 0)
		current_frame = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts / util_mvd_video_decoder_raw_image[session][buffer_num]->pkt_duration;

	timebase = av_q2d(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->time_base);
	if(timebase != 0)
		*current_pos = (double)util_mvd_video_decoder_raw_image[session][buffer_num]->pts * timebase * 1000;//calc pos
	else if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos

	if(util_mvd_video_decoder_raw_image[session][buffer_num])
	{
		Util_safe_linear_free(util_mvd_video_decoder_raw_image[session][buffer_num]->data[0]);
		for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_mvd_video_decoder_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_mvd_video_decoder_raw_image[session][buffer_num]);
	
	if(util_mvd_video_decoder_raw_image_ready_index[session] + 1 < util_mvd_video_decoder_max_raw_image[session])
		util_mvd_video_decoder_raw_image_ready_index[session]++;
	else
		util_mvd_video_decoder_raw_image_ready_index[session] = 0;

	svcWaitSynchronization(util_mvd_video_decoder_raw_image_mutex[session], U64_MAX);
	util_mvd_video_decoder_available_raw_image[session]--;
	svcReleaseMutex(util_mvd_video_decoder_raw_image_mutex[session]);
}

Result_with_string Util_decoder_seek(u64 seek_pos, int flag, int session)
{
	int ffmpeg_result = 0;
	int ffmpeg_seek_flag = 0;
	Result_with_string result;
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(seek_pos < 0 || session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_decoder_opened_file[session])
		goto not_inited;
	
	if(flag == DEF_DECODER_SEEK_FLAG_BACKWARD)
		ffmpeg_seek_flag |= AVSEEK_FLAG_BACKWARD;
	if(flag == DEF_DECODER_SEEK_FLAG_BYTE)
		ffmpeg_seek_flag |= AVSEEK_FLAG_BYTE;
	if(flag == DEF_DECODER_SEEK_FLAG_ANY)
		ffmpeg_seek_flag |= AVSEEK_FLAG_ANY;
	if(flag == DEF_DECODER_SEEK_FLAG_FRAME)
		ffmpeg_seek_flag |= AVSEEK_FLAG_FRAME;

	ffmpeg_result = avformat_seek_file(util_decoder_format_context[session], -1, INT64_MIN, seek_pos * 1000, INT64_MAX, ffmpeg_seek_flag);//AVSEEK_FLAG_FRAME 8 AVSEEK_FLAG_ANY 4  AVSEEK_FLAG_BACKWORD 1
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] avformat_seek_file() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
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

void Util_audio_decoder_exit(int session)
{
	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
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
			swr_free(&util_audio_decoder_swr_context[session][i]);
		}
	}
}

void Util_video_decoder_exit(int session)
{
	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		if(util_video_decoder_init[session][i])
		{
			util_video_decoder_init[session][i] = false;
			util_video_decoder_cache_packet_ready[session][i] = false;
			util_video_decoder_packet_ready[session][i] = false;
			avcodec_free_context(&util_video_decoder_context[session][i]);
			av_packet_free(&util_video_decoder_packet[session][i]);
			av_packet_free(&util_video_decoder_cache_packet[session][i]);
			svcCloseHandle(util_video_decoder_raw_image_mutex[session][i]);
			util_video_decoder_available_raw_image[session][i] = 0;
			util_video_decoder_raw_image_ready_index[session][i] = 0;
			util_video_decoder_raw_image_current_index[session][i] = 0;
			for(int k = 0; k < util_video_decoder_max_raw_image[session][i]; k++)
				av_frame_free(&util_video_decoder_raw_image[session][i][k]);
		}
	}
}

void Util_mvd_video_decoder_exit(int session)
{
	if(!util_mvd_video_decoder_init)
		return;

	util_mvd_video_decoder_init = false;
	mvdstdExit();
	Util_safe_linear_free(util_mvd_video_decoder_packet);
	util_mvd_video_decoder_packet = NULL;
	util_mvd_video_decoder_available_raw_image[session] = 0;
	util_mvd_video_decoder_raw_image_ready_index[session] = 0;
	util_mvd_video_decoder_raw_image_current_index[session] = 0;
	for(int i = 0; i < util_mvd_video_decoder_max_raw_image[session]; i++)
	{
		if(util_mvd_video_decoder_raw_image[session][i])
		{
			Util_safe_linear_free(util_mvd_video_decoder_raw_image[session][i]->data[0]);
			for(int k = 0; k < AV_NUM_DATA_POINTERS; k++)
				util_mvd_video_decoder_raw_image[session][i]->data[k] = NULL;
		}
		av_frame_free(&util_mvd_video_decoder_raw_image[session][i]);
	}
	svcCloseHandle(util_mvd_video_decoder_raw_image_mutex[session]);
}

void Util_subtitle_decoder_exit(int session)
{
	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
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
				Util_safe_linear_free(util_subtitle_decoder_raw_data[session][i]);
				util_subtitle_decoder_raw_data[session][i] = NULL;
			}
		}
	}
}

void Util_decoder_close_file(int session)
{
	if(!util_decoder_init)
		Util_decoder_init_variables();

	if(session < 0 || session > DEF_DECODER_MAX_SESSIONS)
		return;

	if(!util_decoder_opened_file[session])
		return;

	util_decoder_opened_file[session] = false;
	Util_audio_decoder_exit(session);
	Util_video_decoder_exit(session);
	Util_mvd_video_decoder_exit(session);
	Util_subtitle_decoder_exit(session);
	for(int i = 0; i < DEF_DECODER_MAX_CACHE_PACKETS; i++)
		av_packet_free(&util_decoder_cache_packet[session][i]);

	util_decoder_available_cache_packet[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_cache_packet_ready_index[session] = 0;
	avformat_close_input(&util_decoder_format_context[session]);
	svcCloseHandle(util_decoder_cache_packet_mutex[session]);
}

Result_with_string Util_image_decoder_decode(std::string file_name, u8** raw_data, int* width, int* height)
{
	Result_with_string result;
	int image_ch = 0;
	*raw_data = stbi_load(file_name.c_str(), width, height, &image_ch, STBI_rgb);
	if(!*raw_data)
	{
		result.error_description = (std::string)"[Error] " + stbi_failure_reason();
		goto stbi_api_failed;
	}

	return result;

	stbi_api_failed:
	result.code = DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_STB_IMG_RETURNED_NOT_SUCCESS_STR;
	return result;
}
