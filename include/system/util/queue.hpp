#ifndef QUEUE_HPP
#define QUEUE_HPP

#include "system/types.hpp"

/**
 * @brief Create the queue.
 * @param queue (out) Pointer for the queue.
 * @param max_items (in) Max number of items this queue can hold.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_queue_create(Queue* queue, int max_items);

/**
 * @brief Add an event to the queue. Data is passed by reference not by copy.
 * @param queue (in) Pointer for the queue.
 * @param event_id (in) User defined event id.
 * @param data (in) Pointer for user defined data, can be NULL.
 * @param wait_ns (in) Wait time in us when queue is full.
 * @param event_id (in) User defined event id.
 * @param option (in) Queue options.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_queue_add(Queue* queue, u32 event_id, void* data, s64 wait_us, Queue_option option);

/**
 * @brief Get an event from the queue. Data is passed by reference not by copy.
 * @param queue (in) Pointer for the queue.
 * @param event_id (out) Event id.
 * @param data (out) Pointer for data, can be NULL.
 * @param wait_ns (in) Wait time in us when queue is empty.
 * @return On success DEF_SUCCESS, 
 * on failure DEF_ERR_*.
 * @note Thread safe
*/
Result_with_string Util_queue_get(Queue* queue, u32* event_id, void** data, s64 wait_us);

/**
 * @brief Check if the specified event exist in the queue.
 * Always return false if queue is not initialized.
 * @param queue (in) Pointer for the queue.
 * @param event_id (in) Event id to check.
 * @note Thread safe
*/
bool Util_queue_check_event_exist(Queue* queue, u32 event_id);

/**
 * @brief Check how many spaces left in the queue.
 * Always return 0 if queue is not initialized.
 * @param queue (in) Pointer for the queue.
 * @note Thread safe
*/
int Util_queue_get_free_space(Queue* queue);

/**
 * @brief Delete the queue and if any, free all data in the queue.
 * Do nothing if queue is not initialized.
 * @param queue (in) Pointer for the queue.
 * @note Thread safe
*/
void Util_queue_delete(Queue* queue);

#endif
