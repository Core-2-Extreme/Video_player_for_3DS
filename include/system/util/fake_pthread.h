#if !defined(DEF_FAKE_PTHREAD_H)
#define DEF_FAKE_PTHREAD_H
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize fake pthread API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_fake_pthread_init(void);

/**
 * @brief Uninitialize fake pthread API.
 * Do nothing if fake pthread API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_fake_pthread_exit(void);

/**
 * @brief Set enabled cores for creating thread.
 * Do nothing if enabled_core are all false.
 * @param enabled_core (in) Enabled cores.
 * @warning Thread dangerous (untested).
*/
void Util_fake_pthread_set_enabled_core(const bool enabled_core[4]);

#endif //!defined(DEF_FAKE_PTHREAD_H)
