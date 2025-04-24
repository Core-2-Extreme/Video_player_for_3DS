#if !defined(DEF_NET_TYPES_H)
#define DEF_NET_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/str_types.h"

typedef struct
{
	const uint8_t* data;	//(in) Upload data.
	uint32_t size;			//(in) Data size in bytes.
} Net_post_data;

typedef struct
{
	int32_t (*read_callback)(void* buffer, uint32_t max_size, void* user_data);		//(in) Callback function to query upload data.
	void* user_data;		//(in) User defined data, can be NULL.
} Net_post_callback_data;

typedef struct
{
	const char* url;			//(in)  Target URL.
	const char* user_agent;		//(in)  User agent, can be NULL (default value will be used if NULL).
	uint8_t* data;				//(out) Pointer for response data, the pointer will be allocated up to max_size depends on server response.
	uint16_t* status_code;		//(out) HTTP status code, can be NULL.
	uint16_t max_redirect;		//(in)  Maximum allowed number of redirects (0 to disallow redirect).
	uint32_t max_size;			//(in)  Maximum allowed download size in bytes.
	uint32_t* downloaded_size;	//(out) Actual downloaded size in bytes, can be NULL.
	Str_data* last_url;			//(out) Last URL (data contains response of last URL), can be NULL.
} Net_dl_parameters;

typedef struct
{
	const char* url;			//(in)  Target URL.
	const char* user_agent;		//(in)  User agent, can be NULL (default value will be used if NULL).
	const char* dir_path;		//(in)  Output directory path.
	const char* filename;		//(in)  Output filename.
	uint16_t* status_code;		//(out) HTTP status code, can be NULL.
	uint16_t max_redirect;		//(in)  Maximum allowed number of redirects (0 to disallow redirect).
	uint32_t buffer_size;		//(in)  Internal work buffer size in bytes.
	uint32_t* downloaded_size;	//(out) Actual downloaded size in bytes, can be NULL.
	Str_data* last_url;			//(out) Last URL (data contains response of last URL), can be NULL.
} Net_save_parameters;

typedef struct
{
	bool is_callback;			//(in)  Whether use callback function for upload, callback is only supported on curl.
	uint32_t* uploaded_size;	//(out) Actual uploaded size.
	union
	{
		Net_post_data data;
		Net_post_callback_data callback;
	} u;
	Net_dl_parameters dl;		//(in/out) Download related parameters (mostly).
} Net_post_dl_parameters;

typedef struct
{
	bool is_callback;			//(in)  Whether use callback function for upload, callback is only supported on curl.
	uint32_t* uploaded_size;	//(out) Actual uploaded size.
	union
	{
		Net_post_data data;
		Net_post_callback_data callback;
	} u;
	Net_save_parameters save;	//(in/out) Download related parameters (mostly).
} Net_post_save_parameters;

#endif //!defined(DEF_NET_TYPES_H)
