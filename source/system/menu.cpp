#include "definitions.hpp"
#include "system/types.hpp"

#include "system/setting_menu.hpp"
#include "system/variables.hpp"

#include "system/draw/draw.hpp"
#include "system/draw/external_font.hpp"

#include "system/util/change_setting.hpp"
#include "system/util/cpu_usage.hpp"
#include "system/util/curl.hpp"
#include "system/util/error.hpp"
#include "system/util/explorer.hpp"
#include "system/util/file.hpp"
#include "system/util/hid.hpp"
#include "system/util/httpc.hpp"
#include "system/util/log.hpp"
#include "system/util/util.hpp"

#ifdef DEF_ENABLE_VID
#include "video_player.hpp"
#endif

#ifdef DEF_ENABLE_SUB_APP1
#include "sub_app1.hpp"
#endif

#ifdef DEF_ENABLE_SUB_APP2
#include "sub_app2.hpp"
#endif

#ifdef DEF_ENABLE_SUB_APP3
#include "sub_app3.hpp"
#endif

#ifdef DEF_ENABLE_SUB_APP4
#include "sub_app4.hpp"
#endif

#ifdef DEF_ENABLE_SUB_APP5
#include "sub_app5.hpp"
#endif

#ifdef DEF_ENABLE_SUB_APP6
#include "sub_app6.hpp"
#endif

#ifdef DEF_ENABLE_SUB_APP7
#include "sub_app7.hpp"
#endif

//Include myself.
#include "system/menu.hpp"

bool menu_thread_run = false;
bool menu_main_run = true;
bool menu_must_exit = false;
bool menu_check_exit_request = false;
bool menu_update_available = false;
bool menu_init_request[9] = { false, false, false, false, false, false, false, false, false, };
bool menu_exit_request[8] = { false, false, false, false, false, false, false, false, };
int menu_icon_texture_num[9] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, };
void (*menu_worker_thread_callbacks[DEF_MENU_NUM_OF_CALLBACKS])(void) = { NULL, };
std::string menu_msg[DEF_MENU_NUM_OF_MSG];
Thread menu_worker_thread;
LightLock menu_callback_mutex = 1;//Initially unlocked state.
C2D_Image menu_icon_image[10];
Image_data menu_sapp_button[8], menu_sapp_close_button[8], menu_sem_button;

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)

Thread menu_check_connectivity_thread, menu_send_app_info_thread, menu_update_thread;

#endif

void Menu_get_system_info(void);
void Menu_hid_callback(void);
void Menu_worker_thread(void* arg);

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)

void Menu_send_app_info_thread(void* arg);
void Menu_check_connectivity_thread(void* arg);
void Menu_update_thread(void* arg);

#endif

Result_with_string Menu_update_main_directory(void)
{
	std::string old_main_dir = "/Video_player";
	std::string new_main_dir = (DEF_MAIN_DIR).substr(0, (DEF_MAIN_DIR).length() - 1);
	Handle fs_handle = 0;
	FS_Archive archive = 0;
	Result_with_string result;

	if(FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")) == 0)
	{
		if(FSUSER_OpenDirectory(&fs_handle, archive, fsMakePath(PATH_ASCII, old_main_dir.c_str())) == 0)
		{
			FSDIR_Close(fs_handle);
			result.code = FSUSER_RenameDirectory(archive, fsMakePath(PATH_ASCII, old_main_dir.c_str()), archive, fsMakePath(PATH_ASCII, new_main_dir.c_str()));
		}
	}
	FSUSER_CloseArchive(archive);

	if(result.code != 0)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] Failed to move app data directory.";
		result.error_description = old_main_dir + " -> " + new_main_dir + "\nThis may be because destination directory is already exist.";
	}

	return result;
}

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
	var_need_reflesh = true;
}

void Menu_suspend(void)
{
	menu_main_run = false;
}

Result_with_string Menu_load_msg(std::string lang)
{
	return Util_load_msg("menu_" + lang + ".txt", menu_msg, DEF_MENU_NUM_OF_MSG);
}

void Menu_check_core_thread(void* arg)
{
	threadExit(0);
}

