#if !defined(DEF_WATCH_H)
#define DEF_WATCH_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/watch_types.h"

/**
 * @brief Initialize watch API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested)
*/
uint32_t Util_watch_init(void);

/**
 * @brief Uninitialize watch API.
 * Do nothing if watch api is not initialized.
 * @warning Thread dangerous (untested)
*/
void Util_watch_exit(void);

/**
 * @brief Get watch usage.
 * @param handle (in) Watch handle to check.
 * Always return 0 if watch api is not initialized.
 * @return Number of watched variables.
 * @note Thread safe
*/
uint32_t Util_watch_get_usage(Watch_handle handle);

/**
 * @brief Get total watch usage.
 * Always return 0 if watch api is not initialized.
 * @return Number of total watched variables.
 * @note Thread safe
*/
uint32_t Util_watch_get_total_usage(void);

/**
 * @brief Add a variable to watch list.
 * Do nothing if watch api is not initialized.
 * @param handle (in) Watch handle to link with.
 * @param variable (in) Pointer for variable to add to watch list.
 * @param length (in) Data length to watch.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe
*/
uint32_t Util_watch_add(Watch_handle handle, const void* variable, uint32_t length);

/**
 * @brief Remove a variable frin watch list.
 * Do nothing if watch api is not initialized.
 * @param handle (in) Watch handle to search.
 * @param variable (in) Pointer for variable to remove from watch list.
 * @note Thread safe
*/
void Util_watch_remove(Watch_handle handle, const void* variable);

/**
 * @brief Check if watched values were changed since last call of this function.
 * Always return false if watch api is not initialized.
 * @param handles (in) Watch handle to check (bit field).
 * @return True if values were changed otherwise false.
 * @note Thread safe
*/
bool Util_watch_is_changed(Watch_handle_bit handles);

#endif //!defined(DEF_WATCH_H)
