//Includes.
#include "system/util/curl.h"

#if DEF_CURL_API_ENABLE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"
#include "curl/curl.h"

#include "system/menu.h"
#include "system/util/err_types.h"
#include "system/util/file.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/util.h"

//Defines.
#define DEF_CURL_USER_AGENT_FMT			(const char*)("Mozilla/5.0 (Horizon %s; ARMv6K) %s")
#define DEF_CURL_USER_AGENT_SIZE		(uint32_t)(128)

//Typedefs.
typedef struct
{
	uint8_t* data;
	uint32_t max_size;
	uint32_t current_size;
	uint32_t used_size_internal;
	uint32_t* used_size;
	const char* filename;
	const char* dir_path;
} Http_data;

typedef struct
{
	const uint8_t* data;
	uint32_t upload_size;
	uint32_t offset;
	uint32_t* uploaded_size;
	void* user_data;
	int32_t (*callback)(void* buffer, uint32_t max_size, void* user_data);
} Upload_data;

//Prototypes.
static size_t Util_curl_write_callback(char* input_data, size_t size, size_t nmemb, void* user_data);
static size_t Util_curl_save_callback(char* input_data, size_t size, size_t nmemb, void* user_data);
static size_t Util_curl_read_callback(char* output_buffer, size_t size, size_t nitems, void* user_data);
//We can't get rid of this "int" because library uses "int" type as args.
static int Util_curl_seek_callback(void* user_data, curl_off_t offset, int origin);
static uint32_t Util_curl_request(CURL** curl_handle, const char* url, const char* ua, CURLoption method, uint16_t max_redirect, Upload_data* upload_data);
static uint32_t Util_curl_get_request(CURL** curl_handle, const char* url, const char* ua, uint16_t max_redirect);
static uint32_t Util_curl_post_request(CURL** curl_handle, const char* url, const char* ua, Upload_data* upload_data, uint16_t max_redirect);
static void Util_curl_get_response(CURL** curl_handle, uint16_t* status_code, Str_data* new_url);
static uint32_t Util_curl_download_data(CURL** curl_handle, Http_data* http_data);
static uint32_t Util_curl_sv_data_internal(CURL** curl_handle, Http_data* http_data);
static void Util_curl_close(CURL** curl_handle);
static uint32_t Util_curl_dl_data_internal(Net_dl_parameters* parameters, Http_data* http_data);
static uint32_t Util_curl_save_data_internal(Net_save_parameters* parameters, Http_data* http_data);
static uint32_t Util_curl_post_and_dl_data_internal(Net_post_dl_parameters* parameters, Http_data* http_data, Upload_data* upload_data);
static uint32_t Util_curl_post_and_save_data_internal(Net_post_save_parameters* parameters, Http_data* http_data, Upload_data* upload_data);

//Variables.
static bool util_curl_init = false;
static uint32_t* util_curl_buffer = NULL;
static char util_curl_default_user_agent[DEF_CURL_USER_AGENT_SIZE] = { 0, };
static char util_curl_empty_char[1] = { 0, };

//Code.
uint32_t Util_curl_init(uint32_t buffer_size)
{
	uint32_t result = DEF_ERR_OTHER;
	char system_ver[0x20] = { 0, };

	if(util_curl_init)
		goto already_inited;

	//Buffer size must be multiple of 0x1000.
	if((buffer_size % 0x1000) != 0)
		goto invalid_arg;

	util_curl_buffer = (uint32_t*)memalign_heap(0x1000, buffer_size);
	if(!util_curl_buffer)
		goto out_of_memory;

	result = socInit(util_curl_buffer, buffer_size);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(socInit, false, result);
		goto nintendo_api_failed;
	}

	osGetSystemVersionDataString(NULL, NULL, system_ver, sizeof(system_ver));
	snprintf(util_curl_default_user_agent, sizeof(util_curl_default_user_agent),
	DEF_CURL_USER_AGENT_FMT, system_ver, curl_version());

	util_curl_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed:
	free(util_curl_buffer);
	util_curl_buffer = NULL;
	return result;
}

