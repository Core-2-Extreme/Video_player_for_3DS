#if !defined(DEF_UTIL_TYPES_H)
#define DEF_UTIL_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_UTIL_LIB_INFO_COUNT		(uint8_t)(16)

typedef struct
{
	const char* name;				//Name of library.
	const char* ver;				//Library version.
	const char* license;			//License name.
	const char* full_license_url;	//URL for full-license.
} Util_lib_info;

#endif //!defined(DEF_UTIL_TYPES_H)
