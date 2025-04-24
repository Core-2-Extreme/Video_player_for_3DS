//Includes.
#include "system/util/httpc.h"

#if DEF_HTTPC_API_ENABLE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/menu.h"
#include "system/util/err_types.h"
#include "system/util/file.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/util.h"

//Defines.
#define DEF_HTTPC_USER_AGENT_FMT			(const char*)("Mozilla/5.0 (Horizon %s; ARMv6K) httpc/1.0.0")
#define DEF_HTTPC_USER_AGENT_SIZE			(uint32_t)(128)

//Typedefs.
//N/A.

//Prototypes.
static uint32_t Util_httpc_request(httpcContext* httpc_context, const char* url, const char* ua, HTTPC_RequestMethod method, const uint8_t* post_data, uint32_t post_data_size);
static uint32_t Util_httpc_get_request(httpcContext* httpc_context, const char* url, const char* ua);
static uint32_t Util_httpc_post_request(httpcContext* httpc_context, const char* url, const char* ua, const uint8_t* post_data, uint32_t post_data_size);
static void Util_httpc_get_response(httpcContext* httpc_context, uint16_t* status_code, Str_data* new_url, bool* redirected);
static uint32_t Util_httpc_download_data(httpcContext* httpc_context, uint8_t** data, uint32_t max_size, uint32_t* downloaded_size);
static void Util_httpc_close(httpcContext* httpc_context);
static uint32_t Util_httpc_sv_data_internal(httpcContext* httpc_context, uint32_t buffer_size, uint32_t* downloaded_size, const char* dir_path, const char* file_name);

//Variables.
static bool util_httpc_init = false;
static char util_httpc_default_user_agent[DEF_HTTPC_USER_AGENT_SIZE] = { 0, };
static char util_httpc_empty_char[1] = { 0, };

//Code.
uint32_t Util_httpc_init(uint32_t buffer_size)
{
	uint32_t result = DEF_ERR_OTHER;
	char system_ver[0x20] = { 0, };

	if(util_httpc_init)
		goto already_inited;

	//Buffer size must be multiple of 0x1000.
	if((buffer_size % 0x1000) != 0)
		goto invalid_arg;

	result = httpcInit(buffer_size);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(httpcInit, false, result);
		goto nintendo_api_failed;
	}

	osGetSystemVersionDataString(NULL, NULL, system_ver, sizeof(system_ver));
	snprintf(util_httpc_default_user_agent, sizeof(util_httpc_default_user_agent),
	DEF_HTTPC_USER_AGENT_FMT, system_ver);

	util_httpc_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	return result;
}

void Util_httpc_exit(void)
{
	if(!util_httpc_init)
		return;

	util_httpc_init = false;
	httpcExit();
}

const char* Util_httpc_get_default_user_agent(void)
{
	if(!util_httpc_init)
		return util_httpc_empty_char;

	return util_httpc_default_user_agent;
}

uint32_t Util_httpc_dl_data(Net_dl_parameters* parameters)
{
	uint16_t redirected = 0;
	uint32_t result = DEF_ERR_OTHER;
	httpcContext httpc_context = { 0, };
	Str_data current_url = { 0, };

	if(!util_httpc_init)
		goto not_inited;

	//user_agent, downloaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->url || parameters->max_size == 0)
		goto invalid_arg;

	if(parameters->downloaded_size)
		*parameters->downloaded_size = 0;
	if(parameters->status_code)
		*parameters->status_code = 0;

	parameters->data = NULL;

	result = Util_str_init(&current_url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
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

	result = Util_str_set(&current_url, parameters->url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_failed;
	}

	while (true)
	{
		bool is_redirected = false;

		result = Util_httpc_get_request(&httpc_context, current_url.buffer, parameters->user_agent);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_httpc_get_request, false, result);
			goto api_failed;
		}

		Util_httpc_get_response(&httpc_context, parameters->status_code, &current_url, &is_redirected);

		if (is_redirected && parameters->max_redirect > redirected)
		{
			if(parameters->last_url)
			{
				result = Util_str_set(parameters->last_url, current_url.buffer);
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_str_set, false, result);
					goto api_failed;
				}
			}
			redirected++;
		}
		else
			is_redirected = false;

		if (!is_redirected)
		{
			result = Util_httpc_download_data(&httpc_context, &parameters->data, parameters->max_size, parameters->downloaded_size);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_httpc_download_data, false, result);
				goto api_failed;
			}
		}

		Util_httpc_close(&httpc_context);

		if (!is_redirected)
			break;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	Util_httpc_close(&httpc_context);
	Util_str_free(&current_url);
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

