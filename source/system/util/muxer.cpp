#include "system/headers.hpp"

#if DEF_ENABLE_MUXER_API

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

bool util_audio_muxer_init[DEF_MUXER_MAX_SESSIONS];
int util_audio_muxer_stream_num[DEF_MUXER_MAX_SESSIONS];
AVPacket* util_audio_muxer_packet[DEF_MUXER_MAX_SESSIONS];
AVFormatContext* util_audio_muxer_format_context[DEF_MUXER_MAX_SESSIONS];
AVCodecContext* util_audio_muxer_context[DEF_MUXER_MAX_SESSIONS];
const AVCodec* util_audio_muxer_codec[DEF_MUXER_MAX_SESSIONS];
AVStream* util_audio_muxer_format_stream[DEF_MUXER_MAX_SESSIONS];

bool util_video_muxer_init[DEF_MUXER_MAX_SESSIONS];
int util_video_muxer_stream_num[DEF_MUXER_MAX_SESSIONS];
AVPacket* util_video_muxer_packet[DEF_MUXER_MAX_SESSIONS];
AVFormatContext* util_video_muxer_format_context[DEF_MUXER_MAX_SESSIONS];
AVCodecContext* util_video_muxer_context[DEF_MUXER_MAX_SESSIONS];
const AVCodec* util_video_muxer_codec[DEF_MUXER_MAX_SESSIONS];
AVStream* util_video_muxer_format_stream[DEF_MUXER_MAX_SESSIONS];

AVFormatContext* util_muxer_format_context[DEF_MUXER_MAX_SESSIONS];

