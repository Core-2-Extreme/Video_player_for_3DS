#include "headers.hpp"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

int util_audio_muxer_stream_num[2] = { -1, -1, };
AVPacket* util_audio_muxer_packet[2] = { NULL, NULL, };
AVFormatContext* util_audio_muxer_format_context[2] = { NULL, NULL, };
AVCodecContext* util_audio_muxer_context[2] = { NULL, NULL, };
const AVCodec* util_audio_muxer_codec[2] = { NULL, NULL, };
AVStream* util_audio_muxer_format_stream[2] = { NULL, NULL, };

int util_video_muxer_stream_num[2] = { -1, -1, };
AVPacket* util_video_muxer_packet[2] = { NULL, NULL, };
AVFormatContext* util_video_muxer_format_context[2] = { NULL, NULL, };
AVCodecContext* util_video_muxer_context[2] = { NULL, NULL, };
const AVCodec* util_video_muxer_codec[2] = { NULL, NULL, };
AVStream* util_video_muxer_format_stream[2] = { NULL, NULL, };

AVFormatContext* util_muxer_format_context[2] = { NULL, NULL, };

Result_with_string Util_muxer_mux(std::string file_name, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	util_muxer_format_context[session] = avformat_alloc_context();
	if(!util_muxer_format_context[session])
	{
		result.error_description = "avformat_alloc_context() failed";
		goto fail;
	}

	util_muxer_format_context[session]->oformat = av_guess_format(NULL, file_name.c_str(), NULL);
	if(!util_muxer_format_context[session]->oformat)
	{
		result.error_description = "av_guess_format() failed";
		goto fail;
	}

	ffmpeg_result = avio_open(&util_muxer_format_context[session]->pb, file_name.c_str(), AVIO_FLAG_READ_WRITE);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avio_open() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	//setup for audio
	util_audio_muxer_codec[session] = avcodec_find_encoder(util_audio_muxer_format_context[session]->streams[util_audio_muxer_stream_num[session]]->codecpar->codec_id);
	if(!util_audio_muxer_codec[session])
	{
		result.error_description = "avcodec_find_encoder() failed";
		goto fail;
	}
	
	util_audio_muxer_context[session] = avcodec_alloc_context3(util_audio_muxer_codec[session]);
	if(!util_audio_muxer_context[session])
	{
		result.error_description = "avcodec_alloc_context3() failed";
		goto fail;
	}

	ffmpeg_result = avcodec_parameters_to_context(util_audio_muxer_context[session], util_audio_muxer_format_context[session]->streams[util_audio_muxer_stream_num[session]]->codecpar);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_parameters_to_context() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	util_audio_muxer_format_stream[session] = avformat_new_stream(util_muxer_format_context[session], NULL);
	if(!util_audio_muxer_format_stream[session])
	{
		result.error_description = "avformat_new_stream() failed";
		goto fail;
	}

	ffmpeg_result = avcodec_parameters_from_context(util_audio_muxer_format_stream[session]->codecpar, util_audio_muxer_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_parameters_from_context() failed";
		goto fail;
	}

	//setup for video
	util_video_muxer_codec[session] = avcodec_find_encoder(util_video_muxer_format_context[session]->streams[util_video_muxer_stream_num[session]]->codecpar->codec_id);
	if(!util_video_muxer_codec[session])
	{
		result.error_description = "avcodec_find_encoder() failed";
		goto fail;
	}

	util_video_muxer_context[session] = avcodec_alloc_context3(util_video_muxer_codec[session]);
	if(!util_video_muxer_context[session])
	{
		result.error_description = "avcodec_alloc_context3() failed";
		goto fail;
	}

	ffmpeg_result = avcodec_parameters_to_context(util_video_muxer_context[session], util_video_muxer_format_context[session]->streams[util_video_muxer_stream_num[session]]->codecpar);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_parameters_to_context() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	util_video_muxer_format_stream[session] = avformat_new_stream(util_muxer_format_context[session], NULL);
	if(!util_video_muxer_format_stream[session])
	{
		result.error_description = "avformat_new_stream() failed";
		goto fail;
	}

	ffmpeg_result = avcodec_parameters_from_context(util_video_muxer_format_stream[session]->codecpar, util_video_muxer_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_parameters_from_context() failed";
		goto fail;
	}

	if (util_muxer_format_context[session]->oformat->flags & AVFMT_GLOBALHEADER)
		util_muxer_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ffmpeg_result = avformat_write_header(util_muxer_format_context[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avformat_write_header() failed";
		goto fail;
	}
	
	util_audio_muxer_packet[session] = av_packet_alloc();
	if(!util_audio_muxer_packet[session])
	{
		result.error_description = "av_packet_alloc() failed";
		goto fail;
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
			if(ffmpeg_result != 0)
			{
				result.error_description = "av_interleaved_write_frame() failed " + std::to_string(ffmpeg_result);
				goto fail;
			}
		}
	}
	av_packet_free(&util_audio_muxer_packet[session]);

	util_video_muxer_packet[session] = av_packet_alloc();
	if(!util_video_muxer_packet[session])
	{
		result.error_description = "av_packet_alloc() failed";
		goto fail;
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
			if(ffmpeg_result != 0)
			{
				result.error_description = "av_interleaved_write_frame() failed " + std::to_string(ffmpeg_result);
				goto fail;
			}
		}
	}
	av_packet_free(&util_video_muxer_packet[session]);

	av_write_trailer(util_muxer_format_context[session]);
	avio_close(util_muxer_format_context[session]->pb);
	avformat_free_context(util_muxer_format_context[session]);

	return result;

	fail:

	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	avio_close(util_muxer_format_context[session]->pb);
	avformat_free_context(util_muxer_format_context[session]);
	return result;
}

