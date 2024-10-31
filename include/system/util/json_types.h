#if !defined(DEF_JSON_TYPES_H)
#define DEF_JSON_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_JSON_KEY					/*(const char*)(*/"\"%s\":"/*)*/
#define DEF_JSON_STR_DATA(fmt)			/*(const char*)(*/"\"" fmt "\""/*)*/
#define DEF_JSON_NON_STR_DATA(fmt)		/*(const char*)(*/fmt/*)*/
#define DEF_JSON_START_ARRAY			/*(const char*)(*/"["/*)*/
#define DEF_JSON_END_ARRAY				/*(const char*)(*/"]"/*)*/
#define DEF_JSON_START_OBJECT			/*(const char*)(*/"{"/*)*/
#define DEF_JSON_END_OBJECT				/*(const char*)(*/"}"/*)*/
#define DEF_JSON_COMMA					/*(const char*)(*/","/*)*/
#define DEF_JSON_STR_DATA_WITH_KEY(key, data_fmt, data)							(DEF_JSON_KEY DEF_JSON_STR_DATA(data_fmt) DEF_JSON_COMMA), key, data
#define DEF_JSON_STR_DATA_WITH_KEY_WITHOUT_COMMA(key, data_fmt, data)			(DEF_JSON_KEY DEF_JSON_STR_DATA(data_fmt)), key, data
#define DEF_JSON_NON_STR_DATA_WITH_KEY(key, data_fmt, data)						(DEF_JSON_KEY DEF_JSON_NON_STR_DATA(data_fmt) DEF_JSON_COMMA), key, data
#define DEF_JSON_NON_STR_DATA_WITH_KEY_WITHOUT_COMMA(key, data_fmt, data)		(DEF_JSON_KEY DEF_JSON_NON_STR_DATA(data_fmt)), key, data
#define DEF_JSON_ARRAY_WITH_KEY(key, data)										(DEF_JSON_KEY DEF_JSON_START_ARRAY "%s" DEF_JSON_END_ARRAY DEF_JSON_COMMA), key, data
#define DEF_JSON_ARRAY_WITH_KEY_WITHOUT_COMMA(key, data)						(DEF_JSON_KEY DEF_JSON_START_ARRAY "%s" DEF_JSON_END_ARRAY), key, data
#define DEF_JSON_OBJECT_WITH_KEY(key, data)										(DEF_JSON_KEY DEF_JSON_START_OBJECT "%s" DEF_JSON_END_OBJECT DEF_JSON_COMMA), key, data
#define DEF_JSON_OBJECT_WITH_KEY_WITHOUT_COMMA(key, data)						(DEF_JSON_KEY DEF_JSON_START_OBJECT "%s" DEF_JSON_END_OBJECT), key, data

#endif //!defined(DEF_JSON_TYPES_H)
