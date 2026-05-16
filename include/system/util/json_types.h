#if !defined(DEF_JSON_TYPES_H)
#define DEF_JSON_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/log_enum_types.h"

#define DEF_JSON_API_ENABLE				/*(bool)(*/true/*)*/		//Enable json parser API.

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

typedef enum
{
	JSON_TYPE_INVALID = -1,

	JSON_TYPE_STRING,	//Value type is "String", size will be string length without null terminator.
	JSON_TYPE_NUMBER,	//Value type is number, size will be sizeof(double).
	JSON_TYPE_BOOL,		//Value type is bool, size will be sizeof(bool),
	JSON_TYPE_NULL,		//Value type is null, size will be 0 and data will be NULL,

	JSON_TYPE_MAX,
} Json_value_type;

DEF_LOG_ENUM_DEBUG
(
	Json_value_type,
	JSON_TYPE_INVALID,
	JSON_TYPE_STRING,
	JSON_TYPE_NUMBER,
	JSON_TYPE_BOOL,
	JSON_TYPE_NULL,
	JSON_TYPE_MAX
)

typedef struct
{
	void* internal_data;	//Internal data for JSON APIs.
} Json_data;

typedef struct
{
	uint32_t size;			//Size of value may be zero.
	void* value;			//Extracted value may be NULL.
	Json_value_type type;	//Type of value.
} Json_extracted_data;

#endif //!defined(DEF_JSON_TYPES_H)