Result_with_string Util_muxer_open_audio_file(std::string file_path, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	if(file_path == "" || session < 0 || session > DEF_MUXER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_audio_muxer_init[session])
		goto already_inited;

	util_audio_muxer_format_context[session] = avformat_alloc_context();
	if(!util_audio_muxer_format_context[session])
	{
		result.error_description = "[Error] avformat_alloc_context() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_open_input(&util_audio_muxer_format_context[session], file_path.c_str(), NULL, NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avformat_open_input() failed. " + std::to_string(ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_find_stream_info(util_audio_muxer_format_context[session], NULL);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] avformat_find_stream_info() failed. " + std::to_string(ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_audio_muxer_stream_num[session] = -1;
	for(int i = 0; i < (int)util_audio_muxer_format_context[session]->nb_streams; i++)
	{
		if(util_audio_muxer_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			util_audio_muxer_stream_num[session] = i;
	}

	if(util_audio_muxer_stream_num[session] == -1)
	{
		result.error_description = "[Error] No audio data was found. ";
		goto other;
	}

	util_audio_muxer_init[session] = true;
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
	avformat_close_input(&util_audio_muxer_format_context[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;

	other:
	avformat_close_input(&util_audio_muxer_format_context[session]);
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

Result_with_string Util_muxer_open_video_file(std::string file_path, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	if(file_path == "" || session < 0 || session > DEF_MUXER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_video_muxer_init[session])
		goto already_inited;
	
	util_video_muxer_format_context[session] = avformat_alloc_context();
	if(!util_video_muxer_format_context[session])
	{
		result.error_description = "[Error] avformat_alloc_context() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_open_input(&util_video_muxer_format_context[session], file_path.c_str(), NULL, NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avformat_open_input() failed. " + std::to_string(ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_find_stream_info(util_video_muxer_format_context[session], NULL);
	if(ffmpeg_result < 0)
	{
		result.error_description = "[Error] avformat_find_stream_info() failed. " + std::to_string(ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_video_muxer_stream_num[session] = -1;
	for(int i = 0; i < (int)util_video_muxer_format_context[session]->nb_streams; i++)
	{
		if(util_video_muxer_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			util_video_muxer_stream_num[session] = i;
	}

	if(util_video_muxer_stream_num[session] == -1)
	{
		result.error_description = "[Error] No video data was found. ";
		goto other;
	}

	util_video_muxer_init[session] = true;
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
	avformat_close_input(&util_video_muxer_format_context[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;

	other:
	avformat_close_input(&util_video_muxer_format_context[session]);
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

Result_with_string Util_muxer_mux(std::string file_path, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	if(file_path == "" || session < 0 || session > DEF_MUXER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_audio_muxer_init[session] || !util_video_muxer_init[session])
		goto not_inited;

	util_muxer_format_context[session] = avformat_alloc_context();
	if(!util_muxer_format_context[session])
	{
		result.error_description = "[Error] avformat_alloc_context() failed. ";
		goto ffmpeg_api_failed;
	}

	util_muxer_format_context[session]->oformat = av_guess_format(NULL, file_path.c_str(), NULL);
	if(!util_muxer_format_context[session]->oformat)
	{
		result.error_description = "[Error] av_guess_format() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avio_open(&util_muxer_format_context[session]->pb, file_path.c_str(), AVIO_FLAG_READ_WRITE);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avio_open() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	//setup for audio
	util_audio_muxer_codec[session] = avcodec_find_encoder(util_audio_muxer_format_context[session]->streams[util_audio_muxer_stream_num[session]]->codecpar->codec_id);
	if(!util_audio_muxer_codec[session])
	{
		result.error_description = "[Error] avcodec_find_encoder() failed. ";
		goto ffmpeg_api_failed;
	}
	
	util_audio_muxer_context[session] = avcodec_alloc_context3(util_audio_muxer_codec[session]);
	if(!util_audio_muxer_context[session])
	{
		result.error_description = "[Error] avcodec_alloc_context3() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_to_context(util_audio_muxer_context[session], util_audio_muxer_format_context[session]->streams[util_audio_muxer_stream_num[session]]->codecpar);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_parameters_to_context() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_audio_muxer_format_stream[session] = avformat_new_stream(util_muxer_format_context[session], NULL);
	if(!util_audio_muxer_format_stream[session])
	{
		result.error_description = "[Error] avformat_new_stream() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_from_context(util_audio_muxer_format_stream[session]->codecpar, util_audio_muxer_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_parameters_from_context() failed. ";
		goto ffmpeg_api_failed;
	}

	//setup for video
	util_video_muxer_codec[session] = avcodec_find_encoder(util_video_muxer_format_context[session]->streams[util_video_muxer_stream_num[session]]->codecpar->codec_id);
	if(!util_video_muxer_codec[session])
	{
		result.error_description = "[Error] avcodec_find_encoder() failed. ";
		goto ffmpeg_api_failed;
	}

	util_video_muxer_context[session] = avcodec_alloc_context3(util_video_muxer_codec[session]);
	if(!util_video_muxer_context[session])
	{
		result.error_description = "[Error] avcodec_alloc_context3() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_to_context(util_video_muxer_context[session], util_video_muxer_format_context[session]->streams[util_video_muxer_stream_num[session]]->codecpar);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_parameters_to_context() failed. " + std::to_string(ffmpeg_result) + " ";
		goto ffmpeg_api_failed;
	}

	util_video_muxer_format_stream[session] = avformat_new_stream(util_muxer_format_context[session], NULL);
	if(!util_video_muxer_format_stream[session])
	{
		result.error_description = "[Error] avformat_new_stream() failed. ";
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_from_context(util_video_muxer_format_stream[session]->codecpar, util_video_muxer_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "[Error] avcodec_parameters_from_context() failed.";
		goto ffmpeg_api_failed;
	}

	if (util_muxer_format_context[session]->oformat->flags & AVFMT_GLOBALHEADER)
	{
		util_muxer_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		ffmpeg_result = avformat_write_header(util_muxer_format_context[session], NULL);
		if(ffmpeg_result != 0)
		{
			result.error_description = "[Error] avformat_write_header() failed. ";
			goto ffmpeg_api_failed;
		}
	}
	
	util_audio_muxer_packet[session] = av_packet_alloc();
	if(!util_audio_muxer_packet[session])
	{
		result.error_description = "[Error] av_packet_alloc() failed. ";
		goto ffmpeg_api_failed;
	}
	
	while(true)//mux audio
	{
		ffmpeg_result = av_read_frame(util_audio_muxer_format_context[session], util_audio_muxer_packet[session]);
		if(ffmpeg_result != 0)
			break;
		
		if(util_audio_muxer_stream_num[session] == util_audio_muxer_packet[session]->stream_index)
		{
			util_audio_muxer_packet[session]->stream_index = 0;
			ffmpeg_result = av_interleaved_write_frame(util_muxer_format_context[session], util_audio_muxer_packet[session]);
			av_packet_unref(util_audio_muxer_packet[session]);
			if(ffmpeg_result != 0)
			{
				result.error_description = "[Error] av_interleaved_write_frame() failed. " + std::to_string(ffmpeg_result) + " ";
				goto ffmpeg_api_failed;
			}
		}
		else
			av_packet_unref(util_audio_muxer_packet[session]);
	}
	av_packet_free(&util_audio_muxer_packet[session]);

	util_video_muxer_packet[session] = av_packet_alloc();
	if(!util_video_muxer_packet[session])
	{
		result.error_description = "[Error] av_packet_alloc() failed. ";
		goto ffmpeg_api_failed;
	}

	while(true)//mux video
	{
		ffmpeg_result = av_read_frame(util_video_muxer_format_context[session], util_video_muxer_packet[session]);
		if(ffmpeg_result != 0)
			break;
		
		if(util_video_muxer_stream_num[session] == util_video_muxer_packet[session]->stream_index)
		{
			util_video_muxer_packet[session]->stream_index = 1;
			ffmpeg_result = av_interleaved_write_frame(util_muxer_format_context[session], util_video_muxer_packet[session]);
			av_packet_unref(util_video_muxer_packet[session]);
			if(ffmpeg_result != 0)
			{
				result.error_description = "[Error] av_interleaved_write_frame() failed. " + std::to_string(ffmpeg_result) + " ";
				goto ffmpeg_api_failed;
			}
		}
		else
			av_packet_unref(util_video_muxer_packet[session]);
	}
	av_packet_free(&util_video_muxer_packet[session]);
	
	av_write_trailer(util_muxer_format_context[session]);
	avio_close(util_muxer_format_context[session]->pb);
	avformat_free_context(util_muxer_format_context[session]);

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
	av_packet_free(&util_audio_muxer_packet[session]);
	av_packet_free(&util_video_muxer_packet[session]);
	avio_close(util_muxer_format_context[session]->pb);
	avformat_free_context(util_muxer_format_context[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_muxer_close(int session)
{
	if(util_audio_muxer_init[session])
		avformat_close_input(&util_audio_muxer_format_context[session]);
	if(util_video_muxer_init[session])
		avformat_close_input(&util_video_muxer_format_context[session]);

	util_audio_muxer_init[session] = false;
	util_video_muxer_init[session] = false;
}

#endif