void Util_curl_exit(void)
{
	if(!util_curl_init)
		return;

	util_curl_init = false;
	socExit();
	free(util_curl_buffer);
	util_curl_buffer = NULL;
}

const char* Util_curl_get_default_user_agent(void)
{
	if(!util_curl_init)
		return util_curl_empty_char;

	return util_curl_default_user_agent;
}

uint32_t Util_curl_dl_data(Net_dl_parameters* parameters)
{
	Http_data http_data = { 0, };
	return Util_curl_dl_data_internal(parameters, &http_data);
}

uint32_t Util_curl_save_data(Net_save_parameters* parameters)
{
	Http_data http_data = { 0, };
	return Util_curl_save_data_internal(parameters, &http_data);
}

uint32_t Util_curl_post_and_dl_data(Net_post_dl_parameters* parameters)
{
	Http_data http_data = { 0, };
	Upload_data upload_data = { 0, };
	return Util_curl_post_and_dl_data_internal(parameters, &http_data, &upload_data);
}

uint32_t Util_curl_post_and_save_data(Net_post_save_parameters* parameters)
{
	Http_data http_data = { 0, };
	Upload_data upload_data = { 0, };
	return Util_curl_post_and_save_data_internal(parameters, &http_data, &upload_data);
}

static size_t Util_curl_write_callback(char* input_data, size_t size, size_t nmemb, void* user_data)
{
	uint32_t buffer_size = 0;
	uint32_t input_size = (size * nmemb);
	uint32_t allocated_size = 0;
	Http_data* http_data = (Http_data*)user_data;

	if(!user_data)
		return -1;

	//Out of memory.
	if((http_data->used_size_internal + input_size) > http_data->max_size)
		goto error;

	//Need to realloc memory.
	if((http_data->used_size_internal + input_size) > http_data->current_size)
	{
		buffer_size = ((http_data->max_size > (http_data->current_size + 0x40000)) ? (http_data->current_size + 0x40000) : http_data->max_size);

		http_data->data = (uint8_t*)realloc(http_data->data, buffer_size);
		if (!http_data->data)
			goto error;

		allocated_size = buffer_size - http_data->current_size;
		memset(http_data->data + http_data->current_size, 0x0, allocated_size);
		http_data->current_size = buffer_size;
	}

	memcpy((http_data->data + http_data->used_size_internal), input_data, input_size);
	http_data->used_size_internal += input_size;
	if(http_data->used_size)
		*http_data->used_size = http_data->used_size_internal;

	return input_size;

	error:
	free(http_data->data);
	http_data->data = NULL;
	http_data->current_size = 0;
	http_data->max_size = 0;
	http_data->used_size_internal = 0;
	if(http_data->used_size)
		*http_data->used_size = 0;

	return -1;
}

static size_t Util_curl_save_callback(char* input_data, size_t size, size_t nmemb, void* user_data)
{
	uint32_t input_size = (size * nmemb);
	uint32_t input_data_offset = 0;
	uint32_t result = DEF_ERR_OTHER;
	Http_data* http_data = (Http_data*)user_data;

	if(!user_data)
		return -1;

	http_data->used_size_internal += input_size;
	if(http_data->used_size)
		*http_data->used_size = http_data->used_size_internal;

	//If libcurl buffer size is bigger than our buffer size, save it directly without buffering.
	if(input_size > http_data->max_size)
	{
		result = Util_file_save_to_file(http_data->filename, http_data->dir_path, (uint8_t*)input_data, input_size, false);
		http_data->current_size = 0;
		if(result == DEF_SUCCESS)
			return (size * nmemb);
		else
		{
			DEF_LOG_RESULT(Util_file_save_to_file, false, result);
			return -1;
		}
	}
	//If we run out of buffer, save it.
	else if(http_data->current_size + input_size > http_data->max_size)
	{
		memcpy(http_data->data + http_data->current_size, input_data, http_data->max_size - http_data->current_size);
		input_data_offset = http_data->max_size - http_data->current_size;

		http_data->current_size += input_data_offset;
		input_size = input_size - input_data_offset;

		result = Util_file_save_to_file(http_data->filename, http_data->dir_path, http_data->data, http_data->current_size, false);
		http_data->current_size = 0;
	}
	else
		result = DEF_SUCCESS;

	memcpy(http_data->data + http_data->current_size, input_data + input_data_offset, input_size);
	http_data->current_size += input_size;

	if(result == DEF_SUCCESS)
		return (size * nmemb);
	else
	{
		DEF_LOG_RESULT(Util_file_save_to_file, false, result);
		return -1;
	}
}

