#include "headers.hpp"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

extern "C" void memcpy_asm(u8*, u8*, int);

int util_audio_decoder_stream_num[2] = { -1, -1, };
AVPacket* util_audio_decoder_packet[2] = { NULL, NULL, };
AVPacket* util_audio_decoder_cache_packet[2] = { NULL, NULL, };
AVFrame* util_audio_decoder_raw_data[2] = { NULL, NULL, };
AVCodecContext* util_audio_decoder_context[2] = { NULL, NULL, };
const AVCodec* util_audio_decoder_codec[2] = { NULL, NULL, };
SwrContext* util_audio_decoder_swr_context[2] = { NULL, NULL, };

bool util_video_decoder_mvd_first = false;
bool util_video_decoder_lock[2][6] = { { false, false, false, false, false, false, }, { false, false, false, false, false, false, } };
int util_video_decoder_buffer_num[2][2] = { { 0, 3, }, { 0, 3, }, };
int util_video_decoder_ready_buffer_num[2][2] = { { 0, 3, }, { 0, 3, }, };
int util_video_decoder_stream_num[2][2] = { { -1, -1, }, { -1, -1, }, };
u8* util_video_decoder_mvd_raw_data = NULL;
AVPacket* util_video_decoder_packet[2][2] = { { NULL, NULL, }, { NULL, NULL, } };
AVPacket* util_video_decoder_cache_packet[2][2] = { { NULL, NULL, }, { NULL, NULL, } };
AVFrame* util_video_decoder_raw_data[2][6] = { { NULL, NULL, NULL, NULL, NULL, NULL, }, { NULL, NULL, NULL, NULL, NULL, NULL, } };
AVCodecContext* util_video_decoder_context[2][2] = { { NULL, NULL, }, { NULL, NULL, } };
const AVCodec* util_video_decoder_codec[2][2] = { { NULL, NULL, }, { NULL, NULL, } };

AVFormatContext* util_decoder_format_context[2] = { NULL, NULL, };

MVDSTD_Config util_decoder_mvd_config;

Result_with_string Util_mvd_video_decoder_init(void)
{
	Result_with_string result;

	result.code = mvdstdInit(MVDMODE_VIDEOPROCESSING, MVD_INPUT_H264, MVD_OUTPUT_BGR565, MVD_DEFAULT_WORKBUF_SIZE * 2, NULL);
	if(result.code != 0)
		result.string = "mvdstdInit() failed. ";
	else
		util_video_decoder_mvd_first = true;

	return result;
}

