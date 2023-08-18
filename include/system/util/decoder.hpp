#ifndef DECODER_HPP
#define DECODER_HPP

#if (defined(DEF_ENABLE_VIDEO_AUDIO_DECODER_API) || defined(DEF_ENABLE_IMAGE_DECODER_API))
#include "system/types.hpp"
#endif

#if DEF_ENABLE_VIDEO_AUDIO_DECODER_API

/**
 * @brief Open media file.
 * @param file_path (in) File path.
 * @param num_of_audio_tracks (out) Number of audio tracks.
 * @param num_of_video_tracks (out) Number of video tracks.
 * @param num_of_subtitle_tracks (out) Number of subtitle tracks.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_decoder_open_file(std::string file_path, int* num_of_audio_tracks, int* num_of_video_tracks, int* num_of_subtitle_tracks, int session);

/**
 * @brief Initialize a audio decoder.
 * @param num_of_audio_tracks (in) Number of audio tracks.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_audio_decoder_init(int num_of_audio_tracks, int session);

/**
 * @brief Set core mask for multi-threaded decoding.
 * @param frame_threading_cores (in) Core mask for frame decoding.
 * @param slice_threading_cores (in) Core mask for slice decoding.
 * @warning Thread dangerous (untested)
*/
void Util_video_decoder_set_enabled_cores(bool frame_threading_cores[4], bool slice_threading_cores[4]);

/**
 * @brief Initialize a video decoder.
 * @param low_resolution (in) When non-zero lower video resolution if video codec supports it (1 = 50%, 2 = 25%).
 * @param num_of_video_tracks (in) Number of video tracks.
 * @param num_of_threads (in) Number of threads.
 * @param thread_type (in) Thread type when not THREAD_TYPE_NONE, enable multi-threaded decoding if video codec supports it.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_video_decoder_init(int low_resolution, int num_of_video_tracks, int num_of_threads, Multi_thread_type thread_type, int session);

/**
 * @brief Initialize a mvd (hardware) video decoder.
 * You need to call Util_video_decoder_init() first, then call this function.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_mvd_video_decoder_init(int session);

/**
 * @brief Initialize a subtitle decoder.
 * @param num_of_subtitle_tracks (in) Number of subtitle tracks.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_subtitle_decoder_init(int num_of_subtitle_tracks, int session);

/**
 * @brief Get audio info.
 * Do nothing if audio decoder is not initialized.
 * @param audio_info (out) Pointer for audio info.
 * @param audio_index (in) Audio track index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_audio_decoder_get_info(Audio_info* audio_info, int audio_index, int session);

/**
 * @brief Get video info.
 * Do nothing if video decoder is not initialized.
 * @param video_info (out) Pointer for video info.
 * @param video_index (in) Video track index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_video_decoder_get_info(Video_info* video_info, int video_index, int session);

/**
 * @brief Get subtitle info.
 * Do nothing if subtitle decoder is not initialized.
 * @param subtitle_info (out) Pointer for subtitle info.
 * @param subtitle_index (in) Subtitle track index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_subtitle_decoder_get_info(Subtitle_info* subtitle_info, int subtitle_index, int session);

/**
 * @brief Clear cache packet.
 * Do nothing if file is not opened.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_decoder_clear_cache_packet(int session);

/**
 * @brief Get number of buffered packet.
 * Always return 0 if file is not opened.
 * @param session (in) Session number.
 * @return Number of buffered packet.
 * @warning Thread dangerous (untested)
*/
int Util_decoder_get_available_packet_num(int session);

/**
 * @brief Read packet.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_decoder_read_packet(int session);

/**
 * @brief Parse packet type.
 * @param type (out) Pointer for packet type.
 * @param packet_index (out) Pointer for packet index.
 * @param key_frame (out) Pointer for key frame (video packet only).
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_decoder_parse_packet(Packet_type* type, int* packet_index, bool* key_frame, int session);

/**
 * @brief Ready audio packet for decoding.
 * Call it after Util_decoder_parse_packet() returned PACKET_TYPE_AUDIO.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_decoder_ready_audio_packet(int packet_index, int session);

/**
 * @brief Ready video packet for decoding.
 * Call it after Util_decoder_parse_packet() returned PACKET_TYPE_VIDEO.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_decoder_ready_video_packet(int packet_index, int session);

/**
 * @brief Ready subtitle packet for decoding.
 * Call it after Util_decoder_parse_packet() returned PACKET_TYPE_SUBTITLE.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_decoder_ready_subtitle_packet(int packet_index, int session);

/**
 * @brief Skip audio packet.
 * Call it after Util_decoder_parse_packet() returned PACKET_TYPE_AUDIO.
 * Do nothing if file is not opened.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_decoder_skip_audio_packet(int packet_index, int session);

/**
 * @brief Skip video packet.
 * Call it after Util_decoder_parse_packet() returned PACKET_TYPE_VIDEO.
 * Do nothing if file is not opened.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_decoder_skip_video_packet(int packet_index, int session);

/**
 * @brief Skip subtitle packet.
 * Call it after Util_decoder_parse_packet() returned PACKET_TYPE_SUBTITLE.
 * Do nothing if file is not opened.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_decoder_skip_subtitle_packet(int packet_index, int session);

/**
 * @brief Set max raw buffer size.
 * Call it before calling Util_video_decoder_decode().
 * Do nothing if video decoder is not initialized.
 * @param max_num_of_buffer (in) Max num of buffer at least 3, at most DEF_DECODER_MAX_RAW_IMAGE (in frames, not in memory size).
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_video_decoder_set_raw_image_buffer_size(int max_num_of_buffer, int packet_index, int session);

/**
 * @brief Set max raw buffer size.
 * Call it before calling Util_mvd_video_decoder_decode().
 * Do nothing if mvd video decoder is not initialized.
 * @param max_num_of_buffer (in) Max num of buffer at least 3, at most DEF_DECODER_MAX_RAW_IMAGE (in frames, not in memory size).
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_mvd_video_decoder_set_raw_image_buffer_size(int max_num_of_buffer, int session);

/**
 * @brief Get max raw buffer size.
 * Always return 0 if video decoder is not initialized.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return Max number of buffer (in frames, not in memory size).
 * @warning Thread dangerous (untested)
*/
int Util_video_decoder_get_raw_image_buffer_size(int packet_index, int session);

