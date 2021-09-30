#include "system/headers.hpp"

bool sapp2_main_run = false;
bool sapp2_thread_run = false;
bool sapp2_already_init = false;
bool sapp2_thread_suspend = true;
std::string sapp2_msg[DEF_SAPP2_NUM_OF_MSG];
std::string sapp2_status = "";
Thread sapp2_init_thread, sapp2_exit_thread, sapp2_worker_thread;

bool Sapp2_query_init_flag(void)
{
	return sapp2_already_init;
}

bool Sapp2_query_running_flag(void)
{
	return sapp2_main_run;
}

void Sapp2_worker_thread(void* arg)
{
	Util_log_save(DEF_SAPP2_WORKER_THREAD_STR, "Thread started.");
	
	while (sapp2_thread_run)
	{
		if(false)
		{

		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (sapp2_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SAPP2_WORKER_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp2_init_thread(void* arg)
{
	Util_log_save(DEF_SAPP2_INIT_STR, "Thread started.");
	Result_with_string result;
	
	sapp2_status = "Starting threads...";
	var_need_reflesh = true;

	sapp2_thread_run = true;
	sapp2_worker_thread = threadCreate(Sapp2_worker_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	sapp2_already_init = true;

	Util_log_save(DEF_SAPP2_INIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp2_exit_thread(void* arg)
{
	Util_log_save(DEF_SAPP2_EXIT_STR, "Thread started.");

	sapp2_thread_suspend = false;
	sapp2_thread_run = false;

	sapp2_status = "Exiting threads...";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP2_EXIT_STR, "threadJoin()...", threadJoin(sapp2_init_thread, DEF_THREAD_WAIT_TIME));	

	sapp2_status += ".";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP2_EXIT_STR, "threadJoin()...", threadJoin(sapp2_worker_thread, DEF_THREAD_WAIT_TIME));

	sapp2_status = "Cleaning up...";
	var_need_reflesh = true;

	threadFree(sapp2_init_thread);
	threadFree(sapp2_worker_thread);

	sapp2_already_init = false;

	Util_log_save(DEF_SAPP2_EXIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp2_resume(void)
{
	sapp2_thread_suspend = false;
	sapp2_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Sapp2_suspend(void)
{
	sapp2_thread_suspend = true;
	sapp2_main_run = false;
	var_need_reflesh = true;
	Menu_resume();
}

Result_with_string Sapp2_load_msg(std::string lang)
{
	return Util_load_msg("sapp2_" + lang + ".txt", sapp2_msg, DEF_SAPP2_NUM_OF_MSG);
}

void Sapp2_init(void)
{
	Util_log_save(DEF_SAPP2_INIT_STR, "Initializing...");
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	sapp2_status = "";
	var_need_reflesh = true;

	if((var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) && var_core_2_available)
		sapp2_init_thread = threadCreate(Sapp2_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	else
	{
		APT_SetAppCpuTimeLimit(80);
		sapp2_init_thread = threadCreate(Sapp2_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	}

	while(!sapp2_already_init)
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
			Draw(sapp2_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	if(!(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) || !var_core_2_available)
		APT_SetAppCpuTimeLimit(10);

	Sapp2_resume();

	Util_log_save(DEF_SAPP2_INIT_STR, "Initialized.");
}

void Sapp2_exit(void)
{
	Util_log_save(DEF_SAPP2_EXIT_STR, "Exiting...");

	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;
	u64 time_out = 10000000000;

	sapp2_status = "";
	var_need_reflesh = true;

	sapp2_exit_thread = threadCreate(Sapp2_exit_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	while(sapp2_already_init)
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
			Draw(sapp2_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	Util_log_save(DEF_SAPP2_EXIT_STR, "threadJoin()...", threadJoin(sapp2_exit_thread, time_out));	
	threadFree(sapp2_exit_thread);
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP2_EXIT_STR, "Exited.");
}

void Sapp2_main(void)
{
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;
	Hid_info key;
	Util_hid_query_key_state(&key);
	Util_hid_key_flag_reset();

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

			Draw(sapp2_msg[0], 0, 20, 0.5, 0.5, color);
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

			Draw(DEF_SAPP2_VER, 0, 0, 0.4, 0.4, DEF_DRAW_GREEN);

			if(Util_err_query_error_show_flag())
				Util_err_draw();

			Draw_bot_ui();
		}

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();

	if(Util_err_query_error_show_flag())
		Util_err_main(key);
	else
	{
		if (key.p_start || (key.p_touch && key.touch_x >= 110 && key.touch_x <= 230 && key.touch_y >= 220 && key.touch_y <= 240))
			Sapp2_suspend();
	}

	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}
