#if !defined(DEF_LOG_TYPES_H)
#define DEF_LOG_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_LOG_BUFFER_LINES			(uint32_t)(512)
#define DEF_LOG_COLOR					(uint32_t)(0xFF00BB60)
#define DEF_LOG_MAX_LENGTH				(uint32_t)(1024)			//Maximum log text length in bytes.
#define DEF_LOG_INDEX_AUTO				(uint32_t)(UINT32_MAX)		//Use next log index.
#define DEF_LOG_GET_SYMBOL(x)			#x							//Get symbol name.
#define DEF_LOG_GET_FUNCTION_NAME(x)	(const char*)(__func__)		//Get function name.

#endif //!defined(DEF_LOG_TYPES_H)