void Menu_init(void)
{
	bool is_800px = false;
	bool is_3d = false;
	u8* data = NULL;
	u8 dummy = 0;
	u8 region = 0;
	u8 model = 0;
	u32 read_size = 0;
	Thread core_2, core_3;
	Result_with_string result, update_main_dir_result;

	var_disabled_result.string = "";
	var_disabled_result.error_description = DEF_ERR_DISABLED_STR;
	var_disabled_result.code = DEF_ERR_DISABLED;

	for(int i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
		menu_worker_thread_callbacks[i] = NULL;

	result = Util_log_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_log_init()...", result.code);
	Util_log_save(DEF_MENU_INIT_STR, "Initializing..." + DEF_CURRENT_APP_VER);

	osSetSpeedupEnable(true);
	aptSetSleepAllowed(true);
	svcSetThreadPriority(CUR_THREAD_HANDLE, DEF_THREAD_PRIORITY_HIGH - 1);

	Util_log_save(DEF_MENU_INIT_STR, "fsInit()...", fsInit());
	Util_log_save(DEF_MENU_INIT_STR, "acInit()...", acInit());
	Util_log_save(DEF_MENU_INIT_STR, "aptInit()...", aptInit());
	Util_log_save(DEF_MENU_INIT_STR, "mcuHwcInit()...", mcuHwcInit());
	Util_log_save(DEF_MENU_INIT_STR, "ptmuInit()...", ptmuInit());
	Util_log_save(DEF_MENU_INIT_STR, "romfsInit()...", romfsInit());
	Util_log_save(DEF_MENU_INIT_STR, "cfguInit()...", cfguInit());
	Util_log_save(DEF_MENU_INIT_STR, "amInit()...", amInit());
	Util_log_save(DEF_MENU_INIT_STR, "APT_SetAppCpuTimeLimit()...", APT_SetAppCpuTimeLimit(30));

	result = Util_safe_linear_alloc_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_safe_linear_alloc_init()...", result.code);

	//Move data directory.
	update_main_dir_result = Menu_update_main_directory();

	//create directory
	Util_file_save_to_file(".", DEF_MAIN_DIR, &dummy, 1, true);
	Util_file_save_to_file(".", DEF_MAIN_DIR + "screen_recording/", &dummy, 1, true);

	if(Util_file_load_from_file("fake_model.txt", DEF_MAIN_DIR, &data, 1, &read_size).code == 0 && *data <= 5)
	{
		var_fake_model = true;
		var_model = *data;
		Util_safe_linear_free(data);
		data = NULL;
	}

	if(CFGU_SecureInfoGetRegion(&region) == 0)
	{
		if(region == CFG_REGION_CHN)
			var_system_region = 1;
		else if(region == CFG_REGION_KOR)
			var_system_region = 2;
		else if(region == CFG_REGION_TWN)
			var_system_region = 3;
		else
			var_system_region = 0;
	}

	if(CFGU_GetSystemModel(&model) == 0)
	{
		Util_log_save(DEF_MENU_INIT_STR, "Model : " + var_model_name[model]);
		if(!var_fake_model)
			var_model = model;
	}
	else
		Util_log_save(DEF_MENU_INIT_STR, "Model : Unknown");

	if(var_fake_model)
		Util_log_save(DEF_MENU_INIT_STR, "Using fake model : " + var_model_name[var_model]);

	if(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DSXL || var_model == CFG_MODEL_3DS)
		osSetSpeedupEnable(false);

	result = Util_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_init()...", result.code);

	Sem_init();

	Sem_suspend();

	if(var_screen_mode == DEF_SEM_SCREEN_AUTO)
	{
		if(osGet3DSliderState())
			is_3d = true;
		else
			is_800px = true;
	}
	else if(var_screen_mode == DEF_SEM_SCREEN_800PX)
		is_800px = true;
	else if(var_screen_mode == DEF_SEM_SCREEN_3D)
		is_3d = true;

	Util_log_save(DEF_MENU_INIT_STR, "Draw_init()...", Draw_init(is_800px, is_3d).code);
	Draw_frame_ready();
	Draw_screen_ready(SCREEN_TOP_LEFT, DEF_DRAW_WHITE);
	Draw_top_ui();
	Draw_screen_ready(SCREEN_BOTTOM, DEF_DRAW_WHITE);
	Draw_bot_ui();
	Draw_apply_draw();
	Sem_draw_init();

	result = Util_httpc_init(DEF_HTTP_POST_BUFFER_SIZE);
	Util_log_save(DEF_MENU_INIT_STR, "Util_httpc_init()...", result.code);

	result = Util_curl_init(DEF_SOCKET_BUFFER_SIZE);
	Util_log_save(DEF_MENU_INIT_STR, "Util_curl_init()...", result.code);

	result = Util_hid_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_hid_init()...", result.code);

	result.code = Util_hid_add_callback(Menu_hid_callback);
	Util_log_save(DEF_MENU_INIT_STR, "Util_hid_add_callback()...", result.code);

	result = Util_expl_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_expl_init()...", result.code);

	result = Exfont_init();
	Util_log_save(DEF_MENU_INIT_STR, "Exfont_init()...", result.code);

	result = Util_err_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_err_init()...", result.code);

	if(update_main_dir_result.code != 0)
	{
		//We need to call error api after calling Util_err_init().
		Util_err_set_error_message(update_main_dir_result.string, update_main_dir_result.error_description, DEF_MENU_INIT_STR, update_main_dir_result.code);
		Util_err_set_error_show_flag(true);
	}

	for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
		Exfont_set_external_font_request_state(i, true);

	Exfont_request_load_external_font();

	menu_thread_run = true;
	menu_worker_thread = threadCreate(Menu_worker_thread, (void*)(""), DEF_STACKSIZE * 2, DEF_THREAD_PRIORITY_ABOVE_NORMAL, 0, false);

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)
	menu_check_connectivity_thread = threadCreate(Menu_check_connectivity_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	menu_update_thread = threadCreate(Menu_update_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 1, true);
#endif

	if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_N3DS)
	{
		core_2 = threadCreate(Menu_check_core_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
		if(!core_2)
			var_core_2_available = false;
		else
		{
			threadJoin(core_2, U64_MAX);
			var_core_2_available = true;
		}

		core_3 = threadCreate(Menu_check_core_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 3, false);
		if(!core_3)
			var_core_3_available = false;
		else
		{
			threadJoin(core_3, U64_MAX);
			var_core_3_available = true;
		}

		threadFree(core_2);
		threadFree(core_3);
	}

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)
	if (var_allow_send_app_info)
		menu_send_app_info_thread = threadCreate(Menu_send_app_info_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_LOW, 1, true);
#endif

	#ifdef DEF_VID_ENABLE_ICON
	menu_icon_texture_num[0] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_VID_ICON_PATH, menu_icon_texture_num[0], menu_icon_image, 0, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	if(result.code == 0)
	{
		Image_data texture_data;
		texture_data.c2d = menu_icon_image[0];
		Draw_set_texture_filter(&texture_data, true);
	}
	#endif

	#ifdef DEF_SAPP1_ENABLE_ICON
	menu_icon_texture_num[1] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SAPP1_ICON_PATH, menu_icon_texture_num[1], menu_icon_image, 1, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	#ifdef DEF_SAPP2_ENABLE_ICON
	menu_icon_texture_num[2] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SAPP2_ICON_PATH, menu_icon_texture_num[2], menu_icon_image, 2, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	#ifdef DEF_SAPP3_ENABLE_ICON
	menu_icon_texture_num[3] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SAPP3_ICON_PATH, menu_icon_texture_num[3], menu_icon_image, 3, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	#ifdef DEF_SAPP4_ENABLE_ICON
	menu_icon_texture_num[4] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SAPP4_ICON_PATH, menu_icon_texture_num[4], menu_icon_image, 4, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	#ifdef DEF_SAPP5_ENABLE_ICON
	menu_icon_texture_num[5] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SAPP5_ICON_PATH, menu_icon_texture_num[5], menu_icon_image, 5, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	#ifdef DEF_SAPP6_ENABLE_ICON
	menu_icon_texture_num[6] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SAPP6_ICON_PATH, menu_icon_texture_num[6], menu_icon_image, 6, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	#ifdef DEF_SAPP7_ENABLE_ICON
	menu_icon_texture_num[7] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SAPP7_ICON_PATH, menu_icon_texture_num[7], menu_icon_image, 7, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	#ifdef DEF_SEM_ENABLE_ICON
	menu_icon_texture_num[8] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_SEM_ICON_PATH, menu_icon_texture_num[8], menu_icon_image, 8, 2);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
	#endif

	for(int i = 0; i < 8; i++)
	{
		menu_sapp_button[i].c2d = var_square_image[0];
		menu_sapp_close_button[i].c2d = var_square_image[0];
	}
	menu_sem_button.c2d = var_square_image[0];

	Util_add_watch(&menu_must_exit);
	Util_add_watch(&menu_check_exit_request);
	Util_add_watch(&menu_init_request[8]);

	Util_add_watch(&menu_sem_button.selected);
	for(int i = 0; i < 8; i++)
	{
		Util_add_watch(&menu_init_request[i]);
		Util_add_watch(&menu_exit_request[i]);

		Util_add_watch(&menu_sapp_button[i].selected);
		Util_add_watch(&menu_sapp_close_button[i].selected);
	}

	Menu_get_system_info();

	Menu_resume();
	Util_log_save(DEF_MENU_INIT_STR, "Initialized");
}

