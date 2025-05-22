//Includes.
#include "video_player.h"

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "system/menu.h"
#include "system/sem.h"
#include "system/draw/draw.h"
#include "system/util/converter.h"
#include "system/util/cpu_usage.h"
#include "system/util/decoder.h"
#include "system/util/err.h"
#include "system/util/expl.h"
#include "system/util/file.h"
#include "system/util/hid.h"
#include "system/util/hw_config.h"
#include "system/util/keyboard.h"
#include "system/util/log.h"
#include "system/util/queue.h"
#include "system/util/speaker.h"
#include "system/util/str.h"
#include "system/util/sync.h"
#include "system/util/thread_types.h"
#include "system/util/util.h"
#include "system/util/watch.h"

//Defines.
#define DEF_VID_DROP_THRESHOLD_ALLOWED_DURATION(frametime)	(double)(frametime * 7)
#define DEF_VID_DROP_THRESHOLD(frametime)					(double)(Util_max_d(20, frametime) * 0.8)
#define DEF_VID_FORCE_DROP_THRESHOLD(frametime)				(double)(Util_max_d(20, frametime) * 2.2)
#define DEF_VID_WAIT_THRESHOLD_ALLOWED_DURATION(frametime)	(double)(frametime * 5)
#define DEF_VID_WAIT_THRESHOLD(frametime)					(double)(Util_max_d(20, frametime) * -1.4)
#define DEF_VID_FORCE_WAIT_THRESHOLD(frametime)				(double)(Util_max_d(20, frametime) * -2.5)
#define DEF_VID_DELAY_SAMPLES								(uint8_t)(60)
#define DEF_VID_RAM_TO_KEEP_BASE							(uint32_t)(1000 * 1000 * 6)	//6MB.
#define DEF_VID_HW_DECODER_RAW_IMAGE_SIZE					(uint32_t)(vid_player.video_info[0].width * vid_player.video_info[0].height * 2)	//HW decoder always returns raw image in RGB565LE, so number of pixels * 2.
#define DEF_VID_SW_DECODER_RAW_IMAGE_SIZE					(uint32_t)(vid_player.video_info[0].width * vid_player.video_info[0].height * 1.5)	//We are assuming raw image format is YUV420P because it is the most common format, so number of pixels * 1.5.

//System UI.
#define DEF_VID_HID_SYSTEM_UI_SEL(k)					(bool)((DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN((*Draw_get_bot_ui_button()), k)) || DEF_HID_PHY_PR(k.start))
#define DEF_VID_HID_SYSTEM_UI_CFM(k)					(bool)(((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN((*Draw_get_bot_ui_button()), k)) || (DEF_HID_PR_EM(k.start, 1) || DEF_HID_HD(k.start)))
#define DEF_VID_HID_SYSTEM_UI_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch) && DEF_HID_PHY_NP(k.start))
//Enter full screen.
#define DEF_VID_HID_ENTER_FULL_CFM(k)					(bool)(DEF_HID_PR_EM(k.select, 1) || DEF_HID_HD(k.select))
//Toggle playback.
#define DEF_VID_HID_TOGGLE_PLAYBACK_CFM(k)				(bool)(DEF_HID_PR_EM(k.a, 1) || DEF_HID_HD(k.a))
//Abort playback.
#define DEF_VID_HID_ABORT_PLAYBACK_CFM(k)				(bool)(DEF_HID_PR_EM(k.b, 1) || DEF_HID_HD(k.b))
//Open file explorer.
#define DEF_VID_HID_OPEN_EXPL_CFM(k)					(bool)(DEF_HID_PR_EM(k.x, 1) || DEF_HID_HD(k.x))
//Open menu.
#define DEF_VID_HID_OPEN_MENU_CFM(k)					(bool)(DEF_HID_PR_EM(k.y, 1) || DEF_HID_HD(k.y))
//Open setting menu 0.
#define DEF_VID_HID_OPEN_SETTING_0_SEL(k)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.menu_button[0], k))
#define DEF_VID_HID_OPEN_SETTING_0_CFM(k)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.menu_button[0], k))
#define DEF_VID_HID_OPEN_SETTING_0_DESEL(k)				(bool)(DEF_HID_PHY_NP(k.touch))
//Open setting menu 1.
#define DEF_VID_HID_OPEN_SETTING_1_SEL(k)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.menu_button[1], k))
#define DEF_VID_HID_OPEN_SETTING_1_CFM(k)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.menu_button[1], k))
#define DEF_VID_HID_OPEN_SETTING_1_DESEL(k)				(bool)(DEF_HID_PHY_NP(k.touch))
//Open debug info menu.
#define DEF_VID_HID_OPEN_INFO_SEL(k)					(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.menu_button[2], k))
#define DEF_VID_HID_OPEN_INFO_CFM(k)					(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.menu_button[2], k))
#define DEF_VID_HID_OPEN_INFO_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch))
//Open control.
#define DEF_VID_HID_CONTROL_SEL(k)						(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.control_button, k))
#define DEF_VID_HID_CONTROL_CFM(k)						(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.control_button, k))
#define DEF_VID_HID_CONTROL_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch))
//Seek bar.
#define DEF_VID_HID_SEEK_BAR_SEL(k)						(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.seek_bar, k))
#define DEF_VID_HID_SEEK_BAR_PRE_CFM(k)					(bool)(DEF_VID_HID_SEEK_BAR_SEL(k) || (DEF_HID_PHY_HE(k.touch) && vid_player.seek_bar.selected))
#define DEF_VID_HID_SEEK_BAR_CFM(k)						(bool)(DEF_HID_PHY_RE(k.touch) && vid_player.seek_bar.selected)
#define DEF_VID_HID_SEEK_BAR_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch))
//Increase LCD brightness.
#define DEF_VID_HID_BRIGHTNESS_UP_PRE_CFM(k)			(bool)(DEF_HID_PHY_PR(k.d_up) || DEF_HID_HE(k.d_up))
#define DEF_VID_HID_BRIGHTNESS_UP_UPDATE_RANGE(k)		DEF_HID_HE_NEW_INTERVAL(k.d_up, 75, is_new_range)
#define DEF_VID_HID_BRIGHTNESS_UP_CFM(k)				(bool)(DEF_HID_PHY_PR(k.d_up) || DEF_HID_HE_MT(k.d_up, 1500) || is_new_range)
//Decrease LCD brightness.
#define DEF_VID_HID_BRIGHTNESS_DOWN_PRE_CFM(k)			(bool)(DEF_HID_PHY_PR(k.d_down) || DEF_HID_HE(k.d_down))
#define DEF_VID_HID_BRIGHTNESS_DOWN_UPDATE_RANGE(k)		DEF_HID_HE_NEW_INTERVAL(k.d_down, 75, is_new_range)
#define DEF_VID_HID_BRIGHTNESS_DOWN_CFM(k)				(bool)(DEF_HID_PHY_PR(k.d_down) || DEF_HID_HE_MT(k.d_down, 1500) || is_new_range)
//Move content (up).
#define DEF_VID_HID_MOVE_CONTENT_UP_CFM(k)				(bool)(DEF_HID_PHY_PR(k.c_down) || DEF_HID_PHY_HE(k.c_down))
//Move content (down).
#define DEF_VID_HID_MOVE_CONTENT_DOWN_CFM(k)			(bool)(DEF_HID_PHY_PR(k.c_up) || DEF_HID_PHY_HE(k.c_up))
//Move content (right).
#define DEF_VID_HID_MOVE_CONTENT_RIGHT_CFM(k)			(bool)(DEF_HID_PHY_PR(k.c_left) || DEF_HID_PHY_HE(k.c_left))
//Move content (left).
#define DEF_VID_HID_MOVE_CONTENT_LEFT_CFM(k)			(bool)(DEF_HID_PHY_PR(k.c_right) || DEF_HID_PHY_HE(k.c_right))
//Shrink content.
#define DEF_VID_HID_SHRINK_CONTENT_CFM(k)				(bool)(DEF_HID_PHY_PR(k.l) || DEF_HID_PHY_HE(k.l))
//Enlarge content.
#define DEF_VID_HID_ENLARGE_CONTENT_CFM(k)				(bool)(DEF_HID_PHY_PR(k.r) || DEF_HID_PHY_HE(k.r))
//Scroll mode.
#define DEF_VID_HID_SCROLL_MODE_SEL(k)					(bool)(DEF_HID_PHY_HE(k.touch) && ((abs(k.touch_x_initial - k.touch_x) > 6) || (abs(k.touch_y_initial - k.touch_y) > 6)) && !DEF_VID_HID_SCROLL_MODE_DESEL(k))
#define DEF_VID_HID_SCROLL_MODE_DESEL(k)				(bool)(DEF_HID_PHY_NP(k.touch) || vid_player.seek_bar.selected || vid_player.scroll_bar.selected || vid_player.restart_playback_threshold_bar.selected)
//Scroll bar.
#define DEF_VID_HID_SCROLL_BAR_SEL(k)					(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.scroll_bar, k))
#define DEF_VID_HID_SCROLL_BAR_CFM(k)					(bool)(DEF_VID_HID_SCROLL_BAR_SEL(k) || (DEF_HID_PHY_HE(k.touch) && vid_player.scroll_bar.selected))
#define DEF_VID_HID_SCROLL_BAR_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch))
//Full screen : Exit.
#define DEF_VID_HID_FULL_EXIT_CFM(k)					(bool)(DEF_HID_PR_EM(k.select, 1) || DEF_HID_HD(k.select) || DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch))
//Full screen : Toggle playback.
#define DEF_VID_HID_FULL_TOGGLE_PLAYBACK_CFM(k)			(bool)(DEF_HID_PR_EM(k.a, 1) || DEF_HID_HD(k.a))
//Full screen : Seek backward.
#define DEF_VID_HID_FULL_SEEK_BACK_CFM(k)				(bool)(DEF_HID_PR_EM(k.d_left, 1) || DEF_HID_HD(k.d_left))
//Full screen : Seek forward.
#define DEF_VID_HID_FULL_SEEK_FWD_CFM(k)				(bool)(DEF_HID_PR_EM(k.d_right, 1) || DEF_HID_HD(k.d_right))
//Select audio track : Confirm.
#define DEF_VID_HID_A_TRACK_CONFIRM_SEL(k)				(bool)((DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.audio_track_ok_button, k)) || DEF_HID_PHY_PR(k.a))
#define DEF_VID_HID_A_TRACK_CONFIRM_CFM(k)				(bool)(((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.audio_track_ok_button, k)) || DEF_HID_PR_EM(k.a, 1) || DEF_HID_HD(k.a))
#define DEF_VID_HID_A_TRACK_CONFIRM_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch) && DEF_HID_PHY_NP(k.a))
//Select audio track : Track selection.
#define DEF_VID_HID_A_TRACK_ITEM_SEL(k, id)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.audio_track_button[id], k))
#define DEF_VID_HID_A_TRACK_ITEM_CFM(k, id)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.audio_track_button[id], k))
#define DEF_VID_HID_A_TRACK_ITEM_DESEL(k)				(bool)(DEF_HID_PHY_NP(k.touch))
//Select audio track : Next track.
#define DEF_VID_HID_A_TRACK_NEXT_ITEM_CFM(k)			(bool)(DEF_HID_PR_EM(k.d_down, 1) || DEF_HID_HD(k.d_down) || DEF_HID_PR_EM(k.c_down, 1) || DEF_HID_HD(k.c_down))
//Select audio track : Previous track.
#define DEF_VID_HID_A_TRACK_PRE_ITEM_CFM(k)				(bool)(DEF_HID_PR_EM(k.d_up, 1) || DEF_HID_HD(k.d_up) || DEF_HID_PR_EM(k.c_up, 1) || DEF_HID_HD(k.c_up))
//Select subtitle track : Confirm.
#define DEF_VID_HID_S_TRACK_CONFIRM_SEL(k)				(bool)((DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.subtitle_track_ok_button, k)) || DEF_HID_PHY_PR(k.a))
#define DEF_VID_HID_S_TRACK_CONFIRM_CFM(k)				(bool)(((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.subtitle_track_ok_button, k)) || DEF_HID_PR_EM(k.a, 1) || DEF_HID_HD(k.a))
#define DEF_VID_HID_S_TRACK_CONFIRM_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch) && DEF_HID_PHY_NP(k.a))
//Select subtitle track : Track selection.
#define DEF_VID_HID_S_TRACK_ITEM_SEL(k, id)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.audio_track_button[id], k))
#define DEF_VID_HID_S_TRACK_ITEM_CFM(k, id)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.audio_track_button[id], k))
#define DEF_VID_HID_S_TRACK_ITEM_DESEL(k)				(bool)(DEF_HID_PHY_NP(k.touch))
//Select subtitle track : Next track.
#define DEF_VID_HID_S_TRACK_NEXT_ITEM_CFM(k)			(bool)(DEF_HID_PR_EM(k.d_down, 1) || DEF_HID_HD(k.d_down) || DEF_HID_PR_EM(k.c_down, 1) || DEF_HID_HD(k.c_down))
//Select subtitle track : Previous track.
#define DEF_VID_HID_S_TRACK_PRE_ITEM_CFM(k)				(bool)(DEF_HID_PR_EM(k.d_up, 1) || DEF_HID_HD(k.d_up) || DEF_HID_PR_EM(k.c_up, 1) || DEF_HID_HD(k.c_up))
//Control : Close.
#define DEF_VID_HID_CONTROL_CLOSE_SEL(k)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.control_button, k))
#define DEF_VID_HID_CONTROL_CLOSE_CFM(k)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.control_button, k))
#define DEF_VID_HID_CONTROL_CLOSE_DESEL(k)				(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Open audio track selection.
#define DEF_VID_HID_SE0_A_TRACK_SELECTION_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.select_audio_track_button, k))
#define DEF_VID_HID_SE0_A_TRACK_SELECTION_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.select_audio_track_button, k))
#define DEF_VID_HID_SE0_A_TRACK_SELECTION_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Open subtitle track selection.
#define DEF_VID_HID_SE0_S_TRACK_SELECTION_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.select_subtitle_track_button, k))
#define DEF_VID_HID_SE0_S_TRACK_SELECTION_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.select_subtitle_track_button, k))
#define DEF_VID_HID_SE0_S_TRACK_SELECTION_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Toggle texture filter.
#define DEF_VID_HID_SE0_TEXTURE_FILTER_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.texture_filter_button, k))
#define DEF_VID_HID_SE0_TEXTURE_FILTER_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.texture_filter_button, k))
#define DEF_VID_HID_SE0_TEXTURE_FILTER_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Toggle allow skip frames.
#define DEF_VID_HID_SE0_ALLOW_SKIP_FRAMES_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.allow_skip_frames_button, k))
#define DEF_VID_HID_SE0_ALLOW_SKIP_FRAMES_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.allow_skip_frames_button, k))
#define DEF_VID_HID_SE0_ALLOW_SKIP_FRAMES_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Toggle allow skip key frames.
#define DEF_VID_HID_SE0_ALLOW_SKIP_KEY_FRAMES_SEL(k)	(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.allow_skip_key_frames_button, k))
#define DEF_VID_HID_SE0_ALLOW_SKIP_KEY_FRAMES_CFM(k)	(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.allow_skip_key_frames_button, k))
#define DEF_VID_HID_SE0_ALLOW_SKIP_KEY_FRAMES_DESEL(k)	(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Change volume.
#define DEF_VID_HID_SE0_VOLUME_SEL(k)					(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.volume_button, k))
#define DEF_VID_HID_SE0_VOLUME_CFM(k)					(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.volume_button, k))
#define DEF_VID_HID_SE0_VOLUME_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Change seek duration.
#define DEF_VID_HID_SE0_SEEK_DURATION_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.seek_duration_button, k))
#define DEF_VID_HID_SE0_SEEK_DURATION_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.seek_duration_button, k))
#define DEF_VID_HID_SE0_SEEK_DURATION_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Toggle correct aspect ratio.
#define DEF_VID_HID_SE0_CORRECT_ASPECT_RATIO_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.correct_aspect_ratio_button, k))
#define DEF_VID_HID_SE0_CORRECT_ASPECT_RATIO_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.correct_aspect_ratio_button, k))
#define DEF_VID_HID_SE0_CORRECT_ASPECT_RATIO_DESEL(k)	(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Change content moving mode.
#define DEF_VID_HID_SE0_MOVE_CONTENT_MODE_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.move_content_button, k))
#define DEF_VID_HID_SE0_MOVE_CONTENT_MODE_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.move_content_button, k))
#define DEF_VID_HID_SE0_MOVE_CONTENT_MODE_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Toggle remember media position.
#define DEF_VID_HID_SE0_SAVE_MEDIA_POS_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.remember_video_pos_button, k))
#define DEF_VID_HID_SE0_SAVE_MEDIA_POS_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.remember_video_pos_button, k))
#define DEF_VID_HID_SE0_SAVE_MEDIA_POS_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Change playback mode.
#define DEF_VID_HID_SE0_PLAYBACK_MODE_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.playback_mode_button, k))
#define DEF_VID_HID_SE0_PLAYBACK_MODE_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.playback_mode_button, k))
#define DEF_VID_HID_SE0_PLAYBACK_MODE_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 0 : Change restart playback threshold.
#define DEF_VID_HID_SE0_RESUME_PLAYBACK_THRESHOLD_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.restart_playback_threshold_bar, k))
#define DEF_VID_HID_SE0_RESUME_PLAYBACK_THRESHOLD_CFM(k)		(bool)(DEF_VID_HID_SE0_RESUME_PLAYBACK_THRESHOLD_SEL(k) || (DEF_HID_PHY_HE(k.touch) && vid_player.restart_playback_threshold_bar.selected))
#define DEF_VID_HID_SE0_RESUME_PLAYBACK_THRESHOLD_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 1 : Toggle audio.
#define DEF_VID_HID_SE1_AUDIO_SEL(k)					(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.disable_audio_button, k))
#define DEF_VID_HID_SE1_AUDIO_CFM(k)					(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.disable_audio_button, k))
#define DEF_VID_HID_SE1_AUDIO_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 1 : Toggle video.
#define DEF_VID_HID_SE1_VIDEO_SEL(k)					(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.disable_video_button, k))
#define DEF_VID_HID_SE1_VIDEO_CFM(k)					(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.disable_video_button, k))
#define DEF_VID_HID_SE1_VIDEO_DESEL(k)					(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 1 : Toggle subtitle.
#define DEF_VID_HID_SE1_SUBTITLE_SEL(k)					(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.disable_subtitle_button, k))
#define DEF_VID_HID_SE1_SUBTITLE_CFM(k)					(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.disable_subtitle_button, k))
#define DEF_VID_HID_SE1_SUBTITLE_DESEL(k)				(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 1 : Toggle HW decoding.
#define DEF_VID_HID_SE1_HW_DECODING_SEL(k)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.use_hw_decoding_button, k))
#define DEF_VID_HID_SE1_HW_DECODING_CFM(k)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.use_hw_decoding_button, k))
#define DEF_VID_HID_SE1_HW_DECODING_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 1 : Toggle HW color conversion.
#define DEF_VID_HID_SE1_HW_CONVERSION_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.use_hw_color_conversion_button, k))
#define DEF_VID_HID_SE1_HW_CONVERSION_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.use_hw_color_conversion_button, k))
#define DEF_VID_HID_SE1_HW_CONVERSION_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 1 : Toggle multi-threaded decoding.
#define DEF_VID_HID_SE1_MULTI_THREAD_SEL(k)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.use_multi_threaded_decoding_button, k))
#define DEF_VID_HID_SE1_MULTI_THREAD_CFM(k)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.use_multi_threaded_decoding_button, k))
#define DEF_VID_HID_SE1_MULTI_THREAD_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Settings 1 : Lower resolution.
#define DEF_VID_HID_SE1_LOWER_RESOLUTION_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.lower_resolution_button, k))
#define DEF_VID_HID_SE1_LOWER_RESOLUTION_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.lower_resolution_button, k))
#define DEF_VID_HID_SE1_LOWER_RESOLUTION_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Info : Toggle decoding time graph.
#define DEF_VID_HID_INFO_DECODING_GRAPH_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.show_decode_graph_button, k))
#define DEF_VID_HID_INFO_DECODING_GRAPH_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.show_decode_graph_button, k))
#define DEF_VID_HID_INFO_DECODING_GRAPH_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Info : Toggle color converison time graph.
#define DEF_VID_HID_INFO_CONVERSION_GRAPH_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.show_color_conversion_graph_button, k))
#define DEF_VID_HID_INFO_CONVERSION_GRAPH_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.show_color_conversion_graph_button, k))
#define DEF_VID_HID_INFO_CONVERSION_GRAPH_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))
//Info : Toggle compressed buffer graph.
#define DEF_VID_HID_INFO_BUFFER_GRAPH_SEL(k)			(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.show_packet_buffer_graph_button, k))
#define DEF_VID_HID_INFO_BUFFER_GRAPH_CFM(k)			(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.show_packet_buffer_graph_button, k))
#define DEF_VID_HID_INFO_BUFFER_GRAPH_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Info : Toggle raw video buffer graph.
#define DEF_VID_HID_INFO_RAW_V_BUFFER_GRAPH_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.show_raw_video_buffer_graph_button, k))
#define DEF_VID_HID_INFO_RAW_V_BUFFER_GRAPH_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.show_raw_video_buffer_graph_button, k))
#define DEF_VID_HID_INFO_RAW_V_BUFFER_GRAPH_DESEL(k)	(bool)(DEF_HID_PHY_NP(k.touch))
//Info : Toggle raw audio buffer graph.
#define DEF_VID_HID_INFO_RAW_A_BUFFER_GRAPH_SEL(k)		(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(vid_player.show_raw_audio_buffer_graph_button, k))
#define DEF_VID_HID_INFO_RAW_A_BUFFER_GRAPH_CFM(k)		(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(vid_player.show_raw_audio_buffer_graph_button, k))
#define DEF_VID_HID_INFO_RAW_A_BUFFER_GRAPH_DESEL(k)	(bool)(DEF_HID_PHY_NP(k.touch))

//Typedefs.
typedef enum
{
	NONE_REQUEST,

	DECODE_THREAD_PLAY_REQUEST,						//Start playback request.
	DECODE_THREAD_PAUSE_REQUEST,					//Pause playback request.
	DECODE_THREAD_RESUME_REQUEST,					//Resume playback request.
	DECODE_THREAD_CHANGE_AUDIO_TRACK_REQUEST,		//Change audio track request.
	DECODE_THREAD_CHANGE_SUBTITLE_TRACK_REQUEST,	//Change subtitle track request.
	DECODE_THREAD_SEEK_REQUEST,						//Seek request (from hid thread).
	DECODE_THREAD_PLAY_NEXT_REQUEST,				//Change file (or repeat) request.
	DECODE_THREAD_ABORT_REQUEST,					//Stop request.
	DECODE_THREAD_SHUTDOWN_REQUEST,					//Exit request.
	DECODE_THREAD_INCREASE_KEEP_RAM_REQUEST,		//Increase amount of RAM to keep request.

	READ_PACKET_THREAD_READ_PACKET_REQUEST,			//Read a file request.
	READ_PACKET_THREAD_SEEK_REQUEST,				//Seek request (from decode theread).
	READ_PACKET_THREAD_ABORT_REQUEST,				//Stop request.

	DECODE_VIDEO_THREAD_DECODE_REQUEST,				//Decode a frame request.
	DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST,		//Clear cache request (in seek process).
	DECODE_VIDEO_THREAD_ABORT_REQUEST,				//Stop request.

	CONVERT_THREAD_CONVERT_REQUEST,					//Start converting request.
	CONVERT_THREAD_ABORT_REQUEST,					//Stop request.

	MAX_REQUEST = UINT32_MAX,
} Vid_command;

typedef enum
{
	NONE_NOTIFICATION,

	DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION,				//Done buffering because of memory limit or eof.

	READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION,			//Done reading a file because of buffer limit.
	READ_PACKET_THREAD_FINISHED_READING_EOF_NOTIFICATION,		//Done reading a file because of eof.
	READ_PACKET_THREAD_FINISHED_SEEKING_NOTIFICATION,			//Done seeking.
	READ_PACKET_THREAD_FINISHED_ABORTING_NOTIFICATION,			//Done aborting.

	DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION,	//Done copying a video packet.
	DECODE_VIDEO_THREAD_FINISHED_CLEARING_CACHE,				//Done clearing cache.
	DECODE_VIDEO_THREAD_FINISHED_ABORTING_NOTIFICATION,			//Done aborting.

	CONVERT_THREAD_OUT_OF_BUFFER_NOTIFICATION,					//Run out of video buffer.
	CONVERT_THREAD_FINISHED_BUFFERING_NOTIFICATION,				//Done buffering because of threshold or buffer limit.
	CONVERT_THREAD_FINISHED_ABORTING_NOTIFICATION,				//Done aborting.

	MAX_NOTIFICATION = UINT32_MAX,
} Vid_notification;

typedef enum
{
	PLAYER_STATE_IDLE,				//File is not opened.
	PLAYER_STATE_PREPATE_PLAYING,	//File is not opened, but will be opened soon.
	PLAYER_STATE_PLAYING,			//File is opened and playing.
	PLAYER_STATE_PAUSE,				//File is opened but not playing.
	PLAYER_STATE_BUFFERING,			//File is opened but it runs out of video buffer, buffering is in progress.
	PLAYER_STATE_PREPARE_SEEKING,	//File is opened and seek request has been received.
	PLAYER_STATE_SEEKING,			//File is opened and seek is in progress.
} Vid_player_main_state;

typedef enum
{
	PLAYER_SUB_STATE_NONE					= 0,		//Nothing special.
	PLAYER_SUB_STATE_HW_DECODING			= (1 << 0),	//Player is using hardware decoding.
	PLAYER_SUB_STATE_HW_CONVERSION			= (1 << 1),	//Player is using hardware color converter.
	PLAYER_SUB_STATE_TOO_BIG				= (1 << 2),	//Volume is too big.
	PLAYER_SUB_STATE_RESUME_LATER			= (1 << 3),	//After buffering or seeking, playback should be resumed.
	PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT		= (1 << 4),	//Waiting for seek backward.
} Vid_player_sub_state;

typedef struct
{
	uint32_t image_width;
	uint32_t image_height;
	uint16_t num_of_images;
	Draw_image_data* images;
} Large_image;

typedef struct
{
	char name[DEF_VID_MAX_FILE_NAME_LENGTH];	//File name.
	char directory[DEF_VID_MAX_PATH_LENGTH];	//Directory path for this file.
	uint32_t index;								//File index of this file.
} Vid_file;

typedef struct
{
	bool is_key_frame;		//Whether current packet is key frame.
	uint8_t packet_index;	//Packet index for current packet.
} Vid_video_packet_data;

