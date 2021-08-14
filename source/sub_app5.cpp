#include "headers.hpp"

#include "sub_app5.hpp"

bool sapp5_main_run = false;
bool sapp5_thread_run = false;
bool sapp5_already_init = false;
bool sapp5_thread_suspend = true;
std::string sapp5_msg[DEF_SAPP5_NUM_OF_MSG];
Thread sapp5_thread;

bool Sapp5_query_init_flag(void)
{
	return sapp5_already_init;
}

bool Sapp5_query_running_flag(void)
{
	return sapp5_main_run;
}

void Sapp5_thread(void* arg)
{
	Util_log_save(DEF_SAPP5_THREAD_STR, "Thread started.");
	
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

	Util_log_save(DEF_SAPP5_THREAD_STR, "Thread exit.");
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
	Menu_resume();
}

void Sapp5_init(void)
{
	Util_log_save(DEF_SAPP5_INIT_STR, "Initializing...");
	Result_with_string result;

	sapp5_thread_run = true;
	sapp5_thread = threadCreate(Sapp5_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	result = Util_load_msg("sapp5_" + var_lang + ".txt", sapp5_msg, DEF_SAPP5_NUM_OF_MSG);
	Util_log_save(DEF_SAPP5_MAIN_STR, "Util_load_msg()..." + result.string + result.error_description, result.code);

	Sapp5_resume();
	sapp5_already_init = true;
	Util_log_save(DEF_SAPP5_INIT_STR, "Initialized.");
}

void Sapp5_exit(void)
{
	Util_log_save(DEF_SAPP5_EXIT_STR, "Exiting...");
	u64 time_out = 10000000000;
	Result_with_string result;

	sapp5_already_init = false;
	sapp5_thread_suspend = false;
	sapp5_thread_run = false;
	
	Util_log_save(DEF_SAPP5_EXIT_STR, "threadJoin()...", threadJoin(sapp5_thread, time_out));
	threadFree(sapp5_thread);

	Util_log_save(DEF_SAPP5_EXIT_STR, "Exited.");
}

void Sapp5_main(void)
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
		Draw_screen_ready(0, back_color);

		Draw(sapp5_msg[0], 0, 20, 0.5, 0.5, color);
		if(Util_log_query_log_show_flag())
			Util_log_draw();

		Draw_top_ui();

		Draw_screen_ready(1, back_color);

		Draw(DEF_SAPP5_VER, 0, 0, 0.4, 0.4, DEF_DRAW_GREEN);

		if(Util_err_query_error_show_flag())
			Util_err_draw();

		Draw_bot_ui();

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();

	if(Util_err_query_error_show_flag())
		Util_err_main(key);
	else
	{
		if (key.p_start || (key.p_touch && key.touch_x >= 110 && key.touch_x <= 230 && key.touch_y >= 220 && key.touch_y <= 240))
			Sapp5_suspend();
	}

	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}
