#if !defined(DEF_DECODER_H)
#define DEF_DECODER_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/decoder_types.h"
#include "system/util/media_types.h"

#if DEF_DECODER_VIDEO_AUDIO_API_ENABLE

/**
 * @brief Open media file.
 * @param path (in) File path.
 * @param num_of_audio_tracks (out) Number of audio tracks.
 * @param num_of_video_tracks (out) Number of video tracks.
 * @param num_of_subtitle_tracks (out) Number of subtitle tracks.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_open_file(const char* path, uint8_t* num_of_audio_tracks, uint8_t* num_of_video_tracks, uint8_t* num_of_subtitle_tracks, uint8_t session);

/**
 * @brief Initialize a audio decoder.
 * @param num_of_audio_tracks (in) Number of audio tracks.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_audio_init(uint8_t num_of_audio_tracks, uint8_t session);

/**
 * @brief Set core mask for multi-threaded decoding.
 * @param frame_threading_cores (in) Core mask for frame decoding.
 * @param slice_threading_cores (in) Core mask for slice decoding.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_video_set_enabled_cores(const bool frame_threading_cores[4], const bool slice_threading_cores[4]);

/**
 * @brief Initialize a video decoder.
 * @param low_resolution (in) When non-zero lower video resolution if video codec supports it (1 = 50%, 2 = 25%).
 * @param num_of_video_tracks (in) Number of video tracks.
 * @param num_of_threads (in) Number of threads.
 * @param thread_type (in) Thread type when not MEDIA_THREAD_TYPE_NONE, enable multi-threaded decoding if video codec supports it.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_video_init(uint8_t low_resolution, uint8_t num_of_video_tracks, uint8_t num_of_threads, Media_thread_type thread_type, uint8_t session);

/**
 * @brief Initialize a MVD (hardware) video decoder.
 * You need to call Util_decoder_video_init() first, then call this function.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_mvd_init(uint8_t session);

/**
 * @brief Initialize a subtitle decoder.
 * @param num_of_subtitle_tracks (in) Number of subtitle tracks.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_subtitle_init(uint8_t num_of_subtitle_tracks, uint8_t session);

/**
 * @brief Get audio info.
 * Do nothing if audio decoder is not initialized.
 * @param audio_info (out) Pointer for audio info.
 * @param audio_index (in) Audio track index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_audio_get_info(Media_a_info* audio_info, uint8_t audio_index, uint8_t session);

/**
 * @brief Get video info.
 * Do nothing if video decoder is not initialized.
 * @param video_info (out) Pointer for video info.
 * @param video_index (in) Video track index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_video_get_info(Media_v_info* video_info, uint8_t video_index, uint8_t session);

/**
 * @brief Get subtitle info.
 * Do nothing if subtitle decoder is not initialized.
 * @param subtitle_info (out) Pointer for subtitle info.
 * @param subtitle_index (in) Subtitle track index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_subtitle_get_info(Media_s_info* subtitle_info, uint8_t subtitle_index, uint8_t session);

/**
 * @brief Clear cache packet.
 * Do nothing if file is not opened.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_clear_cache_packet(uint8_t session);

/**
 * @brief Get number of buffered packet.
 * Always return 0 if file is not opened.
 * @param session (in) Session number.
 * @return Number of buffered packet.
 * @warning Thread dangerous (untested).
*/
uint16_t Util_decoder_get_available_packet_num(uint8_t session);