typedef struct
{
	//Settings (can be changed while playing videos).
	bool remember_video_pos;						//Whether save playback position so that we can resume from there later.
	bool use_linear_texture_filter;					//Whether apply linear texture filter.
	bool correct_aspect_ratio;						//Whether correct aspect ratio based on SAR values.
	bool allow_skip_frames;							//Whether allow skip non-keyframes when it can't keep up.
	bool allow_skip_key_frames;						//Whether allow skip keyframes when it can't keep up.
	uint8_t playback_mode;							//Playback mode.
	uint8_t move_content_mode;						//Move content mode.
	uint8_t seek_duration;							//Seek duration for DPAD (direction pad) left and right in seconds.
	uint16_t restart_playback_threshold;			//Restart playback threshold after out of video buffer.
	uint16_t volume;								//Software controlled volume level in %.

	//Settings (can NOT be changed while playing videos).
	bool disable_audio;								//Whether disable audio.
	bool disable_video;								//Whether disable video.
	bool disable_subtitle;							//Whether disable subtitle.
	bool use_hw_decoding;							//Whether use hardware decoder if available.
	bool use_hw_color_conversion;					//Whether use hardware color conversion if available.
	bool use_multi_threaded_decoding;				//Whether use multi-threaded software decoding if available.
	uint8_t lower_resolution;						//Downscale level on supported codecs, 0 = 100% (no downscale), 1 = 50%, 2 = 25%.

	//Other settings (user doesn't have permission to change).
	bool show_full_screen_msg;						//Whether show how to exit full screen msg.
	uint8_t num_of_threads;							//Number of threads to use for multi-threaded software decoding.
	uint32_t ram_to_keep_base;						//RAM amount to keep in bytes.
	Media_thread_type thread_mode;					//Thread mode to use for multi-threaded software decoding.

	//Debug view.
	bool show_decoding_graph;						//Whether show decode time graph in debug view.
	bool show_color_conversion_graph;				//Whether show color conversion time graph in debug view.
	bool show_packet_buffer_graph;					//Whether show packet buffer health graph in debug view.
	bool show_raw_video_buffer_graph;				//Whether show video buffer health graph in debug view.
	bool show_raw_audio_buffer_graph;				//Whether show audio buffer health graph in debug view.
	uint32_t total_frames;							//Total number of decoded frames
													//(including decoded frames in seeking, so this is not the same as total_rendered_frames).
	uint32_t total_rendered_frames;					//Total number of rendered frames.
	uint32_t total_dropped_frames;					//Total number of dropped frames that should have rendered.
	uint64_t previous_ts;							//Time stamp for last graph update (only for packet, video and audio buffers).
	double decoding_min_time;						//Minimum video decoding time in ms.
	double decoding_max_time;						//Maximum video decoding time in ms.
	double decoding_total_time;						//Total video decoding time in ms.
	double decoding_recent_total_time;				//Same as above, but only for recent 90 frames.
	bool keyframe_list[320];						//List for keyframe.
	uint16_t packet_buffer_list[320];				//List for packet buffer health.
	uint16_t raw_video_buffer_list[320][DEF_DECODER_MAX_VIDEO_TRACKS];	//List for video buffer health.
	uint32_t raw_audio_buffer_list[320];			//List for audio buffer health.
	double video_decoding_time_list[320];			//List for video decoding time in ms.
	double audio_decoding_time_list[320];			//List for audio decoding time in ms.
	double conversion_time_list[320];				//List for color conversion time in ms.
	const void* frame_list[32];						//Decoding frame list (for multi-threaded software decoding).
	TickCounter decoding_time_tick[32];				//Decoding time (for multi-threaded software decoding).

	//A/V desync management.
	uint64_t wait_threshold_exceeded_ts;			//Timestamp that "wait threshold" has been exceeded (video is fast).
	uint64_t drop_threshold_exceeded_ts;			//Timestamp that "skip threshold" has been exceeded (video is slow).
	uint64_t last_video_frame_updated_ts;			//Timestamp for last video frame update.
	double video_delay_ms[DEF_DECODER_MAX_VIDEO_TRACKS][DEF_VID_DELAY_SAMPLES];	//Raw video delay (A/V desync) data.
	double video_delay_avg_ms[DEF_DECODER_MAX_VIDEO_TRACKS];					//Average video delay (A/V desync) data.

	//Player.
	Vid_player_main_state state;					//Main state.
	Vid_player_sub_state sub_state;					//Sub state.
	Vid_file file;									//File info.

	//Media.
	double media_duration;							//Media duration in ms, if the file contains both video and audio, then the longest one's duration.
	double media_current_pos;						//Current media position in ms, if the file contains both video and audio,
													//then the longest one's position.
	double seek_pos_cache;							//Seek destination position cache, this is used when user is holding the seek bar.
	double seek_pos;								//Seek destination position in ms.

	//Video.
	uint8_t num_of_video_tracks;					//Number of video tracks for current file.
	uint16_t vps;									//Actual video playback framerate.
	uint16_t vfps_cache;							//Actual video playback framerate cache.
	double next_frame_update_time;					//Next timestamp to update a video frame.
	double next_vfps_update;						//Next timestamp to update vps value.
	double _3d_slider_pos;							//3D slider position (0.0~1.0).
	double video_x_offset;							//X (horizontal) offset for video.
	double video_y_offset;							//Y (vertical) offset for video.
	double video_zoom;								//Zoom level for video.
	double video_frametime;							//Video frametime in ms, if file contains 2 video tracks, then this will be (actual_frametime / 2).
	uint8_t next_store_index[DEF_DECODER_MAX_VIDEO_TRACKS];	//Next texture buffer index to store converted image.
	uint8_t next_draw_index[DEF_DECODER_MAX_VIDEO_TRACKS];	//Next texture buffer index that is ready to draw.
	double video_current_pos[DEF_DECODER_MAX_VIDEO_TRACKS];	//Current video position in ms.
	Media_v_info video_info[DEF_DECODER_MAX_VIDEO_TRACKS];	//Video info.
	Large_image large_image[DEF_VID_VIDEO_BUFFERS][DEF_DECODER_MAX_VIDEO_TRACKS];	//Texture for video images.

	//Audio.
	uint8_t num_of_audio_tracks;					//Number of audio tracks for current file.
	uint8_t selected_audio_track_cache;				//Selected audio track cache, this is used when user is selecting audio track.
	uint8_t selected_audio_track;					//Selected audio track.
	double audio_current_pos;						//Current audio position in ms.
	double last_decoded_audio_pos;					//Latest decoded audio position in ms.
	Media_a_info audio_info[DEF_DECODER_MAX_AUDIO_TRACKS];	//Audio info.

	//Subtitle.
	uint8_t num_of_subtitle_tracks;					//Number of subtitle tracks for current file.
	uint8_t selected_subtitle_track_cache;			//Selected subtitle track cache, this is used when user is selecting audio track.
	uint8_t selected_subtitle_track;				//Selected subtitle track.
	uint8_t subtitle_index;							//Next subtitle index for subtitle_data.
	double subtitle_x_offset;						//X (horizontal) offset for subtitle.
	double subtitle_y_offset;						//Y (vertical) offset for subtitle.
	double subtitle_zoom;							//Zoom level for subtitle.
	Media_s_info subtitle_info[DEF_DECODER_MAX_SUBTITLE_TRACKS];	//Subtitle info.
	Media_s_data subtitle_data[DEF_VID_SUBTITLE_BUFFERS];			//Subtitle data.
	Draw_image_data subtitle_image[DEF_VID_SUBTITLE_BUFFERS];		//Bitmap subtitle data.

	//UI.
	bool is_full_screen;							//Whether player is full screen.
	bool is_pause_for_home_menu;					//Whether player state is pause because of nintendo home menu.
	bool is_selecting_audio_track;					//Whether user is selecting a audio track.
	bool is_selecting_subtitle_track;				//Whether user is selecting a subtitle track.
	bool is_setting_volume;							//Whether user is setting volume level.
	bool is_setting_seek_duration;					//Whether user is setting seek duration.
	bool is_displaying_controls;					//Whether user is checking how to control.
	bool is_scroll_mode;							//Whether scroll mode is active.
	int8_t menu_mode;								//Current menu tab.
	uint32_t turn_off_bottom_screen_count;			//Turn bottom screen off after this count in full screen mode.
	uint64_t show_screen_brightness_until;			//Display screen brightness message until this time.
	uint64_t show_current_pos_until;				//Display current position message until this time.
	double ui_y_offset_max;							//Max Y (vertical) offset for UI.
	double ui_y_offset;								//Y (vertical) offset for UI.
	double ui_y_move;								//Remaining Y (vertical) direction movement, this is used for scrolling.

	uint32_t banner_texture_handle;					//Banner texture handle used for freeing texture.
	uint32_t control_texture_handle;				//Control texture handle used for freeing texture.
	C2D_Image banner[2];							//Banner texture.
	C2D_Image control[2];							//Control texture.

	//Buttons.
	Draw_image_data select_audio_track_button, texture_filter_button, allow_skip_frames_button, allow_skip_key_frames_button,
	volume_button, seek_duration_button, use_hw_decoding_button, use_hw_color_conversion_button, use_multi_threaded_decoding_button,
	lower_resolution_button, menu_button[3], control_button, audio_track_ok_button, audio_track_button[DEF_DECODER_MAX_AUDIO_TRACKS],
	correct_aspect_ratio_button, move_content_button, remember_video_pos_button, show_decode_graph_button,
	show_color_conversion_graph_button, show_packet_buffer_graph_button, show_raw_video_buffer_graph_button,
	show_raw_audio_buffer_graph_button, scroll_bar, playback_mode_button, subtitle_track_ok_button,
	subtitle_track_button[DEF_DECODER_MAX_SUBTITLE_TRACKS], select_subtitle_track_button, disable_audio_button,
	disable_video_button, disable_subtitle_button, restart_playback_threshold_bar, seek_bar;

	//Threads and queues.
	Thread decode_thread;							//Decode thread handle.
	Queue_data decode_thread_command_queue;			//Decode thread command queue.
	Queue_data decode_thread_notification_queue;	//Decode thread notification queue.

	Thread decode_video_thread;						//Decode video thread handle.
	Queue_data decode_video_thread_command_queue;	//Decode video thread command queue.

	Thread convert_thread;							//Convert thread handle.
	Queue_data convert_thread_command_queue;		//Convert thread command queue.

	Thread read_packet_thread;						//Read packet thread handle.
	Queue_data read_packet_thread_command_queue;	//Read packet thread command queue.

	//Mutexs.
	Sync_data texture_init_free_lock;				//Mutex for initializing and freeing texture buffers.
	Sync_data delay_update_lock;					//Mutex for updating video delay information.
} Vid_player;

//Prototypes.
static void Vid_draw_init_exit_message(void);
static uint32_t Vid_get_min_texture_size(uint32_t width_or_height);
static void Vid_large_texture_free(Large_image* large_image_data);
static void Vid_large_texture_set_filter(Large_image* large_image_data, bool filter);
static void Vid_large_texture_crop(Large_image* large_image_data, uint32_t width, uint32_t height);
static uint32_t Vid_large_texture_init(Large_image* large_image_data, uint32_t width, uint32_t height, Raw_pixel color_format, bool zero_initialize);
static uint32_t Vid_large_texture_set_data(Large_image* large_image_data, uint8_t* raw_image, uint32_t width, uint32_t height, bool use_direct);
static void Vid_large_texture_draw(Large_image* large_image_data, double x_offset, double y_offset, double pic_width, double pic_height);
static void Vid_fit_to_screen(uint16_t screen_width, uint16_t screen_height);
static void Vid_change_video_size(double change_px);
static void Vid_enter_full_screen(uint32_t bottom_screen_timeout);
static void Vid_exit_full_screen(void);
static void Vid_increase_screen_brightness(void);
static void Vid_decrease_screen_brightness(void);
static void Vid_control_full_screen(void);
static void Vid_update_sleep_policy(void);
static void Vid_update_performance_graph(double decoding_time, bool is_key_frame, double* total_delay);
static void Vid_update_video_delay(uint8_t packet_index);
static void Vid_init_variable(void);
static void Vid_init_settings(void);
static void Vid_init_hidden_settings(void);
static void Vid_init_debug_view_mode(void);
static void Vid_init_debug_view_data(void);
static void Vid_init_player_data(void);
static void Vid_init_desync_data(void);
static void Vid_init_media_data(void);
static void Vid_init_video_data(void);
static void Vid_init_audio_data(void);
static void Vid_init_subtitle_data(void);
static void Vid_init_ui_data(void);
//Removed static from these functions because they are implementation for weak functions.
void frame_worker_thread_start(const void* ptr);
void frame_worker_thread_end(const void* ptr);
void dav1d_worker_task_start(const void* ptr);
void dav1d_worker_task_end(const void* ptr);
//Removed static from thread to make symbol accessble from other files (for debug).
void Vid_init_thread(void* arg);
void Vid_exit_thread(void* arg);
void Vid_decode_thread(void* arg);
void Vid_decode_video_thread(void* arg);
void Vid_convert_thread(void* arg);
void Vid_read_packet_thread(void* arg);
static void Vid_callback(Str_data* file, Str_data* dir);
static void Vid_cancel(void);

//Variables.
static bool vid_main_run = false;
static bool vid_thread_run = false;
static bool vid_already_init = false;
static bool vid_thread_suspend = true;
static Str_data vid_status = { 0, };
static Str_data vid_msg[DEF_VID_NUM_OF_MSG] = { 0, };
static Thread vid_init_thread = 0, vid_exit_thread = 0;
static Vid_player vid_player = { 0, };

//Code.
bool Vid_query_init_flag(void)
{
	return vid_already_init;
}

bool Vid_query_running_flag(void)
{
	return vid_main_run;
}

