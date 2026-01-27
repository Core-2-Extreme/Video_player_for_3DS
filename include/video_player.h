#if !defined(DEF_VIDEO_PLAYER_HPP)
#define DEF_VIDEO_PLAYER_HPP
#include <stdbool.h>
#include <stdint.h>
#include "system/util/hid_types.h"

#define DEF_VID_ENABLE

#define DEF_VID_NUM_OF_MSG				(uint16_t)(42)
#define DEF_VID_ENABLE_ICON
//#define DEF_VID_ENABLE_NAME
#define DEF_VID_ICON_PATH				/*(const char*)(*/"romfs:/gfx/draw/icon/vid_icon.t3x"/*)*/
#define DEF_VID_NAME					/*(const char*)(*/"Video\nplayer"/*)*/
#define DEF_VID_VER						/*(const char*)(*/"v1.6.1 (nightly build)"/*)*/

#define DEF_VID_TEX_FILTER_MSG						(uint8_t)(0)
#define DEF_VID_CONTROLS_MSG						(uint8_t)(1)
#define DEF_VID_SKIP_FRAME_MSG						(uint8_t)(2)
#define DEF_VID_CONTROL_DESCRIPTION_MSG				(uint8_t)(3)
#define DEF_VID_AUDIO_TRACK_DESCRIPTION_MSG			(uint8_t)(10)
#define DEF_VID_AUDIO_TRACK_MSG						(uint8_t)(11)
#define DEF_VID_FULL_SCREEN_MSG						(uint8_t)(12)
#define DEF_VID_HW_DECODER_MSG						(uint8_t)(13)
#define DEF_VID_HW_CONVERTER_MSG					(uint8_t)(14)
#define DEF_VID_MULTI_THREAD_MSG					(uint8_t)(15)
#define DEF_VID_SKIP_KEY_FRAME_MSG					(uint8_t)(16)
#define DEF_VID_LOWER_RESOLUTION_MSG				(uint8_t)(17)
#define DEF_VID_SEEKING_MSG							(uint8_t)(18)
#define DEF_VID_SEEK_MSG							(uint8_t)(19)
#define DEF_VID_VOLUME_MSG							(uint8_t)(20)
#define DEF_VID_ASPECT_RATIO_MSG					(uint8_t)(21)
#define DEF_VID_MOVE_MODE_MSG						(uint8_t)(22)
#define DEF_VID_REMEMBER_POS_MSG					(uint8_t)(23)
#define DEF_VID_PLAY_METHOD_MSG						(uint8_t)(24)
#define DEF_VID_NO_REPEAT_MSG						(uint8_t)(25)
#define DEF_VID_REPEAT_MSG							(uint8_t)(26)
#define DEF_VID_IN_ORDER_MSG						(uint8_t)(27)
#define DEF_VID_RANDOM_MSG							(uint8_t)(28)
#define DEF_VID_MOVE_MODE_EDIABLE_MSG				(uint8_t)(29)
#define DEF_VID_MOVE_MODE_ENABLE_BOTH_MSG			(uint8_t)(30)
#define DEF_VID_MOVE_MODE_ENABLE_VIDEO_MSG			(uint8_t)(31)
#define DEF_VID_MOVE_MODE_ENABLE_SUBTITLE_MSG		(uint8_t)(32)
#define DEF_VID_SUBTITLE_TRACK_DESCRIPTION_MSG		(uint8_t)(33)
#define DEF_VID_SUBTITLE_TRACK_MSG					(uint8_t)(34)
#define DEF_VID_BRIGHTNESS_MSG						(uint8_t)(35)
#define DEF_VID_DISABLE_AUDIO_MSG					(uint8_t)(36)
#define DEF_VID_DISABLE_VIDEO_MSG					(uint8_t)(37)
#define DEF_VID_DISABLE_SUBTITLE_MSG				(uint8_t)(38)
#define DEF_VID_RESTART_PLAYBACK_THRESHOLD_MSG		(uint8_t)(39)
#define DEF_VID_PROCESSING_VIDEO_MSG				(uint8_t)(40)
#define DEF_VID_NUM_OF_THREADS_MSG					(uint8_t)(41)

#define DEF_VID_MENU_NONE				(int8_t)(-1)
#define DEF_VID_MENU_SETTINGS_0			(int8_t)(0)
#define DEF_VID_MENU_SETTINGS_1			(int8_t)(1)
#define DEF_VID_MENU_INFO				(int8_t)(2)

#define DEF_VID_NO_REPEAT				(uint8_t)(0)
#define DEF_VID_REPEAT					(uint8_t)(1)
#define DEF_VID_IN_ORDER				(uint8_t)(2)
#define DEF_VID_RANDOM					(uint8_t)(3)

#define DEF_VID_MOVE_DISABLE			(uint8_t)(0)
#define DEF_VID_MOVE_BOTH				(uint8_t)(1)
#define DEF_VID_MOVE_VIDEO				(uint8_t)(2)
#define DEF_VID_MOVE_SUBTITLE			(uint8_t)(3)

#define DEF_VID_VIDEO_BUFFERS			(uint8_t)(4)
#define DEF_VID_SUBTITLE_BUFFERS		(uint8_t)(16)
#define DEF_VID_MAX_FILE_NAME_LENGTH	(uint16_t)(256)
#define DEF_VID_MAX_PATH_LENGTH			(uint16_t)(8192)

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