void Menu_exit(void)
{
	Util_log_save(DEF_MENU_EXIT_STR, "Exiting...");
	bool draw = !aptShouldClose();
	Result_with_string result;

	menu_thread_run = false;

	#ifdef DEF_ENABLE_VID
	if (Vid_query_init_flag())
		Vid_exit(draw);
	#endif
	#ifdef DEF_ENABLE_SUB_APP1
	if (Sapp1_query_init_flag())
		Sapp1_exit(draw);
	#endif
	#ifdef DEF_ENABLE_SUB_APP2
	if (Sapp2_query_init_flag())
		Sapp2_exit(draw);
	#endif
	#ifdef DEF_ENABLE_SUB_APP3
	if (Sapp3_query_init_flag())
		Sapp3_exit(draw);
	#endif
	#ifdef DEF_ENABLE_SUB_APP4
	if (Sapp4_query_init_flag())
		Sapp4_exit(draw);
	#endif
	#ifdef DEF_ENABLE_SUB_APP5
	if (Sapp5_query_init_flag())
		Sapp5_exit(draw);
	#endif
	#ifdef DEF_ENABLE_SUB_APP6
	if (Sapp6_query_init_flag())
		Sapp6_exit(draw);
	#endif
	#ifdef DEF_ENABLE_SUB_APP7
	if (Sapp7_query_init_flag())
		Sapp7_exit(draw);
	#endif
	if (Sem_query_init_flag())
		Sem_exit();

	for(int i = 0; i < 8; i++)
		Draw_free_texture(menu_icon_texture_num[i]);

	Util_hid_remove_callback(Menu_hid_callback);
	Util_hid_exit();
	Util_expl_exit();
	Exfont_exit();
	Util_err_exit();
	Util_exit();
	Util_cpu_usage_monitor_exit();

	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_worker_thread, DEF_THREAD_WAIT_TIME));

	threadFree(menu_worker_thread);

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)
	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_check_connectivity_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_send_app_info_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_update_thread, DEF_THREAD_WAIT_TIME));

	threadFree(menu_check_connectivity_thread);
#endif

	Util_remove_watch(&menu_must_exit);
	Util_remove_watch(&menu_check_exit_request);
	Util_remove_watch(&menu_init_request[8]);

	Util_remove_watch(&menu_sem_button.selected);
	for(int i = 0; i < 8; i++)
	{
		Util_remove_watch(&menu_init_request[i]);
		Util_remove_watch(&menu_exit_request[i]);

		Util_remove_watch(&menu_sapp_button[i].selected);
		Util_remove_watch(&menu_sapp_close_button[i].selected);
	}

	Util_log_exit();
	Util_httpc_exit();
	Util_curl_exit();

	Util_safe_linear_alloc_exit();
	fsExit();
	acExit();
	aptExit();
	mcuHwcExit();
	ptmuExit();
	romfsExit();
	cfguExit();
	amExit();
	Draw_exit();

	Util_log_save(DEF_MENU_EXIT_STR, "Exited.");
}