Result_with_string Util_muxer_open_audio_file(std::string file_path, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	util_audio_muxer_format_context[session] = avformat_alloc_context();
	ffmpeg_result = avformat_open_input(&util_audio_muxer_format_context[session], file_path.c_str(), NULL, NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avformat_open_input() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	ffmpeg_result = avformat_find_stream_info(util_audio_muxer_format_context[session], NULL);
	if(util_audio_muxer_format_context[session] == NULL)
	{
		result.error_description = "avformat_find_stream_info() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	util_audio_muxer_stream_num[session] = -1;
	for(int i = 0; i < (int)util_audio_muxer_format_context[session]->nb_streams; i++)
	{
		if(util_audio_muxer_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			util_audio_muxer_stream_num[session] = i;
	}

	if(util_audio_muxer_stream_num[session] == -1)
	{
		result.error_description = "No audio data";
		goto fail;
	}
	return result;

	fail:

	Util_muxer_close_audio_file(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_muxer_open_video_file(std::string file_path, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	util_video_muxer_format_context[session] = avformat_alloc_context();
	ffmpeg_result = avformat_open_input(&util_video_muxer_format_context[session], file_path.c_str(), NULL, NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avformat_open_input() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	ffmpeg_result = avformat_find_stream_info(util_video_muxer_format_context[session], NULL);
	if(util_video_muxer_format_context[session] == NULL)
	{
		result.error_description = "avformat_find_stream_info() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	util_video_muxer_stream_num[session] = -1;
	for(int i = 0; i < (int)util_video_muxer_format_context[session]->nb_streams; i++)
	{
		if(util_video_muxer_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			util_video_muxer_stream_num[session] = i;
	}

	if(util_video_muxer_stream_num[session] == -1)
	{
		result.error_description = "No video data";
		goto fail;
	}
	return result;

	fail:

	Util_muxer_close_video_file(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_muxer_close_audio_file(int session)
{
	avformat_close_input(&util_audio_muxer_format_context[session]);
}

void Util_muxer_close_video_file(int session)
{
	avformat_close_input(&util_video_muxer_format_context[session]);
}