Result_with_string Util_decoder_open_file(std::string file_path, bool* has_audio, bool* has_video, bool* has_3d, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;
	*has_audio = false;
	*has_video = false;
	*has_3d = false;

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

	util_audio_decoder_stream_num[session] = -1;
	util_video_decoder_stream_num[session][0] = -1;
	util_video_decoder_stream_num[session][1] = -1;
	for(int i = 0; i < (int)util_decoder_format_context[session]->nb_streams; i++)
	{
		if(util_decoder_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			*has_audio = true;
			util_audio_decoder_stream_num[session] = i;
		}
		else if(util_video_decoder_stream_num[session][0] == -1 && util_decoder_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			*has_video = true;
			util_video_decoder_stream_num[session][0] = i;
		}
		else if(util_video_decoder_stream_num[session][1] == -1 && util_decoder_format_context[session]->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			*has_3d = true;
			util_video_decoder_stream_num[session][1] = i;
		}
	}

	if(util_audio_decoder_stream_num[session] == -1 && util_video_decoder_stream_num[session][0] == -1)
	{
		result.error_description = "No audio and video data";
		goto fail;
	}
	return result;

	fail:

	Util_decoder_close_file(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_audio_decoder_init(int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	util_audio_decoder_codec[session] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session]]->codecpar->codec_id);
	if(!util_audio_decoder_codec[session])
	{
		result.error_description = "avcodec_find_decoder() failed";
		goto fail;
	}

	util_audio_decoder_context[session] = avcodec_alloc_context3(util_audio_decoder_codec[session]);
	if(!util_audio_decoder_context[session])
	{
		result.error_description = "avcodec_alloc_context3() failed";
		goto fail;
	}

	ffmpeg_result = avcodec_parameters_to_context(util_audio_decoder_context[session], util_decoder_format_context[session]->streams[util_audio_decoder_stream_num[session]]->codecpar);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_parameters_to_context() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	ffmpeg_result = avcodec_open2(util_audio_decoder_context[session], util_audio_decoder_codec[session], NULL);
	if(ffmpeg_result != 0)
	{
		result.error_description = "avcodec_open2() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	util_audio_decoder_swr_context[session] = swr_alloc();
	swr_alloc_set_opts(util_audio_decoder_swr_context[session], av_get_default_channel_layout(util_audio_decoder_context[session]->channels), AV_SAMPLE_FMT_S16, util_audio_decoder_context[session]->sample_rate,
		av_get_default_channel_layout(util_audio_decoder_context[session]->channels), util_audio_decoder_context[session]->sample_fmt, util_audio_decoder_context[session]->sample_rate, 0, NULL);
	if(!util_audio_decoder_swr_context[session])
	{
		result.error_description = "swr_alloc_set_opts() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	ffmpeg_result = swr_init(util_audio_decoder_swr_context[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "swr_init() failed " + std::to_string(ffmpeg_result);
		goto fail;
	}

	return result;

	fail:

	Util_audio_decoder_exit(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_video_decoder_init(int low_resolution, bool has_3d, int session)
{
	int ffmpeg_result = 0;
	Result_with_string result;

	util_video_decoder_buffer_num[session][0] = 0;
	util_video_decoder_buffer_num[session][1] = 3;
	util_video_decoder_ready_buffer_num[session][0] = 0;
	util_video_decoder_ready_buffer_num[session][1] = 3;
	for(int i = 0; i < 2; i++)
	{
		util_video_decoder_codec[session][i] = avcodec_find_decoder(util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][i]]->codecpar->codec_id);
		if(!util_video_decoder_codec[session][i])
		{
			result.error_description = "avcodec_find_decoder() failed";
			goto fail;
		}

		util_video_decoder_context[session][i] = avcodec_alloc_context3(util_video_decoder_codec[session][i]);
		if(!util_video_decoder_context[session][i])
		{
			result.error_description = "avcodec_alloc_context3() failed";
			goto fail;
		}

		ffmpeg_result = avcodec_parameters_to_context(util_video_decoder_context[session][i], util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->codecpar);
		if(ffmpeg_result != 0)
		{
			result.error_description = "avcodec_parameters_to_context() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		util_video_decoder_context[session][i]->lowres = low_resolution;
		util_video_decoder_context[session][i]->flags = AV_CODEC_FLAG_OUTPUT_CORRUPT;
		ffmpeg_result = avcodec_open2(util_video_decoder_context[session][i], util_video_decoder_codec[session][i], NULL);
		if(ffmpeg_result != 0)
		{
			result.error_description = "avcodec_open2() failed " + std::to_string(ffmpeg_result);
			goto fail;
		}

		if(!has_3d)
			break;
	}

	return result;

	fail:

	Util_video_decoder_exit(session);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_audio_decoder_get_info(int* bitrate, int* sample_rate, int* ch, std::string* format_name, double* duration, int session)
{
	*bitrate = util_audio_decoder_context[session]->bit_rate;
	*sample_rate = util_audio_decoder_context[session]->sample_rate;
	*ch = util_audio_decoder_context[session]->channels;
	*format_name = util_audio_decoder_codec[session]->long_name;
	*duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
}

void Util_video_decoder_get_info(int* width, int* height, double* framerate, std::string* format_name, double* duration, int session)
{
	*width = util_video_decoder_context[session][0]->width;
	*height = util_video_decoder_context[session][0]->height;
	*framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	*format_name = util_video_decoder_codec[session][0]->long_name;
	*duration = (double)util_decoder_format_context[session]->duration / AV_TIME_BASE;
}

Result_with_string Util_decoder_read_packet(std::string* type, int* packet_index, int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;
	AVPacket* cache_packet = NULL;
	*type = "unknown";
	*packet_index = -1;

	cache_packet = av_packet_alloc();
	if(!cache_packet)
	{
		result.error_description = "av_packet_alloc() failed";
		goto fail;
	}

	ffmpeg_result = av_read_frame(util_decoder_format_context[session], cache_packet);
	if(ffmpeg_result != 0)
	{
		result.error_description = "av_read_frame() failed";
		goto fail;
	}

	if(cache_packet->stream_index == util_audio_decoder_stream_num[session])//audio packet
	{
		util_audio_decoder_cache_packet[session] = av_packet_alloc();
		if(!util_audio_decoder_cache_packet[session])
		{
			result.error_description = "av_packet_alloc() failed" + std::to_string(ffmpeg_result);
			goto fail;
		}

		av_packet_unref(util_audio_decoder_cache_packet[session]);
		ffmpeg_result = av_packet_ref(util_audio_decoder_cache_packet[session], cache_packet);
		if(ffmpeg_result != 0)
		{
			result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result);
			goto fail;
		}
		*type = "audio";
	}
	else
	{
		for(int i = 0; i < 2; i++)
		{
			if(cache_packet->stream_index == util_video_decoder_stream_num[session][i])//video packet
			{
				util_video_decoder_cache_packet[session][i] = av_packet_alloc();
				if(!util_video_decoder_cache_packet[session][i])
				{
					result.error_description = "av_packet_alloc() failed";
					goto fail;
				}

				av_packet_unref(util_video_decoder_cache_packet[session][i]);
				ffmpeg_result = av_packet_ref(util_video_decoder_cache_packet[session][i], cache_packet);
				if(ffmpeg_result != 0)
				{
					result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result);
					goto fail;
				}
				*packet_index = i;
				*type = "video";
				break;
			}
		}
	}

	av_packet_free(&cache_packet);
	return result;

	fail:

	av_packet_free(&util_audio_decoder_cache_packet[session]);
	av_packet_free(&util_video_decoder_cache_packet[session][0]);
	av_packet_free(&util_video_decoder_cache_packet[session][1]);
	av_packet_free(&cache_packet);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_decoder_ready_audio_packet(int session)
{
	Result_with_string result;
	int ffmpeg_result = 0;

	av_packet_free(&util_audio_decoder_packet[session]);
	util_audio_decoder_packet[session] = av_packet_alloc();
	if(!util_audio_decoder_packet[session])
	{
		result.error_description = "av_packet_alloc() failed";
		goto fail;
	}

	av_packet_unref(util_audio_decoder_packet[session]);
	ffmpeg_result = av_packet_ref(util_audio_decoder_packet[session], util_audio_decoder_cache_packet[session]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result);
		goto fail;
	}

	av_packet_free(&util_audio_decoder_cache_packet[session]);
	return result;

	fail:

	av_packet_free(&util_audio_decoder_packet[session]);
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
		result.error_description = "av_packet_alloc() failed";
		goto fail;
	}

	av_packet_unref(util_video_decoder_packet[session][packet_index]);
	ffmpeg_result = av_packet_ref(util_video_decoder_packet[session][packet_index], util_video_decoder_cache_packet[session][packet_index]);
	if(ffmpeg_result != 0)
	{
		result.error_description = "av_packet_ref() failed" + std::to_string(ffmpeg_result);
		goto fail;
	}

	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
	return result;

	fail:

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_decoder_skip_audio_packet(int session)
{
	av_packet_free(&util_audio_decoder_cache_packet[session]);
}