bool Menu_add_worker_thread_callback(void (*callback)(void))
{
	LightLock_Lock(&menu_callback_mutex);

	for(int i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
	{
		if(menu_worker_thread_callbacks[i] == callback)
			goto success;//Already exist.
	}

	for(int i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
	{
		if(!menu_worker_thread_callbacks[i])
		{
			menu_worker_thread_callbacks[i] = callback;
			goto success;
		}
	}

	//No free spaces left.
	LightLock_Unlock(&menu_callback_mutex);
	return false;

	success:
	LightLock_Unlock(&menu_callback_mutex);
	return true;
}

void Menu_remove_worker_thread_callback(void (*callback)(void))
{
	LightLock_Lock(&menu_callback_mutex);

	for(int i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
	{
		if(menu_worker_thread_callbacks[i] == callback)
		{
			menu_worker_thread_callbacks[i] = NULL;
			break;
		}
	}

	LightLock_Unlock(&menu_callback_mutex);
}

void Menu_main(void)
{
	bool is_800px = false;
	bool is_3d = false;
	u8 screen_mode = var_screen_mode;
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	is_800px = (screen_mode == DEF_SEM_SCREEN_800PX);
	is_3d = (screen_mode == DEF_SEM_SCREEN_3D);
	if(screen_mode == DEF_SEM_SCREEN_AUTO)
	{
		if(osGet3DSliderState())
		{
			is_3d = true;
			is_800px = false;
		}
		else
		{
			is_3d = false;
			is_800px = true;
		}
	}

	if(var_model == CFG_MODEL_2DS && is_800px)
	{
		is_800px = false;
		var_screen_mode = DEF_SEM_SCREEN_AUTO;
	}
	if((var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_N2DSXL) && is_3d)
	{
		is_3d = false;
		var_screen_mode = DEF_SEM_SCREEN_AUTO;
	}

	//Update screen mode here.
	if(is_3d != Draw_is_3d_mode() || is_800px != Draw_is_800px_mode())
	{
		Result_with_string result;
		int log = 0;

		log = Util_log_save(DEF_MENU_MAIN_STR, "Draw_reinit()...");
		result = Draw_reinit(is_800px, is_3d);
		Util_log_add(log, result.string + result.error_description, result.code);
		var_need_reflesh = true;
	}

	if(var_debug_mode)
		var_need_reflesh = true;

	if (menu_main_run)
	{
		if(Util_is_watch_changed() || var_need_reflesh || !var_eco_mode)
		{
			var_need_reflesh = false;
			if (var_night_mode)
			{
				color = DEF_DRAW_WHITE;
				back_color = DEF_DRAW_BLACK;
			}

			Draw_frame_ready();
			Draw_screen_ready(SCREEN_TOP_LEFT, back_color);

			if(menu_check_exit_request)
			{
				Draw(menu_msg[DEF_MENU_EXIST_MSG], 0, 105, 0.5, 0.5, color, X_ALIGN_CENTER,
					Y_ALIGN_CENTER, 400, 20);
				Draw(menu_msg[DEF_MENU_CONFIRM_MSG], 10, 140, 0.5, 0.5, DEF_DRAW_GREEN, X_ALIGN_RIGHT,
					Y_ALIGN_CENTER, 190, 20);
				Draw(menu_msg[DEF_MENU_CANCEL_MSG], 210, 140, 0.5, 0.5, DEF_DRAW_RED, X_ALIGN_LEFT,
					Y_ALIGN_CENTER, 190, 20);
			}
			else if(menu_update_available)
			{
				Draw(menu_msg[DEF_MENU_NEW_VERSION_MSG], 10, 30, 0.7, 0.7, DEF_DRAW_RED);
				Draw(menu_msg[DEF_MENU_HOW_TO_UPDATE_MSG], 10, 60, 0.5, 0.5, color);
			}

			if(Util_log_query_log_show_flag())
				Util_log_draw();

			Draw_top_ui();

			if(var_monitor_cpu_usage)
				Draw_cpu_usage_info();

			Draw_screen_ready(SCREEN_BOTTOM, back_color);

			#ifdef DEF_ENABLE_VID
			Draw_texture(&menu_sapp_button[0], menu_sapp_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 60, 100, 100);

			#ifdef DEF_VID_ENABLE_ICON
			Draw_texture(menu_icon_image[0], 110, 60, 100, 100);
			#endif
			#ifdef DEF_VID_ENABLE_NAME
			Draw(DEF_VID_NAME, 110, 60, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 100, 100);
			#endif

			if(Vid_query_init_flag())
			{
				Draw("X", 195, 60, 0.5, 0.5, DEF_DRAW_RED, X_ALIGN_CENTER, Y_ALIGN_CENTER, 15, 15,
				BACKGROUND_ENTIRE_BOX, &menu_sapp_close_button[0], menu_sapp_close_button[0].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP1
			Draw_texture(&menu_sapp_button[1], menu_sapp_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 80, 0, 60, 60);

			#ifdef DEF_SAPP1_ENABLE_ICON
			Draw_texture(menu_icon_image[1], 80, 0, 60, 60);
			#endif
			#ifdef DEF_SAPP1_ENABLE_NAME
			Draw(DEF_SAPP1_NAME, 80, 0, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Sapp1_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[1], menu_sapp_close_button[1].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 125, 0, 15, 15.0);
				Draw("X", 127.5, 0, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP2
			Draw_texture(&menu_sapp_button[2], menu_sapp_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 160, 0, 60, 60);

			#ifdef DEF_SAPP2_ENABLE_ICON
			Draw_texture(menu_icon_image[2], 160, 0, 60, 60);
			#endif
			#ifdef DEF_SAPP2_ENABLE_NAME
			Draw(DEF_SAPP2_NAME, 160, 0, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Sapp2_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[2], menu_sapp_close_button[2].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 205, 0, 15, 15.0);
				Draw("X", 207.5, 0, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP3
			Draw_texture(&menu_sapp_button[3], menu_sapp_button[3].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 240, 0, 60, 60);

			#ifdef DEF_SAPP3_ENABLE_ICON
			Draw_texture(menu_icon_image[3], 240, 0, 60, 60);
			#endif
			#ifdef DEF_SAPP3_ENABLE_NAME
			Draw(DEF_SAPP3_NAME, 240, 0, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Sapp3_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[3], menu_sapp_close_button[3].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 285, 0, 15, 15.0);
				Draw("X", 287.5, 0, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP4
			Draw_texture(&menu_sapp_button[4], menu_sapp_button[4].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 80, 60, 60);

			#ifdef DEF_SAPP4_ENABLE_ICON
			Draw_texture(menu_icon_image[4], 0, 80, 60, 60);
			#endif
			#ifdef DEF_SAPP4_ENABLE_NAME
			Draw(DEF_SAPP4_NAME, 0, 80, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Sapp4_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[4], menu_sapp_close_button[4].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 45, 80, 15, 15.0);
				Draw("X", 47.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP5
			Draw_texture(&menu_sapp_button[5], menu_sapp_button[5].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 80, 80, 60, 60);

			#ifdef DEF_SAPP5_ENABLE_ICON
			Draw_texture(menu_icon_image[5], 80, 80, 60, 60);
			#endif
			#ifdef DEF_SAPP5_ENABLE_NAME
			Draw(DEF_SAPP5_NAME, 80, 80, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Sapp5_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[5], menu_sapp_close_button[5].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 125, 80, 15, 15.0);
				Draw("X", 127.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP6
			Draw_texture(&menu_sapp_button[6], menu_sapp_button[6].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 160, 80, 60, 60);

			#ifdef DEF_SAPP6_ENABLE_ICON
			Draw_texture(menu_icon_image[6], 160, 80, 60, 60);
			#endif
			#ifdef DEF_SAPP6_ENABLE_NAME
			Draw(DEF_SAPP6_NAME, 160, 80, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Sapp6_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[6], menu_sapp_close_button[6].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 205, 80, 15, 15.0);
				Draw("X", 207.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP7
			Draw_texture(&menu_sapp_button[7], menu_sapp_button[7].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 240, 80, 60, 60);

			#ifdef DEF_SAPP7_ENABLE_ICON
			Draw_texture(menu_icon_image[7], 240, 80, 60, 60);
			#endif
			#ifdef DEF_SAPP7_ENABLE_NAME
			Draw(DEF_SAPP7_NAME, 240, 80, 0.4, 0.4, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Sapp7_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[7], menu_sapp_close_button[7].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 285, 80, 15, 15.0);
				Draw("X", 287.5, 80, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif

			Draw_texture(&menu_sem_button, menu_sem_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 260, 170, 60, 60);

			#ifdef DEF_SEM_ENABLE_ICON
			Draw_texture(menu_icon_image[8 + var_night_mode], 260, 170, 60, 60);
			#endif
			#ifdef DEF_SEM_ENABLE_NAME
			Draw(DEF_SEM_NAME, 270, 205, 0.4, 0.4, color);
			#endif

			if(Util_err_query_error_show_flag())
				Util_err_draw();

			Draw_bot_ui();

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();

		#ifdef DEF_ENABLE_VID
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
		#endif
		#ifdef DEF_ENABLE_SUB_APP1
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
		#endif
		#ifdef DEF_ENABLE_SUB_APP2
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
		#endif
		#ifdef DEF_ENABLE_SUB_APP3
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
		#endif
		#ifdef DEF_ENABLE_SUB_APP4
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
		#endif
		#ifdef DEF_ENABLE_SUB_APP5
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
		#endif
		#ifdef DEF_ENABLE_SUB_APP6
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
		#endif
		#ifdef DEF_ENABLE_SUB_APP7
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
		#endif
		if(menu_init_request[8])
		{
			Sem_init();
			menu_init_request[8] = false;
		}
	}
	#ifdef DEF_ENABLE_VID
	else if (Vid_query_running_flag())
		Vid_main();
	#endif
	#ifdef DEF_ENABLE_SUB_APP1
	else if (Sapp1_query_running_flag())
		Sapp1_main();
	#endif
	#ifdef DEF_ENABLE_SUB_APP2
	else if (Sapp2_query_running_flag())
		Sapp2_main();
	#endif
	#ifdef DEF_ENABLE_SUB_APP3
	else if (Sapp3_query_running_flag())
		Sapp3_main();
	#endif
	#ifdef DEF_ENABLE_SUB_APP4
	else if (Sapp4_query_running_flag())
		Sapp4_main();
	#endif
	#ifdef DEF_ENABLE_SUB_APP5
	else if (Sapp5_query_running_flag())
		Sapp5_main();
	#endif
	#ifdef DEF_ENABLE_SUB_APP6
	else if (Sapp6_query_running_flag())
		Sapp6_main();
	#endif
	#ifdef DEF_ENABLE_SUB_APP7
	else if (Sapp7_query_running_flag())
		Sapp7_main();
	#endif
	else if (Sem_query_running_flag())
		Sem_main();
	else
		menu_main_run = true;
}

void Menu_hid_callback(void)
{
	Hid_info key;

	Util_hid_query_key_state(&key);
	if(menu_main_run)
	{
		if(!aptShouldJumpToHome())
		{
			if (Util_err_query_error_show_flag())
				Util_err_main(key);
			else
			{
				if(menu_check_exit_request)
				{
					if (key.p_a)
						menu_must_exit = true;
					else if (key.p_b)
						menu_check_exit_request = false;
				}
				else
				{
					if(Util_hid_is_pressed(key, *Draw_get_bot_ui_button()))
						Draw_get_bot_ui_button()->selected = true;
					else if (key.p_start || (Util_hid_is_released(key, *Draw_get_bot_ui_button()) && Draw_get_bot_ui_button()->selected))
						menu_check_exit_request = true;
					else if (key.p_select)
						Util_log_set_log_show_flag(!Util_log_query_log_show_flag());
					#ifdef DEF_ENABLE_VID
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[0]) && Vid_query_init_flag())
						menu_sapp_close_button[0].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[0]) && Vid_query_init_flag() && menu_sapp_close_button[0].selected)
					{
						menu_exit_request[0] = true;
						while(menu_exit_request[0])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[0]))
						menu_sapp_button[0].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[0]) && menu_sapp_button[0].selected)
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
					#endif
					#ifdef DEF_ENABLE_SUB_APP1
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[1]) && Sapp1_query_init_flag())
						menu_sapp_close_button[1].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[1]) && Sapp1_query_init_flag() && menu_sapp_close_button[1].selected)
					{
						menu_exit_request[1] = true;
						while(menu_exit_request[1])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[1]))
						menu_sapp_button[1].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[1]) && menu_sapp_button[1].selected)
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
					#endif
					#ifdef DEF_ENABLE_SUB_APP2
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[2]) && Sapp2_query_init_flag())
						menu_sapp_close_button[2].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[2]) && Sapp2_query_init_flag() && menu_sapp_close_button[2].selected)
					{
						menu_exit_request[2] = true;
						while(menu_exit_request[2])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[2]))
						menu_sapp_button[2].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[2]) && menu_sapp_button[2].selected)
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
					#endif
					#ifdef DEF_ENABLE_SUB_APP3
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[3]) && Sapp3_query_init_flag())
						menu_sapp_close_button[3].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[3]) && Sapp3_query_init_flag() && menu_sapp_close_button[3].selected)
					{
						menu_exit_request[3] = true;
						while(menu_exit_request[3])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[3]))
						menu_sapp_button[3].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[3]) && menu_sapp_button[3].selected)
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
					#endif
					#ifdef DEF_ENABLE_SUB_APP4
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[4]) && Sapp4_query_init_flag())
						menu_sapp_close_button[4].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[4]) && Sapp4_query_init_flag() && menu_sapp_close_button[4].selected)
					{
						menu_exit_request[4] = true;
						while(menu_exit_request[4])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[4]))
						menu_sapp_button[4].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[4]) && menu_sapp_button[4].selected)
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
					#endif
					#ifdef DEF_ENABLE_SUB_APP5
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[5]) && Sapp5_query_init_flag())
						menu_sapp_close_button[5].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[5]) && Sapp5_query_init_flag() && menu_sapp_close_button[5].selected)
					{
						menu_exit_request[5] = true;
						while(menu_exit_request[5])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[5]))
						menu_sapp_button[5].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[5]) && menu_sapp_button[5].selected)
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
					#endif
					#ifdef DEF_ENABLE_SUB_APP6
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[6]) && Sapp6_query_init_flag())
						menu_sapp_close_button[6].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[6]) && Sapp6_query_init_flag() && menu_sapp_close_button[6].selected)
					{
						menu_exit_request[6] = true;
						while(menu_exit_request[6])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[6]))
						menu_sapp_button[6].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[6]) && menu_sapp_button[6].selected)
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
					#endif
					#ifdef DEF_ENABLE_SUB_APP7
					else if (Util_hid_is_pressed(key, menu_sapp_close_button[7]) && Sapp7_query_init_flag())
						menu_sapp_close_button[7].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_close_button[7]) && Sapp7_query_init_flag() && menu_sapp_close_button[7].selected)
					{
						menu_exit_request[7] = true;
						while(menu_exit_request[7])
							Util_sleep(20000);
					}
					else if (Util_hid_is_pressed(key, menu_sapp_button[7]))
						menu_sapp_button[7].selected = true;
					else if (Util_hid_is_released(key, menu_sapp_button[7]) && menu_sapp_button[7].selected)
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
					#endif
					else if (Util_hid_is_pressed(key, menu_sem_button))
						menu_sem_button.selected = true;
					else if (Util_hid_is_released(key, menu_sem_button) && menu_sem_button.selected)
					{
						if (!Sem_query_init_flag())
						{
							menu_init_request[8] = true;
							while(menu_init_request[8])
								Util_sleep(20000);
						}
						else
							Sem_resume();
					}
				}

				if(!key.p_touch && !key.h_touch)
				{
					for(int i = 0; i < 8; i++)
					{
						menu_sapp_button[i].selected = false;
						menu_sapp_close_button[i].selected = false;
					}
					menu_sem_button.selected = false;
					Draw_get_bot_ui_button()->selected = false;
				}
			}

			if(Util_log_query_log_show_flag())
				Util_log_main(key);
		}
	}
	#ifdef DEF_ENABLE_VID
	else if (Vid_query_running_flag())
		Vid_hid(key);
	#endif
	#ifdef DEF_ENABLE_SUB_APP1
	else if (Sapp1_query_running_flag())
		Sapp1_hid(key);
	#endif
	#ifdef DEF_ENABLE_SUB_APP2
	else if (Sapp2_query_running_flag())
		Sapp2_hid(key);
	#endif
	#ifdef DEF_ENABLE_SUB_APP3
	else if (Sapp3_query_running_flag())
		Sapp3_hid(key);
	#endif
	#ifdef DEF_ENABLE_SUB_APP4
	else if (Sapp4_query_running_flag())
		Sapp4_hid(key);
	#endif
	#ifdef DEF_ENABLE_SUB_APP5
	else if (Sapp5_query_running_flag())
		Sapp5_hid(key);
	#endif
	#ifdef DEF_ENABLE_SUB_APP6
	else if (Sapp6_query_running_flag())
		Sapp6_hid(key);
	#endif
	#ifdef DEF_ENABLE_SUB_APP7
	else if (Sapp7_query_running_flag())
		Sapp7_hid(key);
	#endif
	else if (Sem_query_running_flag())
		Sem_hid(key);
}