void Vid_hid(Hid_info key)
{
	uint32_t result = DEF_ERR_OTHER;
	Sem_config config = { 0, };
	Sem_state state = { 0, };

	if(vid_player.is_setting_volume || vid_player.is_setting_seek_duration || (aptShouldJumpToHome() && vid_player.is_pause_for_home_menu))
		return;

	Sem_get_config(&config);
	Sem_get_state(&state);

	if(Util_err_query_show_flag())
		Util_err_main(key);
	else if(Util_expl_query_show_flag())
		Util_expl_main(key, config.scroll_speed);
	else
	{
		if(vid_player.is_pause_for_home_menu)
		{
			vid_player.is_pause_for_home_menu = false;
			//Resume the video.
			DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
			DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
		}

		if(aptShouldJumpToHome())
		{
			vid_player.is_pause_for_home_menu = true;
			//Pause the video.
			DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
			DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

			while(vid_player.state == PLAYER_STATE_PLAYING)
				Util_sleep(10000);
		}

		//Notify user that button is being pressed.
		if(vid_player.is_full_screen)
		{
			//Do nothing.
		}
		else if(vid_player.is_selecting_audio_track)//Select audio track.
		{
			if(DEF_VID_HID_A_TRACK_CONFIRM_SEL(key))
				vid_player.audio_track_ok_button.selected = true;
			for(uint8_t i = 0; i < vid_player.num_of_audio_tracks; i++)
			{
				if(DEF_VID_HID_A_TRACK_ITEM_SEL(key, i))
					vid_player.audio_track_button[i].selected = true;
			}
		}
		else if(vid_player.is_selecting_subtitle_track)//Select subtitle.
		{
			if(DEF_VID_HID_S_TRACK_CONFIRM_SEL(key))
				vid_player.subtitle_track_ok_button.selected = true;
			for(uint8_t i = 0; i < vid_player.num_of_subtitle_tracks; i++)
			{
				if(DEF_VID_HID_S_TRACK_ITEM_SEL(key, i))
					vid_player.subtitle_track_button[i].selected = true;
			}
		}
		else if(vid_player.is_displaying_controls)
		{
			if(DEF_VID_HID_CONTROL_CLOSE_SEL(key))
				vid_player.control_button.selected = true;
		}
		else
		{
			if(DEF_VID_HID_SYSTEM_UI_SEL(key))
				Draw_get_bot_ui_button()->selected = true;
			if(DEF_VID_HID_CONTROL_SEL(key))
				vid_player.control_button.selected = true;
			else if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_0)
			{
				if(DEF_VID_HID_OPEN_SETTING_1_SEL(key))
					vid_player.menu_button[1].selected = true;
				if(DEF_VID_HID_OPEN_INFO_SEL(key))
					vid_player.menu_button[2].selected = true;
				if(DEF_VID_HID_SCROLL_BAR_SEL(key))
					vid_player.scroll_bar.selected = true;
				if(DEF_VID_HID_SE0_A_TRACK_SELECTION_SEL(key))
					vid_player.select_audio_track_button.selected = true;
				if(DEF_VID_HID_SE0_S_TRACK_SELECTION_SEL(key))
					vid_player.select_subtitle_track_button.selected = true;
				if(DEF_VID_HID_SE0_TEXTURE_FILTER_SEL(key))
					vid_player.texture_filter_button.selected = true;
				if(DEF_VID_HID_SE0_ALLOW_SKIP_FRAMES_SEL(key))
					vid_player.allow_skip_frames_button.selected = true;
				if(DEF_VID_HID_SE0_ALLOW_SKIP_KEY_FRAMES_SEL(key) && vid_player.allow_skip_frames)
					vid_player.allow_skip_key_frames_button.selected = true;
				if(DEF_VID_HID_SE0_VOLUME_SEL(key))
					vid_player.volume_button.selected = true;
				if(DEF_VID_HID_SE0_SEEK_DURATION_SEL(key))
					vid_player.seek_duration_button.selected = true;
				if(DEF_VID_HID_SE0_CORRECT_ASPECT_RATIO_SEL(key))
					vid_player.correct_aspect_ratio_button.selected = true;
				if(DEF_VID_HID_SE0_MOVE_CONTENT_MODE_SEL(key))
					vid_player.move_content_button.selected = true;
				if(DEF_VID_HID_SE0_SAVE_MEDIA_POS_SEL(key))
					vid_player.remember_video_pos_button.selected = true;
				if(DEF_VID_HID_SE0_PLAYBACK_MODE_SEL(key))
					vid_player.playback_mode_button.selected = true;
				if(DEF_VID_HID_SE0_RESUME_PLAYBACK_THRESHOLD_SEL(key))
					vid_player.restart_playback_threshold_bar.selected = true;

				if(DEF_VID_HID_SCROLL_MODE_SEL(key))
					vid_player.is_scroll_mode = true;
			}
			if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_1)
			{
				if(DEF_VID_HID_OPEN_SETTING_0_SEL(key))
					vid_player.menu_button[0].selected = true;
				if(DEF_VID_HID_OPEN_INFO_SEL(key))
					vid_player.menu_button[2].selected = true;
				if(DEF_VID_HID_SCROLL_BAR_SEL(key))
					vid_player.scroll_bar.selected = true;
				if(vid_player.state == PLAYER_STATE_IDLE)
				{
					if(DEF_VID_HID_SE1_AUDIO_SEL(key))
						vid_player.disable_audio_button.selected = true;
					if(DEF_VID_HID_SE1_VIDEO_SEL(key))
						vid_player.disable_video_button.selected = true;
					if(DEF_VID_HID_SE1_SUBTITLE_SEL(key))
						vid_player.disable_subtitle_button.selected = true;
					if(DEF_VID_HID_SE1_HW_DECODING_SEL(key) && DEF_SEM_MODEL_IS_NEW(state.console_model))
						vid_player.use_hw_decoding_button.selected = true;
					if(DEF_VID_HID_SE1_HW_CONVERSION_SEL(key))
						vid_player.use_hw_color_conversion_button.selected = true;
					if(DEF_VID_HID_SE1_MULTI_THREAD_SEL(key))
						vid_player.use_multi_threaded_decoding_button.selected = true;
					if(DEF_VID_HID_SE1_LOWER_RESOLUTION_SEL(key))
						vid_player.lower_resolution_button.selected = true;
				}

				if(DEF_VID_HID_SCROLL_MODE_SEL(key))
					vid_player.is_scroll_mode = true;
			}
			else if(vid_player.menu_mode == DEF_VID_MENU_INFO)
			{
				if(DEF_VID_HID_OPEN_SETTING_0_SEL(key))
					vid_player.menu_button[0].selected = true;
				if(DEF_VID_HID_OPEN_SETTING_1_SEL(key))
					vid_player.menu_button[1].selected = true;
				if(DEF_VID_HID_SCROLL_BAR_SEL(key))
					vid_player.scroll_bar.selected = true;
				if(DEF_VID_HID_INFO_DECODING_GRAPH_SEL(key))
					vid_player.show_decode_graph_button.selected = true;
				if(DEF_VID_HID_INFO_CONVERSION_GRAPH_SEL(key))
					vid_player.show_color_conversion_graph_button.selected = true;
				if(DEF_VID_HID_INFO_BUFFER_GRAPH_SEL(key))
					vid_player.show_packet_buffer_graph_button.selected = true;
				if(DEF_VID_HID_INFO_RAW_V_BUFFER_GRAPH_SEL(key))
					vid_player.show_raw_video_buffer_graph_button.selected = true;
				if(DEF_VID_HID_INFO_RAW_A_BUFFER_GRAPH_SEL(key))
					vid_player.show_raw_audio_buffer_graph_button.selected = true;

				if(DEF_VID_HID_SCROLL_MODE_SEL(key))
					vid_player.is_scroll_mode = true;
			}

			if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PREPATE_PLAYING)
			{
				if(DEF_VID_HID_SEEK_BAR_SEL(key))
					vid_player.seek_bar.selected = true;
			}
		}

		//Execute functions if conditions are satisfied.
		//Check for control screen brightness first.
		if(DEF_VID_HID_BRIGHTNESS_UP_PRE_CFM(key))
		{
			bool is_new_range = false;//Used by UPDATE_RANGE and CONFIRMED macro.

			DEF_VID_HID_BRIGHTNESS_UP_UPDATE_RANGE(key);

			if(DEF_VID_HID_BRIGHTNESS_UP_CFM(key))
				Vid_increase_screen_brightness();
		}
		else if(DEF_VID_HID_BRIGHTNESS_DOWN_PRE_CFM(key))
		{
			bool is_new_range = false;//Used by UPDATE_RANGE and CONFIRMED macro.

			DEF_VID_HID_BRIGHTNESS_DOWN_UPDATE_RANGE(key);

			if(DEF_VID_HID_BRIGHTNESS_DOWN_CFM(key))
				Vid_decrease_screen_brightness();
		}
		else if(vid_player.is_full_screen)
		{
			if(DEF_VID_HID_FULL_EXIT_CFM(key) || aptShouldJumpToHome())
			{
				Vid_fit_to_screen(400, 225);
				Vid_exit_full_screen();
				//Reset key state on scene change.
				Util_hid_reset_key_state(HID_KEY_BIT_ALL);
				vid_player.show_full_screen_msg = false;
			}
			else if(DEF_VID_HID_FULL_TOGGLE_PLAYBACK_CFM(key))
			{
				if(vid_player.state == PLAYER_STATE_IDLE)//Play the video.
				{
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
					DECODE_THREAD_PLAY_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
				}
				//If player state is one of them, pause the video.
				//1. State is playing.
				//2. State is buffering and sub state contains resume later flag.
				//3. State is prepare seeking and sub state contains resume later flag.
				//4. State is seeking and sub state contains resume later flag.
				else if(vid_player.state == PLAYER_STATE_PLAYING
				|| ((vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
				|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING)))
				{
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
					DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
				}
				//If player state is one of them, resume the video.
				//1. State is pause.
				//2. State is buffering and sub state doesn't contain resume later flag.
				//3. State is prepare seeking and sub state doesn't contain resume later flag.
				//4. State is seeking and sub state doesn't contain resume later flag.
				else if(vid_player.state == PLAYER_STATE_PAUSE
				|| (!(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
				|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING)))
				{
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
					DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
				}
			}
		}
		else if(vid_player.is_selecting_audio_track)//Select audio track.
		{
			if(DEF_VID_HID_A_TRACK_NEXT_ITEM_CFM(key))
			{
				if((vid_player.num_of_audio_tracks - vid_player.selected_audio_track_cache) > 1)
					vid_player.selected_audio_track_cache++;
			}
			else if(DEF_VID_HID_A_TRACK_PRE_ITEM_CFM(key))
			{
				if(vid_player.selected_audio_track_cache > 0)
					vid_player.selected_audio_track_cache--;
			}
			else if(DEF_VID_HID_A_TRACK_CONFIRM_CFM(key))
			{
				vid_player.is_selecting_audio_track = false;
				//Reset key state on scene change.
				Util_hid_reset_key_state(HID_KEY_BIT_ALL);
				//Apply the track selection.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
				DECODE_THREAD_CHANGE_AUDIO_TRACK_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
			}

			for(uint8_t i = 0; i < vid_player.num_of_audio_tracks; i++)
			{
				if(DEF_VID_HID_A_TRACK_ITEM_CFM(key, i))
				{
					vid_player.selected_audio_track_cache = i;
					break;
				}
			}
		}
		else if(vid_player.is_selecting_subtitle_track)//Select subtitle.
		{
			if(DEF_VID_HID_S_TRACK_NEXT_ITEM_CFM(key))
			{
				if((vid_player.num_of_subtitle_tracks - vid_player.selected_subtitle_track_cache) > 1)
					vid_player.selected_subtitle_track_cache++;
			}
			else if(DEF_VID_HID_S_TRACK_PRE_ITEM_CFM(key))
			{
				if(vid_player.selected_subtitle_track_cache > 0)
					vid_player.selected_subtitle_track_cache--;
			}
			else if(DEF_VID_HID_S_TRACK_CONFIRM_CFM(key))
			{
				vid_player.is_selecting_subtitle_track = false;
				//Reset key state on scene change.
				Util_hid_reset_key_state(HID_KEY_BIT_ALL);
				//Apply the track selection.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
				DECODE_THREAD_CHANGE_SUBTITLE_TRACK_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
			}

			for(uint8_t i = 0; i < vid_player.num_of_subtitle_tracks; i++)
			{
				if(DEF_VID_HID_S_TRACK_ITEM_CFM(key, i))
				{
					vid_player.selected_subtitle_track_cache = i;
					break;
				}
			}
		}
		else if(vid_player.is_displaying_controls)
		{
			if(DEF_VID_HID_CONTROL_CLOSE_CFM(key))
			{
				vid_player.is_displaying_controls = false;
				//Reset key state on scene change.
				Util_hid_reset_key_state(HID_KEY_BIT_ALL);
			}
		}
		else
		{
			if (DEF_VID_HID_SYSTEM_UI_CFM(key))
				Vid_suspend();
			else if(!vid_player.is_scroll_mode)
			{
				if(DEF_VID_HID_ENTER_FULL_CFM(key))
				{
					Vid_fit_to_screen(400, 240);
					Vid_enter_full_screen(300);
					//Reset key state on scene change.
					Util_hid_reset_key_state(HID_KEY_BIT_ALL);
				}
				else if(DEF_VID_HID_TOGGLE_PLAYBACK_CFM(key))
				{
					if(vid_player.state == PLAYER_STATE_IDLE)//Play the video.
					{
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_PLAY_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
					}
					//If player state is one of them, pause the video.
					//1. State is playing.
					//2. State is buffering and sub state contains resume later flag.
					//3. State is prepare seeking and sub state contains resume later flag.
					//4. State is seeking and sub state contains resume later flag.
					else if(vid_player.state == PLAYER_STATE_PLAYING
					|| ((vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
					|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING)))
					{
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
					}
					//If player state is one of them, resume the video.
					//1. State is pause.
					//2. State is buffering and sub state doesn't contain resume later flag.
					//3. State is prepare seeking and sub state doesn't contain resume later flag.
					//4. State is seeking and sub state doesn't contain resume later flag.
					else if(vid_player.state == PLAYER_STATE_PAUSE
					|| (!(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
					|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING)))
					{
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
					}
				}
				else if(DEF_VID_HID_ABORT_PLAYBACK_CFM(key))//Abort the playback.
				{
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
					DECODE_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_SEND_TO_FRONT), (result == DEF_SUCCESS), result);
				}
				else if(DEF_VID_HID_OPEN_EXPL_CFM(key))
				{
					Util_expl_set_show_flag(true);
					Util_expl_set_callback(Vid_callback);
					Util_expl_set_cancel_callback(Vid_cancel);
					//Reset key state on scene change.
					Util_hid_reset_key_state(HID_KEY_BIT_ALL);
				}
				else if(DEF_VID_HID_OPEN_MENU_CFM(key))
				{
					if(vid_player.menu_mode == DEF_VID_MENU_NONE)
					{
						vid_player.ui_y_offset_max = -230;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_SETTINGS_0;
					}
					else
					{
						vid_player.ui_y_offset_max = 0;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_NONE;
					}
					//Reset key state on scene change.
					Util_hid_reset_key_state(HID_KEY_BIT_ALL);
				}
				else if(DEF_VID_HID_CONTROL_CFM(key))
				{
					vid_player.is_displaying_controls = true;
					//Reset key state on scene change.
					Util_hid_reset_key_state(HID_KEY_BIT_ALL);
				}
				else if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_0)
				{
					//Menu mode button.
					if(DEF_VID_HID_OPEN_SETTING_1_CFM(key))//Menu mode button.
					{
						vid_player.ui_y_offset_max = -100;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_SETTINGS_1;
						//Reset key state on scene change.
						Util_hid_reset_key_state(HID_KEY_BIT_ALL);
					}
					else if(DEF_VID_HID_OPEN_INFO_CFM(key))//Menu mode button.
					{
						vid_player.ui_y_offset_max = -95;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_INFO;
						//Reset key state on scene change.
						Util_hid_reset_key_state(HID_KEY_BIT_ALL);
					}
					else if(DEF_VID_HID_SE0_A_TRACK_SELECTION_CFM(key))//Audio track button.
						vid_player.is_selecting_audio_track = !vid_player.is_selecting_audio_track;
					else if(DEF_VID_HID_SE0_S_TRACK_SELECTION_CFM(key))//Subtitle track button.
						vid_player.is_selecting_subtitle_track = !vid_player.is_selecting_subtitle_track;
					else if(DEF_VID_HID_SE0_TEXTURE_FILTER_CFM(key))//Texture filter button.
					{
						vid_player.use_linear_texture_filter = !vid_player.use_linear_texture_filter;

						//Update texture filter.
						for(uint8_t i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
						{
							for(uint8_t k = 0; k < DEF_DECODER_MAX_VIDEO_TRACKS; k++)
								Vid_large_texture_set_filter(&vid_player.large_image[i][k], vid_player.use_linear_texture_filter);
						}
					}
					else if(DEF_VID_HID_SE0_ALLOW_SKIP_FRAMES_CFM(key))//Allow skip frames button.
					{
						vid_player.allow_skip_frames = !vid_player.allow_skip_frames;
						if(vid_player.allow_skip_key_frames)
							vid_player.allow_skip_key_frames = false;
					}
					else if(DEF_VID_HID_SE0_ALLOW_SKIP_KEY_FRAMES_CFM(key))//Allow skip key frames button.
						vid_player.allow_skip_key_frames = !vid_player.allow_skip_key_frames;
					else if(DEF_VID_HID_SE0_VOLUME_CFM(key))//Change volume button.
						vid_player.is_setting_volume = true;
					else if(DEF_VID_HID_SE0_SEEK_DURATION_CFM(key))//Change seek duration button.
						vid_player.is_setting_seek_duration = true;
					else if(DEF_VID_HID_SE0_CORRECT_ASPECT_RATIO_CFM(key))//Correct aspect ratio button.
					{
						vid_player.correct_aspect_ratio = !vid_player.correct_aspect_ratio;
						Vid_fit_to_screen(400, 225);
					}
					else if(DEF_VID_HID_SE0_MOVE_CONTENT_MODE_CFM(key))//Disable resize and move button.
					{
						if(vid_player.move_content_mode + 1 > DEF_VID_MOVE_SUBTITLE)
							vid_player.move_content_mode = DEF_VID_MOVE_DISABLE;
						else
							vid_player.move_content_mode++;
					}
					else if(DEF_VID_HID_SE0_SAVE_MEDIA_POS_CFM(key))//Remember video pos button.
						vid_player.remember_video_pos = !vid_player.remember_video_pos;
					else if(DEF_VID_HID_SE0_PLAYBACK_MODE_CFM(key))//Playback mode button.
					{
						if(vid_player.playback_mode + 1 > DEF_VID_RANDOM)
							vid_player.playback_mode = DEF_VID_NO_REPEAT;
						else
							vid_player.playback_mode++;
					}
					else if(DEF_VID_HID_SE0_RESUME_PLAYBACK_THRESHOLD_CFM(key))//Restart playback threshold button.
					{
						if(key.touch_x <= 12)
							vid_player.restart_playback_threshold = 0;
						else if(key.touch_x >= 302)
							vid_player.restart_playback_threshold = DEF_DECODER_MAX_RAW_IMAGE - 1;
						else
							vid_player.restart_playback_threshold = (DEF_DECODER_MAX_RAW_IMAGE - 1) * (key.touch_x - 12) / 290;
					}
				}
				else if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_1)
				{
					if(DEF_VID_HID_OPEN_SETTING_0_CFM(key))//Menu mode button.
					{
						vid_player.ui_y_offset_max = -230;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_SETTINGS_0;
						//Reset key state on scene change.
						Util_hid_reset_key_state(HID_KEY_BIT_ALL);
					}
					else if(DEF_VID_HID_OPEN_INFO_CFM(key))//Menu mode button.
					{
						vid_player.ui_y_offset_max = -95;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_INFO;
						//Reset key state on scene change.
						Util_hid_reset_key_state(HID_KEY_BIT_ALL);
					}
					else if(vid_player.state == PLAYER_STATE_IDLE)
					{
						if(DEF_VID_HID_SE1_AUDIO_CFM(key))//Disable audio button.
							vid_player.disable_audio = !vid_player.disable_audio;
						else if(DEF_VID_HID_SE1_VIDEO_CFM(key))//Disable video button.
							vid_player.disable_video = !vid_player.disable_video;
						else if(DEF_VID_HID_SE1_SUBTITLE_CFM(key))//Disable subtitle button.
							vid_player.disable_subtitle = !vid_player.disable_subtitle;
						else if(DEF_VID_HID_SE1_HW_DECODING_CFM(key) && DEF_SEM_MODEL_IS_NEW(state.console_model))//Hardware decoding button.
							vid_player.use_hw_decoding = !vid_player.use_hw_decoding;
						else if(DEF_VID_HID_SE1_HW_CONVERSION_CFM(key))//Hardware color conversion button.
							vid_player.use_hw_color_conversion = !vid_player.use_hw_color_conversion;
						else if(DEF_VID_HID_SE1_MULTI_THREAD_CFM(key))//Multi-threaded decoding button.
							vid_player.use_multi_threaded_decoding = !vid_player.use_multi_threaded_decoding;
						else if(DEF_VID_HID_SE1_LOWER_RESOLUTION_CFM(key))//Lower video resolution button.
						{
							if(vid_player.lower_resolution + 1 > 2)
								vid_player.lower_resolution = 0;
							else
								vid_player.lower_resolution++;
						}
					}
				}
				else if(vid_player.menu_mode == DEF_VID_MENU_INFO)
				{
					if(DEF_VID_HID_OPEN_SETTING_0_CFM(key))//Menu mode button.
					{
						vid_player.ui_y_offset_max = -230;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_SETTINGS_0;
						//Reset key state on scene change.
						Util_hid_reset_key_state(HID_KEY_BIT_ALL);
					}
					else if(DEF_VID_HID_OPEN_SETTING_1_CFM(key))//Menu mode button.
					{
						vid_player.ui_y_offset_max = -100;
						vid_player.ui_y_offset = 0;
						vid_player.menu_mode = DEF_VID_MENU_SETTINGS_1;
						//Reset key state on scene change.
						Util_hid_reset_key_state(HID_KEY_BIT_ALL);
					}
					else if(DEF_VID_HID_INFO_DECODING_GRAPH_CFM(key))//Decoding graph button.
						vid_player.show_decoding_graph = !vid_player.show_decoding_graph;
					else if(DEF_VID_HID_INFO_CONVERSION_GRAPH_CFM(key))//Color conversion graph button.
						vid_player.show_color_conversion_graph = !vid_player.show_color_conversion_graph;
					else if(DEF_VID_HID_INFO_BUFFER_GRAPH_CFM(key))//Packet buffer graph button.
						vid_player.show_packet_buffer_graph = !vid_player.show_packet_buffer_graph;
					else if(DEF_VID_HID_INFO_RAW_V_BUFFER_GRAPH_CFM(key))//Raw video buffer graph button.
						vid_player.show_raw_video_buffer_graph = !vid_player.show_raw_video_buffer_graph;
					else if(DEF_VID_HID_INFO_RAW_A_BUFFER_GRAPH_CFM(key))//Raw audio buffer graph button.
						vid_player.show_raw_audio_buffer_graph = !vid_player.show_raw_audio_buffer_graph;
				}
			}

			if(vid_player.move_content_mode != DEF_VID_MOVE_DISABLE)
			{
				int8_t size_changes = 0;
				double subtitle_size_changes = 0;
				double y_changes = 0;
				double x_changes = 0;
				double sar_width = (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1);
				double sar_height = (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1);
				double x_offset_min = -(vid_player.video_info[0].width * sar_width * vid_player.video_zoom);
				double y_offset_min = -(vid_player.video_info[0].height * sar_height * vid_player.video_zoom);

				if(DEF_VID_HID_MOVE_CONTENT_UP_CFM(key))
				{
					if(DEF_HID_HE_MT(key.c_down, 10000))
						y_changes = -(10 * config.scroll_speed);
					else if(DEF_HID_HE_MT(key.c_down, 4000))
						y_changes = -(7.5 * config.scroll_speed);
					else
						y_changes = -(5 * config.scroll_speed);
				}
				if(DEF_VID_HID_MOVE_CONTENT_DOWN_CFM(key))
				{
					if(DEF_HID_HE_MT(key.c_up, 10000))
						y_changes = (10 * config.scroll_speed);
					else if(DEF_HID_HE_MT(key.c_up, 4000))
						y_changes = (7.5 * config.scroll_speed);
					else
						y_changes = (5 * config.scroll_speed);
				}
				if(DEF_VID_HID_MOVE_CONTENT_LEFT_CFM(key))
				{
					if(DEF_HID_HE_MT(key.c_left, 10000))
						x_changes = -(10 * config.scroll_speed);
					else if(DEF_HID_HE_MT(key.c_left, 4000))
						x_changes = -(7.5 * config.scroll_speed);
					else
						x_changes = -(5 * config.scroll_speed);
				}
				if(DEF_VID_HID_MOVE_CONTENT_RIGHT_CFM(key))
				{
					if(DEF_HID_HE_MT(key.c_right, 10000))
						x_changes = (10 * config.scroll_speed);
					else if(DEF_HID_HE_MT(key.c_right, 4000))
						x_changes = (7.5 * config.scroll_speed);
					else
						x_changes = (5 * config.scroll_speed);
				}
				if(DEF_VID_HID_SHRINK_CONTENT_CFM(key))
				{
					if(DEF_HID_HE_MT(key.l, 6000))
					{
						size_changes = -(5 * config.scroll_speed);
						subtitle_size_changes = -(0.05 * config.scroll_speed);
					}
					else if(DEF_HID_HE_MT(key.l, 2000))
					{
						size_changes = -(3 * config.scroll_speed);
						subtitle_size_changes = -(0.01 * config.scroll_speed);
					}
					else
					{
						size_changes = -1;
						subtitle_size_changes = -(0.005 * config.scroll_speed);
					}
				}
				if(DEF_VID_HID_ENLARGE_CONTENT_CFM(key))
				{
					if(DEF_HID_HE_MT(key.r, 6000))
					{
						size_changes = (5 * config.scroll_speed);
						subtitle_size_changes = (0.05 * config.scroll_speed);
					}
					else if(DEF_HID_HE_MT(key.r, 2000))
					{
						size_changes = (3 * config.scroll_speed);
						subtitle_size_changes = (0.01 * config.scroll_speed);
					}
					else
					{
						size_changes = 1;
						subtitle_size_changes = (0.005 * config.scroll_speed);
					}
				}

				//Update position.
				if(vid_player.move_content_mode == DEF_VID_MOVE_VIDEO || vid_player.move_content_mode == DEF_VID_MOVE_BOTH)
				{
					double new_width = 0;
					double new_height = 0;

					vid_player.video_y_offset += y_changes;
					vid_player.video_x_offset += x_changes;
					Vid_change_video_size(size_changes);

					new_width = (double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) * vid_player.video_zoom;
					new_height = (double)vid_player.video_info[0].height * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1) * vid_player.video_zoom;

					if(vid_player.video_x_offset > 400)
						vid_player.video_x_offset = 400;
					else if(vid_player.video_x_offset < x_offset_min)
						vid_player.video_x_offset = x_offset_min;

					if(vid_player.video_y_offset > 480)
						vid_player.video_y_offset = 480;
					else if(vid_player.video_y_offset < y_offset_min)
						vid_player.video_y_offset = y_offset_min;

					//If video is too large or small, revert it.
					if(new_width < 20 || new_height < 20)
						Vid_change_video_size(abs(size_changes));
					else if(new_width > 2000 || new_height > 2000)
						Vid_change_video_size(-abs(size_changes));
				}
				if(vid_player.move_content_mode == DEF_VID_MOVE_SUBTITLE || vid_player.move_content_mode == DEF_VID_MOVE_BOTH)
				{
					vid_player.subtitle_y_offset += y_changes;
					vid_player.subtitle_x_offset += x_changes;
					vid_player.subtitle_zoom += subtitle_size_changes;

					if(vid_player.subtitle_x_offset > 400)
						vid_player.subtitle_x_offset = 400;
					else if(vid_player.subtitle_x_offset < x_offset_min)
						vid_player.subtitle_x_offset = x_offset_min;

					if(vid_player.subtitle_y_offset > 480)
						vid_player.subtitle_y_offset = 480;
					else if(vid_player.subtitle_y_offset < y_offset_min)
						vid_player.subtitle_y_offset = y_offset_min;

					if(vid_player.subtitle_zoom < 0.05)
						vid_player.subtitle_zoom = 0.05;
					else if(vid_player.subtitle_zoom > 10)
						vid_player.subtitle_zoom = 10;
				}
			}
		}

		if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PREPATE_PLAYING)
		{
			double current_bar_pos = 0;

			if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
				current_bar_pos = vid_player.seek_pos;
			else
				current_bar_pos = vid_player.media_current_pos;

			if(DEF_VID_HID_SEEK_BAR_PRE_CFM(key))
				vid_player.seek_pos_cache = vid_player.media_duration * (((double)key.touch_x - 5) / 310);
			else if(DEF_VID_HID_SEEK_BAR_CFM(key))
			{
				vid_player.seek_pos = vid_player.seek_pos_cache;

				//Seek the video.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
				DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
			}
			else if(DEF_VID_HID_FULL_SEEK_FWD_CFM(key))
			{
				if(current_bar_pos + (vid_player.seek_duration * 1000) > vid_player.media_duration)
					vid_player.seek_pos = vid_player.media_duration;
				else
					vid_player.seek_pos = current_bar_pos + (vid_player.seek_duration * 1000);

				//Seek the video.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
				DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
			}
			else if(DEF_VID_HID_FULL_SEEK_BACK_CFM(key))
			{
				if(current_bar_pos - (vid_player.seek_duration * 1000) < 0)
					vid_player.seek_pos = 0;
				else
					vid_player.seek_pos = current_bar_pos - (vid_player.seek_duration * 1000);

				//Seek the video.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
				DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
			}
		}

		//Scroll.
		if(DEF_VID_HID_SCROLL_BAR_CFM(key))
		{
			vid_player.ui_y_move = 0;

			if(key.touch_y < 50)
				vid_player.ui_y_offset = 0;
			else if(key.touch_y > 175)
				vid_player.ui_y_offset = vid_player.ui_y_offset_max;
			else
				vid_player.ui_y_offset = vid_player.ui_y_offset_max * (50 - key.touch_y) / -125;
		}
		else if(DEF_HID_PHY_PR(key.touch) || DEF_HID_PHY_HE(key.touch))
		{
			vid_player.ui_y_move = 0;

			if(vid_player.is_scroll_mode)
				vid_player.ui_y_move = key.touch_y_move * config.scroll_speed;
		}
		else
		{
			vid_player.ui_y_move -= (vid_player.ui_y_move * 0.025);
			if (vid_player.ui_y_move < 0.5 && vid_player.ui_y_move > -0.5)
				vid_player.ui_y_move = 0;
		}

		if(vid_player.ui_y_offset - vid_player.ui_y_move > 0)
			vid_player.ui_y_offset = 0;
		else if(vid_player.ui_y_offset - vid_player.ui_y_move < vid_player.ui_y_offset_max)
			vid_player.ui_y_offset = vid_player.ui_y_offset_max;
		else
			vid_player.ui_y_offset -= vid_player.ui_y_move;
	}

	//Notify user that button is NOT being pressed anymore.
	if(DEF_VID_HID_SCROLL_MODE_DESEL(key) || vid_player.is_selecting_audio_track
	|| vid_player.is_selecting_subtitle_track || vid_player.is_displaying_controls)
		vid_player.is_scroll_mode = false;
	if(DEF_VID_HID_SCROLL_BAR_DESEL(key))
		vid_player.scroll_bar.selected = false;
	if(DEF_VID_HID_SYSTEM_UI_DESEL(key) || vid_player.is_scroll_mode)
		Draw_get_bot_ui_button()->selected = false;
	if(DEF_VID_HID_CONTROL_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.control_button.selected = false;
	if(DEF_VID_HID_SEEK_BAR_DESEL(key))//We prioritize this over scroll mode.
		vid_player.seek_bar.selected = false;
	if(DEF_VID_HID_OPEN_SETTING_0_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.menu_button[0].selected = false;
	if(DEF_VID_HID_OPEN_SETTING_1_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.menu_button[1].selected = false;
	if(DEF_VID_HID_OPEN_INFO_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.menu_button[2].selected = false;
	if(DEF_VID_HID_A_TRACK_CONFIRM_DESEL(key))//No scroll exists in the sub window.
		vid_player.audio_track_ok_button.selected = false;
	if(DEF_VID_HID_A_TRACK_ITEM_DESEL(key))//No scroll exists in the sub window.
	{
		for(uint8_t i = 0; i < vid_player.num_of_audio_tracks; i++)
			vid_player.audio_track_button[i].selected = false;
	}
	if(DEF_VID_HID_S_TRACK_CONFIRM_DESEL(key))//No scroll exists in the sub window.
		vid_player.subtitle_track_ok_button.selected = false;
	if(DEF_VID_HID_S_TRACK_ITEM_DESEL(key))//No scroll exists in the sub window.
	{
		for(uint8_t i = 0; i < vid_player.num_of_subtitle_tracks; i++)
			vid_player.subtitle_track_button[i].selected = false;
	}
	if(DEF_VID_HID_CONTROL_CLOSE_DESEL(key))//No scroll exists in the sub window.
		vid_player.control_button.selected = false;
	if(DEF_VID_HID_SE0_A_TRACK_SELECTION_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.select_audio_track_button.selected = false;
	if(DEF_VID_HID_SE0_S_TRACK_SELECTION_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.select_subtitle_track_button.selected = false;
	if(DEF_VID_HID_SE0_TEXTURE_FILTER_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.texture_filter_button.selected = false;
	if(DEF_VID_HID_SE0_ALLOW_SKIP_FRAMES_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.allow_skip_frames_button.selected = false;
	if(DEF_VID_HID_SE0_ALLOW_SKIP_KEY_FRAMES_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.allow_skip_key_frames_button.selected = false;
	if(DEF_VID_HID_SE0_VOLUME_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.volume_button.selected = false;
	if(DEF_VID_HID_SE0_SEEK_DURATION_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.seek_duration_button.selected = false;
	if(DEF_VID_HID_SE0_CORRECT_ASPECT_RATIO_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.correct_aspect_ratio_button.selected = false;
	if(DEF_VID_HID_SE0_MOVE_CONTENT_MODE_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.move_content_button.selected = false;
	if(DEF_VID_HID_SE0_SAVE_MEDIA_POS_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.remember_video_pos_button.selected = false;
	if(DEF_VID_HID_SE0_PLAYBACK_MODE_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.playback_mode_button.selected = false;
	if(DEF_VID_HID_SE0_RESUME_PLAYBACK_THRESHOLD_DESEL(key))//We prioritize this over scroll mode.
		vid_player.restart_playback_threshold_bar.selected = false;
	if(DEF_VID_HID_SE1_AUDIO_DESEL(key) || vid_player.is_scroll_mode || vid_player.state != PLAYER_STATE_IDLE)
		vid_player.disable_audio_button.selected = false;
	if(DEF_VID_HID_SE1_VIDEO_DESEL(key) || vid_player.is_scroll_mode || vid_player.state != PLAYER_STATE_IDLE)
		vid_player.disable_video_button.selected = false;
	if(DEF_VID_HID_SE1_SUBTITLE_DESEL(key) || vid_player.is_scroll_mode || vid_player.state != PLAYER_STATE_IDLE)
		vid_player.disable_subtitle_button.selected = false;
	if(DEF_VID_HID_SE1_HW_DECODING_DESEL(key) || vid_player.is_scroll_mode || vid_player.state != PLAYER_STATE_IDLE)
		vid_player.use_hw_decoding_button.selected = false;
	if(DEF_VID_HID_SE1_HW_CONVERSION_DESEL(key) || vid_player.is_scroll_mode || vid_player.state != PLAYER_STATE_IDLE)
		vid_player.use_hw_color_conversion_button.selected = false;
	if(DEF_VID_HID_SE1_MULTI_THREAD_DESEL(key) || vid_player.is_scroll_mode || vid_player.state != PLAYER_STATE_IDLE)
		vid_player.use_multi_threaded_decoding_button.selected = false;
	if(DEF_VID_HID_SE1_LOWER_RESOLUTION_DESEL(key) || vid_player.is_scroll_mode || vid_player.state != PLAYER_STATE_IDLE)
		vid_player.lower_resolution_button.selected = false;
	if(DEF_VID_HID_INFO_DECODING_GRAPH_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.show_decode_graph_button.selected = false;
	if(DEF_VID_HID_INFO_CONVERSION_GRAPH_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.show_color_conversion_graph_button.selected = false;
	if(DEF_VID_HID_INFO_BUFFER_GRAPH_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.show_packet_buffer_graph_button.selected = false;
	if(DEF_VID_HID_INFO_RAW_V_BUFFER_GRAPH_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.show_raw_video_buffer_graph_button.selected = false;
	if(DEF_VID_HID_INFO_RAW_A_BUFFER_GRAPH_DESEL(key) || vid_player.is_scroll_mode)
		vid_player.show_raw_audio_buffer_graph_button.selected = false;

	if(Util_log_query_show_flag())
		Util_log_main(key);
}

void Vid_resume(void)
{
	vid_thread_suspend = false;
	vid_main_run = true;
	//Reset key state on scene change.
	Util_hid_reset_key_state(HID_KEY_BIT_ALL);
	Draw_set_refresh_needed(true);
	Menu_suspend();

	vid_player.next_frame_update_time = osGetTime() + vid_player.video_frametime;
}

void Vid_suspend(void)
{
	vid_thread_suspend = true;
	vid_main_run = false;
	Menu_resume();
}

uint32_t Vid_load_msg(const char* lang)
{
	char file_name[32] = { 0, };

	snprintf(file_name, sizeof(file_name), "vid_%s.txt", (lang ? lang : ""));
	return Util_load_msg(file_name, vid_msg, DEF_VID_NUM_OF_MSG);
}

void Vid_init(bool draw)
{
	DEF_LOG_STRING("Initializing...");
	uint32_t result = DEF_ERR_OTHER;
	Sem_state state = { 0, };

	Sem_get_state(&state);
	DEF_LOG_RESULT_SMART(result, Util_str_init(&vid_status), (result == DEF_SUCCESS), result);

	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_status.sequential_id, sizeof(vid_status.sequential_id));

	if(DEF_SEM_MODEL_IS_NEW(state.console_model) && Util_is_core_available(2))
		vid_init_thread = threadCreate(Vid_init_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	else
	{
		if(DEF_SEM_MODEL_IS_NEW(state.console_model))
			APT_SetAppCpuTimeLimit(80);
		else
			APT_SetAppCpuTimeLimit(70);

		vid_init_thread = threadCreate(Vid_init_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	}

	while(!vid_already_init)
	{
		if(draw)
			Vid_draw_init_exit_message();
		else
			Util_sleep(20000);
	}

	if(!DEF_SEM_MODEL_IS_NEW(state.console_model) || !Util_is_core_available(2))
		APT_SetAppCpuTimeLimit(10);

	DEF_LOG_RESULT_SMART(result, threadJoin(vid_init_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	threadFree(vid_init_thread);

	Util_str_clear(&vid_status);
	Vid_resume();

	DEF_LOG_STRING("Initialized.");
}

void Vid_exit(bool draw)
{
	DEF_LOG_STRING("Exiting...");
	uint32_t result = DEF_ERR_OTHER;

	vid_exit_thread = threadCreate(Vid_exit_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	while(vid_already_init)
	{
		if(draw)
			Vid_draw_init_exit_message();
		else
			Util_sleep(20000);
	}

	DEF_LOG_RESULT_SMART(result, threadJoin(vid_exit_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	threadFree(vid_exit_thread);

	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_status.sequential_id);
	Util_str_free(&vid_status);
	Draw_set_refresh_needed(true);

	DEF_LOG_STRING("Exited.");
}

void Vid_main(void)
{
	uint8_t subtitle_index = UINT8_MAX;
	uint32_t color = DEF_DRAW_BLACK;
	uint32_t disabled_color = DEF_DRAW_WEAK_BLACK;
	uint32_t back_color = DEF_DRAW_WHITE;
	double bitmap_subtitle_width = 0;
	double bitmap_subtitle_height = 0;
	double text_subtitle_width = 0;
	double text_subtitle_height = 0;
	double y_offset = 0;
	//Array 0 == for top screen left eye, array 1 == for top screen right eye.
	uint8_t image_index[DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, 0, };
	double image_width[DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, 0, };
	double image_height[DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, 0, };
	//Array 0 == for top screen left eye, array 1 == for top screen right eye, array 2 == for bottom screen.
	double video_x_offset[3] = { 0, 0, 0, };
	double video_y_offset[3] = { 0, 0, 0, };
	double bitmap_subtitle_x_offset[3] = { 0, 0, 0, };
	double bitmap_subtitle_y_offset[3] = { 0, 0, 0, };
	double text_subtitle_x_offset[3] = { 0, 0, 0, };
	double text_subtitle_y_offset[3] = { 0, 0, 0, };
	const char thread_mode[3][6] = { "none", "frame", "slice" };
	const char lower_resolution_mode[3][11] = { "OFF (x1.0)", "ON (x0.5)", "ON (x0.25)" };
	Draw_image_data background = Draw_get_empty_image();
	Watch_handle_bit watch_handle_bit = (DEF_WATCH_HANDLE_BIT_GLOBAL | DEF_WATCH_HANDLE_BIT_VIDEO_PLAYER);
	Sem_config config = { 0, };
	Sem_state state = { 0, };

	Sem_get_config(&config);
	Sem_get_state(&state);

	if (config.is_night)
	{
		color = DEF_DRAW_WHITE;
		disabled_color = DEF_DRAW_WEAK_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	//Assign previous frame index first.
	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		if(vid_player.next_draw_index[i] > 0)
			image_index[i] = vid_player.next_draw_index[i] - 1;
		else
			image_index[i] = DEF_VID_VIDEO_BUFFERS - 1;
	}

	if(vid_player.state == PLAYER_STATE_PLAYING && vid_player.num_of_video_tracks > 0)
	{
		uint64_t current_ts = osGetTime();

		if(vid_player.next_frame_update_time <= current_ts)
		{
			//Array 0 == for left eye, array 1 == for right eye.
			bool is_both_buffer_ready = false;
			bool is_buffer_full = false;
			uint8_t buffer_health[DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, 0, };
			double next_ts = 0;

			//Check for buffer health.
			for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
			{
				if(vid_player.next_draw_index[i] <= vid_player.next_store_index[i])
					buffer_health[i] = vid_player.next_store_index[i] - vid_player.next_draw_index[i];
				else
					buffer_health[i] = DEF_VID_VIDEO_BUFFERS - vid_player.next_draw_index[i] + vid_player.next_store_index[i];
			}

			if(vid_player.num_of_video_tracks >= 2)
			{
				is_both_buffer_ready = ((buffer_health[0] > 0) && (buffer_health[1] > 0));
				is_buffer_full = ((buffer_health[0] >= (DEF_VID_VIDEO_BUFFERS - 1)) || (buffer_health[1] >= (DEF_VID_VIDEO_BUFFERS - 1)));
			}
			else
			{
				is_both_buffer_ready = (buffer_health[0] > 0);
				is_buffer_full = (buffer_health[0] >= (DEF_VID_VIDEO_BUFFERS - 1));
			}

			//Update video frame if any of them is true :
			//1. Both buffer has at least 1 ready frame.
			//(If number of video tracks is 1, then if buffer for left eye has at least 1 ready frame.)
			//2. Any of buffer is full.
			if(is_both_buffer_ready || is_buffer_full)
			{
				bool wait = false;
				double video_delay = 0;
				double wait_threshold = 0;
				double force_wait_threshold = 0;

				if(vid_player.num_of_audio_tracks > 0 && vid_player.num_of_video_tracks > 0)
				{
					//We only use track 0 for delay checking.
					Util_sync_lock(&vid_player.delay_update_lock, UINT64_MAX);
					Vid_update_video_delay(0);
					if(vid_player.num_of_video_tracks > 1)
						Vid_update_video_delay(1);

					video_delay = vid_player.video_delay_ms[0][DEF_VID_DELAY_SAMPLES - 1];
					Util_sync_unlock(&vid_player.delay_update_lock);
				}
				else
					video_delay = 0;

				//Calc video frame drop threshold.
				wait_threshold = DEF_VID_WAIT_THRESHOLD(vid_player.video_frametime);
				if(vid_player.num_of_video_tracks >= 2)
					wait_threshold *= 2;

				force_wait_threshold = DEF_VID_FORCE_WAIT_THRESHOLD(vid_player.video_frametime);
				if(vid_player.num_of_video_tracks >= 2)
					force_wait_threshold *= 2;

				if(video_delay < wait_threshold)
				{
					if(vid_player.wait_threshold_exceeded_ts == 0)
						vid_player.wait_threshold_exceeded_ts = current_ts;

					if((current_ts - vid_player.wait_threshold_exceeded_ts) > DEF_VID_WAIT_THRESHOLD_ALLOWED_DURATION(vid_player.video_frametime))
						wait = true;
				}
				else
					vid_player.wait_threshold_exceeded_ts = 0;

				if(video_delay < force_wait_threshold)
					wait = true;

				if(wait && Util_speaker_get_available_buffer_num(0) > 0)
				{
					//Video is too fast, don't update a video frame to wait for audio.
				}
				else
				{
					for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
					{
						if(vid_player.num_of_video_tracks <= i)
							break;

						//If no buffer is available, don't update it.
						if(buffer_health[i] == 0)
							continue;

						//Update buffer index.
						image_index[i] = vid_player.next_draw_index[i];

						if(vid_player.next_draw_index[i] + 1 < DEF_VID_VIDEO_BUFFERS)
							vid_player.next_draw_index[i]++;
						else
							vid_player.next_draw_index[i] = 0;
					}

					Draw_set_refresh_needed(true);
					vid_player.vfps_cache++;
					vid_player.last_video_frame_updated_ts = current_ts;
				}

				//Update next frame update timestamp.
				next_ts = vid_player.next_frame_update_time + vid_player.video_frametime;
				if(vid_player.num_of_video_tracks >= 2)
					next_ts += vid_player.video_frametime;

				if(osGetTime() >= next_ts + (vid_player.video_frametime * 10))
					vid_player.next_frame_update_time = osGetTime() + vid_player.video_frametime;
				else
					vid_player.next_frame_update_time = next_ts;
			}
			else
			{
				//Unfortunately, we dropped frame since no frames are ready.
			}
		}
	}
	else
		vid_player.next_frame_update_time = osGetTime() + vid_player.video_frametime;

	//Update vps (video playback framerate).
	if(osGetTime() >= vid_player.next_vfps_update)
	{
		if(osGetTime() >= vid_player.next_vfps_update + 1000)
			vid_player.next_vfps_update = osGetTime() + 1000;
		else
			vid_player.next_vfps_update += 1000;

		vid_player.vps = vid_player.vfps_cache;
		vid_player.vfps_cache = 0;
	}

	//Calculate image size and drawing position.
	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		double sar_width_ratio = 0;
		double sar_height_ratio = 0;

		if(i >= vid_player.num_of_video_tracks)
			break;

		sar_width_ratio = (vid_player.correct_aspect_ratio ? vid_player.video_info[i].sar_width : 1);
		sar_height_ratio = (vid_player.correct_aspect_ratio ? vid_player.video_info[i].sar_height : 1);
		image_width[i] = vid_player.large_image[image_index[i]][i].image_width * sar_width_ratio * vid_player.video_zoom;
		image_height[i] = vid_player.large_image[image_index[i]][i].image_height * sar_height_ratio * vid_player.video_zoom;
	}

	for(uint8_t i = 0; i < 3; i++)
	{
		video_x_offset[i] = vid_player.video_x_offset;
		video_y_offset[i] = vid_player.video_y_offset;
	}

	vid_player._3d_slider_pos = osGet3DSliderState();

	if(Draw_is_3d_mode() && vid_player._3d_slider_pos > 0)
	{
		//Change video offset based on 3D slider bar position for 3D videos
		//so that user can manually adjust it with 3D slider bar.
		video_x_offset[0] -= ((vid_player._3d_slider_pos - 0.5) * image_width[0]) * 0.1;
		video_x_offset[1] += ((vid_player._3d_slider_pos - 0.5) * image_width[0]) * 0.1;
	}

	video_x_offset[2] -= 40;
	video_y_offset[2] -= 240;

	//Find subtitle index.
	for(uint8_t i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)
	{
		if(vid_player.media_current_pos >= vid_player.subtitle_data[i].start_time && vid_player.media_current_pos <= vid_player.subtitle_data[i].end_time)
		{
			subtitle_index = i;
			break;
		}
	}

	//Calculate subtitle size and drawing position.
	if(subtitle_index < DEF_VID_SUBTITLE_BUFFERS)
	{
		//Use of vid_player.video_zoom is intended.
		bitmap_subtitle_width = vid_player.subtitle_data[subtitle_index].bitmap_width * vid_player.video_zoom;
		bitmap_subtitle_height = vid_player.subtitle_data[subtitle_index].bitmap_height * vid_player.video_zoom;
		text_subtitle_width = 0.5 * vid_player.subtitle_zoom;
		text_subtitle_height = 0.5 * vid_player.subtitle_zoom;

		for(uint8_t i = 0; i < 3; i++)
		{
			bitmap_subtitle_x_offset[i] = (vid_player.subtitle_data[subtitle_index].bitmap_x * vid_player.video_zoom) + vid_player.video_x_offset + vid_player.subtitle_x_offset;
			bitmap_subtitle_y_offset[i] = (vid_player.subtitle_data[subtitle_index].bitmap_y * vid_player.video_zoom) + vid_player.video_y_offset + vid_player.subtitle_y_offset;
			text_subtitle_x_offset[i] = vid_player.subtitle_x_offset;
			text_subtitle_y_offset[i] = 195 + vid_player.subtitle_y_offset;
		}
	}

	bitmap_subtitle_x_offset[2] -= 40;
	bitmap_subtitle_y_offset[2] -= 240;
	text_subtitle_x_offset[2] -= 40;
	text_subtitle_y_offset[2] -= 240;

	//Update performance data every 100ms.
	if(osGetTime() >= vid_player.previous_ts + 100)
	{
		vid_player.previous_ts = osGetTime();
		vid_player.packet_buffer_list[319] = Util_decoder_get_available_packet_num(0);
		vid_player.raw_audio_buffer_list[319] = Util_speaker_get_available_buffer_num(0);
		vid_player.raw_video_buffer_list[319][0] = (vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_decoder_mvd_get_available_raw_image_num(0) : Util_decoder_video_get_available_raw_image_num(0, 0);
		vid_player.raw_video_buffer_list[319][1] = Util_decoder_video_get_available_raw_image_num(1, 0);

		for(uint16_t i = 1; i < 320; i++)
			vid_player.packet_buffer_list[i - 1] = vid_player.packet_buffer_list[i];

		for(uint16_t i = 1; i < 320; i++)
			vid_player.raw_audio_buffer_list[i - 1] = vid_player.raw_audio_buffer_list[i];

		for(uint16_t i = 1; i < 320; i++)
		{
			for(uint8_t k = 0; k < DEF_DECODER_MAX_VIDEO_TRACKS; k++)
				vid_player.raw_video_buffer_list[i - 1][k] = vid_player.raw_video_buffer_list[i][k];
		}
	}

	if(Util_err_query_show_flag())
		watch_handle_bit |= DEF_WATCH_HANDLE_BIT_ERR;
	if(Util_expl_query_show_flag())
		watch_handle_bit |= DEF_WATCH_HANDLE_BIT_EXPL;
	if(Util_log_query_show_flag())
		watch_handle_bit |= DEF_WATCH_HANDLE_BIT_LOG;

	Vid_control_full_screen();

	if(Util_watch_is_changed(watch_handle_bit) || Draw_is_refresh_needed() || !config.is_eco)
	{
		Str_data time_str[2] = { 0, };
		Str_data format_str = { 0, };

		Draw_set_refresh_needed(false);
		Util_str_init(&format_str);
		Draw_frame_ready();

		if(config.is_top_lcd_on)
		{
			Str_data top_center_msg = { 0, };
			Str_data bottom_left_msg = { 0, };
			Str_data bottom_center_msg = { 0, };

			Util_str_init(&top_center_msg);
			Util_str_init(&bottom_left_msg);
			Util_str_init(&bottom_center_msg);

			Draw_screen_ready(DRAW_SCREEN_TOP_LEFT, vid_player.is_full_screen ? DEF_DRAW_BLACK : back_color);

			if(!vid_player.is_full_screen)
				Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

			if(vid_player.state != PLAYER_STATE_IDLE)
			{
				//Draw videos.
				if(Util_sync_lock(&vid_player.texture_init_free_lock, 0) == DEF_SUCCESS)
				{
					Vid_large_texture_draw(&vid_player.large_image[image_index[0]][0], video_x_offset[0], video_y_offset[0], image_width[0], image_height[0]);
					Util_sync_unlock(&vid_player.texture_init_free_lock);
				}

				//Draw subtitles.
				if(subtitle_index < DEF_VID_SUBTITLE_BUFFERS)
				{
					if(Util_sync_lock(&vid_player.texture_init_free_lock, 0) == DEF_SUCCESS)
					{
						if(vid_player.subtitle_image[subtitle_index].subtex)
							Draw_texture(&vid_player.subtitle_image[subtitle_index], DEF_DRAW_NO_COLOR, bitmap_subtitle_x_offset[0], bitmap_subtitle_y_offset[0], bitmap_subtitle_width, bitmap_subtitle_height);

						Util_sync_unlock(&vid_player.texture_init_free_lock);
					}

					if(vid_player.subtitle_data[subtitle_index].text)
					{
						Draw_with_background_c(vid_player.subtitle_data[subtitle_index].text, text_subtitle_x_offset[0], text_subtitle_y_offset[0], text_subtitle_width, text_subtitle_height,
						DEF_DRAW_WHITE, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 400, 40, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
					}
				}
			}
			else
			{
				Draw_image_data banner = { .c2d = vid_player.banner[config.is_night], };

				Draw_texture(&banner, DEF_DRAW_NO_COLOR, 0, 15, 400, 225);
			}

			if(vid_player.is_full_screen && vid_player.turn_off_bottom_screen_count > 0 && vid_player.show_full_screen_msg)
			{
				//Display exit full screen message.
				Util_str_add(&top_center_msg, vid_msg[DEF_VID_FULL_SCREEN_MSG].buffer);
			}

			if(vid_player.show_screen_brightness_until >= osGetTime())
			{
				//Display current brightness.
				Util_str_format_append(&bottom_left_msg, "%s%" PRIu8 "/180", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_BRIGHTNESS_MSG]), config.top_lcd_brightness);
			}
			if(vid_player.show_current_pos_until >= osGetTime())
			{
				double current_bar_pos = 0;

				if(Util_str_has_data(&bottom_left_msg))
					Util_str_add(&bottom_left_msg, "\n");

				if(vid_player.seek_bar.selected)
					current_bar_pos = vid_player.seek_pos_cache / 1000;
				else if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					current_bar_pos = vid_player.seek_pos / 1000;
				else
					current_bar_pos = vid_player.media_current_pos / 1000;

				Util_convert_seconds_to_time(current_bar_pos, &time_str[0]);
				Util_convert_seconds_to_time((vid_player.media_duration / 1000), &time_str[1]);

				//Display current video pos.
				Util_str_format_append(&bottom_left_msg, "%s/%s", DEF_STR_NEVER_NULL(&time_str[0]), DEF_STR_NEVER_NULL(&time_str[1]));
			}

			if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
			{
				//Display seeking message.
				Util_str_add(&bottom_center_msg, vid_msg[DEF_VID_SEEKING_MSG].buffer);
			}
			if(vid_player.state == PLAYER_STATE_BUFFERING)
			{
				if(Util_str_has_data(&bottom_center_msg))
					Util_str_add(&bottom_center_msg, "\n");

				//Display decoding message.
				Util_str_add(&bottom_center_msg, vid_msg[DEF_VID_PROCESSING_VIDEO_MSG].buffer);
			}

			if(Util_str_has_data(&top_center_msg))
			{
				Draw_with_background(&top_center_msg, 0, 20, 0.45, 0.45, DEF_DRAW_WHITE, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER,
				400, 30, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
			}

			if(Util_str_has_data(&bottom_left_msg))
			{
				Draw_with_background(&bottom_left_msg, 0, 200, 0.45, 0.45, DEF_DRAW_WHITE, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_BOTTOM,
				400, 40, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
			}

			if(Util_str_has_data(&bottom_center_msg))
			{
				Draw_with_background(&bottom_center_msg, 0, 200, 0.5, 0.5, DEF_DRAW_WHITE, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_BOTTOM,
				400, 40, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
			}

			if(config.is_debug)
				Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

			if(Util_cpu_usage_query_show_flag())
				Util_cpu_usage_draw();

			if(Util_log_query_show_flag())
				Util_log_draw();

			if(Draw_is_3d_mode())
			{
				Draw_screen_ready(DRAW_SCREEN_TOP_RIGHT, vid_player.is_full_screen ? DEF_DRAW_BLACK : back_color);

				if(!vid_player.is_full_screen)
					Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

				if(vid_player.state != PLAYER_STATE_IDLE)
				{
					//Draw 3D videos (right eye).
					if(Util_sync_lock(&vid_player.texture_init_free_lock, 0) == DEF_SUCCESS)
					{
						Vid_large_texture_draw(&vid_player.large_image[image_index[1]][1], video_x_offset[1], video_y_offset[1], image_width[1], image_height[1]);
						Util_sync_unlock(&vid_player.texture_init_free_lock);
					}

					//Draw subtitles.
					if(subtitle_index < DEF_VID_SUBTITLE_BUFFERS)
					{
						if(Util_sync_lock(&vid_player.texture_init_free_lock, 0) == DEF_SUCCESS)
						{
							if(vid_player.subtitle_image[subtitle_index].subtex)
								Draw_texture(&vid_player.subtitle_image[subtitle_index], DEF_DRAW_NO_COLOR, bitmap_subtitle_x_offset[1], bitmap_subtitle_y_offset[1], bitmap_subtitle_width, bitmap_subtitle_height);

							Util_sync_unlock(&vid_player.texture_init_free_lock);
						}

						if(vid_player.subtitle_data[subtitle_index].text)
						{
							Draw_with_background_c(vid_player.subtitle_data[subtitle_index].text, text_subtitle_x_offset[1], text_subtitle_y_offset[1], text_subtitle_width, text_subtitle_height,
							DEF_DRAW_WHITE, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 400, 40, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
						}
					}
				}
				else
				{
					Draw_image_data banner = { .c2d = vid_player.banner[config.is_night], };

					Draw_texture(&banner, DEF_DRAW_NO_COLOR, 0, 15, 400, 225);
				}

				if(Util_str_has_data(&top_center_msg))
				{
					Draw_with_background(&top_center_msg, 0, 20, 0.45, 0.45, DEF_DRAW_WHITE, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER,
					400, 30, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
				}

				if(Util_str_has_data(&bottom_left_msg))
				{
					Draw_with_background(&bottom_left_msg, 0, 200, 0.45, 0.45, DEF_DRAW_WHITE, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_BOTTOM,
					400, 40, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
				}

				if(Util_str_has_data(&bottom_center_msg))
				{
					Draw_with_background(&bottom_center_msg, 0, 200, 0.5, 0.5, DEF_DRAW_WHITE, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_BOTTOM,
					400, 40, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
				}

				if(config.is_debug)
					Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

				if(Util_cpu_usage_query_show_flag())
					Util_cpu_usage_draw();

				if(Util_log_query_show_flag())
					Util_log_draw();
			}

			Util_str_free(&top_center_msg);
			Util_str_free(&bottom_left_msg);
			Util_str_free(&bottom_center_msg);
		}

		if(config.is_bottom_lcd_on)
		{
			if(state.console_model == DEF_SEM_MODEL_OLD2DS && vid_player.turn_off_bottom_screen_count == 0 && vid_player.is_full_screen)
				Draw_screen_ready(DRAW_SCREEN_BOTTOM, DEF_DRAW_BLACK);//OLD2DS can't turn only bottom screen off, so just fill with black.
			else
			{
				double current_bar_pos = 0;

				Draw_screen_ready(DRAW_SCREEN_BOTTOM, back_color);
				Draw_c(DEF_VID_VER, 0, 0, 0.425, 0.425, DEF_DRAW_GREEN);

				//Draw audio, video and subtitle codec info.
				Util_str_format(&format_str, "A : %s", vid_player.audio_info[vid_player.selected_audio_track].format_name);
				Draw(&format_str, 0, 10, 0.45, 0.45, color);

				Util_str_format(&format_str, "V : %s", vid_player.video_info[0].format_name);
				Draw(&format_str, 0, 19, 0.45, 0.45, color);

				Util_str_format(&format_str, "S : %s", vid_player.subtitle_info[vid_player.selected_subtitle_track].format_name);
				Draw(&format_str, 0, 28, 0.45, 0.45, color);

				Util_str_format(&format_str, "%" PRIu32 "x%" PRIu32 "(%" PRIu32 "x%" PRIu32 ")@%.2ffps",
				vid_player.video_info[0].width, vid_player.video_info[0].height, vid_player.video_info[0].codec_width,
				vid_player.video_info[0].codec_height, vid_player.video_info[0].framerate);
				Draw(&format_str, 0, 37, 0.45, 0.45, color);

				if(vid_player.state != PLAYER_STATE_IDLE)
				{
					//Draw videos.
					if(Util_sync_lock(&vid_player.texture_init_free_lock, 0) == DEF_SUCCESS)
					{
						Vid_large_texture_draw(&vid_player.large_image[image_index[0]][0], video_x_offset[2], video_y_offset[2], image_width[0], image_height[0]);
						Util_sync_unlock(&vid_player.texture_init_free_lock);
					}

					//Draw subtitles.
					if(subtitle_index < DEF_VID_SUBTITLE_BUFFERS)
					{
						if(Util_sync_lock(&vid_player.texture_init_free_lock, 0) == DEF_SUCCESS)
						{
							if(vid_player.subtitle_image[subtitle_index].subtex)
								Draw_texture(&vid_player.subtitle_image[subtitle_index], DEF_DRAW_NO_COLOR, bitmap_subtitle_x_offset[2], bitmap_subtitle_y_offset[2], bitmap_subtitle_width, bitmap_subtitle_height);

							Util_sync_unlock(&vid_player.texture_init_free_lock);
						}

						if(vid_player.subtitle_data[subtitle_index].text)
						{
							Draw_with_background_c(vid_player.subtitle_data[subtitle_index].text, text_subtitle_x_offset[2], text_subtitle_y_offset[2], text_subtitle_width, text_subtitle_height,
							DEF_DRAW_WHITE, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 400, 40, DRAW_BACKGROUND_UNDER_TEXT, &background, 0xA0000000);
						}
					}
				}

				if(vid_player.menu_mode != DEF_VID_MENU_NONE)
					Draw_texture(&background, DEF_DRAW_WEAK_GREEN, 0, 50, 320, 130);

				if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_0)
				{
					//Scroll bar.
					Draw_texture(&vid_player.scroll_bar, vid_player.scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_player.ui_y_offset / vid_player.ui_y_offset_max * 120) + 50, 7, 10);

					y_offset = 60;
					//Playback mode.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						uint8_t current_playback_mode = DEF_VID_NO_REPEAT_MSG + vid_player.playback_mode;

						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_PLAY_METHOD_MSG]), DEF_STR_NEVER_NULL(&vid_msg[current_playback_mode]));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.playback_mode_button, vid_player.playback_mode_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.playback_mode_button.x_size = -1;
						vid_player.playback_mode_button.y_size = -1;
					}

					y_offset += 25;
					//Volume.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						uint32_t temp_color = ((vid_player.sub_state & PLAYER_SUB_STATE_TOO_BIG) ? DEF_DRAW_RED : color);

						Util_str_format(&format_str, "%s%" PRIu32 "%%", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_VOLUME_MSG]), vid_player.volume);
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, temp_color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.volume_button, vid_player.volume_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.volume_button.x_size = -1;
						vid_player.volume_button.y_size = -1;
					}

					y_offset += 25;
					//Select audio track.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw_with_background(&vid_msg[DEF_VID_AUDIO_TRACK_MSG], 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.select_audio_track_button, vid_player.select_audio_track_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.select_audio_track_button.x_size = -1;
						vid_player.select_audio_track_button.y_size = -1;
					}

					y_offset += 25;
					//Select subtitle track.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw_with_background(&vid_msg[DEF_VID_SUBTITLE_TRACK_MSG], 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.select_subtitle_track_button, vid_player.select_subtitle_track_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.select_subtitle_track_button.x_size = -1;
						vid_player.select_subtitle_track_button.y_size = -1;
					}

					y_offset += 25;
					//Seek duration.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%" PRIu32 "s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_SEEK_MSG]), vid_player.seek_duration);
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.seek_duration_button, vid_player.seek_duration_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.seek_duration_button.x_size = -1;
						vid_player.seek_duration_button.y_size = -1;
					}

					y_offset += 25;
					//Remember video pos.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_REMEMBER_POS_MSG]), (vid_player.remember_video_pos ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.remember_video_pos_button, vid_player.remember_video_pos_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.remember_video_pos_button.x_size = -1;
						vid_player.remember_video_pos_button.y_size = -1;
					}

					y_offset += 25;
					//Texture filter.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_TEX_FILTER_MSG]), (vid_player.use_linear_texture_filter ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.texture_filter_button, vid_player.texture_filter_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.texture_filter_button.x_size = -1;
						vid_player.texture_filter_button.y_size = -1;
					}

					y_offset += 25;
					//Correct aspect ratio.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_ASPECT_RATIO_MSG]), (vid_player.correct_aspect_ratio ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.correct_aspect_ratio_button, vid_player.correct_aspect_ratio_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.correct_aspect_ratio_button.x_size = -1;
						vid_player.correct_aspect_ratio_button.y_size = -1;
					}

					y_offset += 25;
					//Move content mode.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						uint8_t current_move_content_mode = (DEF_VID_MOVE_MODE_EDIABLE_MSG + vid_player.move_content_mode);

						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_MOVE_MODE_MSG]), DEF_STR_NEVER_NULL(&vid_msg[current_move_content_mode]));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.move_content_button, vid_player.move_content_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.move_content_button.x_size = -1;
						vid_player.move_content_button.y_size = -1;
					}

					y_offset += 25;
					//Allow skip frames.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_SKIP_FRAME_MSG]), (vid_player.allow_skip_frames ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.allow_skip_frames_button, vid_player.allow_skip_frames_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.allow_skip_frames_button.x_size = -1;
						vid_player.allow_skip_frames_button.y_size = -1;
					}

					y_offset += 25;
					//Allow skip key frames.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						uint32_t temp_color = (vid_player.allow_skip_frames ? color : disabled_color);

						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_SKIP_KEY_FRAME_MSG]), (vid_player.allow_skip_key_frames ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, temp_color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 15,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.allow_skip_key_frames_button, vid_player.allow_skip_key_frames_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.allow_skip_key_frames_button.x_size = -1;
						vid_player.allow_skip_key_frames_button.y_size = -1;
					}

					y_offset += 35;
					//Restart playback threshold.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_RESTART_PLAYBACK_THRESHOLD_MSG]), vid_player.restart_playback_threshold);
						Draw(&format_str, 12.5, y_offset + vid_player.ui_y_offset - 15, 0.5, 0.5, color);
						Draw_texture(&background, DEF_DRAW_WEAK_BLACK, 12.5, y_offset + vid_player.ui_y_offset + 7.5, 300, 5);
						Draw_texture(&vid_player.restart_playback_threshold_bar, vid_player.restart_playback_threshold_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED,
						((double)vid_player.restart_playback_threshold / (DEF_DECODER_MAX_RAW_IMAGE - 1) * 290) + 12.5, y_offset + vid_player.ui_y_offset, 10, 20);
					}
					else
					{
						vid_player.restart_playback_threshold_bar.x_size = -1;
						vid_player.restart_playback_threshold_bar.y_size = -1;
					}

					Draw_texture(&background, DEF_DRAW_YELLOW, 0, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[1], vid_player.menu_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[2], vid_player.menu_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
				}
				else if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_1)
				{
					//Scroll bar.
					Draw_texture(&vid_player.scroll_bar, vid_player.scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_player.ui_y_offset / vid_player.ui_y_offset_max * 120) + 50, 7, 10);

					y_offset = 60;
					//Disable audio.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_DISABLE_AUDIO_MSG]), (vid_player.disable_audio ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 20,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.disable_audio_button, vid_player.disable_audio_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.disable_audio_button.x_size = -1;
						vid_player.disable_audio_button.y_size = -1;
					}

					y_offset += 30;
					//Disable video.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_DISABLE_VIDEO_MSG]), (vid_player.disable_video ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 20,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.disable_video_button, vid_player.disable_video_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.disable_video_button.x_size = -1;
						vid_player.disable_video_button.y_size = -1;
					}

					y_offset += 30;
					//Disable subtitle.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_DISABLE_SUBTITLE_MSG]), (vid_player.disable_subtitle ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 20,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.disable_subtitle_button, vid_player.disable_subtitle_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.disable_subtitle_button.x_size = -1;
						vid_player.disable_subtitle_button.y_size = -1;
					}

					y_offset += 30;
					//Use hw decoding.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_HW_DECODER_MSG]), (vid_player.use_hw_decoding ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425,
						(!DEF_SEM_MODEL_IS_NEW(state.console_model) || vid_player.state != PLAYER_STATE_IDLE) ? disabled_color : color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER,
						300, 20, DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.use_hw_decoding_button, vid_player.use_hw_decoding_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.use_hw_decoding_button.x_size = -1;
						vid_player.use_hw_decoding_button.y_size = -1;
					}

					y_offset += 30;
					//Use hw color conversion.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_HW_CONVERTER_MSG]), (vid_player.use_hw_color_conversion ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 20,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.use_hw_color_conversion_button, vid_player.use_hw_color_conversion_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.use_hw_color_conversion_button.x_size = -1;
						vid_player.use_hw_color_conversion_button.y_size = -1;
					}

					y_offset += 30;
					//Use multi-threaded decoding (in software decoding).
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_MULTI_THREAD_MSG]), (vid_player.use_multi_threaded_decoding ? "ON" : "OFF"));
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 20,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.use_multi_threaded_decoding_button, vid_player.use_multi_threaded_decoding_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.use_multi_threaded_decoding_button.x_size = -1;
						vid_player.use_multi_threaded_decoding_button.y_size = -1;
					}

					y_offset += 30;
					//Lower resolution.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Util_str_format(&format_str, "%s%s", DEF_STR_NEVER_NULL(&vid_msg[DEF_VID_LOWER_RESOLUTION_MSG]), lower_resolution_mode[vid_player.lower_resolution]);
						Draw_with_background(&format_str, 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 300, 20,
						DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.lower_resolution_button, vid_player.lower_resolution_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.lower_resolution_button.x_size = -1;
						vid_player.lower_resolution_button.y_size = -1;
					}

					Draw_texture(&vid_player.menu_button[0], vid_player.menu_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
					Draw_texture(&background, DEF_DRAW_YELLOW, 110, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[2], vid_player.menu_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
				}
				else if(vid_player.menu_mode == DEF_VID_MENU_INFO)
				{
					//Scroll bar.
					Draw_texture(&vid_player.scroll_bar, vid_player.scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_player.ui_y_offset / vid_player.ui_y_offset_max * 120) + 50, 7, 10);

					y_offset = 160;
					//Color conversion time.
					if(vid_player.show_color_conversion_graph)
					{
						for(uint16_t i = 0; i < 319; i++)
							Draw_line(i, y_offset - (vid_player.conversion_time_list[i] / 2) + vid_player.ui_y_offset, DEF_DRAW_BLUE, i + 1, y_offset - (vid_player.conversion_time_list[i + 1] / 2) + vid_player.ui_y_offset, DEF_DRAW_BLUE, 1);
					}
					//Decoding time.
					if(vid_player.show_decoding_graph)
					{
						for(uint16_t i = 0; i < 319; i++)
							Draw_line(i, y_offset - (vid_player.video_decoding_time_list[i] / 2) + vid_player.ui_y_offset, DEF_DRAW_RED, i + 1, y_offset - (vid_player.video_decoding_time_list[i + 1] / 2) + vid_player.ui_y_offset, DEF_DRAW_RED, 1);

						Draw_line(320 - ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_decoder_mvd_get_available_raw_image_num(0) : Util_decoder_video_get_available_raw_image_num(0, 0)),
						y_offset + vid_player.ui_y_offset - 110, DEF_DRAW_WEAK_RED, 320 - ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_decoder_mvd_get_available_raw_image_num(0) : Util_decoder_video_get_available_raw_image_num(0, 0)),
						y_offset + vid_player.ui_y_offset + 8, DEF_DRAW_WEAK_RED, 2);
					}
					//Compressed buffer.
					if(vid_player.show_packet_buffer_graph)
					{
						for(uint16_t i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_player.packet_buffer_list[i] / 3.0 + vid_player.ui_y_offset, 0xFFFF00FF, i + 1, y_offset - vid_player.packet_buffer_list[i + 1] / 3.0 + vid_player.ui_y_offset, 0xFFFF00FF, 1);
					}
					//Raw video buffer.
					if(vid_player.show_raw_video_buffer_graph)
					{
						for(uint16_t i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_player.raw_video_buffer_list[i][0] / 1.5 + vid_player.ui_y_offset, 0xFF2060FF, i + 1, y_offset - vid_player.raw_video_buffer_list[i + 1][0] / 1.5 + vid_player.ui_y_offset, 0xFF2060FF, 1);

						for(uint16_t i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_player.raw_video_buffer_list[i][1] / 1.5 + vid_player.ui_y_offset, 0xFF00DDFF, i + 1, y_offset - vid_player.raw_video_buffer_list[i + 1][1] / 1.5 + vid_player.ui_y_offset, 0xFF00DDFF, 1);
					}
					//Raw audio buffer.
					if(vid_player.show_raw_audio_buffer_graph)
					{
						for(uint16_t i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_player.raw_audio_buffer_list[i] / 6.0 + vid_player.ui_y_offset, 0xFF00A000, i + 1, y_offset - vid_player.raw_audio_buffer_list[i + 1] / 6.0 + vid_player.ui_y_offset, 0xFF00A000, 1);
					}

					//Bottom line.
					Draw_line(0, y_offset + vid_player.ui_y_offset, color, 320, y_offset + vid_player.ui_y_offset, color, 1);
					//Deadline.
					Draw_line(0, y_offset + vid_player.ui_y_offset - (vid_player.video_frametime / 2), 0xFF606060, 320, y_offset - (vid_player.video_frametime / 2) + vid_player.ui_y_offset, 0xFF606060, 1);

					//Key frame.
					if(vid_player.show_decoding_graph)
					{
						for(uint16_t i = 0; i < 319; i++)
						{
							if(vid_player.keyframe_list[i])
								Draw_line(i, y_offset + vid_player.ui_y_offset, disabled_color, i, y_offset - 110 + vid_player.ui_y_offset, disabled_color, 2);
						}
					}

					//Compressed buffer button.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						Util_str_format(&format_str, "Compressed buffer : %" PRIu16, Util_decoder_get_available_packet_num(0));
						Draw_texture(&vid_player.show_packet_buffer_graph_button, vid_player.show_packet_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 200, 10);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_packet_buffer_graph ? 0xFFFF00FF : color);
					}
					else
					{
						vid_player.show_packet_buffer_graph_button.x_size = -1;
						vid_player.show_packet_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					//Raw video and audio buffer button.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						uint16_t buffer_health = 0;
						uint32_t buffer_health_ms = 0;

						if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
							buffer_health = Util_decoder_mvd_get_available_raw_image_num(0);
						else
							buffer_health = Util_decoder_video_get_available_raw_image_num(0, 0);

						buffer_health_ms = (buffer_health * vid_player.video_frametime);

						Draw_texture(&vid_player.show_raw_video_buffer_graph_button, vid_player.show_raw_video_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 200, 10);

						Util_str_format(&format_str, "Raw video buffer : %" PRIu16 "(%" PRIu32 "ms)", buffer_health, buffer_health_ms);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_raw_video_buffer_graph ? 0xFF2060FF : color);

						Util_str_format(&format_str, "Frames : %" PRIu32, vid_player.total_frames);
						Draw(&format_str, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}
					else
					{
						vid_player.show_raw_video_buffer_graph_button.x_size = -1;
						vid_player.show_raw_video_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						uint8_t audio_ch = vid_player.audio_info[vid_player.selected_audio_track].ch;
						uint16_t buffer_health = Util_speaker_get_available_buffer_num(0);
						uint32_t buffer_health_ms = (Util_speaker_get_available_buffer_size(0) / 2);
						uint32_t samplerate = vid_player.audio_info[vid_player.selected_audio_track].sample_rate;

						if(audio_ch != 0 && samplerate != 0)
							buffer_health_ms = ((double)buffer_health_ms / audio_ch / samplerate * 1000);
						else
							buffer_health_ms = 0;

						Draw_texture(&vid_player.show_raw_audio_buffer_graph_button, vid_player.show_raw_audio_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 200, 10);

						Util_str_format(&format_str, "Raw audio buffer : %" PRIu16 "(%" PRIu32 "ms)", buffer_health, buffer_health_ms);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_raw_audio_buffer_graph ? 0xFF00A000 : color);
					}
					else
					{
						vid_player.show_raw_audio_buffer_graph_button.x_size = -1;
						vid_player.show_raw_audio_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					//Deadline text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						Util_str_format(&format_str, "Deadline : %.2fms", vid_player.video_frametime);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, 0xFF606060);
					}

					y_offset += 10;
					//Decoding time button.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
						Draw_texture(&vid_player.show_decode_graph_button, vid_player.show_decode_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 200, 10);
					else
					{
						vid_player.show_decode_graph_button.x_size = -1;
						vid_player.show_decode_graph_button.y_size = -1;
					}

					//Video decoding time and decoding mode text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						double decoding_time_avg = 0;
						uint8_t divisor = 90;

						for(uint16_t i = 319; i >= 230; i--)
						{
							if(vid_player.video_decoding_time_list[i] == 0)
							{
								divisor = 320 - (i + 1);
								break;
							}

							decoding_time_avg += vid_player.video_decoding_time_list[i];
						}

						if(divisor != 0)
							decoding_time_avg /= divisor;

						Util_str_format(&format_str, "Video decoding (avg) : %.3fms", decoding_time_avg);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_decoding_graph ? DEF_DRAW_RED : color);

						Util_str_format(&format_str, "Hw decoding : %s", ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? "yes" : "no"));
						Draw(&format_str, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					y_offset += 10;
					//Audio decoding time and thread mode text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						double decoding_time_avg = 0;
						uint8_t divisor = 90;
						uint8_t thread_mode_index = ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? 0 : vid_player.video_info[0].thread_type);
						uint8_t active_threads = (thread_mode_index ? vid_player.num_of_threads : 1);

						for(uint16_t i = 319; i >= 230; i--)
						{
							if(vid_player.audio_decoding_time_list[i] == 0)
							{
								divisor = 320 - (i + 1);
								break;
							}

							decoding_time_avg += vid_player.audio_decoding_time_list[i];
						}

						if(divisor != 0)
							decoding_time_avg /= divisor;

						Util_str_format(&format_str, "Audio decoding (avg) : %.3fms", decoding_time_avg);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_decoding_graph ? 0xFF800080 : color);

						Util_str_format(&format_str, "Threads : %" PRIu8 " (%s)", active_threads, thread_mode[thread_mode_index]);
						Draw(&format_str, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					y_offset += 10;
					//Color conversion button.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
						Draw_texture(&vid_player.show_color_conversion_graph_button, vid_player.show_color_conversion_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 200, 10);
					else
					{
						vid_player.show_color_conversion_graph_button.x_size = -1;
						vid_player.show_color_conversion_graph_button.y_size = -1;
					}

					//Color conversion time and conversion mode text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						bool is_hw = ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) || (vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION));
						double conversion_time_avg = 0;
						uint8_t divisor = 90;

						for(uint16_t i = 319; i >= 230; i--)
						{
							if(vid_player.conversion_time_list[i] == 0)
							{
								divisor = 320 - (i + 1);
								break;
							}

							conversion_time_avg += vid_player.conversion_time_list[i];
						}

						if(divisor != 0)
							conversion_time_avg /= divisor;

						Util_str_format(&format_str, "Color conversion (avg) : %.3fms", conversion_time_avg);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_color_conversion_graph ? DEF_DRAW_BLUE : color);

						Util_str_format(&format_str, "Hw conversion : %s", (is_hw ? "yes" : "no"));
						Draw(&format_str, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					y_offset += 10;
					//Decoding speed note.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 160)
					{
						Draw_c("The values below are unsuitable for benchmarking", 0, (y_offset + vid_player.ui_y_offset), 0.425, 0.425, color);
						Draw_c("if frame level multi-threaded decoding is enabled.", 0, (y_offset + vid_player.ui_y_offset + 10), 0.425, 0.425, color);
					}

					y_offset += 20;
					//Decoding speed.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 160)
					{
						double avg_fps = 0;
						double min_fps = 0;
						double max_fps = 0;
						double recent_avg_fps = 0;

						if(vid_player.total_frames != 0 && vid_player.decoding_max_time
						&& vid_player.decoding_min_time != 0 && vid_player.decoding_recent_total_time != 0)
						{
							avg_fps = (1000 / (vid_player.decoding_total_time / vid_player.total_frames));
							min_fps = (1000 / vid_player.decoding_max_time);
							max_fps = (1000 / vid_player.decoding_min_time);
							recent_avg_fps = (1000 / (vid_player.decoding_recent_total_time/ 90));
						}

						Util_str_format(&format_str, "Avg (90 frames) %.2f fps/thread", recent_avg_fps);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);

						Util_str_format(&format_str, "Min %.2f fps/thread", min_fps);
						Draw(&format_str, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);

						y_offset += 10;
						Util_str_format(&format_str, "Avg (all frames) %.2f fps/thread", avg_fps);
						Draw(&format_str, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);

						Util_str_format(&format_str, "Max %.2f fps/thread", max_fps);
						Draw(&format_str, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					Draw_texture(&vid_player.menu_button[0], vid_player.menu_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[1], vid_player.menu_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
					Draw_texture(&background, DEF_DRAW_YELLOW, 220, 180, 100, 8);
				}

				//Controls.
				Draw_with_background(&vid_msg[DEF_VID_CONTROLS_MSG], 167.5, 195, 0.425, 0.425, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 145, 10,
				DRAW_BACKGROUND_ENTIRE_BOX, &vid_player.control_button, vid_player.control_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);

				//Draw time bar.
				if(vid_player.seek_bar.selected)
					current_bar_pos = vid_player.seek_pos_cache / 1000;
				else if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					current_bar_pos = vid_player.seek_pos / 1000;
				else
					current_bar_pos = vid_player.media_current_pos / 1000;

				Util_convert_seconds_to_time(current_bar_pos, &time_str[0]);
				Util_convert_seconds_to_time((vid_player.media_duration / 1000), &time_str[1]);

				Util_str_format(&format_str, "%s/%s", DEF_STR_NEVER_NULL(&time_str[0]), DEF_STR_NEVER_NULL(&time_str[1]));
				Draw(&format_str, 10, 192.5, 0.5, 0.5, color);

				Draw_texture(&vid_player.seek_bar, DEF_DRAW_GREEN, 5, 210, 310, 10);
				if(vid_player.media_duration != 0)
					Draw_texture(&background, 0xFF800080, 5, 210, 310 * (current_bar_pos / (vid_player.media_duration / 1000)), 10);

				if(vid_player.is_displaying_controls)
				{
					Draw_image_data control = { .c2d = vid_player.control[config.is_night], };

					Draw_texture(&control, DEF_DRAW_NO_COLOR, 80, 20, 160, 160);
					if(strcmp(config.lang, "ro") == 0)
					{
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG], 122.5, 47.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 1], 122.5, 62.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 2], 122.5, 77.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 3], 122.5, 92.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 4], 135, 107.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 5], 122.5, 122.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 6], 132.5, 137.5, 0.425, 0.425, DEF_DRAW_BLACK);
					}
					else
					{
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG], 122.5, 47.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 1], 122.5, 62.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 2], 122.5, 77.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 3], 122.5, 92.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 4], 135, 107.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 5], 122.5, 122.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(&vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 6], 132.5, 137.5, 0.45, 0.45, DEF_DRAW_BLACK);
					}
				}

				if(vid_player.is_selecting_audio_track)
				{
					Draw_texture(&background, DEF_DRAW_GREEN, 40, 20, 240, 140);
					Draw(&vid_msg[DEF_VID_AUDIO_TRACK_DESCRIPTION_MSG], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

					for(uint8_t i = 0; i < vid_player.num_of_audio_tracks; i++)
					{
						Util_str_format(&format_str, "Track %" PRIu8 "(Lang:%s)", i, vid_player.audio_info[i].track_lang);
						Draw_texture(&vid_player.audio_track_button[i], vid_player.audio_track_button[i].selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 40, 40 + (i * 12), 240, 12);
						Draw(&format_str, 42.5, 40 + (i * 12), 0.475, 0.475, i == vid_player.selected_audio_track_cache ? DEF_DRAW_RED : color);
					}

					Draw_texture(&vid_player.audio_track_ok_button, vid_player.audio_track_ok_button.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
					Draw_c("OK", 152.5, 140, 0.425, 0.425, DEF_DRAW_BLACK);
				}

				if(vid_player.is_selecting_subtitle_track)
				{
					Draw_texture(&background, DEF_DRAW_GREEN, 40, 20, 240, 140);
					Draw(&vid_msg[DEF_VID_SUBTITLE_TRACK_DESCRIPTION_MSG], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

					for(uint8_t i = 0; i < vid_player.num_of_subtitle_tracks; i++)
					{
						Util_str_format(&format_str, "Track %" PRIu8 "(Lang:%s)", i, vid_player.subtitle_info[i].track_lang);
						Draw_texture(&vid_player.subtitle_track_button[i], vid_player.subtitle_track_button[i].selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 40, 40 + (i * 12), 240, 12);
						Draw(&format_str, 42.5, 40 + (i * 12), 0.475, 0.475, i == vid_player.selected_subtitle_track_cache ? DEF_DRAW_RED : color);
					}

					Draw_texture(&vid_player.subtitle_track_ok_button, vid_player.subtitle_track_ok_button.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
					Draw_c("OK", 152.5, 140, 0.425, 0.425, DEF_DRAW_BLACK);
				}

				if(Util_expl_query_show_flag())
					Util_expl_draw();

				if(Util_err_query_show_flag())
					Util_err_draw();

				Draw_bot_ui();
			}
		}

		Draw_apply_draw();

		Util_str_free(&format_str);
		Util_str_free(&time_str[0]);
		Util_str_free(&time_str[1]);
	}
	else
		gspWaitForVBlank();

	if(vid_player.is_setting_volume)
	{
		uint32_t result = DEF_ERR_OTHER;
		Str_data dummy = { 0, };
		Str_data init_text = { 0, };
		Str_data out = { 0, };

		Util_str_init(&dummy);
		Util_str_init(&init_text);
		Util_str_format(&init_text, "%" PRIu16, vid_player.volume);

		//Pause the video.
		DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
		DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);

		Util_keyboard_init(KEYBOARD_TYPE_NUMPAD, KEYBOARD_ACCEPTABLE_INPUT_NO_EMPTY, KEYBOARD_DISPLAY_BUTTON_MIDDLE, 3,
		&dummy, &init_text, KEYBOARD_PASSWORD_MODE_OFF, KEYBOARD_FEATURES_BIT_NONE);

		if(Util_keyboard_launch(&out, NULL) == DEF_SUCCESS)
			vid_player.volume = (uint16_t)Util_max(atoi((char*)out.buffer), 0);

		Util_str_free(&dummy);
		Util_str_free(&init_text);
		Util_str_free(&out);

		//Resume the video.
		DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
		DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);

		Draw_set_refresh_needed(true);
		Util_keyboard_exit();
		vid_player.is_setting_volume = false;
	}
	else if(vid_player.is_setting_seek_duration)
	{
		uint32_t result = DEF_ERR_OTHER;
		Str_data dummy = { 0, };
		Str_data init_text = { 0, };
		Str_data out = { 0, };

		Util_str_init(&dummy);
		Util_str_init(&init_text);
		Util_str_format(&init_text, "%" PRIu8, vid_player.seek_duration);

		//Pause the video.
		DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
		DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);

		Util_keyboard_init(KEYBOARD_TYPE_NUMPAD, KEYBOARD_ACCEPTABLE_INPUT_NO_EMPTY, KEYBOARD_DISPLAY_BUTTON_MIDDLE, 2,
		&dummy, &init_text, KEYBOARD_PASSWORD_MODE_OFF, KEYBOARD_FEATURES_BIT_NONE);

		if(Util_keyboard_launch(&out, NULL) == DEF_SUCCESS)
			vid_player.seek_duration = (uint8_t)Util_max(atoi((char*)out.buffer), 0);

		if(vid_player.seek_duration == 0)
			vid_player.seek_duration = 1;

		Util_str_free(&dummy);
		Util_str_free(&init_text);
		Util_str_free(&out);

		//Resume the video.
		DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
		DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);

		Draw_set_refresh_needed(true);
		Util_keyboard_exit();
		vid_player.is_setting_seek_duration = false;
	}

	Vid_update_sleep_policy();
}

