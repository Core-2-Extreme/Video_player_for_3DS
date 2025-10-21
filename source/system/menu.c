//Includes.
#include "system/menu.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "3ds.h"

#include "system/sem.h"
#include "system/draw/draw.h"
#include "system/draw/exfont.h"
#include "system/util/cpu_usage.h"
#include "system/util/curl.h"
#include "system/util/err.h"
#include "system/util/expl.h"
#include "system/util/fake_pthread.h"
#include "system/util/file.h"
#include "system/util/hid.h"
#include "system/util/httpc.h"
#include "system/util/hw_config.h"
#include "system/util/json_types.h"
#include "system/util/log.h"
#include "system/util/queue.h"
#include "system/util/str.h"
#include "system/util/sync.h"
#include "system/util/thread_types.h"
#include "system/util/watch.h"
#include "system/util/util.h"
#include "video_player.h"

//Defines.
#define DEF_MENU_NUM_OF_SUB_APP		(uint8_t)(8)
#define DEF_MENU_SEND_INFO_FMT_VER	(uint32_t)(1)

//Exit check.
#define DEF_MENU_HID_EXIT_CFM(k)			(bool)(DEF_HID_PR_EM(k.a, 1) || DEF_HID_HD(k.a))
#define DEF_MENU_HID_EXIT_CANCEL_CFM(k)		(bool)(DEF_HID_PR_EM(k.b,1 ) || DEF_HID_HD(k.b))
//System UI.
#define DEF_MENU_HID_SYSTEM_UI_SEL(k)		(bool)((DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN((*Draw_get_bot_ui_button()), k)) || DEF_HID_PHY_PR(k.start))
#define DEF_MENU_HID_SYSTEM_UI_CFM(k)		(bool)(((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN((*Draw_get_bot_ui_button()), k)) || (DEF_HID_PR_EM(k.start, 1) || DEF_HID_HD(k.start)))
#define DEF_MENU_HID_SYSTEM_UI_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch) && DEF_HID_PHY_NP(k.start))
//Toggle log.
#define DEF_MENU_HID_LOG_CFM(k)				(bool)(DEF_HID_PR_EM(k.select, 1) || DEF_HID_HD(k.select))
//Settings menu.
#define DEF_MENU_HID_SEM_SEL(k)				(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(menu_sem_button, k))
#define DEF_MENU_HID_SEM_CFM(k)				(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(menu_sem_button, k))
#define DEF_MENU_HID_SEM_DESEL(k)			(bool)(DEF_HID_PHY_NP(k.touch))
//Sub applications : Close.
#define DEF_MENU_HID_SAPP_CLOSE_SEL(k, id)	(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(menu_sapp_close_button[id], k))
#define DEF_MENU_HID_SAPP_CLOSE_CFM(k, id)	(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(menu_sapp_close_button[id], k))
#define DEF_MENU_HID_SAPP_CLOSE_DESEL(k)	(bool)(DEF_HID_PHY_NP(k.touch))
//Sub applications : Open.
#define DEF_MENU_HID_SAPP_OPEN_SEL(k, id)	(bool)(DEF_HID_PHY_PR(k.touch) && DEF_HID_INIT_IN(menu_sapp_button[id], k))
#define DEF_MENU_HID_SAPP_OPEN_CFM(k, id)	(bool)((DEF_HID_PR_EM(k.touch, 1) || DEF_HID_HD(k.touch)) && DEF_HID_INIT_LAST_IN(menu_sapp_button[id], k) && !menu_sapp_close_button[id].selected)
#define DEF_MENU_HID_SAPP_OPEN_DESEL(k)		(bool)(DEF_HID_PHY_NP(k.touch))

//Typedefs.
//N/A.

//Prototypes.
static uint32_t Menu_update_main_directory(void);
static void Menu_hid_callback(void);
void Menu_worker_thread(void* arg);

#if (DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)
void Menu_send_app_info_thread(void* arg);
void Menu_update_thread(void* arg);
#endif //(DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)

//Variables.
static bool menu_thread_run = false;
static bool menu_main_run = true;
static bool menu_must_exit = false;
static bool menu_check_exit_request = false;
static bool menu_update_available = false;
static bool menu_init_request[DEF_MENU_NUM_OF_SUB_APP] = { 0, };
static bool menu_exit_request[DEF_MENU_NUM_OF_SUB_APP] = { 0, };
static uint32_t menu_icon_texture_num[DEF_MENU_NUM_OF_SUB_APP + 1] = { 0, };
static void (*menu_worker_thread_callbacks[DEF_MENU_NUM_OF_CALLBACKS])(void) = { 0, };
static Str_data menu_msg[DEF_MENU_NUM_OF_MSG] = { 0, };
static Thread menu_worker_thread = NULL;
static Sync_data menu_callback_mutex = { 0, };
static Draw_image_data menu_icon_image[DEF_MENU_NUM_OF_SUB_APP + 2] = { 0, };
static Draw_image_data menu_sapp_button[DEF_MENU_NUM_OF_SUB_APP] = { 0, };
static Draw_image_data menu_sapp_close_button[DEF_MENU_NUM_OF_SUB_APP] = { 0, };
static Draw_image_data menu_sem_button = { 0, };

#if (DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)
static Thread menu_send_app_info_thread = NULL, menu_update_thread = NULL;
#endif //(DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)

//Code.
bool Menu_query_must_exit_flag(void)
{
	return menu_must_exit;
}

void Menu_set_must_exit_flag(bool flag)
{
	menu_must_exit = flag;
}

void Menu_resume(void)
{
	menu_main_run = true;
	//Reset key state on scene change.
	Util_hid_reset_key_state(HID_KEY_BIT_ALL);
	Draw_set_refresh_needed(true);
}

void Menu_suspend(void)
{
	menu_main_run = false;
}

uint32_t Menu_load_msg(const char* lang)
{
	char file_name[32] = { 0, };

	snprintf(file_name, sizeof(file_name), "menu_%s.txt", (lang ? lang : ""));
	return Util_load_msg(file_name, menu_msg, DEF_MENU_NUM_OF_MSG);
}

