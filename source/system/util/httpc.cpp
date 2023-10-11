#include "definitions.hpp"

#if DEF_ENABLE_HTTPC_API
#include "system/types.hpp"

#include "system/util/file.hpp"
#include "system/util/util.hpp"

//Include myself.
#include "system/util/httpc.hpp"

bool util_httpc_init = false;


static Result_with_string Util_httpc_request(httpcContext* httpc_context, std::string&& url, HTTPC_RequestMethod method, u8* post_data, int post_data_size);
static Result_with_string Util_httpc_get_request(httpcContext* httpc_context, std::string&& url);
static Result_with_string Util_httpc_post_request(httpcContext* httpc_context, std::string&& url, u8* post_data, int post_data_size);
static void Util_httpc_get_response(httpcContext* httpc_context, u32* status_code, std::string* new_url, bool* redirected);
static Result_with_string Util_httpc_download_data(httpcContext* httpc_context, u8** data, int max_size, u32* downloaded_size);
static void Util_httpc_close(httpcContext* httpc_context);
static Result_with_string Util_httpc_dl_data_internal(std::string&& url, u8** data, int max_size, u32* downloaded_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url);
static Result_with_string Util_httpc_save_data_internal(std::string&& url, int buffer_size, u32* downloaded_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url, std::string&& dir_path, std::string&& file_name);
static Result_with_string Util_httpc_post_and_dl_data_internal(std::string&& url, u8* post_data, int post_size, u8** dl_data, int max_dl_size, u32* downloaded_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url);
static Result_with_string Util_httpc_post_and_save_data_internal(std::string&& url, u8* post_data, int post_size, int buffer_size, u32* downloaded_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url, std::string&& dir_path, std::string&& file_name);
static Result_with_string Util_httpc_save_data(httpcContext* httpc_context, int buffer_size, u32* downloaded_size, std::string&& dir_path, std::string&& file_name);