static void Vid_draw_init_exit_message(void)
{
	uint32_t color = DEF_DRAW_BLACK;
	uint32_t back_color = DEF_DRAW_WHITE;
	Watch_handle_bit watch_handle_bit = (DEF_WATCH_HANDLE_BIT_GLOBAL | DEF_WATCH_HANDLE_BIT_VIDEO_PLAYER);
	Sem_config config = { 0, };
	Sem_state state = { 0, };

	Sem_get_config(&config);
	Sem_get_state(&state);

	if (config.is_night)
	{
		color = DEF_DRAW_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	//Check if we should update the screen.
	if(Util_watch_is_changed(watch_handle_bit) || Draw_is_refresh_needed() || !config.is_eco)
	{
		Draw_set_refresh_needed(false);
		Draw_frame_ready();

		Draw_screen_ready(DRAW_SCREEN_TOP_LEFT, back_color);

		if(Util_log_query_show_flag())
			Util_log_draw();

		Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

		if(config.is_debug)
			Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

		if(Util_cpu_usage_query_show_flag())
			Util_cpu_usage_draw();

		Draw(&vid_status, 0, 20, 0.65, 0.65, color);

		//Draw the same things on right screen if 3D mode is enabled.
		//So that user can easily see them.
		if(Draw_is_3d_mode())
		{
			Draw_screen_ready(DRAW_SCREEN_TOP_RIGHT, back_color);

			if(Util_log_query_show_flag())
				Util_log_draw();

			Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

			if(config.is_debug)
				Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

			if(Util_cpu_usage_query_show_flag())
				Util_cpu_usage_draw();

			Draw(&vid_status, 0, 20, 0.65, 0.65, color);
		}

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();
}

static uint32_t Vid_get_min_texture_size(uint32_t width_or_height)
{
	if(width_or_height <= 16)
		return 16;
	else if(width_or_height <= 32)
		return 32;
	else if(width_or_height <= 64)
		return 64;
	else if(width_or_height <= 128)
		return 128;
	else if(width_or_height <= 256)
		return 256;
	else if(width_or_height <= 512)
		return 512;
	else
		return 1024;
}

static void Vid_large_texture_free(Large_image* large_image_data)
{
	if(!large_image_data || !large_image_data->images)
		return;

	for(uint16_t i = 0; i < large_image_data->num_of_images; i++)
		Draw_texture_free(&large_image_data->images[i]);

	large_image_data->image_width = 0;
	large_image_data->image_height = 0;
	large_image_data->num_of_images = 0;
	free(large_image_data->images);
	large_image_data->images = NULL;
}

static void Vid_large_texture_set_filter(Large_image* large_image_data, bool filter)
{
	if(!large_image_data || !large_image_data->images)
		return;

	for(uint16_t i = 0; i < large_image_data->num_of_images; i++)
		Draw_set_texture_filter(&large_image_data->images[i], filter);
}

static void Vid_large_texture_crop(Large_image* large_image_data, uint32_t width, uint32_t height)
{
	uint32_t width_offset = 0;
	uint32_t height_offset = 0;

	if(!large_image_data || !large_image_data->images
	|| (width == large_image_data->image_width && height == large_image_data->image_height))
		return;

	for(uint16_t i = 0; i < large_image_data->num_of_images; i++)
	{
		float texture_width = large_image_data->images[i].c2d.tex->width;
		float texture_height = large_image_data->images[i].c2d.tex->height;

		if(width_offset + texture_width >= width)
		{
			//Crop for X direction.
			uint32_t new_width = 0;

			if(width > width_offset)
				new_width = (width - width_offset);

			large_image_data->images[i].subtex->width = new_width;
			large_image_data->images[i].subtex->right = new_width / texture_width;
		}

		if(height_offset + texture_height >= height)
		{
			//Crop for Y direction.
			uint32_t new_height = 0;

			if(height > height_offset)
				new_height = (height - height_offset);

			large_image_data->images[i].subtex->height = new_height;
			large_image_data->images[i].subtex->bottom = (texture_height - new_height) / texture_height;
		}

		//Update offset.
		width_offset += texture_width;
		if(width_offset >= large_image_data->image_width)
		{
			width_offset = 0;
			height_offset += texture_height;
		}
	}
}

static uint32_t Vid_large_texture_init(Large_image* large_image_data, uint32_t width, uint32_t height, Raw_pixel color_format, bool zero_initialize)
{
	uint16_t loop = 0;
	uint32_t width_offset = 0;
	uint32_t height_offset = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(!large_image_data || width == 0 || height == 0)
		goto invalid_arg;

	//Calculate how many textures we need.
	if(width % 1024 > 0)
		loop = (width / 1024) + 1;
	else
		loop = (width / 1024);

	if(height % 1024 > 0)
		loop *= ((height / 1024) + 1);
	else
		loop *= (height / 1024);

	//Init parameters.
	large_image_data->image_width = 0;
	large_image_data->image_height = 0;
	large_image_data->num_of_images = 0;
	large_image_data->images = (Draw_image_data*)malloc(sizeof(Draw_image_data) * loop);
	if(!large_image_data->images)
		goto out_of_memory;

	for(uint32_t i = 0; i < loop; i++)
	{
		uint32_t texture_width = Vid_get_min_texture_size(width - width_offset);
		uint32_t texture_height = Vid_get_min_texture_size(height - height_offset);

		result = Draw_texture_init(&large_image_data->images[i], texture_width, texture_height, color_format);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Draw_texture_init, false, result);
			goto error_other;
		}

		if(zero_initialize)
		{
			uint8_t pixel_size = 0;

			if(color_format == RAW_PIXEL_RGB565LE)
				pixel_size = 2;
			else if(color_format == RAW_PIXEL_BGR888)
				pixel_size = 3;
			else if(color_format == RAW_PIXEL_ABGR8888)
				pixel_size = 4;

			memset(large_image_data->images[i].c2d.tex->data, 0x0, texture_width * texture_height * pixel_size);
		}
		large_image_data->num_of_images++;

		//Update offset.
		width_offset += 1024;
		if(width_offset >= width)
		{
			width_offset = 0;
			height_offset += 1024;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	Vid_large_texture_free(large_image_data);
	return DEF_ERR_OUT_OF_LINEAR_MEMORY;

	error_other:
	Vid_large_texture_free(large_image_data);
	return result;
}

static uint32_t Vid_large_texture_set_data(Large_image* large_image_data, uint8_t* raw_image, uint32_t width, uint32_t height, bool use_direct)
{
	uint16_t loop = 0;
	uint32_t width_offset = 0;
	uint32_t height_offset = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(!large_image_data || !large_image_data->images || large_image_data->num_of_images == 0 || !raw_image || width == 0 || height == 0)
		goto invalid_arg;

	large_image_data->image_width = width;
	large_image_data->image_height = height;

	if(use_direct)
	{
		result = Draw_set_texture_data_direct(&large_image_data->images[0], raw_image, width, height);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Draw_set_texture_data_direct, false, result);
			goto error_other;
		}
	}
	else
	{
		//Calculate how many textures we need.
		if(width % 1024 > 0)
			loop = (width / 1024) + 1;
		else
			loop = (width / 1024);

		if(height % 1024 > 0)
			loop *= ((height / 1024) + 1);
		else
			loop *= (height / 1024);

		if(loop > large_image_data->num_of_images)
			loop = large_image_data->num_of_images;

		for(uint16_t i = 0; i < loop; i++)
		{
			result = Draw_set_texture_data(&large_image_data->images[i], raw_image, width, height, width_offset, height_offset);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Draw_set_texture_data, false, result);
				goto error_other;
			}

			//Update offset.
			width_offset += 1024;
			if(width_offset >= width)
			{
				width_offset = 0;
				height_offset += 1024;
			}
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error_other:
	return result;
}

