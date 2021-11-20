#pragma once

/**
 * @brief Initialize a mic.
 * @param buffer_size (in) Internal mic buffer size.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_mic_init(int buffer_size);

/**
 * @brief Start recording.
 * Available sample rate are : 32728, 16364, 10909 and 8182
 * @param sample_rate (in) Audio sample rate (in Hz).
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_mic_start_recording(int sample_rate);

/**
 * @brief Stop recording.
 * Do nothing if mic api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_mic_stop_recording(void);

/**
 * @brief Check whether recording in progress.
 * Always return false if mic api is not initialized.
 * @return True if recording in progress, otherwise false.
 * @warning Thread dangerous (untested)
*/
bool Util_mic_is_recording(void);

/**
 * @brief Query remaining (approximately) time.
 * Call Util_mic_get_audio_data() at least before it gets less than 300,
 * otherwise audio data might be overwritten.
 * Always return false 0 mic api is not initialized.
 * @return Remaining time (in ms).
 * @warning Thread dangerous (untested)
*/
int Util_mic_query_remaining_buffer_time(void);

/**
 * @brief Get audio data.
 * @param raw_data (out) Pointer for raw audio data (PCM_S16LE), the pointer will be allocated inside of function.
 * @param size (out) Audio data size.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_mic_get_audio_data(u8** raw_data, int* size);

/**
 * @brief Uninitialize a mic.
 * Do nothing if mic api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_mic_exit(void);