uint32_t Util_httpc_save_data(Net_save_parameters* parameters)
{
	uint16_t redirected = 0;
	uint32_t result = DEF_ERR_OTHER;
	httpcContext httpc_context = { 0, };
	Str_data current_url = { 0, };

	if(!util_httpc_init)
		goto not_inited;

	//user_agent, downloaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->url || parameters->buffer_size == 0
	|| !parameters->dir_path || !parameters->filename)
		goto invalid_arg;

	if(parameters->downloaded_size)
		*parameters->downloaded_size = 0;
	if(parameters->status_code)
		*parameters->status_code = 0;

	result = Util_str_init(&current_url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
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

	result = Util_str_set(&current_url, parameters->url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_failed;
	}

	while (true)
	{
		bool is_redirected = false;

		result = Util_httpc_get_request(&httpc_context, current_url.buffer, parameters->user_agent);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_httpc_get_request, false, result);
			goto api_failed;
		}

		Util_httpc_get_response(&httpc_context, parameters->status_code, &current_url, &is_redirected);

		if (is_redirected && parameters->max_redirect > redirected)
		{
			if(parameters->last_url)
			{
				result = Util_str_set(parameters->last_url, current_url.buffer);
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_str_set, false, result);
					goto api_failed;
				}
			}
			redirected++;
		}
		else
			is_redirected = false;

		if (!is_redirected)
		{
			result = Util_httpc_sv_data_internal(&httpc_context, parameters->buffer_size,
			parameters->downloaded_size, parameters->dir_path, parameters->filename);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_httpc_sv_data_internal, false, result);
				goto api_failed;
			}
		}

		Util_httpc_close(&httpc_context);

		if (!is_redirected)
			break;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	Util_httpc_close(&httpc_context);
	Util_str_free(&current_url);
	if(parameters->last_url)
		Util_str_free(parameters->last_url);
	if(parameters->downloaded_size)
		*parameters->downloaded_size = 0;
	if(parameters->status_code)
		*parameters->status_code = 0;

	return result;
}