void Util_decoder_skip_video_packet(int packet_index, int session)
{
	av_packet_free(&util_video_decoder_cache_packet[session][packet_index]);
}

Result_with_string Util_audio_decoder_decode(int* size, u8** raw_data, double* current_pos, int session)
{
	int ffmpeg_result = 0;
	double current_frame = (double)util_audio_decoder_packet[session]->dts / util_audio_decoder_packet[session]->duration;
	Result_with_string result;
	*size = 0;
	*current_pos = 0;
	
	util_audio_decoder_raw_data[session] = av_frame_alloc();
	if(!util_audio_decoder_raw_data[session])
	{
		result.error_description = "av_frame_alloc() failed";
		goto fail;
	}
	
	ffmpeg_result = avcodec_send_packet(util_audio_decoder_context[session], util_audio_decoder_packet[session]);
	if(ffmpeg_result == 0)
	{
		ffmpeg_result = avcodec_receive_frame(util_audio_decoder_context[session], util_audio_decoder_raw_data[session]);
		if(ffmpeg_result == 0)
		{
			*current_pos = current_frame * ((1000.0 / util_audio_decoder_raw_data[session]->sample_rate) * util_audio_decoder_raw_data[session]->nb_samples);//calc pos

			*raw_data = (u8*)malloc(util_audio_decoder_raw_data[session]->nb_samples * 2 * util_audio_decoder_context[session]->channels);
			*size = swr_convert(util_audio_decoder_swr_context[session], raw_data, util_audio_decoder_raw_data[session]->nb_samples, (const uint8_t**)util_audio_decoder_raw_data[session]->data, util_audio_decoder_raw_data[session]->nb_samples);
			*size *= 2;
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

	av_packet_free(&util_audio_decoder_packet[session]);
	av_frame_free(&util_audio_decoder_raw_data[session]);
	return result;

	fail:

	av_packet_free(&util_audio_decoder_packet[session]);
	av_frame_free(&util_audio_decoder_raw_data[session]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_video_decoder_decode(int* width, int* height, bool* key_frame, double* current_pos, int packet_index, int session)
{
	int ffmpeg_result = 0;
	int count = 0;
	int buffer_num = util_video_decoder_buffer_num[session][packet_index];
	double framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	double current_frame = (double)util_video_decoder_packet[session][packet_index]->dts / util_video_decoder_packet[session][packet_index]->duration;
	Result_with_string result;
	*width = 0;
	*height = 0;
	*current_pos = 0;
	if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos

	if(util_video_decoder_packet[session][packet_index]->flags == 1)
		*key_frame = true;
	else
		*key_frame = false;
	
	util_video_decoder_raw_data[session][buffer_num] = av_frame_alloc();
	if(!util_video_decoder_raw_data[session][buffer_num])
	{
		result.error_description = "av_frame_alloc() failed";
		goto fail;
	}

	ffmpeg_result = avcodec_send_packet(util_video_decoder_context[session][packet_index], util_video_decoder_packet[session][packet_index]);
	if(ffmpeg_result == 0)
	{
		ffmpeg_result = avcodec_receive_frame(util_video_decoder_context[session][packet_index], util_video_decoder_raw_data[session][buffer_num]);
		if(ffmpeg_result == 0)
		{
			*width = util_video_decoder_raw_data[session][buffer_num]->width;
			*height = util_video_decoder_raw_data[session][buffer_num]->height;
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

	if(buffer_num == 0)
		util_video_decoder_buffer_num[session][packet_index] = 1;
	else if(buffer_num == 1)
		util_video_decoder_buffer_num[session][packet_index] = 2;
	else if (buffer_num == 2)
		util_video_decoder_buffer_num[session][packet_index] = 0;
	else if(buffer_num == 3)
		util_video_decoder_buffer_num[session][packet_index] = 4;
	else if(buffer_num == 4)
		util_video_decoder_buffer_num[session][packet_index] = 5;
	else
		util_video_decoder_buffer_num[session][packet_index] = 3;

	buffer_num = util_video_decoder_buffer_num[session][packet_index];
	while(util_video_decoder_lock[session][buffer_num])
	{
		count++;
		if(count > 10000)//time out 1000ms
			break;
	
		usleep(100);
	}

	if(buffer_num == 0)
		util_video_decoder_ready_buffer_num[session][packet_index] = 2;
	else if(buffer_num == 1)
		util_video_decoder_ready_buffer_num[session][packet_index] = 0;
	else if(buffer_num == 2)
		util_video_decoder_ready_buffer_num[session][packet_index] = 1;
	else if(buffer_num == 3)
		util_video_decoder_ready_buffer_num[session][packet_index] = 5;
	else if(buffer_num == 4)
		util_video_decoder_ready_buffer_num[session][packet_index] = 3;
	else
		util_video_decoder_ready_buffer_num[session][packet_index] = 4;

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	av_frame_free(&util_video_decoder_raw_data[session][buffer_num]);
	return result;

	fail:

	av_packet_free(&util_video_decoder_packet[session][packet_index]);
	av_frame_free(&util_video_decoder_raw_data[session][buffer_num]);
	result.code = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS;
	result.string = DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_mvd_video_decoder_decode(int* width, int* height, bool* key_frame, double* current_pos, int session)
{
	int offset = 0;
	int source_offset = 0;
	int size = 0;
	double framerate = (double)util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.num / util_decoder_format_context[session]->streams[util_video_decoder_stream_num[session][0]]->avg_frame_rate.den;
	double current_frame = (double)util_video_decoder_packet[session][0]->dts / util_video_decoder_packet[session][0]->duration;
	u8* packet = NULL;
	Result_with_string result;
	*width = util_video_decoder_context[session][0]->width;
	*height = util_video_decoder_context[session][0]->height;

	if(*width % 16 != 0)
		*width += 16 - *width % 16;
	if(*height % 16 != 0)
		*height += 16 - *height % 16;

	*current_pos = 0;
	if(framerate != 0.0)
		*current_pos = current_frame * (1000 / framerate);//calc frame pos

	if(util_video_decoder_packet[session][0]->flags == 1)
		*key_frame = true;
	else
		*key_frame = false;
	
	mvdstdGenerateDefaultConfig(&util_decoder_mvd_config, *width, *height, *width, *height, NULL, NULL, NULL);
	
	packet = (u8*)linearAlloc(util_video_decoder_packet[session][0]->size);	
	if(util_video_decoder_mvd_first)
	{
		//set extra data
		offset = 0;
		memset(packet, 0x0, 0x2);
		offset += 2;
		memset(packet + offset, 0x1, 0x1);
		offset += 1;
		memcpy(packet + offset, util_video_decoder_context[session][0]->extradata + 8, *(util_video_decoder_context[session][0]->extradata + 7));
		offset += *(util_video_decoder_context[session][0]->extradata + 7);

		result.code = mvdstdProcessVideoFrame(packet, offset, 0, NULL);

		offset = 0;
		memset(packet, 0x0, 0x2);
		offset += 2;
		memset(packet + offset, 0x1, 0x1);
		offset += 1;
		memcpy(packet + offset, util_video_decoder_context[session][0]->extradata + 11 + *(util_video_decoder_context[session][0]->extradata + 7), *(util_video_decoder_context[session][0]->extradata + 10 + *(util_video_decoder_context[session][0]->extradata + 7)));
		offset += *(util_video_decoder_context[session][0]->extradata + 10 + *(util_video_decoder_context[session][0]->extradata + 7));

		result.code = mvdstdProcessVideoFrame(packet, offset, 0, NULL);
	}

	offset = 0;
	source_offset = 0;

	while(source_offset + 4 < util_video_decoder_packet[session][0]->size)
	{
		//get nal size
		size = *((int*)(util_video_decoder_packet[session][0]->data + source_offset));
		size = __builtin_bswap32(size);
		source_offset += 4;

		//set nal prefix 0x0 0x0 0x1
		memset(packet + offset, 0x0, 0x2);
		offset += 2;
		memset(packet + offset, 0x1, 0x1);
		offset += 1;

		//copy raw nal data
		memcpy(packet + offset, (util_video_decoder_packet[session][0]->data + source_offset), size);
		offset += size;
		source_offset += size;
	}

	if(!util_video_decoder_mvd_raw_data)
		util_video_decoder_mvd_raw_data = (u8*)linearAlloc(*width * *height * 2);

	util_decoder_mvd_config.physaddr_outdata0 = osConvertVirtToPhys(util_video_decoder_mvd_raw_data);

	result.code = mvdstdProcessVideoFrame(packet, offset, 0, NULL);

	if(util_video_decoder_mvd_first)
	{
		//Do I need to send same nal data at first frame?
		result.code = mvdstdProcessVideoFrame(packet, offset, 0, NULL);
		util_video_decoder_mvd_first = false;
	}

	if(result.code == MVD_STATUS_FRAMEREADY)
		result.code = 0;

	linearFree(packet);
	packet = NULL;
	av_packet_free(&util_video_decoder_packet[session][0]);
	return result;
}

Result_with_string Util_video_decoder_get_image(u8** raw_data, int width, int height, int packet_index, int session)
{
	int cpy_size[2] = { 0, 0, };
	int buffer_num = util_video_decoder_ready_buffer_num[session][packet_index];
	Result_with_string result;
	util_video_decoder_lock[session][buffer_num] = true;//lock

	cpy_size[0] = (width * height);
	cpy_size[1] = cpy_size[0] / 4;
	cpy_size[0] -= cpy_size[0] % 32;
	cpy_size[1] -= cpy_size[1] % 32;
	*raw_data = (u8*)malloc(width * height * 1.5);
	if(*raw_data == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		util_video_decoder_lock[session][buffer_num] = false;//unlock
		return result;
	}

	memcpy_asm(*raw_data, util_video_decoder_raw_data[session][buffer_num]->data[0], cpy_size[0]);
	memcpy_asm(*raw_data + (width * height), util_video_decoder_raw_data[session][buffer_num]->data[1], cpy_size[1]);
	memcpy_asm(*raw_data + (width * height) + (width * height / 4), util_video_decoder_raw_data[session][buffer_num]->data[2], cpy_size[1]);

	util_video_decoder_lock[session][buffer_num] = false;//unlock
	return result;
}

Result_with_string Util_mvd_video_decoder_get_image(u8** raw_data, int width, int height, int session)
{
	int cpy_size = 0;
	Result_with_string result;

	cpy_size = (width * height * 2);
	cpy_size -= cpy_size % 32;
	*raw_data = (u8*)malloc(width * height * 2);
	if(*raw_data == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	result.code = mvdstdRenderVideoFrame(&util_decoder_mvd_config, true);
	result.code = 0;

	memcpy_asm(*raw_data, util_video_decoder_mvd_raw_data, cpy_size);
	
	return result;
}

Result_with_string Util_decoder_seek(u64 seek_pos, int flag, int session)
{
	int ffmpeg_result;
	Result_with_string result;

	ffmpeg_result = avformat_seek_file(util_decoder_format_context[session], -1, seek_pos, seek_pos, seek_pos, flag);//AVSEEK_FLAG_FRAME 8 AVSEEK_FLAG_ANY 4  AVSEEK_FLAG_BACKWORD 1
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
	avformat_close_input(&util_decoder_format_context[session]);
}

void Util_audio_decoder_exit(int session)
{
	avcodec_free_context(&util_audio_decoder_context[session]);
	av_packet_free(&util_audio_decoder_packet[session]);
	av_packet_free(&util_audio_decoder_cache_packet[session]);
	av_frame_free(&util_audio_decoder_raw_data[session]);
	swr_free(&util_audio_decoder_swr_context[session]);
}

void Util_video_decoder_exit(int session)
{
	for(int i = 0; i < 2; i ++)
	{
		avcodec_free_context(&util_video_decoder_context[session][i]);
		av_packet_free(&util_video_decoder_packet[session][i]);
		av_packet_free(&util_video_decoder_cache_packet[session][i]);
	}

	util_video_decoder_ready_buffer_num[session][0] = 0;
	util_video_decoder_ready_buffer_num[session][1] = 3;
	util_video_decoder_buffer_num[session][0] = 0;
	util_video_decoder_buffer_num[session][1] = 3;

	for(int i = 0; i < 6; i ++)
		av_frame_free(&util_video_decoder_raw_data[session][i]);
}

void Util_mvd_video_decoder_exit(void)
{
	mvdstdExit();
	linearFree(util_video_decoder_mvd_raw_data);
	util_video_decoder_mvd_raw_data = NULL;
}

Result_with_string Util_image_decoder_decode(std::string file_name, u8** raw_data, int* width, int* height)
{
	Result_with_string result;
	int image_ch = 0;
	*raw_data = stbi_load(file_name.c_str(), width, height, &image_ch, STBI_rgb);

	return result;
}