static void Vid_large_texture_draw(Large_image* large_image_data, double x_offset, double y_offset, double pic_width, double pic_height)
{
	uint32_t width_offset = 0;
	uint32_t height_offset = 0;
	double width_factor = 0;
	double height_factor = 0;

	if(!large_image_data || !large_image_data->images || large_image_data->num_of_images == 0
	|| large_image_data->image_width == 0 || large_image_data->image_height == 0)
		return;

	width_factor = pic_width / large_image_data->image_width;
	height_factor = pic_height / large_image_data->image_height;

	for(uint16_t i = 0; i < large_image_data->num_of_images; i++)
	{
		if(large_image_data->images[i].subtex)
		{
			double texture_x_offset = x_offset + (width_offset * width_factor);
			double texture_y_offset = y_offset + (height_offset * height_factor);
			double texture_width = large_image_data->images[i].subtex->width * width_factor;
			double texture_height = large_image_data->images[i].subtex->height * height_factor;
			Draw_texture(&large_image_data->images[i], DEF_DRAW_NO_COLOR, texture_x_offset, texture_y_offset, texture_width, texture_height);

			//Update offset.
			width_offset += large_image_data->images[i].c2d.tex->width;
			if(width_offset >= large_image_data->image_width)
			{
				width_offset = 0;
				height_offset += large_image_data->images[i].c2d.tex->height;
			}
		}
	}
}

static void Vid_fit_to_screen(uint16_t screen_width, uint16_t screen_height)
{
	if(vid_player.video_info[0].width != 0 && vid_player.video_info[0].height != 0 && vid_player.video_info[0].sar_width != 0 && vid_player.video_info[0].sar_height != 0)
	{
		//Fit to screen size.
		if((((double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1)) / screen_width) >= (((double)vid_player.video_info[0].height * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1)) / screen_height))
			vid_player.video_zoom = 1.0 / (((double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1)) / screen_width);
		else
			vid_player.video_zoom = 1.0 / (((double)vid_player.video_info[0].height * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1)) / screen_height);

		vid_player.video_x_offset = (screen_width - (vid_player.video_info[0].width * vid_player.video_zoom * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1))) / 2;
		vid_player.video_y_offset = (screen_height - (vid_player.video_info[0].height * vid_player.video_zoom * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1))) / 2;
		vid_player.video_y_offset += 240 - screen_height;
		vid_player.video_x_offset += 400 - screen_width;
	}
	vid_player.subtitle_x_offset = 0;
	vid_player.subtitle_y_offset = 0;
	vid_player.subtitle_zoom = 1;
}

static void Vid_change_video_size(double change_px)
{
	double current_width = (double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) * vid_player.video_zoom;
	if(vid_player.video_info[0].width != 0 && vid_player.video_info[0].height != 0 && vid_player.video_info[0].sar_width != 0 && vid_player.video_info[0].sar_height != 0)
		vid_player.video_zoom = 1.0 / ((double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) / (current_width + change_px));
}

static void Vid_enter_full_screen(uint32_t bottom_screen_timeout)
{
	vid_player.turn_off_bottom_screen_count = bottom_screen_timeout;
	vid_player.is_full_screen = true;
}

static void Vid_exit_full_screen(void)
{
	Sem_config config = { 0, };

	Sem_get_config(&config);

	vid_player.turn_off_bottom_screen_count = 0;
	vid_player.is_full_screen = false;

	config.is_bottom_lcd_on = true;
	config.bottom_lcd_brightness = config.top_lcd_brightness;
	Sem_set_config(&config);
}

static void Vid_increase_screen_brightness(void)
{
	Sem_config config = { 0, };

	Sem_get_config(&config);

	if(config.top_lcd_brightness + 1 <= 180)
	{
		config.top_lcd_brightness++;
		if(!vid_player.is_full_screen)
			config.bottom_lcd_brightness = config.top_lcd_brightness;

		vid_player.show_screen_brightness_until = osGetTime() + 2500;
		Sem_set_config(&config);
	}
}

static void Vid_decrease_screen_brightness(void)
{
	Sem_config config = { 0, };

	Sem_get_config(&config);

	if(config.top_lcd_brightness != 0)
	{
		config.top_lcd_brightness--;
		if(!vid_player.is_full_screen)
			config.bottom_lcd_brightness = config.top_lcd_brightness;

		vid_player.show_screen_brightness_until = osGetTime() + 2500;
		Sem_set_config(&config);
	}
}

static void Vid_control_full_screen(void)
{
	if(vid_player.turn_off_bottom_screen_count > 0)
	{
		Sem_config config = { 0, };
		Sem_state state = { 0, };

		Sem_get_config(&config);
		Sem_get_state(&state);

		vid_player.turn_off_bottom_screen_count--;
		if(vid_player.turn_off_bottom_screen_count == 0 && state.console_model != DEF_SEM_MODEL_OLD2DS)
			config.is_bottom_lcd_on = false;
		if(config.bottom_lcd_brightness > 10 && state.console_model != DEF_SEM_MODEL_OLD2DS)
			config.bottom_lcd_brightness--;

		Sem_set_config(&config);
	}
}

static void Vid_update_sleep_policy(void)
{
	//Disallow sleep if headset is connected and media is playing.
	if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PAUSE && osIsHeadsetConnected())
	{
		if(aptIsSleepAllowed())
			aptSetSleepAllowed(false);

		return;
	}
	else//Allow sleep if headset is not connected or no media is playing.
	{
		if(!aptIsSleepAllowed())
			aptSetSleepAllowed(true);

		return;
	}
}

static void Vid_update_performance_graph(double decoding_time, bool is_key_frame, double* total_delay)
{
	if(vid_player.video_frametime - decoding_time < 0 && vid_player.allow_skip_frames && vid_player.video_frametime != 0)
		*total_delay -= vid_player.video_frametime - decoding_time;

	if(vid_player.decoding_min_time > decoding_time)
		vid_player.decoding_min_time = decoding_time;
	if(vid_player.decoding_max_time < decoding_time)
		vid_player.decoding_max_time = decoding_time;

	vid_player.decoding_total_time += decoding_time;

	for(uint16_t i = 1; i < 320; i++)
	{
		vid_player.keyframe_list[i - 1] = vid_player.keyframe_list[i];
		vid_player.video_decoding_time_list[i - 1] = vid_player.video_decoding_time_list[i];
	}

	vid_player.total_frames++;
	vid_player.keyframe_list[319] = is_key_frame;
	vid_player.video_decoding_time_list[319] = decoding_time;
	vid_player.decoding_recent_total_time = 0;

	for(uint16_t i = 230; i < 320; i++)
		vid_player.decoding_recent_total_time += vid_player.video_decoding_time_list[i];
}

static void Vid_update_video_delay(uint8_t packet_index)
{
	uint8_t array_size = DEF_VID_DELAY_SAMPLES;
	uint8_t buffer_health = 0;
	uint16_t next_store_index = vid_player.next_store_index[packet_index];
	uint16_t next_draw_index = vid_player.next_draw_index[packet_index];
	uint64_t current_ts = osGetTime();
	double buffered_video_ms = 0;
	double total_delay = 0;

	if(next_draw_index <= next_store_index)
		buffer_health = next_store_index - next_draw_index;
	else
		buffer_health = DEF_VID_VIDEO_BUFFERS - next_draw_index + next_store_index;

	buffered_video_ms = buffer_health * vid_player.video_frametime;

	if((vid_player.last_video_frame_updated_ts + vid_player.video_frametime) > current_ts)
		buffered_video_ms += (vid_player.last_video_frame_updated_ts + vid_player.video_frametime) - current_ts;

	for(uint8_t i = 0; i < array_size - 1; i++)
		vid_player.video_delay_ms[packet_index][i] = vid_player.video_delay_ms[packet_index][i + 1];

	vid_player.video_delay_ms[packet_index][array_size - 1] = vid_player.audio_current_pos - (vid_player.video_current_pos[packet_index] - buffered_video_ms);

	for(uint8_t i = 0; i < array_size; i++)
		total_delay += vid_player.video_delay_ms[packet_index][i];

	vid_player.video_delay_avg_ms[packet_index] = (total_delay / array_size);
}

static void Vid_init_variable(void)
{
	Vid_init_settings();
	Vid_init_hidden_settings();
	Vid_init_debug_view_mode();
	Vid_init_debug_view_data();
	Vid_init_desync_data();
	Vid_init_player_data();
	Vid_init_media_data();
	Vid_init_video_data();
	Vid_init_audio_data();
	Vid_init_subtitle_data();
	Vid_init_ui_data();
}

static void Vid_init_settings(void)
{
	vid_player.remember_video_pos = false;
	vid_player.use_linear_texture_filter = true;
	vid_player.correct_aspect_ratio = true;
	vid_player.allow_skip_frames = false;
	vid_player.allow_skip_key_frames = false;
	vid_player.playback_mode = DEF_VID_NO_REPEAT;
	vid_player.move_content_mode = DEF_VID_MOVE_BOTH;
	vid_player.seek_duration = 5;
	vid_player.restart_playback_threshold = 48;
	vid_player.volume = 100;

	vid_player.disable_audio = false;
	vid_player.disable_video = false;
	vid_player.disable_subtitle = false;
	vid_player.use_hw_decoding = true;
	vid_player.use_hw_color_conversion = true;
	vid_player.use_multi_threaded_decoding = true;
	vid_player.lower_resolution = 0;

}

static void Vid_init_hidden_settings(void)
{
	bool frame_cores[4] = { false, false, false, false, };
	bool slice_cores[4] = { false, false, false, false, };
	Sem_state state = { 0, };

	Sem_get_state(&state);

	vid_player.show_full_screen_msg = true;
	vid_player.thread_mode = MEDIA_THREAD_TYPE_AUTO;
	vid_player.ram_to_keep_base = DEF_VID_RAM_TO_KEEP_BASE;

	if(DEF_SEM_MODEL_IS_NEW(state.console_model))
	{
		vid_player.num_of_threads = 2;

		for(uint8_t i = 0; i < 4; i++)
		{
			frame_cores[i] = Util_is_core_available(i);
			slice_cores[i] = Util_is_core_available(i);
		}

		vid_player.num_of_threads += Util_is_core_available(2);
		vid_player.num_of_threads += Util_is_core_available(3);
	}
	else
	{
		vid_player.num_of_threads = 2;

		for(uint8_t i = 0; i < 2; i++)
		{
			frame_cores[i] = Util_is_core_available(i);
			slice_cores[i] = Util_is_core_available(i);
		}
	}
	Util_decoder_video_set_enabled_cores(frame_cores, slice_cores);
}

static void Vid_init_debug_view_mode(void)
{
	vid_player.show_decoding_graph = true;
	vid_player.show_color_conversion_graph = true;
	vid_player.show_packet_buffer_graph = true;
	vid_player.show_raw_video_buffer_graph = true;
	vid_player.show_raw_audio_buffer_graph = true;
}

static void Vid_init_debug_view_data(void)
{
	vid_player.total_frames = 0;
	vid_player.total_rendered_frames = 0;
	vid_player.total_dropped_frames = 0;
	vid_player.previous_ts = 0;
	vid_player.decoding_min_time = 0xFFFFFFFF;
	vid_player.decoding_max_time = 0;
	vid_player.decoding_total_time = 0;
	vid_player.decoding_recent_total_time = 0;

	for(uint16_t i = 0 ; i < 320; i++)
	{
		vid_player.keyframe_list[i] = 0;
		vid_player.packet_buffer_list[i] = 0;
		for(uint8_t k = 0; k < DEF_DECODER_MAX_VIDEO_TRACKS; k++)
			vid_player.raw_video_buffer_list[i][k] = 0;

		vid_player.raw_audio_buffer_list[i] = 0;
		vid_player.video_decoding_time_list[i] = 0;
		vid_player.audio_decoding_time_list[i] = 0;
		vid_player.conversion_time_list[i] = 0;
	}

	for(uint8_t i = 0; i < 32; i++)
	{
		osTickCounterStart(&vid_player.decoding_time_tick[i]);
		vid_player.frame_list[i] = NULL;
	}
}

static void Vid_init_player_data(void)
{
	vid_player.state = PLAYER_STATE_IDLE;
	vid_player.sub_state = PLAYER_SUB_STATE_NONE;
	memset(vid_player.file.name, 0x0, sizeof(vid_player.file.name));
	memset(vid_player.file.directory, 0x0, sizeof(vid_player.file.directory));
	vid_player.file.index = 0;
}

static void Vid_init_desync_data(void)
{
	vid_player.wait_threshold_exceeded_ts = 0;
	vid_player.drop_threshold_exceeded_ts = 0;
	vid_player.last_video_frame_updated_ts = 0;
	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		vid_player.video_delay_avg_ms[i] = 0;
		for(uint16_t k = 0; k < DEF_VID_DELAY_SAMPLES; k++)
			vid_player.video_delay_ms[i][k] = 0;
	}
}

static void Vid_init_media_data(void)
{
	vid_player.media_duration = 0;
	vid_player.media_current_pos = 0;
	vid_player.seek_pos_cache = 0;
	vid_player.seek_pos = 0;
}

static void Vid_init_video_data(void)
{
	vid_player.num_of_video_tracks = 0;
	vid_player.next_frame_update_time = osGetTime();
	vid_player.next_vfps_update = osGetTime() + 1000;
	vid_player.vps = 0;
	vid_player.video_x_offset = 0;
	vid_player.video_y_offset = 15;
	vid_player.video_zoom = 1;
	vid_player.video_frametime = 0;
	vid_player._3d_slider_pos = osGet3DSliderState();

	for(uint8_t i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		vid_player.next_store_index[i] = 0;
		vid_player.next_draw_index[i] = 0;
		vid_player.video_current_pos[i] = 0;
		vid_player.video_info[i].width = 0;
		vid_player.video_info[i].height = 0;
		vid_player.video_info[i].codec_width = 0;
		vid_player.video_info[i].codec_height = 0;
		vid_player.video_info[i].framerate = 0;
		memset(vid_player.video_info[i].format_name, 0x0, sizeof(vid_player.video_info[i].format_name));
		memset(vid_player.video_info[i].short_format_name, 0x0, sizeof(vid_player.video_info[i].short_format_name));
		strcpy(vid_player.video_info[i].format_name, "n/a");
		strcpy(vid_player.video_info[i].short_format_name, "n/a");
		vid_player.video_info[i].duration = 0;
		vid_player.video_info[i].thread_type = MEDIA_THREAD_TYPE_NONE;
		vid_player.video_info[i].sar_width = 1;
		vid_player.video_info[i].sar_height = 1;
		vid_player.video_info[i].pixel_format = RAW_PIXEL_INVALID;
	}

	Util_sync_lock(&vid_player.texture_init_free_lock, UINT64_MAX);
	for(uint8_t i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
	{
		for(uint8_t k = 0; k < DEF_DECODER_MAX_VIDEO_TRACKS; k++)
			Vid_large_texture_free(&vid_player.large_image[i][k]);
	}
	Util_sync_unlock(&vid_player.texture_init_free_lock);
}

static void Vid_init_audio_data(void)
{
	vid_player.num_of_audio_tracks = 0;
	vid_player.selected_audio_track_cache = 0;
	vid_player.selected_audio_track = 0;
	vid_player.audio_current_pos = 0;
	vid_player.last_decoded_audio_pos = 0;

	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		vid_player.audio_info[i].bitrate = 0;
		vid_player.audio_info[i].sample_rate = 0;
		vid_player.audio_info[i].ch = 0;
		vid_player.audio_info[i].duration = 0;
		memset(vid_player.audio_info[i].format_name, 0x0, sizeof(vid_player.audio_info[i].format_name));
		memset(vid_player.audio_info[i].short_format_name, 0x0, sizeof(vid_player.audio_info[i].short_format_name));
		memset(vid_player.audio_info[i].track_lang, 0x0, sizeof(vid_player.audio_info[i].track_lang));
		strcpy(vid_player.audio_info[i].format_name, "n/a");
		strcpy(vid_player.audio_info[i].short_format_name, "n/a");
		strcpy(vid_player.audio_info[i].track_lang, "n/a");
		vid_player.audio_info[i].sample_format = RAW_SAMPLE_INVALID;
	}
}

static void Vid_init_subtitle_data(void)
{
	vid_player.num_of_subtitle_tracks = 0;
	vid_player.selected_subtitle_track_cache = 0;
	vid_player.selected_subtitle_track = 0;
	vid_player.subtitle_index = 0;
	vid_player.subtitle_x_offset = 0;
	vid_player.subtitle_y_offset = 0;
	vid_player.subtitle_zoom = 1;

	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
	{
		memset(vid_player.subtitle_info[i].format_name, 0x0, sizeof(vid_player.subtitle_info[i].format_name));
		memset(vid_player.subtitle_info[i].short_format_name, 0x0, sizeof(vid_player.subtitle_info[i].short_format_name));
		memset(vid_player.subtitle_info[i].track_lang, 0x0, sizeof(vid_player.subtitle_info[i].track_lang));
		strcpy(vid_player.subtitle_info[i].format_name, "n/a");
		strcpy(vid_player.subtitle_info[i].short_format_name, "n/a");
		strcpy(vid_player.subtitle_info[i].track_lang, "n/a");
	}

	for(uint8_t i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)
	{
		free(vid_player.subtitle_data[i].bitmap);
		free(vid_player.subtitle_data[i].text);
		vid_player.subtitle_data[i].bitmap = NULL;
		vid_player.subtitle_data[i].bitmap_width = 0;
		vid_player.subtitle_data[i].bitmap_height = 0;
		vid_player.subtitle_data[i].start_time = 0;
		vid_player.subtitle_data[i].end_time = 0;
		vid_player.subtitle_data[i].text = NULL;
	}
}

