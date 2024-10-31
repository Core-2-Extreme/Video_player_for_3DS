//Includes.
#include "system/util/muxer.h"

#if DEF_MUXER_API_ENABLE
#include <stdbool.h>
#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "system/util/err_types.h"
#include "system/util/log.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
static bool util_audio_muxer_init[DEF_MUXER_MAX_SESSIONS] = { 0, };
static uint8_t util_audio_muxer_stream_num[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVPacket* util_audio_muxer_packet[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVFormatContext* util_audio_muxer_format_context[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVCodecContext* util_audio_muxer_context[DEF_MUXER_MAX_SESSIONS] = { 0, };
static const AVCodec* util_audio_muxer_codec[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVStream* util_audio_muxer_format_stream[DEF_MUXER_MAX_SESSIONS] = { 0, };

static bool util_video_muxer_init[DEF_MUXER_MAX_SESSIONS] = { 0, };
static uint8_t util_video_muxer_stream_num[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVPacket* util_video_muxer_packet[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVFormatContext* util_video_muxer_format_context[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVCodecContext* util_video_muxer_context[DEF_MUXER_MAX_SESSIONS] = { 0, };
static const AVCodec* util_video_muxer_codec[DEF_MUXER_MAX_SESSIONS] = { 0, };
static AVStream* util_video_muxer_format_stream[DEF_MUXER_MAX_SESSIONS] = { 0, };

static AVFormatContext* util_muxer_format_context[DEF_MUXER_MAX_SESSIONS] = { 0, };

//Code.
uint32_t Util_muxer_open_audio_file(const char* file_path, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(!file_path || session >= DEF_MUXER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_audio_muxer_init[session])
		goto already_inited;

	util_audio_muxer_format_context[session] = avformat_alloc_context();
	if(!util_audio_muxer_format_context[session])
	{
		DEF_LOG_RESULT(avformat_alloc_context, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_open_input(&util_audio_muxer_format_context[session], file_path, NULL, NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avformat_open_input, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_find_stream_info(util_audio_muxer_format_context[session], NULL);
	if(ffmpeg_result < 0)
	{
		DEF_LOG_RESULT(avformat_find_stream_info, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_audio_muxer_stream_num[session] = UINT8_MAX;
	for(uint8_t i = 0; i < util_audio_muxer_format_context[session]->nb_streams; i++)
	{
		if(util_audio_muxer_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			util_audio_muxer_stream_num[session] = i;
			break;
		}
	}

	if(util_audio_muxer_stream_num[session] == UINT8_MAX)
	{
		DEF_LOG_STRING("No audio data was found.");
		goto other;
	}

	util_audio_muxer_init[session] = true;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	avformat_close_input(&util_audio_muxer_format_context[session]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;

	other:
	avformat_close_input(&util_audio_muxer_format_context[session]);
	return DEF_ERR_OTHER;
}

uint32_t Util_muxer_open_video_file(const char* file_path, uint8_t session)
{
	int32_t ffmpeg_result = 0;

	if(!file_path || session >= DEF_MUXER_MAX_SESSIONS)
		goto invalid_arg;

	if(util_video_muxer_init[session])
		goto already_inited;

	util_video_muxer_format_context[session] = avformat_alloc_context();
	if(!util_video_muxer_format_context[session])
	{
		DEF_LOG_RESULT(avformat_alloc_context, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_open_input(&util_video_muxer_format_context[session], file_path, NULL, NULL);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avformat_open_input, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avformat_find_stream_info(util_video_muxer_format_context[session], NULL);
	if(ffmpeg_result < 0)
	{
		DEF_LOG_RESULT(avformat_find_stream_info, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_video_muxer_stream_num[session] = UINT8_MAX;
	for(uint8_t i = 0; i < util_video_muxer_format_context[session]->nb_streams; i++)
	{
		if(util_video_muxer_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			util_video_muxer_stream_num[session] = i;
			break;
		}
	}

	if(util_video_muxer_stream_num[session] == UINT8_MAX)
	{
		DEF_LOG_STRING("No audio data was found.");
		goto other;
	}

	util_video_muxer_init[session] = true;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	ffmpeg_api_failed:
	avformat_close_input(&util_video_muxer_format_context[session]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;

	other:
	avformat_close_input(&util_video_muxer_format_context[session]);
	return DEF_ERR_OTHER;
}

uint32_t Util_muxer_mux(const char* file_path, uint8_t session)
{
	bool is_audio_turn = false;
	bool is_audio_eof = false;
	bool is_video_eof = false;
	double last_audio_pos = 0;
	double last_video_pos = 0;
	double old_audio_timebase = 0;
	double new_audio_timebase = 0;
	double old_video_timebase = 0;
	double new_video_timebase = 0;
	int32_t ffmpeg_result = 0;
	double video_ts_factor = 0;
	double audio_ts_factor = 0;

	if(!file_path || session >= DEF_MUXER_MAX_SESSIONS)
		goto invalid_arg;

	if(!util_audio_muxer_init[session] || !util_video_muxer_init[session])
		goto not_inited;

	util_muxer_format_context[session] = avformat_alloc_context();
	if(!util_muxer_format_context[session])
	{
		DEF_LOG_RESULT(avformat_alloc_context, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_muxer_format_context[session]->oformat = av_guess_format(NULL, file_path, NULL);
	if(!util_muxer_format_context[session]->oformat)
	{
		DEF_LOG_RESULT(av_guess_format, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avio_open(&util_muxer_format_context[session]->pb, file_path, AVIO_FLAG_READ_WRITE);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avio_open, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	//Setup for audio.
	util_audio_muxer_codec[session] = avcodec_find_encoder(util_audio_muxer_format_context[session]->streams[util_audio_muxer_stream_num[session]]->codecpar->codec_id);
	if(!util_audio_muxer_codec[session])
	{
		DEF_LOG_RESULT(avcodec_find_encoder, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_audio_muxer_context[session] = avcodec_alloc_context3(util_audio_muxer_codec[session]);
	if(!util_audio_muxer_context[session])
	{
		DEF_LOG_RESULT(avcodec_alloc_context3, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_to_context(util_audio_muxer_context[session], util_audio_muxer_format_context[session]->streams[util_audio_muxer_stream_num[session]]->codecpar);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_parameters_to_context, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_audio_muxer_format_stream[session] = avformat_new_stream(util_muxer_format_context[session], NULL);
	if(!util_audio_muxer_format_stream[session])
	{
		DEF_LOG_RESULT(avformat_new_stream, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_from_context(util_audio_muxer_format_stream[session]->codecpar, util_audio_muxer_context[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_parameters_from_context, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	//Setup for video.
	util_video_muxer_codec[session] = avcodec_find_encoder(util_video_muxer_format_context[session]->streams[util_video_muxer_stream_num[session]]->codecpar->codec_id);
	if(!util_video_muxer_codec[session])
	{
		DEF_LOG_RESULT(avcodec_find_encoder, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_video_muxer_context[session] = avcodec_alloc_context3(util_video_muxer_codec[session]);
	if(!util_video_muxer_context[session])
	{
		DEF_LOG_RESULT(avcodec_alloc_context3, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_to_context(util_video_muxer_context[session], util_video_muxer_format_context[session]->streams[util_video_muxer_stream_num[session]]->codecpar);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_parameters_to_context, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_video_muxer_format_stream[session] = avformat_new_stream(util_muxer_format_context[session], NULL);
	if(!util_video_muxer_format_stream[session])
	{
		DEF_LOG_RESULT(avformat_new_stream, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	ffmpeg_result = avcodec_parameters_from_context(util_video_muxer_format_stream[session]->codecpar, util_video_muxer_context[session]);
	if(ffmpeg_result != 0)
	{
		DEF_LOG_RESULT(avcodec_parameters_from_context, false, ffmpeg_result);
		goto ffmpeg_api_failed;
	}

	util_audio_muxer_format_stream[session]->time_base = util_audio_muxer_format_context[session]->streams[util_audio_muxer_stream_num[session]]->time_base;
	util_video_muxer_format_stream[session]->time_base = util_video_muxer_format_context[session]->streams[util_video_muxer_stream_num[session]]->time_base;
	old_audio_timebase = av_q2d(util_audio_muxer_format_stream[session]->time_base);
	old_video_timebase = av_q2d(util_video_muxer_format_stream[session]->time_base);

	if (util_muxer_format_context[session]->oformat->flags & AVFMT_GLOBALHEADER)
	{
		util_muxer_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		ffmpeg_result = avformat_write_header(util_muxer_format_context[session], NULL);
		if(ffmpeg_result != 0)
		{
			DEF_LOG_RESULT(avformat_write_header, false, ffmpeg_result);
			goto ffmpeg_api_failed;
		}
	}

	new_audio_timebase = av_q2d(util_audio_muxer_format_stream[session]->time_base);
	new_video_timebase = av_q2d(util_video_muxer_format_stream[session]->time_base);
	audio_ts_factor = (old_audio_timebase / new_audio_timebase);
	video_ts_factor = (old_video_timebase / new_video_timebase);

	util_audio_muxer_packet[session] = av_packet_alloc();
	if(!util_audio_muxer_packet[session])
	{
		DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	util_video_muxer_packet[session] = av_packet_alloc();
	if(!util_video_muxer_packet[session])
	{
		DEF_LOG_RESULT(av_packet_alloc, false, DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS);
		goto ffmpeg_api_failed;
	}

	while(true)//Mux.
	{
		if(is_audio_eof && is_video_eof)
			break;

		if(is_audio_eof)
			is_audio_turn = false;
		if(is_video_eof)
			is_audio_turn = true;

		if(is_audio_turn)
		{
			ffmpeg_result = av_read_frame(util_audio_muxer_format_context[session], util_audio_muxer_packet[session]);
			if(ffmpeg_result != 0)
			{
				is_audio_eof = true;
				continue;
			}

			if(util_audio_muxer_stream_num[session] == util_audio_muxer_packet[session]->stream_index)
			{
				util_audio_muxer_packet[session]->stream_index = 0;
				//Resample timestamp and calc current pos.
				if(util_audio_muxer_packet[session]->dts != AV_NOPTS_VALUE)
				{
					util_audio_muxer_packet[session]->dts *= audio_ts_factor;
					last_audio_pos = (util_audio_muxer_packet[session]->dts * new_audio_timebase);
				}
				if(util_audio_muxer_packet[session]->pts != AV_NOPTS_VALUE)
				{
					util_audio_muxer_packet[session]->pts *= audio_ts_factor;
					last_audio_pos = (util_audio_muxer_packet[session]->pts * new_audio_timebase);
				}

				ffmpeg_result = av_interleaved_write_frame(util_muxer_format_context[session], util_audio_muxer_packet[session]);
				av_packet_unref(util_audio_muxer_packet[session]);
				if(ffmpeg_result != 0)
				{
					DEF_LOG_RESULT(av_interleaved_write_frame, false, ffmpeg_result);
					goto ffmpeg_api_failed;
				}
			}
			else
				av_packet_unref(util_audio_muxer_packet[session]);
		}
		else
		{
			ffmpeg_result = av_read_frame(util_video_muxer_format_context[session], util_video_muxer_packet[session]);
			if(ffmpeg_result != 0)
			{
				is_video_eof = true;
				continue;
			}

			if(util_video_muxer_stream_num[session] == util_video_muxer_packet[session]->stream_index)
			{
				util_video_muxer_packet[session]->stream_index = 1;
				//Resample timestamp and calc current pos.
				if(util_video_muxer_packet[session]->dts != AV_NOPTS_VALUE)
				{
					util_video_muxer_packet[session]->dts *= video_ts_factor;
					last_video_pos = (util_video_muxer_packet[session]->dts * new_video_timebase);
				}
				if(util_video_muxer_packet[session]->pts != AV_NOPTS_VALUE)
				{
					util_video_muxer_packet[session]->pts *= video_ts_factor;
					last_video_pos = (util_video_muxer_packet[session]->pts * new_video_timebase);
				}

				ffmpeg_result = av_interleaved_write_frame(util_muxer_format_context[session], util_video_muxer_packet[session]);
				av_packet_unref(util_video_muxer_packet[session]);
				if(ffmpeg_result != 0)
				{
					DEF_LOG_RESULT(av_interleaved_write_frame, false, ffmpeg_result);
					goto ffmpeg_api_failed;
				}
			}
			else
				av_packet_unref(util_video_muxer_packet[session]);
		}

		if(last_video_pos > last_audio_pos)
			is_audio_turn = true;
		else if(last_video_pos < last_audio_pos)
			is_audio_turn = false;
		else {} //Do nothing (last_video_pos == last_audio_pos).
	}
	av_packet_free(&util_audio_muxer_packet[session]);
	av_packet_free(&util_video_muxer_packet[session]);

	av_write_trailer(util_muxer_format_context[session]);
	avio_close(util_muxer_format_context[session]->pb);
	avformat_free_context(util_muxer_format_context[session]);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	ffmpeg_api_failed:
	av_packet_free(&util_audio_muxer_packet[session]);
	av_packet_free(&util_video_muxer_packet[session]);
	avio_close(util_muxer_format_context[session]->pb);
	avformat_free_context(util_muxer_format_context[session]);
	return DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
}

void Util_muxer_close(uint8_t session)
{
	if(session >= DEF_MUXER_MAX_SESSIONS)
		return;

	if(util_audio_muxer_init[session])
		avformat_close_input(&util_audio_muxer_format_context[session]);
	if(util_video_muxer_init[session])
		avformat_close_input(&util_video_muxer_format_context[session]);

	util_audio_muxer_init[session] = false;
	util_video_muxer_init[session] = false;
}
#endif //DEF_MUXER_API_ENABLE
