#if !defined(DEF_QUEUE_H)
#define DEF_QUEUE_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/queue_types.h"

/**
 * @brief Create the queue.
 * @param queue (out) Pointer for the queue.
 * @param max_items (in) Max number of items this queue can hold.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_queue_create(Queue_data* queue, uint32_t max_items);

/**
 * @brief Add an event to the queue. Data is passed by reference not by copy.
 * @param queue (in/out) Pointer for the queue.
 * @param event_id (in) User defined event ID.
 * @param data (in) Pointer for user defined data, can be NULL.
 * @param wait_us (in) Wait time in us when queue is full.
 * @param option (in) Queue options.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_queue_add(Queue_data* queue, uint32_t event_id, void* data, int64_t wait_us, Queue_option option);

/**
 * @brief Get an event from the queue. Data is passed by reference not by copy.
 * @param queue (in/out) Pointer for the queue.
 * @param event_id (out) Event ID.
 * @param data (out) Pointer for data, can be NULL.
 * @param wait_us (in) Wait time in us when queue is empty.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Util_queue_get(Queue_data* queue, uint32_t* event_id, void** data, int64_t wait_us);

/**
 * @brief Check if the specified event exists in the queue.
 * Always return false if queue is not initialized.
 * @param queue (in) Pointer for the queue.
 * @param event_id (in) Event ID to check.
 * @note Thread safe.
*/
bool Util_queue_check_event_exist(const Queue_data* queue, uint32_t event_id);

/**
 * @brief Check how many spaces left in the queue.
 * Always return 0 if queue is not initialized.
 * @param queue (in) Pointer for the queue.
 * @note Thread safe.
*/
uint32_t Util_queue_get_free_space(const Queue_data* queue);

/**
 * @brief Delete the queue and if any, free all data in the queue.
 * Do nothing if queue is not initialized.
 * @param queue (in/out) Pointer for the queue.
 * @note Thread safe.
*/
void Util_queue_delete(Queue_data* queue);

#endif //!defined(DEF_QUEUE_H)