/**
 * @brief Get max raw buffer size.
 * Always return 0 if mvd video decoder is not initialized.
 * @param session (in) Session number.
 * @return Max number of buffer (in frames, not in memory size).
 * @warning Thread dangerous (untested)
*/
int Util_mvd_video_decoder_get_raw_image_buffer_size(int session);

/**
 * @brief Decode audio.
 * Call it after calling Util_decoder_ready_audio_packet().
 * @param samples (out) Pointer for number of raw audio samples per channel.
 * @param raw_data (out) Pointer for raw audio data, the pointer will be allocated inside of function.
 * @param current_pos (out) Current audio pos (in ms).
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_audio_decoder_decode(int* samples, u8** raw_data, double* current_pos, int packet_index, int session);

/**
 * @brief Decode video.
 * Call it after calling Util_decoder_ready_video_packet().
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_video_decoder_decode(int packet_index, int session);

/**
 * @brief Decode video using mvd service.
 * Call it after calling Util_decoder_ready_video_packet().
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_mvd_video_decoder_decode(int session);

/**
 * @brief Decode subtitle.
 * Call it after calling Util_decoder_ready_subtitle_packet().
 * @param Subtitle_data (out) Pointer for subtitle data.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_subtitle_decoder_decode(Subtitle_data* subtitle_data, int packet_index, int session);

/**
 * @brief Clear raw buffer (created by Util_video_decoder_decode()).
 * Do nothing if video decoder is not initialized.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_video_decoder_clear_raw_image(int packet_index, int session);

/**
 * @brief Clear raw buffer (created by Util_mvd_video_decoder_decode()).
 * Do nothing if mvd video decoder is not initialized.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_mvd_video_decoder_clear_raw_image(int session);

/**
 * @brief Get buffered raw images (created by Util_video_decoder_decode()).
 * Always return 0 if video decoder is not initialized.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return Number of buffered images (in frames, not in memory size).
 * @warning Thread dangerous (untested)
*/
int Util_video_decoder_get_available_raw_image_num(int packet_index, int session);

/**
 * @brief Get buffered raw images (created by Util_mvd_video_decoder_decode()).
 * Always return 0 if mvd video decoder is not initialized.
 * @param session (in) Session number.
 * @return Number of buffered images (in frames, not in memory size).
 * @warning Thread dangerous (untested)
*/
int Util_mvd_video_decoder_get_available_raw_image_num(int session);

/**
 * @brief Get raw image.
 * Call it after calling Util_video_decoder_decode().
 * @param raw_data (out) Pointer for raw image, the pointer will be allocated inside of function.
 * @param current_pos (out) Current video pos (in ms).
 * @param width (in) Image width.
 * @param height (in) Image height.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_video_decoder_get_image(u8** raw_data, double* current_pos, int width, int height, int packet_index, int session);

/**
 * @brief Get raw image.
 * Call it after calling Util_mvd_video_decoder_decode().
 * @param raw_data (out) Pointer for raw image (RGB565LE), the pointer will be allocated inside of function.
 * @param current_pos (out) Current video pos (in ms).
 * @param width (in) Image width.
 * @param height (in) Image height.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_mvd_video_decoder_get_image(u8** raw_data, double* current_pos, int width, int height, int session);

/**
 * @brief Skip image.
 * Call it after calling Util_video_decoder_decode().
 * Do nothing if video decoder is not initialized.
 * @param current_pos (out) Current video pos (in ms).
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @note Thread safe
*/
void Util_video_decoder_skip_image(double* current_pos, int packet_index, int session);

