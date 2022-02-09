#include "system/headers.hpp"

#include "system/setting_menu.hpp"

#ifdef DEF_ENABLE_SUB_APP0
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

bool menu_thread_run = false;
bool menu_main_run = true;
bool menu_must_exit = false;
bool menu_check_exit_request = false;
bool menu_update_available = false;
bool menu_init_request[9] = { false, false, false, false, false, false, false, false, false, };
bool menu_exit_request[8] = { false, false, false, false, false, false, false, false, };
int menu_icon_texture_num[9] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, };
std::string menu_msg[DEF_MENU_NUM_OF_MSG];
Thread menu_worker_thread, menu_send_app_info_thread, menu_check_connectivity_thread, menu_update_thread, menu_hid_thread;
C2D_Image menu_icon_image[10];
Image_data menu_sapp_button[8], menu_sapp_close_button[8], menu_sem_button;

int Menu_check_free_ram(void);
void Menu_get_system_info(void);
void Menu_hid_thread(void* arg);
void Menu_send_app_info_thread(void* arg);
void Menu_check_connectivity_thread(void* arg);
void Menu_worker_thread(void* arg);
void Menu_update_thread(void* arg);

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
	u8* data = NULL;
	u8 region = 0;
	u8 model = 0;
	u32 read_size = 0;
	Thread core_2, core_3;
	Result_with_string result;

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

	//create directory
	Util_file_save_to_file(".", DEF_MAIN_DIR, NULL, 0, false);
	Util_file_save_to_file(".", DEF_MAIN_DIR + "screen_recording/", NULL, 0, false);

	if(Util_file_load_from_file("fake_model.txt", DEF_MAIN_DIR, &data, 1, &read_size).code == 0 && *data <= 5)
	{
		var_fake_model = true;
		var_model = *data;
		free(data);
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
	Util_log_save(DEF_MENU_INIT_STR, "Draw_init()...", Draw_init(var_high_resolution_mode, var_3d_mode).code);
	Draw_frame_ready();
	Draw_screen_ready(0, DEF_DRAW_WHITE);
	Draw_screen_ready(1, DEF_DRAW_WHITE);
	Draw_apply_draw();
	Sem_draw_init();

	result = Util_httpc_init(DEF_HTTP_POST_BUFFER_SIZE);
	Util_log_save(DEF_MENU_INIT_STR, "Util_httpc_init()...", result.code);

	result = Util_hid_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_hid_init()...", result.code);

	result = Util_expl_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_expl_init()...", result.code);

	result = Exfont_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_expl_init()...", result.code);

	result = Util_err_init();
	Util_log_save(DEF_MENU_INIT_STR, "Util_err_init()...", result.code);

	for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
		Exfont_set_external_font_request_state(i, true);

	for(int i = 0; i < 4; i++)
		Exfont_set_system_font_request_state(i, true);

	Exfont_request_load_external_font();
	Exfont_request_load_system_font();

	menu_thread_run = true;
	menu_worker_thread = threadCreate(Menu_worker_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 1, false);
	menu_check_connectivity_thread = threadCreate(Menu_check_connectivity_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	menu_update_thread = threadCreate(Menu_update_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 1, false);
	menu_hid_thread = threadCreate(Menu_hid_thread, (void*)(""), 1024 * 4, DEF_THREAD_PRIORITY_REALTIME, 0, false);

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

	if (var_allow_send_app_info)
		menu_send_app_info_thread = threadCreate(Menu_send_app_info_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_LOW, 1, true);

	#ifdef DEF_VID_ENABLE_ICON
	menu_icon_texture_num[0] = Draw_get_free_sheet_num();
	result = Draw_load_texture(DEF_VID_ICON_PATH, menu_icon_texture_num[0], menu_icon_image, 0, 1);
	Util_log_save(DEF_MENU_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);
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
	Result_with_string result;

	menu_thread_run = false;

	#ifdef DEF_ENABLE_SUB_APP0
	if (Vid_query_init_flag())
		Vid_exit();
	#endif
	#ifdef DEF_ENABLE_SUB_APP1
	if (Sapp1_query_init_flag())
		Sapp1_exit();
	#endif
	#ifdef DEF_ENABLE_SUB_APP2
	if (Sapp2_query_init_flag())
		Sapp2_exit();
	#endif
	#ifdef DEF_ENABLE_SUB_APP3
	if (Sapp3_query_init_flag())
		Sapp3_exit();
	#endif
	#ifdef DEF_ENABLE_SUB_APP4
	if (Sapp4_query_init_flag())
		Sapp4_exit();
	#endif
	#ifdef DEF_ENABLE_SUB_APP5
	if (Sapp5_query_init_flag())
		Sapp5_exit();
	#endif
	#ifdef DEF_ENABLE_SUB_APP6
	if (Sapp6_query_init_flag())
		Sapp6_exit();
	#endif
	#ifdef DEF_ENABLE_SUB_APP7
	if (Sapp7_query_init_flag())
		Sapp7_exit();
	#endif
	if (Sem_query_init_flag())
		Sem_exit();

	for(int i = 0; i < 8; i++)
		Draw_free_texture(menu_icon_texture_num[i]);

	Util_hid_exit();
	Util_expl_exit();
	Exfont_exit();
	Util_err_exit();
	Util_exit();

	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_worker_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_check_connectivity_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_send_app_info_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_update_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_MENU_EXIT_STR, "threadJoin()...", threadJoin(menu_hid_thread, DEF_THREAD_WAIT_TIME));
	threadFree(menu_worker_thread);
	threadFree(menu_check_connectivity_thread);
	threadFree(menu_send_app_info_thread);
	threadFree(menu_update_thread);
	threadFree(menu_hid_thread);

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

void Menu_main(void)
{
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

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
			Draw_screen_ready(0, back_color);

			if(menu_check_exit_request)
			{
				Draw(menu_msg[DEF_MENU_EXIST_MSG], 0, 105, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_CENTER,
					DEF_DRAW_Y_ALIGN_CENTER, 400, 20);
				Draw(menu_msg[DEF_MENU_CONFIRM_MSG], 10, 140, 0.5, 0.5, DEF_DRAW_GREEN, DEF_DRAW_X_ALIGN_RIGHT,
					DEF_DRAW_Y_ALIGN_CENTER, 190, 20);
				Draw(menu_msg[DEF_MENU_CANCEL_MSG], 210, 140, 0.5, 0.5, DEF_DRAW_RED, DEF_DRAW_X_ALIGN_LEFT,
					DEF_DRAW_Y_ALIGN_CENTER, 190, 20);
			}
			else if(menu_update_available)
			{
				Draw(menu_msg[DEF_MENU_NEW_VERSION_MSG], 10, 30, 0.7, 0.7, DEF_DRAW_RED);
				Draw(menu_msg[DEF_MENU_HOW_TO_UPDATE_MSG], 10, 60, 0.5, 0.5, color);
			}

			if(Util_log_query_log_show_flag())
				Util_log_draw();
	
			Draw_top_ui();

			Draw_screen_ready(1, back_color);

			#ifdef DEF_ENABLE_SUB_APP0
			Draw_texture(&menu_sapp_button[0], menu_sapp_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 0, 60, 60);

			#ifdef DEF_VID_ENABLE_ICON
			Draw_texture(menu_icon_image[0], 0, 0, 60, 60);
			#endif
			#ifdef DEF_VID_ENABLE_NAME
			Draw(DEF_VID_NAME, 0, 0, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
			#endif

			if(Vid_query_init_flag())
			{
				Draw_texture(&menu_sapp_close_button[0], menu_sapp_close_button[0].selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 45, 0, 15, 15);
				Draw("X", 47.5, 0, 0.5, 0.5, DEF_DRAW_RED);
			}
			#endif
			#ifdef DEF_ENABLE_SUB_APP1
			Draw_texture(&menu_sapp_button[1], menu_sapp_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 80, 0, 60, 60);

			#ifdef DEF_SAPP1_ENABLE_ICON
			Draw_texture(menu_icon_image[1], 80, 0, 60, 60);
			#endif
			#ifdef DEF_SAPP1_ENABLE_NAME
			Draw(DEF_SAPP1_NAME, 80, 0, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
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
			Draw(DEF_SAPP2_NAME, 160, 0, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
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
			Draw(DEF_SAPP3_NAME, 240, 0, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
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
			Draw(DEF_SAPP4_NAME, 0, 80, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
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
			Draw(DEF_SAPP5_NAME, 80, 80, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
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
			Draw(DEF_SAPP6_NAME, 160, 80, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
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
			Draw(DEF_SAPP7_NAME, 240, 80, 0.4, 0.4, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 60, 60);
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

		#ifdef DEF_ENABLE_SUB_APP0
		if(menu_init_request[0])
		{
			Vid_init();
			menu_init_request[0] = false;
		}
		else if(menu_exit_request[0])
		{
			Vid_exit();
			menu_exit_request[0] = false;
		}
		#endif
		#ifdef DEF_ENABLE_SUB_APP1
		if(menu_init_request[1])
		{
			Sapp1_init();
			menu_init_request[1] = false;
		}
		else if(menu_exit_request[1])
		{
			Sapp1_exit();
			menu_exit_request[1] = false;
		}
		#endif
		#ifdef DEF_ENABLE_SUB_APP2
		if(menu_init_request[2])
		{
			Sapp2_init();
			menu_init_request[2] = false;
		}
		else if(menu_exit_request[2])
		{
			Sapp2_exit();
			menu_exit_request[2] = false;
		}
		#endif
		#ifdef DEF_ENABLE_SUB_APP3
		if(menu_init_request[3])
		{
			Sapp3_init();
			menu_init_request[3] = false;
		}
		else if(menu_exit_request[3])
		{
			Sapp3_exit();
			menu_exit_request[3] = false;
		}
		#endif
		#ifdef DEF_ENABLE_SUB_APP4
		if(menu_init_request[4])
		{
			Sapp4_init();
			menu_init_request[4] = false;
		}
		else if(menu_exit_request[4])
		{
			Sapp4_exit();
			menu_exit_request[4] = false;
		}
		#endif
		#ifdef DEF_ENABLE_SUB_APP5
		if(menu_init_request[5])
		{
			Sapp5_init();
			menu_init_request[5] = false;
		}
		else if(menu_exit_request[5])
		{
			Sapp5_exit();
			menu_exit_request[5] = false;
		}
		#endif
		#ifdef DEF_ENABLE_SUB_APP6
		if(menu_init_request[6])
		{
			Sapp6_init();
			menu_init_request[6] = false;
		}
		else if(menu_exit_request[6])
		{
			Sapp6_exit();
			menu_exit_request[6] = false;
		}
		#endif
		#ifdef DEF_ENABLE_SUB_APP7
		if(menu_init_request[7])
		{
			Sapp7_init();
			menu_init_request[7] = false;
		}
		else if(menu_exit_request[7])
		{
			Sapp7_exit();
			menu_exit_request[7] = false;
		}
		#endif
		if(menu_init_request[8])
		{
			Sem_init();
			menu_init_request[8] = false;
		}
	}
	#ifdef DEF_ENABLE_SUB_APP0
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

void Menu_hid_thread(void* arg)
{
	Util_log_save(DEF_MENU_HID_THREAD_STR, "Thread started.");
	u64 previous_ts = -1;
	Hid_info key;

	while (menu_thread_run)
	{
		Util_hid_query_key_state(&key);
		if (previous_ts != key.ts)
		{
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
							#ifdef DEF_ENABLE_SUB_APP0
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[0]) && Vid_query_init_flag())
								menu_sapp_close_button[0].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[0]) && Vid_query_init_flag() && menu_sapp_close_button[0].selected)
								menu_exit_request[0] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[0]))
								menu_sapp_button[0].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[0]) && menu_sapp_button[0].selected)
							{
								if (!Vid_query_init_flag())
									menu_init_request[0] = true;
								else
									Vid_resume();
							}
							#endif
							#ifdef DEF_ENABLE_SUB_APP1
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[1]) && Sapp1_query_init_flag())
								menu_sapp_close_button[1].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[1]) && Sapp1_query_init_flag() && menu_sapp_close_button[1].selected)
								menu_exit_request[1] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[1]))
								menu_sapp_button[1].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[1]) && menu_sapp_button[1].selected)
							{
								if (!Sapp1_query_init_flag())
									menu_init_request[1] = true;
								else
									Sapp1_resume();
							}
							#endif
							#ifdef DEF_ENABLE_SUB_APP2
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[2]) && Sapp2_query_init_flag())
								menu_sapp_close_button[2].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[2]) && Sapp2_query_init_flag() && menu_sapp_close_button[2].selected)
								menu_exit_request[2] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[2]))
								menu_sapp_button[2].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[2]) && menu_sapp_button[2].selected)
							{
								if (!Sapp2_query_init_flag())
									menu_init_request[2] = true;
								else
									Sapp2_resume();
							}
							#endif
							#ifdef DEF_ENABLE_SUB_APP3
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[3]) && Sapp3_query_init_flag())
								menu_sapp_close_button[3].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[3]) && Sapp3_query_init_flag() && menu_sapp_close_button[3].selected)
								menu_exit_request[3] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[3]))
								menu_sapp_button[3].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[3]) && menu_sapp_button[3].selected)
							{
								if (!Sapp3_query_init_flag())
									menu_init_request[3] = true;
								else
									Sapp3_resume();
							}
							#endif
							#ifdef DEF_ENABLE_SUB_APP4
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[4]) && Sapp4_query_init_flag())
								menu_sapp_close_button[4].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[4]) && Sapp4_query_init_flag() && menu_sapp_close_button[4].selected)
								menu_exit_request[4] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[4]))
								menu_sapp_button[4].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[4]) && menu_sapp_button[4].selected)
							{
								if (!Sapp4_query_init_flag())
									menu_init_request[4] = true;
								else
									Sapp4_resume();
							}
							#endif
							#ifdef DEF_ENABLE_SUB_APP5
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[5]) && Sapp5_query_init_flag())
								menu_sapp_close_button[5].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[5]) && Sapp5_query_init_flag() && menu_sapp_close_button[5].selected)
								menu_exit_request[5] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[5]))
								menu_sapp_button[5].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[5]) && menu_sapp_button[5].selected)
							{
								if (!Sapp5_query_init_flag())
									menu_init_request[5] = true;
								else
									Sapp5_resume();
							}
							#endif
							#ifdef DEF_ENABLE_SUB_APP6
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[6]) && Sapp6_query_init_flag())
								menu_sapp_close_button[6].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[6]) && Sapp6_query_init_flag() && menu_sapp_close_button[6].selected)
								menu_exit_request[6] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[6]))
								menu_sapp_button[6].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[6]) && menu_sapp_button[6].selected)
							{
								if (!Sapp6_query_init_flag())
									menu_init_request[6] = true;
								else
									Sapp6_resume();
							}
							#endif
							#ifdef DEF_ENABLE_SUB_APP7
							else if (Util_hid_is_pressed(key, menu_sapp_close_button[7]) && Sapp7_query_init_flag())
								menu_sapp_close_button[7].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_close_button[7]) && Sapp7_query_init_flag() && menu_sapp_close_button[7].selected)
								menu_exit_request[7] = true;
							else if (Util_hid_is_pressed(key, menu_sapp_button[7]))
								menu_sapp_button[7].selected = true;
							else if (Util_hid_is_released(key, menu_sapp_button[7]) && menu_sapp_button[7].selected)
							{
								if (!Sapp7_query_init_flag())
									menu_init_request[7] = true;
								else
									Sapp7_resume();
							}
							#endif
							else if (Util_hid_is_pressed(key, menu_sem_button))
								menu_sem_button.selected = true;
							else if (Util_hid_is_released(key, menu_sem_button) && menu_sem_button.selected)
							{
								if (!Sem_query_init_flag())
									menu_init_request[8] = true;
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
			#ifdef DEF_ENABLE_SUB_APP0
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

			previous_ts = key.ts;
		}

		gspWaitForVBlank();
	}

	Util_log_save(DEF_MENU_HID_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Menu_get_system_info(void)
{
	u8 battery_level = -1;
	u8 battery_voltage = -1;
	char* ssid = (char*)malloc(512);
	Result_with_string result;

	PTMU_GetBatteryChargeState(&var_battery_charge);//battery charge
	result.code = MCUHWC_GetBatteryLevel(&battery_level);//battery level(%)
	if(result.code == 0)
	{
		MCUHWC_GetBatteryVoltage(&battery_voltage);
		var_battery_voltage = 5.0 * (battery_voltage / 256); 
		var_battery_level_raw = battery_level;
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
		if (!var_connect_test_succes)
			var_wifi_signal += 4;
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

void Menu_send_app_info_thread(void* arg)
{
	Util_log_save(DEF_MENU_SEND_APP_INFO_THREAD_STR, "Thread started.");
	OS_VersionBin os_ver;
	bool is_new3ds = false;
	u8* dl_data = NULL;
	u32 downloaded_size = 0;
	char system_ver_char[0x50] = " ";
	std::string new3ds;

	osGetSystemVersionDataString(&os_ver, &os_ver, system_ver_char, 0x50);
	std::string system_ver = system_ver_char;

	APT_CheckNew3DS(&is_new3ds);
	new3ds = is_new3ds ? "yes" : "no";

	std::string send_data = "{ \"app_ver\": \"" + DEF_CURRENT_APP_VER + "\",\"system_ver\" : \"" + system_ver + "\",\"start_num_of_app\" : \"" + std::to_string(var_num_of_app_start) + "\",\"language\" : \"" + var_lang + "\",\"new3ds\" : \"" + new3ds + "\",\"time_to_enter_sleep\" : \"" + std::to_string(var_time_to_turn_off_lcd) + "\",\"scroll_speed\" : \"" + std::to_string(var_scroll_speed) + "\" }";
	Util_httpc_post_and_dl_data(DEF_SEND_APP_INFO_URL, (u8*)send_data.c_str(), send_data.length(), &dl_data, 0x10000, &downloaded_size, true, 5);
	free(dl_data);
	dl_data = NULL;

	Util_log_save(DEF_MENU_SEND_APP_INFO_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Menu_check_connectivity_thread(void* arg)
{
	Util_log_save(DEF_MENU_CHECK_INTERNET_THREAD_STR, "Thread started.");
	u8* http_buffer = NULL;
	u32 status_code = 0;
	u32 dl_size = 0;
	int count = 100;

	while (menu_thread_run)
	{
		if (count >= 100)
		{
			count = 0;
			Util_httpc_dl_data(DEF_CHECK_INTERNET_URL, &http_buffer, 0x1000, &dl_size, &status_code, false, 0);
			free(http_buffer);
			http_buffer = NULL;

			if (status_code == 204)
				var_connect_test_succes = true;
			else
				var_connect_test_succes = false;
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		count++;
	}

	Util_log_save(DEF_MENU_CHECK_INTERNET_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Menu_worker_thread(void* arg)
{
	Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Thread started.");
	int count = 0;
	Result_with_string result;

	while (menu_thread_run)
	{
		usleep(49000);
		count++;

		if(count % 5 == 0)
			Menu_get_system_info();

		if (count >= 20)
		{
			var_need_reflesh = true;
			var_afk_time++;
			count = 0;
		}

		if(var_afk_time > var_time_to_turn_off_lcd)
		{
			result = Util_cset_set_screen_state(true, true, false);
			if(result.code != 0)
				Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Util_cset_set_screen_state()..." + result.string + result.error_description, result.code);
		}
		else if(var_afk_time > (var_time_to_turn_off_lcd - 10))
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

		if (var_flash_mode)
		{
			var_night_mode = !var_night_mode;
			var_need_reflesh = true;
		}
	}
	Util_log_save(DEF_MENU_WORKER_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Menu_update_thread(void* arg)
{
	Util_log_save(DEF_MENU_UPDATE_THREAD_STR, "Thread started.");
	u8* http_buffer = NULL;
	u32 status_code = 0;
	u32 dl_size = 0;
	size_t pos[2] = { 0, 0, };
	std::string data = "";
	Result_with_string result;

	result = Util_httpc_dl_data(DEF_CHECK_UPDATE_URL, &http_buffer, 0x1000, &dl_size, &status_code, true, 3);
	Util_log_save(DEF_MENU_UPDATE_THREAD_STR, "Util_httpc_dl_data()..." + result.string + result.error_description, result.code);
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

	free(http_buffer);
	http_buffer = NULL;

	Util_log_save(DEF_MENU_UPDATE_THREAD_STR, "Thread exit.");
	threadExit(0);
}