uint32_t Util_httpc_post_and_dl_data(Net_post_dl_parameters* parameters)
{
	bool post = true;
	uint16_t redirected = 0;
	uint32_t result = DEF_ERR_OTHER;
	httpcContext httpc_context = { 0, };
	Str_data current_url = { 0, };

	if(!util_httpc_init)
		goto not_inited;

	//user_agent, downloaded_size, uploaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->dl.url || parameters->dl.max_size == 0)
		goto invalid_arg;

	//Callback is NOT supported.
	if(parameters->is_callback)
		goto invalid_arg;

	if(!parameters->u.data.data || parameters->u.data.size == 0)
		goto invalid_arg;

	if(parameters->dl.downloaded_size)
		*parameters->dl.downloaded_size = 0;
	if(parameters->uploaded_size)
		*parameters->uploaded_size = 0;
	if(parameters->dl.status_code)
		*parameters->dl.status_code = 0;

	parameters->dl.data = NULL;

	result = Util_str_init(&current_url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
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

	result = Util_str_set(&current_url, parameters->dl.url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_failed;
	}

	while (true)
	{
		bool is_redirected = false;

		if(post)
		{
			result = Util_httpc_post_request(&httpc_context, current_url.buffer, parameters->dl.user_agent, parameters->u.data.data, parameters->u.data.size);
			post = false;
		}
		else
			result = Util_httpc_get_request(&httpc_context, current_url.buffer, parameters->dl.user_agent);

		if(result != DEF_SUCCESS)
		{
			if(post)
				DEF_LOG_RESULT(Util_httpc_post_request, false, result);
			else
				DEF_LOG_RESULT(Util_httpc_get_request, false, result);

			goto api_failed;
		}

		Util_httpc_get_response(&httpc_context, parameters->dl.status_code, &current_url, &is_redirected);

		if (is_redirected && parameters->dl.max_redirect > redirected)
		{
			if(parameters->dl.last_url)
			{
				result = Util_str_set(parameters->dl.last_url, current_url.buffer);
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_str_set, false, result);
					goto api_failed;
				}
			}
			redirected++;
		}
		else
			is_redirected = false;

		if (!is_redirected)
		{
			result = Util_httpc_download_data(&httpc_context, &parameters->dl.data, parameters->dl.max_size, parameters->dl.downloaded_size);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_httpc_download_data, false, result);
				goto api_failed;
			}
		}

		Util_httpc_close(&httpc_context);

		if (!is_redirected)
			break;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	Util_httpc_close(&httpc_context);
	Util_str_free(&current_url);
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

uint32_t Util_httpc_post_and_save_data(Net_post_save_parameters* parameters)
{
	bool post = true;
	uint16_t redirected = 0;
	uint32_t result = DEF_ERR_OTHER;
	httpcContext httpc_context = { 0, };
	Str_data current_url = { 0, };

	if(!util_httpc_init)
		goto not_inited;

	//user_agent, downloaded_size, uploaded_size, status_code and last_url can be NULL.
	if(!parameters || !parameters->save.url || parameters->save.buffer_size == 0
	|| !parameters->save.dir_path || !parameters->save.filename)
		goto invalid_arg;

	//Callback is NOT supported.
	if(parameters->is_callback)
		goto invalid_arg;

	if(!parameters->u.data.data || parameters->u.data.size == 0)
		goto invalid_arg;

	if(parameters->save.downloaded_size)
		*parameters->save.downloaded_size = 0;
	if(parameters->uploaded_size)
		*parameters->uploaded_size = 0;
	if(parameters->save.status_code)
		*parameters->save.status_code = 0;

	result = Util_str_init(&current_url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_failed;
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

	result = Util_str_set(&current_url, parameters->save.url);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_failed;
	}

	while (true)
	{
		bool is_redirected = false;

		if(post)
		{
			result = Util_httpc_post_request(&httpc_context, current_url.buffer, parameters->save.user_agent, parameters->u.data.data, parameters->u.data.size);
			post = false;
		}
		else
			result = Util_httpc_get_request(&httpc_context, current_url.buffer, parameters->save.user_agent);

		if(result != DEF_SUCCESS)
		{
			if(post)
				DEF_LOG_RESULT(Util_httpc_post_request, false, result);
			else
				DEF_LOG_RESULT(Util_httpc_get_request, false, result);

			goto api_failed;
		}

		Util_httpc_get_response(&httpc_context, parameters->save.status_code, &current_url, &is_redirected);

		if (is_redirected && parameters->save.max_redirect > redirected)
		{
			if(parameters->save.last_url)
			{
				result = Util_str_set(parameters->save.last_url, current_url.buffer);
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_str_set, false, result);
					goto api_failed;
				}
			}
			redirected++;
		}
		else
			is_redirected = false;

		if (!is_redirected)
		{
			result = Util_httpc_sv_data_internal(&httpc_context, parameters->save.buffer_size,
			parameters->save.downloaded_size, parameters->save.dir_path, parameters->save.filename);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_httpc_sv_data_internal, false, result);
				goto api_failed;
			}
		}

		Util_httpc_close(&httpc_context);

		if (!is_redirected)
			break;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_failed:
	Util_httpc_close(&httpc_context);
	Util_str_free(&current_url);
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

