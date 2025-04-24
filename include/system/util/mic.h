#if !defined(DEF_MIC_H)
#define DEF_MIC_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/mic_types.h"

#if defined(DEF_MIC_API_ENABLE)

/**
 * @brief Initialize a mic.
 * @param buffer_size (in) Internal mic buffer size.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_mic_init(uint32_t buffer_size);

/**
 * @brief Start recording.
 * @param sample_rate_mode (in) Audio sample rate.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_mic_start_recording(Mic_sample_rate sample_rate_mode);

/**
 * @brief Stop recording.
 * Do nothing if mic API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_mic_stop_recording(void);

/**
 * @brief Check whether recording is in progress.
 * Always return false if mic API is not initialized.
 * @return True if recording is in progress, otherwise false.
 * @warning Thread dangerous (untested).
*/
bool Util_mic_is_recording(void);

/**
 * @brief Query remaining (approximately) time.
 * Call Util_mic_get_audio_data() at least before it gets less than 300,
 * otherwise audio data might be overwritten.
 * Always return 0 if mic API is not initialized.
 * @return Remaining time (in ms).
 * @warning Thread dangerous (untested).
*/
uint32_t Util_mic_query_remaining_buffer_time(void);

/**
 * @brief Get audio data.
 * @param raw_data (out) Pointer for raw audio data (PCM_S16LE), the pointer will be allocated inside of function.
 * @param size (out) Audio data size.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_mic_get_audio_data(uint8_t** raw_data, uint32_t* size);

/**
 * @brief Uninitialize a mic.
 * Do nothing if mic API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_mic_exit(void);

#else

#define Util_mic_init(...) DEF_ERR_DISABLED
#define Util_mic_start_recording(...) DEF_ERR_DISABLED
#define Util_mic_stop_recording()
#define Util_mic_is_recording() false
#define Util_mic_query_remaining_buffer_time() 0
#define Util_mic_get_audio_data(...) DEF_ERR_DISABLED
#define Util_mic_exit()

#endif //defined(DEF_MIC_API_ENABLE)

#endif //!defined(DEF_MIC_H)