static void Vid_init_ui_data(void)
{
	vid_player.is_full_screen = false;
	vid_player.is_pause_for_home_menu = false;
	vid_player.is_selecting_audio_track = false;
	vid_player.is_selecting_subtitle_track = false;
	vid_player.is_setting_volume = false;
	vid_player.is_setting_seek_duration = false;
	vid_player.is_displaying_controls = false;
	vid_player.is_scroll_mode = false;
	vid_player.turn_off_bottom_screen_count = 0;
	vid_player.menu_mode = DEF_VID_MENU_NONE;
	vid_player.show_screen_brightness_until = 0;
	vid_player.show_current_pos_until = 0;
	vid_player.ui_y_offset_max = 0;
	vid_player.ui_y_offset = 0;
	vid_player.ui_y_move = 0;
}

void frame_worker_thread_start(const void* frame_handle)
{
	uint8_t index = UINT8_MAX;

	if(!frame_handle)
		return;

	Util_sync_lock(&vid_player.delay_update_lock, UINT64_MAX);
	//Search for stopwatch index using frame handle.
	for(uint8_t i = 0; i < 32; i++)
	{
		if(vid_player.frame_list[i] == frame_handle)
		{
			index = i;
			break;
		}
	}

	//Not registerd, find free space and register.
	if(index == UINT8_MAX)
	{
		for(uint8_t i = 0; i < 32; i++)
		{
			if(!vid_player.frame_list[i])
			{
				index = i;
				vid_player.frame_list[i] = frame_handle;
				break;
			}
		}
	}
	Util_sync_unlock(&vid_player.delay_update_lock);

	//No free spaces were found.
	if(index == UINT8_MAX)
		return;

	osTickCounterUpdate(&vid_player.decoding_time_tick[index]);
}

void frame_worker_thread_end(const void* frame_handle)
{
	uint8_t index = UINT8_MAX;
	double time = 0;
	double dummy = 0;

	if(!frame_handle)
		return;

	Util_sync_lock(&vid_player.delay_update_lock, UINT64_MAX);
	//Search for stopwatch index using frame handle.
	for(uint8_t i = 0; i < 32; i++)
	{
		if(vid_player.frame_list[i] == frame_handle)
		{
			index = i;
			//Unregister it.
			vid_player.frame_list[i] = NULL;
			break;
		}
	}
	Util_sync_unlock(&vid_player.delay_update_lock);

	//Not registerd.
	if(index == UINT8_MAX)
		return;

	osTickCounterUpdate(&vid_player.decoding_time_tick[index]);
	time = osTickCounterRead(&vid_player.decoding_time_tick[index]);
	Util_sync_lock(&vid_player.delay_update_lock, UINT64_MAX);
	Vid_update_performance_graph(time, false, &dummy);
	Util_sync_unlock(&vid_player.delay_update_lock);
}

void dav1d_worker_task_start(const void* frame_handle)
{
	frame_worker_thread_start(frame_handle);
}

void dav1d_worker_task_end(const void* frame_handle)
{
	frame_worker_thread_end(frame_handle);
}

void Vid_init_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint8_t* cache = NULL;
	uint32_t read_size = 0;
	uint32_t result = DEF_ERR_OTHER;
	Str_data out_data[17] = { 0, };
	Sem_state state = { 0, };

	Sem_get_state(&state);

	Util_str_set(&vid_status, "Initializing variables...");
	memset(&vid_player, 0x00, sizeof(Vid_player));
	Vid_init_variable();
	Vid_exit_full_screen();

	DEF_LOG_RESULT_SMART(result, Util_sync_create(&vid_player.texture_init_free_lock, SYNC_TYPE_NON_RECURSIVE_MUTEX), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_sync_create(&vid_player.delay_update_lock, SYNC_TYPE_NON_RECURSIVE_MUTEX), (result == DEF_SUCCESS), result);

	vid_player.banner_texture_handle = UINT32_MAX;
	vid_player.control_texture_handle = UINT32_MAX;

	vid_player.select_audio_track_button = Draw_get_empty_image();
	vid_player.texture_filter_button = Draw_get_empty_image();
	vid_player.allow_skip_frames_button = Draw_get_empty_image();
	vid_player.allow_skip_key_frames_button = Draw_get_empty_image();
	vid_player.volume_button = Draw_get_empty_image();
	vid_player.seek_duration_button = Draw_get_empty_image();
	vid_player.use_hw_decoding_button = Draw_get_empty_image();
	vid_player.use_hw_color_conversion_button = Draw_get_empty_image();
	vid_player.use_multi_threaded_decoding_button = Draw_get_empty_image();
	vid_player.lower_resolution_button = Draw_get_empty_image();
	vid_player.menu_button[0] = Draw_get_empty_image();
	vid_player.menu_button[1] = Draw_get_empty_image();
	vid_player.menu_button[2] = Draw_get_empty_image();
	vid_player.control_button = Draw_get_empty_image();
	vid_player.audio_track_ok_button = Draw_get_empty_image();
	vid_player.subtitle_track_ok_button = Draw_get_empty_image();
	vid_player.correct_aspect_ratio_button = Draw_get_empty_image();
	vid_player.move_content_button = Draw_get_empty_image();
	vid_player.remember_video_pos_button = Draw_get_empty_image();
	vid_player.show_decode_graph_button = Draw_get_empty_image();
	vid_player.show_color_conversion_graph_button = Draw_get_empty_image();
	vid_player.show_packet_buffer_graph_button = Draw_get_empty_image();
	vid_player.show_raw_video_buffer_graph_button = Draw_get_empty_image();
	vid_player.show_raw_audio_buffer_graph_button = Draw_get_empty_image();
	vid_player.scroll_bar = Draw_get_empty_image();
	vid_player.playback_mode_button = Draw_get_empty_image();
	vid_player.select_subtitle_track_button = Draw_get_empty_image();
	vid_player.disable_audio_button = Draw_get_empty_image();
	vid_player.disable_video_button = Draw_get_empty_image();
	vid_player.disable_subtitle_button = Draw_get_empty_image();
	vid_player.restart_playback_threshold_bar = Draw_get_empty_image();
	vid_player.seek_bar = Draw_get_empty_image();

	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		vid_player.audio_track_button[i] = Draw_get_empty_image();

	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		vid_player.subtitle_track_button[i] = Draw_get_empty_image();

	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.state, sizeof(&vid_player.state));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_selecting_audio_track, sizeof(vid_player.is_selecting_audio_track));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_selecting_subtitle_track, sizeof(vid_player.is_selecting_subtitle_track));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_displaying_controls, sizeof(vid_player.is_displaying_controls));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_setting_seek_duration, sizeof(vid_player.is_setting_seek_duration));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_setting_volume, sizeof(vid_player.is_setting_volume));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_decoding, sizeof(vid_player.use_hw_decoding));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_color_conversion, sizeof(vid_player.use_hw_color_conversion));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_multi_threaded_decoding, sizeof(vid_player.use_multi_threaded_decoding));

	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_linear_texture_filter, sizeof(vid_player.use_linear_texture_filter));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_frames, sizeof(vid_player.allow_skip_frames));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_key_frames, sizeof(vid_player.allow_skip_key_frames));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_full_screen, sizeof(vid_player.is_full_screen));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.correct_aspect_ratio, sizeof(vid_player.correct_aspect_ratio));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.move_content_mode, sizeof(vid_player.move_content_mode));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.remember_video_pos, sizeof(vid_player.remember_video_pos));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.playback_mode, sizeof(vid_player.playback_mode));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.menu_mode, sizeof(vid_player.menu_mode));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.lower_resolution, sizeof(vid_player.lower_resolution));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_decoding_graph, sizeof(vid_player.show_decoding_graph));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_color_conversion_graph, sizeof(vid_player.show_color_conversion_graph));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_packet_buffer_graph, sizeof(vid_player.show_packet_buffer_graph));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_video_buffer_graph, sizeof(vid_player.show_raw_video_buffer_graph));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_audio_buffer_graph, sizeof(vid_player.show_raw_audio_buffer_graph));

	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_pos_cache, sizeof(vid_player.seek_pos_cache));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_pos, sizeof(vid_player.seek_pos));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.ui_y_offset, sizeof(vid_player.ui_y_offset));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.ui_y_offset_max, sizeof(vid_player.ui_y_offset_max));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_x_offset, sizeof(vid_player.subtitle_x_offset));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_y_offset, sizeof(vid_player.subtitle_y_offset));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_zoom, sizeof(vid_player.subtitle_zoom));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player._3d_slider_pos, sizeof(vid_player._3d_slider_pos));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.video_x_offset, sizeof(vid_player.video_x_offset));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.video_y_offset, sizeof(vid_player.video_y_offset));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.video_zoom, sizeof(vid_player.video_zoom));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.selected_audio_track_cache, sizeof(vid_player.selected_audio_track_cache));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.selected_subtitle_track_cache, sizeof(vid_player.selected_subtitle_track_cache));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.restart_playback_threshold, sizeof(vid_player.restart_playback_threshold));

	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.audio_track_ok_button.selected, sizeof(vid_player.audio_track_ok_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_track_ok_button.selected, sizeof(vid_player.subtitle_track_ok_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.control_button.selected, sizeof(vid_player.control_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.scroll_bar.selected, sizeof(vid_player.scroll_bar.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.select_audio_track_button.selected, sizeof(vid_player.select_audio_track_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.select_subtitle_track_button.selected, sizeof(vid_player.select_subtitle_track_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.texture_filter_button.selected, sizeof(vid_player.texture_filter_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_frames_button.selected, sizeof(vid_player.allow_skip_frames_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_key_frames_button.selected, sizeof(vid_player.allow_skip_key_frames_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.volume_button.selected, sizeof(vid_player.volume_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_duration_button.selected, sizeof(vid_player.seek_duration_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.correct_aspect_ratio_button.selected, sizeof(vid_player.correct_aspect_ratio_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.move_content_button.selected, sizeof(vid_player.move_content_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.remember_video_pos_button.selected, sizeof(vid_player.remember_video_pos_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.playback_mode_button.selected, sizeof(vid_player.playback_mode_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.disable_audio_button.selected, sizeof(vid_player.disable_audio_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.disable_video_button.selected, sizeof(vid_player.disable_video_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.disable_subtitle_button.selected, sizeof(vid_player.disable_subtitle_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_decoding_button.selected, sizeof(vid_player.use_hw_decoding_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_color_conversion_button.selected, sizeof(vid_player.use_hw_color_conversion_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_multi_threaded_decoding_button.selected, sizeof(vid_player.use_multi_threaded_decoding_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.lower_resolution_button.selected, sizeof(vid_player.lower_resolution_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_decode_graph_button.selected, sizeof(vid_player.show_decode_graph_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_color_conversion_graph_button.selected, sizeof(vid_player.show_color_conversion_graph_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_packet_buffer_graph_button.selected, sizeof(vid_player.show_packet_buffer_graph_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_video_buffer_graph_button.selected, sizeof(vid_player.show_raw_video_buffer_graph_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_audio_buffer_graph_button.selected, sizeof(vid_player.show_raw_audio_buffer_graph_button.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.restart_playback_threshold_bar.selected, sizeof(vid_player.restart_playback_threshold_bar.selected));
	Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_bar.selected, sizeof(vid_player.seek_bar.selected));

	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.audio_track_button[i].selected, sizeof(vid_player.audio_track_button[i].selected));

	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_track_button[i].selected, sizeof(vid_player.subtitle_track_button[i].selected));

	for(uint8_t i = 0; i < 3; i++)
		Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.menu_button[i].selected, sizeof(vid_player.menu_button[i].selected));

	Util_str_add(&vid_status, "\nInitializing queue...");
	DEF_LOG_RESULT_SMART(result, Util_queue_create(&vid_player.decode_thread_command_queue, 200), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_queue_create(&vid_player.decode_thread_notification_queue, 100), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_queue_create(&vid_player.read_packet_thread_command_queue, 200), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_queue_create(&vid_player.decode_video_thread_command_queue, 200), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_queue_create(&vid_player.convert_thread_command_queue, 200), (result == DEF_SUCCESS), result);

	Util_str_add(&vid_status, "\nStarting threads...");
	vid_thread_run = true;
	if(DEF_SEM_MODEL_IS_NEW(state.console_model))
	{
		vid_player.decode_thread = threadCreate(Vid_decode_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_player.decode_video_thread = threadCreate(Vid_decode_video_thread, NULL, 1024 * 1024, DEF_THREAD_PRIORITY_HIGH, Util_is_core_available(2) ? 2 : 0, false);
		vid_player.convert_thread = threadCreate(Vid_convert_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 0, false);
		vid_player.read_packet_thread = threadCreate(Vid_read_packet_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 1, false);
	}
	else
	{
		vid_player.decode_thread = threadCreate(Vid_decode_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_player.decode_video_thread = threadCreate(Vid_decode_video_thread, NULL, 1024 * 1024, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_player.convert_thread = threadCreate(Vid_convert_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
		vid_player.read_packet_thread = threadCreate(Vid_read_packet_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 1, false);
	}

	Util_str_add(&vid_status, "\nLoading settings...");
	DEF_LOG_RESULT_SMART(result, Util_file_load_from_file("vid_settings.txt", DEF_MENU_MAIN_DIR, &cache, 0x1000, 0, &read_size), (result == DEF_SUCCESS), result);

	if(result == DEF_SUCCESS)
	{
		//Settings file for v1.5.1.
		DEF_LOG_RESULT_SMART(result, Util_parse_file((char*)cache, 17, out_data), (result == DEF_SUCCESS), result);

		if(result != DEF_SUCCESS)
		{
			//Settings file for v1.5.0.
			DEF_LOG_RESULT_SMART(result, Util_parse_file((char*)cache, 16, out_data), (result == DEF_SUCCESS), result);
			Util_str_set(&out_data[16], "48");
		}
		if(result != DEF_SUCCESS)
		{
			//Settings file for v1.4.2.
			DEF_LOG_RESULT_SMART(result, Util_parse_file((char*)cache, 13, out_data), (result == DEF_SUCCESS), result);
			Util_str_set(&out_data[13], "0");
			Util_str_set(&out_data[14], "0");
			Util_str_set(&out_data[15], "0");
			Util_str_set(&out_data[16], "48");
		}
		if(result != DEF_SUCCESS)
		{
			//Settings file for v1.3.2, v1.3.3, v1.4.0 and v1.4.1.
			DEF_LOG_RESULT_SMART(result, Util_parse_file((char*)cache, 12, out_data), (result == DEF_SUCCESS), result);
			Util_str_set(&out_data[12], "0");
			Util_str_set(&out_data[13], "0");
			Util_str_set(&out_data[14], "0");
			Util_str_set(&out_data[15], "0");
			Util_str_set(&out_data[16], "48");
		}
		if(result != DEF_SUCCESS)
		{
			//Settings file for v1.3.1.
			DEF_LOG_RESULT_SMART(result, Util_parse_file((char*)cache, 9, out_data), (result == DEF_SUCCESS), result);
			Util_str_set(&out_data[9], "1");
			Util_str_set(&out_data[10], "0");
			Util_str_set(&out_data[11], "1");
			Util_str_set(&out_data[12], "0");
			Util_str_set(&out_data[13], "0");
			Util_str_set(&out_data[14], "0");
			Util_str_set(&out_data[15], "0");
			Util_str_set(&out_data[16], "48");
		}
		if(result != DEF_SUCCESS)
		{
			//Settings file for v1.3.0.
			DEF_LOG_RESULT_SMART(result, Util_parse_file((char*)cache, 7, out_data), (result == DEF_SUCCESS), result);
			Util_str_set(&out_data[7], "100");
			Util_str_set(&out_data[8], "10");
			Util_str_set(&out_data[9], "1");
			Util_str_set(&out_data[10], "0");
			Util_str_set(&out_data[11], "1");
			Util_str_set(&out_data[12], "0");
			Util_str_set(&out_data[13], "0");
			Util_str_set(&out_data[14], "0");
			Util_str_set(&out_data[15], "0");
			Util_str_set(&out_data[16], "48");
		}
	}

	if(result == DEF_SUCCESS)
	{
		vid_player.use_linear_texture_filter = (out_data[0].buffer[0] == '1');
		vid_player.allow_skip_frames = (out_data[1].buffer[0] == '1');
		vid_player.allow_skip_key_frames = (out_data[2].buffer[0] == '1');
		vid_player.use_hw_decoding = (out_data[3].buffer[0] == '1');
		vid_player.use_hw_color_conversion = (out_data[4].buffer[0] == '1');
		vid_player.use_multi_threaded_decoding = (out_data[5].buffer[0] == '1');
		vid_player.lower_resolution = (uint8_t)Util_max(atoi((char*)out_data[6].buffer), 0);
		vid_player.volume = (uint16_t)Util_max(atoi((char*)out_data[7].buffer), 0);
		vid_player.seek_duration = (uint8_t)Util_max(atoi((char*)out_data[8].buffer), 0);
		vid_player.correct_aspect_ratio = (out_data[9].buffer[0] == '1');
		vid_player.move_content_mode = (uint8_t)Util_max(atoi((char*)out_data[10].buffer), 0);
		vid_player.remember_video_pos = (out_data[11].buffer[0] == '1');
		vid_player.playback_mode = (uint8_t)Util_max(atoi((char*)out_data[12].buffer), 0);
		vid_player.disable_audio = (out_data[13].buffer[0] == '1');
		vid_player.disable_video = (out_data[14].buffer[0] == '1');
		vid_player.disable_subtitle = (out_data[15].buffer[0] == '1');
		vid_player.restart_playback_threshold = (uint16_t)Util_max(atoi((char*)out_data[16].buffer), 0);
	}

	for(uint8_t i = 0; i < (sizeof(out_data) / sizeof(out_data[0])); i++)
		Util_str_free(&out_data[i]);

	if(!DEF_SEM_MODEL_IS_NEW(state.console_model))
		vid_player.use_hw_decoding = false;

	if(!vid_player.allow_skip_frames)
		vid_player.allow_skip_key_frames = false;

	if(vid_player.lower_resolution > 2)
		vid_player.lower_resolution = 0;

	if(vid_player.volume > 999)
		vid_player.volume = 100;

	if(vid_player.seek_duration > 99 || vid_player.seek_duration < 1)
		vid_player.seek_duration = 10;

	if(vid_player.playback_mode != DEF_VID_NO_REPEAT && vid_player.playback_mode != DEF_VID_REPEAT
		&& vid_player.playback_mode != DEF_VID_IN_ORDER && vid_player.playback_mode != DEF_VID_RANDOM)
		vid_player.playback_mode = DEF_VID_NO_REPEAT;

	if(vid_player.move_content_mode != DEF_VID_MOVE_BOTH && vid_player.move_content_mode != DEF_VID_MOVE_VIDEO
	&& vid_player.move_content_mode != DEF_VID_MOVE_SUBTITLE && vid_player.move_content_mode != DEF_VID_MOVE_DISABLE)
		vid_player.move_content_mode = DEF_VID_MOVE_BOTH;

	if(vid_player.restart_playback_threshold >= DEF_DECODER_MAX_RAW_IMAGE)
		vid_player.restart_playback_threshold = (48 >= DEF_DECODER_MAX_RAW_IMAGE ? DEF_DECODER_MAX_RAW_IMAGE : 48);

	free(cache);
	cache = NULL;

	Util_str_add(&vid_status, "\nLoading textures...");
	vid_player.banner_texture_handle = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture("romfs:/gfx/draw/video_player/banner.t3x",
	vid_player.banner_texture_handle, vid_player.banner, 0, 2), (result == DEF_SUCCESS), result);

	vid_player.control_texture_handle = Draw_get_free_sheet_num();

	DEF_LOG_RESULT_SMART(result, Draw_load_texture("romfs:/gfx/draw/video_player/control.t3x",
	vid_player.control_texture_handle, vid_player.control, 0, 2), (result == DEF_SUCCESS), result);

	vid_already_init = true;

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

void Vid_exit_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;
	Str_data data = { 0, };

	vid_already_init = false;
	vid_thread_suspend = false;
	vid_player.is_selecting_audio_track = false;
	vid_player.is_selecting_subtitle_track = false;

	DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
	DECODE_THREAD_SHUTDOWN_REQUEST, NULL, 100000, QUEUE_OPTION_SEND_TO_FRONT), (result == DEF_SUCCESS), result);

	Util_str_set(&vid_status, "Saving settings...");

	Util_str_init(&data);
	Util_str_format_append(&data, "<0>%" PRIu8 "</0>", vid_player.use_linear_texture_filter);
	Util_str_format_append(&data, "<1>%" PRIu8 "</1>", vid_player.allow_skip_frames);
	Util_str_format_append(&data, "<2>%" PRIu8 "</2>", vid_player.allow_skip_key_frames);
	Util_str_format_append(&data, "<3>%" PRIu8 "</3>", vid_player.use_hw_decoding);
	Util_str_format_append(&data, "<4>%" PRIu8 "</4>", vid_player.use_hw_color_conversion);
	Util_str_format_append(&data, "<5>%" PRIu8 "</5>", vid_player.use_multi_threaded_decoding);
	Util_str_format_append(&data, "<6>%" PRIu8 "</6>", vid_player.lower_resolution);
	Util_str_format_append(&data, "<7>%" PRIu16 "</7>", vid_player.volume);
	Util_str_format_append(&data, "<8>%" PRIu8 "</8>", vid_player.seek_duration);
	Util_str_format_append(&data, "<9>%" PRIu8 "</9>", vid_player.correct_aspect_ratio);
	Util_str_format_append(&data, "<10>%" PRIu8 "</10>", vid_player.move_content_mode);
	Util_str_format_append(&data, "<11>%" PRIu8 "</11>", vid_player.remember_video_pos);
	Util_str_format_append(&data, "<12>%" PRIu8 "</12>", vid_player.playback_mode);
	Util_str_format_append(&data, "<13>%" PRIu8 "</13>", vid_player.disable_audio);
	Util_str_format_append(&data, "<14>%" PRIu8 "</14>", vid_player.disable_video);
	Util_str_format_append(&data, "<15>%" PRIu8 "</15>", vid_player.disable_subtitle);
	Util_str_format_append(&data, "<16>%" PRIu16 "</16>", vid_player.restart_playback_threshold);

	Util_file_save_to_file("vid_settings.txt", DEF_MENU_MAIN_DIR, (uint8_t*)data.buffer, data.capacity, true);
	Util_str_free(&data);

	Util_str_add(&vid_status, "\nExiting threads...");
	DEF_LOG_RESULT_SMART(result, threadJoin(vid_player.decode_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, threadJoin(vid_player.decode_video_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, threadJoin(vid_player.convert_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, threadJoin(vid_player.read_packet_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);

	Util_str_add(&vid_status, "\nCleaning up...");
	threadFree(vid_player.decode_thread);
	threadFree(vid_player.decode_video_thread);
	threadFree(vid_player.convert_thread);
	threadFree(vid_player.read_packet_thread);

	Draw_free_texture(vid_player.banner_texture_handle);
	Draw_free_texture(vid_player.control_texture_handle);

	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.state);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_selecting_audio_track);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_selecting_subtitle_track);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_displaying_controls);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_setting_seek_duration);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_setting_volume);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_decoding);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_color_conversion);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_multi_threaded_decoding);

	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_linear_texture_filter);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_frames);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_key_frames);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.is_full_screen);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.correct_aspect_ratio);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.move_content_mode);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.remember_video_pos);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.playback_mode);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.menu_mode);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.lower_resolution);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_decoding_graph);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_color_conversion_graph);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_packet_buffer_graph);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_video_buffer_graph);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_audio_buffer_graph);

	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_pos_cache);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_pos);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.ui_y_offset);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.ui_y_offset_max);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_x_offset);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_y_offset);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_zoom);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player._3d_slider_pos);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.video_x_offset);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.video_y_offset);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.video_zoom);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.selected_audio_track_cache);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.selected_subtitle_track_cache);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.restart_playback_threshold);

	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.audio_track_ok_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_track_ok_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.control_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.scroll_bar.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.select_audio_track_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.select_subtitle_track_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.texture_filter_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_frames_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.allow_skip_key_frames_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.volume_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_duration_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.correct_aspect_ratio_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.move_content_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.remember_video_pos_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.playback_mode_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.disable_audio_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.disable_video_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.disable_subtitle_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_decoding_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_hw_color_conversion_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.use_multi_threaded_decoding_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.lower_resolution_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_decode_graph_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_color_conversion_graph_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_packet_buffer_graph_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_video_buffer_graph_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.show_raw_audio_buffer_graph_button.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.restart_playback_threshold_bar.selected);
	Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.seek_bar.selected);

	for(uint8_t i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.audio_track_button[i].selected);

	for(uint8_t i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.subtitle_track_button[i].selected);

	for(uint8_t i = 0; i < 3; i++)
		Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &vid_player.menu_button[i].selected);

	Util_queue_delete(&vid_player.decode_thread_command_queue);
	Util_queue_delete(&vid_player.decode_thread_notification_queue);
	Util_queue_delete(&vid_player.read_packet_thread_command_queue);
	Util_queue_delete(&vid_player.decode_video_thread_command_queue);
	Util_queue_delete(&vid_player.convert_thread_command_queue);

	Util_sync_destroy(&vid_player.texture_init_free_lock);
	Util_sync_destroy(&vid_player.delay_update_lock);

	vid_already_init = false;

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

