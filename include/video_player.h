#if !defined(DEF_VIDEO_PLAYER_HPP)
#define DEF_VIDEO_PLAYER_HPP
#include <stdbool.h>
#include <stdint.h>
#include "system/util/hid_types.h"

#define DEF_VID_ENABLE

#define DEF_VID_ENABLE_ICON
//#define DEF_VID_ENABLE_NAME
#define DEF_VID_ICON_PATH				/*(const char*)(*/"romfs:/gfx/draw/icon/vid_icon.t3x"/*)*/
#define DEF_VID_NAME					/*(const char*)(*/"Video\nplayer"/*)*/
#define DEF_VID_VER						/*(const char*)(*/"v1.6.1"/*)*/
#define DEF_VID_SPEAKER_SESSION_ID		(uint8_t)(0)
#define DEF_VID_DECORDER_SESSION_ID		(uint8_t)(0)

bool Vid_query_init_flag(void);

bool Vid_query_running_flag(void);

void Vid_hid(const Hid_info* key);

void Vid_resume(void);

void Vid_suspend(void);

uint32_t Vid_load_msg(const char* lang);

void Vid_init(bool draw);

void Vid_exit(bool draw);

void Vid_main(void);

#endif //!defined(DEF_VIDEO_PLAYER_HPP)
