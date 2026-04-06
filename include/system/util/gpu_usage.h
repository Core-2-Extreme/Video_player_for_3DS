#if !defined(DEF_GPU_USAGE_H)
#define DEF_GPU_USAGE_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/gpu_usage_types.h"

#if DEF_GPU_USAGE_API_ENABLE

/**
 * @brief Initialize GPU usage API.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_* or Nintendo API's error.
 * @warning Thread dangerous (untested).
*/
uint32_t Util_gpu_usage_init(void);

/**
 * @brief Uninitialize GPU usage API.
 * Do nothing if GPU usage API is not initialized.
 * @warning Thread dangerous (untested).
*/
void Util_gpu_usage_exit(void);

/**
 * @brief Get GPU usage.
 * Always return NAN if GPU usage API is not initialized.
 * @return GPU usage in %.
 * @note Thread safe.
*/
float Util_gpu_usage_get_gpu_usage(void);

/**
 * @brief Query GPU usage show flag.
 * Always return false if GPU usage API is not initialized.
 * @return Internal GPU usage show flag.
 * @warning Thread dangerous (untested).
*/
bool Util_gpu_usage_query_show_flag(void);

/**
 * @brief Set GPU usage show flag.
 * Do nothing if GPU usage API is not initialized.
 * @param flag (in) When true, internal GPU usage show flag will be set to true otherwise set to false.
 * @warning Thread dangerous (untested).
*/
void Util_gpu_usage_set_show_flag(bool flag);

/**
 * @brief Draw GPU usage.
 * @warning Thread dangerous (untested).
 * @warning Call it only from rendering thread.
*/
void Util_gpu_usage_draw(void);

#else

#define Util_gpu_usage_init() DEF_ERR_DISABLED
#define Util_gpu_usage_exit()
#define Util_gpu_usage_get_gpu_usage() NAN
#define Util_gpu_usage_query_show_flag() false
#define Util_gpu_usage_set_show_flag(...)
#define Util_gpu_usage_draw()

#endif //DEF_GPU_USAGE_API_ENABLE

#endif //!defined(DEF_GPU_USAGE_H)