static size_t Util_curl_read_callback(char* output_buffer, size_t size, size_t nitems, void* user_data)
{
	uint32_t buffer_size = (size * nitems);
	int32_t copy_size = 0;
	Upload_data* upload_data = (Upload_data*)user_data;

	if(!user_data)
		return -1;

	//If callback is provided, use it.
	if(upload_data->callback)
	{
		int32_t read_size = upload_data->callback(output_buffer, buffer_size, upload_data->user_data);
		if(upload_data->uploaded_size && read_size > 0)
			*upload_data->uploaded_size += read_size;

		return read_size;
	}

	//We've reached EOF.
	if(upload_data->upload_size - upload_data->offset == 0)
		return 0;

	//If buffer size is smaller than available post data size.
	if(buffer_size < upload_data->upload_size - upload_data->offset)
		copy_size = buffer_size;
	else
		copy_size = upload_data->upload_size - upload_data->offset;

	memcpy(output_buffer, upload_data->data + upload_data->offset, copy_size);
	if(upload_data->uploaded_size)
		*upload_data->uploaded_size += copy_size;

	upload_data->offset += copy_size;

	return copy_size;
}

//We can't get rid of this "int" because library uses "int" type as args.
static int Util_curl_seek_callback(void* user_data, curl_off_t offset, int origin)
{
	Upload_data* upload_data = (Upload_data*)user_data;

	if(!user_data)
		return CURL_SEEKFUNC_FAIL;

	if(origin == SEEK_SET)
		upload_data->offset = offset;
	else
		return CURL_SEEKFUNC_FAIL;

	return CURL_SEEKFUNC_OK;
}

static uint32_t Util_curl_request(CURL** curl_handle, const char* url, const char* ua, CURLoption method, uint16_t max_redirect, Upload_data* upload_data)
{
	uint32_t result = DEF_ERR_OTHER;

	result = curl_easy_setopt(*curl_handle, CURLOPT_URL, url);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_URL");
		goto curl_api_failed;
	}

	if(method == CURLOPT_POST)
		result = curl_easy_setopt(*curl_handle, CURLOPT_POST, 1);
	else
		result = curl_easy_setopt(*curl_handle, CURLOPT_HTTPGET, 1);

	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		if(method == CURLOPT_POST)
			DEF_LOG_STRING("CURLOPT_POST");
		else
			DEF_LOG_STRING("CURLOPT_HTTPGET");

		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_CAINFO, DEF_CURL_CERT_PATH);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_CAINFO");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_SSL_VERIFYPEER, 1);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_SSL_VERIFYPEER");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_FOLLOWLOCATION, (max_redirect > 0));
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_FOLLOWLOCATION");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_MAXREDIRS, max_redirect);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_MAXREDIRS");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_USERAGENT, (ua ? ua : util_curl_default_user_agent));
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_USERAGENT");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_BUFFERSIZE, 1000 * 128);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_BUFFERSIZE");
		goto curl_api_failed;
	}

	if(method == CURLOPT_POST)
	{
		result = curl_easy_setopt(*curl_handle, CURLOPT_POSTFIELDS, NULL);
		if (result != CURLE_OK)
		{
			DEF_LOG_RESULT(curl_easy_setopt, false, result);
			DEF_LOG_STRING("CURLOPT_POSTFIELDS");
			goto curl_api_failed;
		}

		/*result = curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, upload_data->upload_size);
		if (result != CURLE_OK)
		{
			DEF_LOG_RESULT(curl_easy_setopt, false, result);
			DEF_LOG_STRING("CURLOPT_POSTFIELDSIZE");
			goto curl_api_failed;
		}*/

		result = curl_easy_setopt(*curl_handle, CURLOPT_READFUNCTION, Util_curl_read_callback);
		if (result != CURLE_OK)
		{
			DEF_LOG_RESULT(curl_easy_setopt, false, result);
			DEF_LOG_STRING("CURLOPT_READFUNCTION");
			goto curl_api_failed;
		}

		result = curl_easy_setopt(*curl_handle, CURLOPT_READDATA, (void*)upload_data);
		if (result != CURLE_OK)
		{
			DEF_LOG_RESULT(curl_easy_setopt, false, result);
			DEF_LOG_STRING("CURLOPT_READDATA");
			goto curl_api_failed;
		}

		result = curl_easy_setopt(*curl_handle, CURLOPT_SEEKFUNCTION, Util_curl_seek_callback);
		if (result != CURLE_OK)
		{
			DEF_LOG_RESULT(curl_easy_setopt, false, result);
			DEF_LOG_STRING("CURLOPT_SEEKFUNCTION");
			goto curl_api_failed;
		}

		result = curl_easy_setopt(*curl_handle, CURLOPT_SEEKDATA, (void*)upload_data);
		if (result != CURLE_OK)
		{
			DEF_LOG_RESULT(curl_easy_setopt, false, result);
			DEF_LOG_STRING("CURLOPT_SEEKDATA");
			goto curl_api_failed;
		}
	}

	return DEF_SUCCESS;

	curl_api_failed:
	return DEF_ERR_CURL_RETURNED_NOT_SUCCESS;
}

