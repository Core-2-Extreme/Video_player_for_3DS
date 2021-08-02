#include "headers.hpp"

#include "system/setting_menu.hpp"

bool sem_main_run = false;
bool sem_already_init = false;
bool sem_thread_run = false;
bool sem_thread_suspend = false;
bool sem_record_request = false;
bool sem_encode_request = false;
bool sem_wait_request = false;
bool sem_stop_record_request = false;
bool sem_reload_msg_request = false;
bool sem_new_version_available = false;
bool sem_check_update_request = false;
bool sem_show_patch_note_request = false;
bool sem_select_ver_request = false;
bool sem_change_brightness_request = false;
bool sem_bar_selected[2] = { false, false, };
bool sem_dl_file_request = false;
bool sem_scroll_bar_selected = false;
bool sem_scroll_mode = false;
bool sem_button_selected[DEF_EXFONT_NUM_OF_FONT_NAME];
u8* sem_yuv420p = NULL;
u32 sem_dled_size = 0;
int sem_rec_width = 400;
int sem_rec_height = 480;
int sem_selected_recording_mode = 0;
int sem_selected_menu_mode = 0;
int sem_update_progress = -1;
int sem_check_update_progress = 0;
int sem_selected_edition_num = 0;
int sem_installed_size = 0;
int sem_total_cia_size = 0;
double sem_y_offset = 0.0;
double sem_y_max = 0.0;
double sem_touch_x_move_left = 0.0;
double sem_touch_y_move_left = 0.0;
std::string sem_msg[DEF_SEM_NUM_OF_MSG];
std::string sem_newest_ver_data[6];//0 newest version number, 1 3dsx available, 2 cia available, 3 3dsx dl url, 4 cia dl url, 5 patch note

Thread sem_update_thread, sem_worker_thread, sem_record_thread, sem_encode_thread;

bool Sem_query_init_flag(void)
{
	return sem_already_init;
}

bool Sem_query_running_flag(void)
{
	return sem_main_run;
}

void Sem_suspend(void)
{
	Menu_resume();
	sem_thread_suspend = true;
	sem_main_run = false;
}

