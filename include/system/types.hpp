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
	int bitrate = 0;				//Audio bitrate in Bps.
	int sample_rate = 0;			//Audio smaple rate in Hz.
	int ch = 0;						//Number of audio channels.
	std::string format_name = "";	//Audio codec name.
	double duration = 0;			//Audio track duration in seconds.
	std::string track_lang = "";	//Track languages
	int sample_format = DEF_CONVERTER_SAMPLE_FORMAT_NONE;	//Audio sample format (DEF_CONVERTER_SAMPLE_FORMAT_*).
};

struct Video_info
{
	int width = 0;					//Video width.
	int height = 0;					//Video height.
	double framerate = 0;			//Video framerate.
	std::string format_name = "";	//Video codec name.
	double duration = 0;			//Video track duration in seconds.
	int thread_type = DEF_DECODER_THREAD_TYPE_NONE;	//Threading method (DEF_DECODER_THREAD_TYPE_*).
	int sar_width = 1;				//Sample aspect ratio for width.
	int sar_height = 1;				//Sample aspect ratio for height.
	int pixel_format = DEF_CONVERTER_PIXEL_FORMAT_NONE;	//Video pixel format (DEF_CONVERTER_PIXEL_FORMAT_*).
};

struct Color_converter_parameters
{
	u8* source = NULL;			//(in)  Source raw image data, user must allocate the buffer.
	u8* converted = NULL;		//(out) Converted raw image data, this buffer will be allocated inside of function.
	int in_width = 0;			//(in)  Source image width.
	int in_height = 0;			//(in)  Source image height.
	int out_width = 0;			//(in)  Converted image width.
	int out_height = 0;			//(in)  Converted image height.
	int in_color_format = DEF_CONVERTER_PIXEL_FORMAT_NONE;	//(in) Source image pixel format (DEF_CONVERTER_PIXEL_FORMAT_*).
	int out_color_format = DEF_CONVERTER_PIXEL_FORMAT_NONE;	//(in) Converted image pixel format (DEF_CONVERTER_PIXEL_FORMAT_*).
};

struct Audio_converter_parameters
{
	u8* source = NULL;			//(in)  Source raw audio data, user must allocate the buffer.
	u8* converted = NULL;		//(out) Converted raw audio data, this buffer will be allocated inside of function.
	int in_samples = 0;			//(in)  Number of source audio samples per channel.
	int in_ch = 0;				//(in)  Source audio ch.
	int in_sample_rate = 0;		//(in)  Source audio saple rate in Hz.
	int out_samples = 0;		//(out)  Number of converted audio samples per channel.
	int out_ch = 0;				//(in)  Converted audio ch.
	int out_sample_rate = 0;	//(in)  Converted audio saple rate in Hz.
	int in_sample_format = DEF_CONVERTER_SAMPLE_FORMAT_NONE;	//(in) Source audio sample format (DEF_CONVERTER_SAMPLE_FORMAT_*).
	int out_sample_format = DEF_CONVERTER_SAMPLE_FORMAT_NONE;	//(in) Converted audio sample format (DEF_CONVERTER_SAMPLE_FORMAT_*).
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
