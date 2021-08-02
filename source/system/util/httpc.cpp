#include "headers.hpp"

Result_with_string Util_httpc_dl_data(std::string url, u8* data_buffer, int buffer_size, u32* downloaded_data_size, u32* status_code, bool follow_redirect,
int max_redirect)
{
	std::string last_url = "";
	return Util_httpc_dl_data(url, data_buffer, buffer_size, downloaded_data_size, status_code, follow_redirect, max_redirect, &last_url, "", "");
}

Result_with_string Util_httpc_dl_data(std::string url, u8* data_buffer, int buffer_size, u32* downloaded_data_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string dir_path, std::string file_path)
{
	std::string last_url = "";
	return Util_httpc_dl_data(url, data_buffer, buffer_size, downloaded_data_size, status_code, follow_redirect, max_redirect, &last_url, dir_path, file_path);
}

Result_with_string Util_httpc_dl_data(std::string url, u8* data_buffer, int buffer_size, u32* downloaded_data_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string* last_url)
{
	return Util_httpc_dl_data(url, data_buffer, buffer_size, downloaded_data_size, status_code, follow_redirect, max_redirect, last_url, "", "");
}

Result_with_string Util_httpc_dl_data(std::string url, u8* data_buffer, int buffer_size, u32* downloaded_data_size, u32* status_code, bool follow_redirect,
int max_redirect, std::string* last_url, std::string dir_path, std::string file_path)
{
	bool redirect = false;
	bool function_fail = false;
	int redirected = 0;
	u32 dl_size = 0;
	char* moved_url;
	httpcContext httpc_context;
	Result_with_string result;
	Result_with_string fs_result;
	*last_url = url;
	*downloaded_data_size = 0;

	for(int i = 0; i < 40; i++)
	{
		result.code = acWaitInternetConnection();
		if(result.code != 0xE0A09D2E)
			break;
		
		usleep(100000);
	}
	
	if(result.code != 0)
	{
		result.string = "acWaitInternetConnection() failed. ";
		return result;
	}
	
	moved_url = (char*)malloc(0x1000);
	if (moved_url == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	while (true)
	{
		redirect = false;

		if (!function_fail)
		{
			result.code = httpcOpenContext(&httpc_context, HTTPC_METHOD_GET, url.c_str(), 0);
			if (result.code != 0)
			{
				result.error_description = "This'll occur in the case the wrong URL was specified.\nPlease check the URL.";
				result.string = "[Error] httpcOpenContext failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
		{
			result.code = httpcSetSSLOpt(&httpc_context, SSLCOPT_DisableVerify);
			if (result.code != 0)
			{
				result.error_description = "N/A";
				result.string = "[Error] httpcSetSSLOpt failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
		{
			result.code = httpcSetKeepAlive(&httpc_context, HTTPC_KEEPALIVE_ENABLED);
			if (result.code != 0)
			{
				result.error_description = "N/A";
				result.string = "[Error] httpcSetKeepAlive failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
		{
			httpcAddRequestHeaderField(&httpc_context, "Connection", "Keep-Alive");
			httpcAddRequestHeaderField(&httpc_context, "User-Agent", (DEF_HTTP_USER_AGENT).c_str());
			result.code = httpcBeginRequest(&httpc_context);
			if (result.code != 0)
			{
				result.error_description = "N/A";
				result.string = "[Error] httpcBeginRequest failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
			httpcGetResponseStatusCode(&httpc_context, status_code);

		if (!function_fail && follow_redirect && max_redirect > redirected)
		{
			result.code = httpcGetResponseHeader(&httpc_context, "location", moved_url, 0x1000);
			if (result.code == 0)
			{
				*last_url = moved_url;
				url = moved_url;
				redirect = true;
			}
			else
			{
				result.code = httpcGetResponseHeader(&httpc_context, "Location", moved_url, 0x1000);
				if (result.code == 0)
				{
					*last_url = moved_url;
					url = moved_url;
					redirect = true;
				}
			}
			result.code = 0;
		}

		if (!function_fail && !redirect)
		{
			while(true)
			{
				result.code = httpcDownloadData(&httpc_context, data_buffer, buffer_size, &dl_size);
				*downloaded_data_size += dl_size;
				if (result.code != 0)
				{
					if(dir_path != "" && file_path != "" && result.code == 0xD840A02B)
					{
						fs_result = Util_file_save_to_file(file_path, dir_path, data_buffer, (int)dl_size, false);
						if(fs_result.code != 0)
						{
							function_fail = true;
							result = fs_result;
							break;
						}
					}
					else
					{
						if(dir_path != "" && file_path != "")
							Util_file_delete_file(file_path, dir_path);
						
						result.error_description = "It may occur in case of wrong internet connection.\nPlease check internet connection.";
						result.string = "[Error] httpcDownloadData failed. ";
						function_fail = true;
						break;
					}
				}
				else
				{
					if(dir_path != "" && file_path != "")
					{
						fs_result = Util_file_save_to_file(file_path, dir_path, data_buffer, (int)dl_size, false);
						if(fs_result.code != 0)
						{
							function_fail = true;
							result = fs_result;
						}
					}
					break;
				}
			}
		}

		httpcCloseContext(&httpc_context);

		if (function_fail || !redirect)
			break;
		else
			redirected++;
	}
	free(moved_url);
	moved_url = NULL;

	return result;
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, char* post_data_buffer, int post_buffer_size, u8* dl_data_buffer, int dl_buffer_size,
u32* downloaded_data_size, u32* status_code, bool follow_redirect, int max_redirect)
{
	std::string last_url = "";
	return Util_httpc_post_and_dl_data(url, post_data_buffer, post_buffer_size, dl_data_buffer, dl_buffer_size, downloaded_data_size, status_code, follow_redirect, max_redirect, &last_url, "", "");
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, char* post_data_buffer, int post_buffer_size, u8* dl_data_buffer, int dl_buffer_size,
u32* downloaded_data_size, u32* status_code, bool follow_redirect, int max_redirect, std::string dir_path, std::string file_path)
{
	std::string last_url = "";
	return Util_httpc_post_and_dl_data(url, post_data_buffer, post_buffer_size, dl_data_buffer, dl_buffer_size, downloaded_data_size, status_code, follow_redirect, max_redirect, &last_url, dir_path, file_path);
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, char* post_data_buffer, int post_buffer_size, u8* dl_data_buffer, int dl_buffer_size,
u32* downloaded_data_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url)
{
	return Util_httpc_post_and_dl_data(url, post_data_buffer, post_buffer_size, dl_data_buffer, dl_buffer_size, downloaded_data_size, status_code, follow_redirect, max_redirect, last_url, "", "");
}

Result_with_string Util_httpc_post_and_dl_data(std::string url, char* post_data_buffer, int post_buffer_size, u8* dl_data_buffer, int dl_buffer_size,
u32* downloaded_data_size, u32* status_code, bool follow_redirect, int max_redirect, std::string* last_url, std::string dir_path, std::string file_path)
{
	bool redirect = false;
	bool function_fail = false;
	bool post = true;
	int redirected = 0;
	u32 dl_size = 0;
	char* moved_url;
	httpcContext httpc_context;
	Result_with_string result;
	Result_with_string fs_result;

	for(int i = 0; i < 40; i++)
	{
		result.code = acWaitInternetConnection();
		if(result.code != 0xE0A09D2E)
			break;
		
		usleep(100000);
	}
	
	if(result.code != 0)
	{
		result.string = "acWaitInternetConnection() failed. ";
		return result;
	}
	
	moved_url = (char*)malloc(0x1000);
	if (moved_url == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	while (true)
	{
		redirect = false;

		if (!function_fail)
		{
			if (post)
				result.code = httpcOpenContext(&httpc_context, HTTPC_METHOD_POST, url.c_str(), 0);
			else
				result.code = httpcOpenContext(&httpc_context, HTTPC_METHOD_GET, url.c_str(), 0);

			if (result.code != 0)
			{
				result.error_description = "This'll occur in the case the wrong URL was specified.\nPlease check the URL.";
				result.string = "[Error] httpcOpenContext failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
		{
			result.code = httpcSetSSLOpt(&httpc_context, SSLCOPT_DisableVerify);
			if (result.code != 0)
			{
				result.error_description = "N/A";
				result.string = "[Error] httpcSetSSLOpt failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
		{
			result.code = httpcSetKeepAlive(&httpc_context, HTTPC_KEEPALIVE_ENABLED);
			if (result.code != 0)
			{
				result.error_description = "N/A";
				result.string = "[Error] httpcSetKeepAlive failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
		{
			httpcAddRequestHeaderField(&httpc_context, "Connection", "Keep-Alive");
			httpcAddRequestHeaderField(&httpc_context, "User-Agent", (DEF_HTTP_USER_AGENT).c_str());
			if (post)
			{
				httpcAddPostDataRaw(&httpc_context, (u32*)post_data_buffer, post_buffer_size);
				post = false;
			}
			result.code = httpcBeginRequest(&httpc_context);
			if (result.code != 0)
			{
				result.error_description = "N/A";
				result.string = "[Error] httpcBeginRequest failed. ";
				function_fail = true;
			}
		}

		if (!function_fail)
			result.code = httpcGetResponseStatusCode(&httpc_context, status_code);

		if (!function_fail && follow_redirect && max_redirect > redirected)
		{
			result.code = httpcGetResponseHeader(&httpc_context, "location", moved_url, 0x1000);
			if (result.code == 0)
			{
				*last_url = moved_url;
				url = moved_url;
				redirect = true;
			}
			else
			{
				result.code = httpcGetResponseHeader(&httpc_context, "Location", moved_url, 0x1000);
				if (result.code == 0)
				{
					*last_url = moved_url;
					url = moved_url;
					redirect = true;
				}
			}
			result.code = 0;
		}

		if (!function_fail && !redirect)
		{
			while(true)
			{
				result.code = httpcDownloadData(&httpc_context, dl_data_buffer, dl_buffer_size, &dl_size);
				*downloaded_data_size += dl_size;
				if (result.code != 0)
				{
					if(dir_path != "" && file_path != "" && result.code == 0xD840A02B)
					{
						fs_result = Util_file_save_to_file(file_path, dir_path, dl_data_buffer, (int)dl_size, false);
						if(fs_result.code != 0)
						{
							function_fail = true;
							result = fs_result;
							break;
						}
					}
					else
					{
						if(dir_path != "" && file_path != "")
							Util_file_delete_file(file_path, dir_path);
						
						result.error_description = "It may occur in case of wrong internet connection.\nPlease check internet connection.";
						result.string = "[Error] httpcDownloadData failed. ";
						function_fail = true;
						break;
					}
				}
				else
				{
					if(dir_path != "" && file_path != "")
					{
						fs_result = Util_file_save_to_file(file_path, dir_path, dl_data_buffer, (int)dl_size, false);
						if(fs_result.code != 0)
						{
							function_fail = true;
							result = fs_result;
						}
					}
					break;
				}
			}
		}

		httpcCloseContext(&httpc_context);

		if (function_fail || !redirect)
			break;
		else
			redirected++;
	}
	free(moved_url);
	moved_url = NULL;

	return result;
}