static uint32_t Util_httpc_request(httpcContext* httpc_context, const char* url, const char* ua, HTTPC_RequestMethod method, const uint8_t* post_data, uint32_t post_data_size)
{
	uint32_t result = DEF_ERR_OTHER;

	if(method == HTTPC_METHOD_POST)
		result = httpcOpenContext(httpc_context, HTTPC_METHOD_POST, url, 0);
	else
		result = httpcOpenContext(httpc_context, HTTPC_METHOD_GET, url, 0);

	if (result != DEF_SUCCESS)
	{
		if(method == HTTPC_METHOD_POST)
			DEF_LOG_STRING("HTTPC_METHOD_POST");
		else
			DEF_LOG_STRING("HTTPC_METHOD_GET");

		DEF_LOG_RESULT(httpcOpenContext, false, result);
		goto nintendo_api_failed;
	}

	result = httpcSetSSLOpt(httpc_context, SSLCOPT_DisableVerify);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(httpcSetSSLOpt, false, result);
		goto nintendo_api_failed;
	}

	result = httpcSetKeepAlive(httpc_context, HTTPC_KEEPALIVE_ENABLED);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(httpcSetKeepAlive, false, result);
		goto nintendo_api_failed;
	}

	result = httpcAddRequestHeaderField(httpc_context, "Connection", "Keep-Alive");
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_STRING("Connection");
		DEF_LOG_RESULT(httpcAddRequestHeaderField, false, result);
		goto nintendo_api_failed;
	}

	result = httpcAddRequestHeaderField(httpc_context, "User-Agent", (ua ? ua : util_httpc_default_user_agent));
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_STRING("User-Agent");
		DEF_LOG_RESULT(httpcAddRequestHeaderField, false, result);
		goto nintendo_api_failed;
	}

	if(method == HTTPC_METHOD_POST)
	{
		result = httpcAddPostDataRaw(httpc_context, (const uint32_t*)post_data, post_data_size);
		if (result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(httpcAddPostDataRaw, false, result);
			goto nintendo_api_failed;
		}
	}

	result = httpcBeginRequest(httpc_context);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(httpcBeginRequest, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	nintendo_api_failed:
	return result;
}

static uint32_t Util_httpc_get_request(httpcContext* httpc_context, const char* url, const char* ua)
{
	return Util_httpc_request(httpc_context, url, ua, HTTPC_METHOD_GET, NULL, 0);
}

static uint32_t Util_httpc_post_request(httpcContext* httpc_context, const char* url, const char* ua, const uint8_t* post_data, uint32_t post_data_size)
{
	return Util_httpc_request(httpc_context, url, ua, HTTPC_METHOD_POST, post_data, post_data_size);
}

static void Util_httpc_get_response(httpcContext* httpc_context, uint16_t* status_code, Str_data* new_url, bool* redirected)
{
	uint32_t result = DEF_ERR_OTHER;
	char moved_url[4096] = { 0, };

	*redirected = false;

	if(status_code)
	{
		uint32_t out = 0;

		result = httpcGetResponseStatusCode(httpc_context, &out);
		if(result == DEF_SUCCESS)
			*status_code = (uint16_t)out;
		else
		{
			DEF_LOG_RESULT(httpcGetResponseStatusCode, false, result);
			*status_code = 0;
		}
	}

	result = httpcGetResponseHeader(httpc_context, "location", moved_url, 0x1000);
	if (result == DEF_SUCCESS)
	{
		result = Util_str_set(new_url, moved_url);
		if(result == DEF_SUCCESS)
			*redirected = true;
		else
			DEF_LOG_RESULT(Util_str_set, false, result);
	}
	else
	{
		result = httpcGetResponseHeader(httpc_context, "Location", moved_url, 0x1000);
		if (result == DEF_SUCCESS)
		{
			result = Util_str_set(new_url, moved_url);
			if(result == DEF_SUCCESS)
				*redirected = true;
			else
				DEF_LOG_RESULT(Util_str_set, false, result);
		}
	}
}