/**
 * @brief Read packet.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_decoder_read_packet(uint8_t session);

/**
 * @brief Parse packet type.
 * @param type (out) Pointer for packet type.
 * @param packet_index (out) Pointer for packet index.
 * @param key_frame (out) Pointer for keyframe (video packet only).
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_decoder_parse_packet(Media_packet_type* type, uint8_t* packet_index, bool* key_frame, uint8_t session);

/**
 * @brief Ready audio packet for decoding.
 * Call it after Util_decoder_parse_packet() returned MEDIA_PACKET_TYPE_AUDIO.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_ready_audio_packet(uint8_t packet_index, uint8_t session);

/**
 * @brief Ready video packet for decoding.
 * Call it after Util_decoder_parse_packet() returned MEDIA_PACKET_TYPE_VIDEO.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_ready_video_packet(int8_t packet_index, int8_t session);

/**
 * @brief Ready subtitle packet for decoding.
 * Call it after Util_decoder_parse_packet() returned MEDIA_PACKET_TYPE_SUBTITLE.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_ready_subtitle_packet(uint8_t packet_index, uint8_t session);

/**
 * @brief Skip audio packet.
 * Call it after Util_decoder_parse_packet() returned MEDIA_PACKET_TYPE_AUDIO.
 * Do nothing if file is not opened.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_skip_audio_packet(uint8_t packet_index, uint8_t session);

/**
 * @brief Skip video packet.
 * Call it after Util_decoder_parse_packet() returned MEDIA_PACKET_TYPE_VIDEO.
 * Do nothing if file is not opened.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_skip_video_packet(uint8_t packet_index, uint8_t session);

/**
 * @brief Skip subtitle packet.
 * Call it after Util_decoder_parse_packet() returned MEDIA_PACKET_TYPE_SUBTITLE.
 * Do nothing if file is not opened.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_skip_subtitle_packet(uint8_t packet_index, uint8_t session);

/**
 * @brief Set max raw buffer size.
 * Call it before calling Util_decoder_video_decode().
 * Do nothing if video decoder is not initialized.
 * @param max_num_of_buffer (in) Max num of buffer at least 3, at most DEF_DECODER_MAX_RAW_IMAGE (in frames, not in memory size).
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_video_set_raw_image_buffer_size(uint32_t max_num_of_buffer, uint8_t packet_index, uint8_t session);

/**
 * @brief Set max raw buffer size.
 * Call it before calling Util_decoder_mvd_decode().
 * Do nothing if MVD video decoder is not initialized.
 * @param max_num_of_buffer (in) Max num of buffer at least 3, at most DEF_DECODER_MAX_RAW_IMAGE (in frames, not in memory size).
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_mvd_set_raw_image_buffer_size(uint32_t max_num_of_buffer, uint8_t session);

/**
 * @brief Get max raw buffer size.
 * Always return 0 if video decoder is not initialized.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return Max number of buffer (in frames, not in memory size).
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_video_get_raw_image_buffer_size(uint8_t packet_index, uint8_t session);

/**
 * @brief Get max raw buffer size.
 * Always return 0 if MVD video decoder is not initialized.
 * @param session (in) Session number.
 * @return Max number of buffer (in frames, not in memory size).
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_mvd_get_raw_image_buffer_size(uint8_t session);

/**
 * @brief Decode audio.
 * Call it after calling Util_decoder_ready_audio_packet().
 * @param samples (out) Pointer for number of raw audio samples per channel.
 * @param raw_data (out) Pointer for raw audio data, the pointer will be allocated inside of function.
 * @param current_pos (out) Current audio pos (in ms).
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_audio_decode(uint32_t* samples, uint8_t** raw_data, double* current_pos, uint8_t packet_index, uint8_t session);

/**
 * @brief Decode video.
 * Call it after calling Util_decoder_ready_video_packet().
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_decoder_video_decode(uint8_t packet_index, uint8_t session);

/**
 * @brief Decode video using MVD service.
 * Call it after calling Util_decoder_ready_video_packet().
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @note Thread safe.
*/
uint32_t Util_decoder_mvd_decode(uint8_t session);

/**
 * @brief Decode subtitle.
 * Call it after calling Util_decoder_ready_subtitle_packet().
 * @param subtitle_data (out) Pointer for subtitle data.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_subtitle_decode(Media_s_data* subtitle_data, uint8_t packet_index, uint8_t session);

/**
 * @brief Clear raw buffer (created by Util_decoder_video_decode()).
 * Do nothing if video decoder is not initialized.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_video_clear_raw_image(uint8_t packet_index, uint8_t session);

/**
 * @brief Clear raw buffer (created by Util_decoder_mvd_decode()).
 * Do nothing if MVD video decoder is not initialized.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_mvd_clear_raw_image(uint8_t session);

/**
 * @brief Get buffered raw images (created by Util_decoder_video_decode()).
 * Always return 0 if video decoder is not initialized.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return Number of buffered images (in frames, not in memory size).
 * @warning Thread dangerous (untested).
*/
uint16_t Util_decoder_video_get_available_raw_image_num(uint8_t packet_index, uint8_t session);

/**
 * @brief Get buffered raw images (created by Util_decoder_mvd_decode()).
 * Always return 0 if MVD video decoder is not initialized.
 * @param session (in) Session number.
 * @return Number of buffered images (in frames, not in memory size).
 * @warning Thread dangerous (untested).
*/
uint16_t Util_decoder_mvd_get_available_raw_image_num(uint8_t session);

/**
 * @brief Get raw image.
 * Call it after calling Util_decoder_video_decode().
 * @param raw_data (out) Pointer for raw image, the pointer will be allocated inside of function.
 * @param current_pos (out) Current video pos (in ms).
 * @param width (in) Image width.
 * @param height (in) Image height.
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_decoder_video_get_image(uint8_t** raw_data, double* current_pos, uint32_t width, uint32_t height, uint8_t packet_index, uint8_t session);

/**
 * @brief Get raw image.
 * Call it after calling Util_decoder_mvd_decode().
 * @param raw_data (out) Pointer for raw image (RGB565LE), the pointer will be allocated inside of function.
 * @param current_pos (out) Current video pos (in ms).
 * @param width (in) Image width.
 * @param height (in) Image height.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_decoder_mvd_get_image(uint8_t** raw_data, double* current_pos, uint32_t width, uint32_t height, uint8_t session);

/**
 * @brief Skip image.
 * Call it after calling Util_decoder_video_decode().
 * Do nothing if video decoder is not initialized.
 * @param current_pos (out) Current video pos (in ms).
 * @param packet_index (in) Packet index.
 * @param session (in) Session number.
 * @note Thread safe.
*/
void Util_decoder_video_skip_image(double* current_pos, uint8_t packet_index, uint8_t session);

