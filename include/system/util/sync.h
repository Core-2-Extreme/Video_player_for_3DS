#if !defined(DEF_SYNC_H)
#define DEF_SYNC_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/sync_types.h"

/**
 * @brief Initialize sync API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_sync_init(void);

/**
 * @brief Uninitialize sync API.
 * Do nothing if sync API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_sync_exit(void);

/**
 * @brief Create the sync object.
 * @param sync_object (out) Pointer for the sync object.
 * @param type (in) Sync object type to create.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_create(Sync_data* sync_object, Sync_type type);

/**
 * @brief Lock lock object.
 * @param lock_object (in/out) Pointer for the lock object.
 * @param wait_us (in) Wait time in us.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_lock(Sync_data* lock_object, uint64_t wait_us);

/**
 * @brief Unlock lock object.
 * @param lock_object (in/out) Pointer for the lock object.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_unlock(Sync_data* lock_object);

/**
 * @brief Wait on condition object.
 * @param cond_object (in/out) Pointer for the cond object.
 * @param lock_object (in/out) Pointer for the lock object.
 * @param wait_us (in) Wait time in us.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_cond_wait(Sync_data* cond_object, Sync_data* lock_object, uint64_t wait_us);

/**
 * @brief Signal condition object.
 * @param cond_object (in/out) Pointer for the cond object.
 * @param is_broadcast (in) Whether wake up all waiting threads.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_cond_signal(Sync_data* cond_object, bool is_broadcast);

/**
 * @brief Delete the sync object.
 * Do nothing if sync object is not initialized.
 * @param sync_object (in/out) Pointer for the sync object.
 * @note Thread safe.
 * @warning Reference count must be 0 (i.e. other threads don't use it) to destroy the object,
 * otherwise this function will wait forever unless reference count gets 0.
*/
void Util_sync_destroy(Sync_data* sync_object);

#endif //!defined(DEF_SYNC_H)
