#include "system/headers.hpp"

bool sapp5_main_run = false;
bool sapp5_thread_run = false;
bool sapp5_already_init = false;
bool sapp5_thread_suspend = true;
std::string sapp5_msg[DEF_SAPP5_NUM_OF_MSG];
std::string sapp5_status = "";
Thread sapp5_init_thread, sapp5_exit_thread, sapp5_worker_thread, sapp5_hid_thread;

void Sapp5_suspend(void);

bool Sapp5_query_init_flag(void)
{
	return sapp5_already_init;
}

bool Sapp5_query_running_flag(void)
{
	return sapp5_main_run;
}

void Sapp5_worker_thread(void* arg)
{
	Util_log_save(DEF_SAPP5_WORKER_THREAD_STR, "Thread started.");
	
	while (sapp5_thread_run)
	{
		if(false)
		{

		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (sapp5_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SAPP5_WORKER_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp5_hid_thread(void* arg)
{
	Util_log_save(DEF_SAPP5_HID_THREAD_STR, "Thread started.");
	Hid_info key;

	while (sapp5_thread_run)
	{
		Util_hid_query_key_state(&key);
		if (sapp5_main_run && var_previous_ts != key.ts)
		{
			if(Util_err_query_error_show_flag())
				Util_err_main(key);
			else
			{
				if(Util_hid_is_pressed(key, *Draw_get_bot_ui_button()))
				{
					Draw_get_bot_ui_button()->selected = true;
					var_need_reflesh = true;
				}
				else if (key.p_start || (Util_hid_is_released(key, *Draw_get_bot_ui_button()) && Draw_get_bot_ui_button()->selected))
					Sapp5_suspend();
			}

			if(!key.p_touch && !key.h_touch)
			{
				if(Draw_get_bot_ui_button()->selected)
					var_need_reflesh = true;

				Draw_get_bot_ui_button()->selected = false;
			}

			if(Util_log_query_log_show_flag())
				Util_log_main(key);

			var_previous_ts = key.ts;
		}
		else
			usleep(12000);
	}

	Util_log_save(DEF_SAPP5_HID_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp5_init_thread(void* arg)
{
	Util_log_save(DEF_SAPP5_INIT_STR, "Thread started.");
	Result_with_string result;
	
	sapp5_status = "Starting threads...";
	var_need_reflesh = true;

	sapp5_thread_run = true;
	sapp5_worker_thread = threadCreate(Sapp5_worker_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	sapp5_hid_thread = threadCreate(Sapp5_hid_thread, (void*)(""), 1024 * 4, DEF_THREAD_PRIORITY_REALTIME, 0, false);

	sapp5_already_init = true;

	Util_log_save(DEF_SAPP5_INIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp5_exit_thread(void* arg)
{
	Util_log_save(DEF_SAPP5_EXIT_STR, "Thread started.");

	sapp5_thread_suspend = false;
	sapp5_thread_run = false;

	sapp5_status = "Exiting threads...";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP5_EXIT_STR, "threadJoin()...", threadJoin(sapp5_init_thread, DEF_THREAD_WAIT_TIME));	

	sapp5_status += ".";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP5_EXIT_STR, "threadJoin()...", threadJoin(sapp5_worker_thread, DEF_THREAD_WAIT_TIME));

	sapp5_status += ".";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP5_EXIT_STR, "threadJoin()...", threadJoin(sapp5_hid_thread, DEF_THREAD_WAIT_TIME));

	sapp5_status = "Cleaning up...";
	var_need_reflesh = true;

	threadFree(sapp5_init_thread);
	threadFree(sapp5_worker_thread);
	threadFree(sapp5_hid_thread);

	sapp5_already_init = false;

	Util_log_save(DEF_SAPP5_EXIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp5_resume(void)
{
	sapp5_thread_suspend = false;
	sapp5_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Sapp5_suspend(void)
{
	sapp5_thread_suspend = true;
	sapp5_main_run = false;
	var_need_reflesh = true;
	Menu_resume();
}

Result_with_string Sapp5_load_msg(std::string lang)
{
	return Util_load_msg("sapp5_" + lang + ".txt", sapp5_msg, DEF_SAPP5_NUM_OF_MSG);
}

void Sapp5_init(void)
{
	Util_log_save(DEF_SAPP5_INIT_STR, "Initializing...");
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	sapp5_status = "";
	var_need_reflesh = true;

	if((var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) && var_core_2_available)
		sapp5_init_thread = threadCreate(Sapp5_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	else
	{
		APT_SetAppCpuTimeLimit(80);
		sapp5_init_thread = threadCreate(Sapp5_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	}

	while(!sapp5_already_init)
	{
		if (var_night_mode)
		{
			color = DEF_DRAW_WHITE;
			back_color = DEF_DRAW_BLACK;
		}

		if(var_need_reflesh || !var_eco_mode)
		{
			var_need_reflesh = false;
			Draw_frame_ready();
			Draw_screen_ready(0, back_color);
			Draw_top_ui();
			Draw(sapp5_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	if(!(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) || !var_core_2_available)
		APT_SetAppCpuTimeLimit(10);

	Sapp5_resume();

	Util_log_save(DEF_SAPP5_INIT_STR, "Initialized.");
}

void Sapp5_exit(void)
{
	Util_log_save(DEF_SAPP5_EXIT_STR, "Exiting...");

	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;
	u64 time_out = 10000000000;

	sapp5_status = "";
	var_need_reflesh = true;

	sapp5_exit_thread = threadCreate(Sapp5_exit_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	while(sapp5_already_init)
	{
		if (var_night_mode)
		{
			color = DEF_DRAW_WHITE;
			back_color = DEF_DRAW_BLACK;
		}

		if(var_need_reflesh || !var_eco_mode)
		{
			var_need_reflesh = false;
			Draw_frame_ready();
			Draw_screen_ready(0, back_color);
			Draw_top_ui();
			Draw(sapp5_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	Util_log_save(DEF_SAPP5_EXIT_STR, "threadJoin()...", threadJoin(sapp5_exit_thread, time_out));	
	threadFree(sapp5_exit_thread);
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP5_EXIT_STR, "Exited.");
}

void Sapp5_main(void)
{
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	if (var_night_mode)
	{
		color = DEF_DRAW_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	if(var_need_reflesh || !var_eco_mode)
	{
		var_need_reflesh = false;
		Draw_frame_ready();

		if(var_turn_on_top_lcd)
		{
			Draw_screen_ready(0, back_color);

			Draw(sapp5_msg[0], 0, 20, 0.5, 0.5, color);
			if(Util_log_query_log_show_flag())
				Util_log_draw();

			Draw_top_ui();

			if(var_3d_mode)
			{
				Draw_screen_ready(2, back_color);

				if(Util_log_query_log_show_flag())
					Util_log_draw();

				Draw_top_ui();
			}
		}
		
		if(var_turn_on_bottom_lcd)
		{
			Draw_screen_ready(1, back_color);

			Draw(DEF_SAPP5_VER, 0, 0, 0.4, 0.4, DEF_DRAW_GREEN);

			if(Util_err_query_error_show_flag())
				Util_err_draw();

			Draw_bot_ui();
		}

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();
}