static uint32_t Util_curl_get_request(CURL** curl_handle, const char* url, const char* ua, uint16_t max_redirect)
{
	return Util_curl_request(curl_handle, url, ua, CURLOPT_HTTPGET, max_redirect, NULL);
}

static uint32_t Util_curl_post_request(CURL** curl_handle, const char* url, const char* ua, Upload_data* upload_data, uint16_t max_redirect)
{
	return Util_curl_request(curl_handle, url, ua, CURLOPT_POST, max_redirect, upload_data);
}

static void Util_curl_get_response(CURL** curl_handle, uint16_t* status_code, Str_data* new_url)
{
	uint32_t result = DEF_ERR_OTHER;

	if(status_code)
	{
		//We can't get rid of this "long" because library uses "long" type as args.
		long out = 0;

		result = curl_easy_getinfo(*curl_handle, CURLINFO_RESPONSE_CODE, &out);
		if(result == CURLE_OK)
			*status_code = (uint16_t)out;
		else
		{
			DEF_LOG_RESULT(curl_easy_getinfo, false, result);
			DEF_LOG_STRING("CURLINFO_RESPONSE_CODE");
			*status_code = 0;
		}
	}

	if(new_url)
	{
		char* moved_url = NULL;

		result = curl_easy_getinfo(*curl_handle, CURLINFO_EFFECTIVE_URL, &moved_url);
		if(result == CURLE_OK && moved_url)
		{
			result = Util_str_set(new_url, moved_url);
			if(result != DEF_SUCCESS)
				DEF_LOG_RESULT(Util_str_set, false, result);
		}
	}
}

static uint32_t Util_curl_download_data(CURL** curl_handle, Http_data* http_data)
{
	uint32_t result = DEF_ERR_OTHER;
	char error_message[CURL_ERROR_SIZE] = { 0, };

	result = curl_easy_setopt(*curl_handle, CURLOPT_WRITEFUNCTION, Util_curl_write_callback);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_WRITEFUNCTION");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_WRITEDATA, (void*)http_data);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_WRITEDATA");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_ERRORBUFFER, error_message);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_ERRORBUFFER");
		goto curl_api_failed;
	}

	result = curl_easy_perform(*curl_handle);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_perform, false, result);
		DEF_LOG_STRING(error_message);
		goto curl_api_failed;
	}

	return DEF_SUCCESS;

	curl_api_failed:
	free(http_data->data);
	http_data->data = NULL;
	http_data->current_size = 0;
	http_data->max_size = 0;
	http_data->used_size_internal = 0;
	if(http_data->used_size)
		*http_data->used_size = 0;

	return DEF_ERR_CURL_RETURNED_NOT_SUCCESS;
}

