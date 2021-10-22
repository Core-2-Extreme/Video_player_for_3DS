#include "system/headers.hpp"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
}

#include "stb_image/stb_image.h"

extern "C" void memcpy_asm(u8*, u8*, int);

int util_audio_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVPacket* util_audio_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVPacket* util_audio_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVFrame* util_audio_decoder_raw_data[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
AVCodecContext* util_audio_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
const AVCodec* util_audio_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];
SwrContext* util_audio_decoder_swr_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_AUDIO_TRACKS];

bool util_video_decoder_mvd_first = false;
int util_video_decoder_available_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_raw_image_ready_index[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_raw_image_current_index[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_mvd_available_raw_image[DEF_DECODER_MAX_SESSIONS];
int util_video_decoder_mvd_raw_image_ready_index[DEF_DECODER_MAX_SESSIONS];
int util_video_decoder_mvd_raw_image_current_index[DEF_DECODER_MAX_SESSIONS];
int util_video_decoder_stream_num[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_mvd_packet_size = 0;
int util_video_decoder_max_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_mvd_max_raw_image[DEF_DECODER_MAX_SESSIONS];

int util_video_decoder_previous_pts[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
int util_video_decoder_increase_per_pts[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];

u8* util_video_decoder_mvd_packet = NULL;
Handle util_video_decoder_raw_image_counter_mutex[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
Handle util_video_decoder_add_raw_image_mutex[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
Handle util_video_decoder_get_raw_image_mutex[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
Handle util_video_decoder_mvd_raw_image_counter_mutex[DEF_DECODER_MAX_SESSIONS];
Handle util_video_decoder_mvd_add_raw_image_mutex[DEF_DECODER_MAX_SESSIONS];
Handle util_video_decoder_mvd_get_raw_image_mutex[DEF_DECODER_MAX_SESSIONS];
AVPacket* util_video_decoder_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
AVPacket* util_video_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
AVFrame* util_video_decoder_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS][DEF_DECODER_MAX_RAW_IMAGE];
AVFrame* util_video_decoder_mvd_raw_image[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_RAW_IMAGE];
AVCodecContext* util_video_decoder_context[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];
const AVCodec* util_video_decoder_codec[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_VIDEO_TRACKS];

int util_decoder_available_cache_packet[DEF_DECODER_MAX_SESSIONS];
int util_decoder_cache_packet_ready_index[DEF_DECODER_MAX_SESSIONS];
int util_decoder_cache_packet_current_index[DEF_DECODER_MAX_SESSIONS];
Handle util_decoder_cache_packet_counter_mutex[DEF_DECODER_MAX_SESSIONS];
Handle util_decoder_read_cache_packet_mutex[DEF_DECODER_MAX_SESSIONS];
Handle util_decoder_parse_cache_packet_mutex[DEF_DECODER_MAX_SESSIONS];
MVDSTD_Config util_decoder_mvd_config;
AVPacket* util_decoder_cache_packet[DEF_DECODER_MAX_SESSIONS][DEF_DECODER_MAX_CACHE_PACKETS];
AVFormatContext* util_decoder_format_context[DEF_DECODER_MAX_SESSIONS];

void Util_video_decoder_free(void *opaque, uint8_t *data)
{
	free(data);
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
		width = avctx->width;
		height = avctx->height;
		if(width % 16 != 0)
			width += 16 - width % 16;
		if(height % 16 != 0)
			height += 16 - height % 16;

		frame->linesize[0] = width;
		frame->linesize[1] = width / 2;
		frame->linesize[2] = width / 2;
		
        frame->data[0] = (u8*)malloc(width * height);
        frame->data[1] = (u8*)malloc(width * height / 4);
        frame->data[2] = (u8*)malloc(width * height / 4);

        frame->buf[0] = av_buffer_create(frame->data[0], frame->linesize[0] * frame->height, Util_video_decoder_free, NULL, 0);
        frame->buf[1] = av_buffer_create(frame->data[1], frame->linesize[1] * frame->height / 4, Util_video_decoder_free, NULL, 0);
        frame->buf[2] = av_buffer_create(frame->data[2], frame->linesize[2] * frame->height / 4, Util_video_decoder_free, NULL, 0);
		return 0;
	}
	else
		return -1;
}

Result_with_string Util_mvd_video_decoder_init(int session)
{
	int width = 0;
	int height = 0;
	Result_with_string result;
	width = util_video_decoder_context[session][0]->width;
	height = util_video_decoder_context[session][0]->height;

	util_video_decoder_mvd_raw_image_current_index[session] = 0;
	util_video_decoder_mvd_raw_image_ready_index[session] = 0;
	util_video_decoder_mvd_available_raw_image[session] = 0;

	result.code = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264, MVD_OUTPUT_BGR565, MVD_DEFAULT_WORKBUF_SIZE * 1.5, NULL);
	//result.code = mvdstdInit(MVDMODE_VIDEOPROCESSING, (MVDSTD_InputFormat)0x00180001, MVD_OUTPUT_BGR565, width * height * 9, NULL);
	if(result.code != 0)
		result.string = "mvdstdInit() failed. ";
	else
	{
		if(width % 16 != 0)
			width += 16 - width % 16;
		if(height % 16 != 0)
			height += 16 - height % 16;
		
		util_video_decoder_mvd_packet_size = 1024 * 256;
		util_video_decoder_mvd_packet = (u8*)Util_safe_linear_alloc(util_video_decoder_mvd_packet_size);
		if(!util_video_decoder_mvd_packet)
		{
			result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
			result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
			Util_mvd_video_decoder_exit(session);
		}
		else
		{
			util_video_decoder_mvd_max_raw_image[session] = DEF_DECODER_MAX_RAW_IMAGE;
			svcCreateMutex(&util_video_decoder_mvd_raw_image_counter_mutex[session], false);
			svcCreateMutex(&util_video_decoder_mvd_add_raw_image_mutex[session], false);
			svcCreateMutex(&util_video_decoder_mvd_get_raw_image_mutex[session], false);
			util_video_decoder_mvd_first = true;
		}
	}

	return result;
}

Result_with_string Util_decoder_open_file(std::string file_path, int* num_of_audio_tracks, int* num_of_video_tracks, int session)
{
	int ffmpeg_result = 0;
	int audio_index = 0;
	int video_index = 0;
	Result_with_string result;
	*num_of_video_tracks = 0;
	*num_of_audio_tracks = 0;

	util_decoder_format_context[session] = avformat_alloc_context();
	ffmpeg_result = avformat_open_input(&util_decoder_format_context[session], file_path.c_str(), NULL, NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avformat_open_input() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}
	
	ffmpeg_result = avformat_find_stream_info(util_decoder_format_context[session], NULL);
	if(util_decoder_format_context[session] == NULL)
	{
		result.error_description = "avformat_find_stream_info() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	svcCreateMutex(&util_decoder_cache_packet_counter_mutex[session], false);
	svcCreateMutex(&util_decoder_read_cache_packet_mutex[session], false);
	svcCreateMutex(&util_decoder_parse_cache_packet_mutex[session], false);
	util_decoder_cache_packet_ready_index[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_available_cache_packet[session] = 0;
	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		util_audio_decoder_stream_num[session][i] = -1;

	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
		util_video_decoder_stream_num[session][i] = -1;

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
	}

	if(audio_index == 0 && video_index == 0)
	{
		result.error_description = "No audio and video data ";
		goto fail;
	}

	*num_of_audio_tracks = audio_index;
	*num_of_video_tracks = video_index;

	return result;

	fail:

	Util_decoder_close_file(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_audio_decoder_init(int num_of_audio_tracks, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;
	
	for(int i = 0; i < num_of_audio_tracks; i++)
	{
		util_audio_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_audio_decoder_codec[session][i])
		{
			result.error_description = "avcodec_find_decoder() failed ";
			goto fail;
		}

		util_audio_decoder_context[session][i] = avcodec_alloc_context3(util_audio_decoder_codec[session][i]);
		if(!util_audio_decoder_context[session][i])
		{
			result.error_description = "avcodec_alloc_context3() failed ";
			goto fail;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_audio_decoder_context[session][i], util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			result.error_description = "avcodec_parameters_to_context() failed " + std::to_string(ffmpeg_result) + " ";
			goto fail;
		}

		ffmpeg_result = avcodec_open2(util_audio_decoder_context[session][i], util_audio_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			result.error_description = "avcodec_open2() failed " + std::to_string(ffmpeg_result) + " ";
			goto fail;
		}

		util_audio_decoder_swr_context[session][i] = swr_alloc();
		swr_alloc_set_opts(util_audio_decoder_swr_context[session][i], av_get_default_channel_layout(util_audio_decoder_context[session][i]->channels), AV_SAMPLE_FMT_S16, util_audio_decoder_context[session][i]->sample_rate,
			av_get_default_channel_layout(util_audio_decoder_context[session][i]->channels), util_audio_decoder_context[session][i]->sample_fmt, util_audio_decoder_context[session][i]->sample_rate, 0, NULL);
		if(!util_audio_decoder_swr_context[session][i])
		{
			result.error_description = "swr_alloc_set_opts() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		ffmpeg_result = swr_init(util_audio_decoder_swr_context[session][i]);
		if(ffmpeg_result != 0)
		{
			result.error_description = "swr_init() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}
	}

	return result;

	fail:

	Util_audio_decoder_exit(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_video_decoder_init(int low_resolution, int num_of_video_tracks, int num_of_threads, int thread_type, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	for (int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		util_video_decoder_raw_image_current_index[session][i] = 0;
		util_video_decoder_raw_image_ready_index[session][i] = 0;
		util_video_decoder_available_raw_image[session][i] = 0;
	}

	for(int i = 0; i < num_of_video_tracks; i++)
	{
		util_video_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_video_decoder_codec[session][i])
		{
			result.error_description = "avcodec_find_decoder() failed ";
			goto fail;
		}

		util_video_decoder_context[session][i] = avcodec_alloc_context3(util_video_decoder_codec[session][i]);
		if(!util_video_decoder_context[session][i])
		{
			result.error_description = "avcodec_alloc_context3() failed ";
			goto fail;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_video_decoder_context[session][i], util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][i]]->codecpar);
		if(ffmpeg_result != 0)
		{
			result.error_description = "avcodec_parameters_to_context() failed " + std::to_string(ffmpeg_result) + " ";
			goto fail;
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
				util_video_decoder_context[session][i]->thread_type = 0;
				util_video_decoder_context[session][i]->thread_count = 1;
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

		util_video_decoder_context[session][i]->get_buffer2 = Util_video_decoder_allocate_yuv420p_buffer;
		ffmpeg_result = avcodec_open2(util_video_decoder_context[session][i], util_video_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			result.error_description = "avcodec_open2() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		util_video_decoder_max_raw_image[session][i] = DEF_DECODER_MAX_RAW_IMAGE;
		util_video_decoder_previous_pts[session][i] = 0;
		util_video_decoder_increase_per_pts[session][i] = 0;
		svcCreateMutex(&util_video_decoder_raw_image_counter_mutex[session][i], false);
		svcCreateMutex(&util_video_decoder_add_raw_image_mutex[session][i], false);
		svcCreateMutex(&util_video_decoder_get_raw_image_mutex[session][i], false);
	}

	return result;

	fail:

	Util_video_decoder_exit(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_audio_decoder_get_info(int* bitrate, int* sample_rate, int* ch, std::string* format_name, double* duration, int audio_index, std::string* track_lang, int session)
{
	*track_lang = "language:und";
	AVDictionaryEntry *data = NULL;

	if(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][audio_index]]->metadata)
	{
		data = av_dict_get(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session][audio_index]]->metadata, "language", data, AV_DICT_IGNORE_SUFFIX);
		if(data)
			*track_lang = (std::string)data->key + ":" + data->value;
	}
		
	*bitrate = util_audio_decoder_context[session][audio_index]->bit_rate;
	*sample_rate = util_audio_decoder_context[session][audio_index]->sample_rate;
	*ch = util_audio_decoder_context[session][audio_index]->channels;
	*format_name = util_audio_decoder_codec[session][audio_index]->long_name;
	*duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
}

void Util_video_decoder_get_info(int* width, int* height, double* framerate, std::string* format_name, double* duration, int* thread_type, int* sar_width, int* sar_height, int video_index, int session)
{
	//Util_log_save("debug", std::to_string(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->sample_aspect_ratio.num) + ":" + std::to_string(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->sample_aspect_ratio.den));
	AVRational sar = av_guess_sample_aspect_ratio(util_decoder_format_context[session], util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]], NULL);
	//Util_log_save("debug", std::to_string(sar.num) + ":" + std::to_string(sar.den));
	if(sar.num == 0 && sar.den == 1)
	{
		*sar_width = 1;
		*sar_height = 1;
	}
	else
	{
		*sar_width = sar.num;
		*sar_height = sar.den;
	}

	*width = util_video_decoder_context[session][video_index]->width;
	*height = util_video_decoder_context[session][video_index]->height;
	*thread_type = util_video_decoder_context[session][video_index]->thread_type;
	*framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][video_index]]->avg_frame_rate.den;
	*format_name = util_video_decoder_codec[session][video_index]->long_name;
	*duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
}

void Util_decoder_clear_cache_packet(int session)
{
	svcWaitSynchronization(util_decoder_read_cache_packet_mutex[session], U64_MAX);
	svcWaitSynchronization(util_decoder_parse_cache_packet_mutex[session], U64_MAX);
	svcWaitSynchronization(util_decoder_cache_packet_counter_mutex[session], U64_MAX);
	
	for(int i = 0; i < DEF_DECODER_MAX_CACHE_PACKETS; i++)
		av_packet_free(&util_decoder_cache_packet[session][i]);

	util_decoder_available_cache_packet[session] = 0;
	util_decoder_cache_packet_current_index[session] = 0;
	util_decoder_cache_packet_ready_index[session] = 0;

	svcReleaseMutex(util_decoder_read_cache_packet_mutex[session]);
	svcReleaseMutex(util_decoder_parse_cache_packet_mutex[session]);
	svcReleaseMutex(util_decoder_cache_packet_counter_mutex[session]);
}

int Util_decoder_get_available_packet_num(int session)
{
	return util_decoder_available_cache_packet[session];
}

Result_with_string Util_decoder_read_packet(int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	svcWaitSynchronization(util_decoder_read_cache_packet_mutex[session], U64_MAX);
	if(util_decoder_available_cache_packet[session] + 1 >= DEF_DECODER_MAX_CACHE_PACKETS)
	{
		svcReleaseMutex(util_decoder_read_cache_packet_mutex[session]);
		result.code = DEF_ERR_TRY_AGAIN;
		result.string = DEF_ERR_TRY_AGAIN_STR;
		result.error_description = "[Error] Queues are full ";
		return result;
	}

	util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]] = av_packet_alloc();
	if(!util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]])
	{
		result.error_description = "av_packet_alloc() failed ";
		goto fail;
	}

	ffmpeg_result = av_read_frame(util_decoder_format_context[session], util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "av_read_frame() failed ";
		goto fail;
	}

	if(util_decoder_cache_packet_ready_index[session] + 1 < DEF_DECODER_MAX_CACHE_PACKETS)
		util_decoder_cache_packet_ready_index[session]++;
	else
		util_decoder_cache_packet_ready_index[session] = 0;

	svcWaitSynchronization(util_decoder_cache_packet_counter_mutex[session], U64_MAX);
	util_decoder_available_cache_packet[session]++;
	svcReleaseMutex(util_decoder_cache_packet_counter_mutex[session]);
	svcReleaseMutex(util_decoder_read_cache_packet_mutex[session]);

	return result;

	fail:

	av_packet_free(&util_decoder_cache_packet[session][util_decoder_cache_packet_ready_index[session]]);
	svcReleaseMutex(util_decoder_read_cache_packet_mutex[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_parse_packet(std::string* type, int* packet_index, bool* key_frame, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;
	//int packet_buffer_index = 0;
	//int available_packet = 0;
	*key_frame = false;
	*packet_index = -1;
	*type = "unknown";

	svcWaitSynchronization(util_decoder_parse_cache_packet_mutex[session], U64_MAX);
	if(util_decoder_available_cache_packet[session] <= 0)
	{
		svcReleaseMutex(util_decoder_parse_cache_packet_mutex[session]);
		result.code = DEF_ERR_TRY_AGAIN;
		result.string = DEF_ERR_TRY_AGAIN_STR;
		result.error_description = "[Error] No packet available ";
		return result;
	}
	//available_packet = util_decoder_available_cache_packet[session];

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)	
	{
		if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_audio_decoder_stream_num[session][i])//audio packet
		{
			util_audio_decoder_cache_packet[session][i] = av_packet_alloc();
			if(!util_audio_decoder_cache_packet[session][i])
			{
				result.error_description = "av_packet_alloc() failed" + std::to_string(ffmpeg_result);
				av_packet_free(&util_audio_decoder_cache_packet[session][i]);
				goto fail;
			}

			av_packet_unref(util_audio_decoder_cache_packet[session][i]);
			ffmpeg_result = av_packet_ref(util_audio_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
			if(ffmpeg_result != 0)
			{
				result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result);
				av_packet_free(&util_audio_decoder_cache_packet[session][i]);
				goto fail;
			}
			*packet_index = i;
			*type = "audio";
			break;
		}
	}

	/*if(var_debug_bool[0])
	{
		if(*type == "unknown")
		{
			for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
			{
				if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_video_decoder_stream_num[session][i])//video packet
				{
					if(util_video_decoder_increase_per_pts[session][i] == 0)
					{
						util_video_decoder_increase_per_pts[session][i] = util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->duration;
						util_video_decoder_previous_pts[session][i] = -util_video_decoder_increase_per_pts[session][i];
					}

					packet_buffer_index = util_decoder_cache_packet_current_index[session];
					Util_log_save("debug", "started");
					for(int k = 0; k < available_packet; k++)
					{
						if(util_decoder_cache_packet[session][packet_buffer_index]->stream_index == util_video_decoder_stream_num[session][i])
						{
							Util_log_save("debug", "expected : " + std::to_string(util_video_decoder_previous_pts[session][i] + util_video_decoder_increase_per_pts[session][i]) + " got : " + std::to_string(util_decoder_cache_packet[session][packet_buffer_index]->pts));
							if(util_video_decoder_previous_pts[session][i] + util_video_decoder_increase_per_pts[session][i] == util_decoder_cache_packet[session][packet_buffer_index]->pts)
							{
								util_video_decoder_previous_pts[session][i] += util_video_decoder_increase_per_pts[session][i];
								Util_log_save("debug", "found");
								util_video_decoder_cache_packet[session][i] = av_packet_alloc();
								if(!util_video_decoder_cache_packet[session][i])
								{
									result.error_description = "av_packet_alloc() failed ";
									av_packet_free(&util_video_decoder_cache_packet[session][i]);
									goto fail;
								}

								av_packet_unref(util_video_decoder_cache_packet[session][i]);
								ffmpeg_result = av_packet_ref(util_video_decoder_cache_packet[session][i], util_decoder_cache_packet[session][packet_buffer_index]);
								if(ffmpeg_result != 0)
								{
									result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result) + " ";
									av_packet_free(&util_video_decoder_cache_packet[session][i]);
									goto fail;
								}

								if(util_decoder_cache_packet_current_index[session] != packet_buffer_index)
								{
									av_packet_free(&util_decoder_cache_packet[session][packet_buffer_index]);
									util_decoder_cache_packet[session][packet_buffer_index] = av_packet_alloc();
									if(!util_decoder_cache_packet[session][packet_buffer_index])
									{
										result.error_description = "av_packet_alloc() failed ";
										av_packet_free(&util_decoder_cache_packet[session][packet_buffer_index]);
										goto fail;
									}

									ffmpeg_result = av_packet_ref(util_decoder_cache_packet[session][packet_buffer_index], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
									if(ffmpeg_result != 0)
									{
										result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result) + " ";
										av_packet_free(&util_video_decoder_cache_packet[session][i]);
										goto fail;
									}
								}
								*packet_index = i;
								*type = "video";
								*key_frame = util_video_decoder_cache_packet[session][i]->flags & AV_PKT_FLAG_KEY;

								goto success;
							}
						}

						if(packet_buffer_index + 1 < DEF_DECODER_MAX_CACHE_PACKETS)
							packet_buffer_index++;
						else
							packet_buffer_index = 0;
					}
					Util_log_save("debug", "not found");
					result.error_description = "not found";
					goto fail;
				}
			}
		}
	}
	else
	{*/
		if(*type == "unknown")
		{
			for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
			{
				if(util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->stream_index == util_video_decoder_stream_num[session][i])//video packet
				{

					if(util_video_decoder_increase_per_pts[session][i] == 0)
					{
						util_video_decoder_increase_per_pts[session][i] = util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]->duration;
						util_video_decoder_previous_pts[session][i] = -util_video_decoder_increase_per_pts[session][i];
					}

					util_video_decoder_cache_packet[session][i] = av_packet_alloc();
					if(!util_video_decoder_cache_packet[session][i])
					{
						result.error_description = "av_packet_alloc() failed ";
						av_packet_free(&util_video_decoder_cache_packet[session][i]);
						goto fail;
					}

					av_packet_unref(util_video_decoder_cache_packet[session][i]);
					ffmpeg_result = av_packet_ref(util_video_decoder_cache_packet[session][i], util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
					if(ffmpeg_result != 0)
					{
						result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result) + " ";
						av_packet_free(&util_video_decoder_cache_packet[session][i]);
						goto fail;
					}
					*packet_index = i;
					*type = "video";
					*key_frame = util_video_decoder_cache_packet[session][i]->flags & AV_PKT_FLAG_KEY;
					break;
				}
			}
		}
	//}

	//success:

	av_packet_free(&util_decoder_cache_packet[session][util_decoder_cache_packet_current_index[session]]);
	if(util_decoder_cache_packet_current_index[session] + 1 < DEF_DECODER_MAX_CACHE_PACKETS)
		util_decoder_cache_packet_current_index[session]++;
	else
		util_decoder_cache_packet_current_index[session] = 0;
	
	svcWaitSynchronization(util_decoder_cache_packet_counter_mutex[session], U64_MAX);
	util_decoder_available_cache_packet[session]--;
	svcReleaseMutex(util_decoder_cache_packet_counter_mutex[session]);
	svcReleaseMutex(util_decoder_parse_cache_packet_mutex[session]);
	return result;

	fail:

	svcReleaseMutex(util_decoder_parse_cache_packet_mutex[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_ready_audio_packet(int packet_index, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	util_audio_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_audio_decoder_packet[session][packet_index])
	{
		result.error_description = "av_packet_alloc() failed ";
		goto fail;
	}

	av_packet_unref(util_audio_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_audio_decoder_packet[session][packet_index], util_audio_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result) + " ";
		goto fail;
	}

	av_packet_free(&util_audio_decoder_cache_packet[session][packet_index]);
	return result;

	fail:

	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_packet_free(&util_audio_decoder_cache_packet[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_ready_video_packet(int packet_index, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	util_video_decoder_packet[session][packet_index] = av_packet_alloc();
	if(!util_video_decoder_packet[session][packet_index])
	{
		result.error_description = "av_packet_alloc() failed ";
		goto fail;
	}

	av_packet_unref(util_video_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_video_decoder_packet[session][packet_index], util_video_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result) + " ";
		goto fail;
	}

	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
	return result;

	fail:

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_decoder_skip_audio_packet(int packet_index, int session)
{
	av_packet_free(&util_audio_decoder_cache_packet[session][packet_index]);
}

void Util_decoder_skip_video_packet(int packet_index, int session)
{
	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
}

void Util_video_decoder_set_raw_image_buffer_size(int max_num_of_buffer, int session, int packet_index)
{
	util_video_decoder_max_raw_image[session][packet_index] = max_num_of_buffer;
}

void Util_mvd_video_decoder_set_raw_image_buffer_size(int max_num_of_buffer, int session)
{
	util_video_decoder_mvd_max_raw_image[session] = max_num_of_buffer;
}

Result_with_string Util_audio_decoder_decode(int* size, u8** raw_data, double* current_pos, int packet_index, int session)
{
	int ffmpeg_result = 0;
	double current_frame = (double)util_audio_decoder_packet[session][packet_index]->dts / util_audio_decoder_packet[session][packet_index]->duration;
	Result_with_string result;
	*size = 0;
	*current_pos = 0;
	
	util_audio_decoder_raw_data[session][packet_index] = av_frame_alloc();
	if(!util_audio_decoder_raw_data[session][packet_index])
	{
		result.error_description = "av_frame_alloc() failed ";
		goto fail;
	}
	
	ffmpeg_result = avcodec_send_packet(util_audio_decoder_context[session][packet_index], util_audio_decoder_packet[session][packet_index]);
	if(ffmpeg_result == 0)
	{
		ffmpeg_result = avcodec_receive_frame(util_audio_decoder_context[session][packet_index], util_audio_decoder_raw_data[session][packet_index]);
		if(ffmpeg_result == 0)
		{
			*current_pos = current_frame * ((1000.0 / util_audio_decoder_raw_data[session][packet_index]->sample_rate) * util_audio_decoder_raw_data[session][packet_index]->nb_samples);//calc pos

			*raw_data = (u8*)malloc(util_audio_decoder_raw_data[session][packet_index]->nb_samples * 2 * util_audio_decoder_context[session][packet_index]->channels);
			*size = swr_convert(util_audio_decoder_swr_context[session][packet_index], raw_data, util_audio_decoder_raw_data[session][packet_index]->nb_samples, (const uint8_t**)util_audio_decoder_raw_data[session][packet_index]->data, util_audio_decoder_raw_data[session][packet_index]->nb_samples);
			*size *= 2;
		}
		else
		{
			result.error_description = "avcodec_receive_frame() failed " + std::to_string(ffmpeg_result) + " ";
			goto fail;
		}
	}
	else
	{
		result.error_description = "avcodec_send_packet() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_frame_free(&util_audio_decoder_raw_data[session][packet_index]);
	return result;

	fail:

	av_packet_free(&util_audio_decoder_packet[session][packet_index]);
	av_frame_free(&util_audio_decoder_raw_data[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_video_decoder_decode(int* width, int* height, double* current_pos, int packet_index, int session)
{
	int ffmpeg_result = 0;
	//int count = 0;
	int buffer_num = 0;
	double framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][packet_index]]->avg_frame_rate.den;
	double current_frame = (double)util_video_decoder_packet[session][packet_index]->dts / util_video_decoder_packet[session][packet_index]->duration;
	Result_with_string result;

	svcWaitSynchronization(util_video_decoder_add_raw_image_mutex[session][packet_index], U64_MAX);
	if(util_video_decoder_available_raw_image[session][packet_index] + 1 >= util_video_decoder_max_raw_image[session][packet_index])
	{
		svcReleaseMutex(util_video_decoder_add_raw_image_mutex[session][packet_index]);
		result.code = DEF_ERR_TRY_AGAIN;
		result.string = DEF_ERR_TRY_AGAIN_STR;
		result.error_description = "[Error] Queues are full ";
		return result;
	}
	
	buffer_num = util_video_decoder_raw_image_current_index[session][packet_index];
	*width = util_video_decoder_context[session][0]->width;
	*height = util_video_decoder_context[session][0]->height;
	if(*width % 16 != 0)
		*width += 16 - *width % 16;
	if(*height % 16 != 0)
		*height += 16 - *height % 16;

	*current_pos = 0;
	if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos
	
	util_video_decoder_raw_image[session][packet_index][buffer_num] = av_frame_alloc();
	if(!util_video_decoder_raw_image[session][packet_index][buffer_num])
	{
		result.error_description = "av_frame_alloc() failed ";
		goto fail;
	}

	ffmpeg_result = avcodec_send_packet(util_video_decoder_context[session][packet_index], util_video_decoder_packet[session][packet_index]);
	if(ffmpeg_result == 0)
	{
		ffmpeg_result = avcodec_receive_frame(util_video_decoder_context[session][packet_index], util_video_decoder_raw_image[session][packet_index][buffer_num]);
		if(ffmpeg_result == 0)
		{
			*width = util_video_decoder_raw_image[session][packet_index][buffer_num]->width;
			*height = util_video_decoder_raw_image[session][packet_index][buffer_num]->height;
		}
		else
		{
			result.error_description = "avcodec_receive_frame() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}
	}
	else
	{
		result.error_description = "avcodec_send_packet() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	if(buffer_num + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_current_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_current_index[session][packet_index] = 0;

	svcReleaseMutex(util_video_decoder_add_raw_image_mutex[session][packet_index]);

	svcWaitSynchronization(util_video_decoder_raw_image_counter_mutex[session][packet_index], U64_MAX);
	util_video_decoder_available_raw_image[session][packet_index]++;
	svcReleaseMutex(util_video_decoder_raw_image_counter_mutex[session][packet_index]);

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	return result;

	fail:

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	/*if(util_video_decoder_raw_image[session][packet_index][buffer_num])
	{
		for(int i = 0; i < 3; i++)
		{
			Util_safe_linear_free(util_video_decoder_raw_image[session][packet_index][buffer_num]->data[i]);
			util_video_decoder_raw_image[session][packet_index][buffer_num]->data[i] = NULL;
		}
		av_freep(&util_video_decoder_raw_image[session][packet_index][buffer_num]);
	}*/
	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);
	svcReleaseMutex(util_video_decoder_add_raw_image_mutex[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_mvd_video_decoder_decode(int* width, int* height, double* current_pos, int session)
{
	int offset = 0;
	int source_offset = 0;
	int size = 0;
	int buffer_num = 0;
	double framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	double current_frame = (double)util_video_decoder_packet[session][0]->dts / util_video_decoder_packet[session][0]->duration;
	Result_with_string result;

	svcWaitSynchronization(util_video_decoder_mvd_add_raw_image_mutex[session], U64_MAX);
	if(util_video_decoder_mvd_available_raw_image[session] + 1 >= util_video_decoder_mvd_max_raw_image[session])
	{
		svcReleaseMutex(util_video_decoder_mvd_add_raw_image_mutex[session]);
		result.code = DEF_ERR_TRY_AGAIN;
		result.string = DEF_ERR_TRY_AGAIN_STR;
		result.error_description = "[Error] Queues are full ";
		return result;
	}

	//Util_log_save("debug", "dts : " + std::to_string((double)util_video_decoder_packet[session][0]->dts / util_video_decoder_packet[session][0]->duration) + " pts : " + std::to_string((double)util_video_decoder_packet[session][0]->pts / util_video_decoder_packet[session][0]->duration));
	
	buffer_num = util_video_decoder_mvd_raw_image_current_index[session];
	*width = util_video_decoder_context[session][0]->width;
	*height = util_video_decoder_context[session][0]->height;

	if(*width % 16 != 0)
		*width += 16 - *width % 16;
	if(*height % 16 != 0)
		*height += 16 - *height % 16;

	*current_pos = 0;
	if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos
	
	util_video_decoder_mvd_raw_image[session][buffer_num] = av_frame_alloc();
	if(!util_video_decoder_mvd_raw_image[session][buffer_num])
	{
		result.error_description = "av_frame_alloc() failed ";
		goto fail_f;
	}

	/*if(av_image_alloc(util_video_decoder_mvd_raw_image[session][buffer_num]->data,
	util_video_decoder_mvd_raw_image[session][buffer_num]->linesize, *width, *height, AV_PIX_FMT_RGB24, 32) < 0)
	{
		result.error_description = "av_image_alloc() failed ";
		goto fail_f;
	}*/

	/*util_video_decoder_mvd_raw_image[session][buffer_num]->data[0] = (u8*)malloc(*width * *height * 2);
	if(!util_video_decoder_mvd_raw_image[session][buffer_num]->data[0])
		goto fail;*/

	util_video_decoder_mvd_raw_image[session][buffer_num]->data[0] = (u8*)Util_safe_linear_alloc(*width * *height * 2);
	if(!util_video_decoder_mvd_raw_image[session][buffer_num]->data[0])
		goto fail;

	//to detect all blue image if mvd service won't write anything to the buffer, fill the buffer by 0x8
	memset((util_video_decoder_mvd_raw_image[session][buffer_num]->data[0] + (*width * (*height / 2))), 0x8, *width * 2);

	if(util_video_decoder_packet[session][0]->size > util_video_decoder_mvd_packet_size)
	{
		util_video_decoder_mvd_packet_size = util_video_decoder_packet[session][0]->size;
		Util_safe_linear_free(util_video_decoder_mvd_packet);
		util_video_decoder_mvd_packet = NULL;
		util_video_decoder_mvd_packet = (u8*)Util_safe_linear_alloc(util_video_decoder_mvd_packet_size);
		if(!util_video_decoder_mvd_packet)
			goto fail;
	}
	
	if(util_video_decoder_mvd_first)
	{
		mvdstdGenerateDefaultConfig(&util_decoder_mvd_config, *width, *height, *width, *height, NULL, NULL, NULL);
		//util_decoder_mvd_config.output_type = (MVDSTD_OutputFormat)0x00040000;

		//set extra data
		offset = 0;
		memset(util_video_decoder_mvd_packet, 0x0, 0x2);
		offset += 2;
		memset(util_video_decoder_mvd_packet + offset, 0x1, 0x1);
		offset += 1;
		memcpy(util_video_decoder_mvd_packet + offset, util_video_decoder_context[session][0]->extradata + 8, *(util_video_decoder_context[session][0]->extradata + 7));
		offset += *(util_video_decoder_context[session][0]->extradata + 7);

		//mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, var_debug_int[0], NULL);
		mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, 0, NULL);

		offset = 0;
		memset(util_video_decoder_mvd_packet, 0x0, 0x2);
		offset += 2;
		memset(util_video_decoder_mvd_packet + offset, 0x1, 0x1);
		offset += 1;
		memcpy(util_video_decoder_mvd_packet + offset, util_video_decoder_context[session][0]->extradata + 11 + *(util_video_decoder_context[session][0]->extradata + 7), *(util_video_decoder_context[session][0]->extradata + 10 + *(util_video_decoder_context[session][0]->extradata + 7)));
		offset += *(util_video_decoder_context[session][0]->extradata + 10 + *(util_video_decoder_context[session][0]->extradata + 7));

		//mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, var_debug_int[0], NULL);
		mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, 0, NULL);
	}
	util_decoder_mvd_config.physaddr_outdata0 = osConvertVirtToPhys(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0]);

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
			goto fail_m;
		}
		
		//set nal prefix 0x0 0x0 0x1
		memset(util_video_decoder_mvd_packet + offset, 0x0, 0x2);
		offset += 2;
		memset(util_video_decoder_mvd_packet + offset, 0x1, 0x1);
		offset += 1;

		//copy raw nal data
		memcpy(util_video_decoder_mvd_packet + offset, (util_video_decoder_packet[session][0]->data + source_offset), size);
		offset += size;
		source_offset += size;
	}

	//result.code = mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, var_debug_int[0], NULL);
	result.code = mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, 0, NULL);
	//Util_log_save("debug", "mvdstdProcessVideoFrame", result.code);

	if(util_video_decoder_mvd_first)
	{
		//Do I need to send same nal data at first frame?
		//result.code = mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, var_debug_int[0], NULL);
		result.code = mvdstdProcessVideoFrame(util_video_decoder_mvd_packet, offset, 0, NULL);
		util_video_decoder_mvd_first = false;
	}

	if(result.code == MVD_STATUS_FRAMEREADY)
	{
		result.code = mvdstdRenderVideoFrame(&util_decoder_mvd_config, true);

		if(result.code == MVD_STATUS_OK)
		{
			//util_video_decoder_mvd_raw_image[session][buffer_num]->pts = util_video_decoder_packet[session][0]->pts;
			//Util_file_save_to_file("pts_" + std::to_string(util_video_decoder_packet[session][0]->pts / util_video_decoder_increase_per_pts[session][0]) + "_dts_" + std::to_string(util_video_decoder_packet[session][0]->dts / util_video_decoder_increase_per_pts[session][0]), DEF_MAIN_DIR + "debug/", 
			//util_video_decoder_mvd_raw_image[session][buffer_num]->data[0], *width * *height * 2, true);
			result.code = 0;
		}
		else
		{
			result.string = "mvdstdRenderVideoFrame() failed ";
			goto fail_m;
		}
	}
	else
	{
		result.string = "mvdstdProcessVideoFrame() failed ";
		goto fail_m;
	}

	if(buffer_num + 1 < util_video_decoder_mvd_max_raw_image[session])
		util_video_decoder_mvd_raw_image_current_index[session]++;
	else
		util_video_decoder_mvd_raw_image_current_index[session] = 0;

	svcReleaseMutex(util_video_decoder_mvd_add_raw_image_mutex[session]);

	svcWaitSynchronization(util_video_decoder_mvd_raw_image_counter_mutex[session], U64_MAX);
	util_video_decoder_mvd_available_raw_image[session]++;
	svcReleaseMutex(util_video_decoder_mvd_raw_image_counter_mutex[session]);

	av_packet_free(&util_video_decoder_packet[session][0]);
	return result;

	fail:

	av_packet_free(&util_video_decoder_packet[session][0]);
	if(util_video_decoder_mvd_raw_image[session][buffer_num])
	{
		Util_safe_linear_free(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0]);
		for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_video_decoder_mvd_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_video_decoder_mvd_raw_image[session][buffer_num]);
	svcReleaseMutex(util_video_decoder_mvd_add_raw_image_mutex[session]);
	result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
	result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
	return result;

	fail_m:

	av_packet_free(&util_video_decoder_packet[session][0]);
	if(util_video_decoder_mvd_raw_image[session][buffer_num])
	{
		Util_safe_linear_free(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0]);
		for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_video_decoder_mvd_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_video_decoder_mvd_raw_image[session][buffer_num]);
	svcReleaseMutex(util_video_decoder_mvd_add_raw_image_mutex[session]);
	return result;

	fail_f:

	av_packet_free(&util_video_decoder_packet[session][0]);
	if(util_video_decoder_mvd_raw_image[session][buffer_num])
	{
		Util_safe_linear_free(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0]);
		for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_video_decoder_mvd_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_video_decoder_mvd_raw_image[session][buffer_num]);
	svcReleaseMutex(util_video_decoder_mvd_add_raw_image_mutex[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_video_decoder_clear_raw_image(int session, int packet_index)
{
	svcWaitSynchronization(util_video_decoder_get_raw_image_mutex[session][packet_index], U64_MAX);
	svcWaitSynchronization(util_video_decoder_add_raw_image_mutex[session][packet_index], U64_MAX);
	svcWaitSynchronization(util_video_decoder_raw_image_counter_mutex[session][packet_index], U64_MAX);
	
	for(int k = 0; k < util_video_decoder_max_raw_image[session][packet_index]; k++)
	{
		/*if(util_video_decoder_raw_image[session][packet_index][k])
		{
			av_freep(&util_video_decoder_raw_image[session][packet_index][k]);
		}*/
		av_frame_free(&util_video_decoder_raw_image[session][packet_index][k]);
	}

	util_video_decoder_available_raw_image[session][packet_index] = 0;
	util_video_decoder_raw_image_ready_index[session][packet_index] = 0;
	util_video_decoder_raw_image_current_index[session][packet_index] = 0;

	svcReleaseMutex(util_video_decoder_get_raw_image_mutex[session][packet_index]);
	svcReleaseMutex(util_video_decoder_add_raw_image_mutex[session][packet_index]);
	svcReleaseMutex(util_video_decoder_raw_image_counter_mutex[session][packet_index]);
}

void Util_mvd_video_decoder_clear_raw_image(int session)
{
	svcWaitSynchronization(util_video_decoder_mvd_get_raw_image_mutex[session], U64_MAX);
	svcWaitSynchronization(util_video_decoder_mvd_add_raw_image_mutex[session], U64_MAX);
	svcWaitSynchronization(util_video_decoder_mvd_raw_image_counter_mutex[session], U64_MAX);
	
	for(int i = 0; i < util_video_decoder_mvd_max_raw_image[session]; i++)
	{
		if(util_video_decoder_mvd_raw_image[session][i])
		{
			Util_safe_linear_free(util_video_decoder_mvd_raw_image[session][i]->data[0]);
			for(int k = 0; k < AV_NUM_DATA_POINTERS; k++)
				util_video_decoder_mvd_raw_image[session][i]->data[k] = NULL;
		}
		av_frame_free(&util_video_decoder_mvd_raw_image[session][i]);
	}

	util_video_decoder_mvd_available_raw_image[session] = 0;
	util_video_decoder_mvd_raw_image_ready_index[session] = 0;
	util_video_decoder_mvd_raw_image_current_index[session] = 0;

	svcReleaseMutex(util_video_decoder_mvd_get_raw_image_mutex[session]);
	svcReleaseMutex(util_video_decoder_mvd_add_raw_image_mutex[session]);
	svcReleaseMutex(util_video_decoder_mvd_raw_image_counter_mutex[session]);
}

int Util_video_decoder_get_available_raw_image_num(int session, int packet_index)
{
	return util_video_decoder_available_raw_image[session][packet_index];
}

int Util_mvd_video_decoder_get_available_raw_image_num(int session)
{
	return util_video_decoder_mvd_available_raw_image[session];
}

Result_with_string Util_video_decoder_get_image(u8** raw_data, int width, int height, int packet_index, int session)
{
	int cpy_size[2] = { 0, 0, };
	int buffer_num = 0;
	//u32 command[8];
	Result_with_string result;
	svcWaitSynchronization(util_video_decoder_get_raw_image_mutex[session][packet_index], U64_MAX);
	if(util_video_decoder_available_raw_image[session][packet_index] <= 0)
	{
		svcReleaseMutex(util_video_decoder_get_raw_image_mutex[session][packet_index]);
		result.code = DEF_ERR_TRY_AGAIN;
		result.string = DEF_ERR_TRY_AGAIN_STR;
		result.error_description = "[Error] No raw image available ";
		return result;
	}

	cpy_size[0] = (width * height);
	cpy_size[1] = cpy_size[0] / 4;
	*raw_data = (u8*)malloc(width * height * 1.5);
	if(*raw_data == NULL)
	{
		svcReleaseMutex(util_video_decoder_get_raw_image_mutex[session][packet_index]);
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	/*Util_log_save("debug", "GX_TextureCopy()...", GX_TextureCopy((u32*)util_video_decoder_raw_image[session][buffer_num]->data[0], cpy_size[0], (u32*)*raw_data, cpy_size[0], cpy_size[0], 8));
	gspWaitForPPF();
	Util_log_save("debug", "GX_TextureCopy()...", GX_TextureCopy((u32*)util_video_decoder_raw_image[session][buffer_num]->data[1], cpy_size[1], (u32*)(*raw_data + (width * height)), cpy_size[1], cpy_size[1], 8));
	gspWaitForPPF();
	Util_log_save("debug", "GX_TextureCopy()...", GX_TextureCopy((u32*)util_video_decoder_raw_image[session][buffer_num]->data[2], cpy_size[1], (u32*)(*raw_data + (width * height) + (width * height / 4)), cpy_size[1], cpy_size[1], 8));
	gspWaitForPPF();*/

	/*command[0] = 0x01000100 | 0x04; //CommandID
	command[1] = (u32)((u8*)util_video_decoder_raw_image[session][buffer_num]->data[0]);
	command[2] = (u32)((u8*)*raw_data);
	command[3] = cpy_size[0];
	command[4] = 0;
	command[5] = 0;
	command[6] = 8;
	command[7] = 0x0;
	gspSubmitGxCommand((const u32*)command);
	gspWaitForPPF();

	command[0] = 0x01000100 | 0x04; //CommandID
	command[1] = (u32)((u8*)util_video_decoder_raw_image[session][buffer_num]->data[1]);
	command[2] = (u32)((u8*)*raw_data + (width * height));
	command[3] = cpy_size[1];
	command[4] = 0;
	command[5] = 0;
	command[6] = 8;
	command[7] = 0x0;
	gspSubmitGxCommand((const u32*)command);
	gspWaitForPPF();

	command[0] = 0x01000100 | 0x04; //CommandID
	command[1] = (u32)((u8*)util_video_decoder_raw_image[session][buffer_num]->data[2]);
	command[2] = (u32)((u8*)*raw_data + (width * height) + (width * height / 4));
	command[3] = cpy_size[1];
	command[4] = 0;
	command[5] = 0;
	command[6] = 8;
	command[7] = 0x0;
	gspSubmitGxCommand((const u32*)command);
	gspWaitForPPF();*/

	buffer_num = util_video_decoder_raw_image_ready_index[session][packet_index];
	memcpy_asm(*raw_data, util_video_decoder_raw_image[session][packet_index][buffer_num]->data[0], cpy_size[0]);
	memcpy_asm(*raw_data + (width * height), util_video_decoder_raw_image[session][packet_index][buffer_num]->data[1], cpy_size[1]);
	memcpy_asm(*raw_data + (width * height) + (width * height / 4), util_video_decoder_raw_image[session][packet_index][buffer_num]->data[2], cpy_size[1]);

	/*if(util_video_decoder_raw_image[session][packet_index][buffer_num])
	{
		for(int i = 0; i < 3; i++)
		{
			Util_safe_linear_free(util_video_decoder_raw_image[session][packet_index][buffer_num]->data[i]);
			util_video_decoder_raw_image[session][packet_index][buffer_num]->data[i] = NULL;
		}
		av_freep(&util_video_decoder_raw_image[session][packet_index][buffer_num]);
	}*/
	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);

	if(util_video_decoder_raw_image_ready_index[session][packet_index] + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_ready_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_ready_index[session][packet_index] = 0;

	svcWaitSynchronization(util_video_decoder_raw_image_counter_mutex[session][packet_index], U64_MAX);
	util_video_decoder_available_raw_image[session][packet_index]--;
	svcReleaseMutex(util_video_decoder_raw_image_counter_mutex[session][packet_index]);
	svcReleaseMutex(util_video_decoder_get_raw_image_mutex[session][packet_index]);

	return result;
}

Result_with_string Util_mvd_video_decoder_get_image(u8** raw_data, int width, int height, int session)
{
	bool all_blue = true;
	int cpy_size = 0;
	int buffer_num = 0;
	int available_raw_image = 0;
	//u32 command[0x8];
	Result_with_string result;

	svcWaitSynchronization(util_video_decoder_mvd_get_raw_image_mutex[session], U64_MAX);
	if(util_video_decoder_mvd_available_raw_image[session] <= 0)
	{
		svcReleaseMutex(util_video_decoder_mvd_get_raw_image_mutex[session]);
		result.code = DEF_ERR_TRY_AGAIN;
		result.string = DEF_ERR_TRY_AGAIN_STR;
		result.error_description = "[Error] No raw image available ";
		return result;
	}
	available_raw_image = util_video_decoder_mvd_available_raw_image[session];

	cpy_size = (width * height * 2);

	//int log = Util_log_save("", "");

	/*
	GX_TextureCopy((u32*)util_video_decoder_mvd_raw_image, cpy_size, (u32*)*raw_data, cpy_size, cpy_size, var_debug_int[0]);
	Util_log_add(log, "");*/

	/*command[0] = 0x01000100 | 0x04; //CommandID
	command[1] = (u32)((u8*)util_video_decoder_mvd_raw_image);
	command[2] = (u32)((u8*)*raw_data);
	command[3] = cpy_size;
	command[4] = 0;
	command[5] = 0;
	command[6] = 8;
	command[7] = 0x0;
	gspSubmitGxCommand((const u32*)command);
	gspWaitForPPF();
	GSPGPU_FlushDataCache((u32*)*raw_data, cpy_size);*/
	//GX_ProcessCommandList(command, 8, 0);
	//GX_BindQueue(&test_queue);
	//GX_TextureCopy((u32*)util_video_decoder_mvd_raw_image, 0, (u32*)*raw_data, 0, cpy_size, 8);
	//Util_log_save("debug", "GX_TextureCopy()...", GX_TextureCopy((u32*)util_video_decoder_mvd_raw_image, 0, (u32*)*raw_data, 0, cpy_size, 8));
	//C3D_ProcessQueue();
	//gxCmdQueueAdd(&test_queue, (gxCmdEntry_s*)command);
	/*gxCmdQueueRun(&test_queue);
	gxCmdQueueWait(&test_queue, U64_MAX);
	gxCmdQueueStop(&test_queue);
	gxCmdQueueClear(&test_queue);
	usleep(100000);
	
	C3D_RestoreGxQueue();
	free(test_queue.entries);
	test_queue.entries = NULL;*/
	buffer_num = util_video_decoder_mvd_raw_image_ready_index[session];

	if(var_debug_bool[0])
	{
		*raw_data = (u8*)malloc(width * height * 2);
		if(*raw_data == NULL)
		{
			result.code = DEF_ERR_OUT_OF_MEMORY;
			result.string = DEF_ERR_OUT_OF_MEMORY_STR;
			return result;
		}

		for(int s = 0; s < available_raw_image; s++)
		{
			for(int i = 0; i + 4 < width; i+= 4)//scan for 0x0808 color horizontally
			{
				if(*(u16*)(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0] + (width * (height / 2)) + i) != 0x0808)
				{
					all_blue = false;
					break;
				}
			}

			Util_log_save("debug", (std::string)"maybe all #0x0808 : " + (all_blue ? "true" : "false"));

			if(all_blue && s + 1 < available_raw_image)
			{
				Util_log_save("debug", "use next frame");
				if(buffer_num + 1 < util_video_decoder_mvd_max_raw_image[session])
					buffer_num++;
				else
					buffer_num = 0;
			}
			else if(all_blue)
			{
				Util_log_save("debug", "maybe all #0x0808 but there is no image left in queue");
				break;
			}
			else
				break;
		}

		memcpy_asm(*raw_data, util_video_decoder_mvd_raw_image[session][buffer_num]->data[0], cpy_size);
		/*
		//Util_log_save("debug", "started");
		for(int i = 0; i < available_raw_image; i++)
		{
			//Util_log_save("debug", "expected : " + std::to_string(util_video_decoder_previous_pts[session][0] + util_video_decoder_increase_per_pts[session][0]) + " got : " + std::to_string(util_video_decoder_mvd_raw_image[session][buffer_num]->pts));
			if(util_video_decoder_previous_pts[session][0] + util_video_decoder_increase_per_pts[session][0] == util_video_decoder_mvd_raw_image[session][buffer_num]->pts)
			{
				util_video_decoder_previous_pts[session][0] = util_video_decoder_mvd_raw_image[session][buffer_num]->pts;
				//Util_log_save("debug", "found");
				Util_log_save("debug", "pts : " + std::to_string(util_video_decoder_mvd_raw_image[session][buffer_num]->pts / util_video_decoder_increase_per_pts[session][0]));

				*raw_data = (u8*)malloc(width * height * 2);
				if(*raw_data == NULL)
				{
					result.code = DEF_ERR_OUT_OF_MEMORY;
					result.string = DEF_ERR_OUT_OF_MEMORY_STR;
					return result;
				}

				memcpy_asm(*raw_data, util_video_decoder_mvd_raw_image[session][buffer_num]->data[0], cpy_size);
				if(buffer_num != util_video_decoder_mvd_raw_image_ready_index[session])
				{
					//Util_log_save("debug", "replace " + std::to_string(buffer_num) + " with " + std::to_string(util_video_decoder_mvd_raw_image_ready_index[session]));
					util_video_decoder_mvd_raw_image[session][buffer_num]->pts = util_video_decoder_mvd_raw_image[session][util_video_decoder_mvd_raw_image_ready_index[session]]->pts;
					memcpy_asm(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0], util_video_decoder_mvd_raw_image[session][util_video_decoder_mvd_raw_image_ready_index[session]]->data[0], cpy_size);
				}
				goto success;
			}
			if(buffer_num + 1 < util_video_decoder_mvd_max_raw_image[session])
				buffer_num++;
			else
				buffer_num = 0;
		}

		//Util_log_save("debug", "not found");
		result.code = DEF_ERR_TRY_AGAIN;
		result.string = DEF_ERR_TRY_AGAIN_STR;
		result.error_description = "[Error] No raw image available ";
		return result;*/
	}
	else
	{
		*raw_data = (u8*)malloc(width * height * 2);
		if(*raw_data == NULL)
		{
			result.code = DEF_ERR_OUT_OF_MEMORY;
			result.string = DEF_ERR_OUT_OF_MEMORY_STR;
			return result;
		}

		memcpy_asm(*raw_data, util_video_decoder_mvd_raw_image[session][buffer_num]->data[0], cpy_size);
	}

	//success:
	buffer_num = util_video_decoder_mvd_raw_image_ready_index[session];

	if(util_video_decoder_mvd_raw_image[session][buffer_num])
	{
		Util_safe_linear_free(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0]);
		for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_video_decoder_mvd_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_video_decoder_mvd_raw_image[session][buffer_num]);

	if(util_video_decoder_mvd_raw_image_ready_index[session] + 1 < util_video_decoder_mvd_max_raw_image[session])
		util_video_decoder_mvd_raw_image_ready_index[session]++;
	else
		util_video_decoder_mvd_raw_image_ready_index[session] = 0;

	svcWaitSynchronization(util_video_decoder_mvd_raw_image_counter_mutex[session], U64_MAX);
	util_video_decoder_mvd_available_raw_image[session]--;
	svcReleaseMutex(util_video_decoder_mvd_raw_image_counter_mutex[session]);
	svcReleaseMutex(util_video_decoder_mvd_get_raw_image_mutex[session]);

	return result;
}

void Util_video_decoder_skip_image(int packet_index, int session)
{
	int buffer_num = 0;
	svcWaitSynchronization(util_video_decoder_get_raw_image_mutex[session][packet_index], U64_MAX);
	if(util_video_decoder_available_raw_image[session][packet_index] <= 0)
	{
		svcReleaseMutex(util_video_decoder_get_raw_image_mutex[session][packet_index]);
		return;
	}

	buffer_num = util_video_decoder_raw_image_ready_index[session][packet_index];
	/*if(util_video_decoder_raw_image[session][packet_index][buffer_num])
	{
		av_freep(&util_video_decoder_raw_image[session][packet_index][buffer_num]);
	}*/
	av_frame_free(&util_video_decoder_raw_image[session][packet_index][buffer_num]);

	if(util_video_decoder_raw_image_ready_index[session][packet_index] + 1 < util_video_decoder_max_raw_image[session][packet_index])
		util_video_decoder_raw_image_ready_index[session][packet_index]++;
	else
		util_video_decoder_raw_image_ready_index[session][packet_index] = 0;

	svcWaitSynchronization(util_video_decoder_raw_image_counter_mutex[session][packet_index], U64_MAX);
	util_video_decoder_available_raw_image[session][packet_index]--;
	svcReleaseMutex(util_video_decoder_raw_image_counter_mutex[session][packet_index]);
	svcReleaseMutex(util_video_decoder_get_raw_image_mutex[session][packet_index]);
}

void Util_mvd_video_decoder_skip_image(int session)
{
	int buffer_num = 0;
	//int available_raw_image = 0;
	//int cpy_size = util_video_decoder_context[session][0]->width * util_video_decoder_context[session][0]->height * 2;
	svcWaitSynchronization(util_video_decoder_mvd_get_raw_image_mutex[session], U64_MAX);
	if(util_video_decoder_mvd_available_raw_image[session] <= 0)
	{
		svcReleaseMutex(util_video_decoder_mvd_get_raw_image_mutex[session]);
		return;
	}
	//available_raw_image = util_video_decoder_mvd_available_raw_image[session];
	buffer_num = util_video_decoder_mvd_raw_image_ready_index[session];

	/*if(var_debug_bool[0])
	{
		//Util_log_save("debug", "(skip) started");
		for(int i = 0; i < available_raw_image; i++)
		{
			//Util_log_save("debug", "(skip) expected : " + std::to_string(util_video_decoder_previous_pts[session][0] + util_video_decoder_increase_per_pts[session][0]) + " got : " + std::to_string(util_video_decoder_mvd_raw_image[session][buffer_num]->pts));
			if(util_video_decoder_previous_pts[session][0] + util_video_decoder_increase_per_pts[session][0] == util_video_decoder_mvd_raw_image[session][buffer_num]->pts)
			{
				util_video_decoder_previous_pts[session][0] = util_video_decoder_mvd_raw_image[session][buffer_num]->pts;
				//Util_log_save("debug", "(skip) found");
				Util_log_save("debug", "(skip) pts : " + std::to_string(util_video_decoder_mvd_raw_image[session][buffer_num]->pts / util_video_decoder_increase_per_pts[session][0]));

				if(buffer_num != util_video_decoder_mvd_raw_image_ready_index[session])
				{
					//Util_log_save("debug", "replace " + std::to_string(buffer_num) + " with " + std::to_string(util_video_decoder_mvd_raw_image_ready_index[session]));
					util_video_decoder_mvd_raw_image[session][buffer_num]->pts = util_video_decoder_mvd_raw_image[session][util_video_decoder_mvd_raw_image_ready_index[session]]->pts;
					memcpy_asm(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0], util_video_decoder_mvd_raw_image[session][util_video_decoder_mvd_raw_image_ready_index[session]]->data[0], cpy_size);
				}
				goto success;
			}
			if(buffer_num + 1 < util_video_decoder_mvd_max_raw_image[session])
				buffer_num++;
			else
				buffer_num = 0;
		}
		//Util_log_save("debug", "(skip) not found");
		return;
	}
	else
	{
	}*/

	//success:

	//buffer_num = util_video_decoder_mvd_raw_image_ready_index[session];

	if(util_video_decoder_mvd_raw_image[session][buffer_num])
	{
		Util_safe_linear_free(util_video_decoder_mvd_raw_image[session][buffer_num]->data[0]);
		for(int i = 0; i < AV_NUM_DATA_POINTERS; i++)
			util_video_decoder_mvd_raw_image[session][buffer_num]->data[i] = NULL;
	}
	av_frame_free(&util_video_decoder_mvd_raw_image[session][buffer_num]);
	
	if(util_video_decoder_mvd_raw_image_ready_index[session] + 1 < util_video_decoder_mvd_max_raw_image[session])
		util_video_decoder_mvd_raw_image_ready_index[session]++;
	else
		util_video_decoder_mvd_raw_image_ready_index[session] = 0;

	svcWaitSynchronization(util_video_decoder_mvd_raw_image_counter_mutex[session], U64_MAX);
	util_video_decoder_mvd_available_raw_image[session]--;
	svcReleaseMutex(util_video_decoder_mvd_raw_image_counter_mutex[session]);
	svcReleaseMutex(util_video_decoder_mvd_get_raw_image_mutex[session]);
}

Result_with_string Util_decoder_seek(u64 seek_pos, int flag, int session)
{
	int ffmpeg_result;
	Result_with_string result;

	svcWaitSynchronization(util_decoder_read_cache_packet_mutex[session], U64_MAX);
	svcWaitSynchronization(util_decoder_parse_cache_packet_mutex[session], U64_MAX);
	ffmpeg_result = avformat_seek_file(util_decoder_format_context[session], -1, INT64_MIN, seek_pos, INT64_MAX, flag);//AVSEEK_FLAG_FRAME 8 AVSEEK_FLAG_ANY 4  AVSEEK_FLAG_BACKWORD 1
	svcReleaseMutex(util_decoder_read_cache_packet_mutex[session]);
	svcReleaseMutex(util_decoder_parse_cache_packet_mutex[session]);
	if(ffmpeg_result < 0)
	{
		result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
		result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
		result.error_description = "avformat_seek_file() failed " + std::to_string(ffmpeg_result);
	}

	return result;
}

void Util_decoder_close_file(int session)
{
	for(int i = 0; i < DEF_DECODER_MAX_CACHE_PACKETS; i++)
		av_packet_free(&util_decoder_cache_packet[session][i]);

	avformat_close_input(&util_decoder_format_context[session]);
	svcCloseHandle(util_decoder_cache_packet_counter_mutex[session]);
	svcCloseHandle(util_decoder_read_cache_packet_mutex[session]);
	svcCloseHandle(util_decoder_parse_cache_packet_mutex[session]);
}

void Util_audio_decoder_exit(int session)
{
	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		avcodec_free_context(&util_audio_decoder_context[session][i]);
		av_packet_free(&util_audio_decoder_packet[session][i]);
		av_packet_free(&util_audio_decoder_cache_packet[session][i]);
		av_frame_free(&util_audio_decoder_raw_data[session][i]);
		swr_free(&util_audio_decoder_swr_context[session][i]);
	}
}

void Util_video_decoder_exit(int session)
{
	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		avcodec_free_context(&util_video_decoder_context[session][i]);
		av_packet_free(&util_video_decoder_packet[session][i]);
		av_packet_free(&util_video_decoder_cache_packet[session][i]);
		svcCloseHandle(util_video_decoder_raw_image_counter_mutex[session][i]);
		svcCloseHandle(util_video_decoder_add_raw_image_mutex[session][i]);
		svcCloseHandle(util_video_decoder_get_raw_image_mutex[session][i]);
		util_video_decoder_available_raw_image[session][i] = 0;
		util_video_decoder_raw_image_ready_index[session][i] = 0;
		util_video_decoder_raw_image_current_index[session][i] = 0;
		for(int k = 0; k < util_video_decoder_max_raw_image[session][i]; k++)
		{
			/*if(util_video_decoder_raw_image[session][i][k])
			{
				av_freep(&util_video_decoder_raw_image[session][i][k]);
			}*/
			av_frame_free(&util_video_decoder_raw_image[session][i][k]);
		}
	}
}

void Util_mvd_video_decoder_exit(int session)
{
	mvdstdExit();
	Util_safe_linear_free(util_video_decoder_mvd_packet);
	util_video_decoder_mvd_packet = NULL;
	for(int i = 0; i < util_video_decoder_mvd_max_raw_image[session]; i++)
	{
		if(util_video_decoder_mvd_raw_image[session][i])
		{
			Util_safe_linear_free(util_video_decoder_mvd_raw_image[session][i]->data[0]);
			for(int k = 0; k < AV_NUM_DATA_POINTERS; k++)
				util_video_decoder_mvd_raw_image[session][i]->data[k] = NULL;
		}
		av_frame_free(&util_video_decoder_mvd_raw_image[session][i]);
	}
}

Result_with_string Util_image_decoder_decode(std::string file_name, u8** raw_data, int* width, int* height)
{
	Result_with_string result;
	int image_ch = 0;
	*raw_data = stbi_load(file_name.c_str(), width, height, &image_ch, STBI_rgb);

	return result;
}
