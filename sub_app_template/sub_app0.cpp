#include "system/headers.hpp"

bool sapp0_main_run = false;
bool sapp0_thread_run = false;
bool sapp0_already_init = false;
bool sapp0_thread_suspend = true;
std::string sapp0_msg[DEF_SAPP0_NUM_OF_MSG];
std::string sapp0_status = "";
Thread sapp0_init_thread, sapp0_exit_thread, sapp0_worker_thread;

bool Sapp0_query_init_flag(void)
{
	return sapp0_already_init;
}

bool Sapp0_query_running_flag(void)
{
	return sapp0_main_run;
}

void Sapp0_worker_thread(void* arg)
{
	Util_log_save(DEF_SAPP0_WORKER_THREAD_STR, "Thread started.");
	
	while (sapp0_thread_run)
	{
		if(false)
		{

		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (sapp0_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SAPP0_WORKER_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp0_init_thread(void* arg)
{
	Util_log_save(DEF_SAPP0_INIT_STR, "Thread started.");
	Result_with_string result;
	
	sapp0_status = "Starting threads...";
	var_need_reflesh = true;

	sapp0_thread_run = true;
	sapp0_worker_thread = threadCreate(Sapp0_worker_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	sapp0_already_init = true;

	Util_log_save(DEF_SAPP0_INIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp0_exit_thread(void* arg)
{
	Util_log_save(DEF_SAPP0_EXIT_STR, "Thread started.");

	sapp0_thread_suspend = false;
	sapp0_thread_run = false;

	sapp0_status = "Exiting threads...";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(sapp0_init_thread, DEF_THREAD_WAIT_TIME));	

	sapp0_status += ".";
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(sapp0_worker_thread, DEF_THREAD_WAIT_TIME));

	sapp0_status = "Cleaning up...";
	var_need_reflesh = true;
	
	threadFree(sapp0_init_thread);
	threadFree(sapp0_worker_thread);

	sapp0_already_init = false;

	Util_log_save(DEF_SAPP0_EXIT_STR, "Thread exit.");
	threadExit(0);
}

void Sapp0_resume(void)
{
	sapp0_thread_suspend = false;
	sapp0_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Sapp0_suspend(void)
{
	sapp0_thread_suspend = true;
	sapp0_main_run = false;
	var_need_reflesh = true;
	Menu_resume();
}

Result_with_string Sapp0_load_msg(void)
{
	return Util_load_msg("sapp0_" + var_lang + ".txt", sapp0_msg, DEF_SAPP0_NUM_OF_MSG);
}

void Sapp0_init(void)
{
	Util_log_save(DEF_SAPP0_INIT_STR, "Initializing...");
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	sapp0_status = "";
	var_need_reflesh = true;

	if((var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) && var_core_2_available)
		sapp0_init_thread = threadCreate(Sapp0_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	else
	{
		APT_SetAppCpuTimeLimit(80);
		sapp0_init_thread = threadCreate(Sapp0_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	}

	while(!sapp0_already_init)
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
			Draw(sapp0_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	if(!(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_3DSXL) || !var_core_2_available)
		APT_SetAppCpuTimeLimit(10);

	Sapp0_resume();

	Util_log_save(DEF_SAPP0_INIT_STR, "Initialized.");
}

void Sapp0_exit(void)
{
	Util_log_save(DEF_SAPP0_EXIT_STR, "Exiting...");

	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;
	u64 time_out = 10000000000;

	sapp0_status = "";
	var_need_reflesh = true;

	sapp0_exit_thread = threadCreate(Sapp0_exit_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	while(sapp0_already_init)
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
			Draw(sapp0_status, 0, 20, 0.65, 0.65, color);

			Draw_apply_draw();
		}
		else
			gspWaitForVBlank();
	}

	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(sapp0_exit_thread, time_out));	
	threadFree(sapp0_exit_thread);
	var_need_reflesh = true;

	Util_log_save(DEF_SAPP0_EXIT_STR, "Exited.");
}

void Sapp0_main(void)
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

			Draw(sapp0_msg[0], 0, 20, 0.5, 0.5, color);
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

			Draw(DEF_SAPP0_VER, 0, 0, 0.4, 0.4, DEF_DRAW_GREEN);

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
			Sapp0_suspend();
	}

	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}