void Menu_init(void)
{
	bool is_800px = false;
	bool is_3d = false;
	uint8_t dummy = 0;
	uint32_t sync_init_result = DEF_ERR_OTHER;
	uint32_t queue_init_result = DEF_ERR_OTHER;
	uint32_t watch_init_result = DEF_ERR_OTHER;
	uint32_t result = DEF_ERR_OTHER;
	uint32_t update_main_dir_result = DEF_ERR_OTHER;
	C2D_Image cache[2] = { 0, };
	Sem_config config = { 0, };
	Sem_state state = { 0, };

	for(uint16_t i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
		menu_worker_thread_callbacks[i] = NULL;

	for(uint16_t i = 0; i < (DEF_MENU_NUM_OF_SUB_APP + 1); i++)
		menu_icon_texture_num[i] = UINT32_MAX;

	sync_init_result = Util_sync_init();
	queue_init_result = Util_queue_init();
	watch_init_result = Util_watch_init();
	result = Util_log_init();

	//We must log after initializing log API.
	DEF_LOG_RESULT(Util_sync_init, (sync_init_result == DEF_SUCCESS), sync_init_result);
	DEF_LOG_RESULT(Util_queue_init, (queue_init_result == DEF_SUCCESS), queue_init_result);
	DEF_LOG_RESULT(Util_watch_init, (watch_init_result == DEF_SUCCESS), watch_init_result);
	DEF_LOG_RESULT(Util_log_init, (result == DEF_SUCCESS), result);
	DEF_LOG_FORMAT("Initializing...v%s", DEF_MENU_CURRENT_APP_VER);

	DEF_LOG_RESULT_SMART(result, Util_sync_create(&menu_callback_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX), (result == DEF_SUCCESS), result);

	osSetSpeedupEnable(true);
	aptSetSleepAllowed(true);
	svcSetThreadPriority(CUR_THREAD_HANDLE, DEF_THREAD_PRIORITY_HIGH - 1);

	//Init system modules.
	DEF_LOG_RESULT_SMART(result, fsInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, acInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, aptInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, mcuHwcInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, ptmuInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, romfsInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, cfguInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, amInit(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, APT_SetAppCpuTimeLimit(30), (result == DEF_SUCCESS), result);

	//Move data directory.
	update_main_dir_result = Menu_update_main_directory();

	//Create directories.
	Util_file_save_to_file(".", DEF_MENU_MAIN_DIR, &dummy, 1, true);
	Util_file_save_to_file(".", DEF_MENU_MAIN_DIR "screen_recording/", &dummy, 1, true);
	Util_file_save_to_file(".", DEF_MENU_MAIN_DIR "error/", &dummy, 1, true);
	Util_file_save_to_file(".", DEF_MENU_MAIN_DIR "logs/", &dummy, 1, true);
	Util_file_save_to_file(".", DEF_MENU_MAIN_DIR "ver/", &dummy, 1, true);

	//Init our modules.
	DEF_LOG_RESULT_SMART(result, Util_init(), (result == DEF_SUCCESS), result);

	Sem_init();
	Sem_suspend();
	Sem_get_config(&config);
	Sem_get_state(&state);

	if(config.screen_mode == DEF_SEM_SCREEN_MODE_AUTO)
	{
		if(osGet3DSliderState())
			is_3d = true;
		else
			is_800px = true;
	}
	else if(config.screen_mode == DEF_SEM_SCREEN_MODE_800PX)
		is_800px = true;
	else if(config.screen_mode == DEF_SEM_SCREEN_MODE_3D)
		is_3d = true;

	DEF_LOG_RESULT_SMART(result, Draw_init(is_800px, is_3d), (result == DEF_SUCCESS), result);

	//Init screen.
	Draw_frame_ready();
	Draw_screen_ready(DRAW_SCREEN_TOP_LEFT, DEF_DRAW_WHITE);
	Draw_top_ui(false, false, DEF_SEM_WIFI_SIGNAL_DISABLED, 0, NULL);
	Draw_screen_ready(DRAW_SCREEN_BOTTOM, DEF_DRAW_WHITE);
	Draw_bot_ui();
	Draw_apply_draw();
	Sem_draw_init();

	//Init rest of our modules.
	DEF_LOG_RESULT_SMART(result, Util_httpc_init(DEF_MENU_HTTP_POST_BUFFER_SIZE), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_curl_init(DEF_MENU_SOCKET_BUFFER_SIZE), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_hid_init(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_hid_add_callback(Menu_hid_callback), result, result);
	DEF_LOG_RESULT_SMART(result, Util_expl_init(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Exfont_init(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_err_init(), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_fake_pthread_init(), (result == DEF_SUCCESS), result);

	if(update_main_dir_result != DEF_SUCCESS)
	{
		const char* msg = ("/Video_player/ -> " DEF_MENU_MAIN_DIR "\nMaybe destination directory already exist?");

		//We need to call error API after calling Util_err_init().
		Util_err_set_error_message("Failed to move app data directory.", msg, DEF_LOG_GET_SYMBOL(), update_main_dir_result);
		Util_err_set_show_flag(true);
	}

	for (uint16_t i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
		Exfont_set_external_font_request_state(i, true);

	Exfont_request_load_external_font();

	menu_thread_run = true;
	menu_worker_thread = threadCreate(Menu_worker_thread, NULL, DEF_THREAD_STACKSIZE * 2, DEF_THREAD_PRIORITY_ABOVE_NORMAL, 0, false);

#if (DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)
	menu_update_thread = threadCreate(Menu_update_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 1, true);
	if (config.is_send_info_allowed)
		menu_send_app_info_thread = threadCreate(Menu_send_app_info_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_LOW, 1, true);
#endif //(DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)

	//Load sub application icons.
#ifdef DEF_VID_ENABLE_ICON
	menu_icon_texture_num[0] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_VID_ICON_PATH, menu_icon_texture_num[0], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[0].c2d = cache[0];
	if(result == DEF_SUCCESS)
		Draw_set_texture_filter(&menu_icon_image[0], true);
#endif //DEF_VID_ENABLE_ICON

#ifdef DEF_SAPP1_ENABLE_ICON
	menu_icon_texture_num[1] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SAPP1_ICON_PATH, menu_icon_texture_num[1], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[1].c2d = cache[0];
#endif //DEF_SAPP1_ENABLE_ICON

#ifdef DEF_SAPP2_ENABLE_ICON
	menu_icon_texture_num[2] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SAPP2_ICON_PATH, menu_icon_texture_num[2], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[2].c2d = cache[0];
#endif //DEF_SAPP2_ENABLE_ICON

#ifdef DEF_SAPP3_ENABLE_ICON
	menu_icon_texture_num[3] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SAPP3_ICON_PATH, menu_icon_texture_num[3], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[3].c2d = cache[0];
#endif //DEF_SAPP3_ENABLE_ICON

#ifdef DEF_SAPP4_ENABLE_ICON
	menu_icon_texture_num[4] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SAPP4_ICON_PATH, menu_icon_texture_num[4], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[4].c2d = cache[0];
#endif //DEF_SAPP4_ENABLE_ICON

#ifdef DEF_SAPP5_ENABLE_ICON
	menu_icon_texture_num[5] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SAPP5_ICON_PATH, menu_icon_texture_num[5], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[5].c2d = cache[0];
#endif //DEF_SAPP5_ENABLE_ICON

#ifdef DEF_SAPP6_ENABLE_ICON
	menu_icon_texture_num[6] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SAPP6_ICON_PATH, menu_icon_texture_num[6], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[6].c2d = cache[0];
#endif //DEF_SAPP6_ENABLE_ICON

#ifdef DEF_SAPP7_ENABLE_ICON
	menu_icon_texture_num[7] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SAPP7_ICON_PATH, menu_icon_texture_num[7], cache, 0, 1), (result == DEF_SUCCESS), result);
	menu_icon_image[7].c2d = cache[0];
#endif //DEF_SAPP7_ENABLE_ICON

#ifdef DEF_SEM_ENABLE_ICON
	menu_icon_texture_num[8] = Draw_get_free_sheet_num();
	DEF_LOG_RESULT_SMART(result, Draw_load_texture(DEF_SEM_ICON_PATH, menu_icon_texture_num[8], cache, 0, 2), (result == DEF_SUCCESS), result);
	menu_icon_image[8].c2d = cache[0];
	menu_icon_image[9].c2d = cache[1];
#endif //DEF_SEM_ENABLE_ICON

	for(uint8_t i = 0; i < DEF_MENU_NUM_OF_SUB_APP; i++)
	{
		menu_sapp_button[i] = Draw_get_empty_image();
		menu_sapp_close_button[i] = Draw_get_empty_image();
	}
	menu_sem_button = Draw_get_empty_image();

	Util_watch_add(WATCH_HANDLE_MAIN_MENU, &menu_must_exit, sizeof(menu_must_exit));
	Util_watch_add(WATCH_HANDLE_MAIN_MENU, &menu_check_exit_request, sizeof(menu_check_exit_request));

	Util_watch_add(WATCH_HANDLE_MAIN_MENU, &menu_sem_button.selected, sizeof(menu_sem_button.selected));
	for(uint8_t i = 0; i < DEF_MENU_NUM_OF_SUB_APP; i++)
	{
		Util_watch_add(WATCH_HANDLE_MAIN_MENU, &menu_init_request[i], sizeof(menu_init_request[i]));
		Util_watch_add(WATCH_HANDLE_MAIN_MENU, &menu_exit_request[i], sizeof(menu_exit_request[i]));

		Util_watch_add(WATCH_HANDLE_MAIN_MENU, &menu_sapp_button[i].selected, sizeof(menu_sapp_button[i].selected));
		Util_watch_add(WATCH_HANDLE_MAIN_MENU, &menu_sapp_close_button[i].selected, sizeof(menu_sapp_close_button[i].selected));
	}

	Menu_resume();
	DEF_LOG_STRING("Initialized.");
}

void Menu_exit(void)
{
	DEF_LOG_STRING("Exiting...");
	bool draw = !aptShouldClose();
	uint32_t result = DEF_ERR_OTHER;

	menu_thread_run = false;

#ifdef DEF_VID_ENABLE
	if (Vid_query_init_flag())
		Vid_exit(draw);
#endif //DEF_VID_ENABLE

#ifdef DEF_SAPP1_ENABLE
	if (Sapp1_query_init_flag())
		Sapp1_exit(draw);
#endif //DEF_SAPP1_ENABLE

#ifdef DEF_SAPP2_ENABLE
	if (Sapp2_query_init_flag())
		Sapp2_exit(draw);
#endif //DEF_SAPP2_ENABLE

#ifdef DEF_SAPP3_ENABLE
	if (Sapp3_query_init_flag())
		Sapp3_exit(draw);
#endif //DEF_SAPP3_ENABLE

#ifdef DEF_SAPP4_ENABLE
	if (Sapp4_query_init_flag())
		Sapp4_exit(draw);
#endif //DEF_SAPP4_ENABLE

#ifdef DEF_SAPP5_ENABLE
	if (Sapp5_query_init_flag())
		Sapp5_exit(draw);
#endif //DEF_SAPP5_ENABLE

#ifdef DEF_SAPP6_ENABLE
	if (Sapp6_query_init_flag())
		Sapp6_exit(draw);
#endif //DEF_SAPP6_ENABLE

#ifdef DEF_SAPP7_ENABLE
	if (Sapp7_query_init_flag())
		Sapp7_exit(draw);
#endif //DEF_SAPP7_ENABLE

	if (Sem_query_init_flag())
		Sem_exit();

	for(uint8_t i = 0; i < (DEF_MENU_NUM_OF_SUB_APP + 1); i++)
		Draw_free_texture(menu_icon_texture_num[i]);

	Util_hid_remove_callback(Menu_hid_callback);
	Util_hid_exit();
	Util_expl_exit();
	Exfont_exit();
	Util_err_exit();
	Util_exit();
	Util_cpu_usage_exit();
	Util_fake_pthread_exit();

	DEF_LOG_RESULT_SMART(result, threadJoin(menu_worker_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	threadFree(menu_worker_thread);

#if (DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)
	DEF_LOG_RESULT_SMART(result, threadJoin(menu_send_app_info_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, threadJoin(menu_update_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
#endif //(DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)

	Util_watch_remove(WATCH_HANDLE_MAIN_MENU, &menu_must_exit);
	Util_watch_remove(WATCH_HANDLE_MAIN_MENU, &menu_check_exit_request);

	Util_watch_remove(WATCH_HANDLE_MAIN_MENU, &menu_sem_button.selected);
	for(uint8_t i = 0; i < DEF_MENU_NUM_OF_SUB_APP; i++)
	{
		Util_watch_remove(WATCH_HANDLE_MAIN_MENU, &menu_init_request[i]);
		Util_watch_remove(WATCH_HANDLE_MAIN_MENU, &menu_exit_request[i]);

		Util_watch_remove(WATCH_HANDLE_MAIN_MENU, &menu_sapp_button[i].selected);
		Util_watch_remove(WATCH_HANDLE_MAIN_MENU, &menu_sapp_close_button[i].selected);
	}

	Util_watch_exit();
	Util_log_exit();
	Util_httpc_exit();
	Util_curl_exit();

	fsExit();
	acExit();
	aptExit();
	mcuHwcExit();
	ptmuExit();
	romfsExit();
	cfguExit();
	amExit();
	Draw_exit();

	Util_sync_destroy(&menu_callback_mutex);

	Util_queue_exit();
	Util_sync_exit();

	DEF_LOG_STRING("Exited.");
}

bool Menu_add_worker_thread_callback(void (*const callback)(void))
{
	Util_sync_lock(&menu_callback_mutex, UINT64_MAX);

	for(uint16_t i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
	{
		if(menu_worker_thread_callbacks[i] == callback)
			goto success;//Already exist.
	}

	for(uint16_t i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
	{
		if(!menu_worker_thread_callbacks[i])
		{
			menu_worker_thread_callbacks[i] = callback;
			goto success;
		}
	}

	//No free spaces left.
	Util_sync_unlock(&menu_callback_mutex);
	return false;

	success:
	Util_sync_unlock(&menu_callback_mutex);
	return true;
}

void Menu_remove_worker_thread_callback(void (*const callback)(void))
{
	Util_sync_lock(&menu_callback_mutex, UINT64_MAX);

	for(uint16_t i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
	{
		if(menu_worker_thread_callbacks[i] == callback)
		{
			menu_worker_thread_callbacks[i] = NULL;
			break;
		}
	}

	Util_sync_unlock(&menu_callback_mutex);
}

void Menu_main(void)
{
	bool is_800px = false;
	bool is_3d = false;
	uint32_t color = DEF_DRAW_BLACK;
	uint32_t back_color = DEF_DRAW_WHITE;
	Sem_config config = { 0, };
	Sem_state state = { 0, };

	Sem_get_config(&config);
	Sem_get_state(&state);

	is_800px = (config.screen_mode == DEF_SEM_SCREEN_MODE_800PX);
	is_3d = (config.screen_mode == DEF_SEM_SCREEN_MODE_3D);
	if(config.screen_mode == DEF_SEM_SCREEN_MODE_AUTO)
	{
		if(osGet3DSliderState())
		{
			is_3d = true;
			is_800px = false;
		}
		else
		{
			is_3d = false;
			is_800px = ((state.console_model == DEF_SEM_MODEL_OLD2DS) ? false : true);
		}
	}

	if(state.console_model == DEF_SEM_MODEL_OLD2DS && is_800px)
	{
		is_800px = false;
		config.screen_mode = DEF_SEM_SCREEN_MODE_AUTO;
		Sem_set_config(&config);
	}
	if((state.console_model == DEF_SEM_MODEL_OLD2DS || state.console_model == DEF_SEM_MODEL_NEW2DSXL) && is_3d)
	{
		is_3d = false;
		config.screen_mode = DEF_SEM_SCREEN_MODE_AUTO;
		Sem_set_config(&config);
	}

	//Update screen mode here.
	if(is_3d != Draw_is_3d_mode() || is_800px != Draw_is_800px_mode())
	{
		uint32_t result = DEF_ERR_OTHER;

		DEF_LOG_RESULT_SMART(result, Draw_reinit(is_800px, is_3d), (result == DEF_SUCCESS), result);
		Draw_set_refresh_needed(true);
	}

	if(config.is_debug)
		Draw_set_refresh_needed(true);

	if (menu_main_run)
	{
		Watch_handle_bit watch_handle_bit = (DEF_WATCH_HANDLE_BIT_GLOBAL | DEF_WATCH_HANDLE_BIT_MAIN_MENU);

		if(Util_err_query_show_flag())
			watch_handle_bit |= DEF_WATCH_HANDLE_BIT_ERR;
		if(Util_log_query_show_flag())
			watch_handle_bit |= DEF_WATCH_HANDLE_BIT_LOG;

		//Check if we should update the screen.
		if(Util_watch_is_changed(watch_handle_bit) || Draw_is_refresh_needed() || !config.is_eco)
		{
			Draw_set_refresh_needed(false);

			if (config.is_night)
			{
				color = DEF_DRAW_WHITE;
				back_color = DEF_DRAW_BLACK;
			}

			Draw_frame_ready();
			Draw_screen_ready(DRAW_SCREEN_TOP_LEFT, back_color);

			if(menu_check_exit_request)
			{
				Draw_align(&menu_msg[DEF_MENU_EXIST_MSG], 0, 105, 0.5, 0.5, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 400, 20);
				Draw_align(&menu_msg[DEF_MENU_CONFIRM_MSG], 10, 140, 0.5, 0.5, DEF_DRAW_GREEN, DRAW_X_ALIGN_RIGHT, DRAW_Y_ALIGN_CENTER, 190, 20);
				Draw_align(&menu_msg[DEF_MENU_CANCEL_MSG], 210, 140, 0.5, 0.5, DEF_DRAW_RED, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER, 190, 20);
			}
			else if(menu_update_available)
			{
				Draw(&menu_msg[DEF_MENU_NEW_VERSION_MSG], 10, 30, 0.7, 0.7, DEF_DRAW_RED);
				Draw(&menu_msg[DEF_MENU_HOW_TO_UPDATE_MSG], 10, 60, 0.5, 0.5, color);
			}

			if(Util_log_query_show_flag())
				Util_log_draw();

			Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

			if(config.is_debug)
				Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

			if(Util_cpu_usage_query_show_flag())
				Util_cpu_usage_draw();

			Draw_screen_ready(DRAW_SCREEN_BOTTOM, back_color);

#ifdef DEF_VID_ENABLE
			Draw_texture(&menu_sapp_button[0], menu_sapp_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 60, 100, 100);

#ifdef DEF_VID_ENABLE_ICON
			Draw_texture(&menu_icon_image[0], DEF_DRAW_NO_COLOR, 110, 60, 100, 100);
#endif //DEF_VID_ENABLE_ICON
#ifdef DEF_VID_ENABLE_NAME
			Draw_align_c(DEF_VID_NAME, 110, 60, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 100, 100);
#endif //DEF_VID_ENABLE_NAME

			if(Vid_query_init_flag())
			{
				Draw_with_background_c("X", 195, 60, 0.5, 0.5, DEF_DRAW_RED, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 15, 15,
				DRAW_BACKGROUND_ENTIRE_BOX, &menu_sapp_close_button[0], menu_sapp_close_button[0].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED);
			}
#endif //DEF_VID_ENABLE

#ifdef DEF_SAPP1_ENABLE
			Draw_texture(&menu_sapp_button[1], menu_sapp_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 80, 0, 60, 60);

#ifdef DEF_SAPP1_ENABLE_ICON
			Draw_texture(&menu_icon_image[1], DEF_DRAW_NO_COLOR, 80, 0, 60, 60);
#endif //DEF_SAPP1_ENABLE_ICON
#ifdef DEF_SAPP1_ENABLE_NAME
			Draw_align_c(DEF_SAPP1_NAME, 80, 0, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 60, 60);
#endif //DEF_SAPP1_ENABLE_NAME

			if(Sapp1_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[1], menu_sapp_close_button[1].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 125, 0, 15, 15.0);
				Draw_c("X", 127.5, 0, 0.5, 0.5, DEF_DRAW_RED);
			}
#endif //DEF_SAPP1_ENABLE

#ifdef DEF_SAPP2_ENABLE
			Draw_texture(&menu_sapp_button[2], menu_sapp_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 160, 0, 60, 60);

#ifdef DEF_SAPP2_ENABLE_ICON
			Draw_texture(&menu_icon_image[2], DEF_DRAW_NO_COLOR, 160, 0, 60, 60);
#endif //DEF_SAPP2_ENABLE_ICON
#ifdef DEF_SAPP2_ENABLE_NAME
			Draw_align_c(DEF_SAPP2_NAME, 160, 0, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 60, 60);
#endif //DEF_SAPP2_ENABLE_NAME

			if(Sapp2_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[2], menu_sapp_close_button[2].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 205, 0, 15, 15.0);
				Draw_c("X", 207.5, 0, 0.5, 0.5, DEF_DRAW_RED);
			}
#endif //DEF_SAPP2_ENABLE

#ifdef DEF_SAPP3_ENABLE
			Draw_texture(&menu_sapp_button[3], menu_sapp_button[3].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 240, 0, 60, 60);

#ifdef DEF_SAPP3_ENABLE_ICON
			Draw_texture(&menu_icon_image[3], DEF_DRAW_NO_COLOR, 240, 0, 60, 60);
#endif //DEF_SAPP3_ENABLE_ICON
#ifdef DEF_SAPP3_ENABLE_NAME
			Draw_align_c(DEF_SAPP3_NAME, 240, 0, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 60, 60);
#endif //DEF_SAPP3_ENABLE_NAME

			if(Sapp3_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[3], menu_sapp_close_button[3].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 285, 0, 15, 15.0);
				Draw_c("X", 287.5, 0, 0.5, 0.5, DEF_DRAW_RED);
			}
#endif //DEF_SAPP3_ENABLE

#ifdef DEF_SAPP4_ENABLE
			Draw_texture(&menu_sapp_button[4], menu_sapp_button[4].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 80, 60, 60);

#ifdef DEF_SAPP4_ENABLE_ICON
			Draw_texture(&menu_icon_image[4], DEF_DRAW_NO_COLOR, 0, 80, 60, 60);
#endif //DEF_SAPP4_ENABLE_ICON
#ifdef DEF_SAPP4_ENABLE_NAME
			Draw_align_c(DEF_SAPP4_NAME, 0, 80, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 60, 60);
#endif //DEF_SAPP4_ENABLE_NAME

			if(Sapp4_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[4], menu_sapp_close_button[4].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 45, 80, 15, 15.0);
				Draw_c("X", 47.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
#endif //DEF_SAPP4_ENABLE

#ifdef DEF_SAPP5_ENABLE
			Draw_texture(&menu_sapp_button[5], menu_sapp_button[5].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 80, 80, 60, 60);

#ifdef DEF_SAPP5_ENABLE_ICON
			Draw_texture(&menu_icon_image[5], DEF_DRAW_NO_COLOR, 80, 80, 60, 60);
#endif //DEF_SAPP5_ENABLE_ICON
#ifdef DEF_SAPP5_ENABLE_NAME
			Draw_align_c(DEF_SAPP5_NAME, 80, 80, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 60, 60);
#endif //DEF_SAPP5_ENABLE_NAME

			if(Sapp5_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[5], menu_sapp_close_button[5].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 125, 80, 15, 15.0);
				Draw_c("X", 127.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
#endif //DEF_SAPP5_ENABLE

#ifdef DEF_SAPP6_ENABLE
			Draw_texture(&menu_sapp_button[6], menu_sapp_button[6].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 160, 80, 60, 60);

#ifdef DEF_SAPP6_ENABLE_ICON
			Draw_texture(&menu_icon_image[6], DEF_DRAW_NO_COLOR, 160, 80, 60, 60);
#endif //DEF_SAPP6_ENABLE_ICON
#ifdef DEF_SAPP6_ENABLE_NAME
			Draw_align_c(DEF_SAPP6_NAME, 160, 80, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 60, 60);
#endif //DEF_SAPP6_ENABLE_NAME

			if(Sapp6_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[6], menu_sapp_close_button[6].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 205, 80, 15, 15.0);
				Draw_c("X", 207.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
#endif //DEF_SAPP6_ENABLE

#ifdef DEF_SAPP7_ENABLE
			Draw_texture(&menu_sapp_button[7], menu_sapp_button[7].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 240, 80, 60, 60);

#ifdef DEF_SAPP7_ENABLE_ICON
			Draw_texture(&menu_icon_image[7], DEF_DRAW_NO_COLOR, 240, 80, 60, 60);
#endif //DEF_SAPP7_ENABLE_ICON
#ifdef DEF_SAPP7_ENABLE_NAME
			Draw_align_c(DEF_SAPP7_NAME, 240, 80, 0.4, 0.4, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, 60, 60);
#endif //DEF_SAPP7_ENABLE_NAME

			if(Sapp7_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[7], menu_sapp_close_button[7].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 285, 80, 15, 15.0);
				Draw_c("X", 287.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
#endif //DEF_SAPP7_ENABLE_ICON

			Draw_texture(&menu_sem_button, menu_sem_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 260, 170, 60, 60);

#ifdef DEF_SEM_ENABLE_ICON
			Draw_texture(&menu_icon_image[8 + config.is_night], DEF_DRAW_NO_COLOR, 260, 170, 60, 60);
#endif //DEF_SEM_ENABLE_ICON
#ifdef DEF_SEM_ENABLE_NAME
			Draw_c(DEF_SEM_NAME, 270, 205, 0.4, 0.4, color);
#endif //DEF_SEM_ENABLE_NAME

			if(Util_err_query_show_flag())
				Util_err_draw();

			Draw_bot_ui();

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();

#ifdef DEF_VID_ENABLE
		if(menu_init_request[0])
		{
			Vid_init(true);
			menu_init_request[0] = false;
		}
		else if(menu_exit_request[0])
		{
			Vid_exit(true);
			menu_exit_request[0] = false;
		}
#endif //DEF_VID_ENABLE

#ifdef DEF_SAPP1_ENABLE
		if(menu_init_request[1])
		{
			Sapp1_init(true);
			menu_init_request[1] = false;
		}
		else if(menu_exit_request[1])
		{
			Sapp1_exit(true);
			menu_exit_request[1] = false;
		}
#endif //DEF_SAPP1_ENABLE

#ifdef DEF_SAPP2_ENABLE
		if(menu_init_request[2])
		{
			Sapp2_init(true);
			menu_init_request[2] = false;
		}
		else if(menu_exit_request[2])
		{
			Sapp2_exit(true);
			menu_exit_request[2] = false;
		}
#endif //DEF_SAPP2_ENABLE

#ifdef DEF_SAPP3_ENABLE
		if(menu_init_request[3])
		{
			Sapp3_init(true);
			menu_init_request[3] = false;
		}
		else if(menu_exit_request[3])
		{
			Sapp3_exit(true);
			menu_exit_request[3] = false;
		}
#endif //DEF_SAPP3_ENABLE

#ifdef DEF_SAPP4_ENABLE
		if(menu_init_request[4])
		{
			Sapp4_init(true);
			menu_init_request[4] = false;
		}
		else if(menu_exit_request[4])
		{
			Sapp4_exit(true);
			menu_exit_request[4] = false;
		}
#endif //DEF_SAPP4_ENABLE

#ifdef DEF_SAPP5_ENABLE
		if(menu_init_request[5])
		{
			Sapp5_init(true);
			menu_init_request[5] = false;
		}
		else if(menu_exit_request[5])
		{
			Sapp5_exit(true);
			menu_exit_request[5] = false;
		}
#endif //DEF_SAPP5_ENABLE

#ifdef DEF_SAPP6_ENABLE
		if(menu_init_request[6])
		{
			Sapp6_init(true);
			menu_init_request[6] = false;
		}
		else if(menu_exit_request[6])
		{
			Sapp6_exit(true);
			menu_exit_request[6] = false;
		}
#endif //DEF_SAPP6_ENABLE

#ifdef DEF_SAPP7_ENABLE
		if(menu_init_request[7])
		{
			Sapp7_init(true);
			menu_init_request[7] = false;
		}
		else if(menu_exit_request[7])
		{
			Sapp7_exit(true);
			menu_exit_request[7] = false;
		}
#endif //DEF_SAPP7_ENABLE
	}

#ifdef DEF_VID_ENABLE
	else if (Vid_query_running_flag())
		Vid_main();
#endif //DEF_VID_ENABLE

#ifdef DEF_SAPP1_ENABLE
	else if (Sapp1_query_running_flag())
		Sapp1_main();
#endif //DEF_SAPP1_ENABLE

#ifdef DEF_SAPP2_ENABLE
	else if (Sapp2_query_running_flag())
		Sapp2_main();
#endif //DEF_SAPP2_ENABLE

#ifdef DEF_SAPP3_ENABLE
	else if (Sapp3_query_running_flag())
		Sapp3_main();
#endif //DEF_SAPP3_ENABLE

#ifdef DEF_SAPP4_ENABLE
	else if (Sapp4_query_running_flag())
		Sapp4_main();
#endif //DEF_SAPP4_ENABLE

#ifdef DEF_SAPP5_ENABLE
	else if (Sapp5_query_running_flag())
		Sapp5_main();
#endif //DEF_SAPP5_ENABLE

#ifdef DEF_SAPP6_ENABLE
	else if (Sapp6_query_running_flag())
		Sapp6_main();
#endif //DEF_SAPP6_ENABLE

#ifdef DEF_SAPP7_ENABLE
	else if (Sapp7_query_running_flag())
		Sapp7_main();
#endif //DEF_SAPP7_ENABLE

	else if (Sem_query_running_flag())
		Sem_main();
	else
		menu_main_run = true;
}

static uint32_t Menu_update_main_directory(void)
{
	const char* old_main_dir = "/Video_player";
	char new_main_dir[] = DEF_MENU_MAIN_DIR;
	Handle fs_handle = 0;
	FS_Archive archive = 0;
	uint32_t result = DEF_ERR_OTHER;

	//Remove last slash ("/").
	new_main_dir[sizeof(new_main_dir) - 1] = 0x00;

	result = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
	if(result != DEF_SUCCESS)
	{
		result = FSUSER_OpenDirectory(&fs_handle, archive, fsMakePath(PATH_ASCII, old_main_dir));
		if(result != DEF_SUCCESS)
		{
			FSDIR_Close(fs_handle);
			result = FSUSER_RenameDirectory(archive, fsMakePath(PATH_ASCII, old_main_dir), archive, fsMakePath(PATH_ASCII, new_main_dir));
			if(result != DEF_SUCCESS)
				DEF_LOG_RESULT(FSUSER_RenameDirectory, false, result);
		}
		else//No old directory was found.
			result = DEF_SUCCESS;
	}
	else
		DEF_LOG_RESULT(FSUSER_OpenArchive, false, result);

	FSUSER_CloseArchive(archive);

	return result;
}

static void Menu_hid_callback(void)
{
	Hid_info key = { 0, };

	Util_hid_query_key_state(&key);
	if(menu_main_run)
	{
		if(!aptShouldJumpToHome())
		{
			if (Util_err_query_show_flag())
				Util_err_main(key);
			else
			{
				//Notify user that button is being pressed.
				if(!menu_check_exit_request)
				{
					if(DEF_MENU_HID_SYSTEM_UI_SEL(key))
						Draw_get_bot_ui_button()->selected = true;
					if(DEF_MENU_HID_SEM_SEL(key))
						menu_sem_button.selected = true;

#ifdef DEF_VID_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 0) && Vid_query_init_flag())
						menu_sapp_close_button[0].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 0))
						menu_sapp_button[0].selected = true;
#endif //DEF_VID_ENABLE

#ifdef DEF_SAPP1_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 1) && Sapp1_query_init_flag())
						menu_sapp_close_button[1].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 1))
						menu_sapp_button[1].selected = true;
#endif //DEF_SAPP1_ENABLE

#ifdef DEF_SAPP2_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 2) && Sapp2_query_init_flag())
						menu_sapp_close_button[2].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 2))
						menu_sapp_button[2].selected = true;
#endif //DEF_SAPP2_ENABLE

#ifdef DEF_SAPP3_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 3) && Sapp3_query_init_flag())
						menu_sapp_close_button[3].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 3))
						menu_sapp_button[3].selected = true;
#endif //DEF_SAPP3_ENABLE

#ifdef DEF_SAPP4_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 4) && Sapp4_query_init_flag())
						menu_sapp_close_button[4].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 4))
						menu_sapp_button[4].selected = true;
#endif //DEF_SAPP4_ENABLE

#ifdef DEF_SAPP5_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 5) && Sapp5_query_init_flag())
						menu_sapp_close_button[5].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 5))
						menu_sapp_button[5].selected = true;
#endif //DEF_SAPP5_ENABLE

#ifdef DEF_SAPP6_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 6) && Sapp6_query_init_flag())
						menu_sapp_close_button[6].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 6))
						menu_sapp_button[6].selected = true;
#endif //DEF_SAPP6_ENABLE

#ifdef DEF_SAPP7_ENABLE
					if (DEF_MENU_HID_SAPP_CLOSE_SEL(key, 7) && Sapp7_query_init_flag())
						menu_sapp_close_button[7].selected = true;
					else if (DEF_MENU_HID_SAPP_OPEN_SEL(key, 7))
						menu_sapp_button[7].selected = true;
#endif //DEF_SAPP7_ENABLE
				}

				//Execute functions if conditions are satisfied.
				if(menu_check_exit_request)
				{
					if (DEF_MENU_HID_EXIT_CFM(key))
						menu_must_exit = true;
					else if (DEF_MENU_HID_EXIT_CANCEL_CFM(key))
						menu_check_exit_request = false;
				}
				else
				{
					if(DEF_MENU_HID_SYSTEM_UI_CFM(key))
						menu_check_exit_request = true;
					else if (DEF_MENU_HID_LOG_CFM(key))
						Util_log_set_show_flag(!Util_log_query_show_flag());
					else if (DEF_MENU_HID_SEM_CFM(key))
						Sem_resume();

#ifdef DEF_VID_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 0) && Vid_query_init_flag())
					{
						menu_exit_request[0] = true;
						while(menu_exit_request[0])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 0))
					{
						if (!Vid_query_init_flag())
						{
							menu_init_request[0] = true;
							while(menu_init_request[0])
								Util_sleep(20000);
						}
						else
							Vid_resume();
					}
#endif //DEF_VID_ENABLE

#ifdef DEF_SAPP1_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 1) && Sapp1_query_init_flag())
					{
						menu_exit_request[1] = true;
						while(menu_exit_request[1])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 1))
					{
						if (!Sapp1_query_init_flag())
						{
							menu_init_request[1] = true;
							while(menu_init_request[1])
								Util_sleep(20000);
						}
						else
							Sapp1_resume();
					}
#endif //DEF_SAPP1_ENABLE

#ifdef DEF_SAPP2_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 2) && Sapp2_query_init_flag())
					{
						menu_exit_request[2] = true;
						while(menu_exit_request[2])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 2))
					{
						if (!Sapp2_query_init_flag())
						{
							menu_init_request[2] = true;
							while(menu_init_request[2])
								Util_sleep(20000);
						}
						else
							Sapp2_resume();
					}
#endif //DEF_SAPP2_ENABLE

#ifdef DEF_SAPP3_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 3) && Sapp3_query_init_flag())
					{
						menu_exit_request[3] = true;
						while(menu_exit_request[3])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 3))
					{
						if (!Sapp3_query_init_flag())
						{
							menu_init_request[3] = true;
							while(menu_init_request[3])
								Util_sleep(20000);
						}
						else
							Sapp3_resume();
					}
#endif //DEF_SAPP3_ENABLE

#ifdef DEF_SAPP4_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 4) && Sapp4_query_init_flag())
					{
						menu_exit_request[4] = true;
						while(menu_exit_request[4])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 4))
					{
						if (!Sapp4_query_init_flag())
						{
							menu_init_request[4] = true;
							while(menu_init_request[4])
								Util_sleep(20000);
						}
						else
							Sapp4_resume();
					}
