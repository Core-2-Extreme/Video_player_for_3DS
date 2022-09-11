#pragma once

/**
 * @brief Set screen brightness.
 * @param top_screen (in) When true, top screen's brightness will be set.
 * @param bottom_screen (in) When true, bottom screen's brightness will be set.
 * @param brightness (in) brightness level 0~200.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cset_set_screen_brightness(bool top_screen, bool bottom_screen, int brightness);

/**
 * @brief Set wifi state.
 * @param wifi_state (in) When true, wifi will be turned on.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cset_set_wifi_state(bool wifi_state);

/**
 * @brief Set screen state(ON or OFF).
 * @param top_screen (in) When true, top screen's state will be set.
 * @param bottom_screen (in) When true, bottom screen's state will be set.
 * @param state (in) When true, screen will be turned on.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cset_set_screen_state(bool top_screen, bool bottom_screen, bool state);

/**
 * @brief Sleep the system.
 * If sleep is not allowed and parameter is valid, system won't sleep but this function still returns DEF_SUCCESS.
 * @param wakeup_event (in) Wakeup event, combination of DEF_CSET_WAKE_UP_*.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested)
*/
Result_with_string Util_cset_sleep_system(int wake_up_event);
