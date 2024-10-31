#if !defined(DEF_EXPL_TYPES_H)
#define DEF_EXPL_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_EXPL_API_ENABLE			/*(bool)(*/true/*)*/	//Enable file explorer API.

#define DEF_EXPL_MAX_FILES			(uint32_t)(1024)
#define DEF_EXPL_INVALID_INDEX		(uint32_t)(UINT32_MAX)

#define EXPL_FILE_TYPE_NONE			(Expl_file_type)(0 << 0)	//File type is not set.
#define EXPL_FILE_TYPE_FILE			(Expl_file_type)(1 << 0)	//This entry is a file.
#define EXPL_FILE_TYPE_DIR			(Expl_file_type)(1 << 1)	//This entry is a directory.
#define EXPL_FILE_TYPE_READ_ONLY	(Expl_file_type)(1 << 2)	//This entry is read only.
#define EXPL_FILE_TYPE_HIDDEN		(Expl_file_type)(1 << 3)	//This entry is hidden.
typedef uint8_t Expl_file_type;

#endif //!defined(DEF_EXPL_TYPES_H)
