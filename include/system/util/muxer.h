#if !defined(DEF_MUXER_H)
#define DEF_MUXER_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/muxer_types.h"

#if DEF_MUXER_API_ENABLE

/**
 * @brief Open the audio file.
 * @param file_name (in) File path.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_muxer_open_audio_file(const char* file_path, uint8_t session);

/**
 * @brief Open the video file.
 * @param file_name (in) File path.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_muxer_open_video_file(const char* file_path, uint8_t session);

/**
 * @brief Mux file.
 * @param file_name (in) Output file path.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_muxer_mux(const char* file_path, uint8_t session);

/**
 * @brief Close the file.
 * Do nothing if file is not opend.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_muxer_close(uint8_t session);

#else

#define Util_muxer_open_audio_file(...) DEF_ERR_DISABLED
#define Util_muxer_open_video_file(...) DEF_ERR_DISABLED
#define Util_muxer_mux(...) DEF_ERR_DISABLED
#define Util_muxer_close(...)

#endif //DEF_MUXER_API_ENABLE

#endif //!defined(DEF_MUXER_H)