void Menu_get_system_info(void)
{
	u8 battery_level = -1;
	u8 battery_voltage = -1;
	u8 battery_temp = -1;
	char* ssid = (char*)malloc(512);
	Result_with_string result;

	PTMU_GetBatteryChargeState(&var_battery_charge);//battery charge
	result.code = MCUHWC_GetBatteryLevel(&battery_level);//battery level(%)
	if(result.code == 0)
	{
		MCUHWC_GetBatteryVoltage(&battery_voltage);
		MCUHWC_ReadRegister(0x0A, &battery_temp, 1);
		var_battery_voltage = 5.0 * (battery_voltage / 256.0);
		var_battery_level_raw = battery_level;
		var_battery_temp = battery_temp;
	}
	else
	{
		PTMU_GetBatteryLevel(&battery_level);
		if ((int)battery_level == 0)
			var_battery_level_raw = 0;
		else if ((int)battery_level == 1)
			var_battery_level_raw = 5;
		else if ((int)battery_level == 2)
			var_battery_level_raw = 10;
		else if ((int)battery_level == 3)
			var_battery_level_raw = 30;
		else if ((int)battery_level == 4)
			var_battery_level_raw = 60;
		else if ((int)battery_level == 5)
			var_battery_level_raw = 100;
	}

	//ssid
	result.code = ACU_GetSSID(ssid);
	if(result.code == 0)
		var_connected_ssid = ssid;
	else
		var_connected_ssid = "";

	free(ssid);
	ssid = NULL;

	var_wifi_signal = osGetWifiStrength();
	//Get wifi state from shared memory #0x1FF81067
	var_wifi_state = *(u8*)0x1FF81067;
	if(var_wifi_state == 2)
	{
#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)
		if (!var_connect_test_succes)
			var_wifi_signal += 4;
#endif
	}
	else
	{
		var_wifi_signal = 8;
		var_connect_test_succes = false;
	}

	//Get time
	time_t unixTime = time(NULL);
	struct tm* timeStruct = gmtime((const time_t*)&unixTime);
	var_years = timeStruct->tm_year + 1900;
	var_months = timeStruct->tm_mon + 1;
	var_days = timeStruct->tm_mday;
	var_hours = timeStruct->tm_hour;
	var_minutes = timeStruct->tm_min;
	var_seconds = timeStruct->tm_sec;

	if (var_debug_mode)
	{
		//check free RAM
		var_free_ram = Util_check_free_ram();
		var_free_linear_ram = Util_check_free_linear_space();
	}

	sprintf(var_status, "%02dfps %04d/%02d/%02d %02d:%02d:%02d ", (int)Draw_query_fps(), var_years, var_months, var_days, var_hours, var_minutes, var_seconds);
}

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)
void Menu_send_app_info_thread(void* arg)
{
	Util_log_save(DEF_MENU_SEND_APP_INFO_THREAD_STR, "Thread started.");
	OS_VersionBin os_ver;
	bool is_new3ds = false;
	u8* dl_data = NULL;

#if DEF_ENABLE_CURL_API
	int downloaded_size = 0;
	int uploaded_size = 0;
#else
	u32 downloaded_size = 0;
#endif
	char system_ver_char[0x50] = " ";
	std::string new3ds;

	osGetSystemVersionDataString(&os_ver, &os_ver, system_ver_char, 0x50);
	std::string system_ver = system_ver_char;

	APT_CheckNew3DS(&is_new3ds);
	new3ds = is_new3ds ? "yes" : "no";

	std::string send_data = "{ \"app_ver\": \"" + DEF_CURRENT_APP_VER + "\",\"system_ver\" : \"" + system_ver + "\",\"start_num_of_app\" : \"" + std::to_string(var_num_of_app_start) + "\",\"language\" : \"" + var_lang + "\",\"new3ds\" : \"" + new3ds + "\",\"time_to_enter_sleep\" : \"" + std::to_string(var_time_to_turn_off_lcd) + "\",\"scroll_speed\" : \"" + std::to_string(var_scroll_speed) + "\" }";

#if DEF_ENABLE_CURL_API
	Util_curl_post_and_dl_data(DEF_SEND_APP_INFO_URL, (u8*)send_data.c_str(), send_data.length(), &dl_data, 0x10000, &downloaded_size, &uploaded_size, true, 5);
#else
	Util_httpc_post_and_dl_data(DEF_SEND_APP_INFO_URL, (u8*)send_data.c_str(), send_data.length(), &dl_data, 0x10000, &downloaded_size, true, 5);
#endif

	Util_safe_linear_free(dl_data);
	dl_data = NULL;

	Util_log_save(DEF_MENU_SEND_APP_INFO_THREAD_STR, "Thread exit.");
	threadExit(0);
}
#endif

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)
void Menu_check_connectivity_thread(void* arg)
{
	Util_log_save(DEF_MENU_CHECK_INTERNET_THREAD_STR, "Thread started.");
	u8* http_buffer = NULL;
#if DEF_ENABLE_HTTPC_API
	u32 status_code = 0;
	u32 dl_size = 0;
#else
	int status_code = 0;
	int dl_size = 0;
#endif
	int count = 100;

	while (menu_thread_run)
	{
		if (count >= 100)
		{
			count = 0;
#if DEF_ENABLE_HTTPC_API//Curl uses more CPU so prefer to use httpc module here.
			Util_httpc_dl_data(DEF_CHECK_INTERNET_URL, &http_buffer, 0x1000, &dl_size, &status_code, false, 0);
#else
			Util_curl_dl_data(DEF_CHECK_INTERNET_URL, &http_buffer, 0x1000, &dl_size, &status_code, false, 0);
#endif
			Util_safe_linear_free(http_buffer);
			http_buffer = NULL;

			if (status_code == 204)
				var_connect_test_succes = true;
			else
				var_connect_test_succes = false;
		}
		else
			Util_sleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		count++;
	}

	Util_log_save(DEF_MENU_CHECK_INTERNET_THREAD_STR, "Thread exit.");
	threadExit(0);
}
#endif

