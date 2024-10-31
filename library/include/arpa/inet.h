#pragma once

#if __GNUC__
//We don't want to see warnings in 3rd party headers.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif //__GNUC__

#include <netinet/in.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	in_addr_t inet_addr(const char *cp);
	int       inet_aton(const char *cp, struct in_addr *inp);
	char*     inet_ntoa(struct in_addr in);

	const char *inet_ntop(int af, const void * src, char * dst, socklen_t size);
	int        inet_pton(int af, const char * src, void * dst);

	uint32_t htonl(uint32_t hostlong);
	uint16_t htons(uint16_t hostshort);
	uint32_t ntohl(uint32_t netlong);
	uint16_t ntohs(uint16_t netshort);

#ifdef __cplusplus
}

#if __GNUC__
#pragma GCC diagnostic pop
#endif //__GNUC__

#endif
