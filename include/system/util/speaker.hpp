#pragma once

/**
 * @brief Initialize a speaker.
 * Run dsp1(https://github.com/zoogie/DSP1/releases) first if 0xd880A7FA is returned.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_speaker_init(void);

/**
 * @brief Set audio info.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @param music_ch (in) Audio ch (1 or 2).
 * @param sample_rate (in) Audio sample rate (in Hz).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_speaker_set_audio_info(int play_ch, int music_ch, int sample_rate);

/**
 * @brief Add audio buffer.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @param music_ch (in) Audio ch (1 or 2).
 * @param buffer (in) Pointer for raw audio data (PCM_S16LE).
 * @param size (in) Audio data size (in byte).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_speaker_add_buffer(int play_ch, int music_ch, u8* buffer, int size);

/**
 * @brief Clear audio buffer.
 * Do nothing if speaker api is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @warning Thread dangerous (untested)
*/
void Util_speaker_clear_buffer(int play_ch);

/**
 * @brief Pause audio playback.
 * Do nothing if speaker api is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @warning Thread dangerous (untested)
*/
void Util_speaker_pause(int play_ch);

/**
 * @brief Resume audio playback.
 * Do nothing if speaker api is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @warning Thread dangerous (untested)
*/
void Util_speaker_resume(int play_ch);

/**
 * @brief Check whether audio playback is paused.
 * Always return false if speaker api is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @return True if play_ch is paused, otherwise false.
 * @warning Thread dangerous (untested)
*/
bool Util_speaker_is_paused(int play_ch);

/**
 * @brief Check whether audio is playing.
 * Always return false if speaker api is not initialized.
 * @param play_ch (in) Internal speaker ch (0 ~ 23).
 * @return True if play_ch is not paused(playing), otherwise false.
 * @warning Thread dangerous (untested)
*/
bool Util_speaker_is_playing(int play_ch);

/**
 * @brief Uninitialize a speaker.
 * Do nothing if speaker api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_speaker_exit(void);
