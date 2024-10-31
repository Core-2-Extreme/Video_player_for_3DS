#if !defined(DEF_FAKE_PTHREAD_H)
#define DEF_FAKE_PTHREAD_H
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Set enabled cores for creating thread.
 * Do nothing if enabled_core are all false.
 * @param enabled_core (in) Enabled cores.
 * @warning Thread dangerous (untested)
*/
void Util_fake_pthread_set_enabled_core(const bool enabled_core[4]);

#endif //!defined(DEF_FAKE_PTHREAD_H)