void Menu_worker_thread(void* arg)
{
	Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Thread started.");
	int count = 0;
	u64 previous_ts = 0;
	Result_with_string result;

	while (menu_thread_run)
	{
		if(previous_ts + 50 <= osGetTime())
		{
			if(var_flash_mode)
			{
				var_night_mode = !var_night_mode;
				var_need_reflesh = true;
			}
			count++;

			if(previous_ts + 100 >= osGetTime())
				previous_ts += 50;
			else
				previous_ts = osGetTime();
		}

		if(count % 5 == 0)
			Menu_get_system_info();

		if(count >= 20)
		{
			var_need_reflesh = true;
			var_afk_time++;
			count = 0;
		}

		//If var_time_to_turn_off_lcd < 0, it means turn off LCD settings has been disabled.
		if(var_time_to_turn_off_lcd > 0 && var_afk_time > var_time_to_turn_off_lcd)
		{
			result = Util_cset_set_screen_state(true, true, false);
			if(result.code != 0)
				Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_state()..." + result.string + result.error_description, result.code);
		}
		else if(var_time_to_turn_off_lcd > 0 &&var_afk_time > (var_time_to_turn_off_lcd - 10))
		{
			result = Util_cset_set_screen_brightness(true, true, 10);
			if(result.code != 0)
				Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_brightness()..." + result.string + result.error_description, result.code);
		}
		else
		{
			result = Util_cset_set_screen_state(true, false, var_turn_on_top_lcd);
			if(result.code != 0)
				Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_state()..." + result.string + result.error_description, result.code);

			result = Util_cset_set_screen_state(false, true, var_turn_on_bottom_lcd);
			if(result.code != 0)
				Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_state()..." + result.string + result.error_description, result.code);

			if(var_top_lcd_brightness == var_lcd_brightness && var_bottom_lcd_brightness == var_lcd_brightness)
			{
				result = Util_cset_set_screen_brightness(true, true, var_lcd_brightness);
				if(result.code != 0)
					Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_brightness()..." + result.string + result.error_description, result.code);
			}
			else
			{
				result = Util_cset_set_screen_brightness(true, false, var_top_lcd_brightness);
				if(result.code != 0)
					Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_brightness()..." + result.string + result.error_description, result.code);
				result = Util_cset_set_screen_brightness(false, true, var_bottom_lcd_brightness);
				if(result.code != 0)
					Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_brightness()..." + result.string + result.error_description, result.code);
			}
		}

		if(var_time_to_enter_sleep > 0 && var_afk_time > var_time_to_enter_sleep)
		{
			result = Util_cset_sleep_system((Wake_up_event)(WAKE_UP_EVENT_OPEN_SHELL | WAKE_UP_EVENT_PRESS_HOME_BUTTON));
			if(result.code == 0)
			{
				//We woke up from sleep.
				var_afk_time = 0;
			}
			else
				Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_sleep_system()..." + result.string + result.error_description, result.code);
		}

		LightLock_Lock(&menu_callback_mutex);

		//Call callback functions.
		for(int i = 0; i < DEF_MENU_NUM_OF_CALLBACKS; i++)
		{
			if(menu_worker_thread_callbacks[i])
				menu_worker_thread_callbacks[i]();
		}

		LightLock_Unlock(&menu_callback_mutex);

		gspWaitForVBlank();
	}
	Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Thread exit.");
	threadExit(0);
}

