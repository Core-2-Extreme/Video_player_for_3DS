#pragma once
#include <string>

struct Result_with_string
{
	std::string string = "[Success] ";
	std::string error_description = "";
	uint code = DEF_SUCCESS;
};

struct Watch_bool
{
	bool* address = NULL;
	bool previous_value = false;
};

struct Watch_int
{
	int* address = NULL;
	int previous_value = 0;
};

struct Watch_double
{
	double* address = NULL;
	double previous_value = 0;
};

struct Watch_string
{
	std::string* address = NULL;
	std::string previous_value = "";
};

struct Audio_info
{
	int bitrate = 0;
	int sample_rate = 0;
	int ch = 0;
	std::string format_name = "";
	double duration = 0;
	std::string track_lang = "";
};

struct Video_info
{
	int width = 0;
	int height = 0;
	double framerate = 0;
	std::string format_name = "";
	double duration = 0;
	int thread_type = DEF_DECODER_THREAD_TYPE_NONE;
	int sar_width = 1;
	int sar_height = 1;
};

struct Subtitle_info
{
	std::string format_name = "";
	std::string track_lang = "";
};

struct Subtitle_data
{
	std::string text = "";
	double start_time = 0;
	double end_time = 0;
};

struct Image_data
{
	C2D_Image c2d = { .tex = NULL, };
	Tex3DS_SubTexture* subtex = NULL;
	bool selected = false;
	double x = -1;
	double y = -1;
	double x_size = -1;
	double y_size = -1;
};

struct Hid_info
{
	bool p_a = false;
	bool p_b = false;
	bool p_x = false;
	bool p_y = false;
	bool p_c_up = false;
	bool p_c_down = false;
	bool p_c_left = false;
	bool p_c_right = false;
	bool p_d_up = false;
	bool p_d_down = false;
	bool p_d_left = false;
	bool p_d_right = false;
	bool p_l = false;
	bool p_r = false;
	bool p_zl = false;
	bool p_zr = false;
	bool p_start = false;
	bool p_select = false;
	bool p_cs_up = false;
	bool p_cs_down = false;
	bool p_cs_left = false;
	bool p_cs_right = false;
	bool p_touch = false;
	bool p_any = false;
	bool h_a = false;
	bool h_b = false;
	bool h_x = false;
	bool h_y = false;
	bool h_c_up = false;
	bool h_c_down = false;
	bool h_c_left = false;
	bool h_c_right = false;
	bool h_d_up = false;
	bool h_d_down = false;
	bool h_d_left = false;
	bool h_d_right = false;
	bool h_l = false;
	bool h_r = false;
	bool h_zl = false;
	bool h_zr = false;
	bool h_start = false;
	bool h_select = false;
	bool h_cs_up = false;
	bool h_cs_down = false;
	bool h_cs_left = false;
	bool h_cs_right = false;
	bool h_touch = false;
	bool h_any = false;
	bool r_a = false;
	bool r_b = false;
	bool r_x = false;
	bool r_y = false;
	bool r_c_up = false;
	bool r_c_down = false;
	bool r_c_left = false;
	bool r_c_right = false;
	bool r_d_up = false;
	bool r_d_down = false;
	bool r_d_left = false;
	bool r_d_right = false;
	bool r_l = false;
	bool r_r = false;
	bool r_zl = false;
	bool r_zr = false;
	bool r_start = false;
	bool r_select = false;
	bool r_cs_up = false;
	bool r_cs_down = false;
	bool r_cs_left = false;
	bool r_cs_right = false;
	bool r_touch = false;
	bool r_any = false;
	int cpad_x = 0;
	int cpad_y = 0;
	int touch_x = 0;
	int touch_y = 0;
	int touch_x_move = 0;
	int touch_y_move = 0;
	int held_time = 0;
	u64 ts = 0;
};