/**
 * @brief Skip image.
 * Call it after calling Util_mvd_video_decoder_decode().
 * Do nothing if mvd video decoder is not initialized.
 * @param current_pos (out) Current video pos (in ms).
 * @param session (in) Session number.
 * @note Thread safe
*/
void Util_mvd_video_decoder_skip_image(double* current_pos, int session);

/**
 * @brief Seek file.
 * @param seek_pos (in) Target pos (in ms).
 * @param flag (in) Seek flag.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_decoder_seek(u64 seek_pos, Seek_flag flag, int session);

/**
 * @brief Uninitialize decoders and close the file.
 * Do nothing if file is not opend.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_decoder_close_file(int session);

#else

#define Util_decoder_open_file(...) Util_return_result_with_string(var_disabled_result)
#define Util_audio_decoder_init(...) Util_return_result_with_string(var_disabled_result)
#define Util_video_decoder_set_enabled_cores(...)
#define Util_video_decoder_init(...) Util_return_result_with_string(var_disabled_result)
#define Util_mvd_video_decoder_init(...) Util_return_result_with_string(var_disabled_result)
#define Util_subtitle_decoder_init(...) Util_return_result_with_string(var_disabled_result)
#define Util_audio_decoder_get_info(...)
#define Util_video_decoder_get_info(...)
#define Util_subtitle_decoder_get_info(...)
#define Util_decoder_clear_cache_packet(...)
#define Util_decoder_get_available_packet_num(...) Util_return_int(0)
#define Util_decoder_read_packet(...) Util_return_result_with_string(var_disabled_result)
#define Util_decoder_parse_packet(...) Util_return_result_with_string(var_disabled_result)
#define Util_decoder_ready_audio_packet(...) Util_return_result_with_string(var_disabled_result)
#define Util_decoder_ready_video_packet(...) Util_return_result_with_string(var_disabled_result)
#define Util_decoder_ready_subtitle_packet(...) Util_return_result_with_string(var_disabled_result)
#define Util_decoder_skip_audio_packet(...)
#define Util_decoder_skip_video_packet(...)
#define Util_decoder_skip_subtitle_packet(...)
#define Util_video_decoder_set_raw_image_buffer_size(...) Util_return_int(0)
#define Util_mvd_video_decoder_set_raw_image_buffer_size(...)
#define Util_video_decoder_get_raw_image_buffer_size(...) Util_return_int(0)
#define Util_mvd_video_decoder_get_raw_image_buffer_size(...) Util_return_int(0)
#define Util_audio_decoder_decode(...) Util_return_result_with_string(var_disabled_result)
#define Util_video_decoder_decode(...) Util_return_result_with_string(var_disabled_result)
#define Util_mvd_video_decoder_decode(...) Util_return_result_with_string(var_disabled_result)
#define Util_subtitle_decoder_decode(...) Util_return_result_with_string(var_disabled_result)
#define Util_video_decoder_clear_raw_image(...)
#define Util_mvd_video_decoder_clear_raw_image(...)
#define Util_video_decoder_get_available_raw_image_num(...) Util_return_int(0)
#define Util_mvd_video_decoder_get_available_raw_image_num(...) Util_return_int(0)
#define Util_video_decoder_get_image(...) Util_return_result_with_string(var_disabled_result)
#define Util_mvd_video_decoder_get_image(...) Util_return_result_with_string(var_disabled_result)
#define Util_video_decoder_skip_image(...)
#define Util_mvd_video_decoder_skip_image(...)
#define Util_decoder_seek(...) Util_return_result_with_string(var_disabled_result)
#define Util_decoder_close_file(...)

#endif

#if DEF_ENABLE_IMAGE_DECODER_API

/**
 * @brief Decode image file.
 * @param file_path (in) File path.
 * @param raw_data (out) Pointer for raw image (RGB888BE or RGBA8888BE), the pointer will be allocated inside of function.
 * @param width (out) Image width.
 * @param height (out) Image height.
 * @param format (out) Image format (PIXEL_FORMAT_RGBA8888, PIXEL_FORMAT_RGB888, PIXEL_FORMAT_GRAYALPHA88 or PIXEL_FORMAT_GRAY8).
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_image_decoder_decode(std::string file_name, u8** raw_data, int* width, int* height, Pixel_format* format);

/**
 * @brief Decode image file.
 * @param compressed_data (in) Compressed data (.png, .jpg etc...).
 * @param compressed_buffer_size (in) Compressed data size.
 * @param raw_data (out) Pointer for raw image (RGB888BE or RGBA8888BE), the pointer will be allocated inside of function.
 * @param width (out) Image width.
 * @param height (out) Image height.
 * @param format (out) Image format (PIXEL_FORMAT_RGBA8888, PIXEL_FORMAT_RGB888, PIXEL_FORMAT_GRAYALPHA88 or PIXEL_FORMAT_GRAY8).
 * @return On success DEF_SUCCESS,
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_image_decoder_decode(u8* compressed_data, int compressed_buffer_size, u8** raw_data, int* width, int* height, Pixel_format* format);

#else

#define Util_image_decoder_decode(...) Util_return_result_with_string(var_disabled_result)

#endif

#endif