static uint32_t Util_curl_sv_data_internal(CURL** curl_handle, Http_data* http_data)
{
	uint32_t result = DEF_ERR_OTHER;
	char error_message[CURL_ERROR_SIZE] = { 0, };

	http_data->data = (uint8_t*)malloc(http_data->max_size);
	if(!http_data->data)
		goto out_of_memory;

	result = curl_easy_setopt(*curl_handle, CURLOPT_WRITEFUNCTION, Util_curl_save_callback);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_WRITEFUNCTION");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_WRITEDATA, (void*)http_data);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_WRITEDATA");
		goto curl_api_failed;
	}

	result = curl_easy_setopt(*curl_handle, CURLOPT_ERRORBUFFER, error_message);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_setopt, false, result);
		DEF_LOG_STRING("CURLOPT_ERRORBUFFER");
		goto curl_api_failed;
	}

	Util_file_delete_file(http_data->filename, http_data->dir_path);

	result = curl_easy_perform(*curl_handle);
	if (result != CURLE_OK)
	{
		DEF_LOG_RESULT(curl_easy_perform, false, result);
		DEF_LOG_STRING(error_message);
		goto curl_api_failed;
	}

	Util_file_save_to_file(http_data->filename, http_data->dir_path, http_data->data, http_data->current_size, false);

	free(http_data->data);
	http_data->data = NULL;
	return DEF_SUCCESS;

	out_of_memory:
	http_data->current_size = 0;
	http_data->max_size = 0;
	http_data->used_size_internal = 0;
	if(http_data->used_size)
		*http_data->used_size = 0;

	return DEF_ERR_OUT_OF_MEMORY;

	curl_api_failed:
	free(http_data->data);
	http_data->data = NULL;
	http_data->current_size = 0;
	http_data->max_size = 0;
	http_data->used_size_internal = 0;
	if(http_data->used_size)
		*http_data->used_size = 0;

	return DEF_ERR_CURL_RETURNED_NOT_SUCCESS;
}

static void Util_curl_close(CURL** curl_handle)
{
	if(!curl_handle)
		return;

	if(*curl_handle)
		curl_easy_cleanup(*curl_handle);

	*curl_handle = NULL;
}

static uint32_t Util_curl_dl_data_internal(Net_dl_parameters* parameters, Http_data* http_data)
{
	uint32_t result = DEF_ERR_OTHER;
	CURL* curl_handle = NULL;

	if(!util_curl_init)
		goto not_inited;

	//user_agent, downloaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->url || parameters->max_size == 0 || !http_data)
		goto invalid_arg;

	if(parameters->downloaded_size)
		*parameters->downloaded_size = 0;
	if(parameters->status_code)
		*parameters->status_code = 0;

	parameters->data = NULL;

	curl_handle = curl_easy_init();
	if(!curl_handle)
	{
		DEF_LOG_RESULT(curl_easy_init, false, DEF_ERR_CURL_RETURNED_NOT_SUCCESS);
		goto curl_api_failed;
	}

	if(parameters->last_url)
	{
		result = Util_str_init(parameters->last_url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto api_failed;
		}

		result = Util_str_set(parameters->last_url, parameters->url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_set, false, result);
			goto api_failed;
		}
	}

	http_data->max_size = parameters->max_size;
	http_data->used_size = parameters->downloaded_size;

	result = Util_curl_get_request(&curl_handle, parameters->url, parameters->user_agent, parameters->max_redirect);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_get_request, false, result);
		goto api_failed;
	}

	result = Util_curl_download_data(&curl_handle, http_data);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_download_data, false, result);
		goto api_failed;
	}

	parameters->data = http_data->data;
	Util_curl_get_response(&curl_handle, parameters->status_code, parameters->last_url);
	Util_curl_close(&curl_handle);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	curl_api_failed:
	return DEF_ERR_CURL_RETURNED_NOT_SUCCESS;

	api_failed:
	Util_curl_close(&curl_handle);
	free(parameters->data);
	parameters->data = NULL;
	if(parameters->last_url)
		Util_str_free(parameters->last_url);
	if(parameters->downloaded_size)
		*parameters->downloaded_size = 0;
	if(parameters->status_code)
		*parameters->status_code = 0;

	return result;
}

