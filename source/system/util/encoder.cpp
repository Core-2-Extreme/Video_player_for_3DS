#include "headers.hpp"

extern "C" 
{
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
}

extern "C" void memcpy_asm(u8*, u8*, int);

int util_audio_pos[3] = { 0, 0, 0, };
int util_audio_increase_pts[3] = { 0, 0, 0, };
AVPacket* util_audio_encoder_packet[3] = { NULL, NULL, NULL, };
AVFrame* util_audio_encoder_raw_data[3] = { NULL, NULL, NULL, };
AVCodecContext* util_audio_encoder_context[3] = { NULL, NULL, NULL, };
const AVCodec* util_audio_encoder_codec[3] = { NULL, NULL, NULL, };
SwrContext* util_audio_encoder_swr_context[3] = { NULL, NULL, NULL, };
AVStream* util_audio_encoder_stream[3] = { NULL, NULL, NULL, };

int util_video_pos[3] = { 0, 0, 0, };
int util_video_increase_pts[3] = { 0, 0, 0, };
AVPacket* util_video_encoder_packet[3] = { NULL, NULL, NULL, };
AVFrame* util_video_encoder_raw_data[3] = { NULL, NULL, NULL, };
AVCodecContext* util_video_encoder_context[3] = { NULL, NULL, NULL, };
const AVCodec* util_video_encoder_codec[3] = { NULL, NULL, NULL, };
AVStream* util_video_encoder_stream[3] = { NULL, NULL, NULL, };

AVFormatContext* util_encoder_format_context[3] = { NULL, NULL, NULL, };

Result_with_string Util_encoder_create_output_file(std::string file_name, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	util_encoder_format_context[session] = avformat_alloc_context();
	if(!util_encoder_format_context[session])
	{
		result.error_description = "avformat_alloc_context() failed";
		goto fail;
	}

	util_encoder_format_context[session]->oformat = av_guess_format(NULL, file_name.c_str(), NULL);
	if(!util_encoder_format_context[session]->oformat)
	{
		result.error_description = "av_guess_format() failed";
		goto fail;
	}

	ffmpeg_result = avio_open(&util_encoder_format_context[session]->pb, file_name.c_str(), AVIO_FLAG_READ_WRITE);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avio_open() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	return result;

	fail:

	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	avio_close(util_encoder_format_context[session]->pb);
	avformat_free_context(util_encoder_format_context[session]);
	return result;
}