/**
 * @brief Skip image.
 * Call it after calling Util_decoder_mvd_decode().
 * Do nothing if MVD video decoder is not initialized.
 * @param current_pos (out) Current video pos (in ms).
 * @param session (in) Session number.
 * @note Thread safe.
*/
void Util_decoder_mvd_skip_image(double* current_pos, uint8_t session);

/**
 * @brief Seek file.
 * @param seek_pos (in) Target pos (in ms).
 * @param flag (in) Seek flag.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_seek(uint64_t seek_pos, Media_seek_flag flag, uint8_t session);

/**
 * @brief Uninitialize decoders and close the file.
 * Do nothing if file is not opened.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested).
*/
void Util_decoder_close_file(uint8_t session);

#else

#define Util_decoder_open_file(...) DEF_ERR_DISABLED
#define Util_decoder_audio_init(...) DEF_ERR_DISABLED
#define Util_decoder_video_set_enabled_cores(...)
#define Util_decoder_video_init(...) DEF_ERR_DISABLED
#define Util_decoder_mvd_init(...) DEF_ERR_DISABLED
#define Util_decoder_subtitle_init(...) DEF_ERR_DISABLED
#define Util_decoder_audio_get_info(...)
#define Util_decoder_video_get_info(...)
#define Util_decoder_subtitle_get_info(...)
#define Util_decoder_clear_cache_packet(...)
#define Util_decoder_get_available_packet_num(...) 0
#define Util_decoder_read_packet(...) DEF_ERR_DISABLED
#define Util_decoder_parse_packet(...) DEF_ERR_DISABLED
#define Util_decoder_ready_audio_packet(...) DEF_ERR_DISABLED
#define Util_decoder_ready_video_packet(...) DEF_ERR_DISABLED
#define Util_decoder_ready_subtitle_packet(...) DEF_ERR_DISABLED
#define Util_decoder_skip_audio_packet(...)
#define Util_decoder_skip_video_packet(...)
#define Util_decoder_skip_subtitle_packet(...)
#define Util_decoder_video_set_raw_image_buffer_size(...)
#define Util_decoder_mvd_set_raw_image_buffer_size(...)
#define Util_decoder_video_get_raw_image_buffer_size(...) 0
#define Util_decoder_mvd_get_raw_image_buffer_size(...) 0
#define Util_decoder_audio_decode(...) DEF_ERR_DISABLED
#define Util_decoder_video_decode(...) DEF_ERR_DISABLED
#define Util_decoder_mvd_decode(...) DEF_ERR_DISABLED
#define Util_decoder_subtitle_decode(...) DEF_ERR_DISABLED
#define Util_decoder_video_clear_raw_image(...)
#define Util_decoder_mvd_clear_raw_image(...)
#define Util_decoder_video_get_available_raw_image_num(...) 0
#define Util_decoder_mvd_get_available_raw_image_num(...) 0
#define Util_decoder_video_get_image(...) DEF_ERR_DISABLED
#define Util_decoder_mvd_get_image(...) DEF_ERR_DISABLED
#define Util_decoder_video_skip_image(...)
#define Util_decoder_mvd_skip_image(...)
#define Util_decoder_seek(...) DEF_ERR_DISABLED
#define Util_decoder_close_file(...)

#endif //DEF_DECODER_VIDEO_AUDIO_API_ENABLE

#if DEF_DECODER_IMAGE_API_ENABLE

/**
 * @brief Decode image file.
 * @param path (in) File path.
 * @param raw_data (out) Pointer for raw image (RGB888BE or RGBA8888BE), the pointer will be allocated inside of function.
 * @param width (out) Image width.
 * @param height (out) Image height.
 * @param format (out) Image format (RAW_PIXEL_RGBA8888, RAW_PIXEL_RGB888, RAW_PIXEL_GRAYALPHA88 or RAW_PIXEL_GRAY8).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_image_decode(const char* path, uint8_t** raw_data, uint32_t* width, uint32_t* height, Raw_pixel* format);

/**
 * @brief Decode image data.
 * @param compressed_data (in) Compressed data (.png, .jpg etc...).
 * @param compressed_buffer_size (in) Compressed data size.
 * @param raw_data (out) Pointer for raw image (RGB888BE or RGBA8888BE), the pointer will be allocated inside of function.
 * @param width (out) Image width.
 * @param height (out) Image height.
 * @param format (out) Image format (RAW_PIXEL_RGBA8888, RAW_PIXEL_RGB888, RAW_PIXEL_GRAYALPHA88 or RAW_PIXEL_GRAY8).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_decoder_image_decode_data(const uint8_t* compressed_data, uint32_t compressed_buffer_size, uint8_t** raw_data, uint32_t* width, uint32_t* height, Raw_pixel* format);

#else

#define Util_decoder_image_decode(...) DEF_ERR_DISABLED
#define Util_decoder_image_decode_data(...) DEF_ERR_DISABLED

#endif //DEF_DECODER_IMAGE_API_ENABLE

#endif //!defined(DEF_DECODER_H)
