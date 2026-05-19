#if !defined(DEF_FTPD_H)
#define DEF_FTPD_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/hid_types.h"

#define DEF_FTPD_ENABLE

//#define DEF_FTPD_ENABLE_ICON
#define DEF_FTPD_ENABLE_NAME
#define DEF_FTPD_ICON_PATH		/*(const char*)(*/"romfs:/"/*)*/
#define DEF_FTPD_NAME			/*(const char*)(*/"FTP\nserver"/*)*/
#define DEF_FTPD_VER			/*(const char*)(*/"v1.0.0"/*)*/

bool Ftpd_query_init_flag(void);

bool Ftpd_query_running_flag(void);

void Ftpd_hid(Hid_info* key);

void Ftpd_resume(void);

void Ftpd_suspend(void);

uint32_t Ftpd_load_msg(const char* lang);

void Ftpd_init(bool draw);

void Ftpd_exit(bool draw);

void Ftpd_main(void);

#endif //!defined(DEF_FTPD_H)
