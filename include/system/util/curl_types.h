#if !defined(DEF_CURL_TYPES_H)
#define DEF_CURL_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_CURL_API_ENABLE				/*(bool)(*/true/*)*/							//Enable curl API. This will use curl functions.
																						//This API supports up to TLS 1.3.
#define DEF_CURL_CERT_PATH				(const char*)("romfs:/gfx/cert/cacert.pem")		//Certification file path.

#endif //!defined(DEF_CURL_TYPES_H)