static uint32_t Util_curl_save_data_internal(Net_save_parameters* parameters, Http_data* http_data)
{
	uint32_t result = DEF_ERR_OTHER;
	CURL* curl_handle = NULL;

	if(!util_curl_init)
		goto not_inited;

	//user_agent, downloaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->url || parameters->buffer_size == 0
	|| !parameters->dir_path || !parameters->filename || !http_data)
		goto invalid_arg;

	if(parameters->downloaded_size)
		*parameters->downloaded_size = 0;
	if(parameters->status_code)
		*parameters->status_code = 0;

	curl_handle = curl_easy_init();
	if(!curl_handle)
	{
		DEF_LOG_RESULT(curl_easy_init, false, DEF_ERR_CURL_RETURNED_NOT_SUCCESS);
		goto curl_api_failed;
	}

	if(parameters->last_url)
	{
		result = Util_str_init(parameters->last_url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto api_failed;
		}

		result = Util_str_set(parameters->last_url, parameters->url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_set, false, result);
			goto api_failed;
		}
	}

	http_data->max_size = parameters->buffer_size;
	http_data->used_size = parameters->downloaded_size;
	http_data->dir_path = parameters->dir_path;
	http_data->filename = parameters->filename;

	result = Util_curl_get_request(&curl_handle, parameters->url, parameters->user_agent, parameters->max_redirect);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_get_request, false, result);
		goto api_failed;
	}

	result = Util_curl_sv_data_internal(&curl_handle, http_data);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_sv_data_internal, false, result);
		goto api_failed;
	}

	Util_curl_get_response(&curl_handle, parameters->status_code, parameters->last_url);
	Util_curl_close(&curl_handle);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	curl_api_failed:
	return DEF_ERR_CURL_RETURNED_NOT_SUCCESS;

	api_failed:
	Util_curl_close(&curl_handle);
	if(parameters->last_url)
		Util_str_free(parameters->last_url);
	if(parameters->downloaded_size)
		*parameters->downloaded_size = 0;
	if(parameters->status_code)
		*parameters->status_code = 0;

	return result;
}

static uint32_t Util_curl_post_and_dl_data_internal(Net_post_dl_parameters* parameters, Http_data* http_data, Upload_data* upload_data)
{
	uint32_t result = DEF_ERR_OTHER;
	CURL* curl_handle = NULL;

	if(!util_curl_init)
		goto not_inited;

	//user_agent, downloaded_size, uploaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->dl.url || parameters->dl.max_size == 0 || !http_data || !upload_data)
		goto invalid_arg;

	if(parameters->is_callback)
	{
		//user_data can be NULL.
		if(!parameters->u.callback.read_callback)
			goto invalid_arg;
	}
	else
	{
		if(!parameters->u.data.data || parameters->u.data.size == 0)
			goto invalid_arg;
	}

	if(parameters->dl.downloaded_size)
		*parameters->dl.downloaded_size = 0;
	if(parameters->uploaded_size)
		*parameters->uploaded_size = 0;
	if(parameters->dl.status_code)
		*parameters->dl.status_code = 0;

	parameters->dl.data = NULL;

	curl_handle = curl_easy_init();
	if(!curl_handle)
	{
		DEF_LOG_RESULT(curl_easy_init, false, DEF_ERR_CURL_RETURNED_NOT_SUCCESS);
		goto curl_api_failed;
	}

	if(parameters->dl.last_url)
	{
		result = Util_str_init(parameters->dl.last_url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto api_failed;
		}

		result = Util_str_set(parameters->dl.last_url, parameters->dl.url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_set, false, result);
			goto api_failed;
		}
	}

	http_data->max_size = parameters->dl.max_size;
	http_data->used_size = parameters->dl.downloaded_size;
	upload_data->uploaded_size = parameters->uploaded_size;
	if(parameters->is_callback)
	{
		upload_data->callback = parameters->u.callback.read_callback;
		upload_data->user_data = parameters->u.callback.user_data;
	}
	else
	{
		upload_data->upload_size = parameters->u.data.size;
		upload_data->data = parameters->u.data.data;
	}

	result = Util_curl_post_request(&curl_handle, parameters->dl.url, parameters->dl.user_agent, upload_data, parameters->dl.max_redirect);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_post_request, false, result);
		goto api_failed;
	}

	result = Util_curl_download_data(&curl_handle, http_data);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_download_data, false, result);
		goto api_failed;
	}

	parameters->dl.data = http_data->data;
	Util_curl_get_response(&curl_handle, parameters->dl.status_code, parameters->dl.last_url);
	Util_curl_close(&curl_handle);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	curl_api_failed:
	return DEF_ERR_CURL_RETURNED_NOT_SUCCESS;

	api_failed:
	Util_curl_close(&curl_handle);
	free(parameters->dl.data);
	parameters->dl.data = NULL;
	if(parameters->dl.last_url)
		Util_str_free(parameters->dl.last_url);
	if(parameters->dl.downloaded_size)
		*parameters->dl.downloaded_size = 0;
	if(parameters->dl.status_code)
		*parameters->dl.status_code = 0;
	if(parameters->uploaded_size)
		*parameters->uploaded_size = 0;

	return result;
}