#if (DEF_ENABLE_CURL_API || DEF_ENABLE_HTTPC_API)
void Menu_update_thread(void* arg)
{
	Util_log_save(DEF_MENU_UPDATE_THREAD_STR, "Thread started.");
	u8* http_buffer = NULL;
#if DEF_ENABLE_CURL_API
	int dl_size = 0;
#else
	u32 dl_size = 0;
#endif
	size_t pos[2] = { 0, 0, };
	std::string data = "";
	Result_with_string result;

#if DEF_ENABLE_CURL_API
	result = Util_curl_dl_data(DEF_CHECK_UPDATE_URL, &http_buffer, 0x1000, &dl_size, true, 3);
	Util_log_save(DEF_MENU_UPDATE_THREAD_STR, "Util_curl_dl_data()..." + result.string + result.error_description, result.code);
#else
	result = Util_httpc_dl_data(DEF_CHECK_UPDATE_URL, &http_buffer, 0x1000, &dl_size, true, 3);
	Util_log_save(DEF_MENU_UPDATE_THREAD_STR, "Util_httpc_dl_data()..." + result.string + result.error_description, result.code);
#endif

	if(result.code == 0)
	{
		data = (char*)http_buffer;
		pos[0] = data.find("<newest>");
		pos[1] = data.find("</newest>");
		if(pos[0] != std::string::npos && pos[1] != std::string::npos)
		{
			data = data.substr(pos[0] + 8, pos[1] - (pos[0] + 8));
			if(DEF_CURRENT_APP_VER_INT < atoi(data.c_str()))
				menu_update_available = true;
		}
	}

	Util_safe_linear_free(http_buffer);
	http_buffer = NULL;

	Util_log_save(DEF_MENU_UPDATE_THREAD_STR, "Thread exit.");
	threadExit(0);
}
#endif