#endif //DEF_SAPP4_ENABLE

#ifdef DEF_SAPP5_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 5) && Sapp5_query_init_flag())
					{
						menu_exit_request[5] = true;
						while(menu_exit_request[5])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 5))
					{
						if (!Sapp5_query_init_flag())
						{
							menu_init_request[5] = true;
							while(menu_init_request[5])
								Util_sleep(20000);
						}
						else
							Sapp5_resume();
					}
#endif //DEF_SAPP5_ENABLE

#ifdef DEF_SAPP6_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 6) && Sapp6_query_init_flag())
					{
						menu_exit_request[6] = true;
						while(menu_exit_request[6])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 6))
					{
						if (!Sapp6_query_init_flag())
						{
							menu_init_request[6] = true;
							while(menu_init_request[6])
								Util_sleep(20000);
						}
						else
							Sapp6_resume();
					}
#endif //DEF_SAPP6_ENABLE

#ifdef DEF_SAPP7_ENABLE
					else if (DEF_MENU_HID_SAPP_CLOSE_CFM(key, 7) && Sapp7_query_init_flag())
					{
						menu_exit_request[7] = true;
						while(menu_exit_request[7])
							Util_sleep(20000);
					}
					else if (DEF_MENU_HID_SAPP_OPEN_CFM(key, 7))
					{
						if (!Sapp7_query_init_flag())
						{
							menu_init_request[7] = true;
							while(menu_init_request[7])
								Util_sleep(20000);
						}
						else
							Sapp7_resume();
					}