void Vid_decode_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	bool is_eof = false;
	bool is_read_packet_thread_active = false;
	bool is_waiting_video_decoder = false;
	uint8_t backward_timeout = 20;
	uint8_t wait_count = 0;
	uint8_t dummy = 0;
	uint32_t result = 0;
	uint32_t audio_bar_pos = 0;
	double seek_start_pos = 0;
	Str_data cache_file_name = { 0, };

	Util_file_save_to_file(".", DEF_MENU_MAIN_DIR "saved_pos/", &dummy, 1, true);//Create directory.
	Util_str_init(&cache_file_name);

	while (vid_thread_run)
	{
		uint64_t timeout_us = DEF_THREAD_ACTIVE_SLEEP_TIME;
		void* message = NULL;
		Vid_command event = NONE_REQUEST;
		Vid_notification notification = NONE_NOTIFICATION;

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_THREAD_INACTIVE_SLEEP_TIME);
		}

		//If player state is not idle, don't wait for commands
		//as we want to decode audio/videos/subtitles as quick as possible.
		if(vid_player.state != PLAYER_STATE_IDLE)
			timeout_us = 0;

		//Reset afk count so that system won't go sleep.
		if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PAUSE)
			Util_hid_reset_afk_time();

		result = Util_queue_get(&vid_player.decode_thread_command_queue, (uint32_t*)&event, &message, timeout_us);
		if(result == DEF_SUCCESS)
		{
			switch (event)
			{
				case DECODE_THREAD_PLAY_REQUEST:
				{
					Vid_file* new_file = (Vid_file*)message;

					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
					{
						uint8_t num_of_audio_tracks = 0;
						uint8_t num_of_video_tracks = 0;
						uint8_t num_of_subtitle_tracks = 0;
						Str_data path = { 0, };

						Util_str_init(&path);
						Vid_init_debug_view_data();
						Vid_init_desync_data();
						Vid_init_media_data();
						Vid_init_video_data();
						Vid_init_audio_data();
						Vid_init_subtitle_data();
						vid_player.state = PLAYER_STATE_PLAYING;
						vid_player.sub_state = PLAYER_SUB_STATE_NONE;
						is_eof = false;
						is_read_packet_thread_active = false;
						is_waiting_video_decoder = false;

						//Update the file.
						if(new_file)
						{
							snprintf(vid_player.file.name, sizeof(vid_player.file.name), "%s", new_file->name);
							snprintf(vid_player.file.directory, sizeof(vid_player.file.directory), "%s", new_file->directory);
							vid_player.file.index = new_file->index;

							free(new_file);
							new_file = NULL;
						}

						seek_start_pos = 0;
						wait_count = 0;
						audio_bar_pos = 0;
						Util_str_clear(&cache_file_name);
						Util_str_set(&path, vid_player.file.directory);
						Util_str_add(&path, vid_player.file.name);

						//Generate cache file name.
						if(Util_str_has_data(&path))
						{
							for(uint32_t i = 0; i < path.length; i++)
							{
								//Replace "/" with "_".
								if(path.buffer[i] == '/')
									Util_str_add(&cache_file_name, "_");
								else
								{
									char temp_char[2] = { path.buffer[i], 0, };
									Util_str_add(&cache_file_name, temp_char);
								}
							}
						}

						DEF_LOG_RESULT_SMART(result, Util_decoder_open_file(path.buffer, &num_of_audio_tracks,
						&num_of_video_tracks, &num_of_subtitle_tracks, 0), (result == DEF_SUCCESS), result);

						Util_str_free(&path);

						if(result != DEF_SUCCESS)
							goto error;

						//Overwirte number of tracks if disable flag is set.
						if(vid_player.disable_audio)
						{
							num_of_audio_tracks = 0;
							strcpy(vid_player.audio_info[0].format_name, "disabled");
						}
						if(vid_player.disable_video)
						{
							num_of_video_tracks = 0;
							strcpy(vid_player.video_info[0].format_name, "disabled");
						}
						if(vid_player.disable_subtitle)
						{
							num_of_subtitle_tracks = 0;
							strcpy(vid_player.subtitle_info[0].format_name, "disabled");
						}

						if(num_of_audio_tracks > 0)
						{
							DEF_LOG_RESULT_SMART(result, Util_speaker_init(), (result == DEF_SUCCESS), result);
							if(result != DEF_SUCCESS)
							{
								DEF_LOG_RESULT(Util_speaker_init, false, result);
								Util_err_set_error_message(Util_err_get_error_msg(result), "You have to run dsp1 in order to listen to audio.\n(https://github.com/zoogie/DSP1/releases)", DEF_LOG_GET_SYMBOL(), result);
								Util_err_set_show_flag(true);
								//Continue initialization.
							}

							DEF_LOG_RESULT_SMART(result, Util_decoder_audio_init(num_of_audio_tracks, 0), (result == DEF_SUCCESS), result);
							if(result == DEF_SUCCESS)
							{
								uint8_t ch = 0;

								for(uint8_t i = 0; i < num_of_audio_tracks; i++)
									Util_decoder_audio_get_info(&vid_player.audio_info[i], i, 0);

								vid_player.selected_audio_track = 0;

								//3DS only supports up to 2ch.
								ch = (vid_player.audio_info[vid_player.selected_audio_track].ch > 2 ? 2 : vid_player.audio_info[vid_player.selected_audio_track].ch);
								DEF_LOG_RESULT_SMART(result, Util_speaker_set_audio_info(0, ch, vid_player.audio_info[vid_player.selected_audio_track].sample_rate), (result == DEF_SUCCESS), result);
							}

							if(result != DEF_SUCCESS)//If audio format is not supported, disable audio so that video can be played without audio.
							{
								Util_speaker_exit();
								vid_player.num_of_audio_tracks = 0;
								strcpy(vid_player.audio_info[0].format_name, "Unsupported format");
								//Ignore the error.
								result = 0;
							}
							else
								vid_player.num_of_audio_tracks = num_of_audio_tracks;
						}

						if(num_of_video_tracks > 0)
						{
							uint8_t request_threads = (vid_player.use_multi_threaded_decoding ? vid_player.num_of_threads : 1);
							uint8_t loop = (num_of_video_tracks > 2 ? 2 : num_of_video_tracks);
							Media_thread_type request_thread_type = vid_player.use_multi_threaded_decoding ? vid_player.thread_mode : MEDIA_THREAD_TYPE_NONE;

							DEF_LOG_RESULT_SMART(result, Util_decoder_video_init(vid_player.lower_resolution, num_of_video_tracks,
							request_threads, request_thread_type, 0), (result == DEF_SUCCESS), result);
							if(result == DEF_SUCCESS)
							{
								Sem_state state = { 0, };

								Sem_get_state(&state);

								for(uint8_t i = 0; i < num_of_video_tracks; i++)
									Util_decoder_video_get_info(&vid_player.video_info[i], i, 0);

								//Use sar 1:2 if 800x240 and no sar value is set.
								if(vid_player.video_info[0].width == 800 && vid_player.video_info[0].height == 240
								&& vid_player.video_info[0].sar_width == 1 && vid_player.video_info[0].sar_height == 1)
									vid_player.video_info[0].sar_height = 2;

								if(vid_player.video_info[0].framerate == 0)
									vid_player.video_frametime = 0;
								else
									vid_player.video_frametime = (1000.0 / vid_player.video_info[0].framerate);

								if(num_of_video_tracks >= 2 && vid_player.video_frametime != 0)
									vid_player.video_frametime /= 2;

								if(vid_player.use_hw_decoding && strcmp(vid_player.video_info[0].format_name, "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10") == 0
								&& vid_player.video_info[0].pixel_format == RAW_PIXEL_YUV420P)
								{
									//We can use hw decoding for this video.
									vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_HW_DECODING);

									DEF_LOG_RESULT_SMART(result, Util_decoder_mvd_init(0), (result == DEF_SUCCESS), result);
									if(result != DEF_SUCCESS)
									{
										Util_err_set_error_message(Util_err_get_error_msg(result), "Couldn't initialize HW video decoder!!!!!", DEF_LOG_GET_SYMBOL(), result);
										goto error;
									}
								}

								if(!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) && vid_player.use_hw_color_conversion)
								{
									if(vid_player.video_info[0].codec_width <= 1024 && vid_player.video_info[0].codec_height <= 1024
									&& (vid_player.video_info[0].pixel_format == RAW_PIXEL_YUV420P || vid_player.video_info[0].pixel_format == RAW_PIXEL_YUVJ420P))
									{
										//We can use hw color converter for this video.
										vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_HW_CONVERSION);

										DEF_LOG_RESULT_SMART(result, Util_converter_y2r_init(), (result == DEF_SUCCESS), result);
										if(result != DEF_SUCCESS)
										{
											Util_err_set_error_message(Util_err_get_error_msg(result), "Couldn't initialize HW color converter!!!!!", DEF_LOG_GET_SYMBOL(), result);
											goto error;
										}
									}
								}

								//Allocate texture buffers.
								for(uint8_t i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
								{
									for(uint8_t k = 0; k < loop; k++)
									{
										Util_sync_lock(&vid_player.texture_init_free_lock, UINT64_MAX);
										result = Vid_large_texture_init(&vid_player.large_image[i][k], vid_player.video_info[k].codec_width, vid_player.video_info[k].codec_height, RAW_PIXEL_RGB565LE, true);
										Util_sync_unlock(&vid_player.texture_init_free_lock);

										if(result != DEF_SUCCESS)
										{
											DEF_LOG_RESULT(Vid_large_texture_init, false, result);
											Util_err_set_error_message(Util_err_get_error_msg(result), "Couldn't allocate texture buffers!!!!!", DEF_LOG_GET_SYMBOL(), result);
											goto error;
										}
									}
								}

								//Apply texture filter.
								for(uint8_t i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
								{
									for(uint8_t k = 0; k < DEF_DECODER_MAX_VIDEO_TRACKS; k++)
										Vid_large_texture_set_filter(&vid_player.large_image[i][k], vid_player.use_linear_texture_filter);
								}

								if((DEF_SEM_MODEL_IS_NEW(state.console_model))
								&& (vid_player.video_info[0].thread_type == MEDIA_THREAD_TYPE_NONE || (vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)))
									APT_SetAppCpuTimeLimit(20);
								else
								{
									if(DEF_SEM_MODEL_IS_NEW(state.console_model))
										APT_SetAppCpuTimeLimit(80);
									else
										APT_SetAppCpuTimeLimit(70);
								}

								vid_player.num_of_video_tracks = num_of_video_tracks;
							}
							else//If video format is not supported, disable video so that audio can be played without video.
							{
								vid_player.num_of_video_tracks = 0;
								strcpy(vid_player.video_info[0].format_name, "Unsupported format");
								//Ignore the error.
								result = 0;
							}
						}

						if(num_of_subtitle_tracks > 0)
						{
							DEF_LOG_RESULT_SMART(result, Util_decoder_subtitle_init(num_of_subtitle_tracks, 0), (result == DEF_SUCCESS), result);

							if(result == DEF_SUCCESS)
							{
								for(uint8_t i = 0; i < num_of_subtitle_tracks; i++)
									Util_decoder_subtitle_get_info(&vid_player.subtitle_info[i], i, 0);

								vid_player.selected_subtitle_track = 0;
								vid_player.num_of_subtitle_tracks = num_of_subtitle_tracks;
							}
							else//If subtitle format is not supported, disable subtitle so that audio or video can be played without subtitle.
							{
								vid_player.num_of_subtitle_tracks = 0;
								strcpy(vid_player.subtitle_info[0].format_name, "Unsupported format");
								//Ignore the error.
								result = 0;
							}
						}

						//Use the longest duration as duration for this file.
						if(num_of_video_tracks > 0)
							vid_player.media_duration = vid_player.video_info[0].duration * 1000;
						if(num_of_audio_tracks > 0 && vid_player.audio_info[vid_player.selected_audio_track].duration * 1000 > vid_player.media_duration)
							vid_player.media_duration = vid_player.audio_info[vid_player.selected_audio_track].duration * 1000;

						//Can't play subtitle alone.
						if(num_of_audio_tracks == 0 && num_of_video_tracks == 0)
						{
							result = DEF_ERR_OTHER;
							DEF_LOG_STRING("No playable media!!!!!");
							Util_err_set_error_message(Util_err_get_error_msg(result), "No playable media!!!!!", DEF_LOG_GET_SYMBOL(), result);
							goto error;
						}

						//Enter full screen mode if file has video tracks.
						if(num_of_video_tracks > 0 && !Util_err_query_show_flag() && !Util_expl_query_show_flag())
						{
							Vid_fit_to_screen(400, 240);
							if(!vid_player.is_full_screen)
							{
								Vid_enter_full_screen(300);
								//Reset key state on scene change.
								Util_hid_reset_key_state(HID_KEY_BIT_ALL);
							}
						}
						else
						{
							Vid_fit_to_screen(400, 225);
							Vid_exit_full_screen();
							//Reset key state on scene change.
							Util_hid_reset_key_state(HID_KEY_BIT_ALL);
						}

						//If remember video pos is set, try to find previous playback position.
						if(vid_player.remember_video_pos)
						{
							uint8_t* saved_data = NULL;
							uint32_t read_size = 0;

							result = Util_file_load_from_file(cache_file_name.buffer, DEF_MENU_MAIN_DIR "saved_pos/", &saved_data, sizeof(double), 0, &read_size);
							if(result == DEF_SUCCESS && read_size >= sizeof(double))
							{
								double saved_pos = *(double*)saved_data;
								if(saved_pos > 0 && saved_pos < vid_player.media_duration)
								{
									vid_player.seek_pos = saved_pos;
									DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
									DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
								}
							}

							free(saved_data);
							saved_data = NULL;
						}

						//If we have video tracks, buffer some data before starting/resuming playback.
						//If there is seek event in queue, don't do it because it will automatically
						//enter buffering state after seeking.
						if(!Util_queue_check_event_exist(&vid_player.decode_thread_command_queue, DECODE_THREAD_SEEK_REQUEST)
						&& num_of_video_tracks > 0 && vid_player.video_frametime != 0)
						{
							//Pause the playback.
							Util_speaker_pause(0);
							//Add resume later bit.
							vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
							vid_player.state = PLAYER_STATE_BUFFERING;
						}

						{
							double ram_to_keep_base = (vid_player.ram_to_keep_base / 1000.0 / 1000.0);
							DEF_LOG_DOUBLE(ram_to_keep_base);
						}

						//Start reading packets.
						is_read_packet_thread_active = true;
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.read_packet_thread_command_queue,
						READ_PACKET_THREAD_READ_PACKET_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

						//Start converting color, this thread keeps running unless we send abort request.
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.convert_thread_command_queue,
						CONVERT_THREAD_CONVERT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

						if(num_of_video_tracks == 0 || vid_player.video_frametime == 0)
							Util_watch_add(WATCH_HANDLE_VIDEO_PLAYER, &audio_bar_pos, sizeof(audio_bar_pos));

						break;

						error:
						//An error occurred, reset everything.
						Util_err_set_show_flag(true);
						Draw_set_refresh_needed(true);

						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_SEND_TO_FRONT), (result == DEF_SUCCESS), result);

						continue;
					}
					else
					{
						//If currently player state is not idle, abort current playback first.
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

						//Then play new one, pass the received new_file again.
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_PLAY_REQUEST, new_file, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
					}

					break;
				}

				case DECODE_THREAD_PAUSE_REQUEST:
				{
					//Do nothing if player state is idle, prepare playing or pause.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
					|| vid_player.state == PLAYER_STATE_PAUSE)
						break;

					if(vid_player.state == PLAYER_STATE_PLAYING)
					{
						Util_speaker_pause(0);
						vid_player.state = PLAYER_STATE_PAUSE;
					}
					else
					{
						//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);
					}

					break;
				}

				case DECODE_THREAD_RESUME_REQUEST:
				{
					//Do nothing if player state is idle, prepare playing or playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
					|| vid_player.state == PLAYER_STATE_PLAYING)
						break;

					if(vid_player.state == PLAYER_STATE_PAUSE)
					{
						Util_speaker_resume(0);
						vid_player.state = PLAYER_STATE_PLAYING;
					}
					else
					{
						//Add resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
					}

					break;
				}

				case DECODE_THREAD_SEEK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					if(vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					{
						//Currently threre is on going seek request, retry later.
						//Add seek request if threre is no another seek request.
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);

						Util_sleep(1000);
						break;
					}

					if(vid_player.state == PLAYER_STATE_PLAYING)//Add resume later bit.
					{
						Util_speaker_pause(0);
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
					}
					else if(vid_player.state == PLAYER_STATE_PAUSE)//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

					//Handle seek request.
					vid_player.state = PLAYER_STATE_PREPARE_SEEKING;
					vid_player.show_current_pos_until = U64_MAX;
					seek_start_pos = vid_player.media_current_pos;

					if(seek_start_pos > vid_player.seek_pos)//Add seek backward wait bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);
					else//Remove seek backward wait bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);

					Util_speaker_clear_buffer(0);

					//Reset EOF flag.
					is_eof = false;

					//Seek the video.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.read_packet_thread_command_queue,
					READ_PACKET_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

					break;
				}

				case DECODE_THREAD_CHANGE_AUDIO_TRACK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					//Change audio track.
					if(!vid_player.is_selecting_audio_track && vid_player.selected_audio_track != vid_player.selected_audio_track_cache
					&& vid_player.selected_audio_track_cache < vid_player.num_of_audio_tracks)
					{
						vid_player.selected_audio_track = vid_player.selected_audio_track_cache;
						Util_speaker_clear_buffer(0);
						//3DS only supports up to 2ch.
						Util_speaker_set_audio_info(0, (vid_player.audio_info[vid_player.selected_audio_track].ch > 2 ? 2 : vid_player.audio_info[vid_player.selected_audio_track].ch), vid_player.audio_info[vid_player.selected_audio_track].sample_rate);

						//Use the longest duration as duration for this file.
						if(vid_player.num_of_video_tracks > 0)
							vid_player.media_duration = vid_player.video_info[0].duration * 1000;
						if(vid_player.num_of_audio_tracks > 0 && vid_player.audio_info[vid_player.selected_audio_track].duration * 1000 > vid_player.media_duration)
							vid_player.media_duration = vid_player.audio_info[vid_player.selected_audio_track].duration * 1000;
					}

					break;
				}

				case DECODE_THREAD_CHANGE_SUBTITLE_TRACK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					//Change subtitle track.
					if(!vid_player.is_selecting_subtitle_track && vid_player.selected_subtitle_track != vid_player.selected_subtitle_track_cache
					&& vid_player.selected_subtitle_track_cache < vid_player.num_of_subtitle_tracks)
					{
						vid_player.selected_subtitle_track = vid_player.selected_subtitle_track_cache;
						for(uint8_t i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)
						{
							free(vid_player.subtitle_data[i].bitmap);
							free(vid_player.subtitle_data[i].text);
							vid_player.subtitle_data[i].bitmap = NULL;
							vid_player.subtitle_data[i].bitmap_width = 0;
							vid_player.subtitle_data[i].bitmap_height = 0;
							vid_player.subtitle_data[i].start_time = 0;
							vid_player.subtitle_data[i].end_time = 0;
							vid_player.subtitle_data[i].text = NULL;
						}
					}

					break;
				}

				case DECODE_THREAD_PLAY_NEXT_REQUEST:
				case DECODE_THREAD_ABORT_REQUEST:
				case DECODE_THREAD_SHUTDOWN_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
					{
						vid_player.state = PLAYER_STATE_IDLE;
						vid_player.sub_state = PLAYER_SUB_STATE_NONE;

						if(event == DECODE_THREAD_SHUTDOWN_REQUEST)
						{
							//Exit the threads.
							vid_thread_run = false;
							continue;
						}

						break;
					}

					if(vid_player.remember_video_pos)
					{
						if(event == DECODE_THREAD_PLAY_NEXT_REQUEST)//If playback has finished, remove saved time.
							Util_file_delete_file(cache_file_name.buffer, DEF_MENU_MAIN_DIR "saved_pos/");
						else//If remember video pos is set, save playback position.
						{
							double saved_pos = vid_player.media_current_pos;
							Str_data time = { 0, };

							if(Util_convert_seconds_to_time((saved_pos / 1000), &time) == DEF_SUCCESS)
								DEF_LOG_FORMAT("last pos : %s", time.buffer);

							Util_file_save_to_file(cache_file_name.buffer, DEF_MENU_MAIN_DIR "saved_pos/", (uint8_t*)(&saved_pos), sizeof(double), true);
							Util_str_free(&time);
						}
					}

					//Wait for read packet thread (also flush queues).
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.read_packet_thread_command_queue,
					READ_PACKET_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_thread_notification_queue, (uint32_t*)&notification, NULL, 100000);
						if(result == DEF_SUCCESS && notification == READ_PACKET_THREAD_FINISHED_ABORTING_NOTIFICATION)
							break;
					}

					//Wait for convert thread (also flush queues).
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.convert_thread_command_queue,
					CONVERT_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_thread_notification_queue, (uint32_t*)&notification, NULL, 100000);
						if(result == DEF_SUCCESS && notification == CONVERT_THREAD_FINISHED_ABORTING_NOTIFICATION)
							break;
					}

					//Wait for video decoding thread (also flush queues).
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_video_thread_command_queue,
					DECODE_VIDEO_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_thread_notification_queue, (uint32_t*)&notification, NULL, 100000);
						if(result == DEF_SUCCESS && notification == DECODE_VIDEO_THREAD_FINISHED_ABORTING_NOTIFICATION)
							break;
					}

					if(vid_player.num_of_audio_tracks > 0)
						Util_speaker_exit();

					Util_decoder_close_file(0);
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION)
						Util_converter_y2r_exit();

					Util_sync_lock(&vid_player.texture_init_free_lock, UINT64_MAX);
					for(uint8_t i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
					{
						for(uint8_t k = 0; k < DEF_DECODER_MAX_VIDEO_TRACKS; k++)
							Vid_large_texture_free(&vid_player.large_image[i][k]);
					}
					Util_sync_unlock(&vid_player.texture_init_free_lock);

					if(vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0)
						Util_watch_remove(WATCH_HANDLE_VIDEO_PLAYER, &audio_bar_pos);

					vid_player.show_full_screen_msg = false;
					vid_player.seek_bar.selected = false;
					vid_player.show_current_pos_until = 0;

					if(event == DECODE_THREAD_PLAY_NEXT_REQUEST && vid_player.playback_mode != DEF_VID_NO_REPEAT)
					{
						Vid_file* file_data = (Vid_file*)malloc(sizeof(Vid_file));

						if(vid_player.playback_mode == DEF_VID_IN_ORDER || vid_player.playback_mode == DEF_VID_RANDOM)
						{
							for(uint32_t i = 0; i < (vid_player.playback_mode == DEF_VID_RANDOM ? Util_expl_query_num_of_file() + 256 : Util_expl_query_num_of_file()); i++)
							{
								if(vid_player.file.index + 1 >= Util_expl_query_num_of_file())
									vid_player.file.index = 0;
								else
									vid_player.file.index++;

								if(Util_expl_query_type(vid_player.file.index) & EXPL_FILE_TYPE_FILE)
								{
									srand(time(NULL) + i);
									if(vid_player.playback_mode == DEF_VID_IN_ORDER)
										break;
									else if(rand() % 5 == 1)//1 in 5
										break;
								}
							}
						}

						if(file_data)
						{
							Str_data temp = { 0, };

							//Create a message.
							file_data->index = vid_player.file.index;

							if(Util_expl_query_file_name(file_data->index, &temp) == DEF_SUCCESS)
								snprintf(file_data->name, sizeof(file_data->name), "%s", temp.buffer);
							if(Util_expl_query_current_dir(&temp) == DEF_SUCCESS)
								snprintf(file_data->directory, sizeof(file_data->directory), "%s", temp.buffer);

							Util_str_free(&temp);
						}

						//Play next video.
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_PLAY_REQUEST, file_data, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

						vid_player.state = PLAYER_STATE_PREPATE_PLAYING;
					}
					else
					{
						Vid_exit_full_screen();
						//Reset key state on scene change.
						Util_hid_reset_key_state(HID_KEY_BIT_ALL);
						vid_player.state = PLAYER_STATE_IDLE;
					}

					vid_player.sub_state = PLAYER_SUB_STATE_NONE;

					if(event == DECODE_THREAD_SHUTDOWN_REQUEST)
					{
						//Exit the threads.
						vid_thread_run = false;
						continue;
					}

					break;
				}

				case DECODE_THREAD_INCREASE_KEEP_RAM_REQUEST:
				{
					uint32_t previous_size = vid_player.ram_to_keep_base;
					uint32_t raw_image_size = 0;

					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
						raw_image_size = DEF_VID_HW_DECODER_RAW_IMAGE_SIZE;
					else
						raw_image_size = DEF_VID_SW_DECODER_RAW_IMAGE_SIZE;

					//Increase RAM amount to keep to reduce the chance of out of memory loop.
					vid_player.ram_to_keep_base = (previous_size + raw_image_size);

					DEF_LOG_STRING("Out of memory has been detected!!!!!");
					DEF_LOG_FORMAT("Increasing RAM amount to keep (%.3fMB->%.3fMB)",
					(previous_size / 1000.0 / 1000.0), ((previous_size + raw_image_size) / 1000.0 / 1000.0));

					break;
				}
				default:
					break;
			}
		}

		result = Util_queue_get(&vid_player.decode_thread_notification_queue, (uint32_t*)&notification, NULL, 0);
		if(result == DEF_SUCCESS)
		{
			switch (notification)
			{
				case READ_PACKET_THREAD_FINISHED_READING_EOF_NOTIFICATION:
					is_eof = true;

				//Fall through.
				case READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION:
				{
					is_read_packet_thread_active = false;
					break;
				}

				case READ_PACKET_THREAD_FINISHED_SEEKING_NOTIFICATION:
				{
					//Do nothing if player state is not prepare seeking.
					if(vid_player.state != PLAYER_STATE_PREPARE_SEEKING)
						break;

					if(vid_player.num_of_video_tracks <= 0)
					{
						//If there are no video tracks, start seeking.
						//Sometimes library caches previous frames even after clearing packet,
						//so ignore first 5 (+ num_of_threads if frame threading is used) frames.
						wait_count = 5 + (vid_player.video_info[0].thread_type == MEDIA_THREAD_TYPE_FRAME ? vid_player.num_of_threads : 0);
						backward_timeout = 20;
						vid_player.state = PLAYER_STATE_SEEKING;
					}
					else
					{
						//If there are video tracks, clear the video cache first.
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_video_thread_command_queue,
						DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
					}

					break;
				}

				case DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION:
				{
					is_waiting_video_decoder = false;
					break;
				}

				case DECODE_VIDEO_THREAD_FINISHED_CLEARING_CACHE:
				{
					//Do nothing if player state is not prepare seeking.
					if(vid_player.state != PLAYER_STATE_PREPARE_SEEKING)
						break;

					//After clearing cache start seeking.
					//Sometimes library caches previous frames even after clearing packet,
					//so ignore first 5 (+ num_of_threads if frame threading is used) frames.
					wait_count = 5 + (vid_player.video_info[0].thread_type == MEDIA_THREAD_TYPE_FRAME ? vid_player.num_of_threads : 0);
					backward_timeout = 20;
					vid_player.state = PLAYER_STATE_SEEKING;

					break;
				}

				case DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION:
				case CONVERT_THREAD_FINISHED_BUFFERING_NOTIFICATION:
				{
					//Do nothing if player state is not buffering.
					if(vid_player.state != PLAYER_STATE_BUFFERING)
						break;

					if(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER)
					{
						//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

						//Resume the playback.
						Util_speaker_resume(0);
						vid_player.state = PLAYER_STATE_PLAYING;
					}
					else
					{
						//Don't resume.
						vid_player.state = PLAYER_STATE_PAUSE;
					}

					break;
				}

				case CONVERT_THREAD_OUT_OF_BUFFER_NOTIFICATION:
				{
					//Do nothing if player state is not playing nor pause.
					if(vid_player.state != PLAYER_STATE_PLAYING && vid_player.state != PLAYER_STATE_PAUSE)
						break;

					//Do nothing if we completely reached EOF.
					if(is_eof && Util_decoder_get_available_packet_num(0) == 0)
						break;

					if(vid_player.state == PLAYER_STATE_PLAYING)
					{
						//Pause the playback.
						Util_speaker_pause(0);
						//Add resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
					}
					else//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

					vid_player.state = PLAYER_STATE_BUFFERING;

					break;
				}

				default:
					break;
			}
		}

		if(vid_player.state == PLAYER_STATE_PLAYING || vid_player.state == PLAYER_STATE_SEEKING
		|| vid_player.state == PLAYER_STATE_PAUSE || vid_player.state == PLAYER_STATE_BUFFERING)
		{
			bool key_frame = false;
			uint8_t packet_index = 0;
			uint16_t num_of_cached_packets = 0;
			uint16_t num_of_video_buffers = 0;
			uint32_t num_of_audio_buffers = 0;
			uint32_t audio_buffers_size = 0;
			uint32_t required_free_ram = 0;
			double audio_buffer_health_ms = 0;
			Media_packet_type type = MEDIA_PACKET_TYPE_UNKNOWN;

			//Calculate how much free RAM we need and check buffer health.
			//To prevent out of memory on other tasks, make sure we have at least :
			//ram_to_keep_base + (raw image size * 2) for hardware decoding.
			//ram_to_keep_base + (raw image size * (1 + num_of_threads)) for software decoding.
			if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
			{
				required_free_ram = (vid_player.ram_to_keep_base + (DEF_VID_HW_DECODER_RAW_IMAGE_SIZE * 2));
				num_of_video_buffers = Util_decoder_mvd_get_available_raw_image_num(0);
			}
			else
			{
				uint8_t num_of_active_threads = 1;

				if(vid_player.video_info[0].thread_type == MEDIA_THREAD_TYPE_FRAME)
					num_of_active_threads = vid_player.num_of_threads;

				required_free_ram = (vid_player.ram_to_keep_base + (DEF_VID_SW_DECODER_RAW_IMAGE_SIZE * (num_of_active_threads + 1)));
				num_of_video_buffers = Util_decoder_video_get_available_raw_image_num(0, 0);

				if(vid_player.num_of_video_tracks > 1 && Util_decoder_video_get_available_raw_image_num(1, 0) > num_of_video_buffers)
					num_of_video_buffers = Util_decoder_video_get_available_raw_image_num(1, 0);
			}
			num_of_cached_packets = Util_decoder_get_available_packet_num(0);
			num_of_audio_buffers = Util_speaker_get_available_buffer_num(0);
			audio_buffers_size = Util_speaker_get_available_buffer_size(0);

			//Audio buffer health (in ms) is ((buffer_size / bytes_per_sample / num_of_ch / sample_rate) * 1000).
			audio_buffer_health_ms = (audio_buffers_size / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000);

			//Update audio position.
			if(vid_player.num_of_audio_tracks > 0)
				vid_player.audio_current_pos = vid_player.last_decoded_audio_pos - audio_buffer_health_ms;

			//Get position from audio track if file does not have video tracks,
			//frametime is unknown or audio duration is longer than video track.
			if(vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0
			|| vid_player.audio_info[vid_player.selected_audio_track].duration > vid_player.video_info[0].duration)
				vid_player.media_current_pos = vid_player.audio_current_pos;

			//If file does not have video tracks, update bar pos to see if the position has changed
			//so that we can update the screen when it gets changed.
			if(vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0)
			{
				//We want to update screen every 100ms hence divide by 100.
				//(audio_bar_pos has been registerd in watch list.)
				audio_bar_pos = (uint32_t)(vid_player.media_current_pos / 100);
			}

			//If we only have half number of packets in buffer, we have not reached eof
			//and read packet thread is inactive, send a read packet request.
			if((num_of_cached_packets < (DEF_DECODER_MAX_CACHE_PACKETS / 2)) && !is_read_packet_thread_active && !is_eof)
			{
				is_read_packet_thread_active = true;

				//Start reading packets.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.read_packet_thread_command_queue,
				READ_PACKET_THREAD_READ_PACKET_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
			}

			if(num_of_cached_packets == 0 && is_eof)
			{
				//We consumed all of packets.
				bool is_audio_done = true;
				bool is_video_done = true;

				//Wait for finish playback audio.
				if(vid_player.num_of_audio_tracks > 0)
				{
					if(audio_buffers_size > 0)
						is_audio_done = false;
				}

				//Wait for finish playback video.
				if(vid_player.num_of_video_tracks > 0 && vid_player.video_frametime != 0)
				{
					if(num_of_video_buffers > 0)
						is_video_done = false;
				}

				//Stop buffering if we completely reached EOF.
				if(vid_player.state == PLAYER_STATE_BUFFERING)
				{
					//Notify we've done buffering (can't buffer anymore).
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
					DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
					continue;
				}

				if(is_audio_done && is_video_done)
				{
					//We've finished playing.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
					DECODE_THREAD_PLAY_NEXT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
					continue;
				}

				Util_sleep(10000);
				continue;
			}

			if(is_waiting_video_decoder || (num_of_video_buffers + 1 >= DEF_DECODER_MAX_RAW_IMAGE)
			|| ((num_of_audio_buffers + 1) >= DEF_SPEAKER_MAX_BUFFERS) || Util_check_free_linear_space() < required_free_ram)
			{
				//If one of them is true, wait and try again later.
				//1. Video decoder thread has not completed copying the packet yet.
				//2. Video buffer is full.
				//3. Speaker buffer is full.
				//4. No enough free RAM.

				if(is_waiting_video_decoder)
				{
					//Wait for video decoder thread.
					Util_sleep(1000);
					continue;
				}
				else if((num_of_audio_buffers + 1) >= DEF_SPEAKER_MAX_BUFFERS)
				{
					//Speaker buffer is full.
					if(vid_player.state == PLAYER_STATE_BUFFERING)
					{
						//Notify we've done buffering (can't buffer anymore).
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
						DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
						continue;
					}

					Util_sleep(10000);
					continue;
				}
				else if(num_of_video_buffers > 0)
				{
					//We don't have enough free RAM but have cached raw pictures,
					//so wait frametime ms for them to get playbacked and freed.
					//If framerate is unknown, sleep 10ms.
					uint64_t sleep = (vid_player.video_info[0].framerate > 0 ? (1000000 / vid_player.video_info[0].framerate) : 10000);

					if(vid_player.state == PLAYER_STATE_BUFFERING)
					{
						//Notify we've done buffering (can't buffer anymore).
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
						DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
						continue;
					}

					Util_sleep(sleep);
					continue;
				}
				else
				{
					//If we don't have enough RAM and there are no cached raw pictures,
					//it is unlikely to increase amount of free RAM by waiting since
					//no video buffer get freed, so we have to try to decode it anyway.
				}
			}

			result = Util_decoder_parse_packet(&type, &packet_index, &key_frame, 0);
			if(result == DEF_ERR_TRY_AGAIN)//Packet is not ready, try again later.
			{
				Util_sleep(5000);
				continue;
			}
			else if(result != DEF_SUCCESS)//For other errors, log error detail.
			{
				DEF_LOG_RESULT(Util_decoder_parse_packet, false, result);
				Util_sleep(10000);
				continue;
			}

			//Handle seek request.
			if(vid_player.state == PLAYER_STATE_SEEKING && (vid_player.num_of_video_tracks == 0
			|| vid_player.video_frametime == 0 || type == MEDIA_PACKET_TYPE_VIDEO))
			{
				//Make sure we went back.
				if((wait_count == 0 && vid_player.video_current_pos[0] < seek_start_pos)
				|| vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0)//Remove seek backward wait bit.
					vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);

				if(!(vid_player.sub_state & PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT) && vid_player.media_current_pos >= vid_player.seek_pos)
				{
					//Seek has finished.
					vid_player.show_current_pos_until = osGetTime() + 4000;

					if(vid_player.num_of_video_tracks > 0 && vid_player.video_frametime != 0)//Buffer some data before resuming playback if file contains video tracks.
						vid_player.state = PLAYER_STATE_BUFFERING;
					else
					{
						if(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER)
						{
							//Remove resume later bit.
							vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

							//Resume the playback.
							Util_speaker_resume(0);
							vid_player.state = PLAYER_STATE_PLAYING;
						}
						else
						{
							//Don't resume.
							vid_player.state = PLAYER_STATE_PAUSE;
						}
					}
				}

				if(wait_count > 0)
					wait_count--;
				else if(backward_timeout > 0)
					backward_timeout--;
				else//Timeout, remove seek backward wait bit.
					vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);
			}

			if(type == MEDIA_PACKET_TYPE_UNKNOWN)
			{
				DEF_LOG_STRING("Unknown packet type!!!!!");
				Util_sleep(10000);
				continue;
			}
			else if(type == MEDIA_PACKET_TYPE_AUDIO)
			{
				if(vid_player.num_of_audio_tracks != 0 && packet_index == vid_player.selected_audio_track)
				{
					result = Util_decoder_ready_audio_packet(packet_index, 0);
					if(result == DEF_SUCCESS)
					{
						uint8_t* audio = NULL;
						uint32_t audio_samples = 0;
						double pos = 0;
						TickCounter counter = { 0, };
						Converter_audio_parameters parameters = { 0, };

						parameters.converted = NULL;

						osTickCounterStart(&counter);
						result = Util_decoder_audio_decode(&audio_samples, &audio, &pos, packet_index, 0);
						osTickCounterUpdate(&counter);
						vid_player.audio_decoding_time_list[319] = osTickCounterRead(&counter);

						for(uint16_t i = 1; i < 320; i++)
							vid_player.audio_decoding_time_list[i - 1] = vid_player.audio_decoding_time_list[i];

						if(result == DEF_SUCCESS && vid_player.state != PLAYER_STATE_SEEKING)
						{
							parameters.source = audio;
							parameters.in_ch = vid_player.audio_info[packet_index].ch;
							parameters.in_sample_rate = vid_player.audio_info[packet_index].sample_rate;
							parameters.in_sample_format = vid_player.audio_info[packet_index].sample_format;
							parameters.in_samples = audio_samples;
							parameters.converted = NULL;
							parameters.out_ch = (vid_player.audio_info[packet_index].ch > 2 ? 2 : vid_player.audio_info[packet_index].ch);
							parameters.out_sample_rate = vid_player.audio_info[packet_index].sample_rate;
							parameters.out_sample_format = RAW_SAMPLE_S16;

							result = Util_converter_convert_audio(&parameters);
							if(result == DEF_SUCCESS)
							{
								bool too_big = false;

								//Change volume.
								if(vid_player.volume != 100)
								{
									for(uint32_t i = 0; i < (parameters.out_samples * parameters.out_ch * 2); i += 2)
									{
										if(*(int16_t*)(parameters.converted + i) * ((double)vid_player.volume / 100) > INT16_MAX)
										{
											*(int16_t*)(parameters.converted + i) = INT16_MAX;
											too_big = true;
										}
										else
											*(int16_t*)(parameters.converted + i) = *(int16_t*)(parameters.converted + i) * ((double)vid_player.volume / 100);
									}

									if(too_big)
										vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_TOO_BIG);
									else
										vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_TOO_BIG);
								}

								//Add audio to speaker buffer, wait up to 250ms.
								for(uint8_t i = 0; i < 125; i++)
								{
									result = Util_speaker_add_buffer(0, parameters.converted, (parameters.out_samples * parameters.out_ch * 2));
									if(result != DEF_ERR_TRY_AGAIN)
										break;

									Util_sleep(2000);
								}
							}
							else
								DEF_LOG_RESULT(Util_converter_convert_audio, false, result);
						}
						else if(result != DEF_SUCCESS)
							DEF_LOG_RESULT(Util_decoder_audio_decode, false, result);

						//Update audio position.
						vid_player.last_decoded_audio_pos = pos;

						free(parameters.converted);
						free(audio);
						parameters.converted = NULL;
						audio = NULL;
					}
					else
						DEF_LOG_RESULT(Util_decoder_ready_audio_packet, false, result);
				}
				else//This packet is not what we are looking for now, just skip it.
					Util_decoder_skip_audio_packet(packet_index, 0);
			}
			else if(type == MEDIA_PACKET_TYPE_SUBTITLE)
			{
				if(vid_player.num_of_subtitle_tracks != 0 && packet_index == vid_player.selected_subtitle_track)
				{
					result = Util_decoder_ready_subtitle_packet(packet_index, 0);
					if(result == DEF_SUCCESS)
					{
						result = Util_decoder_subtitle_decode(&vid_player.subtitle_data[vid_player.subtitle_index], packet_index, 0);
						if(result == DEF_SUCCESS)
						{
							if(vid_player.subtitle_data[vid_player.subtitle_index].bitmap)
							{
								//Subtitle data format is bitmap, copy raw data to subtitle texture.
								uint32_t texture_width = Vid_get_min_texture_size(vid_player.subtitle_data[vid_player.subtitle_index].bitmap_width);
								uint32_t texture_height = Vid_get_min_texture_size(vid_player.subtitle_data[vid_player.subtitle_index].bitmap_height);

								Util_sync_lock(&vid_player.texture_init_free_lock, UINT64_MAX);
								Draw_texture_free(&vid_player.subtitle_image[vid_player.subtitle_index]);
								result = Draw_texture_init(&vid_player.subtitle_image[vid_player.subtitle_index], texture_width, texture_height, RAW_PIXEL_ABGR8888);
								memset(vid_player.subtitle_image[vid_player.subtitle_index].c2d.tex->data, 0x0, texture_width * texture_height * 4);
								Util_sync_unlock(&vid_player.texture_init_free_lock);

								if(result == DEF_SUCCESS)
								{
									Draw_set_texture_data(&vid_player.subtitle_image[vid_player.subtitle_index], vid_player.subtitle_data[vid_player.subtitle_index].bitmap,
									vid_player.subtitle_data[vid_player.subtitle_index].bitmap_width, vid_player.subtitle_data[vid_player.subtitle_index].bitmap_height, 0, 0);
								}
								else
									DEF_LOG_RESULT(Draw_texture_init, false, result);

								free(vid_player.subtitle_data[vid_player.subtitle_index].bitmap);
								vid_player.subtitle_data[vid_player.subtitle_index].bitmap = NULL;
							}

							if(vid_player.subtitle_index + 1 >= DEF_VID_SUBTITLE_BUFFERS)
								vid_player.subtitle_index = 0;
							else
								vid_player.subtitle_index++;
						}
						else
							DEF_LOG_RESULT(Util_decoder_subtitle_decode, false, result);
					}
					else
						DEF_LOG_RESULT(Util_decoder_ready_subtitle_packet, false, result);

				}
				else//This packet is not what we are looking for now, just skip it.
					Util_decoder_skip_subtitle_packet(packet_index, 0);
			}
			else if(type == MEDIA_PACKET_TYPE_VIDEO)
			{
				if(vid_player.num_of_video_tracks != 0 && (!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
				|| ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) && packet_index == 0)))
				{
					Vid_video_packet_data* packet_info = (Vid_video_packet_data*)malloc(sizeof(Vid_video_packet_data));

					if(packet_info)
					{
						packet_info->is_key_frame = key_frame;
						packet_info->packet_index = packet_index;

						is_waiting_video_decoder = true;
						//Decode the next frame.
						//Too noisy.
						// DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_video_thread_command_queue,
						// DECODE_VIDEO_THREAD_DECODE_REQUEST, packet_info, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

						result = Util_queue_add(&vid_player.decode_video_thread_command_queue,
						DECODE_VIDEO_THREAD_DECODE_REQUEST, packet_info, 100000, QUEUE_OPTION_NONE);
						if(result != DEF_SUCCESS)
							DEF_LOG_RESULT(Util_queue_add, false, result);
					}
					else
						Util_decoder_skip_video_packet(packet_index, 0);
				}
				else if(type == MEDIA_PACKET_TYPE_VIDEO)//This packet is not what we are looking for now, just skip it.
					Util_decoder_skip_video_packet(packet_index, 0);
			}
		}
		else
			Util_sleep(1000);
	}

	Util_str_free(&cache_file_name);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

