#ifndef FAKE_PTHREAD_HPP
#define FAKE_PTHREAD_HPP

/**
 * @brief Set enabled cores for creating thread.
 * Do nothing if enabled_core are all false.
 * @param enabled_core (in) Enabled cores.
 * @warning Thread dangerous (untested)
*/
extern "C" void Util_fake_pthread_set_enabled_core(bool enabled_core[4]);

#endif