Result_with_string Util_httpc_init(int buffer_size)
{
	Result_with_string result;
	if(util_httpc_init)
		goto already_inited;

	if(buffer_size < 0)
		goto invalid_arg;

	result.code = httpcInit(buffer_size);
	if(result.code != 0)
	{
		result.error_description = "[Error] httpcInit() failed. ";
		goto nintendo_api_failed;
	}

	util_httpc_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_httpc_exit(void)
{
	if(!util_httpc_init)
		return;

	util_httpc_init = false;
	httpcExit();
}

Result_with_string Util_httpc_dl_data(std::string url, u8** data, int max_size, u32* downloaded_size, bool follow_redirect,
int max_redirect)
{
	std::string last_url = "";
	u32 status_code = 0;
	return Util_httpc_dl_data_internal(std::move(url), data, max_size, downloaded_size, &status_code, follow_redirect, max_redirect, &last_url);
}

Result_with_string Util_httpc_dl_data(std::string url, u8** data, int max_size, u32* downloaded_size, bool follow_redirect,
int max_redirect, std::string* last_url)
{
	u32 status_code = 0;
	return Util_httpc_dl_data_internal(std::move(url), data, max_size, downloaded_size, &status_code, follow_redirect, max_redirect, last_url);
}

Result_with_string Util_httpc_dl_data(std::string url, u8** data, int max_size, u32* downloaded_size, u32* status_code, bool follow_redirect,
int max_redirect)
{
	std::string last_url = "";
	return Util_httpc_dl_data_internal(std::move(url), data, max_size, downloaded_size, status_code, follow_redirect, max_redirect, &last_url);
}

Result_with_string Util_httpc_dl_data(std::string url, u8** data, int max_size, u32* downloaded_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string* last_url)
{
	return Util_httpc_dl_data_internal(std::move(url), data, max_size, downloaded_size, status_code, follow_redirect, max_redirect, last_url);
}

Result_with_string Util_httpc_save_data(std::string url, int buffer_size, u32* downloaded_size, bool follow_redirect,
int max_redirect, std::string dir_path, std::string file_name)
{
	std::string last_url = "";
	u32 status_code = 0;
	return Util_httpc_save_data_internal(std::move(url), buffer_size, downloaded_size, &status_code, follow_redirect, max_redirect,
	&last_url, std::move(dir_path), std::move(file_name));
}

Result_with_string Util_httpc_save_data(std::string url, int buffer_size, u32* downloaded_size, bool follow_redirect,
int max_redirect, std::string* last_url, std::string dir_path, std::string file_name)
{
	u32 status_code = 0;
	return Util_httpc_save_data_internal(std::move(url), buffer_size, downloaded_size, &status_code, follow_redirect, max_redirect,
	last_url, std::move(dir_path), std::move(file_name));
}

Result_with_string Util_httpc_save_data(std::string url, int buffer_size, u32* downloaded_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string dir_path, std::string file_name)
{
	std::string last_url = "";
	return Util_httpc_save_data_internal(std::move(url), buffer_size, downloaded_size, status_code, follow_redirect, max_redirect,
	&last_url, std::move(dir_path), std::move(file_name));
}

Result_with_string Util_httpc_save_data(std::string url, int buffer_size, u32* downloaded_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string* last_url, std::string dir_path, std::string file_name)
{
	return Util_httpc_save_data_internal(std::move(url), buffer_size, downloaded_size, status_code, follow_redirect, max_redirect,
	last_url, std::move(dir_path), std::move(file_name));
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, u8* post_data, int post_size, u8** dl_data, int max_dl_size,
u32* downloaded_size, bool follow_redirect, int max_redirect)
{
	std::string last_url = "";
	u32 status_code = 0;
	return Util_httpc_post_and_dl_data_internal(std::move(url), post_data, post_size, dl_data, max_dl_size, downloaded_size,
	&status_code, follow_redirect, max_redirect, &last_url);
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, u8* post_data, int post_size, u8** dl_data, int max_dl_size,
u32* downloaded_size, bool follow_redirect, int max_redirect, std::string* last_url)
{
	u32 status_code = 0;
	return Util_httpc_post_and_dl_data_internal(std::move(url), post_data, post_size, dl_data, max_dl_size, downloaded_size,
	&status_code, follow_redirect, max_redirect, last_url);
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, u8* post_data, int post_size, u8** dl_data, int max_dl_size,
u32* downloaded_size, u32* status_code, bool follow_redirect, int max_redirect)
{
	std::string last_url = "";
	return Util_httpc_post_and_dl_data_internal(std::move(url), post_data, post_size, dl_data, max_dl_size, downloaded_size,
	status_code, follow_redirect, max_redirect, &last_url);
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, u8* post_data, int post_size, u8** dl_data, int max_dl_size,
u32* downloaded_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url)
{
	return Util_httpc_post_and_dl_data_internal(std::move(url), post_data, post_size, dl_data, max_dl_size, downloaded_size,
	status_code, follow_redirect, max_redirect, last_url);
}

Result_with_string Util_httpc_post_and_save_data(std::string url, u8* post_data, int post_size, int buffer_size, u32* downloaded_size,
bool follow_redirect, int max_redirect, std::string dir_path, std::string file_name)
{
	std::string last_url = "";
	u32 status_code = 0;
	return Util_httpc_post_and_save_data_internal(std::move(url), post_data, post_size, buffer_size, downloaded_size,
	&status_code, follow_redirect, max_redirect, &last_url, std::move(dir_path), std::move(file_name));
}

Result_with_string Util_httpc_post_and_save_data(std::string url, u8* post_data, int post_size, int buffer_size, u32* downloaded_size,
bool follow_redirect, int max_redirect, std::string* last_url, std::string dir_path, std::string file_name)
{
	u32 status_code = 0;
	return Util_httpc_post_and_save_data_internal(std::move(url), post_data, post_size, buffer_size, downloaded_size,
	&status_code, follow_redirect, max_redirect, last_url, std::move(dir_path), std::move(file_name));
}

Result_with_string Util_httpc_post_and_save_data(std::string url, u8* post_data, int post_size, int buffer_size, u32* downloaded_size,
u32* status_code, bool follow_redirect, int max_redirect, std::string dir_path, std::string file_name)
{
	std::string last_url = "";
	return Util_httpc_post_and_save_data_internal(std::move(url), post_data, post_size, buffer_size, downloaded_size,
	status_code, follow_redirect, max_redirect, &last_url, std::move(dir_path), std::move(file_name));
}

Result_with_string Util_httpc_post_and_save_data(std::string url, u8* post_data, int post_size, int buffer_size, u32* downloaded_size,
u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url, std::string dir_path, std::string file_name)
{
	return Util_httpc_post_and_save_data_internal(std::move(url), post_data, post_size, buffer_size, downloaded_size,
	status_code, follow_redirect, max_redirect, last_url, std::move(dir_path), std::move(file_name));
}

static Result_with_string Util_httpc_request(httpcContext* httpc_context, std::string&& url, HTTPC_RequestMethod method, u8* post_data, int post_data_size)
{
	Result_with_string result;

	if(method == HTTPC_METHOD_POST)
		result.code = httpcOpenContext(httpc_context, HTTPC_METHOD_POST, url.c_str(), 0);
	else
		result.code = httpcOpenContext(httpc_context, HTTPC_METHOD_GET, url.c_str(), 0);

	if (result.code != 0)
	{
		result.error_description = "[Error] httpcOpenContext() failed. ";
		goto nintendo_api_failed;
	}

	result.code = httpcSetSSLOpt(httpc_context, SSLCOPT_DisableVerify);
	if (result.code != 0)
	{
		result.error_description = "[Error] httpcSetSSLOpt() failed. ";
		goto nintendo_api_failed;
	}

	result.code = httpcSetKeepAlive(httpc_context, HTTPC_KEEPALIVE_ENABLED);
	if (result.code != 0)
	{
		result.error_description = "[Error] httpcSetKeepAlive() failed. ";
		goto nintendo_api_failed;
	}

	result.code = httpcAddRequestHeaderField(httpc_context, "Connection", "Keep-Alive");
	if (result.code != 0)
	{
		result.error_description = "[Error] httpcAddRequestHeaderField() failed. ";
		goto nintendo_api_failed;
	}

	result.code = httpcAddRequestHeaderField(httpc_context, "User-Agent", (DEF_HTTP_USER_AGENT).c_str());
	if (result.code != 0)
	{
		result.error_description = "[Error] httpcAddRequestHeaderField() failed. ";
		goto nintendo_api_failed;
	}

	if(method == HTTPC_METHOD_POST)
	{
		result.code = httpcAddPostDataRaw(httpc_context, (u32*)post_data, post_data_size);
		if (result.code != 0)
		{
			result.error_description = "[Error] httpcAddPostDataRaw() failed. ";
			goto nintendo_api_failed;
		}
	}

	result.code = httpcBeginRequest(httpc_context);
	if (result.code != 0)
	{
		result.error_description = "[Error] httpcBeginRequest() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	nintendo_api_failed:
	Util_httpc_close(httpc_context);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

static Result_with_string Util_httpc_get_request(httpcContext* httpc_context, std::string&& url)
{
	return Util_httpc_request(httpc_context, std::move(url), HTTPC_METHOD_GET, NULL, 0);
}

static Result_with_string Util_httpc_post_request(httpcContext* httpc_context, std::string&& url, u8* post_data, int post_data_size)
{
	return Util_httpc_request(httpc_context, std::move(url), HTTPC_METHOD_POST, post_data, post_data_size);
}

static void Util_httpc_get_response(httpcContext* httpc_context, u32* status_code, std::string* new_url, bool* redirected)
{
	char moved_url[4096];
	Result_with_string result;

	memset(moved_url, 0x0, 4096);
	*new_url = "";
	*redirected = false;
	*status_code = 0;

	result.code = httpcGetResponseStatusCode(httpc_context, status_code);
	if(result.code != 0)
		*status_code = 0;

	result.code = httpcGetResponseHeader(httpc_context, "location", moved_url, 0x1000);
	if (result.code == 0)
	{
		*new_url = moved_url;
		*redirected = true;
	}
	else
	{
		result.code = httpcGetResponseHeader(httpc_context, "Location", moved_url, 0x1000);
		if (result.code == 0)
		{
			*new_url = moved_url;
			*redirected = true;
		}
	}
}

static Result_with_string Util_httpc_download_data(httpcContext* httpc_context, u8** data, int max_size, u32* downloaded_size)
{
	u8* new_buffer = NULL;
	u32 dl_size = 0;
	int remain_buffer_size = 0;
	int buffer_offset = 0;
	int buffer_size = 0;
	Result_with_string result;

	buffer_size = max_size > 0x40000 ? 0x40000 : max_size;
	Util_safe_linear_free(*data);
	*data = (u8*)Util_safe_linear_alloc(buffer_size);
	if (!*data)
		goto out_of_memory;

	remain_buffer_size = buffer_size;
	memset(*data, 0x0, remain_buffer_size);

	while(true)
	{
		result.code = httpcDownloadData(httpc_context, (*data) + buffer_offset, remain_buffer_size, &dl_size);
		*downloaded_size += dl_size;
		buffer_offset += dl_size;
		if (result.code != 0)
		{
			if(result.code == HTTPC_RESULTCODE_DOWNLOADPENDING)
			{
				if(buffer_size >= max_size)
					goto out_of_memory;

				buffer_size = max_size > buffer_size + 0x40000 ? buffer_size + 0x40000 : max_size;
				new_buffer = (u8*)Util_safe_linear_realloc(*data, buffer_size);
				remain_buffer_size = buffer_size - buffer_offset;
				if(!new_buffer)
					goto out_of_memory;

				*data = new_buffer;
				memset((*data) + buffer_offset, 0x0, remain_buffer_size);
			}
			else
			{
				result.error_description = "[Error] httpcDownloadData() failed. ";
				goto nintendo_api_failed;
			}
		}
		else
			break;
	}

	return result;

	out_of_memory:
	Util_safe_linear_free(*data);
	*data = NULL;
	Util_httpc_close(httpc_context);
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	Util_safe_linear_free(*data);
	*data = NULL;
	Util_httpc_close(httpc_context);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

static void Util_httpc_close(httpcContext* httpc_context)
{
	if(httpc_context)
		httpcCloseContext(httpc_context);

	httpc_context = NULL;
}

static Result_with_string Util_httpc_dl_data_internal(std::string&& url, u8** data, int max_size, u32* downloaded_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string* last_url)
{
	bool redirect = false;
	int redirected = 0;
	std::string new_url = "";
	httpcContext httpc_context;
	Result_with_string result;

	if(!util_httpc_init)
		goto not_inited;

	if(url == "" || !data || max_size <= 0 || !downloaded_size || !status_code || (follow_redirect && max_redirect < 0) || !last_url)
		goto invalid_arg;

	*last_url = url;
	*downloaded_size = 0;

	for(int i = 0; i < 40; i++)
	{
		result.code = acWaitInternetConnection();
		if(result.code != 0xE0A09D2E)
			break;

		Util_sleep(100000);
	}

	if(result.code != 0)
	{
		result.error_description = "[Error] acWaitInternetConnection() failed. ";
		goto nintendo_api_failed;
	}

	while (true)
	{
		redirect = false;

		result = Util_httpc_get_request(&httpc_context, std::move(url));
		if(result.code != 0)
			goto api_failed;

		Util_httpc_get_response(&httpc_context, status_code, &new_url, &redirect);

		if (redirect && follow_redirect && max_redirect > redirected)
		{
			url = new_url;
			*last_url = url;
			redirected++;
		}
		else
			redirect = false;

		if (!redirect)
		{
			result = Util_httpc_download_data(&httpc_context, data, max_size, downloaded_size);
			if(result.code != 0)
				goto api_failed;
		}

		Util_httpc_close(&httpc_context);

		if (!redirect)
			break;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	api_failed:
	return result;
}

static Result_with_string Util_httpc_save_data_internal(std::string&& url, int buffer_size, u32* downloaded_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string* last_url, std::string&& dir_path, std::string&& file_name)
{
	bool redirect = false;
	int redirected = 0;
	std::string new_url = "";
	httpcContext httpc_context;
	Result_with_string result;

	if(!util_httpc_init)
		goto not_inited;

	if(url == "" || buffer_size <= 0 || !downloaded_size || !status_code || (follow_redirect && max_redirect < 0) || !last_url)
		goto invalid_arg;

	*last_url = url;
	*downloaded_size = 0;

	for(int i = 0; i < 40; i++)
	{
		result.code = acWaitInternetConnection();
		if(result.code != 0xE0A09D2E)
			break;

		Util_sleep(100000);
	}

	if(result.code != 0)
	{
		result.error_description = "[Error] acWaitInternetConnection() failed. ";
		goto nintendo_api_failed;
	}

	while (true)
	{
		redirect = false;

		result = Util_httpc_get_request(&httpc_context, std::move(url));
		if(result.code != 0)
			goto api_failed;

		Util_httpc_get_response(&httpc_context, status_code, &new_url, &redirect);

		if (redirect && follow_redirect && max_redirect > redirected)
		{
			url = new_url;
			*last_url = url;
			redirected++;
		}
		else
			redirect = false;

		if (!redirect)
		{
			result = Util_httpc_save_data(&httpc_context, buffer_size, downloaded_size, std::move(dir_path), std::move(file_name));
			if(result.code != 0)
				goto api_failed;
		}

		Util_httpc_close(&httpc_context);

		if (!redirect)
			break;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	api_failed:
	return result;
}

static Result_with_string Util_httpc_post_and_dl_data_internal(std::string&& url, u8* post_data, int post_size, u8** dl_data, int max_dl_size,
u32* downloaded_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url)
{
	bool post = true;
	bool redirect = false;
	int redirected = 0;
	std::string new_url = "";
	httpcContext httpc_context;
	Result_with_string result;

	if(!util_httpc_init)
		goto not_inited;

	if(url == "" || !post_data || post_size <= 0 || !dl_data || max_dl_size <= 0 || !downloaded_size || !status_code
	|| (follow_redirect && max_redirect < 0) || !last_url)
		goto invalid_arg;

	*last_url = url;
	*downloaded_size = 0;

	for(int i = 0; i < 40; i++)
	{
		result.code = acWaitInternetConnection();
		if(result.code != 0xE0A09D2E)
			break;

		Util_sleep(100000);
	}

	if(result.code != 0)
	{
		result.error_description = "[Error] acWaitInternetConnection() failed. ";
		goto nintendo_api_failed;
	}

	while (true)
	{
		redirect = false;

		if(post)
		{
			result = Util_httpc_post_request(&httpc_context, std::move(url), post_data, post_size);
			post = false;
		}
		else
			result = Util_httpc_get_request(&httpc_context, std::move(url));

		if(result.code != 0)
			goto api_failed;

		Util_httpc_get_response(&httpc_context, status_code, &new_url, &redirect);

		if (redirect && follow_redirect && max_redirect > redirected)
		{
			url = new_url;
			*last_url = url;
			redirected++;
		}
		else
			redirect = false;

		if (!redirect)
		{
			result = Util_httpc_download_data(&httpc_context, dl_data, max_dl_size, downloaded_size);
			if(result.code != 0)
				goto api_failed;
		}

		Util_httpc_close(&httpc_context);

		if (!redirect)
			break;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	api_failed:
	return result;
}

static Result_with_string Util_httpc_post_and_save_data_internal(std::string&& url, u8* post_data, int post_size, int buffer_size, u32* downloaded_size,
u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url, std::string&& dir_path, std::string&& file_name)
{
	bool post = true;
	bool redirect = false;
	int redirected = 0;
	std::string new_url = "";
	httpcContext httpc_context;
	Result_with_string result;

	if(!util_httpc_init)
		goto not_inited;

	if(url == "" || !post_data || post_size <= 0 || buffer_size <= 0 || !downloaded_size || !status_code
	|| (follow_redirect && max_redirect < 0) || !last_url)
		goto invalid_arg;

	*last_url = url;
	*downloaded_size = 0;

	for(int i = 0; i < 40; i++)
	{
		result.code = acWaitInternetConnection();
		if(result.code != 0xE0A09D2E)
			break;

		Util_sleep(100000);
	}

	if(result.code != 0)
	{
		result.error_description = "[Error] acWaitInternetConnection() failed. ";
		goto nintendo_api_failed;
	}

	while (true)
	{
		redirect = false;

		if(post)
		{
			result = Util_httpc_post_request(&httpc_context, std::move(url), post_data, post_size);
			post = false;
		}
		else
			result = Util_httpc_get_request(&httpc_context, std::move(url));

		if(result.code != 0)
			goto api_failed;

		Util_httpc_get_response(&httpc_context, status_code, &new_url, &redirect);

		if (redirect && follow_redirect && max_redirect > redirected)
		{
			url = new_url;
			*last_url = url;
			redirected++;
		}
		else
			redirect = false;

		if (!redirect)
		{
			result = Util_httpc_save_data(&httpc_context, buffer_size, downloaded_size, std::move(dir_path), std::move(file_name));
			if(result.code != 0)
				goto api_failed;
		}

		Util_httpc_close(&httpc_context);

		if (!redirect)
			break;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	api_failed:
	return result;
}

static Result_with_string Util_httpc_save_data(httpcContext* httpc_context, int buffer_size, u32* downloaded_size, std::string&& dir_path, std::string&& file_name)
{
	bool first = true;
	u8* cache = NULL;
	u32 dl_size = 0;
	Result_with_string result;
	Result_with_string fs_result;

	cache = (u8*)Util_safe_linear_alloc(buffer_size);
	if (!cache)
		goto out_of_memory;

	while(true)
	{
		result.code = httpcDownloadData(httpc_context, cache, buffer_size, &dl_size);
		*downloaded_size += dl_size;
		if (result.code != 0)
		{
			if(result.code == HTTPC_RESULTCODE_DOWNLOADPENDING)
			{
				fs_result = Util_file_save_to_file(file_name, dir_path, cache, (int)dl_size, first);
				first = false;
				if(fs_result.code != 0)
				{
					result = fs_result;
					goto api_failed;
				}
			}
			else
			{
				Util_file_delete_file(file_name, dir_path);

				result.error_description = "[Error] httpcDownloadData() failed. ";
				goto nintendo_api_failed;
			}
		}
		else
		{
			if(dl_size > 0)
			{
				fs_result = Util_file_save_to_file(file_name, dir_path, cache, (int)dl_size, first);
				first = false;
				if(fs_result.code != 0)
				{
					result = fs_result;
					goto api_failed;
				}
			}
			break;
		}
	}

	Util_safe_linear_free(cache);
	cache = NULL;
	return result;

	out_of_memory:
	Util_httpc_close(httpc_context);
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	Util_safe_linear_free(cache);
	cache = NULL;
	Util_httpc_close(httpc_context);
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;

	api_failed:
	Util_safe_linear_free(cache);
	cache = NULL;
	return result;
}

#endif