static uint32_t Util_httpc_download_data(httpcContext* httpc_context, uint8_t** data, uint32_t max_size, uint32_t* downloaded_size)
{
	uint8_t* new_buffer = NULL;
	uint32_t dl_size = 0;
	uint32_t remain_buffer_size = 0;
	uint32_t buffer_offset = 0;
	uint32_t buffer_size = 0;
	uint32_t result = DEF_ERR_OTHER;

	buffer_size = ((max_size > 0x40000) ? 0x40000 : max_size);
	*data = (uint8_t*)malloc(buffer_size);
	if (!*data)
		goto out_of_memory;

	if(downloaded_size)
		*downloaded_size = 0;

	remain_buffer_size = buffer_size;
	memset(*data, 0x0, remain_buffer_size);

	while(true)
	{
		result = httpcDownloadData(httpc_context, (*data) + buffer_offset, remain_buffer_size, &dl_size);
		if(downloaded_size)
			*downloaded_size += dl_size;

		buffer_offset += dl_size;
		if (result != DEF_SUCCESS)
		{
			if(result == HTTPC_RESULTCODE_DOWNLOADPENDING)
			{
				if(buffer_size >= max_size)
					goto out_of_memory;

				buffer_size = ((max_size > (buffer_size + 0x40000)) ? (buffer_size + 0x40000) : max_size);
				new_buffer = (uint8_t*)realloc(*data, buffer_size);
				remain_buffer_size = buffer_size - buffer_offset;
				if(!new_buffer)
					goto out_of_memory;

				*data = new_buffer;
				memset((*data) + buffer_offset, 0x0, remain_buffer_size);
			}
			else
			{
				DEF_LOG_RESULT(httpcDownloadData, false, result);
				goto nintendo_api_failed;
			}
		}
		else
			break;
	}

	return DEF_SUCCESS;

	out_of_memory:
	free(*data);
	*data = NULL;
	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed:
	free(*data);
	*data = NULL;
	return result;
}

static void Util_httpc_close(httpcContext* httpc_context)
{
	if(httpc_context)
	{
		httpcCloseContext(httpc_context);
		httpc_context->httphandle = 0;
		httpc_context->servhandle = 0;
	}
}

static uint32_t Util_httpc_sv_data_internal(httpcContext* httpc_context, uint32_t buffer_size, uint32_t* downloaded_size, const char* dir_path, const char* file_name)
{
	bool first = true;
	uint8_t* cache = NULL;
	uint32_t dl_size = 0;
	uint32_t result = DEF_ERR_OTHER;

	cache = (uint8_t*)malloc(buffer_size);
	if (!cache)
		goto out_of_memory;

	if(downloaded_size)
		*downloaded_size = 0;

	while(true)
	{
		result = httpcDownloadData(httpc_context, cache, buffer_size, &dl_size);
		if(downloaded_size)
			*downloaded_size += dl_size;

		if (result != DEF_SUCCESS)
		{
			if(result == HTTPC_RESULTCODE_DOWNLOADPENDING)
			{
				result = Util_file_save_to_file(file_name, dir_path, cache, dl_size, first);
				first = false;
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_file_save_to_file, false, result);
					goto api_failed;
				}
			}
			else
			{
				Util_file_delete_file(file_name, dir_path);

				DEF_LOG_RESULT(httpcDownloadData, false, result);
				goto nintendo_api_failed;
			}
		}
		else
		{
			if(dl_size > 0)
			{
				result = Util_file_save_to_file(file_name, dir_path, cache, dl_size, first);
				first = false;
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_file_save_to_file, false, result);
					goto api_failed;
				}
			}
			break;
		}
	}

	free(cache);
	cache = NULL;
	return DEF_SUCCESS;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed:
	api_failed:
	free(cache);
	cache = NULL;
	return result;
}
#endif //DEF_HTTPC_API_ENABLE