void Sem_resume(void)
{
	sem_thread_suspend = false;
	sem_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Sem_init(void)
{
	Util_log_save(DEF_SEM_INIT_STR, "Initializing...");
	bool wifi_state = true;
	u8 region;
	u8 model;
	u8* cache = (u8*)malloc(0x1000);
	u32 read_size = 0;
	std::string data[11];
	Result_with_string result;

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

	if(CFGU_GetSystemModel(&model))
	{
		if(model == 0)
			var_model = "OLD 3DS";
		else if(model == 1)
			var_model = "OLD 3DS XL";
		else if(model == 2)
			var_model = "NEW 3DS";
		else if(model == 3)
			var_model = "OLD 2DS";
		else if(model == 4)
			var_model = "NEW 3DS XL";
		else if(model == 5)
			var_model = "NEW 2DS XL";
	}

	result = Util_file_load_from_file("settings.txt", DEF_MAIN_DIR, cache, 0x1000, &read_size);
	Util_log_save(DEF_SEM_INIT_STR , "Util_file_load_from_file()..." + result.string + result.error_description, result.code);

	result = Util_parse_file((char*)cache, 11, data);
	Util_log_save(DEF_SEM_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
	if(result.code == 0)
	{
		var_lang = data[0];
		var_lcd_brightness = atoi(data[1].c_str());
		var_time_to_turn_off_lcd = atoi(data[2].c_str());
		var_scroll_speed = strtod(data[3].c_str(), NULL);
		var_allow_send_app_info = (data[4] == "1");
		var_num_of_app_start = atoi(data[5].c_str());
		var_night_mode = (data[6] == "1");
		var_eco_mode = (data[7] == "1");
		wifi_state = (data[8] == "1");
		var_high_resolution_mode = (data[9] == "1");
		var_3d_mode = (data[10] == "1");

		if(var_lang != "jp" && var_lang != "en")
			var_lang = "en";
		if(var_lcd_brightness < 15 || var_lcd_brightness > 163)
			var_lcd_brightness = 100;
		if(var_time_to_turn_off_lcd < 10 || var_time_to_turn_off_lcd > 310)
			var_time_to_turn_off_lcd = 150;
		if(var_scroll_speed < 0.033 || var_scroll_speed > 1.030)
			var_scroll_speed = 0.5;
		if(var_num_of_app_start < 0)
			var_num_of_app_start = 0;
		
		var_top_lcd_brightness = var_lcd_brightness;
		var_bottom_lcd_brightness = var_lcd_brightness;
	}

	if(var_model == "OLD 2DS")//OLD 2DS doesn't support high resolution mode
		var_high_resolution_mode = false;
	
	if(var_model == "OLD 2DS" || var_model == "NEW 2DS XL")//2DSs don't support 3d mode
		var_3d_mode = false;

	sem_thread_run = true;
	sem_update_thread = threadCreate(Sem_update_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 0, false);
	sem_worker_thread = threadCreate(Sem_worker_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 0, false);
	sem_record_thread = threadCreate(Sem_record_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
	if(var_model == "NEW 2DS XL" || var_model == "NEW 3DS XL" || var_model == "NEW 3DS")
		sem_encode_thread = threadCreate(Sem_encode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 2, false);
	else
		sem_encode_thread = threadCreate(Sem_encode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 1, false);

	sem_reload_msg_request = true;

	result = Util_cset_set_wifi_state(wifi_state);
	if(result.code == 0 || result.code == 0xC8A06C0D)
		var_wifi_enabled = wifi_state;
	
	free(cache);
	cache = NULL;

	Sem_resume();
	sem_already_init = true;
	Util_log_save(DEF_SEM_INIT_STR, "Initialized.");
}

void Sem_exit(void)
{
	Util_log_save(DEF_SEM_EXIT_STR, "Exiting...");
	u64 time_out = 10000000000;
	int log_num;
	var_num_of_app_start++;
	std::string data = "<0>" + var_lang + "</0><1>" + std::to_string(var_lcd_brightness) + "</1><2>" + std::to_string(var_time_to_turn_off_lcd)
	+ "</2><3>" + std::to_string(var_scroll_speed) + "</3><4>" + std::to_string(var_allow_send_app_info) + "</4><5>" + std::to_string(var_num_of_app_start)
	+ "</5><6>" + std::to_string(var_night_mode) + "</6><7>" + std::to_string(var_eco_mode) + "</7><8>" + std::to_string(var_wifi_enabled) + "</8>"
	+ "<9>" + std::to_string(var_high_resolution_mode) + "</9><10>" + std::to_string(var_3d_mode) + "</10>";
	Result_with_string result;

	sem_stop_record_request = true;
	sem_already_init = false;
	sem_thread_suspend = false;
	sem_thread_run = false;

	log_num = Util_log_save(DEF_SEM_EXIT_STR, "Util_file_save_to_file()...");
	result = Util_file_save_to_file("settings.txt", DEF_MAIN_DIR, (u8*)data.c_str(), data.length(), true);
	Util_log_add(log_num, result.string, result.code);

	Util_log_save(DEF_SEM_EXIT_STR, "threadJoin()...", threadJoin(sem_update_thread, time_out));
	Util_log_save(DEF_SEM_EXIT_STR, "threadJoin()...", threadJoin(sem_worker_thread, time_out));
	Util_log_save(DEF_SEM_EXIT_STR, "threadJoin()...", threadJoin(sem_encode_thread, time_out));
	Util_log_save(DEF_SEM_EXIT_STR, "threadJoin()...", threadJoin(sem_record_thread, time_out));

	threadFree(sem_update_thread);
	threadFree(sem_worker_thread);
	threadFree(sem_encode_thread);
	threadFree(sem_record_thread);

	Util_log_save(DEF_SEM_EXIT_STR, "Exited.");
}

void Sem_main(void)
{
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;
	int cache_color[DEF_EXFONT_NUM_OF_FONT_NAME];
	double draw_x;
	double draw_y;
	Result_with_string result;
	Hid_info key;
	Util_hid_query_key_state(&key);
	Util_hid_key_flag_reset();

	if (var_night_mode)
	{
		color = DEF_DRAW_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	for(int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
		cache_color[i] = color;

	if(var_need_reflesh || !var_eco_mode)
	{
		var_need_reflesh = false;
		Draw_frame_ready();
		Draw_screen_ready(0, back_color);

		if(Util_log_query_log_show_flag())
			Util_log_draw();
	
		Draw_top_ui();

		Draw_screen_ready(1, back_color);

		if (sem_selected_menu_mode >= 1 && sem_selected_menu_mode <= 9)
		{
			draw_y = 0.0;
			if (draw_y + sem_y_offset >= -30 && draw_y + sem_y_offset <= 240)
			{
				//Back
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_RED, 0.0, draw_y + sem_y_offset, 40, 25);
				Draw(sem_msg[30], 0.0, draw_y + sem_y_offset + 5.0, 0.6, 0.6, color);
			}
		}

		if (sem_selected_menu_mode == 5)
		{
			Draw_texture(var_square_image[0], color, 312.5, 0.0, 7.5, 15.0);
			Draw_texture(var_square_image[0], color, 312.5, 215.0, 7.5, 10.0);
			Draw_texture(var_square_image[0], DEF_DRAW_BLUE, 312.5, 15.0 + (195 * (sem_y_offset / sem_y_max)), 7.5, 5.0);
		}

		if (sem_selected_menu_mode == 0)
		{
			//Update
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 0, 240, 20);
			Draw(sem_msg[0], 0, 0, 0.75, 0.75, color);

			//Lang
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 25, 240, 20);
			Draw(sem_msg[1], 0, 25, 0.75, 0.75, color);

			//LCD
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 50, 240, 20);
			Draw(sem_msg[2], 0, 50, 0.75, 0.75, color);

			//Controll
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 75, 240, 20);
			Draw(sem_msg[3], 0, 75, 0.75, 0.75, color);

			//Font
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 100, 240, 20);
			Draw(sem_msg[4], 0, 100, 0.75, 0.75, color);

			//Wireless
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 125, 240, 20);
			Draw(sem_msg[5], 0, 125, 0.75, 0.75, color);

			//Advanced
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 150, 240, 20);
			Draw(sem_msg[6], 0, 150, 0.75, 0.75, color);

			//Battery
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 175, 240, 20);
			Draw(sem_msg[7], 0, 175, 0.75, 0.75, color);

			//Screen recording
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 0, 200, 240, 20);
			Draw(sem_msg[49], 0, 200, 0.75, 0.75, color);
		}
		else if (sem_selected_menu_mode == 1)
		{
			//Check for updates
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 25, 240, 20);
			Draw(sem_msg[8], 10, 25, 0.75, 0.75, color);
		}
		else if (sem_selected_menu_mode == 2)
		{
			//Languages
			if (var_lang == "en")
				cache_color[0] = DEF_DRAW_RED;
			else
				cache_color[1] = DEF_DRAW_RED;

			//English
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 25, 240, 20);
			Draw(sem_msg[9], 10, 25, 0.75, 0.75, cache_color[0]);

			//Japanese
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 50, 240, 20);
			Draw(sem_msg[10], 10, 50, 0.75, 0.75, cache_color[1]);
		}
		else if (sem_selected_menu_mode == 3)
		{
			if (var_night_mode)
				cache_color[0] = DEF_DRAW_RED;
			else
				cache_color[1] = DEF_DRAW_RED;

			if (var_flash_mode)
				cache_color[2] = DEF_DRAW_RED;

			if(sem_record_request && var_night_mode)
			{
				cache_color[3] = DEF_DRAW_WEAK_WHITE;
				cache_color[4] = DEF_DRAW_WEAK_WHITE;
				cache_color[5] = DEF_DRAW_WEAK_WHITE;
			}
			else if(sem_record_request)
			{
				cache_color[3] = DEF_DRAW_WEAK_BLACK;
				cache_color[4] = DEF_DRAW_WEAK_BLACK;
				cache_color[5] = DEF_DRAW_WEAK_BLACK;
			}

			if(var_model == "OLD 2DS" && var_night_mode)
			{
				cache_color[3] = DEF_DRAW_WEAK_WHITE;
				cache_color[4] = DEF_DRAW_WEAK_WHITE;
			}
			else if(var_model == "OLD 2DS")
			{
				cache_color[3] = DEF_DRAW_WEAK_BLACK;
				cache_color[4] = DEF_DRAW_WEAK_BLACK;
			}

			if(var_model == "NEW 2DS XL" && var_night_mode)
				cache_color[4] = DEF_DRAW_WEAK_WHITE;
			else if(var_model == "NEW 2DS XL")
				cache_color[4] = DEF_DRAW_WEAK_BLACK;

			if (var_high_resolution_mode)
				cache_color[3] = DEF_DRAW_RED;
			else if(var_3d_mode)
				cache_color[4] = DEF_DRAW_RED;
			else
				cache_color[5] = DEF_DRAW_RED;

			//Night mode
			Draw(sem_msg[11], 0, 25, 0.5, 0.5, color);
			//ON
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 40, 90, 20);
			Draw(sem_msg[12], 10, 40, 0.65, 0.65, cache_color[0]);
			//OFF
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 110, 40, 90, 20);
			Draw(sem_msg[13], 110, 40, 0.65, 0.65, cache_color[1]);
			//Flash
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 210, 40, 50, 20);
			Draw(sem_msg[14], 210, 40, 0.65, 0.65, cache_color[2]);

			//Screen brightness
			Draw(sem_msg[15] + std::to_string(var_lcd_brightness), 0, 65, 0.5, 0.5, color);
			//Bar
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_RED, 10, 87.5, 300, 5);
			Draw_texture(var_square_image[0], color, (var_lcd_brightness - 10) * 2, 80, 4, 20);

			//Time to turn off LCDs
			Draw(sem_msg[16] + std::to_string(var_time_to_turn_off_lcd) + sem_msg[17], 0, 105, 0.5, 0.5, color);
			//Bar
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_RED, 10, 127.5, 300, 5);
			Draw_texture(var_square_image[0], color, (var_time_to_turn_off_lcd), 120, 4, 20);

			//Screen mode
			Draw(sem_msg[54], 0, 145, 0.5, 0.5, color);
			//800px
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 160, 90, 20);
			Draw(sem_msg[56], 10, 160, 0.65, 0.65, cache_color[3]);
			//3D
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 110, 160, 90, 20);
			Draw(sem_msg[57], 110, 160, 0.65, 0.65, cache_color[4]);
			//Nothing
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 210, 160, 90, 20);
			Draw(sem_msg[58], 210, 160, 0.65, 0.65, cache_color[5]);
		}
		else if (sem_selected_menu_mode == 4)
		{
			//Scroll speed
			Draw(sem_msg[18] + std::to_string(var_scroll_speed), 0, 25, 0.5, 0.5, color);
			//Bar
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_RED, 10, 47.5, 300, 5);
			Draw_texture(var_square_image[0], color, (var_scroll_speed * 300), 40, 4, 20);
		}
		else if (sem_selected_menu_mode == 5)
		{
			//Font
			if (30 + sem_y_offset >= -70 && 30 + sem_y_offset <= 240)
			{
				for (int i = 0; i < 4; i++)
				{
					if (Exfont_is_loaded_system_font(i) || i == var_system_region)
					{
						if (Exfont_is_loading_system_font() || Exfont_is_unloading_system_font())
							cache_color[i] = DEF_DRAW_WEAK_RED;
						else
							cache_color[i] = DEF_DRAW_RED;
					}
					else if ((Exfont_is_loading_system_font() || Exfont_is_unloading_system_font()) && var_night_mode)
						cache_color[i] = DEF_DRAW_WEAK_WHITE;
					else if (Exfont_is_loading_system_font() || Exfont_is_unloading_system_font())
						cache_color[i] = DEF_DRAW_WEAK_BLACK;
				}

				//JPN
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 30 + sem_y_offset, 200, 20);
				Draw(sem_msg[19], 10, 30 + sem_y_offset, 0.75, 0.75, cache_color[0]);

				//CHN
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 50 + sem_y_offset, 200, 20);
				Draw(sem_msg[20], 10, 50 + sem_y_offset, 0.75, 0.75, cache_color[1]);

				//KOR
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 70 + sem_y_offset, 200, 20);
				Draw(sem_msg[21], 10, 70 + sem_y_offset, 0.75, 0.75, cache_color[2]);

				//TWN
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 90 + sem_y_offset, 200, 20);
				Draw(sem_msg[22], 10, 90 + sem_y_offset, 0.75, 0.75, cache_color[3]);
			}

			if (130 + sem_y_offset >= -30 && 130 + sem_y_offset <= 240)
			{
				cache_color[0] = color;
				if ((Exfont_is_unloading_external_font() || Exfont_is_loading_external_font()) && var_night_mode)
					cache_color[0] = DEF_DRAW_WEAK_WHITE;
				else if (Exfont_is_unloading_external_font() || Exfont_is_loading_external_font())
					cache_color[0] = DEF_DRAW_WEAK_BLACK;

				//Load all
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_RED, 10, 130 + sem_y_offset, 100, 20);
				Draw(sem_msg[23], 10, 130 + sem_y_offset, 0.65, 0.65, cache_color[0]);

				//Unload all
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_YELLOW, 110, 130 + sem_y_offset, 100, 20);
				Draw(sem_msg[24], 110, 130 + sem_y_offset, 0.65, 0.65, cache_color[0]);
			}

			draw_x = 10.0;
			draw_y = 150.0;
			for(int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
				cache_color[i] = color;

			for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
			{
				if (Exfont_is_loaded_external_font(i))
				{
					if(Exfont_is_unloading_external_font() || Exfont_is_loading_external_font())
						cache_color[i] = DEF_DRAW_WEAK_RED;
					else
						cache_color[i] = DEF_DRAW_RED;
				}
				else if ((Exfont_is_unloading_external_font() || Exfont_is_loading_external_font()) && var_night_mode)
					cache_color[i] = DEF_DRAW_WEAK_WHITE;
				else if (Exfont_is_unloading_external_font() || Exfont_is_loading_external_font())
					cache_color[i] = DEF_DRAW_WEAK_BLACK;

				if (draw_y + sem_y_offset >= -30 && draw_y + sem_y_offset <= 240)
				{
					Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, draw_x, draw_y + sem_y_offset, 200, 20);
					Draw(Exfont_query_external_font_name(i), draw_x, draw_y + sem_y_offset, 0.45, 0.45, cache_color[i]);
				}
				draw_y += 20.0;
			}
		}
		else if (sem_selected_menu_mode == 6)
		{
			if (var_wifi_enabled)
				cache_color[0] = DEF_DRAW_RED;
			else
				cache_color[1] = DEF_DRAW_RED;

			//Wifi
			Draw(sem_msg[47], 0, 25, 0.5, 0.5, color);
			//ON
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 40, 90, 20);
			Draw(sem_msg[12], 10, 40, 0.75, 0.75, cache_color[0]);
			//OFF
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 110, 40, 90, 20);
			Draw(sem_msg[13], 110, 40, 0.75, 0.75, cache_color[1]);

			//Connected SSID
			Draw(sem_msg[48] + var_connected_ssid, 0, 65, 0.4, 0.4, color);
		}
		else if (sem_selected_menu_mode == 7)
		{
			draw_x = 10.0;
			draw_y = 25.0;
			if (var_allow_send_app_info)
				cache_color[0] = DEF_DRAW_RED;
			else
				cache_color[1] = DEF_DRAW_RED;

			//Allow send app info
			Draw(sem_msg[25], 0, 25, 0.5, 0.5, color);
			//Allow
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 40, 90, 20);
			Draw(sem_msg[26], 10, 40, 0.75, 0.75, cache_color[0]);
			//Deny
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 110, 40, 90, 20);
			Draw(sem_msg[27], 110, 40, 0.75, 0.75, cache_color[1]);

			draw_x = 10.0;
			draw_y = 65.0;
			for(int i = 0; i < 2; i++)
				cache_color[i] = color;
		
			if (var_debug_mode)
				cache_color[0] = DEF_DRAW_RED;
			else
				cache_color[1] = DEF_DRAW_RED;

			//Debug mode
			Draw(sem_msg[28], 0, 65, 0.5, 0.5, color);
			//ON
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 80, 90, 20);
			Draw(sem_msg[12], 10, 80, 0.75, 0.75, cache_color[0]);
			//OFF
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 110, 80, 90, 20);
			Draw(sem_msg[13], 110, 80, 0.75, 0.75, cache_color[1]);
		}
		else if (sem_selected_menu_mode == 8)
		{
			if (var_eco_mode)
				cache_color[0] = DEF_DRAW_RED;
			else
				cache_color[1] = DEF_DRAW_RED;

			//Eco mode
			Draw(sem_msg[29], 0, 25, 0.5, 0.5, color);
			//ON
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 40, 90, 20);
			Draw(sem_msg[12], 10, 40, 0.75, 0.75, cache_color[0]);
			//OFF
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 110, 40, 90, 20);
			Draw(sem_msg[13], 110, 40, 0.75, 0.75, cache_color[1]);
		}
		else if (sem_selected_menu_mode == 9)
		{
			if(var_high_resolution_mode && var_night_mode)
				cache_color[0] = DEF_DRAW_WEAK_WHITE;
			else if(var_high_resolution_mode)
				cache_color[0] = DEF_DRAW_WEAK_BLACK;
			
			//Record both screen
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 25, 240, 20);
			if(sem_record_request)
				Draw(sem_msg[53], 10, 25, 0.6, 0.6, cache_color[0]);
			else
				Draw(sem_msg[50], 10, 25, 0.6, 0.6, cache_color[0]);

			//Record bottom screen
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 60, 240, 20);
			if(sem_record_request)
				Draw(sem_msg[53], 10, 60, 0.6, 0.6, cache_color[0]);
			else
				Draw(sem_msg[51], 10, 60, 0.6, 0.6, cache_color[0]);

			//Record top screen
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 95, 240, 20);
			if(sem_record_request)
				Draw(sem_msg[53], 10, 95, 0.6, 0.6, cache_color[0]);
			else
				Draw(sem_msg[52], 10, 95, 0.6, 0.6, cache_color[0]);

			if(var_high_resolution_mode)
				Draw(sem_msg[55], 10, 120, 0.5, 0.5, DEF_DRAW_RED);
		}

		if (sem_show_patch_note_request)
		{
			Draw_texture(var_square_image[0], DEF_DRAW_AQUA, 15, 15, 290, 200);
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 15, 200, 145, 15);
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_WHITE, 160, 200, 145, 15);

			if(sem_update_progress == 0)//checking...
				Draw(sem_msg[31], 17.5, 15, 0.5, 0.5, DEF_DRAW_BLACK);
			else if(sem_update_progress == -1)//failed
				Draw(sem_msg[32], 17.5, 15, 0.5, 0.5, DEF_DRAW_BLACK);
			else if (sem_update_progress == 1)//success
			{
				Draw(sem_msg[33 + sem_new_version_available], 17.5, 15, 0.5, 0.5, DEF_DRAW_BLACK);
				Draw(sem_newest_ver_data[5], 17.5, 35, 0.45, 0.45, DEF_DRAW_BLACK);
			}
			Draw(sem_msg[36], 17.5, 200, 0.4, 0.4, DEF_DRAW_BLACK);
			Draw(sem_msg[35], 162.5, 200, 0.4, 0.4, DEF_DRAW_BLACK);
		}
		if (sem_select_ver_request)
		{
			Draw_texture(var_square_image[0], DEF_DRAW_AQUA, 15.0, 15.0, 290.0, 200.0);
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_WHITE, 15.0, 200.0, 145.0, 15.0);
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 160.0, 200.0, 145.0, 15.0);

			//3dsx
			if(sem_selected_edition_num == 0)
				Draw(sem_msg[37], 17.5, 15, 0.8, 0.8, DEF_DRAW_RED);
			else if(sem_newest_ver_data[1] == "1")
				Draw(sem_msg[37], 17.5, 15, 0.8, 0.8, DEF_DRAW_BLACK);
			else
				Draw(sem_msg[37], 17.5, 15, 0.8, 0.8, DEF_DRAW_WEAK_BLACK);

			//cia
			if(sem_selected_edition_num == 1)
				Draw(sem_msg[38], 17.5, 45, 0.8, 0.8, DEF_DRAW_RED);
			else if(sem_newest_ver_data[2] == "1")
				Draw(sem_msg[38], 17.5, 45, 0.8, 0.8, DEF_DRAW_BLACK);
			else
				Draw(sem_msg[38], 17.5, 45, 0.8, 0.8, DEF_DRAW_WEAK_BLACK);

			if (sem_selected_edition_num == 0)
			{
				Draw(sem_msg[39], 17.5, 140, 0.5, 0.5, DEF_DRAW_BLACK);
				Draw("sdmc:" + DEF_UPDATE_DIR_PREFIX + sem_newest_ver_data[0] + "/" + DEF_UPDATE_FILE_PREFIX + ".3dsx", 17.5, 150, 0.45, 0.45, DEF_DRAW_RED);
			}

			if(sem_update_progress == 2)
			{
				//downloading...
				Draw(std::to_string(sem_dled_size / 1024.0 / 1024.0).substr(0, 4) + "MB(" + std::to_string(sem_dled_size / 1024) + "KB)", 17.5, 180, 0.4, 0.4, DEF_DRAW_BLACK);
				Draw(sem_msg[40], 17.5, 160, 0.75, 0.75, DEF_DRAW_BLACK);
			}
			else if(sem_update_progress == 3)
			{
				//installing...
				Draw(std::to_string(sem_installed_size / 1024.0 / 1024.0).substr(0, 4) + "MB/" + std::to_string(sem_total_cia_size / 1024.0 / 1024.0).substr(0, 4) + "MB", 17.5, 180, 0.4, 0.4, DEF_DRAW_BLACK);
				Draw(sem_msg[41], 17.5, 160, 0.75, 0.75, DEF_DRAW_BLACK);
			}
			else if (sem_update_progress == 4)
			{
				//success
				Draw(sem_msg[42], 17.5, 160, 0.75, 0.75, DEF_DRAW_BLACK);
				Draw(sem_msg[44], 17.5, 180, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 250, 180, 55.0, 20.0);
				Draw(sem_msg[46], 250, 180, 0.375, 0.375, DEF_DRAW_BLACK);
			}
			else if (sem_update_progress == -2)
				Draw(sem_msg[43], 17.5, 160, 0.75, 0.75, DEF_DRAW_BLACK);

			Draw(sem_msg[45], 162.5, 200, 0.4, 0.4, DEF_DRAW_BLACK);
			Draw(sem_msg[35], 17.5, 200, 0.45, 0.45, DEF_DRAW_BLACK);
		}

		if(Util_err_query_error_show_flag())
			Util_err_draw();

		Draw_bot_ui();
		Draw_touch_pos();

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();

	if (Util_err_query_error_show_flag())
		Util_err_main(key);
	else
	{
		if (key.p_touch || key.h_touch)
		{
			sem_touch_x_move_left = 0;
			sem_touch_y_move_left = 0;

			if(sem_scroll_mode)
			{
				sem_touch_x_move_left = key.touch_x_move;
				sem_touch_y_move_left = key.touch_y_move;
			}
			var_need_reflesh = true;
		}
		else
		{
			sem_scroll_mode = false;
			sem_scroll_bar_selected = false;
			sem_touch_x_move_left -= (sem_touch_x_move_left * 0.025);
			sem_touch_y_move_left -= (sem_touch_y_move_left * 0.025);
			sem_bar_selected[0] = false;
			sem_bar_selected[1] = false;
			for(int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
				sem_button_selected[i] = false;
			if (sem_touch_x_move_left < 0.5 && sem_touch_x_move_left > -0.5)
				sem_touch_x_move_left = 0;
			if (sem_touch_y_move_left < 0.5 && sem_touch_y_move_left > -0.5)
				sem_touch_y_move_left = 0;

			if(sem_touch_x_move_left != 0 || sem_touch_y_move_left != 0)
				var_need_reflesh = true;
		}

		if (key.p_start || (key.p_touch && key.touch_x >= 110 && key.touch_x <= 230 && key.touch_y >= 220 && key.touch_y <= 240))
			Sem_suspend();
		else if (sem_show_patch_note_request)
		{
			if (key.p_b || (key.p_touch && key.touch_x >= 160 && key.touch_x <= 304 && key.touch_y >= 200 && key.touch_y < 215))
			{
				sem_show_patch_note_request = false;
				var_need_reflesh = true;
			}
			if (key.p_a || (key.p_touch && key.touch_x >= 15 && key.touch_x <= 159 && key.touch_y >= 200 && key.touch_y < 215))
			{
				sem_show_patch_note_request = false;
				sem_select_ver_request = true;
				var_need_reflesh = true;
			}
		}
		else if (sem_select_ver_request && !sem_dl_file_request)
		{
			if (key.p_b || (key.p_touch && key.touch_x >= 15 && key.touch_x <= 159 && key.touch_y >= 200 && key.touch_y < 215))
			{
				sem_show_patch_note_request = true;
				sem_select_ver_request = false;
				var_need_reflesh = true;
			}
			else if ((key.p_x || (key.p_touch && key.touch_x >= 160 && key.touch_x <= 304 && key.touch_y >= 200 && key.touch_y < 215)) && sem_selected_edition_num != -1 && sem_newest_ver_data[1 + sem_selected_edition_num] == "1")
			{
				sem_dl_file_request = true;
				var_need_reflesh = true;
			}
			else if(key.p_touch && key.touch_x >= 250 && key.touch_x <= 304 && key.touch_y >= 180 && key.touch_y <= 199 && sem_update_progress == 4)
				Menu_set_must_exit_flag(true);

			if (key.p_touch && key.touch_x >= 17 && key.touch_x <= 250 && key.touch_y >= 15 && key.touch_y <= 34 && sem_newest_ver_data[1] == "1")
			{
				sem_selected_edition_num = 0;
				var_need_reflesh = true;
			}
			else if (key.p_touch && key.touch_x >= 17 && key.touch_x <= 250 && key.touch_y >= 45 && key.touch_y <= 64 && sem_newest_ver_data[2] == "1")
			{
				sem_selected_edition_num = 1;
				var_need_reflesh = true;
			}
		}
		else if(!sem_dl_file_request)
		{
			if (key.p_touch && key.touch_x >= 0 && key.touch_x <= 40 && key.touch_y >= 0 + sem_y_offset && key.touch_y <= 24 + sem_y_offset
			&& sem_selected_menu_mode >= 1 && sem_selected_menu_mode <= 9)
			{
				sem_y_offset = 0.0;
				sem_y_max = 0.0;
				sem_selected_menu_mode = 0;
				var_need_reflesh = true;
			}
			else
			{
				if(sem_selected_menu_mode == 5)//Scroll bar
				{
					if (key.h_c_down || key.h_c_up)
						sem_y_offset += (double)key.cpad_y * var_scroll_speed * 0.0625;

					if (key.h_touch && sem_scroll_bar_selected)
						sem_y_offset = ((key.touch_y - 15.0) / 195.0) * sem_y_max;

					if (key.p_touch && key.touch_x >= 305 && key.touch_x <= 320 && key.touch_y >= 15)
						sem_scroll_bar_selected = true;

					sem_y_offset -= sem_touch_y_move_left * var_scroll_speed;
				}

				if (sem_selected_menu_mode == 0)
				{
					for (int i = 0; i < 9; i++)
					{
						if (key.p_touch && key.touch_x >= 0 && key.touch_x <= 240 && key.touch_y >= 0 + (i * 25) && key.touch_y <= 19 + (i * 25))
						{
							sem_y_offset = 0.0;
							sem_selected_menu_mode = i + 1;
							if (i + 1 == 5)
								sem_y_max = -950.0;

							var_need_reflesh = true;
							break;
						}
					}
				}
				else if (sem_selected_menu_mode == 1)//Check for updates
				{
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 239 && key.touch_y >= 25 && key.touch_y <= 44)
					{
						sem_check_update_request = true;
						sem_show_patch_note_request = true;
						var_need_reflesh = true;
					}
				}
				else if (sem_selected_menu_mode == 2 && !sem_reload_msg_request)//Language
				{
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 249 && key.touch_y >= 25 && key.touch_y <= 44)
					{
						var_lang = "en";
						sem_reload_msg_request = true;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 249 && key.touch_y >= 50 && key.touch_y <= 69)
					{
						var_lang = "jp";
						sem_reload_msg_request = true;
						var_need_reflesh = true;
					}
				}
				else if (sem_selected_menu_mode == 3)//LCD
				{
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 99 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						var_night_mode = true;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 110 && key.touch_x <= 199 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						var_night_mode = false;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 210 && key.touch_x <= 249 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						var_flash_mode = !var_flash_mode;
						var_need_reflesh = true;
					}
					else if (key.h_touch && sem_bar_selected[0] && key.touch_x >= 10 && key.touch_x <= 309)
					{
						var_lcd_brightness = (key.touch_x / 2) + 10;
						var_top_lcd_brightness = var_lcd_brightness;
						var_bottom_lcd_brightness = var_lcd_brightness;
						sem_change_brightness_request = true;
						var_need_reflesh = true;
					}
					else if (key.h_touch && sem_bar_selected[1] && key.touch_x >= 10 && key.touch_x <= 309)
					{
						var_time_to_turn_off_lcd = key.touch_x;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 309 && key.touch_y >= 80 && key.touch_y <= 99)
						sem_bar_selected[0] = true;
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 309 && key.touch_y >= 120 && key.touch_y <= 139)
						sem_bar_selected[1] = true;
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 99 && key.touch_y >= 160 && key.touch_y <= 179 && !sem_record_request && var_model != "OLD 2DS")
					{
						var_high_resolution_mode = true;
						var_3d_mode = false;
						Draw_reinit(var_high_resolution_mode, var_3d_mode);
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 110 && key.touch_x <= 199 && key.touch_y >= 160 && key.touch_y <= 179 && !sem_record_request && var_model != "OLD 2DS" && var_model != "NEW 2DS XL")
					{
						var_high_resolution_mode = false;
						var_3d_mode = true;
						Draw_reinit(var_high_resolution_mode, var_3d_mode);
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 210 && key.touch_x <= 299 && key.touch_y >= 160 && key.touch_y <= 179 && !sem_record_request)
					{
						var_high_resolution_mode = false;
						var_3d_mode = false;
						Draw_reinit(var_high_resolution_mode, var_3d_mode);
						var_need_reflesh = true;
					}
				}
				else if (sem_selected_menu_mode == 4)//Scroll speed
				{
					if (key.h_touch && sem_bar_selected[0] && key.touch_x >= 10 && key.touch_x <= 309)
					{
						var_scroll_speed = (double)key.touch_x / 300;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 309 && key.touch_y >= 35 && key.touch_y <= 54)
						sem_bar_selected[0] = true;
				}
				else if (sem_selected_menu_mode == 5)//Font
				{
					sem_scroll_mode = true;
					for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
					{
						if(sem_button_selected[i])
							sem_scroll_mode = false;
					}

					if (key.p_touch && key.touch_y <= 219 && key.touch_x >= 10 && key.touch_x <= 209 && key.touch_y >= 30 + sem_y_offset && key.touch_y <= 109 + sem_y_offset && !Exfont_is_loading_system_font() && !Exfont_is_unloading_system_font())
					{
						sem_scroll_mode = false;
						for (int i = 0; i < 4; i++)
						{
							if (key.touch_y >= 30 + sem_y_offset + (i * 20) && key.touch_y <= 49 + sem_y_offset + (i * 20))
							{
								if(i != var_system_region)
								{
									sem_button_selected[i] = true;
									if (Exfont_is_loaded_system_font(i))
									{
										Exfont_set_system_font_request_state(i, false);
										Exfont_request_unload_system_font();
										var_need_reflesh = true;
									}
									else
									{
										Exfont_set_system_font_request_state(i, true);
										Exfont_request_load_system_font();
										var_need_reflesh = true;
									}
								}
								break;
							}
						}
					}
					else if (key.p_touch && key.touch_y <= 219 && key.touch_x >= 10 && key.touch_x <= 209 && key.touch_y >= 150 + sem_y_offset && key.touch_y <= 1149 + sem_y_offset && !Exfont_is_loading_external_font() && !Exfont_is_unloading_external_font())
					{
						sem_scroll_mode = false;
						for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
						{
							if (key.touch_y >= 150 + sem_y_offset + (i * 20) && key.touch_y <= 169 + sem_y_offset + (i * 20))
							{
								sem_button_selected[i] = true;
								if (Exfont_is_loaded_external_font(i))
								{
									if(i != 0)
									{
										Exfont_set_external_font_request_state(i ,false);
										Exfont_request_unload_external_font();
										var_need_reflesh = true;
									}
								}
								else
								{
									Exfont_set_external_font_request_state(i ,true);
									Exfont_request_load_external_font();
									var_need_reflesh = true;
								}
								break;
							}
						}
					}
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 109 && key.touch_y >= 130 + sem_y_offset && key.touch_y <= 149 + sem_y_offset && !Exfont_is_loading_external_font() && !Exfont_is_unloading_external_font())
					{
						sem_scroll_mode = false;
						for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
							Exfont_set_external_font_request_state(i ,true);
						
						sem_button_selected[0] = true;
						Exfont_request_load_external_font();
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 110 && key.touch_x <= 209 && key.touch_y >= 130 + sem_y_offset && key.touch_y <= 149 + sem_y_offset && !Exfont_is_loading_external_font() && !Exfont_is_unloading_external_font())
					{
						sem_scroll_mode = false;
						for (int i = 1; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
							Exfont_set_external_font_request_state(i ,false);

						sem_button_selected[0] = true;
						Exfont_request_unload_external_font();
						var_need_reflesh = true;
					}
				}
				else if (sem_selected_menu_mode == 6)//Wireless
				{
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 99 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						result = Util_cset_set_wifi_state(true);
						if(result.code == 0 || result.code == 0xC8A06C0D)
							var_wifi_enabled = true;

						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 100 && key.touch_x <= 199 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						result = Util_cset_set_wifi_state(false);
						if(result.code == 0 || result.code == 0xC8A06C0D)
							var_wifi_enabled = false;

						var_need_reflesh = true;
					}
				}
				else if (sem_selected_menu_mode == 7)//Advanced settings
				{
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 99 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						var_allow_send_app_info = true;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 100 && key.touch_x <= 199 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						var_allow_send_app_info = false;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 99 && key.touch_y >= 80 && key.touch_y <= 99)
					{
						var_debug_mode = true;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 100 && key.touch_x <= 199 && key.touch_y >= 80 && key.touch_y <= 99)
					{
						var_debug_mode = false;
						var_need_reflesh = true;
					}
				}
				else if (sem_selected_menu_mode == 8)//Battery
				{
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 99 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						var_eco_mode = true;
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 100 && key.touch_x <= 199 && key.touch_y >= 40 && key.touch_y <= 59)
					{
						var_eco_mode = false;
						var_need_reflesh = true;
					}
				}
				else if (sem_selected_menu_mode == 9)//Screen recording
				{
					if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 259 && key.touch_y >= 25 && key.touch_y <= 44 && !var_high_resolution_mode)
					{
						if(sem_record_request)
							sem_stop_record_request = true;
						else
						{
							sem_selected_recording_mode = 0;
							sem_record_request = true;
						}
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 259 && key.touch_y >= 60 && key.touch_y <= 79 && !var_high_resolution_mode)
					{
						if(sem_record_request)
							sem_stop_record_request = true;
						else
						{
							sem_selected_recording_mode = 1;
							sem_record_request = true;
						}
						var_need_reflesh = true;
					}
					else if (key.p_touch && key.touch_x >= 10 && key.touch_x <= 259 && key.touch_y >= 95 && key.touch_y <= 114 && !var_high_resolution_mode)
					{
						if(sem_record_request)
							sem_stop_record_request = true;
						else
						{
							sem_selected_recording_mode = 2;
							sem_record_request = true;
						}
						var_need_reflesh = true;
					}
				}
			}

			if (sem_y_offset >= 0)
				sem_y_offset = 0.0;
			else if (sem_y_offset <= sem_y_max)
				sem_y_offset = sem_y_max;
		}
	}

	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}

