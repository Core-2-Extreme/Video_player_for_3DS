#if !defined(DEF_FILE_TYPES_H)
#define DEF_FILE_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define FILE_TYPE_NONE			(File_type)(0 << 0)	//File type is not set.
#define FILE_TYPE_FILE			(File_type)(1 << 0)	//This entry is a file.
#define FILE_TYPE_DIR			(File_type)(1 << 1)	//This entry is a directory.
#define FILE_TYPE_READ_ONLY		(File_type)(1 << 2)	//This entry is read only.
#define FILE_TYPE_HIDDEN		(File_type)(1 << 3)	//This entry is hidden.
typedef uint8_t File_type;

#endif //!defined(DEF_FILE_TYPES_H)