#endif //DEF_SAPP7_ENABLE
				}

				//Notify user that button is NOT being pressed anymore.
				if(DEF_MENU_HID_SYSTEM_UI_DESEL(key))
					Draw_get_bot_ui_button()->selected = false;
				if(DEF_MENU_HID_SEM_DESEL(key))
					menu_sem_button.selected = false;
				if(DEF_MENU_HID_SAPP_CLOSE_DESEL(key))
				{
					for(uint8_t i = 0; i < DEF_MENU_NUM_OF_SUB_APP; i++)
						menu_sapp_close_button[i].selected = false;
				}
				if(DEF_MENU_HID_SAPP_OPEN_DESEL(key))
				{
					for(uint8_t i = 0; i < DEF_MENU_NUM_OF_SUB_APP; i++)
						menu_sapp_button[i].selected = false;
				}
			}

			if(Util_log_query_show_flag())
				Util_log_main(key);
		}
	}

#ifdef DEF_VID_ENABLE
	else if (Vid_query_running_flag())
		Vid_hid(key);
#endif //DEF_VID_ENABLE

#ifdef DEF_SAPP1_ENABLE
	else if (Sapp1_query_running_flag())
		Sapp1_hid(key);
#endif //DEF_SAPP1_ENABLE

#ifdef DEF_SAPP2_ENABLE
	else if (Sapp2_query_running_flag())
		Sapp2_hid(key);