void Vid_decode_video_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;
	double skip = 0;
	TickCounter counter = { 0, };

	osTickCounterStart(&counter);

	while (vid_thread_run)
	{
		void* message = NULL;
		Vid_command event = NONE_REQUEST;

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_THREAD_INACTIVE_SLEEP_TIME);
		}

		result = Util_queue_get(&vid_player.decode_video_thread_command_queue, (uint32_t*)&event, &message, DEF_THREAD_ACTIVE_SLEEP_TIME);
		if(result == DEF_SUCCESS)
		{
			switch (event)
			{
				case DECODE_VIDEO_THREAD_DECODE_REQUEST:
				{
					bool key_frame = false;
					uint8_t packet_index = 0;
					Vid_video_packet_data* packet_info = (Vid_video_packet_data*)message;

					//Do nothing if player state is idle or prepare playing or message is NULL.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING || !packet_info)
						break;

					key_frame = packet_info->is_key_frame;
					packet_index = packet_info->packet_index;

					free(packet_info);
					packet_info = NULL;

					if(vid_player.allow_skip_frames && skip > vid_player.video_frametime && (!key_frame || vid_player.allow_skip_key_frames) && vid_player.video_frametime != 0)
					{
						skip -= vid_player.video_frametime;
						Util_decoder_skip_video_packet(packet_index, 0);

						//Notify we've done copying packet to video decoder buffer (we skipped the frame here)
						//so that decode thread can read the next packet.
						//Too noisy.
						// DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
						// DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
						result = Util_queue_add(&vid_player.decode_thread_notification_queue,
						DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE);
						if(result != DEF_SUCCESS)
							DEF_LOG_RESULT(Util_queue_add, false, result);
					}
					else
					{
						result = Util_decoder_ready_video_packet(packet_index, 0);

						//Notify we've done copying packet to video decoder buffer
						//so that decode thread can read the next packet.
						//Too noisy.
						// DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
						// DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
						result = Util_queue_add(&vid_player.decode_thread_notification_queue,
						DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE);
						if(result != DEF_SUCCESS)
							DEF_LOG_RESULT(Util_queue_add, false, result);

						if(result == DEF_SUCCESS)
						{
							while(true)
							{
								osTickCounterUpdate(&counter);

								if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
									result = Util_decoder_mvd_decode(0);
								else
									result = Util_decoder_video_decode(packet_index, 0);

								osTickCounterUpdate(&counter);

								if(result == DEF_ERR_DECODER_TRY_AGAIN || result == DEF_ERR_DECODER_TRY_AGAIN_NO_OUTPUT
								|| result == DEF_ERR_TRY_AGAIN)
								{
									if(result == DEF_ERR_DECODER_TRY_AGAIN)//Got a frame.
									{
										if(vid_player.video_info[packet_index].thread_type != MEDIA_THREAD_TYPE_FRAME
										|| (vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING))
											Vid_update_performance_graph(osTickCounterRead(&counter), key_frame, &skip);

										key_frame = false;
									}
									else if(result == DEF_ERR_TRY_AGAIN)//Buffer is full.
									{
										if(vid_player.video_frametime <= 0)
											Util_sleep(10000);
										else
											Util_sleep(vid_player.video_frametime * 1000);

										//If we get clear cache or abort request while waiting, break the loop.
										if(Util_queue_check_event_exist(&vid_player.decode_video_thread_command_queue, DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST)
										|| Util_queue_check_event_exist(&vid_player.decode_video_thread_command_queue, DECODE_VIDEO_THREAD_ABORT_REQUEST))
											break;
									}

									continue;
								}
								else
									break;
							}

							if(result == DEF_SUCCESS)
							{
								if(vid_player.video_info[packet_index].thread_type != MEDIA_THREAD_TYPE_FRAME
								|| (vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING))
									Vid_update_performance_graph(osTickCounterRead(&counter), key_frame, &skip);
							}
							else if(result != DEF_ERR_NEED_MORE_INPUT)
							{
								if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
									DEF_LOG_RESULT(Util_decoder_mvd_decode, false, result);
								else
									DEF_LOG_RESULT(Util_decoder_video_decode, false, result);
							}
						}
						else
						{
							Util_decoder_skip_video_packet(packet_index, 0);
							DEF_LOG_RESULT(Util_decoder_ready_video_packet, false, result);
						}

						if(result == DEF_ERR_OUT_OF_MEMORY || result == DEF_ERR_OUT_OF_LINEAR_MEMORY)
						{
							//Request to increase amount of RAM to keep.
							DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
							DECODE_THREAD_INCREASE_KEEP_RAM_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
						}
					}

					break;
				}

				case DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					//Clear cache.
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
						Util_decoder_mvd_clear_raw_image(0);
					else
					{
						for(uint8_t i = 0; i < vid_player.num_of_video_tracks; i++)
							Util_decoder_video_clear_raw_image(i, 0);
					}

					//Flush the decoder.
					while(true)
					{
						if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
							result = Util_decoder_mvd_decode(0);
						else
						{
							result = Util_decoder_video_decode(0, 0);
							if(vid_player.num_of_video_tracks > 1)
								result = Util_decoder_video_decode(1, 0);
						}

						if(result != DEF_SUCCESS && result != DEF_ERR_DECODER_TRY_AGAIN_NO_OUTPUT && result != DEF_ERR_DECODER_TRY_AGAIN)
							break;
					}

					//Notify we've done clearing cache.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
					DECODE_VIDEO_THREAD_FINISHED_CLEARING_CACHE, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

					break;
				}

				case DECODE_VIDEO_THREAD_ABORT_REQUEST:
				{
					skip = 0;

					//Clear cache.
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
						Util_decoder_mvd_clear_raw_image(0);
					else
					{
						for(uint8_t i = 0; i < vid_player.num_of_video_tracks; i++)
							Util_decoder_video_clear_raw_image(i, 0);
					}

					//Flush the command queue.
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_video_thread_command_queue, (uint32_t*)&event, NULL, 0);
						if(result != DEF_SUCCESS)
							break;
					}

					//Notify we've done aborting.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
					DECODE_VIDEO_THREAD_FINISHED_ABORTING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

					break;
				}

				default:
					break;
			}
		}
	}

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

void Vid_convert_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	bool should_convert = false;
	uint8_t packet_index = 0;
	uint32_t result = DEF_ERR_OTHER;
	double total_delay = 0;
	TickCounter conversion_time_counter = { 0, };

	osTickCounterStart(&conversion_time_counter);

	while (vid_thread_run)
	{
		bool drop = false;
		uint16_t num_of_cached_raw_images = 0;
		uint64_t timeout_us = DEF_THREAD_ACTIVE_SLEEP_TIME;
		double drop_threshold = 0;
		double force_drop_threshold = 0;
		Vid_command event = NONE_REQUEST;

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_THREAD_INACTIVE_SLEEP_TIME);
		}

		//If player state is playing or seeking, don't wait for commands.
		if((vid_player.state == PLAYER_STATE_PLAYING || vid_player.state == PLAYER_STATE_SEEKING)
		&& should_convert)
			timeout_us = 0;

		result = Util_queue_get(&vid_player.convert_thread_command_queue, (uint32_t*)&event, NULL, timeout_us);
		if(result == DEF_SUCCESS)
		{
			switch (event)
			{
				case CONVERT_THREAD_CONVERT_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing or file doesn't have video tracks.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
					|| vid_player.num_of_video_tracks <= 0)
						break;

					should_convert = true;
					packet_index = 0;
					total_delay = 0;

					break;
				}

				case CONVERT_THREAD_ABORT_REQUEST:
				{
					should_convert = false;
					packet_index = 0;
					total_delay = 0;

					//Flush the command queue.
					while(true)
					{
						result = Util_queue_get(&vid_player.convert_thread_command_queue, (uint32_t*)&event, NULL, 0);
						if(result != DEF_SUCCESS)
							break;
					}

					//Notify we've done aborting.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
					CONVERT_THREAD_FINISHED_ABORTING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

					break;
				}

				default:
					break;
			}
		}

		//Do nothing if player state is idle, prepare playing, prepare seeking or should_convert flag is not set.
		if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
		|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || !should_convert)
			continue;

		//Check if we have cached raw images.
		if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
			num_of_cached_raw_images = Util_decoder_mvd_get_available_raw_image_num(0);
		else
			num_of_cached_raw_images = Util_decoder_video_get_available_raw_image_num(packet_index, 0);

		if(vid_player.video_frametime != 0)
		{
			uint64_t current_ts = osGetTime();

			if(vid_player.num_of_audio_tracks > 0)
			{
				double video_delay = 0;

				//Calc video frame drop threshold.
				drop_threshold = DEF_VID_DROP_THRESHOLD(vid_player.video_frametime);
				if(vid_player.num_of_video_tracks >= 2)
					drop_threshold *= 2;

				force_drop_threshold = DEF_VID_FORCE_DROP_THRESHOLD(vid_player.video_frametime);
				if(vid_player.num_of_video_tracks >= 2)
					force_drop_threshold *= 2;

				Util_sync_lock(&vid_player.delay_update_lock, UINT64_MAX);
				Vid_update_video_delay(packet_index);
				video_delay = vid_player.video_delay_ms[packet_index][DEF_VID_DELAY_SAMPLES - 1];
				Util_sync_unlock(&vid_player.delay_update_lock);

				if(video_delay > drop_threshold)
				{
					if(vid_player.drop_threshold_exceeded_ts == 0)
						vid_player.drop_threshold_exceeded_ts = current_ts;

					if((current_ts - vid_player.drop_threshold_exceeded_ts) > DEF_VID_DROP_THRESHOLD_ALLOWED_DURATION(vid_player.video_frametime))
						drop = true;
				}
				else
					vid_player.drop_threshold_exceeded_ts = 0;

				if(video_delay > force_drop_threshold)
					drop = true;
			}
			else
			{
				//No audio time reference available, use local delay.
				if(total_delay > drop_threshold)
				{
					if(vid_player.drop_threshold_exceeded_ts == 0)
						vid_player.drop_threshold_exceeded_ts = current_ts;

					if((current_ts - vid_player.drop_threshold_exceeded_ts) > DEF_VID_DROP_THRESHOLD_ALLOWED_DURATION(vid_player.video_frametime))
					{
						drop = true;
						if(vid_player.num_of_video_tracks >= 2)
							total_delay -= (vid_player.video_frametime * 2);
						else
							total_delay -= vid_player.video_frametime;
					}
				}
				else
					vid_player.drop_threshold_exceeded_ts = 0;

				if(total_delay > force_drop_threshold)
				{
					drop = true;
					if(vid_player.num_of_video_tracks >= 2)
						total_delay -= (vid_player.video_frametime * 2);
					else
						total_delay -= vid_player.video_frametime;
				}
			}
		}

		//Skip video frame if we can't keep up or we are seeking.
		if((drop || vid_player.state == PLAYER_STATE_SEEKING) && vid_player.video_frametime != 0)
		{
			if(num_of_cached_raw_images > 0)
			{
				double pos = 0;

				//We doropped a frame.
				if(vid_player.state != PLAYER_STATE_SEEKING)
					vid_player.total_dropped_frames++;

				if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
				{
					//Hardware decoder can't decode 2 tracks at the same time.
					Util_decoder_mvd_skip_image(&pos, 0);
					vid_player.video_current_pos[0] = pos;
				}
				else
				{
					Util_decoder_video_skip_image(&pos, packet_index, 0);
					vid_player.video_current_pos[packet_index] = pos;
				}
			}
			else
			{
				if(vid_player.state == PLAYER_STATE_SEEKING)
					Util_sleep(3000);
				else if(num_of_cached_raw_images == 0 && vid_player.video_frametime != 0 && (Util_speaker_get_available_buffer_num(0) + 1) < DEF_SPEAKER_MAX_BUFFERS)
				{
					//Notify we've run out of buffer.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
					CONVERT_THREAD_OUT_OF_BUFFER_NOTIFICATION, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
					Util_sleep(3000);
				}
			}

			osTickCounterUpdate(&conversion_time_counter);

			//Get position from video track if duration is longer than or equal to audio track.
			if(vid_player.video_frametime != 0 && (vid_player.num_of_audio_tracks == 0
			|| vid_player.video_info[0].duration >= vid_player.audio_info[vid_player.selected_audio_track].duration))
				vid_player.media_current_pos = vid_player.video_current_pos[0];
		}
		else if(vid_player.state == PLAYER_STATE_PLAYING)
		{
			if(vid_player.num_of_video_tracks >= 2)
				num_of_cached_raw_images = Util_max(Util_decoder_video_get_available_raw_image_num(0, 0), Util_decoder_video_get_available_raw_image_num(1, 0));

			if(num_of_cached_raw_images == 0 && vid_player.video_frametime != 0 && (Util_speaker_get_available_buffer_num(0) + 1) < DEF_SPEAKER_MAX_BUFFERS)
			{
				//Notify we've run out of buffer.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
				CONVERT_THREAD_OUT_OF_BUFFER_NOTIFICATION, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
				Util_sleep(5000);
				continue;
			}
			else if(num_of_cached_raw_images == 0)
			{
				//We've run out of buffer, but frametime is unknown, this is most likely tagged mp3
				//that has only one picture, so don't send out of buffer notification.
				Util_sleep(5000);
				continue;
			}
			else
			{
				uint8_t buffer_health = 0;
				uint8_t next_store_index = vid_player.next_store_index[packet_index];
				uint8_t next_draw_index = vid_player.next_draw_index[packet_index];
				uint8_t* yuv_video = NULL;
				uint8_t* video = NULL;
				uint32_t width = vid_player.video_info[packet_index].codec_width;
				uint32_t height = vid_player.video_info[packet_index].codec_height;
				double pos = 0;

				if(next_draw_index <= next_store_index)
					buffer_health = next_store_index - next_draw_index;
				else
					buffer_health = DEF_VID_VIDEO_BUFFERS - next_draw_index + next_store_index;

				if(buffer_health + 1 >= DEF_VID_VIDEO_BUFFERS)
				{
					//Buffer is full.
					Util_sleep(vid_player.video_frametime * 1000);

					if(vid_player.num_of_audio_tracks >= 1 && Util_speaker_get_available_buffer_num(0) >= 1)
					{
						//Audio exist, sync with audio time.
						//Audio buffer health (in ms) is ((buffer_size / bytes_per_sample / num_of_ch / sample_rate) * 1000).
						double audio_buffer_health_ms = (Util_speaker_get_available_buffer_size(0) / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000);
						audio_buffer_health_ms = (Util_speaker_get_available_buffer_size(0) / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000);
						vid_player.audio_current_pos = vid_player.last_decoded_audio_pos - audio_buffer_health_ms;
					}

					continue;
				}

				osTickCounterUpdate(&conversion_time_counter);

				if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)//Hardware decoder only supports 1 track at a time.
					result = Util_decoder_mvd_get_image(&video, &pos, width, height, 0);
				else
					result = Util_decoder_video_get_image(&yuv_video, &pos, width, height, packet_index, 0);

				//Update audio position.
				if(vid_player.num_of_audio_tracks > 0)
				{
					//Audio buffer health (in ms) is ((buffer_size / bytes_per_sample / num_of_ch / sample_rate) * 1000).
					double audio_buffer_health_ms = (Util_speaker_get_available_buffer_size(0) / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000);
					audio_buffer_health_ms = (Util_speaker_get_available_buffer_size(0) / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000);
					vid_player.audio_current_pos = vid_player.last_decoded_audio_pos - audio_buffer_health_ms;
				}

				if(result == DEF_SUCCESS)
				{
					//Get position from video track if duration is longer than or equal to audio track.
					if(vid_player.video_frametime != 0 && (vid_player.num_of_audio_tracks == 0
					|| vid_player.video_info[0].duration >= vid_player.audio_info[vid_player.selected_audio_track].duration))
					{
						if(vid_player.num_of_video_tracks >= 2)
							vid_player.video_current_pos[packet_index] = pos - (vid_player.video_frametime * buffer_health * 2);
						else
							vid_player.video_current_pos[packet_index] = pos - (vid_player.video_frametime * buffer_health);

						//We use track 0 as a time reference.
						vid_player.media_current_pos = vid_player.video_current_pos[0];
					}

					//Hardware decoder returns BGR565, so we don't have to convert it.
					if(!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING))
					{
						if(vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION)//Use hardware color converter.
							result = Util_converter_y2r_yuv420p_to_rgb565le(yuv_video, &video, width, height, true);
						else//Use software color converter.
						{
							Converter_color_parameters color_converter_parameters = { 0, };
							color_converter_parameters.source = yuv_video;
							color_converter_parameters.converted = NULL;
							color_converter_parameters.in_width = width;
							color_converter_parameters.in_height = height;
							color_converter_parameters.in_color_format = vid_player.video_info[packet_index].pixel_format;
							color_converter_parameters.out_width = width;
							color_converter_parameters.out_height = height;
							color_converter_parameters.out_color_format = RAW_PIXEL_RGB565LE;
							result = Util_converter_convert_color(&color_converter_parameters);
							video = color_converter_parameters.converted;
						}
					}

					//Set texture data.
					if(result == DEF_SUCCESS)
					{
						uint8_t image_index = vid_player.next_store_index[packet_index];

						//We don't need to copy padding area for Y direction.
						height = vid_player.video_info[packet_index].height;

						if(!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) && (vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION))//Raw image is texture format.
							result = Vid_large_texture_set_data(&vid_player.large_image[image_index][packet_index], video, width, height, true);
						else//Raw image is NOT texture format.
							result = Vid_large_texture_set_data(&vid_player.large_image[image_index][packet_index], video, width, height, false);

						if(result != DEF_SUCCESS)
							DEF_LOG_RESULT(Vid_large_texture_set_data, false, result);

						//Crop the image so that user won't see glitch on videos.
						Vid_large_texture_crop(&vid_player.large_image[image_index][packet_index], vid_player.video_info[packet_index].width, vid_player.video_info[packet_index].height);
					}
					else
						DEF_LOG_RESULT(Util_converter_convert_color, false, result);
				}
				else
				{
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
						DEF_LOG_RESULT(Util_decoder_mvd_get_image, false, result);
					else
						DEF_LOG_RESULT(Util_decoder_video_get_image, false, result);

					if(result == DEF_ERR_OUT_OF_MEMORY || result == DEF_ERR_OUT_OF_LINEAR_MEMORY)
					{
						//Give up on this image on memory allocation failure.
						if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
							Util_decoder_mvd_skip_image(&pos, 0);
						else
							Util_decoder_video_skip_image(&pos, packet_index, 0);
					}
				}

				if(result == DEF_SUCCESS)
				{
					//Update buffer index.
					if(next_store_index + 1 < DEF_VID_VIDEO_BUFFERS)
						next_store_index++;
					else
						next_store_index = 0;

					vid_player.next_store_index[packet_index] = next_store_index;

					vid_player.total_rendered_frames++;
				}
				else
				{
					if(result == DEF_ERR_OUT_OF_MEMORY || result == DEF_ERR_OUT_OF_LINEAR_MEMORY)
					{
						//Request to increase amount of RAM to keep.
						DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
						DECODE_THREAD_INCREASE_KEEP_RAM_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST), (result == DEF_SUCCESS), result);
					}

					vid_player.total_dropped_frames++;
					Util_sleep(1000);
				}

				free(yuv_video);
				free(video);
				yuv_video = NULL;
				video = NULL;

				for(uint16_t i = 1; i < 320; i++)
					vid_player.conversion_time_list[i - 1] = vid_player.conversion_time_list[i];

				osTickCounterUpdate(&conversion_time_counter);
				vid_player.conversion_time_list[319] = osTickCounterRead(&conversion_time_counter);

				if(vid_player.num_of_audio_tracks == 0)//No audio time reference available, use local delay.
					total_delay += (vid_player.conversion_time_list[319]) - vid_player.video_frametime;

				if(vid_player.num_of_video_tracks >= 2)
					packet_index = (packet_index == 0 ? 1 : 0);
			}
		}
		else if(vid_player.state == PLAYER_STATE_BUFFERING)
		{
			Util_sync_lock(&vid_player.delay_update_lock, UINT64_MAX);
			Vid_init_desync_data();
			Util_sync_unlock(&vid_player.delay_update_lock);

			if(num_of_cached_raw_images >= vid_player.restart_playback_threshold
			|| Util_speaker_get_available_buffer_num(0) + 1 >= DEF_SPEAKER_MAX_BUFFERS)
			{
				//Notify we've finished buffering.
				DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
				CONVERT_THREAD_FINISHED_BUFFERING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
			}
		}
		else
			Util_sleep(1000);
	}

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

void Vid_read_packet_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;

	while (vid_thread_run)
	{
		Vid_command event = NONE_REQUEST;

		result = Util_queue_get(&vid_player.read_packet_thread_command_queue, (uint32_t*)&event, NULL, DEF_THREAD_ACTIVE_SLEEP_TIME);
		if(result == DEF_SUCCESS)
		{
			switch (event)
			{
				case READ_PACKET_THREAD_READ_PACKET_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					while(true)
					{
						result = Util_decoder_read_packet(0);
						if(result == DEF_SUCCESS)
						{
							if(Util_decoder_get_available_packet_num(0) + 1 >= DEF_DECODER_MAX_CACHE_PACKETS)
							{
								//Notify we've done reading.
								DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
								READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

								break;
							}
						}
						if(result == DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS)
						{
							//Notify we've reached EOF.
							DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
							READ_PACKET_THREAD_FINISHED_READING_EOF_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

							break;
						}
						else if(result != DEF_SUCCESS)
						{
							if(result == DEF_ERR_TRY_AGAIN)
							{
								//Wait and try again.
								Util_sleep(4000);
							}
							else
							{
								DEF_LOG_RESULT(Util_decoder_read_packet, false, result);
								//Notify we've done reading.
								DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
								READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

								break;
							}
						}

						//If we get seek or abort request while reading, break the loop.
						if(Util_queue_check_event_exist(&vid_player.read_packet_thread_command_queue, READ_PACKET_THREAD_SEEK_REQUEST)
						|| Util_queue_check_event_exist(&vid_player.read_packet_thread_command_queue, READ_PACKET_THREAD_ABORT_REQUEST))
						{
							//Notify we've done reading.
							DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
							READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

							break;
						}
					}

					break;
				}

				case READ_PACKET_THREAD_SEEK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					DEF_LOG_RESULT_SMART(result, Util_decoder_seek(vid_player.seek_pos, MEDIA_SEEK_FLAG_BACKWARD, 0),
					(result == DEF_SUCCESS), result);
					Util_decoder_clear_cache_packet(0);

					//Notify we've done seeking.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
					READ_PACKET_THREAD_FINISHED_SEEKING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

					break;
				}

				case READ_PACKET_THREAD_ABORT_REQUEST:
				{
					//Flush the command queue.
					while(true)
					{
						result = Util_queue_get(&vid_player.read_packet_thread_command_queue, (uint32_t*)&event, NULL, 0);
						if(result != DEF_SUCCESS)
							break;
					}

					//Notify we've done aborting.
					DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_notification_queue,
					READ_PACKET_THREAD_FINISHED_ABORTING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);

					break;
				}

				default:
					break;
			}
		}

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_THREAD_INACTIVE_SLEEP_TIME);
		}
	}

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

static void Vid_callback(Str_data* file, Str_data* dir)
{
	uint32_t result = DEF_ERR_OTHER;
	Vid_file* file_data = (Vid_file*)malloc(sizeof(Vid_file));

	//Create a message.
	if(file_data)
	{
		file_data->index = Util_expl_query_current_file_index();
		if(Util_str_has_data(file))
			snprintf(file_data->name, sizeof(file_data->name), "%s", file->buffer);
		if(Util_str_has_data(dir))
			snprintf(file_data->directory, sizeof(file_data->directory), "%s", dir->buffer);
	}

	DEF_LOG_RESULT_SMART(result, Util_queue_add(&vid_player.decode_thread_command_queue,
	DECODE_THREAD_PLAY_REQUEST, file_data, 100000, QUEUE_OPTION_NONE), (result == DEF_SUCCESS), result);
}

static void Vid_cancel(void)
{
}
