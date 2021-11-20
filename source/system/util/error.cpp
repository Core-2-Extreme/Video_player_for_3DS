#include "system/headers.hpp"

bool util_err_show_flag = false;
bool util_err_thread_run = false;
bool util_err_save_request = false;
bool util_err_init = false;
std::string util_err_summary = "N/A";
std::string util_err_description = "N/A";
std::string util_err_place = "N/A";
std::string util_err_code = "N/A";
Thread util_err_save_thread;
Image_data util_err_ok_button, util_err_save_button;

void Util_err_save_thread(void* args);

Result_with_string Util_err_init(void)
{
	Result_with_string result;
	if(util_err_init)
		goto already_inited;
	
	util_err_ok_button.c2d = var_square_image[0];
	util_err_save_button.c2d = var_square_image[0];
	util_err_thread_run = true;
	util_err_save_thread = threadCreate(Util_err_save_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
	if(!util_err_save_thread)
	{
		result.code = DEF_ERR_OTHER;
		result.error_description = "[Error] threadCreate() failed. ";
		goto nintendo_api_failed;
	}

	util_err_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_err_exit(void)
{
	if(!util_err_init)
		return;
	
	util_err_init = false;
	util_err_thread_run = false;
	threadJoin(util_err_save_thread, DEF_THREAD_WAIT_TIME);
	threadFree(util_err_save_thread);
}

bool Util_err_query_error_show_flag(void)
{
	if(!util_err_init)
		return false;
	
	return util_err_show_flag;
}

void Util_err_set_error_message(std::string summary, std::string description, std::string place)
{
	Util_err_set_error_message(summary, description, place, 1234567890);
}

void Util_err_set_error_message(std::string summary, std::string description, std::string place, int error_code)
{
	if(!util_err_init)
		return;

	char cache[32];
	memset(cache, 0x0, 32);
	Util_err_clear_error_message();
	util_err_summary = summary;
	util_err_description = description;
	util_err_place = place;
	if (error_code == 1234567890)
		util_err_code = "N/A";
	else
	{
		sprintf(cache, "0x%x", error_code);
		util_err_code = cache;
	}
}

void Util_err_set_error_show_flag(bool flag)
{
	if(!util_err_init)
		return;
	
	util_err_show_flag = flag;
}

void Util_err_clear_error_message(void)
{
	if(!util_err_init)
		return;

	util_err_summary = "N/A";
	util_err_description = "N/A";
	util_err_place = "N/A";
	util_err_code = "N/A";
}

void Util_err_save_error(void)
{
	if(!util_err_init)
		return;

	util_err_save_request = true;
}

void Util_err_main(Hid_info key)
{
	if(!util_err_init)
	{
		if (key.p_a)
		{
			util_err_show_flag = false;
			var_need_reflesh = true;
		}
		return;
	}

	if(Util_hid_is_pressed(key, util_err_ok_button) && !util_err_save_request)
	{
		util_err_ok_button.selected = true;
		var_need_reflesh = true;
	}
	else if ((key.p_a || (Util_hid_is_released(key, util_err_ok_button) && util_err_ok_button.selected)) && !util_err_save_request)
	{
		util_err_show_flag = false;
		var_need_reflesh = true;
	}
	else if(Util_hid_is_pressed(key, util_err_save_button) && !util_err_save_request)
	{
		util_err_save_button.selected = true;
		var_need_reflesh = true;
	}
	else if ((key.p_x || (Util_hid_is_released(key, util_err_save_button) && util_err_save_button.selected)) && !util_err_save_request)
	{
		Util_err_save_error();
		var_need_reflesh = true;
	}

	if(!key.p_touch && !key.h_touch)
	{
		if(util_err_ok_button.selected || util_err_save_button.selected)
			var_need_reflesh = true;

		util_err_ok_button.selected = util_err_save_button.selected = false;
	}
}

void Util_err_draw(void)
{
	if(!util_err_init)
	{
		Draw_texture(var_square_image[0], DEF_DRAW_AQUA, 20.0, 30.0, 280.0, 150.0);
		Draw("Error api is not initialized.\nPress A to close.", 22.5, 40.0, 0.45, 0.45, DEF_DRAW_RED);
		return;
	}

	Draw_texture(var_square_image[0], DEF_DRAW_AQUA, 20.0, 30.0, 280.0, 150.0);
	Draw_texture(&util_err_ok_button, util_err_ok_button.selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 150.0, 150.0, 30.0, 20.0);
	Draw_texture(&util_err_save_button, util_err_save_button.selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 210.0, 150.0, 40.0, 20.0);

	Draw("Summary : ", 22.5, 40.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_summary, 22.5, 50.0, 0.45, 0.45, DEF_DRAW_BLACK);
	Draw("Description : ", 22.5, 60.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_description, 22.5, 70.0, 0.4, 0.4, DEF_DRAW_BLACK);
	Draw("Place : ", 22.5, 90.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_place, 22.5, 100.0, 0.45, 0.45, DEF_DRAW_BLACK);
	Draw("Error code : ", 22.5, 110.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_code, 22.5, 120.0, 0.45, 0.45, DEF_DRAW_BLACK);
	Draw("OK(A)", 152.5, 152.5, 0.375, 0.375, util_err_save_request ? DEF_DRAW_WEAK_BLACK : DEF_DRAW_BLACK);
	Draw("SAVE(X)", 212.5, 152.5, 0.375, 0.375, util_err_save_request ? DEF_DRAW_WEAK_BLACK : DEF_DRAW_BLACK);
}

void Util_err_save_thread(void* args)
{
	Util_log_save(DEF_ERR_SAVE_THREAD_STR, "Thread started.");
	char file_name[128];
	std::string save_data = "";
	Result_with_string result;

	while (util_err_thread_run)
	{
		if (util_err_save_request)
		{
			memset(file_name, 0x0, 128);
			sprintf(file_name, "%04d_%02d_%02d_%02d_%02d_%02d.txt", var_years, var_months, var_days, var_hours, var_minutes, var_seconds);
			save_data = util_err_summary + "\n" + util_err_description + "\n" + util_err_place + "\n" + util_err_code;

			result = Util_file_save_to_file(file_name, DEF_MAIN_DIR + "error/" , (u8*)save_data.c_str() , save_data.length(), true);
			if(result.code != 0)
				Util_log_save(DEF_ERR_SAVE_THREAD_STR, "Util_file_save_to_file()..." + result.string + result.error_description, result.code);

			Util_err_set_error_show_flag(false);
			util_err_save_request = false;
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);
	}
	Util_log_save(DEF_ERR_SAVE_THREAD_STR, "Thread exit.");
	threadExit(0);
}