static uint32_t Util_curl_post_and_save_data_internal(Net_post_save_parameters* parameters, Http_data* http_data, Upload_data* upload_data)
{
	uint32_t result = DEF_ERR_OTHER;
	CURL* curl_handle = NULL;

	if(!util_curl_init)
		goto not_inited;

	//user_agent, downloaded_size, uploaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->save.url || parameters->save.buffer_size == 0
	|| !parameters->save.dir_path || !parameters->save.filename)
		goto invalid_arg;

	if(parameters->is_callback)
	{
		//user_data can be NULL.
		if(!parameters->u.callback.read_callback)
			goto invalid_arg;
	}
	else
	{
		if(!parameters->u.data.data || parameters->u.data.size == 0)
			goto invalid_arg;
	}

	if(parameters->save.downloaded_size)
		*parameters->save.downloaded_size = 0;
	if(parameters->uploaded_size)
		*parameters->uploaded_size = 0;
	if(parameters->save.status_code)
		*parameters->save.status_code = 0;

	curl_handle = curl_easy_init();
	if(!curl_handle)
	{
		DEF_LOG_RESULT(curl_easy_init, false, DEF_ERR_CURL_RETURNED_NOT_SUCCESS);
		goto curl_api_failed;
	}

	if(parameters->save.last_url)
	{
		result = Util_str_init(parameters->save.last_url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto api_failed;
		}

		result = Util_str_set(parameters->save.last_url, parameters->save.url);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_set, false, result);
			goto api_failed;
		}
	}

	http_data->max_size = parameters->save.buffer_size;
	http_data->used_size = parameters->save.downloaded_size;
	http_data->dir_path = parameters->save.dir_path;
	http_data->filename = parameters->save.filename;
	upload_data->uploaded_size = parameters->uploaded_size;
	if(parameters->is_callback)
	{
		upload_data->callback = parameters->u.callback.read_callback;
		upload_data->user_data = parameters->u.callback.user_data;
	}
	else
	{
		upload_data->upload_size = parameters->u.data.size;
		upload_data->data = parameters->u.data.data;
	}

	result = Util_curl_post_request(&curl_handle, parameters->save.url, parameters->save.user_agent, upload_data, parameters->save.max_redirect);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_post_request, false, result);
		goto api_failed;
	}

	result = Util_curl_sv_data_internal(&curl_handle, http_data);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_curl_sv_data_internal, false, result);
		goto api_failed;
	}

	Util_curl_get_response(&curl_handle, parameters->save.status_code, parameters->save.last_url);
	Util_curl_close(&curl_handle);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	curl_api_failed:
	return DEF_ERR_CURL_RETURNED_NOT_SUCCESS;

	api_failed:
	Util_curl_close(&curl_handle);
	if(parameters->save.last_url)
		Util_str_free(parameters->save.last_url);
	if(parameters->save.downloaded_size)
		*parameters->save.downloaded_size = 0;
	if(parameters->save.status_code)
		*parameters->save.status_code = 0;
	if(parameters->uploaded_size)
		*parameters->uploaded_size = 0;

	return result;
}
#endif //DEF_CURL_API_ENABLE