#endif //DEF_SAPP2_ENABLE

#ifdef DEF_SAPP3_ENABLE
	else if (Sapp3_query_running_flag())
		Sapp3_hid(key);
#endif //DEF_SAPP3_ENABLE

#ifdef DEF_SAPP4_ENABLE
	else if (Sapp4_query_running_flag())
		Sapp4_hid(key);
#endif //DEF_SAPP4_ENABLE

#ifdef DEF_SAPP5_ENABLE
	else if (Sapp5_query_running_flag())
		Sapp5_hid(key);
#endif //DEF_SAPP5_ENABLE

#ifdef DEF_SAPP6_ENABLE
	else if (Sapp6_query_running_flag())
		Sapp6_hid(key);
#endif //DEF_SAPP6_ENABLE

#ifdef DEF_SAPP7_ENABLE
	else if (Sapp7_query_running_flag())
		Sapp7_hid(key);
#endif //DEF_SAPP7_ENABLE

	else if (Sem_query_running_flag())
		Sem_hid(key);
}

void Menu_worker_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");

	while (menu_thread_run)
	{
		Util_sync_lock(&menu_callback_mutex, UINT64_MAX);

		//Call callback functions.
		for(uint16_t i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
		{
			if(menu_worker_thread_callbacks[i])
				menu_worker_thread_callbacks[i]();
		}

		Util_sync_unlock(&menu_callback_mutex);

		gspWaitForVBlank();
	}

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

#if (DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)
void Menu_send_app_info_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint8_t model = 0;
	uint32_t result = DEF_ERR_OTHER;
	char system_ver_char[0x20] = { 0, };
	char user_agent[128] = { 0, };
	const char* sem_model_name[DEF_SEM_MODEL_MAX] = { "O3DS", "O3DSXL", "O2DS", "N3DS", "N3DSXL", "N2DSXL", };
	const char* screen_mode_name[DEF_SEM_SCREEN_MODE_MAX] = { "AUTO", "400PX", "800PX", "3D", };
	Net_post_dl_parameters post_parameters = { 0, };
	Str_data send_data = { 0, };
	Sem_config config = { 0, };
	Sem_state state = { 0, };

#if DEF_CURL_API_ENABLE
	snprintf(user_agent, sizeof(user_agent), "%s %s", Util_curl_get_default_user_agent(), DEF_MENU_APP_INFO);
#else
	snprintf(user_agent, sizeof(user_agent), "%s %s", Util_httpc_get_default_user_agent(), DEF_MENU_APP_INFO);
#endif //DEF_CURL_API_ENABLE

	Util_str_init(&send_data);

	//Gather information.
	Sem_get_config(&config);
	Sem_get_state(&state);
	osGetSystemVersionDataString(NULL, NULL, system_ver_char, sizeof(system_ver_char));
	//We need real model here (in case fake model is enabled).
	if(CFGU_GetSystemModel(&model) == DEF_SUCCESS)
	{
		if(model == CFG_MODEL_3DS)
			state.console_model = DEF_SEM_MODEL_OLD3DS;
		else if(model == CFG_MODEL_3DSXL)
			state.console_model = DEF_SEM_MODEL_OLD3DSXL;
		else if(model == CFG_MODEL_2DS)
			state.console_model = DEF_SEM_MODEL_OLD2DS;
		else if(model == CFG_MODEL_N3DS)
			state.console_model = DEF_SEM_MODEL_NEW3DS;
		else if(model == CFG_MODEL_N3DSXL)
			state.console_model = DEF_SEM_MODEL_NEW3DSXL;
		else if(model == CFG_MODEL_N2DSXL)
			state.console_model = DEF_SEM_MODEL_NEW2DSXL;
	}

	//Make a json data, then send it.
	Util_str_format(&send_data, DEF_JSON_START_OBJECT);
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("fmt_ver", "%" PRIu32, DEF_MENU_SEND_INFO_FMT_VER));
	Util_str_format_append(&send_data, DEF_JSON_STR_DATA_WITH_KEY("app_ver", "v%s", DEF_MENU_CURRENT_APP_VER));
	Util_str_format_append(&send_data, DEF_JSON_STR_DATA_WITH_KEY("system_ver", "%s", system_ver_char));
	Util_str_format_append(&send_data, DEF_JSON_STR_DATA_WITH_KEY("model", "%s", sem_model_name[state.console_model]));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("num_of_launch", "%" PRIu32, state.num_of_launch));
	Util_str_format_append(&send_data, DEF_JSON_STR_DATA_WITH_KEY("lang", "%s", config.lang));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("turn_off_lcd", "%" PRIu16, config.time_to_turn_off_lcd));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("enter_sleep", "%" PRIu16, config.time_to_enter_sleep));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("top_brightness", "%" PRIu8, config.top_lcd_brightness));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("bottom_brightness", "%" PRIu8, config.bottom_lcd_brightness));
	Util_str_format_append(&send_data, DEF_JSON_STR_DATA_WITH_KEY("screen_mode", "%s", screen_mode_name[config.screen_mode]));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("scroll_speed", "%f", config.scroll_speed));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("is_eco", "%s", (config.is_eco ? "true" : "false")));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY("is_night", "%s", (config.is_night ? "true" : "false")));
	Util_str_format_append(&send_data, DEF_JSON_NON_STR_DATA_WITH_KEY_WITHOUT_COMMA("is_wifi_on", "%s", (config.is_wifi_on ? "true" : "false")));
	Util_str_format_append(&send_data, DEF_JSON_END_OBJECT);

	post_parameters.dl.url = DEF_MENU_SEND_APP_INFO_URL;
	post_parameters.dl.max_redirect = 5;
	post_parameters.dl.max_size = 0x10000;
	post_parameters.u.data.data = (uint8_t*)send_data.buffer;
	post_parameters.u.data.size = send_data.length;
	post_parameters.dl.user_agent = user_agent;

