#include "system/headers.hpp"

bool util_err_show_flag = false;
std::string util_err_summary = "N/A";
std::string util_err_description = "N/A";
std::string util_err_place = "N/A";
std::string util_err_code = "N/A";

bool Util_err_query_error_show_flag(void)
{
	return util_err_show_flag;
}

void Util_err_set_error_message(std::string summary, std::string description, std::string place)
{
	Util_err_set_error_message(summary, description, place, 1234567890);
}

void Util_err_set_error_message(std::string summary, std::string description, std::string place, int error_code)
{
	char cache[128];
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
	util_err_show_flag = flag;
}

void Util_err_clear_error_message(void)
{
	util_err_summary = "N/A";
	util_err_description = "N/A";
	util_err_place = "N/A";
	util_err_code = "N/A";
}

void Util_err_save_error(void)
{
	Util_file_save_to_file(std::to_string(var_years) + std::to_string(var_months) + std::to_string(var_days)
	+ std::to_string(var_minutes) + std::to_string(var_seconds) + ".txt", DEF_MAIN_DIR + "error/"
	, (u8*)(util_err_summary + "\n" + util_err_description + "\n" + util_err_place + "\n" + util_err_code).c_str()
	, (util_err_summary + "\n" + util_err_description + "\n" + util_err_place + "\n" + util_err_code).length(), true);
	Util_err_set_error_show_flag(false);
}

void Util_err_main(Hid_info key)
{
	if (key.p_a || (key.p_touch && key.touch_x >= 150 && key.touch_x <= 169 && key.touch_y >= 150 && key.touch_y <= 169))
	{
		util_err_show_flag = false;
		var_need_reflesh = true;
	}
	else if(key.p_x || (key.p_touch && key.touch_x >= 200 && key.touch_x <= 239 && key.touch_y >= 150 && key.touch_y <= 169))
	{	
		Util_err_save_error();
		var_need_reflesh = true;
	}
}

void Util_err_draw(void)
{
	Draw_texture(var_square_image[0], DEF_DRAW_AQUA, 20.0, 30.0, 280.0, 150.0);
	Draw_texture(var_square_image[0], DEF_DRAW_WEAK_YELLOW, 150.0, 150.0, 20.0, 20.0);
	Draw_texture(var_square_image[0], DEF_DRAW_WEAK_YELLOW, 200.0, 150.0, 40.0, 20.0);

	Draw("Summary : ", 22.5, 40.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_summary, 22.5, 50.0, 0.45, 0.45, DEF_DRAW_BLACK);
	Draw("Description : ", 22.5, 60.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_description, 22.5, 70.0, 0.4, 0.4, DEF_DRAW_BLACK);
	Draw("Place : ", 22.5, 90.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_place, 22.5, 100.0, 0.45, 0.45, DEF_DRAW_BLACK);
	Draw("Error code : ", 22.5, 110.0, 0.45, 0.45, DEF_DRAW_RED);
	Draw(util_err_code, 22.5, 120.0, 0.45, 0.45, DEF_DRAW_BLACK);
	Draw("OK(A)", 152.5, 152.5, 0.375, 0.375, DEF_DRAW_BLACK);
	Draw("SAVE(X)", 202.5, 152.5, 0.375, 0.375, DEF_DRAW_BLACK);
}