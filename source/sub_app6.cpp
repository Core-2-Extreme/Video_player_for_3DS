#include "system/headers.hpp"

bool sapp6_main_run = false;
bool sapp6_thread_run = false;
bool sapp6_already_init = false;
bool sapp6_thread_suspend = true;
std::string sapp6_msg[DEF_SAPP6_NUM_OF_MSG];
std::string sapp6_status = "";
Thread sapp6_init_thread, sapp6_exit_thread, sapp6_worker_thread, sapp6_hid_thread;

void Sapp6_suspend(void);

bool Sapp6_query_init_flag(void)
{
	return sapp6_already_init;
}

bool Sapp6_query_running_flag(void)
{
	return sapp6_main_run;
}

void Sapp6_worker_thread(void* arg)
{
	Util_log_save(DEF_SAPP6_WORKER_THREAD_STR, "Thread started.");
	
	while (sapp6_thread_run)
	{
		if(false)
		{

		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (sapp6_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SAPP6_WORKER_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp6_hid_thread(void* arg)
{
	Util_log_save(DEF_SAPP6_HID_THREAD_STR, "Thread started.");
	Hid_info key;

	while (sapp6_thread_run)
	{
		Util_hid_query_key_state(&key);
		if (sapp6_main_run && var_previous_ts != key.ts)
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
					Sapp6_suspend();
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

	Util_log_save(DEF_SAPP6_HID_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp6_init_thread(void* arg)
{
	Util_log_save(DEF_SAPP6_INIT_STR, "Thread started.");
	Result_with_string result;
	
	sapp6_status = "Starting threads...";
	var_need_reflesh = true;

	sapp6_thread_run = true;
	sapp6_worker_thread = threadCreate(Sapp6_worker_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	sapp6_hid_thread = threadCreate(Sapp6_hid_thread, (void*)(""), 1024 * 4, DEF_THREAD_PRIORITY_REALTIME, 0, false);

	sapp6_already_init = true;

	Util_log_save(DEF_SAPP6_INIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp6_exit_thread(void* arg)
{
	Util_log_save(DEF_SAPP6_EXIT_STR, "Thread started.");

	sapp6_thread_suspend = false;
	sapp6_thread_run = false;

	sapp6_status = "Exiting threads...";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP6_EXIT_STR, "threadJoin()...", threadJoin(sapp6_init_thread, DEF_THREAD_WAIT_TIME));	

	sapp6_status += ".";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP6_EXIT_STR, "threadJoin()...", threadJoin(sapp6_worker_thread, DEF_THREAD_WAIT_TIME));

	sapp6_status += ".";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP6_EXIT_STR, "threadJoin()...", threadJoin(sapp6_hid_thread, DEF_THREAD_WAIT_TIME));

	sapp6_status = "Cleaning up...";
	var_need_reflesh = true;

	threadFree(sapp6_init_thread);
	threadFree(sapp6_worker_thread);
	threadFree(sapp6_hid_thread);

	sapp6_already_init = false;

	Util_log_save(DEF_SAPP6_EXIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp6_resume(void)
{
	sapp6_thread_suspend = false;
	sapp6_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Sapp6_suspend(void)
{
	sapp6_thread_suspend = true;
	sapp6_main_run = false;
	var_need_reflesh = true;
	Menu_resume();
}

Result_with_string Sapp6_load_msg(std::string lang)
{
	return Util_load_msg("sapp6_" + lang + ".txt", sapp6_msg, DEF_SAPP6_NUM_OF_MSG);
}

void Sapp6_init(void)
{
	Util_log_save(DEF_SAPP6_INIT_STR, "Initializing...");
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	sapp6_status = "";
	var_need_reflesh = true;

	if((var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) && var_core_2_available)
		sapp6_init_thread = threadCreate(Sapp6_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	else
	{
		APT_SetAppCpuTimeLimit(80);
		sapp6_init_thread = threadCreate(Sapp6_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	}

	while(!sapp6_already_init)
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
			Draw(sapp6_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	if(!(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) || !var_core_2_available)
		APT_SetAppCpuTimeLimit(10);

	Sapp6_resume();

	Util_log_save(DEF_SAPP6_INIT_STR, "Initialized.");
}

void Sapp6_exit(void)
{
	Util_log_save(DEF_SAPP6_EXIT_STR, "Exiting...");

	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;
	u64 time_out = 10000000000;

	sapp6_status = "";
	var_need_reflesh = true;

	sapp6_exit_thread = threadCreate(Sapp6_exit_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	while(sapp6_already_init)
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
			Draw(sapp6_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	Util_log_save(DEF_SAPP6_EXIT_STR, "threadJoin()...", threadJoin(sapp6_exit_thread, time_out));	
	threadFree(sapp6_exit_thread);
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP6_EXIT_STR, "Exited.");
}

void Sapp6_main(void)
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

			Draw(sapp6_msg[0], 0, 20, 0.5, 0.5, color);
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

			Draw(DEF_SAPP6_VER, 0, 0, 0.4, 0.4, DEF_DRAW_GREEN);

			if(Util_err_query_error_show_flag())
				Util_err_draw();

			Draw_bot_ui();
		}

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();
}
