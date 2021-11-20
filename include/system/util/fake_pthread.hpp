#pragma once

/**
 * @brief Set enabled cores for creating thread.
 * Do nothing if enabled_core are all false.
 * @param enabled_core (in) Enabled cores.
 * @warning Thread dangerous (untested)
*/
void Util_fake_pthread_set_enabled_core(bool enabled_core[4]);