Result_with_string Util_audio_encoder_init(AVCodecID codec, int original_sample_rate, int encode_sample_rate, int bitrate, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	util_audio_pos[session] = 0;
	util_audio_encoder_codec[session] = avcodec_find_encoder(codec);
	if(!util_audio_encoder_codec[session])
	{
		result.error_description = "avcodec_find_encoder() failed";
		goto fail;
	}

	util_audio_encoder_context[session] = avcodec_alloc_context3(util_audio_encoder_codec[session]);
	if(!util_audio_encoder_context[session])
	{
		result.error_description = "avcodec_alloc_context3() failed";
		goto fail;
	}

	if(codec == AV_CODEC_ID_MP2)
		util_audio_encoder_context[session]->sample_fmt = AV_SAMPLE_FMT_S16;
	else if(codec == AV_CODEC_ID_ALAC || codec == AV_CODEC_ID_FLAC)
		util_audio_encoder_context[session]->sample_fmt = AV_SAMPLE_FMT_S16P;
	else
		util_audio_encoder_context[session]->sample_fmt = AV_SAMPLE_FMT_FLT;
	
	util_audio_encoder_context[session]->bit_rate = bitrate;
	util_audio_encoder_context[session]->sample_rate = encode_sample_rate;
	util_audio_encoder_context[session]->channel_layout = AV_CH_LAYOUT_MONO;
	util_audio_encoder_context[session]->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_MONO);
	util_audio_encoder_context[session]->codec_type = AVMEDIA_TYPE_AUDIO;
	util_audio_encoder_context[session]->time_base = (AVRational){ 1, encode_sample_rate };
	if(codec == AV_CODEC_ID_AAC)
		util_audio_encoder_context[session]->profile = FF_PROFILE_AAC_LOW;

	ffmpeg_result = avcodec_open2(util_audio_encoder_context[session], util_audio_encoder_codec[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_open2() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}
	
	util_audio_increase_pts[session] = util_audio_encoder_context[session]->frame_size;
	util_audio_encoder_packet[session] = av_packet_alloc();
	util_audio_encoder_raw_data[session] = av_frame_alloc();
	if(!util_audio_encoder_raw_data[session] || !util_audio_encoder_packet)
	{
		result.error_description = "av_packet_alloc() / av_frame_alloc() failed";
		goto fail;
	}
	
	util_audio_encoder_raw_data[session]->nb_samples = util_audio_encoder_context[session]->frame_size;
	util_audio_encoder_raw_data[session]->format = util_audio_encoder_context[session]->sample_fmt;
	util_audio_encoder_raw_data[session]->channel_layout = util_audio_encoder_context[session]->channel_layout;

	ffmpeg_result = av_frame_get_buffer(util_audio_encoder_raw_data[session], 0);
	if(ffmpeg_result != 0)
	{
		result.error_description = "av_frame_get_buffer() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	av_frame_make_writable(util_audio_encoder_raw_data[session]);

	util_audio_encoder_swr_context[session] = swr_alloc();
	swr_alloc_set_opts(util_audio_encoder_swr_context[session], av_get_default_channel_layout(util_audio_encoder_context[session]->channels), util_audio_encoder_context[session]->sample_fmt, util_audio_encoder_context[session]->sample_rate,
	av_get_default_channel_layout(util_audio_encoder_context[session]->channels), AV_SAMPLE_FMT_S16, original_sample_rate, 0, NULL);
	if(!util_audio_encoder_swr_context[session])
	{
		result.error_description = "swr_alloc_set_opts() failed";
		goto fail;
	}

	ffmpeg_result = swr_init(util_audio_encoder_swr_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "swr_init() failed";
		goto fail;
	}

	util_audio_encoder_stream[session] = avformat_new_stream(util_encoder_format_context[session], util_audio_encoder_codec[session]);
	if(!util_audio_encoder_stream[session])
	{
		result.error_description = "avformat_new_stream() failed";
		goto fail;
	}

	if (util_encoder_format_context[session]->oformat->flags & AVFMT_GLOBALHEADER)
		util_encoder_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ffmpeg_result = avcodec_parameters_from_context(util_audio_encoder_stream[session]->codecpar, util_audio_encoder_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_parameters_from_context() failed";
		goto fail;
	}

	return result;

	fail:
	Util_audio_encoder_exit(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_video_encoder_init(AVCodecID codec, int width, int height, int fps, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	util_video_pos[session] = 0;
	util_video_encoder_codec[session] = avcodec_find_encoder(codec);
	if(!util_video_encoder_codec[session])
	{
		result.error_description = "avcodec_find_encoder() failed";
		goto fail;
	}

	util_video_encoder_context[session] = avcodec_alloc_context3(util_video_encoder_codec[session]);
	if(!util_video_encoder_context[session])
	{
		result.error_description = "avcodec_alloc_context3() failed";
		goto fail;
	}
	
	util_video_encoder_context[session]->bit_rate = 100000;
	util_video_encoder_context[session]->width = width;
	util_video_encoder_context[session]->height = height;
	util_video_encoder_context[session]->time_base = (AVRational){ 1, fps };
	util_video_encoder_context[session]->pix_fmt = AV_PIX_FMT_YUV420P;
	util_video_encoder_context[session]->gop_size = fps;
	//util_video_encoder_context[session]->flags2 = AV_CODEC_FLAG2_FAST;
	/*util_video_encoder_context[session]->flags |= AV_CODEC_FLAG_QSCALE;
	util_video_encoder_context[session]->global_quality = 31;
	util_video_encoder_context[session]->qmin = 1;
	util_video_encoder_context[session]->qmax = 1;
	util_video_encoder_context[session]->i_quant_factor = 0.1;
	util_video_encoder_context[session]->qcompress = 0.1;
	*/
	
	if(codec == AV_CODEC_ID_H264)
	{
		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "crf", "32", 0);
		if(ffmpeg_result < 0)
		{
			result.error_description = "av_opt_set() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "profile", "baseline", 0);
		if(ffmpeg_result < 0)
		{
			result.error_description = "av_opt_set() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "preset", "ultrafast", 0);
		if(ffmpeg_result < 0)
		{
			result.error_description = "av_opt_set() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		ffmpeg_result = av_opt_set(util_video_encoder_context[session]->priv_data, "me_method", "dia", 0);
		if(ffmpeg_result < 0)
		{
			result.error_description = "av_opt_set() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}
	}

	ffmpeg_result = avcodec_open2(util_video_encoder_context[session], util_video_encoder_codec[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_open2() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}
	
	util_video_increase_pts[session] = 90000 / fps;

	util_video_encoder_packet[session] = av_packet_alloc();
	util_video_encoder_raw_data[session] = av_frame_alloc();
	if(!util_video_encoder_raw_data[session] || !util_video_encoder_packet[session])
	{
		result.error_description = "av_packet_alloc() / av_frame_alloc() failed";
		goto fail;
	}
	
	util_video_encoder_raw_data[session]->format = util_video_encoder_context[session]->pix_fmt;
	util_video_encoder_raw_data[session]->width = util_video_encoder_context[session]->width;
	util_video_encoder_raw_data[session]->height = util_video_encoder_context[session]->height;
	ffmpeg_result = av_frame_get_buffer(util_video_encoder_raw_data[session], 0);
	if(ffmpeg_result < 0)
	{
		result.error_description = "av_image_alloc() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}
	/*Util_log_save("", std::to_string(util_video_encoder_raw_data[session]->linesize[0]));
	Util_log_save("", std::to_string(util_video_encoder_raw_data[session]->linesize[1]));
	Util_log_save("", std::to_string(util_video_encoder_raw_data[session]->linesize[2]));
	Util_log_save("", std::to_string(util_video_encoder_raw_data[session]->linesize[3]));*/

	av_frame_make_writable(util_video_encoder_raw_data[session]);

	util_video_encoder_stream[session] = avformat_new_stream(util_encoder_format_context[session], util_video_encoder_codec[session]);
	if(!util_video_encoder_stream[session])
	{
		result.error_description = "avformat_new_stream() failed";
		goto fail;
	}
	
	if (util_encoder_format_context[session]->oformat->flags & AVFMT_GLOBALHEADER)
		util_encoder_format_context[session]->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ffmpeg_result = avcodec_parameters_from_context(util_video_encoder_stream[session]->codecpar, util_video_encoder_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_parameters_from_context() failed";
		goto fail;
	}

	return result;

	fail:
	Util_video_encoder_exit(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_encoder_write_header(int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	ffmpeg_result = avformat_write_header(util_encoder_format_context[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
		result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
		result.error_description = "avformat_write_header() failed";
	}

	return result;
}

Result_with_string Util_audio_encoder_encode(int size, u8* raw_data, int session)
{
	int encode_offset = 0;
	int ffmpeg_result = 0;
	int one_frame_size = av_samples_get_buffer_size(NULL, util_audio_encoder_context[session]->channels, util_audio_encoder_context[session]->frame_size, util_audio_encoder_context[session]->sample_fmt, 0);
	int out_samples = 0;
	int bytes_per_sample = av_get_bytes_per_sample(util_audio_encoder_context[session]->sample_fmt);
	u8* swr_in_cache[1] = { NULL, };
	u8* swr_out_cache[1] = { NULL, };
	Result_with_string result;

	swr_in_cache[0] = raw_data;
	swr_out_cache[0] = (u8*)malloc(size * bytes_per_sample);
	if(swr_out_cache[0] == NULL)
		goto fail_;

	out_samples = swr_convert(util_audio_encoder_swr_context[session], (uint8_t**)swr_out_cache, size, (const uint8_t**)swr_in_cache, size);
	out_samples = out_samples / 2;
	out_samples *= bytes_per_sample;

	while(true)
	{
		memcpy(util_audio_encoder_raw_data[session]->data[0], swr_out_cache[0] + encode_offset, one_frame_size);
		//set pts
		util_audio_encoder_raw_data[session]->pts = util_audio_pos[session];
		util_audio_pos[session] += util_audio_increase_pts[session];

		ffmpeg_result = avcodec_send_frame(util_audio_encoder_context[session], util_audio_encoder_raw_data[session]);
		if(ffmpeg_result != 0)
		{
			result.error_description = "avcodec_send_frame() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		ffmpeg_result = avcodec_receive_packet(util_audio_encoder_context[session], util_audio_encoder_packet[session]);
		if(ffmpeg_result == 0)
		{
			util_audio_encoder_packet[session]->stream_index = 0;
			ffmpeg_result = av_interleaved_write_frame(util_encoder_format_context[session], util_audio_encoder_packet[session]);
			av_packet_unref(util_audio_encoder_packet[session]);
			if(ffmpeg_result != 0)
			{
				result.error_description = "av_interleaved_write_frame() failed " + std::to_string(ffmpeg_result);
				goto fail;
			}
		}
		else
			av_packet_unref(util_audio_encoder_packet[session]);

		out_samples -= one_frame_size;
		encode_offset += one_frame_size;
		if(one_frame_size > out_samples)
			break;
	}
	free(swr_out_cache[0]);
	swr_out_cache[0] = NULL;

	return result;

	fail:

	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;

	fail_:

	free(swr_out_cache[0]);
	swr_out_cache[0] = NULL;
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

Result_with_string Util_video_encoder_encode(u8* yuv420p, int session)
{
	int ffmpeg_result = 0;
	int width = util_video_encoder_raw_data[session]->width;
	int height = util_video_encoder_raw_data[session]->height;
	Result_with_string result;

	memcpy(util_video_encoder_raw_data[session]->data[0], yuv420p, width * height);
	memcpy(util_video_encoder_raw_data[session]->data[1], yuv420p + width * height, width * height / 4);
	memcpy(util_video_encoder_raw_data[session]->data[2], yuv420p + width * height + width * height / 4, width * height / 4);
	util_video_encoder_raw_data[session]->linesize[0] = util_video_encoder_raw_data[session]->width;
	util_video_encoder_raw_data[session]->linesize[1] = util_video_encoder_raw_data[session]->width / 2;
	util_video_encoder_raw_data[session]->linesize[2] = util_video_encoder_raw_data[session]->width / 2;

	//set pts
	util_video_encoder_raw_data[session]->pts = util_video_pos[session];
	util_video_pos[session] += util_video_increase_pts[session];
	
	ffmpeg_result = avcodec_send_frame(util_video_encoder_context[session], util_video_encoder_raw_data[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_send_frame() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	ffmpeg_result = avcodec_receive_packet(util_video_encoder_context[session], util_video_encoder_packet[session]);
	if(ffmpeg_result == 0)
	{
		util_video_encoder_packet[session]->stream_index = 0;
		ffmpeg_result = av_interleaved_write_frame(util_encoder_format_context[session], util_video_encoder_packet[session]);
		av_packet_unref(util_video_encoder_packet[session]);
		if(ffmpeg_result != 0)
		{
			result.error_description = "av_interleaved_write_frame() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}
	}
	else
		av_packet_unref(util_video_encoder_packet[session]);

	return result;

	fail:

	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_encoder_close_output_file(int session)
{
	av_write_trailer(util_encoder_format_context[session]);
	avio_close(util_encoder_format_context[session]->pb);
	avformat_free_context(util_encoder_format_context[session]);
}

void Util_audio_encoder_exit(int session)
{
	avcodec_free_context(&util_audio_encoder_context[session]);
	av_packet_free(&util_audio_encoder_packet[session]);
	av_frame_free(&util_audio_encoder_raw_data[session]);
	swr_free(&util_audio_encoder_swr_context[session]);	
}

void Util_video_encoder_exit(int session)
{
	avcodec_free_context(&util_video_encoder_context[session]);
	av_packet_free(&util_video_encoder_packet[session]);
	av_frame_free(&util_video_encoder_raw_data[session]);
}