void Sem_encode_thread(void* arg)
{
	Util_log_save(DEF_SEM_ENCODE_THREAD_STR, "Thread started.");
	u8* yuv420p = NULL;
	Result_with_string result;

	while (sem_thread_run)
	{
		while(sem_record_request)
		{
			if(sem_encode_request)
			{
				sem_wait_request = true;
				sem_encode_request = false;
				yuv420p = (u8*)malloc(sem_rec_width * sem_rec_height * 1.5);
				if(yuv420p == NULL)
					sem_stop_record_request = true;
				else
				{
					memcpy(yuv420p, sem_yuv420p, sem_rec_width * sem_rec_height * 1.5);

					int log_num = Util_log_save("", "");
					result = Util_video_encoder_encode(yuv420p, 0);
					Util_log_add(log_num, "");
					if(result.code != 0)
					{
						Util_log_save(DEF_SEM_ENCODE_THREAD_STR, "Util_video_encoder_encode()..." + result.string + result.error_description, result.code);
						break;
					}
				}

				free(yuv420p);
				yuv420p = NULL;
				sem_wait_request = false;
			}
			else
				usleep(1000);
		}

		usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SEM_ENCODE_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sem_record_thread(void* arg)
{
	Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Thread started.");
	bool new_3ds = false;
	int new_width = 0;
	int new_height = 0;
	int bot_bgr_offset = 0;
	int offset = 0;
	int mode = 0;
	int rec_width = 400;
	int rec_height = 480;
	int rec_framerate = 10;
	int log_num = 0;
	u8* top_framebuffer = NULL;
	u8* bot_framebuffer = NULL;
	u8* top_bgr = NULL;
	u8* bot_bgr = NULL;
	u8* both_bgr = NULL;
	u8* yuv420p = NULL;
	u16 width = 0;
	u16 height = 0;
	double time = 0;
	Result_with_string result;
	TickCounter counter;
	APT_CheckNew3DS(&new_3ds);

	Util_file_save_to_file(".", DEF_MAIN_DIR + "screen_recording/", NULL, 0, false);//create directory
	osTickCounterStart(&counter);

	while (sem_thread_run)
	{
		if (sem_record_request)
		{
			APT_SetAppCpuTimeLimit(80);
			mode = sem_selected_recording_mode;
			if(mode == 0)
			{
				rec_width = 400;
				rec_height = 480;
				if(new_3ds)
					rec_framerate = 20;
				else
					rec_framerate = 5;
			}
			else if(mode == 1)
			{
				rec_width = 400;
				rec_height = 240;
				if(new_3ds)
					rec_framerate = 30;
				else
					rec_framerate = 10;
			}
			else if(mode == 2)
			{
				rec_width = 320;
				rec_height = 240;
				if(new_3ds)
					rec_framerate = 30;
				else
					rec_framerate = 10;
			}
			sem_rec_width = rec_width;
			sem_rec_height = rec_height;

			log_num = Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Util_encoder_reate_output_file()...");
			result = Util_encoder_create_output_file(DEF_MAIN_DIR + "screen_recording/" + std::to_string(var_years) + "_" + std::to_string(var_months) 
			+ "_" + std::to_string(var_days) + "_" + std::to_string(var_hours) + "_" + std::to_string(var_minutes) + "_" + std::to_string(var_seconds) + ".mp4", 0);
			Util_log_add(log_num, result.string + result.error_description, result.code);
			if(result.code != 0)
				sem_record_request = false;

			log_num = Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Util_video_encoder_init()...");
			result = Util_video_encoder_init(AV_CODEC_ID_MJPEG, rec_width, rec_height, rec_framerate, 0);
			Util_log_add(log_num, result.string + result.error_description, result.code);
			if(result.code != 0)
				sem_record_request = false;
			
			sem_yuv420p = (u8*)malloc(rec_width * rec_height * 1.5);
			if(sem_yuv420p == NULL)
				sem_stop_record_request = true;

			while(sem_record_request)
			{
				if(sem_stop_record_request)
					break;

				osTickCounterUpdate(&counter);
				
				if(mode == 0)
				{
					top_framebuffer = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
					result = Util_converter_bgr888_rotate_90_degree(top_framebuffer, &top_bgr, width, height, &new_width, &new_height);
					if(result.code != 0)
					{
						Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Util_converter_bgr888_rotate_90_degree()..." + result.string + result.error_description, result.code);
						break;
					}

					bot_framebuffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &width, &height);
					result = Util_converter_bgr888_rotate_90_degree(bot_framebuffer, &bot_bgr, width, height, &new_width, &new_height);
					if(result.code != 0)
					{
						Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Util_converter_bgr888_rotate_90_degree()..." + result.string + result.error_description, result.code);
						break;
					}

					both_bgr = (u8*)malloc(rec_width * rec_height * 3);
					if(both_bgr == NULL)
						break;

					memcpy(both_bgr, top_bgr, 400 * 240 * 3);
					free(top_bgr);
					top_bgr = NULL;

					offset = 400 * 240 * 3;
					bot_bgr_offset = 0;

					for(int i = 0; i < 240; i++)
					{
						memset(both_bgr + offset, 0x0, 40 * 3);
						offset += 40 * 3;
						memcpy(both_bgr + offset, bot_bgr + bot_bgr_offset, 320 * 3);
						offset += 320 * 3;
						bot_bgr_offset += 320 * 3;
						memset(both_bgr + offset, 0x0, 40 * 3);
						offset += 40 * 3;
					}
					free(bot_bgr);
					bot_bgr = NULL;
				}
				else if(mode == 1)
				{
					top_framebuffer = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, &width, &height);
					result = Util_converter_bgr888_rotate_90_degree(top_framebuffer, &both_bgr, width, height, &new_width, &new_height);
					if(result.code != 0)
					{
						Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Util_converter_bgr888_rotate_90_degree()..." + result.string + result.error_description, result.code);
						break;
					}
				}
				else if(mode == 2)
				{
					bot_framebuffer = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, &width, &height);
					result = Util_converter_bgr888_rotate_90_degree(bot_framebuffer, &both_bgr, width, height, &new_width, &new_height);
					if(result.code != 0)
					{
						Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Util_converter_bgr888_rotate_90_degree()..." + result.string + result.error_description, result.code);
						break;
					}
				}
				
				result = Util_converter_bgr888_to_yuv420p(both_bgr, &yuv420p, rec_width, rec_height);
				free(both_bgr);
				both_bgr = NULL;
				if(result.code != 0)
				{
					Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Util_converter_bgr888_to_yuv420p()..." + result.string + result.error_description, result.code);
					break;
				}
				memcpy(sem_yuv420p, yuv420p, rec_width * rec_height * 1.5);
				free(yuv420p);
				yuv420p = NULL;

				sem_encode_request = true;
				osTickCounterUpdate(&counter);
				time = osTickCounterRead(&counter);
				if(1000.0 / rec_framerate > time)
					usleep(((1000.0 / rec_framerate) - time) * 1000);
			}

			while(sem_wait_request)
				usleep(100000);

			Util_encoder_close_output_file(0);
			Util_video_encoder_exit(0);
			free(both_bgr);
			free(bot_bgr);
			free(top_bgr);
			free(yuv420p);
			free(sem_yuv420p);
			both_bgr = NULL;
			bot_bgr = NULL;
			top_bgr = NULL;
			yuv420p = NULL;
			sem_yuv420p = NULL;
			sem_record_request = false;
			sem_stop_record_request = false;
			var_need_reflesh = true;
			APT_SetAppCpuTimeLimit(30);
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SEM_RECORD_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sem_worker_thread(void* arg)
{
	Util_log_save(DEF_SEM_WORKER_THREAD_STR, "Thread started.");
	Result_with_string result;

	while (sem_thread_run)
	{
		if (sem_reload_msg_request)
		{
			result = Util_load_msg("sem_" + var_lang + ".txt", sem_msg, DEF_SEM_NUM_OF_MSG);
			Util_log_save(DEF_SEM_WORKER_THREAD_STR, "Util_load_msg()..." + result.string + result.error_description, result.code);
			sem_reload_msg_request = false;
		}
		else if(sem_change_brightness_request)
		{
			result = Util_cset_set_screen_brightness(true, true, var_lcd_brightness);
			Util_log_save(DEF_SEM_WORKER_THREAD_STR, "Util_cset_set_screen_brightness()..." + result.string + result.error_description, result.code);
			sem_change_brightness_request = false;
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SEM_WORKER_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sem_update_thread(void* arg)
{
	Util_log_save(DEF_SEM_UPDATE_THREAD_STR, "Thread started.");

	u8* buffer = NULL;
	u32 status_code = 0;
	u32 write_size = 0;
	u32 read_size = 0;
	u64 offset = 0;
	int log_num = 0;
	size_t parse_start_pos = std::string::npos;
	size_t parse_end_pos = std::string::npos;
	std::string dir_path = "";
	std::string file_name = "";
	std::string url = "";
	std::string last_url = "";
	std::string parse_cache = "";
	std::string parse_start[6] = {"<newest>", "<3dsx_available>", "<cia_available>", "<3dsx_url>", "<cia_url>", "<patch_note>", };
	std::string parse_end[6] = { "</newest>", "</3dsx_available>", "</cia_available>", "</3dsx_url>", "</cia_url>", "</patch_note>", };
	Handle am_handle = 0;
	Result_with_string result;

	while (sem_thread_run)
	{
		if (sem_check_update_request || sem_dl_file_request)
		{
			if (sem_check_update_request)
			{
				sem_update_progress = 0;
				sem_selected_edition_num = -1;
				url = DEF_CHECK_UPDATE_URL;
				sem_new_version_available = false;
				for (int i = 0; i < 6; i++)
					sem_newest_ver_data[i] = "";
			}
			else if (sem_dl_file_request)
			{
				sem_update_progress = 2;
				url = sem_newest_ver_data[3 + sem_selected_edition_num];
			}

			sem_dled_size = 0;
			offset = 0;
			sem_installed_size = 0;
			sem_total_cia_size = 0;
			buffer = (u8*)malloc(0x20000);
			if (buffer == NULL)
			{
				Util_err_set_error_message(DEF_ERR_OUT_OF_MEMORY_STR, "", DEF_SEM_UPDATE_THREAD_STR, DEF_ERR_OUT_OF_MEMORY);
				Util_err_set_error_show_flag(true);
				Util_log_save(DEF_SEM_UPDATE_THREAD_STR, DEF_ERR_OUT_OF_MEMORY_STR, DEF_ERR_OUT_OF_MEMORY);
			}
			else
			{
				log_num = Util_log_save(DEF_SEM_UPDATE_THREAD_STR, "Util_httpc_dl_data()...");
				if(sem_dl_file_request)
				{
					dir_path = DEF_UPDATE_DIR_PREFIX + sem_newest_ver_data[0] + "/";
					file_name = DEF_UPDATE_FILE_PREFIX;
					if(sem_selected_edition_num == 0)
						file_name += ".3dsx";
					else if(sem_selected_edition_num == 1)
						file_name += ".cia";

					Util_file_delete_file(file_name, dir_path);//delete old file if exist
				}

				if(sem_dl_file_request)
					result = Util_httpc_dl_data(url, buffer, 0x20000, &sem_dled_size, &status_code, true, 5, dir_path, file_name);
				else
					result = Util_httpc_dl_data(url, buffer, 0x20000, &sem_dled_size, &status_code, true, 5);

				Util_log_add(log_num, result.string, result.code);

				if (result.code != 0)
				{
					Util_err_set_error_message(result.string, result.error_description, DEF_SEM_UPDATE_THREAD_STR, result.code);
					Util_err_set_error_show_flag(true);
					if (sem_check_update_request)
						sem_update_progress = -1;
					else if (sem_dl_file_request)
						sem_update_progress = -2;
				}
				else
				{
					if (sem_check_update_request)
					{
						parse_cache = (char*)buffer;

						for (int i = 0; i < 6; i++)
						{
							parse_start_pos = parse_cache.find(parse_start[i]);
							parse_end_pos = parse_cache.find(parse_end[i]);

							parse_start_pos += parse_start[i].length();
							parse_end_pos -= parse_start_pos;
							if (parse_start_pos != std::string::npos && parse_end_pos != std::string::npos)
								sem_newest_ver_data[i] = parse_cache.substr(parse_start_pos, parse_end_pos);
							else
							{
								sem_update_progress = -1;
								break;
							}
						}

						if(sem_update_progress != -1)
						{
							if (DEF_CURRENT_APP_VER_INT < atoi(sem_newest_ver_data[0].c_str()))
								sem_new_version_available = true;
							else
								sem_new_version_available = false;
						}

						sem_update_progress = 1;
					}
					else if (sem_dl_file_request)
					{
						sem_update_progress = 3;
						if (sem_selected_edition_num == 0)
							sem_update_progress = 4;

						if (sem_selected_edition_num > 0 && sem_selected_edition_num < 8)
						{
							sem_total_cia_size = sem_dled_size;
							log_num = Util_log_save(DEF_SEM_UPDATE_THREAD_STR, "AM_StartCiaInstall()...");
							result.code = AM_StartCiaInstall(MEDIATYPE_SD, &am_handle);
							Util_log_add(log_num, "", result.code);

							while (true)
							{
								log_num = Util_log_save(DEF_SEM_UPDATE_THREAD_STR, "Util_file_load_from_file_with_range()...");
								result = Util_file_load_from_file_with_range(file_name, dir_path, (u8*)buffer, 0x20000, offset, &read_size);
								Util_log_add(log_num, result.string, result.code);
								if(result.code != 0 || read_size <= 0)
									break;

								log_num = Util_log_save(DEF_SEM_UPDATE_THREAD_STR, "FSFILE_Write()...");
								result.code = FSFILE_Write(am_handle, &write_size, offset, (u8*)buffer, read_size, FS_WRITE_FLUSH);
								Util_log_add(log_num, "", result.code);
								if(result.code != 0)
									break;

								offset += write_size;
								sem_installed_size += write_size;
								var_need_reflesh = true;
							}

							log_num = Util_log_save(DEF_SEM_UPDATE_THREAD_STR, "AM_FinishCiaInstall()...");
							result.code = AM_FinishCiaInstall(am_handle);
							Util_log_add(log_num, "", result.code);
							if (result.code == 0)
								sem_update_progress = 4;
							else
								sem_update_progress = -2;
						}
					}
				}
			}

			free(buffer);
			buffer = NULL;
			if(sem_check_update_request)
				sem_check_update_request = false;
			else if(sem_dl_file_request)
				sem_dl_file_request = false;
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (sem_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}
	Util_log_save(DEF_SEM_UPDATE_THREAD_STR, "Thread exit.");
	threadExit(0);
}
