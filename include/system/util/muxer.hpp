#pragma once

/**
 * @brief Open the audio file.
 * @param file_name (in) File path.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_muxer_open_audio_file(std::string file_path, int session);

/**
 * @brief Open the video file.
 * @param file_name (in) File path.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * or DEF_ERR_OTHER.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_muxer_open_video_file(std::string file_path, int session);

/**
 * @brief Mux file.
 * @param file_name (in) Output file path.
 * @param session (in) Session number.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_muxer_mux(std::string file_path, int session);

/**
 * @brief Close the file.
 * Do nothing if file is not opend.
 * @param session (in) Session number.
 * @warning Thread dangerous (untested)
*/
void Util_muxer_close(int session);
