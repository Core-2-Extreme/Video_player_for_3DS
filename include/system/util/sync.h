#if !defined(DEF_SYNC_H)
#define DEF_SYNC_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/sync_types.h"

/**
 * @brief Create the sync object.
 * @param sync_object (out) Pointer for the sync object.
 * @param type (in) Sync object type to create.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_create(Sync_data* sync_object, Sync_type type);

/**
 * @brief Lock sync object.
 * @param sync_object (in/out) Pointer for the sync object.
 * @param wait_us (in) Wait time in us.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_lock(Sync_data* sync_object, uint64_t wait_us);

/**
 * @brief Unlock sync object.
 * @param sync_object (in/out) Pointer for the sync object.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_sync_unlock(Sync_data* sync_object);

/**
 * @brief Delete the sync object.
 * Do nothing if sync object is not initialized.
 * @param sync_object (in/out) Pointer for the sync object.
 * @note Thread safe.
*/
void Util_sync_destroy(Sync_data* sync_object);

#endif //!defined(DEF_SYNC_H)
