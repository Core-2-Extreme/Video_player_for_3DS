#if !defined(DEF_SPEAKER_H)
#define DEF_SPEAKER_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/speaker_types.h"

#if DEF_SPEAKER_API_ENABLE

/**
 * @brief Initialize a speaker.
 * Run dsp1(https://github.com/zoogie/DSP1/releases) first if 0xD880A7FA is returned.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_speaker_init(void);

/**
 * @brief Set audio info.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @param music_ch (in) Number of audio chs (1 or 2).
 * @param sample_rate (in) Audio sample rate (in Hz).
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_speaker_set_audio_info(uint8_t play_ch, uint8_t music_ch, uint32_t sample_rate);

/**
 * @brief Add audio buffer.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @param buffer (in) Pointer for raw audio data (PCM_S16LE).
 * @param size (in) Audio data size in bytes.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_speaker_add_buffer(uint8_t play_ch, const uint8_t* buffer, uint32_t size);

/**
 * @brief Get number of queued audio buffers.
 * Always return 0 if speaker API is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @return Number of audio buffers (not in time or bytes).
 * @warning Thread dangerous (untested).
*/
uint32_t Util_speaker_get_available_buffer_num(uint8_t play_ch);

/**
 * @brief Get audio buffer size.
 * Always return 0 if speaker API is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @return Audio buffer size in bytes.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_speaker_get_available_buffer_size(uint8_t play_ch);

/**
 * @brief Clear audio buffer.
 * Do nothing if speaker API is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @warning Thread dangerous (untested).
*/
void Util_speaker_clear_buffer(uint8_t play_ch);

/**
 * @brief Pause audio playback.
 * Do nothing if speaker API is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @warning Thread dangerous (untested).
*/
void Util_speaker_pause(uint8_t play_ch);

/**
 * @brief Resume audio playback.
 * Do nothing if speaker API is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @warning Thread dangerous (untested).
*/
void Util_speaker_resume(uint8_t play_ch);

/**
 * @brief Check whether audio playback is paused.
 * Always return false if speaker API is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @return True if play_ch is paused, otherwise false.
 * @warning Thread dangerous (untested).
*/
bool Util_speaker_is_paused(uint8_t play_ch);

/**
 * @brief Check whether audio is playing.
 * Always return false if speaker API is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @return True if play_ch is playing, otherwise false.
 * @warning Thread dangerous (untested).
*/
bool Util_speaker_is_playing(uint8_t play_ch);

/**
 * @brief Uninitialize a speaker.
 * Do nothing if speaker API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_speaker_exit(void);

#else

#define Util_speaker_init() DEF_ERR_DISABLED
#define Util_speaker_set_audio_info(...) DEF_ERR_DISABLED
#define Util_speaker_add_buffer(...) DEF_ERR_DISABLED
#define Util_speaker_get_available_buffer_num(...) 0
#define Util_speaker_get_available_buffer_size(...) 0
#define Util_speaker_clear_buffer(...)
#define Util_speaker_pause(...)
#define Util_speaker_resume(...)
#define Util_speaker_is_paused(...) false
#define Util_speaker_is_playing(...) false
#define Util_speaker_exit()

#endif //DEF_SPEAKER_API_ENABLE

#endif //!defined(DEF_SPEAKER_H)