#if DEF_CURL_API_ENABLE
	DEF_LOG_RESULT_SMART(result, Util_curl_post_and_dl_data(&post_parameters), (result == DEF_SUCCESS), result);
#else
	DEF_LOG_RESULT_SMART(result, Util_httpc_post_and_dl_data(&post_parameters), (result == DEF_SUCCESS), result);
#endif //DEF_CURL_API_ENABLE

	Util_str_free(&send_data);
	free(post_parameters.dl.data);
	post_parameters.dl.data = NULL;

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

void Menu_update_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;
	Net_dl_parameters parameters = { 0, };

	parameters.url = DEF_SEM_CHECK_UPDATE_URL;
	parameters.max_redirect = 3;
	parameters.max_size = 0x1000;

#if DEF_CURL_API_ENABLE
	DEF_LOG_RESULT_SMART(result, Util_curl_dl_data(&parameters), (result == DEF_SUCCESS), result);
#else
	DEF_LOG_RESULT_SMART(result, Util_httpc_dl_data(&parameters), (result == DEF_SUCCESS), result);
#endif //DEF_CURL_API_ENABLE

	if(result == DEF_SUCCESS)
	{
		char* pos[2] = { 0, };

		pos[0] = strstr((char*)parameters.data, "<newest>");
		pos[1] = strstr((char*)parameters.data, "</newest>");
		if(pos[0] && pos[1])
		{
			uint32_t size = 0;
			char ver[32] = { 0, };

			pos[0] += strlen("<newest>");
			size = (pos[1] - pos[0]);
			if((pos[1] > pos[0]) && (size < sizeof(ver)))
			{
				memcpy(ver, pos[0], size);
				ver[size] = 0x00;

				if(DEF_MENU_CURRENT_APP_VER_INT < (uint32_t)strtoul(ver, NULL, 10))
					menu_update_available = true;
			}
		}
	}

	free(parameters.data);
	parameters.data = NULL;

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}
#endif //(DEF_CURL_API_ENABLE || DEF_HTTPC_API_ENABLE)
