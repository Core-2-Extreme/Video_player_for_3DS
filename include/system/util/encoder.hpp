#pragma once

extern "C" 
{
#include "libavcodec/avcodec.h"
}

/**
 * @brief Create the output file.
 * @param file_path (in) Output file path.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_encoder_create_output_file(std::string file_path, int session);

/**
 * @brief Initialize a audio encoder.
 * @param codec (in) Audio codec (DEF_ENCODER_AUDIO_CODEC_*).
 * @param original_sample_rate (in) Input audio sample rate (in Hz).
 * @param encode_sample_rate (in) Encode audio sample rate (in Hz) (audio data will be converted 
 * original_sample_rate -> encode_sample_rate internally when you call Util_audio_encoder_encode()).
 * @param bitrate (in) Audio bitrate (in bit).
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_audio_encoder_init(int codec, int original_sample_rate, int encode_sample_rate, int bitrate, int session);

/**
 * @brief Initialize a video encoder.
 * @param codec (in) Video codec (DEF_ENCODER_VIDEO_CODEC_*).
 * @param width (in) Video width.
 * @param height (in) Video height.
 * @param bitrate (in) Video bitrate (in bit).
 * @param fps (in) Video framerate.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_video_encoder_init(int codec, int width, int height, int bitrate, int fps, int session);

/**
 * @brief Write a header (if needed by container).
 * Call it after calling Util_*_encoder_init().
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_encoder_write_header(int session);

/**
 * @brief Encode audio.
 * @param size (in) Raw audio data size (in byte).
 * @param raw_audio (in) Pointer for raw audio data (PCM_S16LE).
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_audio_encoder_encode(int size, u8* raw_data, int session);

/**
 * @brief Encode a image.
 * @param raw_image (in) Pointer for raw image data (yuv420p).
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_video_encoder_encode(u8* raw_image, int session);

/**
 * @brief Uninitialize encoders and close the output file.
 * Do nothing if file is not opend.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_encoder_close_output_file(int session);

/**
 * @brief Encode image file.
 * @param file_path (in) File path.
 * @param raw_data (in) Pointer for raw image (RGB888LE).
 * @param width (in) Image width.
 * @param height (in) Image height.
 * @param format (in) Image format DEF_ENCODER_IMAGE_CODEC_*.
 * @param quality (in) Image quality (jpg only).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_image_encoder_encode(std::string file_path, u8* raw_data, int width, int height, int format, int quality);
