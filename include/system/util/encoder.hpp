#pragma once

extern "C" 
{
#include "libavcodec/avcodec.h"
}

Result_with_string Util_encoder_create_output_file(std::string file_name, int session);

Result_with_string Util_audio_encoder_init(AVCodecID format, int original_sample_rate, int encode_sample_rate, int bitrate, int session);

Result_with_string Util_video_encoder_init(AVCodecID format, int width, int height, int fps, int session);

Result_with_string Util_audio_encoder_encode(int size, u8* raw_data, int session);

Result_with_string Util_video_encoder_encode(u8* yuv420p, int session);

void Util_encoder_close_output_file(int session);

void Util_audio_encoder_exit(int session);

void Util_video_encoder_exit(int session);
