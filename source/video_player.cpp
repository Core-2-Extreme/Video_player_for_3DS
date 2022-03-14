#include "system/headers.hpp"

bool vid_main_run = false;
bool vid_thread_run = false;
bool vid_already_init = false;
bool vid_thread_suspend = true;
bool vid_play_request = false;
bool vid_change_video_request = false;
bool vid_convert_request = false;
bool vid_convert_wait_request = false;
bool vid_decode_video_request = false;
bool vid_decode_video_wait_request = false;
bool vid_select_audio_track_request = false;
bool vid_select_subtitle_track_request = false;
bool vid_use_hw_decoding_request = true;
bool vid_use_hw_color_conversion_request = true;
bool vid_use_multi_threaded_decoding_request = true;
bool vid_pause_request = false;
bool vid_seek_request = false;
bool vid_seek_adjust_request = false;
bool vid_read_packet_request = false;
bool vid_read_packet_wait_request = false;
bool vid_pause_for_home_menu_request = false;
bool vid_set_volume_request = false;
bool vid_set_seek_duration_request = false;
bool vid_clear_raw_buffer_request[2] = { false, false, };
bool vid_clear_cache_packet_request = false;
bool vid_linear_filter = true;
bool vid_show_controls_request = false;
bool vid_allow_skip_frames = false;
bool vid_allow_skip_key_frames = false;
bool vid_key_frame = false;
bool vid_full_screen_mode = false;
bool vid_hw_decoding_mode = false;
bool vid_hw_color_conversion_mode = false;
bool vid_correct_aspect_ratio_mode = false;
bool vid_remember_video_pos_mode = false;
bool vid_show_decode_graph_mode = true;
bool vid_show_color_conversion_graph_mode = true;
bool vid_show_packet_buffer_graph_mode = true;
bool vid_show_raw_video_buffer_graph_mode = true;
bool vid_show_raw_audio_buffer_graph_mode = true;
bool vid_disable_audio_mode = false;
bool vid_disable_video_mode = false;
bool vid_disable_subtitle_mode = false;
bool vid_show_full_screen_msg = true;
bool vid_too_big = false;
bool vid_eof = false;
bool vid_image_enabled[4][DEF_VID_BUFFERS][2];
bool vid_key_frame_list[320];
double vid_time[2][320];
double vid_copy_time[2] = { 0, 0, };
double vid_audio_time = 0;
double vid_video_time = 0;
double vid_convert_time = 0;
double vid_frametime = 0;
double vid_duration = 0;//in ms
double vid_video_zoom = 1;
double vid_subtitle_zoom = 1;
double vid_y_move_speed = 0;
double vid_ui_y_max = 0;
double vid_ui_y_offset = 0;
double vid_video_x_offset = 0;
double vid_video_y_offset = 15;
double vid_subtitle_x_offset = 0;
double vid_subtitle_y_offset = 0;
double vid_current_pos = 0;//in ms
double vid_seek_pos = 0;//in ms
double vid_min_time = 0;
double vid_max_time = 0;
double vid_total_time = 0;
double vid_recent_time[90];
double vid_recent_total_time = 0;
int vid_hid_wait = 0;
int vid_move_content_mode = DEF_VID_MOVE_BOTH;
int vid_packet_buffer[320];
int vid_raw_video_buffer[320];
int vid_raw_audio_buffer[320];
int vid_playback_mode = DEF_VID_NO_REPEAT;
int vid_seek_duration = 5;
int vid_volume = 100;
int vid_lower_resolution = 0;
int vid_menu_mode = 0;
int vid_num_of_threads = 1;
int vid_request_thread_mode = DEF_DECODER_THREAD_TYPE_AUTO;
int vid_turn_off_bottom_screen_count = 0;
int vid_num_of_video_tracks = 0;
int vid_num_of_audio_tracks = 0;
int vid_num_of_subtitle_tracks = 0;
int vid_selected_audio_track = 0;
int vid_selected_subtitle_track = 0;
int vid_total_frames = 0;
int vid_codec_width = 0;
int vid_codec_height = 0;
int vid_tex_width[4] = { 0, 0, 0, 0, };
int vid_tex_height[4] = { 0, 0, 0, 0, };
int vid_lr_count = 0;
int vid_cd_count = 0;
int vid_image_num = 0;
int vid_image_num_3d = 0;
int vid_packet_index = 0;
int vid_control_texture_num = -1;
int vid_banner_texture_num = -1;
int vid_file_index = 0;
int vid_subtitle_index = 0;
u64 vid_show_screen_brightness_until = 0;
u64 vid_previous_ts = 0;
std::string vid_file = "";
std::string vid_dir = "";
std::string vid_audio_track_lang[DEF_DECODER_MAX_AUDIO_TRACKS];
std::string vid_subtitle_track_lang[DEF_DECODER_MAX_SUBTITLE_TRACKS];
std::string vid_msg[DEF_VID_NUM_OF_MSG];
Image_data vid_image[4][DEF_VID_BUFFERS][2];
C2D_Image vid_banner[2];
C2D_Image vid_control[2];
Thread vid_decode_thread, vid_decode_video_thread, vid_convert_thread, vid_read_packet_thread;
Image_data vid_select_audio_track_button, vid_texture_filter_button, vid_allow_skip_frames_button, vid_allow_skip_key_frames_button,
vid_volume_button, vid_seek_duration_button, vid_use_hw_decoding_button, vid_use_hw_color_conversion_button, vid_use_multi_threaded_decoding_button,
vid_lower_resolution_button, vid_menu_button[3], vid_control_button, vid_ok_button, vid_audio_track_button[DEF_DECODER_MAX_AUDIO_TRACKS],
vid_correct_aspect_ratio_button, vid_move_content_button, vid_remember_video_pos_button, vid_show_decode_graph_button,
vid_show_color_conversion_graph_button, vid_show_packet_buffer_graph_button, vid_show_raw_video_buffer_graph_button, 
vid_show_raw_audio_buffer_graph_button, vid_menu_background, vid_scroll_bar, vid_playback_mode_button,
vid_subtitle_track_button[DEF_DECODER_MAX_SUBTITLE_TRACKS], vid_select_subtitle_track_button, vid_disable_audio_button,
vid_disable_video_button, vid_disable_subtitle_button;
Audio_info vid_audio_info;
Video_info vid_video_info;
Subtitle_info vid_subtitle_info;
Subtitle_data vid_subtitle_data[DEF_DECODER_MAX_SUBTITLE_DATA];

void Vid_suspend(void);

void Vid_callback(std::string file, std::string dir)
{
	vid_file = file;
	vid_dir = dir;
	vid_file_index = Util_expl_query_current_file_index();
	vid_change_video_request = true;
}

void Vid_cancel(void)
{

}

bool Vid_query_init_flag(void)
{
	return vid_already_init;
}

bool Vid_query_running_flag(void)
{
	return vid_main_run;
}

void Vid_fit_to_screen(int screen_width, int screen_height)
{
	if(vid_video_info.width != 0 && vid_video_info.height != 0 && vid_video_info.sar_width != 0 && vid_video_info.sar_height != 0)
	{
		//fit to screen size
		if((((double)vid_video_info.width * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1)) / screen_width) >= (((double)vid_video_info.height * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_height : 1)) / screen_height))
			vid_video_zoom = 1.0 / (((double)vid_video_info.width * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1)) / screen_width);
		else
			vid_video_zoom = 1.0 / (((double)vid_video_info.height * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_height : 1)) / screen_height);

		vid_video_x_offset = (screen_width - (vid_video_info.width * vid_video_zoom * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1))) / 2;
		vid_video_y_offset = (screen_height - (vid_video_info.height * vid_video_zoom * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_height : 1))) / 2;
		vid_video_y_offset += 240 - screen_height;
		vid_video_x_offset += 400 - screen_width;
	}
	vid_subtitle_x_offset = 0;
	vid_subtitle_y_offset = 0;
	vid_subtitle_zoom = 1;
}

void Vid_change_video_size(double change_px)
{
	double current_width = (double)vid_video_info.width * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1) * vid_video_zoom;
	if(vid_video_info.width != 0 && vid_video_info.height != 0 && vid_video_info.sar_width != 0 && vid_video_info.sar_height != 0)
		vid_video_zoom = 1.0 / ((double)vid_video_info.width * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1) / (current_width + change_px));
}

void Vid_enter_full_screen(int bottom_screen_timeout)
{
	vid_turn_off_bottom_screen_count = bottom_screen_timeout;
	vid_full_screen_mode = true;
	var_top_lcd_brightness = var_lcd_brightness;
	var_bottom_lcd_brightness = var_lcd_brightness;
}

void Vid_exit_full_screen(void)
{
	vid_turn_off_bottom_screen_count = 0;
	vid_full_screen_mode = false;
	var_turn_on_bottom_lcd = true;
	var_top_lcd_brightness = var_lcd_brightness;
	var_bottom_lcd_brightness = var_lcd_brightness;
}

void Vid_increase_screen_brightness(void)
{
	if(vid_hid_wait <= 0)
	{
		if(var_lcd_brightness + 1 <= 180)
		{
			var_lcd_brightness++;
			var_top_lcd_brightness = var_lcd_brightness;
			if(!vid_full_screen_mode)
				var_bottom_lcd_brightness = var_lcd_brightness;

			vid_show_screen_brightness_until = osGetTime() + 2500;
		}
		vid_hid_wait = 10;
	}
	else
		vid_hid_wait--;
}

void Vid_decrease_screen_brightness(void)
{
	if(vid_hid_wait <= 0)
	{
		if(var_lcd_brightness - 1 >= 0)
		{
			var_lcd_brightness--;
			var_top_lcd_brightness = var_lcd_brightness;
			if(!vid_full_screen_mode)
				var_bottom_lcd_brightness = var_lcd_brightness;

			vid_show_screen_brightness_until = osGetTime() + 2500;
		}
		vid_hid_wait = 10;
	}
	else
		vid_hid_wait--;
}

void Vid_control_full_screen(void)
{
	if(vid_turn_off_bottom_screen_count > 0)
	{
		vid_turn_off_bottom_screen_count--;
		if(vid_turn_off_bottom_screen_count == 0 && var_model != CFG_MODEL_2DS)
			var_turn_on_bottom_lcd = false;
		if(var_bottom_lcd_brightness > 10 && var_model != CFG_MODEL_2DS)
			var_bottom_lcd_brightness--;
	}
}

void Vid_init_variable(void)
{
	//requests
	vid_seek_request = false;
	vid_seek_adjust_request = false;
	vid_read_packet_request = false;
	vid_pause_for_home_menu_request = false;
	vid_set_volume_request = false;
	vid_set_seek_duration_request = false;
	vid_clear_raw_buffer_request[0] = false;
	vid_clear_raw_buffer_request[1] = false;
	vid_clear_cache_packet_request = false;
	vid_show_controls_request = false;
	vid_decode_video_request = false;
	vid_convert_request = false;
	vid_convert_wait_request = false;
	vid_decode_video_wait_request = false;
	vid_select_audio_track_request = false;
	vid_select_subtitle_track_request = false;
	vid_pause_request = false;
	vid_read_packet_wait_request = false;
	vid_key_frame = false;

	//mode
	vid_show_decode_graph_mode = true;
	vid_show_color_conversion_graph_mode = true;
	vid_show_packet_buffer_graph_mode = true;
	vid_show_raw_video_buffer_graph_mode = true;
	vid_show_raw_audio_buffer_graph_mode = true;
	vid_request_thread_mode = DEF_DECODER_THREAD_TYPE_AUTO;
	vid_hw_decoding_mode = false;
	vid_hw_color_conversion_mode = false;

	//video
	vid_eof = false;
	vid_image_num_3d = 0;
	vid_packet_index = 0;
	vid_num_of_video_tracks = 0;
	vid_image_num = 0;
	vid_frametime = 0;
	vid_current_pos = 0;
	vid_duration = 0;
	vid_codec_width = 0;
	vid_codec_height = 0;
	vid_video_info.width = 0;
	vid_video_info.height = 0;
	vid_video_info.framerate = 0;
	vid_video_info.format_name = "n/a";
	vid_video_info.duration = 0;
	vid_video_info.thread_type = DEF_DECODER_THREAD_TYPE_NONE;
	vid_video_info.sar_width = 1;
	vid_video_info.sar_height = 1;
	for(int i = 0; i < 4; i++)
	{
		vid_tex_width[i] = 0;
		vid_tex_height[i] = 0;
	}
	for(int k = 0; k < 2; k++)
	{
		for(int i = 0; i < 4; i++)
		{
			for(int s = 0; s < DEF_VID_BUFFERS; s++)
				vid_image_enabled[i][s][k] = false;
		}
	}

	//audio
	vid_too_big = false;
	vid_num_of_audio_tracks = 0;
	vid_selected_audio_track = 0;
	vid_audio_info.bitrate = 0;
	vid_audio_info.sample_rate = 0;
	vid_audio_info.ch = 0;
	vid_audio_info.format_name = "n/a";
	vid_audio_info.duration = 0;
	vid_audio_info.track_lang = "n/a";
	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		vid_audio_track_lang[i] = "";

	//subtitle	
	vid_num_of_subtitle_tracks = 0;
	vid_selected_subtitle_track = 0;
	vid_subtitle_index = 0;
	vid_subtitle_info.format_name = "n/a";
	vid_subtitle_info.track_lang = "n/a";
	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_DATA; i++)
	{
		vid_subtitle_data[i].text = "";
		vid_subtitle_data[i].start_time = 0;
		vid_subtitle_data[i].end_time = 0;
	}
	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		vid_subtitle_track_lang[i] = "";

	//debug
	vid_previous_ts = 0;
	vid_total_time = 0;
	vid_total_frames = 0;
	vid_min_time = 99999999;
	vid_max_time = 0;
	vid_recent_total_time = 0;
	vid_audio_time = 0;
	vid_video_time = 0;
	vid_copy_time[0] = 0;
	vid_copy_time[1] = 0;
	vid_convert_time = 0;
	for(int i = 0; i < 90; i++)
		vid_recent_time[i] = 0;

	for(int i = 0 ; i < 320; i++)
	{
		vid_time[0][i] = 0;
		vid_time[1][i] = 0;
		vid_packet_buffer[i] = 0;
		vid_raw_video_buffer[i] = 0;
		vid_raw_audio_buffer[i] = 0;
		vid_key_frame_list[i] = false;
	}
}

void Vid_hid(Hid_info key)
{
	double video_width = 0;
	if(vid_set_volume_request || vid_set_seek_duration_request || (aptShouldJumpToHome() && vid_pause_for_home_menu_request))
		return;

	if(Util_err_query_error_show_flag())
		Util_err_main(key);
	else if(Util_expl_query_show_flag())
		Util_expl_main(key);
	else
	{
		if(vid_pause_for_home_menu_request)
		{
			vid_pause_request = false;
			vid_pause_for_home_menu_request = false;
		}

		if(aptShouldJumpToHome())
		{
			vid_pause_request = true;
			vid_pause_for_home_menu_request = true;
			while(vid_num_of_audio_tracks > 0 && !Util_speaker_is_paused(0) && vid_play_request)
				usleep(10000);
		}

		if(vid_full_screen_mode)
		{
			if(key.p_select || key.p_touch || aptShouldJumpToHome())
			{
				Vid_fit_to_screen(400, 225);
				Vid_exit_full_screen();
				vid_show_full_screen_msg = false;
			}
			else if(key.p_a)
			{
				if(vid_play_request)
					vid_pause_request = !vid_pause_request;
				else
					vid_play_request = true;
			}
		}
		else if(vid_select_audio_track_request)
		{
			if(key.p_d_down || key.p_c_down)
			{
				if(vid_selected_audio_track + 1 < vid_num_of_audio_tracks)
					vid_selected_audio_track++;
			}
			else if(key.p_d_up || key.p_c_up)
			{
				if(vid_selected_audio_track - 1 > -1)
					vid_selected_audio_track--;
			}
			else if(Util_hid_is_pressed(key, vid_ok_button))
				vid_ok_button.selected = true;
			else if(key.p_a || (Util_hid_is_released(key, vid_ok_button) && vid_ok_button.selected))
				vid_select_audio_track_request = false;

			for(int i = 0; i < vid_num_of_audio_tracks; i++)
			{
				if(Util_hid_is_pressed(key, vid_audio_track_button[i]))
				{
					vid_audio_track_button[i].selected = true;
					break;
				}
				else if(Util_hid_is_released(key, vid_audio_track_button[i]) && vid_audio_track_button[i].selected)
				{
					vid_selected_audio_track = i;
					break;
				}
			}
		}
		else if(vid_select_subtitle_track_request)//subtitle selection
		{
			if(key.p_d_down || key.p_c_down)
			{
				if(vid_selected_subtitle_track + 1 < vid_num_of_subtitle_tracks)
					vid_selected_subtitle_track++;
			}
			else if(key.p_d_up || key.p_c_up)
			{
				if(vid_selected_subtitle_track - 1 > -1)
					vid_selected_subtitle_track--;
			}
			else if(Util_hid_is_pressed(key, vid_ok_button))
				vid_ok_button.selected = true;
			else if(key.p_a || (Util_hid_is_released(key, vid_ok_button) && vid_ok_button.selected))
				vid_select_subtitle_track_request = false;

			for(int i = 0; i < vid_num_of_subtitle_tracks; i++)
			{
				if(Util_hid_is_pressed(key, vid_subtitle_track_button[i]))
				{
					vid_subtitle_track_button[i].selected = true;
					break;
				}
				else if(Util_hid_is_released(key, vid_subtitle_track_button[i]) && vid_subtitle_track_button[i].selected)
				{
					vid_selected_subtitle_track = i;
					break;
				}
			}
		}
		else if(vid_show_controls_request)
		{
			if(Util_hid_is_pressed(key, vid_control_button))
				vid_control_button.selected = true;
			else if((key.p_a || Util_hid_is_released(key, vid_control_button)) && vid_control_button.selected)
				vid_show_controls_request = false;
		}
		else
		{
			if(vid_menu_mode == DEF_VID_MENU_SETTINGS_0)
			{
				//scroll
				if(vid_menu_background.selected)
					vid_y_move_speed = key.touch_y_move * var_scroll_speed;
				else if(Util_hid_is_pressed(key, vid_scroll_bar))
					vid_scroll_bar.selected = true;
				else if(vid_scroll_bar.selected)
				{
					if(key.touch_y < 50)
						vid_ui_y_offset = 0;
					else if(key.touch_y > 175)
						vid_ui_y_offset = vid_ui_y_max;
					else
						vid_ui_y_offset = vid_ui_y_max * (50 - key.touch_y) / -125;
				}
				//audio track button
				else if(Util_hid_is_pressed(key, vid_select_audio_track_button))
					vid_select_audio_track_button.selected = true;
				else if(Util_hid_is_released(key, vid_select_audio_track_button) && vid_select_audio_track_button.selected)
					vid_select_audio_track_request = !vid_select_audio_track_request;
				else if(!Util_hid_is_held(key, vid_select_audio_track_button) && vid_select_audio_track_button.selected)
				{
					vid_select_audio_track_button.selected = false;
					vid_menu_background.selected = true;
				}
				//subtitle track button
				else if(Util_hid_is_pressed(key, vid_select_subtitle_track_button))
					vid_select_subtitle_track_button.selected = true;
				else if(Util_hid_is_released(key, vid_select_subtitle_track_button) && vid_select_subtitle_track_button.selected)
					vid_select_subtitle_track_request = !vid_select_subtitle_track_request;
				else if(!Util_hid_is_held(key, vid_select_subtitle_track_button) && vid_select_subtitle_track_button.selected)
				{
					vid_select_subtitle_track_button.selected = false;
					vid_menu_background.selected = true;
				}
				//texture filter button
				else if(Util_hid_is_pressed(key, vid_texture_filter_button))
					vid_texture_filter_button.selected = true;
				else if(Util_hid_is_released(key, vid_texture_filter_button) && vid_texture_filter_button.selected)
				{
					vid_linear_filter = !vid_linear_filter;
					for(int k = 0; k < 2; k++)
					{
						for(int i = 0; i < 4; i++)
						{
							for(int s = 0; s < DEF_VID_BUFFERS; s++)
							{
								if(vid_image_enabled[i][s][k])
									Draw_set_texture_filter(&vid_image[i][s][k], vid_linear_filter);
							}
						}
					}
				}
				else if(!Util_hid_is_held(key, vid_texture_filter_button) && vid_texture_filter_button.selected)
				{
					vid_texture_filter_button.selected = false;
					vid_menu_background.selected = true;
				}
				//allow skip frames button
				else if(Util_hid_is_pressed(key, vid_allow_skip_frames_button))
					vid_allow_skip_frames_button.selected = true;
				else if(Util_hid_is_released(key, vid_allow_skip_frames_button) && vid_allow_skip_frames_button.selected)
				{
					vid_allow_skip_frames = !vid_allow_skip_frames;
					if(vid_allow_skip_key_frames)
						vid_allow_skip_key_frames = false;
				}
				else if(!Util_hid_is_held(key, vid_allow_skip_frames_button) && vid_allow_skip_frames_button.selected)
				{
					vid_allow_skip_frames_button.selected = false;
					vid_menu_background.selected = true;
				}
				//allow skip key frames button
				else if(Util_hid_is_pressed(key, vid_allow_skip_key_frames_button) && vid_allow_skip_frames)
					vid_allow_skip_key_frames_button.selected = true;
				else if(Util_hid_is_released(key, vid_allow_skip_key_frames_button) && vid_allow_skip_key_frames_button.selected && vid_allow_skip_frames)
					vid_allow_skip_key_frames = !vid_allow_skip_key_frames;
				else if(!Util_hid_is_held(key, vid_allow_skip_key_frames_button) && vid_allow_skip_key_frames_button.selected)
				{
					vid_allow_skip_key_frames_button.selected = false;
					vid_menu_background.selected = true;
				}
				//change volume button
				else if(Util_hid_is_pressed(key, vid_volume_button))
					vid_volume_button.selected = true;
				else if(Util_hid_is_released(key, vid_volume_button) && vid_volume_button.selected)
					vid_set_volume_request = true;
				else if(!Util_hid_is_held(key, vid_volume_button) && vid_volume_button.selected)
				{
					vid_volume_button.selected = false;
					vid_menu_background.selected = true;
				}
				//change seek duration button
				else if(Util_hid_is_pressed(key, vid_seek_duration_button))
					vid_seek_duration_button.selected = true;
				else if(Util_hid_is_released(key, vid_seek_duration_button) && vid_seek_duration_button.selected)
					vid_set_seek_duration_request = true;
				else if(!Util_hid_is_held(key, vid_seek_duration_button) && vid_seek_duration_button.selected)
				{
					vid_seek_duration_button.selected = false;
					vid_menu_background.selected = true;
				}
				//correct aspect ratio button
				else if(Util_hid_is_pressed(key, vid_correct_aspect_ratio_button))
					vid_correct_aspect_ratio_button.selected = true;
				else if(Util_hid_is_released(key, vid_correct_aspect_ratio_button) && vid_correct_aspect_ratio_button.selected)
				{
					vid_correct_aspect_ratio_mode = !vid_correct_aspect_ratio_mode;
					Vid_fit_to_screen(400, 225);
				}
				else if(!Util_hid_is_held(key, vid_correct_aspect_ratio_button) && vid_correct_aspect_ratio_button.selected)
				{
					vid_correct_aspect_ratio_button.selected = false;
					vid_menu_background.selected = true;
				}
				//disable resize and move button
				else if(Util_hid_is_pressed(key, vid_move_content_button))
					vid_move_content_button.selected = true;
				else if(Util_hid_is_released(key, vid_move_content_button) && vid_move_content_button.selected)
				{
					if(vid_move_content_mode + 1 > DEF_VID_MOVE_SUBTITLE)
						vid_move_content_mode = DEF_VID_MOVE_DISABLE;
					else
						vid_move_content_mode++;
				}
				else if(!Util_hid_is_held(key, vid_move_content_button) && vid_move_content_button.selected)
				{
					vid_move_content_button.selected = false;
					vid_menu_background.selected = true;
				}
				//remember video pos button
				else if(Util_hid_is_pressed(key, vid_remember_video_pos_button))
					vid_remember_video_pos_button.selected = true;
				else if(Util_hid_is_released(key, vid_remember_video_pos_button) && vid_remember_video_pos_button.selected)
					vid_remember_video_pos_mode = !vid_remember_video_pos_mode;
				else if(!Util_hid_is_held(key, vid_remember_video_pos_button) && vid_remember_video_pos_button.selected)
				{
					vid_remember_video_pos_button.selected = false;
					vid_menu_background.selected = true;
				}
				//playback mode button
				else if(Util_hid_is_pressed(key, vid_playback_mode_button))
					vid_playback_mode_button.selected = true;
				else if(Util_hid_is_released(key, vid_playback_mode_button) && vid_playback_mode_button.selected)
				{
					if(vid_playback_mode + 1 > DEF_VID_RANDOM)
						vid_playback_mode = DEF_VID_NO_REPEAT;
					else
						vid_playback_mode++;
				}
				else if(!Util_hid_is_held(key, vid_playback_mode_button) && vid_playback_mode_button.selected)
				{
					vid_playback_mode_button.selected = false;
					vid_menu_background.selected = true;
				}
				//background
				else if(Util_hid_is_pressed(key, vid_menu_background))
					vid_menu_background.selected = true;
				//menu mode button
				else if(Util_hid_is_pressed(key, vid_menu_button[1]))
					vid_menu_button[1].selected = true;
				else if(Util_hid_is_released(key, vid_menu_button[1]) && vid_menu_button[1].selected)
				{
					vid_ui_y_max = -100;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_SETTINGS_1;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[2]))
					vid_menu_button[2].selected = true;
				else if(Util_hid_is_released(key, vid_menu_button[2]) && vid_menu_button[2].selected)
				{
					vid_ui_y_max = -100;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_INFO;
				}

				if (vid_menu_background.selected || vid_y_move_speed != 0)
				{
					if(!vid_menu_background.selected)
						vid_y_move_speed *= 0.95;
					
					if(vid_y_move_speed < 0.75 && vid_y_move_speed > -0.75)
						vid_y_move_speed = 0;

					if(vid_ui_y_offset - vid_y_move_speed > 0)
						vid_ui_y_offset = 0;
					else if(vid_ui_y_offset - vid_y_move_speed < vid_ui_y_max)
						vid_ui_y_offset = vid_ui_y_max;
					else
						vid_ui_y_offset -= vid_y_move_speed;
				}
			}
			if(vid_menu_mode == DEF_VID_MENU_SETTINGS_1)
			{
				//scroll
				if(vid_menu_background.selected)
					vid_y_move_speed = key.touch_y_move * var_scroll_speed;
				else if(Util_hid_is_pressed(key, vid_scroll_bar))
					vid_scroll_bar.selected = true;
				else if(vid_scroll_bar.selected)
				{
					if(key.touch_y < 50)
						vid_ui_y_offset = 0;
					else if(key.touch_y > 175)
						vid_ui_y_offset = vid_ui_y_max;
					else
						vid_ui_y_offset = vid_ui_y_max * (50 - key.touch_y) / -125;
				}
				//disable audio button
				else if(Util_hid_is_pressed(key, vid_disable_audio_button) && !vid_play_request)
					vid_disable_audio_button.selected = true;
				else if(Util_hid_is_released(key, vid_disable_audio_button) && vid_disable_audio_button.selected)
					vid_disable_audio_mode = !vid_disable_audio_mode;
				else if(!Util_hid_is_held(key, vid_disable_audio_button) && vid_disable_audio_button.selected)
				{
					vid_disable_audio_button.selected = false;
					vid_menu_background.selected = true;
				}
				//disable video button
				else if(Util_hid_is_pressed(key, vid_disable_video_button) && !vid_play_request)
					vid_disable_video_button.selected = true;
				else if(Util_hid_is_released(key, vid_disable_video_button) && vid_disable_video_button.selected)
					vid_disable_video_mode = !vid_disable_video_mode;
				else if(!Util_hid_is_held(key, vid_disable_video_button) && vid_disable_video_button.selected)
				{
					vid_disable_video_button.selected = false;
					vid_menu_background.selected = true;
				}
				//disable subtitle button
				else if(Util_hid_is_pressed(key, vid_disable_subtitle_button) && !vid_play_request)
					vid_disable_subtitle_button.selected = true;
				else if(Util_hid_is_released(key, vid_disable_subtitle_button) && vid_disable_subtitle_button.selected)
					vid_disable_subtitle_mode = !vid_disable_subtitle_mode;
				else if(!Util_hid_is_held(key, vid_disable_subtitle_button) && vid_disable_subtitle_button.selected)
				{
					vid_disable_subtitle_button.selected = false;
					vid_menu_background.selected = true;
				}
				//hardware decoding button
				else if(Util_hid_is_pressed(key, vid_use_hw_decoding_button) && !(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL) && !vid_play_request)
					vid_use_hw_decoding_button.selected = true;
				else if(Util_hid_is_released(key, vid_use_hw_decoding_button) && !(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL) && !vid_play_request && vid_use_hw_decoding_button.selected)
					vid_use_hw_decoding_request = !vid_use_hw_decoding_request;
				else if(!Util_hid_is_held(key, vid_use_hw_decoding_button) && vid_use_hw_decoding_button.selected)
				{
					vid_use_hw_decoding_button.selected = false;
					vid_menu_background.selected = true;
				}
				//hardware color conversion button
				else if(Util_hid_is_pressed(key, vid_use_hw_color_conversion_button) && !vid_play_request)
					vid_use_hw_color_conversion_button.selected = true;
				else if(Util_hid_is_released(key, vid_use_hw_color_conversion_button) && !vid_play_request && vid_use_hw_color_conversion_button.selected)
					vid_use_hw_color_conversion_request = !vid_use_hw_color_conversion_request;
				else if(!Util_hid_is_held(key, vid_use_hw_color_conversion_button) && vid_use_hw_color_conversion_button.selected)
				{
					vid_use_hw_color_conversion_button.selected = false;
					vid_menu_background.selected = true;
				}
				//multi-threaded decoding button
				else if(Util_hid_is_pressed(key, vid_use_multi_threaded_decoding_button) && !vid_play_request)
					vid_use_multi_threaded_decoding_button.selected = true;
				else if(Util_hid_is_released(key, vid_use_multi_threaded_decoding_button) && !vid_play_request && vid_use_multi_threaded_decoding_button.selected)
					vid_use_multi_threaded_decoding_request = !vid_use_multi_threaded_decoding_request;
				else if(!Util_hid_is_held(key, vid_use_multi_threaded_decoding_button) && vid_use_multi_threaded_decoding_button.selected)
				{
					vid_use_multi_threaded_decoding_button.selected = false;
					vid_menu_background.selected = true;
				}
				//lower video resolution button
				else if(Util_hid_is_pressed(key, vid_lower_resolution_button) && !vid_play_request)
					vid_lower_resolution_button.selected = true;
				else if(Util_hid_is_released(key, vid_lower_resolution_button) && !vid_play_request && vid_lower_resolution_button.selected)
				{
					if(vid_lower_resolution + 1 > 2)
						vid_lower_resolution = 0;
					else
						vid_lower_resolution++;
				}
				else if(!Util_hid_is_held(key, vid_lower_resolution_button) && vid_lower_resolution_button.selected)
				{
					vid_lower_resolution_button.selected = false;
					vid_menu_background.selected = true;
				}
				//background
				else if(Util_hid_is_pressed(key, vid_menu_background))
					vid_menu_background.selected = true;
				//menu mode button
				else if(Util_hid_is_pressed(key, vid_menu_button[0]))
					vid_menu_button[0].selected = true;
				else if(Util_hid_is_released(key, vid_menu_button[0]) && vid_menu_button[0].selected)
				{
					vid_ui_y_max = -150;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_SETTINGS_0;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[2]))
					vid_menu_button[2].selected = true;
				else if(Util_hid_is_released(key, vid_menu_button[2]) && vid_menu_button[2].selected)
				{
					vid_ui_y_max = -100;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_INFO;
				}

				if (vid_menu_background.selected || vid_y_move_speed != 0)
				{
					if(!vid_menu_background.selected)
						vid_y_move_speed *= 0.95;
					
					if(vid_y_move_speed < 0.75 && vid_y_move_speed > -0.75)
						vid_y_move_speed = 0;

					if(vid_ui_y_offset - vid_y_move_speed > 0)
						vid_ui_y_offset = 0;
					else if(vid_ui_y_offset - vid_y_move_speed < vid_ui_y_max)
						vid_ui_y_offset = vid_ui_y_max;
					else
						vid_ui_y_offset -= vid_y_move_speed;
				}
			}
			else if(vid_menu_mode == DEF_VID_MENU_INFO)
			{
				//scroll
				if(vid_menu_background.selected)
					vid_y_move_speed = key.touch_y_move * var_scroll_speed;
				else if(Util_hid_is_pressed(key, vid_scroll_bar))
					vid_scroll_bar.selected = true;
				else if(vid_scroll_bar.selected)
				{
					if(key.touch_y < 50)
						vid_ui_y_offset = 0;
					else if(key.touch_y > 175)
						vid_ui_y_offset = vid_ui_y_max;
					else
						vid_ui_y_offset = vid_ui_y_max * (50 - key.touch_y) / -125;
				}
				//decoding graph button
				else if(Util_hid_is_pressed(key, vid_show_decode_graph_button))
					vid_show_decode_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_show_decode_graph_button) && vid_show_decode_graph_button.selected)
					vid_show_decode_graph_mode = !vid_show_decode_graph_mode;
				else if(!Util_hid_is_held(key, vid_show_decode_graph_button) && vid_show_decode_graph_button.selected)
				{
					vid_show_decode_graph_button.selected = false;
					vid_menu_background.selected = true;
				}
				//color conversion graph button
				else if(Util_hid_is_pressed(key, vid_show_color_conversion_graph_button))
					vid_show_color_conversion_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_show_color_conversion_graph_button) && vid_show_color_conversion_graph_button.selected)
					vid_show_color_conversion_graph_mode = !vid_show_color_conversion_graph_mode;
				else if(!Util_hid_is_held(key, vid_show_color_conversion_graph_button) && vid_show_color_conversion_graph_button.selected)
				{
					vid_show_color_conversion_graph_button.selected = false;
					vid_menu_background.selected = true;
				}
				//packet buffer graph button
				else if(Util_hid_is_pressed(key, vid_show_packet_buffer_graph_button))
					vid_show_packet_buffer_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_show_packet_buffer_graph_button) && vid_show_packet_buffer_graph_button.selected)
					vid_show_packet_buffer_graph_mode = !vid_show_packet_buffer_graph_mode;
				else if(!Util_hid_is_held(key, vid_show_packet_buffer_graph_button) && vid_show_packet_buffer_graph_button.selected)
				{
					vid_show_packet_buffer_graph_button.selected = false;
					vid_menu_background.selected = true;
				}
				//raw video buffer graph button
				else if(Util_hid_is_pressed(key, vid_show_raw_video_buffer_graph_button))
					vid_show_raw_video_buffer_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_show_raw_video_buffer_graph_button) && vid_show_raw_video_buffer_graph_button.selected)
					vid_show_raw_video_buffer_graph_mode = !vid_show_raw_video_buffer_graph_mode;
				else if(!Util_hid_is_held(key, vid_show_raw_video_buffer_graph_button) && vid_show_raw_video_buffer_graph_button.selected)
				{
					vid_show_raw_video_buffer_graph_button.selected = false;
					vid_menu_background.selected = true;
				}
				//raw audio buffer graph button
				else if(Util_hid_is_pressed(key, vid_show_raw_audio_buffer_graph_button))
					vid_show_raw_audio_buffer_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_show_raw_audio_buffer_graph_button) && vid_show_raw_audio_buffer_graph_button.selected)
					vid_show_raw_audio_buffer_graph_mode = !vid_show_raw_audio_buffer_graph_mode;
				else if(!Util_hid_is_held(key, vid_show_raw_audio_buffer_graph_button) && vid_show_raw_audio_buffer_graph_button.selected)
				{
					vid_show_raw_audio_buffer_graph_button.selected = false;
					vid_menu_background.selected = true;
				}
				//background
				else if(Util_hid_is_pressed(key, vid_menu_background))
					vid_menu_background.selected = true;
				//menu mode button
				else if(Util_hid_is_pressed(key, vid_menu_button[0]))
					vid_menu_button[0].selected = true;
				else if(Util_hid_is_released(key, vid_menu_button[0]) && vid_menu_button[0].selected)
				{
					vid_ui_y_max = -150;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_SETTINGS_0;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[1]))
					vid_menu_button[1].selected = true;
				else if(Util_hid_is_released(key, vid_menu_button[1]) && vid_menu_button[1].selected)
				{
					vid_ui_y_max = -100;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_SETTINGS_1;
				}

				if (vid_menu_background.selected || vid_y_move_speed != 0)
				{
					if(!vid_menu_background.selected)
						vid_y_move_speed *= 0.95;
					
					if(vid_y_move_speed < 0.75 && vid_y_move_speed > -0.75)
						vid_y_move_speed = 0;

					if(vid_ui_y_offset - vid_y_move_speed > 0)
						vid_ui_y_offset = 0;
					else if(vid_ui_y_offset - vid_y_move_speed < vid_ui_y_max)
						vid_ui_y_offset = vid_ui_y_max;
					else
						vid_ui_y_offset -= vid_y_move_speed;
				}
			}

			if(Util_hid_is_pressed(key, *Draw_get_bot_ui_button()))
				Draw_get_bot_ui_button()->selected = true;
			else if (key.p_start || (Util_hid_is_released(key, *Draw_get_bot_ui_button()) && Draw_get_bot_ui_button()->selected))
				Vid_suspend();
			else if(key.p_select)
			{
				Vid_fit_to_screen(400, 240);
				Vid_enter_full_screen(300);
			}
			else if(key.p_a)
			{
				if(vid_play_request)
					vid_pause_request = !vid_pause_request;
				else
					vid_play_request = true;
			}
			else if(key.p_b)
				vid_play_request = false;
			else if(key.p_x)
			{
				Util_expl_set_show_flag(true);
				Util_expl_set_callback(Vid_callback);
				Util_expl_set_cancel_callback(Vid_cancel);
			}
			else if(key.p_y)
			{
				if(vid_menu_mode == DEF_VID_MENU_NONE)
				{
					vid_ui_y_max = -150;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_SETTINGS_0;
				}
				else
				{
					vid_ui_y_max = 0;
					vid_ui_y_offset = 0;
					vid_menu_mode = DEF_VID_MENU_NONE;
				}
			}
			else if(Util_hid_is_pressed(key, vid_control_button))
				vid_control_button.selected = true;
			else if(Util_hid_is_released(key, vid_control_button) && vid_control_button.selected)
				vid_show_controls_request = true;
			else if(key.p_touch && key.touch_x >= 5 && key.touch_x <= 314 && key.touch_y >= 210 && key.touch_y <= 219)
			{
				vid_seek_pos = vid_duration * (((double)key.touch_x - 5) / 310);
				vid_seek_request = true;
			}
			
			if((key.p_c_down || key.p_c_up || key.p_c_right || key.p_c_left || key.h_c_down || key.h_c_up || key.h_c_right || key.h_c_left)
			 && vid_move_content_mode)
			{
				if(vid_move_content_mode == DEF_VID_MOVE_VIDEO || vid_move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.p_c_down)
						vid_video_y_offset -= 1 * var_scroll_speed;
					else if(key.h_c_down)
					{
						if(!vid_select_audio_track_request && !vid_select_subtitle_track_request)
						{
							if(vid_cd_count > 600)
								vid_video_y_offset -= 10 * var_scroll_speed;
							else if(vid_cd_count > 240)
								vid_video_y_offset -= 7.5 * var_scroll_speed;
							else if(vid_cd_count > 5)
								vid_video_y_offset -= 5 * var_scroll_speed;
						}
					}

					if(key.p_c_up)
						vid_video_y_offset += 1 * var_scroll_speed;
					else if(key.h_c_up)
					{
						if(!vid_select_audio_track_request && !vid_select_subtitle_track_request)
						{
							if(vid_cd_count > 600)
								vid_video_y_offset += 10 * var_scroll_speed;
							else if(vid_cd_count > 240)
								vid_video_y_offset += 7.5 * var_scroll_speed;
							else if(vid_cd_count > 5)
								vid_video_y_offset += 5 * var_scroll_speed;
						}
					}

					if(key.p_c_right)
						vid_video_x_offset -= 1 * var_scroll_speed;
					else if(key.h_c_right)
					{
						if(vid_cd_count > 600)
							vid_video_x_offset -= 10 * var_scroll_speed;
						else if(vid_cd_count > 240)
							vid_video_x_offset -= 7.5 * var_scroll_speed;
						else if(vid_cd_count > 5)
							vid_video_x_offset -= 5 * var_scroll_speed;
					}

					if(key.p_c_left)
						vid_video_x_offset += 1 * var_scroll_speed;
					else if(key.h_c_left)
					{
						if(vid_cd_count > 600)
							vid_video_x_offset += 10 * var_scroll_speed;
						else if(vid_cd_count > 240)
							vid_video_x_offset += 7.5 * var_scroll_speed;
						else if(vid_cd_count > 5)
							vid_video_x_offset += 5 * var_scroll_speed;
					}

					if(vid_video_x_offset > 400)
						vid_video_x_offset = 400;
					else if(vid_video_x_offset < -vid_video_info.width * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1) * vid_video_zoom)
						vid_video_x_offset = -vid_video_info.width * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1) * vid_video_zoom;

					if(vid_video_y_offset > 480)
						vid_video_y_offset = 480;
					else if(vid_video_y_offset < -vid_video_info.height * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_height : 1) * vid_video_zoom)
						vid_video_y_offset = -vid_video_info.height * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_height : 1) * vid_video_zoom;
				}
				if(vid_move_content_mode == DEF_VID_MOVE_SUBTITLE || vid_move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.p_c_down)
						vid_subtitle_y_offset -= 1 * var_scroll_speed;
					else if(key.h_c_down)
					{
						if(!vid_select_audio_track_request && !vid_select_subtitle_track_request)
						{
							if(vid_cd_count > 600)
								vid_subtitle_y_offset -= 10 * var_scroll_speed;
							else if(vid_cd_count > 240)
								vid_subtitle_y_offset -= 7.5 * var_scroll_speed;
							else if(vid_cd_count > 5)
								vid_subtitle_y_offset -= 5 * var_scroll_speed;
						}
					}

					if(key.p_c_up)
						vid_subtitle_y_offset += 1 * var_scroll_speed;
					else if(key.h_c_up)
					{
						if(!vid_select_audio_track_request && !vid_select_subtitle_track_request)
						{
							if(vid_cd_count > 600)
								vid_subtitle_y_offset += 10 * var_scroll_speed;
							else if(vid_cd_count > 240)
								vid_subtitle_y_offset += 7.5 * var_scroll_speed;
							else if(vid_cd_count > 5)
								vid_subtitle_y_offset += 5 * var_scroll_speed;
						}
					}

					if(key.p_c_right)
						vid_subtitle_x_offset -= 1 * var_scroll_speed;
					else if(key.h_c_right)
					{
						if(vid_cd_count > 600)
							vid_subtitle_x_offset -= 10 * var_scroll_speed;
						else if(vid_cd_count > 240)
							vid_subtitle_x_offset -= 7.5 * var_scroll_speed;
						else if(vid_cd_count > 5)
							vid_subtitle_x_offset -= 5 * var_scroll_speed;
					}

					if(key.p_c_left)
						vid_subtitle_x_offset += 1 * var_scroll_speed;
					else if(key.h_c_left)
					{
						if(vid_cd_count > 600)
							vid_subtitle_x_offset += 10 * var_scroll_speed;
						else if(vid_cd_count > 240)
							vid_subtitle_x_offset += 7.5 * var_scroll_speed;
						else if(vid_cd_count > 5)
							vid_subtitle_x_offset += 5 * var_scroll_speed;
					}

					if(vid_subtitle_x_offset > 400)
						vid_subtitle_x_offset = 400;
					else if(vid_subtitle_x_offset < -vid_codec_width * vid_video_zoom)
						vid_subtitle_x_offset = -vid_codec_width * vid_video_zoom;

					if(vid_subtitle_y_offset > 480)
						vid_subtitle_y_offset = 480;
					else if(vid_subtitle_y_offset < -vid_codec_height * vid_video_zoom)
						vid_subtitle_y_offset = -vid_codec_height * vid_video_zoom;
				}

				vid_cd_count++;
			}
			else
				vid_cd_count = 0;

			if((key.p_l || key.p_r || key.h_l || key.h_r) && vid_move_content_mode)
			{
				if(vid_move_content_mode == DEF_VID_MOVE_VIDEO || vid_move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.h_l || key.p_l)
					{
						if(vid_lr_count > 360)
							Vid_change_video_size(-5);
						else if(vid_lr_count > 120)
							Vid_change_video_size(-3);
						else if(vid_lr_count > 5)
							Vid_change_video_size(-1);
					}

					if(key.h_r || key.p_r)
					{
						if(vid_lr_count > 360)
							Vid_change_video_size(5);
						else if(vid_lr_count > 120)
							Vid_change_video_size(3);
						else if(vid_lr_count > 5)
							Vid_change_video_size(1);
					}

					video_width = (double)vid_video_info.width * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1) * vid_video_zoom;
					//If video is too large or small, don't enlarge or make it small anymore.
					if(video_width < 20)
					{
						if(vid_lr_count > 360)
							Vid_change_video_size(5);
						else if(vid_lr_count > 120)
							Vid_change_video_size(3);
						else if(vid_lr_count > 5)
							Vid_change_video_size(1);
					}
					else if(video_width > 2000)
					{
						if(vid_lr_count > 360)
							Vid_change_video_size(-5);
						else if(vid_lr_count > 120)
							Vid_change_video_size(-3);
						else if(vid_lr_count > 5)
							Vid_change_video_size(-1);
					}
				}
				if(vid_move_content_mode == DEF_VID_MOVE_SUBTITLE || vid_move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.h_l || key.p_l)
					{
						if(vid_lr_count > 360)
							vid_subtitle_zoom -= 0.05 * var_scroll_speed;
						else if(vid_lr_count > 120)
							vid_subtitle_zoom -= 0.01 * var_scroll_speed;
						else if(vid_lr_count > 5)
							vid_subtitle_zoom -= 0.005 * var_scroll_speed;
					}

					if(key.h_r || key.p_r)
					{
						if(vid_lr_count > 360)
							vid_subtitle_zoom += 0.05 * var_scroll_speed;
						else if(vid_lr_count > 120)
							vid_subtitle_zoom += 0.01 * var_scroll_speed;
						else if(vid_lr_count > 5)
							vid_subtitle_zoom += 0.005 * var_scroll_speed;
					}

					if(vid_subtitle_zoom < 0.05)
						vid_subtitle_zoom = 0.05;
					else if(vid_subtitle_zoom > 10)
						vid_subtitle_zoom = 10;
				}

				vid_lr_count++;
			}
			else
				vid_lr_count = 0;
		}

		if(key.p_d_right)
		{
			if(vid_current_pos + (vid_seek_duration * 1000) > vid_duration)
				vid_seek_pos = vid_duration;
			else
				vid_seek_pos = vid_current_pos + (vid_seek_duration * 1000);

			vid_seek_request = true;
		}
		else if(key.p_d_left)
		{
			if(vid_current_pos - (vid_seek_duration * 1000) < 0)
				vid_seek_pos = 0;
			else
				vid_seek_pos = vid_current_pos - (vid_seek_duration * 1000);
			
			vid_seek_request = true;
		}
		else if(key.p_d_up)
		{
			vid_hid_wait = 0;
			Vid_increase_screen_brightness();
		}
		else if(key.h_d_up)
		{
			if(key.held_time > 120)
			{
				for(int i = 0 ; i < 5; i++)
				Vid_increase_screen_brightness();
			}
			else
				Vid_increase_screen_brightness();
		}
		else if(key.p_d_down)
		{
			vid_hid_wait = 0;
			Vid_decrease_screen_brightness();
		}
		else if(key.h_d_down)
		{
			if(key.held_time > 120)
			{
				for(int i = 0 ; i < 5; i++)
					Vid_decrease_screen_brightness();
			}
			else
				Vid_decrease_screen_brightness();
		}

		if(!key.p_touch && !key.h_touch)
		{
			vid_select_audio_track_button.selected = vid_texture_filter_button.selected = vid_allow_skip_frames_button.selected
			= vid_allow_skip_key_frames_button.selected = vid_volume_button.selected = vid_seek_duration_button.selected
			= vid_use_hw_decoding_button.selected = vid_use_hw_color_conversion_button.selected = vid_use_multi_threaded_decoding_button.selected
			= vid_lower_resolution_button.selected = vid_menu_button[0].selected = vid_menu_button[1].selected = vid_menu_button[2].selected 
			= vid_control_button.selected = vid_ok_button.selected = vid_correct_aspect_ratio_button.selected 
			= vid_move_content_button.selected = vid_remember_video_pos_button.selected = Draw_get_bot_ui_button()->selected
			= vid_show_decode_graph_button.selected = vid_show_color_conversion_graph_button.selected = vid_show_packet_buffer_graph_button.selected
			= vid_show_raw_video_buffer_graph_button.selected = vid_show_raw_audio_buffer_graph_button.selected = vid_menu_background.selected 
			= vid_scroll_bar.selected = vid_playback_mode_button.selected = vid_select_subtitle_track_button.selected = false;

			for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
				vid_audio_track_button[i].selected = false;

			for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
				vid_subtitle_track_button[i].selected = false;
		}
	}
	
	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}

void Vid_decode_thread(void* arg)
{
	Util_log_save(DEF_VID_DECODE_THREAD_STR, "Thread started.");

	bool key_frame = false;
	int wait_count = 0;
	int audio_track = 0;
	int subtitle_track = 0;
	int audio_size = 0;
	int packet_index = 0;
	int num_of_audio_tracks = 0;
	int num_of_video_tracks = 0;
	int num_of_subtitle_tracks = 0;
	int max_buffer = 0;
	int free_ram = 0;
	int type = DEF_DECODER_PACKET_TYPE_UNKNOWN;
	double pos = 0;
	double saved_pos = 0;
	u8 dummy = 0;
	u8* audio = NULL;
	u8* saved_data = NULL;
	u32 read_size = 0;
	std::string cache_file_name = "";
	std::string cache = "";
	TickCounter counter;
	Result_with_string result;
	osTickCounterStart(&counter);
	
	Util_file_save_to_file(".", DEF_MAIN_DIR + "saved_pos/", &dummy, 1, true);//create directory

	while (vid_thread_run)
	{
		if(vid_play_request || vid_change_video_request)
		{
			aptSetSleepAllowed(false);
			key_frame = false;
			wait_count = 0;
			audio_track = 0;
			subtitle_track = 0;
			audio_size = 0;
			packet_index = 0;
			num_of_audio_tracks = 0;
			num_of_video_tracks = 0;
			num_of_subtitle_tracks = 0;
			max_buffer = 0;
			free_ram = 0;
			type = DEF_DECODER_PACKET_TYPE_UNKNOWN;
			pos = 0;
			saved_pos = 0;
			audio = NULL;
			saved_data = NULL;
			read_size = 0;
			cache_file_name = "";
			cache = vid_dir + vid_file;
			
			Vid_init_variable();
			vid_change_video_request = false;
			vid_play_request = true;

			//generate cache file name
			for(int i = 0; i < (int)cache.length(); i++)
			{
				if(cache.substr(i, 1) == "/")
					cache_file_name += "_";
				else
					cache_file_name += cache.substr(i, 1);
			}

			result = Util_decoder_open_file(vid_dir + vid_file, &num_of_audio_tracks, &num_of_video_tracks, &num_of_subtitle_tracks, 0);
			Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_open_file()..." + result.string + result.error_description, result.code);
			if(result.code != 0)
				vid_play_request = false;

			if(vid_play_request)
			{
				//Overwirte number of tracks if disable flag is set
				if(vid_disable_audio_mode)
					num_of_audio_tracks = 0;
				if(vid_disable_video_mode)
					num_of_video_tracks = 0;
				if(vid_disable_subtitle_mode)
					num_of_subtitle_tracks = 0;

				//Can't play subtitle alone
				if(num_of_audio_tracks == 0 && num_of_video_tracks == 0)
				{
					Util_err_set_error_message(DEF_ERR_OTHER_STR, "No playable media. ", DEF_VID_DECODE_THREAD_STR, DEF_ERR_OTHER);
					Util_err_set_error_show_flag(true);
				}
			}

			if(num_of_audio_tracks > 0 && vid_play_request)
			{
				result = Util_speaker_init();
				Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_speaker_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
				{
					Util_err_set_error_message(result.string, "You have to run dsp1 in order to listen to audio.\n(https://github.com/zoogie/DSP1/releases)", DEF_VID_DECODE_THREAD_STR, result.code);
					Util_err_set_error_show_flag(true);
				}

				vid_num_of_audio_tracks = num_of_audio_tracks;
				result = Util_audio_decoder_init(num_of_audio_tracks, 0);
				Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_audio_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_play_request = false;
				
				if(vid_play_request)
				{
					for(int i = 0; i < num_of_audio_tracks; i++)
					{
						Util_audio_decoder_get_info(&vid_audio_info, i, 0);
						vid_audio_track_lang[i] = vid_audio_info.track_lang;
					}

					vid_duration = vid_audio_info.duration;
					//if there is more than 1 audio tracks, select a audio track
					if(num_of_audio_tracks > 1)
					{
						Vid_exit_full_screen();
						vid_select_audio_track_request = true;
						while(vid_select_audio_track_request)
							usleep(16600);
					}

					audio_track = vid_selected_audio_track;
					Util_audio_decoder_get_info(&vid_audio_info, audio_track, 0);
					vid_duration *= 1000;
					result = Util_speaker_set_audio_info(0, vid_audio_info.ch, vid_audio_info.sample_rate);
					Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_speaker_set_audio_info()..." + result.string + result.error_description, result.code);
					if(result.code == DEF_ERR_INVALID_ARG)
					{
						Util_err_set_error_message(result.string, "Unsupported audio.", DEF_VID_DECODE_THREAD_STR, result.code);
						Util_err_set_error_show_flag(true);
					}
				}
			}
			if(num_of_video_tracks > 0 && vid_play_request)
			{
				vid_num_of_video_tracks = num_of_video_tracks;
				result = Util_video_decoder_init(vid_lower_resolution, num_of_video_tracks, vid_use_multi_threaded_decoding_request ? vid_num_of_threads : 1, vid_use_multi_threaded_decoding_request ? vid_request_thread_mode : 0, 0);
				Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_video_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_play_request = false;
				
				if(vid_play_request)
				{
					Util_video_decoder_get_info(&vid_video_info, 0, 0);
					vid_duration = vid_video_info.duration;
					vid_video_info.sar_height = vid_video_info.sar_height;
					
					//use sar 1:2 if 800x240 and no sar value is set
					if(vid_video_info.width == 800 && vid_video_info.height == 240 && vid_video_info.sar_width == 1 && vid_video_info.sar_height == 1)
						vid_video_info.sar_height = 2;
					
					vid_duration *= 1000;

					if(vid_video_info.framerate == 0)
						vid_frametime = 0;
					else
						vid_frametime = (1000.0 / vid_video_info.framerate);

					if(num_of_video_tracks == 2 && vid_frametime != 0)
						vid_frametime /= 2;

					vid_codec_width = vid_video_info.width;
					vid_codec_height = vid_video_info.height;

					//av1 buffer size needs to be multiple of 128
					if(vid_video_info.format_name == "dav1d AV1 decoder by VideoLAN")
					{
						if(vid_codec_width % 128 != 0)
							vid_codec_width += 128 - vid_codec_width % 128;
						if(vid_codec_height % 128 != 0)
							vid_codec_height += 128 - vid_codec_height % 128;
					}
					else
					{
						if(vid_codec_width % 16 != 0)
							vid_codec_width += 16 - vid_codec_width % 16;
						if(vid_codec_height % 16 != 0)
							vid_codec_height += 16 - vid_codec_height % 16;
					}

					if(vid_use_hw_decoding_request && vid_video_info.format_name == "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10")
					{
						vid_hw_decoding_mode = true;
						result = Util_mvd_video_decoder_init(0);
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_mvd_video_decoder_init()..." + result.string + result.error_description, result.code);
						if(result.code != 0)
							vid_play_request = false;
					}

					if(!vid_hw_decoding_mode && vid_use_hw_color_conversion_request)
					{
						if(vid_codec_width <= 1024 && vid_codec_height <= 1024)
						{
							vid_hw_color_conversion_mode = true;
							result = Util_converter_y2r_init();
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_converter_y2r_init()..." + result.string + result.error_description, result.code);
							if(result.code != 0)
								vid_play_request = false;
						}
					}
				}

				if(vid_play_request)
				{
					for(int k = 0; k < num_of_video_tracks; k++)
					{
						for(int s = 0; s < DEF_VID_BUFFERS; s++)
						{
							result = Draw_texture_init(&vid_image[0][s][k], 1024, 1024, DEF_DRAW_FORMAT_RGB565);
							if(result.code != 0)
							{
								k = num_of_video_tracks;
								s = DEF_VID_BUFFERS;
								break;
							}
							else
								vid_image_enabled[0][s][k] = true;

							if(vid_codec_width > 1024)
							{
								result = Draw_texture_init(&vid_image[1][s][k], 1024, 1024, DEF_DRAW_FORMAT_RGB565);
								if(result.code != 0)
								{
									k = num_of_video_tracks;
									s = DEF_VID_BUFFERS;
									break;
								}
								else
									vid_image_enabled[1][s][k] = true;
							}

							if(vid_codec_height > 1024)
							{
								result = Draw_texture_init(&vid_image[2][s][k], 1024, 1024, DEF_DRAW_FORMAT_RGB565);
								if(result.code != 0)
								{
									k = num_of_video_tracks;
									s = DEF_VID_BUFFERS;
									break;
								}
								else
									vid_image_enabled[2][s][k] = true;
							}

							if(vid_codec_width > 1024 && vid_codec_height > 1024)
							{
								result = Draw_texture_init(&vid_image[3][s][k], 1024, 1024, DEF_DRAW_FORMAT_RGB565);
								if(result.code != 0)
								{
									k = num_of_video_tracks;
									s = DEF_VID_BUFFERS;
									break;
								}
								else
									vid_image_enabled[3][s][k] = true;
							}
						}
					}

					if(result.code != 0)
					{
						result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
						result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
						vid_play_request = false;
					}
					else
					{
						for(int k = 0; k < 2; k++)
						{
							for(int i = 0; i < 4; i++)
							{
								for(int s = 0; s < DEF_VID_BUFFERS; s++)
								{
									if(vid_image_enabled[i][s][k])
										Draw_set_texture_filter(&vid_image[i][s][k], vid_linear_filter);
								}
							}
						}

						if((var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL) && (vid_video_info.thread_type == DEF_DECODER_THREAD_TYPE_NONE || vid_hw_decoding_mode))
							APT_SetAppCpuTimeLimit(20);
						else
							APT_SetAppCpuTimeLimit(80);

						if(vid_codec_width > 1024 && vid_codec_height > 1024)
						{
							vid_tex_width[0] = 1024;
							vid_tex_width[1] = vid_video_info.width - 1024;
							vid_tex_width[2] = 1024;
							vid_tex_width[3] = vid_video_info.width - 1024;
							vid_tex_height[0] = 1024;
							vid_tex_height[1] = 1024;
							vid_tex_height[2] = vid_video_info.height - 1024;
							vid_tex_height[3] = vid_video_info.height - 1024;
						}
						else if(vid_codec_width > 1024)
						{
							vid_tex_width[0] = 1024;
							vid_tex_width[1] = vid_video_info.width - 1024;
							vid_tex_height[0] = vid_video_info.height;
							vid_tex_height[1] = vid_video_info.height;
						}
						else if(vid_codec_height > 1024)
						{
							vid_tex_width[0] = vid_video_info.width;
							vid_tex_width[2] = vid_video_info.width;
							vid_tex_height[0] = 1024;
							vid_tex_height[2] = vid_video_info.height - 1024;
						}
						else
						{
							vid_tex_width[0] = vid_video_info.width;
							vid_tex_height[0] = vid_video_info.height;
						}
					}
				}
			}
			if(num_of_subtitle_tracks > 0 && vid_play_request)
			{
				vid_num_of_subtitle_tracks = num_of_subtitle_tracks;
				result = Util_subtitle_decoder_init(num_of_subtitle_tracks, 0);
				Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_subtitle_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_play_request = false;

				if(vid_play_request)
				{
					for(int i = 0; i < num_of_subtitle_tracks; i++)
					{
						Util_subtitle_decoder_get_info(&vid_subtitle_info, i, 0);
						vid_subtitle_track_lang[i] = vid_subtitle_info.track_lang;
					}

					//if there is more than 1 subtitle tracks, select a subtitle track
					if(num_of_subtitle_tracks > 1)
					{
						Vid_exit_full_screen();
						vid_select_subtitle_track_request = true;
						while(vid_select_subtitle_track_request)
							usleep(16600);
					}

					subtitle_track = vid_selected_subtitle_track;
				}
			}

			//Enter full screen mode if file has video track
			if(num_of_video_tracks > 0 && !Util_err_query_error_show_flag())
			{
				Vid_fit_to_screen(400, 240);
				if(!vid_full_screen_mode)
					Vid_enter_full_screen(300);
			}
			else
			{
				Vid_fit_to_screen(400, 225);
				Vid_exit_full_screen();
			}

			//if remember video pos is set, try to find previous playback position
			if(vid_remember_video_pos_mode && vid_play_request)
			{
				result = Util_file_load_from_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/", &saved_data, sizeof(double), &read_size);
				if(result.code == 0)
				{
					saved_pos = *(double*)saved_data;
					if(saved_pos > 0 && saved_pos < vid_duration)
					{
						vid_seek_pos = saved_pos;
						vid_convert_request = true;
						vid_seek_request = true;
					}
				}
				result.code = 0;
				Util_safe_linear_free(saved_data);
				saved_data = NULL;
			}

			if(vid_play_request)
			{
				if(vid_num_of_video_tracks > 0)
				{
					//to prevent out of memory in other task(e.g. compressed packet buffer, audio decoder), keep 10MB
					free_ram = Util_check_free_linear_space();
					free_ram -= (1024 * 1024 * 10);

					if(vid_hw_decoding_mode)
						max_buffer = free_ram / (vid_codec_width * vid_codec_height * 2);
					else
					{
						//(+ raw_picture_size * num_of_thread if hw decoding is not enabled)
						free_ram -= (vid_codec_width * vid_codec_height * 1.5 * (vid_video_info.thread_type != DEF_DECODER_THREAD_TYPE_NONE ? vid_num_of_threads : 1));
						max_buffer = free_ram / (vid_codec_width * vid_codec_height * 1.5) / vid_num_of_video_tracks;
					}

					if(max_buffer > DEF_DECODER_MAX_RAW_IMAGE)
						max_buffer = DEF_DECODER_MAX_RAW_IMAGE;
					if(max_buffer < 3)
					{
						result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
						result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
						vid_play_request = false;
					}
					else
					{
						if(vid_hw_decoding_mode)
							Util_mvd_video_decoder_set_raw_image_buffer_size(max_buffer, 0);
						else
						{
							for(int i = 0; i < vid_num_of_video_tracks; i++)
								Util_video_decoder_set_raw_image_buffer_size(max_buffer, i, 0);
						}
					}
					Util_log_save(DEF_VID_DECODE_THREAD_STR, "max raw buffer(per track) : " + std::to_string(max_buffer));
				}

				vid_read_packet_request = true;
			}

			//Wait for packet thread
			while(vid_play_request)
			{
				usleep(10000);
				if(Util_decoder_get_available_packet_num(0) > 0)
					break;
			}

			//Don't start audio playback until first video frame is ready
			if(vid_num_of_video_tracks > 0)
				Util_speaker_pause(0);
			
			if(result.code != 0)
			{
				Util_err_set_error_message(result.string, result.error_description, DEF_VID_DECODE_THREAD_STR, result.code);
				Util_err_set_error_show_flag(true);
				var_need_reflesh = true;
			}

			while(vid_play_request)
			{
				while(vid_decode_video_request && vid_play_request && !vid_change_video_request)
					usleep(2000);

				while(true)
				{
					result = Util_decoder_parse_packet(&type, &packet_index, &key_frame, 0);
					if(result.code != DEF_ERR_TRY_AGAIN || !vid_play_request || vid_change_video_request || (result.code == DEF_ERR_TRY_AGAIN && vid_eof))
						break;
					else
					{
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_parse_packet()..." + result.string + result.error_description, result.code);
						usleep(4000);
					}
				}

				if(num_of_video_tracks < 1 && vid_pause_request)
				{
					Util_speaker_pause(0);
					aptSetSleepAllowed(true);
					while(vid_pause_request && vid_play_request && !vid_seek_request && !vid_change_video_request)
						usleep(5000);
					
					Util_speaker_resume(0);
					aptSetSleepAllowed(false);
				}

				var_afk_time = 0;

				//change audio track
				if(!vid_select_audio_track_request && audio_track != vid_selected_audio_track)
				{
					audio_track = vid_selected_audio_track;
					Util_speaker_clear_buffer(0);
					Util_audio_decoder_get_info(&vid_audio_info, audio_track, 0);
					vid_duration = vid_audio_info.duration * 1000;
					Util_speaker_set_audio_info(0, vid_audio_info.ch, vid_audio_info.sample_rate);
				}

				//change subtitle track
				if(!vid_select_subtitle_track_request && subtitle_track != vid_selected_subtitle_track)
				{
					subtitle_track = vid_selected_subtitle_track;
					for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_DATA; i++)
					{
						vid_subtitle_data[i].text = "";
						vid_subtitle_data[i].start_time = 0;
						vid_subtitle_data[i].end_time = 0;
					}
				}

				//Handle seek adjust request
				if(vid_seek_adjust_request && (num_of_video_tracks == 0 || type == DEF_DECODER_PACKET_TYPE_VIDEO || vid_frametime == 0))
				{
					if(wait_count <= 0)
					{
						if(vid_current_pos >= vid_seek_pos)
							vid_seek_adjust_request = false;
					}
					else
						wait_count--;
				}

				//Handle seek request
				if(vid_seek_request)
				{
					Util_speaker_clear_buffer(0);

					vid_clear_cache_packet_request = true;
					while (vid_clear_cache_packet_request && vid_play_request && !vid_change_video_request)
						usleep(10000);

					if(num_of_video_tracks > 0)
					{
						vid_clear_raw_buffer_request[0] = true; 
						while ((vid_clear_raw_buffer_request[0] || vid_clear_raw_buffer_request[1]) && vid_play_request && !vid_change_video_request)
							usleep(10000);
					}
										
					if(result.code == 0)
					{
						vid_eof = false;
						vid_read_packet_request = true;
						vid_seek_adjust_request = true;
						//sometimes cached previous frames so ignore first 4 (+ num_of_threads if frame threading is used) frames
						wait_count = 4 + (vid_video_info.thread_type == DEF_DECODER_THREAD_TYPE_FRAME ? vid_num_of_threads : 0);
					}

					vid_seek_request = false;
				}

				if(!vid_play_request || vid_change_video_request || result.code != 0)
				{
					if(result.code != 0)
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_parse_packet()..." + result.error_description, result.code);
					//if remember video pos is set, save playback position
					if((!vid_play_request || vid_change_video_request) && vid_remember_video_pos_mode)
					{
						saved_pos = vid_current_pos;
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "last pos : " + Util_convert_seconds_to_time(saved_pos / 1000));
						Util_file_save_to_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/", (u8*)(&saved_pos), sizeof(double), true);
					}
					else if(vid_eof && vid_remember_video_pos_mode)//if playback end due to read packet error(EOF), remove saved time
						Util_file_delete_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/");

					break;
				}

				if(type == DEF_DECODER_PACKET_TYPE_UNKNOWN)
					Util_log_save("debug", "unknown packet!!!!!");
				else if(type == DEF_DECODER_PACKET_TYPE_AUDIO && packet_index == audio_track 
				&& (!vid_seek_adjust_request || num_of_video_tracks == 0 || vid_frametime == 0) && !vid_disable_audio_mode)//Always decode audio if video framerate is unknown or file does not have video track
				{
					result = Util_decoder_ready_audio_packet(audio_track, 0);
					if(result.code == 0)
					{
						osTickCounterUpdate(&counter);
						result = Util_audio_decoder_decode(&audio_size, &audio, &pos, audio_track, 0);
						osTickCounterUpdate(&counter);
						vid_audio_time = osTickCounterRead(&counter);
						
						//Get position from audio if video framerate is unknown or or file does not have video track
						if((num_of_video_tracks == 0 || vid_frametime == 0) && !std::isnan(pos) && !std::isinf(pos))
							vid_current_pos = pos - (Util_speaker_get_available_buffer_size(0) / 2.0 / vid_audio_info.ch / vid_audio_info.sample_rate * 1000);
						
						if(result.code == 0 && !vid_seek_adjust_request)
						{
							vid_too_big = false;
							if(vid_volume != 100)
							{
								for(int i = 0; i < audio_size; i += 2)
								{
 									if(*(s16*)(audio + i) * ((double)vid_volume / 100) > INT16_MAX)
									{
										*(s16*)(audio + i) = INT16_MAX;
										vid_too_big = true;
									}
									else
										*(s16*)(audio + i) = *(s16*)(audio + i) * ((double)vid_volume / 100);
								}

								if(vid_too_big)
									Util_log_save(DEF_VID_DECODE_THREAD_STR, "Volume is too big!!!");
							}

							while(true)
							{
								result = Util_speaker_add_buffer(0, audio, audio_size);
								if(result.code != DEF_ERR_TRY_AGAIN || !vid_play_request || vid_seek_request || vid_change_video_request)
									break;
								
								usleep(2000);
							}
						}
						else
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_audio_decoder_decode()..." + result.string + result.error_description, result.code);

						Util_safe_linear_free(audio);
						audio = NULL;
					}
					else
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_ready_audio_packet()..." + result.string + result.error_description, result.code);
				}
				else if(type == DEF_DECODER_PACKET_TYPE_AUDIO)
					Util_decoder_skip_audio_packet(packet_index, 0);
				else if(type == DEF_DECODER_PACKET_TYPE_SUBTITLE && packet_index == subtitle_track && !vid_seek_adjust_request && !vid_disable_subtitle_mode)
				{
					result = Util_decoder_ready_subtitle_packet(packet_index, 0);
					if(result.code == 0)
					{
						result = Util_subtitle_decoder_decode(&vid_subtitle_data[vid_subtitle_index], packet_index, 0);
						if(result.code == 0)
						{
							if(vid_subtitle_index + 1 >= DEF_DECODER_MAX_SUBTITLE_DATA)
								vid_subtitle_index = 0;
							else
								vid_subtitle_index++;
						}
						else
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_subtitle_decoder_decode()..." + result.string + result.error_description, result.code);
					}
					else
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_ready_subtitle_packet()..." + result.string + result.error_description, result.code);

				}
				else if(type == DEF_DECODER_PACKET_TYPE_SUBTITLE)
					Util_decoder_skip_subtitle_packet(packet_index, 0);
				else if(type == DEF_DECODER_PACKET_TYPE_VIDEO && (!vid_hw_decoding_mode || (vid_hw_decoding_mode && packet_index == 0)) && !vid_disable_video_mode)
				{
					vid_packet_index = packet_index;
					vid_key_frame = key_frame;
					vid_decode_video_request = true;
				}
				else if(type == DEF_DECODER_PACKET_TYPE_VIDEO)
					Util_decoder_skip_video_packet(packet_index, 0);
			}
			
			//wait for read packet thread
			vid_read_packet_request = false;
			while(vid_read_packet_wait_request)
				usleep(10000);

			//wait for decode video thread
			vid_decode_video_request = false;
			while(vid_decode_video_wait_request)
				usleep(10000);

			//wait for speaker
			if(num_of_audio_tracks > 0)
			{
				while(Util_speaker_is_playing(0) && vid_play_request && !vid_change_video_request)
				{
					vid_current_pos = vid_duration - (Util_speaker_get_available_buffer_size(0) / 2.0 / vid_audio_info.ch / vid_audio_info.sample_rate * 1000);
					usleep(10000);
				}
				
				Util_speaker_exit();
			}

			//wait for video
			if(num_of_video_tracks > 0)
			{
				if(vid_hw_decoding_mode)
				{
					while(Util_mvd_video_decoder_get_available_raw_image_num(0) && vid_play_request && !vid_change_video_request)
						usleep(10000);
				}
				else
				{
					while(Util_video_decoder_get_available_raw_image_num(0, 0) && vid_play_request && !vid_change_video_request)
						usleep(10000);
				}
			}

			//wait for conversion thread
			vid_convert_request = false;
			while(vid_convert_wait_request)
				usleep(100000);


			Util_decoder_close_file(0);
			if(vid_hw_color_conversion_mode)
				Util_converter_y2r_exit();
			
			for(int k = 0; k < 2; k++)
			{
				for(int i = 0; i < 4; i++)
				{
					for(int s = 0; s < DEF_VID_BUFFERS; s++)
					{
						if(vid_image_enabled[i][s][k])
						{
							Draw_texture_free(&vid_image[i][s][k]);
							vid_image_enabled[i][s][k] = false;
						}
					}
				}
			}

			vid_pause_request = false;
			vid_seek_adjust_request = false;
			vid_seek_request = false;
			vid_show_full_screen_msg = false;

			if(!vid_change_video_request && vid_playback_mode == DEF_VID_NO_REPEAT)
				vid_play_request = false;

			if(!vid_play_request)
			{
				Vid_exit_full_screen();
				aptSetSleepAllowed(true);
			}

			if(!vid_change_video_request && vid_play_request && (vid_playback_mode == DEF_VID_IN_ORDER || vid_playback_mode == DEF_VID_RANDOM))
			{
				for(int i = 0; i < (vid_playback_mode == DEF_VID_RANDOM ? Util_expl_query_num_of_file() + 256 : Util_expl_query_num_of_file()); i++)
				{
					if(vid_file_index + 1 >= Util_expl_query_num_of_file())
						vid_file_index = 0;
					else
						vid_file_index++;
					
					if(Util_expl_query_type(vid_file_index) & DEF_EXPL_TYPE_FILE)
					{
						srand(time(NULL) + i);
						if(vid_playback_mode == DEF_VID_IN_ORDER)
							break;
						else if(rand() % 5 == 1)//1 in 5
							break;
					}
				}

				vid_file = Util_expl_query_file_name(vid_file_index);
			}
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_VID_DECODE_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Vid_decode_video_thread(void* arg)
{
	Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Thread started.");
	bool key_frame = false;
	int packet_index = 0;
	double skip = 0;
	TickCounter counter;
	Result_with_string result;

	osTickCounterStart(&counter);

	while (vid_thread_run)
	{
		if(vid_play_request)
		{
			skip = 0;
			while(vid_play_request)
			{
				if(vid_clear_raw_buffer_request[1])
				{
					if(vid_hw_decoding_mode)
						Util_mvd_video_decoder_clear_raw_image(0);
					else
					{
						for(int i = 0; i < vid_num_of_video_tracks; i++)
							Util_video_decoder_clear_raw_image(i, 0);
					}

					if(vid_hw_decoding_mode)
						result = Util_mvd_video_decoder_decode(0);
					else
						result = Util_video_decoder_decode(packet_index, 0);

					vid_clear_raw_buffer_request[0] = false;
					vid_clear_raw_buffer_request[1] = false;
				}
				else if(vid_decode_video_request)
				{
					vid_decode_video_wait_request = true;
					packet_index = vid_packet_index;
					key_frame = vid_key_frame;

					if(vid_allow_skip_frames && skip > vid_frametime && (!key_frame || vid_allow_skip_key_frames) && vid_frametime != 0)
					{
						skip -= vid_frametime;
						Util_decoder_skip_video_packet(packet_index, 0);
						vid_decode_video_wait_request = false;
						vid_decode_video_request = false;
					}
					else
					{
						result = Util_decoder_ready_video_packet(packet_index, 0);
						vid_decode_video_request = false;

						if(result.code == 0)
						{
							while(true)
							{
								osTickCounterUpdate(&counter);
								if(vid_hw_decoding_mode)
									result = Util_mvd_video_decoder_decode(0);
								else
									result = Util_video_decoder_decode(packet_index, 0);
								osTickCounterUpdate(&counter);

								if(result.code != DEF_ERR_TRY_AGAIN || !vid_play_request || vid_change_video_request || vid_clear_raw_buffer_request[1])
									break;
								else
									usleep(1000);
							}

							vid_video_time = osTickCounterRead(&counter);

							if(result.code == 0)
							{
								if(!vid_convert_request)
									vid_convert_request = true;
							}
							else
							{
								if(vid_hw_decoding_mode)
									Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_mvd_video_decoder_decode()..." + result.string + result.error_description, result.code);
								else
									Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_video_decoder_decode()..." + result.string + result.error_description, result.code);
							}

							vid_decode_video_wait_request = false;

							vid_key_frame_list[319] = key_frame;
							vid_time[0][319] = vid_video_time;

							if(vid_frametime - vid_video_time < 0 && vid_allow_skip_frames && vid_frametime != 0)
								skip -= vid_frametime - vid_video_time;

							if(vid_min_time > vid_video_time)
								vid_min_time = vid_video_time;
							if(vid_max_time < vid_video_time)
								vid_max_time = vid_video_time;
							
							vid_total_time += vid_video_time;
							vid_total_frames++;

							vid_recent_time[89] = vid_video_time;
							vid_recent_total_time = 0;
							for(int i = 0; i < 90; i++)
								vid_recent_total_time += vid_recent_time[i];

							for(int i = 1; i < 90; i++)
								vid_recent_time[i - 1] = vid_recent_time[i];

							for(int i = 1; i < 320; i++)
							{
								vid_key_frame_list[i - 1] = vid_key_frame_list[i];
								vid_time[0][i - 1] = vid_time[0][i];
							}
						}
						else
						{
							vid_decode_video_wait_request = false;
							Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_decoder_ready_video_packet()..." + result.string + result.error_description, result.code);
						}
					}
				}
				else
					usleep(1000);
			}
		}
		
		usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}
	
	Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Vid_convert_thread(void* arg)
{
	Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Thread started.");
	bool first = false;
	u8* yuv_video = NULL;
	u8* video = NULL;
	int packet_index = 0;
	int image_num = 0;
	double skip = 0;
	double pos = 0;
	TickCounter counter[5];
	Result_with_string result;

	osTickCounterStart(&counter[0]);
	osTickCounterStart(&counter[1]);
	osTickCounterStart(&counter[2]);
	osTickCounterStart(&counter[3]);
	osTickCounterStart(&counter[4]);

	while (vid_thread_run)
	{
		if(vid_play_request)
		{
			skip = 0;
			packet_index = 0;
			pos = 0;
			first = true;
			vid_convert_wait_request = true;
			while(vid_convert_request)
			{
				if(vid_pause_request)
				{
					Util_speaker_pause(0);
					aptSetSleepAllowed(true);
					while(vid_pause_request && vid_play_request && !vid_seek_request && !vid_change_video_request)
						usleep(5000);
					
					Util_speaker_resume(0);
					aptSetSleepAllowed(false);
				}

				if(vid_clear_raw_buffer_request[0])
				{
					vid_clear_raw_buffer_request[1] = true;
					while((vid_clear_raw_buffer_request[0] || vid_clear_raw_buffer_request[1]) && vid_play_request && !vid_change_video_request)
						usleep(10000);
				}
				osTickCounterUpdate(&counter[3]);

				if(first)//start audio playback when first frame is ready
				{
					Util_speaker_resume(0);
					first = false;
				}
				
				//skip video frame if can't keep up
				if((skip > vid_frametime || vid_seek_adjust_request) && vid_frametime != 0)
				{
					if(vid_hw_decoding_mode ? Util_mvd_video_decoder_get_available_raw_image_num(0) > 0 
					: Util_video_decoder_get_available_raw_image_num(packet_index, 0) > 0)
					{
						if(vid_hw_decoding_mode)
							Util_mvd_video_decoder_skip_image(&pos, 0);
						else
							Util_video_decoder_skip_image(&pos, packet_index, 0);

						if(!std::isnan(pos) && !std::isinf(pos))
							vid_current_pos = pos;
					}
					else if(vid_seek_adjust_request)
						usleep(3000);

					if(vid_seek_adjust_request)
						skip = 0;

					osTickCounterUpdate(&counter[3]);
					skip -= vid_frametime - osTickCounterRead(&counter[3]);
				}
				else
				{
					while(true)
					{
						osTickCounterUpdate(&counter[2]);
						if(vid_hw_decoding_mode)
							result = Util_mvd_video_decoder_get_image(&video, &pos, vid_codec_width, vid_codec_height, 0);
						else
							result = Util_video_decoder_get_image(&yuv_video, &pos, vid_codec_width, vid_codec_height, packet_index, 0);
						osTickCounterUpdate(&counter[2]);

						if(result.code != DEF_ERR_TRY_AGAIN || !vid_play_request || vid_change_video_request || (result.code == DEF_ERR_TRY_AGAIN && vid_eof) || vid_clear_raw_buffer_request[0])
							break;
						else if(vid_frametime != 0)
							usleep((vid_frametime / 4) * 1000);
						else//if framerate is unknown just sleep 10ms
							usleep(10000);
					}
					
					vid_copy_time[0] = osTickCounterRead(&counter[2]);

					if(result.code == 0)
					{
						//Get position from video if video framerate is known
						if(!std::isnan(pos) && !std::isinf(pos) && vid_frametime != 0)
							vid_current_pos = pos;

						osTickCounterUpdate(&counter[0]);
						if(!vid_hw_decoding_mode && vid_hw_color_conversion_mode)
							result = Util_converter_y2r_yuv420p_to_rgb565le(yuv_video, &video, vid_codec_width, vid_codec_height, true);
						else if(!vid_hw_decoding_mode)
							result = Util_converter_yuv420p_to_rgb565le_asm(yuv_video, &video, vid_codec_width, vid_codec_height);
						osTickCounterUpdate(&counter[0]);
						vid_convert_time = osTickCounterRead(&counter[0]);

						//set texture data
						if(result.code == 0)
						{
							osTickCounterUpdate(&counter[1]);
							if(packet_index == 0)
								image_num = vid_image_num;
							else if(packet_index == 1)
								image_num = vid_image_num_3d;

							if(!vid_hw_decoding_mode && vid_hw_color_conversion_mode)
								result = Draw_set_texture_data_direct(&vid_image[0][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, DEF_DRAW_FORMAT_RGB565);
							else
								result = Draw_set_texture_data(&vid_image[0][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, DEF_DRAW_FORMAT_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);

							if(vid_codec_width > 1024)
							{
								result = Draw_set_texture_data(&vid_image[1][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 0, 1024, 1024, DEF_DRAW_FORMAT_RGB565);
								if(result.code != 0)
									Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
							}
							if(vid_codec_height > 1024)
							{
								result = Draw_set_texture_data(&vid_image[2][image_num][packet_index], video, vid_codec_width, vid_codec_height, 0, 1024, 1024, 1024, DEF_DRAW_FORMAT_RGB565);
								if(result.code != 0)
									Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
							}
							if(vid_codec_width > 1024 && vid_codec_height > 1024)
							{
								result = Draw_set_texture_data(&vid_image[3][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, 1024, 1024, DEF_DRAW_FORMAT_RGB565);
								if(result.code != 0)
									Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
							}

							//Adjust image size so that user won't see glitch on videos
							if(vid_codec_width > 1024 && vid_codec_height > 1024)
							{
								vid_image[1][image_num][packet_index].subtex->width = (vid_video_info.width - 1024);
								vid_image[1][image_num][packet_index].subtex->right = (vid_video_info.width - 1024) / 1024.0;
								vid_image[2][image_num][packet_index].subtex->height = (vid_video_info.height - 1024);
								vid_image[2][image_num][packet_index].subtex->bottom = 1 - (vid_video_info.height - 1024) / 1024.0;
								vid_image[3][image_num][packet_index].subtex->width = (vid_video_info.width - 1024);
								vid_image[3][image_num][packet_index].subtex->right = (vid_video_info.width - 1024) / 1024.0;
								vid_image[3][image_num][packet_index].subtex->height = (vid_video_info.height - 1024);
								vid_image[3][image_num][packet_index].subtex->bottom = 1 - (vid_video_info.height - 1024) / 1024.0;
							}
							else if(vid_codec_width > 1024)
							{
								vid_image[0][image_num][packet_index].subtex->height = vid_video_info.height;
								vid_image[0][image_num][packet_index].subtex->bottom = 1 - vid_video_info.height / 1024.0;
								vid_image[1][image_num][packet_index].subtex->width = (vid_video_info.width - 1024);
								vid_image[1][image_num][packet_index].subtex->height = vid_video_info.height;
								vid_image[1][image_num][packet_index].subtex->right = (vid_video_info.width - 1024) / 1024.0;
								vid_image[1][image_num][packet_index].subtex->bottom = 1 - vid_video_info.height / 1024.0;
							}
							else if(vid_codec_height > 1024)
							{
								vid_image[0][image_num][packet_index].subtex->width = vid_video_info.width;
								vid_image[0][image_num][packet_index].subtex->right = vid_video_info.width / 1024.0;
								vid_image[2][image_num][packet_index].subtex->width = vid_video_info.width;
								vid_image[2][image_num][packet_index].subtex->height = (vid_video_info.height - 1024);
								vid_image[2][image_num][packet_index].subtex->right = vid_video_info.width / 1024.0;
								vid_image[2][image_num][packet_index].subtex->bottom = 1 - (vid_video_info.height - 1024) / 1024.0;
							}
							else
							{
								vid_image[0][image_num][packet_index].subtex->width = vid_video_info.width;
								vid_image[0][image_num][packet_index].subtex->height = vid_video_info.height;
								vid_image[0][image_num][packet_index].subtex->right = vid_video_info.width / 1024.0;
								vid_image[0][image_num][packet_index].subtex->bottom = 1 - vid_video_info.height / 1024.0;
							}

							osTickCounterUpdate(&counter[1]);
							vid_copy_time[1] = osTickCounterRead(&counter[1]);

							if(packet_index == 0)
							{
								if(image_num + 1 < DEF_VID_BUFFERS)
									vid_image_num++;
								else
									vid_image_num = 0;
							}
							else if(packet_index == 1)
							{
								if(image_num + 1 < DEF_VID_BUFFERS)
									vid_image_num_3d++;
								else
									vid_image_num_3d = 0;
							}

							if(packet_index == 0)
								var_need_reflesh = true;
							
							if(vid_num_of_video_tracks == 2)
								packet_index = !packet_index;
						}
						else
							Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Util_converter_yuv420p_to_bgr565()..." + result.string + result.error_description, result.code);
					}
					else
					{
						if(vid_hw_decoding_mode)
							Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Util_mvd_video_decoder_get_image()..." + result.string + result.error_description, result.code);
						else
							Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Util_video_decoder_get_image()..." + result.string + result.error_description, result.code);
					}

					Util_safe_linear_free(yuv_video);
					Util_safe_linear_free(video);
					yuv_video = NULL;
					video = NULL;

					for(int i = 1; i < 320; i++)
						vid_time[1][i - 1] = vid_time[1][i];

					osTickCounterUpdate(&counter[3]);
					vid_time[1][319] = osTickCounterRead(&counter[3]);

					if(vid_frametime - vid_time[1][319] > 0 && vid_frametime != 0)
					{
						osTickCounterUpdate(&counter[4]);
						usleep((vid_frametime - vid_time[1][319]) * 1000);
						osTickCounterUpdate(&counter[4]);
						skip -= (vid_frametime - vid_time[1][319]) - osTickCounterRead(&counter[4]);
					}
					else if(vid_frametime != 0)
						skip -= vid_frametime - vid_time[1][319];
				}
			}
			vid_convert_wait_request = false;
		}
		
		usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Vid_read_packet_thread(void* arg)
{
	Util_log_save(DEF_VID_READ_PACKET_THREAD_STR, "Thread started.");

	Result_with_string result;

	while (vid_thread_run)
	{	
		while(vid_play_request)
		{
			if(vid_read_packet_request)
			{
				vid_read_packet_wait_request = true;
				while(vid_play_request && !vid_change_video_request && !vid_eof && vid_read_packet_request)
				{
					if(vid_clear_cache_packet_request)
					{
						result = Util_decoder_seek(vid_seek_pos, DEF_DECODER_SEEK_FLAG_BACKWARD, 0);
						Util_log_save(DEF_VID_READ_PACKET_THREAD_STR, "Util_decoder_seek()..." + result.string + result.error_description, result.code);
						Util_decoder_clear_cache_packet(0);
						vid_clear_cache_packet_request = false;
					}
					else
					{
						result = Util_decoder_read_packet(0);
						if(result.code == DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS)
						{
							Util_log_save(DEF_VID_READ_PACKET_THREAD_STR, "Util_decoder_read_packet()..." + result.string + result.error_description, result.code);
							vid_eof = true;
						}
						else if(result.code != 0)
							usleep(4000);
					}
				}
				vid_read_packet_wait_request = false;
				vid_read_packet_request = false;
			}
			else
				usleep(10000);
		}
		
		usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_VID_READ_PACKET_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Vid_resume(void)
{
	vid_thread_suspend = false;
	vid_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Vid_suspend(void)
{
	vid_thread_suspend = true;
	vid_main_run = false;
	Menu_resume();
}

Result_with_string Vid_load_msg(std::string lang)
{
	return Util_load_msg("vid_" + lang + ".txt", vid_msg, DEF_VID_NUM_OF_MSG);
}

void Vid_init(void)
{
	Util_log_save(DEF_VID_INIT_STR, "Initializing...");
	bool frame_cores[4] = { false, false, false, false, };
	bool slice_cores[4] = { false, false, false, false, };
	u8* cache = NULL;
	u32 read_size = 0;
	std::string out_data[16];
	Result_with_string result;

	vid_thread_run = true;
	if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
	{
		vid_num_of_threads = 1;
		frame_cores[0] = false;
		frame_cores[1] = true;
		frame_cores[2] = var_core_2_available;
		frame_cores[3] = var_core_3_available;

		slice_cores[0] = false;
		slice_cores[1] = true;
		slice_cores[2] = false;
		slice_cores[3] = var_core_3_available;

		vid_num_of_threads += var_core_2_available;
		vid_num_of_threads += var_core_3_available;

		Util_video_decoder_set_enabled_cores(frame_cores, slice_cores);
		vid_decode_thread = threadCreate(Vid_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_decode_video_thread = threadCreate(Vid_decode_video_thread, (void*)(""), 1024 * 1024, DEF_THREAD_PRIORITY_HIGH, var_core_2_available ? 2 : 0, false);
		vid_convert_thread = threadCreate(Vid_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 0, false);
		vid_read_packet_thread = threadCreate(Vid_read_packet_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 1, false);
	}
	else
	{
		vid_num_of_threads = 2;
		frame_cores[0] = true;
		frame_cores[1] = true;

		slice_cores[0] = false;
		slice_cores[1] = true;

		Util_video_decoder_set_enabled_cores(frame_cores, slice_cores);
		vid_decode_thread = threadCreate(Vid_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_decode_video_thread = threadCreate(Vid_decode_video_thread, (void*)(""), 1024 * 1024, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_convert_thread = threadCreate(Vid_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
		vid_read_packet_thread = threadCreate(Vid_read_packet_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 1, false);
	}

	Vid_init_variable();
	Vid_exit_full_screen();
	vid_show_full_screen_msg = true;
	vid_show_screen_brightness_until = 0;
	vid_menu_mode = DEF_VID_MENU_NONE;
	vid_file = "";
	vid_dir = "";

	result = Util_file_load_from_file("vid_settings.txt", DEF_MAIN_DIR, &cache, 0x1000, &read_size);
	Util_log_save(DEF_VID_INIT_STR, "Util_file_load_from_file()..." + result.string + result.error_description, result.code);

	if(result.code == 0)
	{
		result = Util_parse_file((char*)cache, 16, out_data);//settings file for v1.5.0
		Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 13, out_data);//settings file for v1.4.2
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
		}
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 12, out_data);//settings file for v1.3.2, v1.3.3, v1.4.0 and v1.4.1
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[12] = "0";
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
		}
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 9, out_data);//settings file for v1.3.1
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[9] = "0";
			out_data[10] = "0";
			out_data[11] = "1";
			out_data[12] = "0";
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
		}
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 7, out_data);//settings file for v1.3.0
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[7] = "100";
			out_data[8] = "10";
			out_data[9] = "0";
			out_data[10] = "0";
			out_data[11] = "1";
			out_data[12] = "0";
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
		}
	}

	if(result.code == 0)
	{
		vid_linear_filter = (out_data[0] == "1");
		vid_allow_skip_frames = (out_data[1] == "1");
		vid_allow_skip_key_frames = (out_data[2] == "1");
		vid_use_hw_decoding_request = (out_data[3] == "1");
		vid_use_hw_color_conversion_request = (out_data[4] == "1");
		vid_use_multi_threaded_decoding_request = (out_data[5] == "1");
		vid_lower_resolution = atoi((char*)out_data[6].c_str());
		vid_volume = atoi((char*)out_data[7].c_str());
		vid_seek_duration = atoi((char*)out_data[8].c_str());
		vid_correct_aspect_ratio_mode = (out_data[9] == "1");
		vid_move_content_mode = atoi((char*)out_data[10].c_str());
		vid_remember_video_pos_mode = (out_data[11] == "1");
		vid_playback_mode = atoi((char*)out_data[12].c_str());
		vid_disable_audio_mode = (out_data[13] == "1");
		vid_disable_video_mode = (out_data[14] == "1");
		vid_disable_subtitle_mode = (out_data[15] == "1");
	}

	if(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL)
		vid_use_hw_decoding_request = false;

	if(!vid_allow_skip_frames)
		vid_allow_skip_key_frames = false;

	if(vid_lower_resolution > 2 || vid_lower_resolution < 0)
		vid_lower_resolution = 0;

	if(vid_volume > 999 || vid_volume < 0)
		vid_volume = 100;

	if(vid_seek_duration > 99 || vid_seek_duration < 1)
		vid_seek_duration = 10;

	if(vid_playback_mode != DEF_VID_NO_REPEAT && vid_playback_mode != DEF_VID_REPEAT 
		&& vid_playback_mode != DEF_VID_IN_ORDER && vid_playback_mode != DEF_VID_RANDOM)
		vid_playback_mode = DEF_VID_NO_REPEAT;

	if(vid_move_content_mode != DEF_VID_MOVE_BOTH && vid_move_content_mode != DEF_VID_MOVE_VIDEO
	&& vid_move_content_mode != DEF_VID_MOVE_SUBTITLE && vid_move_content_mode != DEF_VID_MOVE_DISABLE)
		vid_move_content_mode = DEF_VID_MOVE_BOTH;

	Util_safe_linear_free(cache);
	cache = NULL;

	vid_banner_texture_num = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/banner.t3x", vid_banner_texture_num, vid_banner, 0, 2);
	Util_log_save(DEF_VID_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	vid_control_texture_num = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/control.t3x", vid_control_texture_num, vid_control, 0, 2);
	Util_log_save(DEF_VID_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	vid_select_audio_track_button.c2d = var_square_image[0];
	vid_texture_filter_button.c2d = var_square_image[0];
	vid_allow_skip_frames_button.c2d = var_square_image[0];
	vid_allow_skip_key_frames_button.c2d = var_square_image[0];
	vid_volume_button.c2d = var_square_image[0];
	vid_seek_duration_button.c2d = var_square_image[0];
	vid_use_hw_decoding_button.c2d = var_square_image[0];
	vid_use_hw_color_conversion_button.c2d = var_square_image[0];
	vid_use_multi_threaded_decoding_button.c2d = var_square_image[0];
	vid_lower_resolution_button.c2d = var_square_image[0];
	vid_menu_button[0].c2d = var_square_image[0];
	vid_menu_button[1].c2d = var_square_image[0];
	vid_menu_button[2].c2d = var_square_image[0];
	vid_control_button.c2d = var_square_image[0];
	vid_ok_button.c2d = var_square_image[0];
	vid_correct_aspect_ratio_button.c2d = var_square_image[0];
	vid_move_content_button.c2d = var_square_image[0];
	vid_remember_video_pos_button.c2d = var_square_image[0];
	vid_show_decode_graph_button.c2d = var_square_image[0];
	vid_show_color_conversion_graph_button.c2d = var_square_image[0];
	vid_show_packet_buffer_graph_button.c2d = var_square_image[0];
	vid_show_raw_video_buffer_graph_button.c2d = var_square_image[0];
	vid_show_raw_audio_buffer_graph_button.c2d = var_square_image[0];
	vid_menu_background.c2d = var_square_image[0];
	vid_scroll_bar.c2d = var_square_image[0];
	vid_playback_mode_button.c2d = var_square_image[0];
	vid_select_subtitle_track_button.c2d = var_square_image[0];
	vid_disable_audio_button.c2d = var_square_image[0];
	vid_disable_video_button.c2d = var_square_image[0];
	vid_disable_subtitle_button.c2d = var_square_image[0];

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		vid_audio_track_button[i].c2d = var_square_image[0];

	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		vid_subtitle_track_button[i].c2d = var_square_image[0];


	Util_add_watch(&vid_pause_request);
	Util_add_watch(&vid_play_request);
	Util_add_watch(&vid_seek_request);
	Util_add_watch(&vid_select_audio_track_request);
	Util_add_watch(&vid_select_subtitle_track_request);
	Util_add_watch(&vid_show_controls_request);
	Util_add_watch(&vid_set_seek_duration_request);
	Util_add_watch(&vid_set_volume_request);
	Util_add_watch(&vid_use_hw_decoding_request);
	Util_add_watch(&vid_use_hw_color_conversion_request);
	Util_add_watch(&vid_use_multi_threaded_decoding_request);

	Util_add_watch(&vid_linear_filter);
	Util_add_watch(&vid_allow_skip_frames);
	Util_add_watch(&vid_allow_skip_key_frames);
	Util_add_watch(&vid_full_screen_mode);
	Util_add_watch(&vid_correct_aspect_ratio_mode);
	Util_add_watch(&vid_move_content_mode);
	Util_add_watch(&vid_remember_video_pos_mode);
	Util_add_watch(&vid_playback_mode);
	Util_add_watch(&vid_menu_mode);
	Util_add_watch(&vid_lower_resolution);
	Util_add_watch(&vid_show_decode_graph_mode);
	Util_add_watch(&vid_show_color_conversion_graph_mode);
	Util_add_watch(&vid_show_packet_buffer_graph_mode);
	Util_add_watch(&vid_show_raw_video_buffer_graph_mode);
	Util_add_watch(&vid_show_raw_audio_buffer_graph_mode);

	Util_add_watch(&vid_ui_y_offset);
	Util_add_watch(&vid_ui_y_max);
	Util_add_watch(&vid_subtitle_x_offset);
	Util_add_watch(&vid_subtitle_y_offset);
	Util_add_watch(&vid_subtitle_zoom);
	Util_add_watch(&vid_video_x_offset);
	Util_add_watch(&vid_video_y_offset);
	Util_add_watch(&vid_video_zoom);
	Util_add_watch(&vid_selected_audio_track);
	Util_add_watch(&vid_selected_subtitle_track);

	Util_add_watch(&vid_ok_button.selected);
	Util_add_watch(&vid_control_button.selected);
	Util_add_watch(&vid_scroll_bar.selected);
	Util_add_watch(&vid_select_audio_track_button.selected);
	Util_add_watch(&vid_select_subtitle_track_button.selected);
	Util_add_watch(&vid_texture_filter_button.selected);
	Util_add_watch(&vid_allow_skip_frames_button.selected);
	Util_add_watch(&vid_allow_skip_key_frames_button.selected);
	Util_add_watch(&vid_volume_button.selected);
	Util_add_watch(&vid_seek_duration_button.selected);
	Util_add_watch(&vid_correct_aspect_ratio_button.selected);
	Util_add_watch(&vid_move_content_button.selected);
	Util_add_watch(&vid_remember_video_pos_button.selected);
	Util_add_watch(&vid_playback_mode_button.selected);
	Util_add_watch(&vid_menu_background.selected);
	Util_add_watch(&vid_disable_audio_button.selected);
	Util_add_watch(&vid_disable_video_button.selected);
	Util_add_watch(&vid_disable_subtitle_button.selected);
	Util_add_watch(&vid_use_hw_decoding_button.selected);
	Util_add_watch(&vid_use_hw_color_conversion_button.selected);
	Util_add_watch(&vid_use_multi_threaded_decoding_button.selected);
	Util_add_watch(&vid_lower_resolution_button.selected);
	Util_add_watch(&vid_show_decode_graph_button.selected);
	Util_add_watch(&vid_show_color_conversion_graph_button.selected);
	Util_add_watch(&vid_show_packet_buffer_graph_button.selected);
	Util_add_watch(&vid_show_raw_video_buffer_graph_button.selected);
	Util_add_watch(&vid_show_raw_audio_buffer_graph_button.selected);

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		Util_add_watch(&vid_audio_track_button[i].selected);

	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_DATA; i++)
		Util_add_watch(&vid_subtitle_track_button[i].selected);

	for(int i = 0; i < 3; i++)
		Util_add_watch(&vid_menu_button[i].selected);

	Vid_resume();
	vid_already_init = true;
	Util_log_save(DEF_VID_INIT_STR, "Initialized.");
}

void Vid_exit(void)
{
	Util_log_save(DEF_VID_EXIT_STR, "Exiting...");
	std::string data = "";
	Result_with_string result;

	vid_already_init = false;
	vid_thread_suspend = false;
	vid_thread_run = false;
	vid_convert_request = false;
	vid_decode_video_request = false;
	vid_play_request = false;
	vid_select_audio_track_request = false;
	vid_select_subtitle_track_request = false;

	data = "<0>" + std::to_string(vid_linear_filter) + "</0><1>" + std::to_string(vid_allow_skip_frames) + "</1><2>"
	+ std::to_string(vid_allow_skip_key_frames) + "</2><3>" + std::to_string(vid_use_hw_decoding_request) + "</3><4>"
	+ std::to_string(vid_use_hw_color_conversion_request) + "</4><5>" + std::to_string(vid_use_multi_threaded_decoding_request) + "</5><6>"
	+ std::to_string(vid_lower_resolution) + "</6><7>"	+ std::to_string(vid_volume) + "</7><8>"
	+ std::to_string(vid_seek_duration) + "</8><9>" + std::to_string(vid_correct_aspect_ratio_mode) + "</9><10>"
	+ std::to_string(vid_move_content_mode) + "</10><11>" + std::to_string(vid_remember_video_pos_mode) + "</11><12>"
	+ std::to_string(vid_playback_mode) + "</12><13>" + std::to_string(vid_disable_audio_mode) + "</13><14>"
	+ std::to_string(vid_disable_video_mode) + "</14><15>" + std::to_string(vid_disable_subtitle_mode) + "</15>";
	Util_file_save_to_file("vid_settings.txt", DEF_MAIN_DIR, (u8*)data.c_str(), data.length(), true);

	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_decode_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_decode_video_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_convert_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_read_packet_thread, DEF_THREAD_WAIT_TIME));
	threadFree(vid_decode_thread);
	threadFree(vid_decode_video_thread);
	threadFree(vid_convert_thread);
	threadFree(vid_read_packet_thread);

	Draw_free_texture(vid_banner_texture_num);
	Draw_free_texture(vid_control_texture_num);
	
	Util_remove_watch(&vid_pause_request);
	Util_remove_watch(&vid_play_request);
	Util_remove_watch(&vid_seek_request);
	Util_remove_watch(&vid_select_audio_track_request);
	Util_remove_watch(&vid_select_subtitle_track_request);
	Util_remove_watch(&vid_show_controls_request);
	Util_remove_watch(&vid_set_seek_duration_request);
	Util_remove_watch(&vid_set_volume_request);
	Util_remove_watch(&vid_use_hw_decoding_request);
	Util_remove_watch(&vid_use_hw_color_conversion_request);
	Util_remove_watch(&vid_use_multi_threaded_decoding_request);

	Util_remove_watch(&vid_linear_filter);
	Util_remove_watch(&vid_allow_skip_frames);
	Util_remove_watch(&vid_allow_skip_key_frames);
	Util_remove_watch(&vid_full_screen_mode);
	Util_remove_watch(&vid_correct_aspect_ratio_mode);
	Util_remove_watch(&vid_move_content_mode);
	Util_remove_watch(&vid_remember_video_pos_mode);
	Util_remove_watch(&vid_playback_mode);
	Util_remove_watch(&vid_menu_mode);
	Util_remove_watch(&vid_lower_resolution);
	Util_remove_watch(&vid_show_decode_graph_mode);
	Util_remove_watch(&vid_show_color_conversion_graph_mode);
	Util_remove_watch(&vid_show_packet_buffer_graph_mode);
	Util_remove_watch(&vid_show_raw_video_buffer_graph_mode);
	Util_remove_watch(&vid_show_raw_audio_buffer_graph_mode);

	Util_remove_watch(&vid_ui_y_offset);
	Util_remove_watch(&vid_ui_y_max);
	Util_remove_watch(&vid_subtitle_x_offset);
	Util_remove_watch(&vid_subtitle_y_offset);
	Util_remove_watch(&vid_subtitle_zoom);
	Util_remove_watch(&vid_video_x_offset);
	Util_remove_watch(&vid_video_y_offset);
	Util_remove_watch(&vid_video_zoom);
	Util_remove_watch(&vid_selected_audio_track);
	Util_remove_watch(&vid_selected_subtitle_track);

	Util_remove_watch(&vid_ok_button.selected);
	Util_remove_watch(&vid_control_button.selected);
	Util_remove_watch(&vid_scroll_bar.selected);
	Util_remove_watch(&vid_select_audio_track_button.selected);
	Util_remove_watch(&vid_select_subtitle_track_button.selected);
	Util_remove_watch(&vid_texture_filter_button.selected);
	Util_remove_watch(&vid_allow_skip_frames_button.selected);
	Util_remove_watch(&vid_allow_skip_key_frames_button.selected);
	Util_remove_watch(&vid_volume_button.selected);
	Util_remove_watch(&vid_seek_duration_button.selected);
	Util_remove_watch(&vid_correct_aspect_ratio_button.selected);
	Util_remove_watch(&vid_move_content_button.selected);
	Util_remove_watch(&vid_remember_video_pos_button.selected);
	Util_remove_watch(&vid_playback_mode_button.selected);
	Util_remove_watch(&vid_menu_background.selected);
	Util_remove_watch(&vid_disable_audio_button.selected);
	Util_remove_watch(&vid_disable_video_button.selected);
	Util_remove_watch(&vid_disable_subtitle_button.selected);
	Util_remove_watch(&vid_use_hw_decoding_button.selected);
	Util_remove_watch(&vid_use_hw_color_conversion_button.selected);
	Util_remove_watch(&vid_use_multi_threaded_decoding_button.selected);
	Util_remove_watch(&vid_lower_resolution_button.selected);
	Util_remove_watch(&vid_show_decode_graph_button.selected);
	Util_remove_watch(&vid_show_color_conversion_graph_button.selected);
	Util_remove_watch(&vid_show_packet_buffer_graph_button.selected);
	Util_remove_watch(&vid_show_raw_video_buffer_graph_button.selected);
	Util_remove_watch(&vid_show_raw_audio_buffer_graph_button.selected);

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		Util_remove_watch(&vid_audio_track_button[i].selected);

	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_DATA; i++)
		Util_remove_watch(&vid_subtitle_track_button[i].selected);

	for(int i = 0; i < 3; i++)
		Util_remove_watch(&vid_menu_button[i].selected);
	
	Util_log_save(DEF_VID_EXIT_STR, "Exited.");
}

void Vid_main(void)
{
	int color = DEF_DRAW_BLACK;
	int disabled_color = DEF_DRAW_WEAK_BLACK;
	int back_color = DEF_DRAW_WHITE;
	int image_num = 0;
	int image_num_3d = 0;
	double image_width[4] = { 0, 0, 0, 0, };
	double image_height[4] = { 0, 0, 0, 0, };
	double y_offset = 0;
	std::string thread_mode[3] = { "none", "frame", "slice" };
	std::string lower_resolution_mode[3] = { "OFF (x1.0)", "ON (x0.5)", "ON (x0.25)" };
	std::string swkbd_input = "";
	Hid_info key;
	Util_hid_query_key_state(&key);

	if (var_night_mode)
	{
		color = DEF_DRAW_WHITE;
		disabled_color = DEF_DRAW_WEAK_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	if(vid_image_num - 1 >= 0)
		image_num = vid_image_num - 1;
	else
		image_num = DEF_VID_BUFFERS - 1;

	if(vid_image_num_3d - 1 >= 0)
		image_num_3d = vid_image_num_3d - 1;
	else
		image_num_3d = DEF_VID_BUFFERS - 1;

	Vid_control_full_screen();

	for(int i = 0; i < 4; i++)
	{
		image_width[i] = vid_tex_width[i] * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_width : 1) * vid_video_zoom;
		image_height[i] = vid_tex_height[i] * (vid_correct_aspect_ratio_mode ? vid_video_info.sar_height : 1) * vid_video_zoom;
	}

	//Update performance data every 100ms
	if(osGetTime() >= vid_previous_ts + 100)
	{
		vid_previous_ts = osGetTime();
		vid_packet_buffer[319] = Util_decoder_get_available_packet_num(0);
		vid_raw_audio_buffer[319] = Util_speaker_get_available_buffer_num(0);
		vid_raw_video_buffer[319] = vid_hw_decoding_mode ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0);

		for(int i = 1; i < 320; i++)
			vid_packet_buffer[i - 1] = vid_packet_buffer[i];

		for(int i = 1; i < 320; i++)
			vid_raw_audio_buffer[i - 1] = vid_raw_audio_buffer[i];

		for(int i = 1; i < 320; i++)
			vid_raw_video_buffer[i - 1] = vid_raw_video_buffer[i];
	}


	if(Util_is_watch_changed() || var_need_reflesh || !var_eco_mode)
	{
		var_need_reflesh = false;
		Draw_frame_ready();

		if(var_turn_on_top_lcd)
		{
			Draw_screen_ready(0, vid_full_screen_mode ? DEF_DRAW_BLACK : back_color);

			if(!vid_full_screen_mode)
				Draw_top_ui();

			if(vid_play_request)
			{
				//video
				Draw_texture(vid_image[0][image_num][0].c2d, vid_video_x_offset, vid_video_y_offset, image_width[0], image_height[0]);
				if(vid_codec_width > 1024)
					Draw_texture(vid_image[1][image_num][0].c2d, (vid_video_x_offset + image_width[0]), vid_video_y_offset, image_width[1], image_height[1]);
				if(vid_codec_height > 1024)
					Draw_texture(vid_image[2][image_num][0].c2d, vid_video_x_offset, (vid_video_y_offset + image_height[0]), image_width[2], image_height[2]);
				if(vid_codec_width > 1024 && vid_codec_height > 1024)
					Draw_texture(vid_image[3][image_num][0].c2d, (vid_video_x_offset + image_width[0]), (vid_video_y_offset + image_height[0]), image_width[3], image_height[3]);

				for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_DATA; i++)//subtitle
				{
					if(vid_current_pos >= vid_subtitle_data[i].start_time && vid_current_pos <= vid_subtitle_data[i].end_time)
					{
						Draw(vid_subtitle_data[i].text, vid_subtitle_x_offset, 195 + vid_subtitle_y_offset, 0.5 * vid_subtitle_zoom, 0.5 * vid_subtitle_zoom, DEF_DRAW_WHITE, 
							DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 400, 40, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
						break;
					}
				}
			}
			else
				Draw_texture(vid_banner[var_night_mode], 0, 15, 400, 225);
						
			if(vid_full_screen_mode && vid_turn_off_bottom_screen_count > 0 && vid_show_full_screen_msg)
			{
				//Exit full screen message
				Draw(vid_msg[DEF_VID_FULL_SCREEN_MSG], 0, 20, 0.45, 0.45, DEF_DRAW_WHITE, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 400, 30,
				DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLACK);
			}

			if(vid_show_screen_brightness_until >= osGetTime())
			{
				//Display current brightness
				Draw(vid_msg[DEF_VID_BRIGHTNESS_MSG] + std::to_string(var_lcd_brightness) + "/180", 0, 220, 0.45, 0.45, DEF_DRAW_WHITE, DEF_DRAW_X_ALIGN_LEFT,
				DEF_DRAW_Y_ALIGN_BOTTOM, 400, 20, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLACK);
			}

			if(vid_seek_request || vid_seek_adjust_request)
			{
				//Seek message
				Draw(vid_msg[DEF_VID_SEEKING_MSG], 0, 220, 0.5, 0.5, DEF_DRAW_WHITE, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 400, 20,
				DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLACK);
			}

			if(Util_log_query_log_show_flag())
				Util_log_draw();

			if(var_3d_mode)
			{
				Draw_screen_ready(2, vid_full_screen_mode ? DEF_DRAW_BLACK : back_color);

				if(vid_play_request)
				{
					//3d video (right eye)
					Draw_texture(vid_image[0][image_num_3d][1].c2d, vid_video_x_offset, vid_video_y_offset, image_width[0], image_height[0]);
					if(vid_codec_width > 1024)
						Draw_texture(vid_image[1][image_num_3d][1].c2d, (vid_video_x_offset + image_width[0]), vid_video_y_offset, image_width[1], image_height[1]);
					if(vid_codec_height > 1024)
						Draw_texture(vid_image[2][image_num_3d][1].c2d, vid_video_x_offset, (vid_video_y_offset + image_height[0]), image_width[2], image_height[2]);
					if(vid_codec_width > 1024 && vid_codec_height > 1024)
						Draw_texture(vid_image[3][image_num_3d][1].c2d, (vid_video_x_offset + image_width[0]), (vid_video_y_offset + image_height[0]), image_width[3], image_height[3]);

					for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_DATA; i++)//subtitle
					{
						if(vid_current_pos >= vid_subtitle_data[i].start_time && vid_current_pos <= vid_subtitle_data[i].end_time)
						{
							Draw(vid_subtitle_data[i].text, vid_subtitle_x_offset, 195 + vid_subtitle_y_offset, 0.5 * vid_subtitle_zoom, 0.5 * vid_subtitle_zoom, DEF_DRAW_WHITE, 
								DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 400, 40, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
							break;
						}
					}
				}
				else
					Draw_texture(vid_banner[var_night_mode], 0, 15, 400, 225);

				if(Util_log_query_log_show_flag())
					Util_log_draw();

				if(!vid_full_screen_mode)
					Draw_top_ui();
			}
		}

		if(var_turn_on_bottom_lcd)
		{
			if(var_model == CFG_MODEL_2DS && vid_turn_off_bottom_screen_count == 0 && vid_full_screen_mode)
				Draw_screen_ready(1, DEF_DRAW_BLACK);
			else
			{
				Draw_screen_ready(1, back_color);
				Draw(DEF_VID_VER, 0, 0, 0.425, 0.425, DEF_DRAW_GREEN);

				//codec info
				Draw(vid_video_info.format_name, 0, 10, 0.5, 0.5, color);
				Draw(vid_audio_info.format_name, 0, 20, 0.5, 0.5, color);
				Draw(std::to_string(vid_video_info.width) + "x" + std::to_string(vid_video_info.height) + "(" + std::to_string(vid_codec_width) + "x" + std::to_string(vid_codec_height) + ")" + "@" + std::to_string(vid_video_info.framerate).substr(0, 5) + "fps", 0, 30, 0.5, 0.5, color);

				if(vid_play_request)
				{
					//video
					Draw_texture(vid_image[0][image_num][0].c2d, vid_video_x_offset - 40, vid_video_y_offset - 240, image_width[0], image_height[0]);
					if(vid_codec_width > 1024)
						Draw_texture(vid_image[1][image_num][0].c2d, (vid_video_x_offset + image_width[0] - 40), vid_video_y_offset - 240, image_width[1], image_height[1]);
					if(vid_codec_height > 1024)
						Draw_texture(vid_image[2][image_num][0].c2d, vid_video_x_offset - 40, (vid_video_y_offset + image_height[0] - 240), image_width[2], image_height[2]);
					if(vid_codec_width > 1024 && vid_codec_height > 1024)
						Draw_texture(vid_image[3][image_num][0].c2d, (vid_video_x_offset + image_width[0] - 40), (vid_video_y_offset + image_height[0] - 240), image_width[3], image_height[3]);

					for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_DATA; i++)//subtitle
					{
						if(vid_current_pos >= vid_subtitle_data[i].start_time && vid_current_pos <= vid_subtitle_data[i].end_time)
						{
							Draw(vid_subtitle_data[i].text, vid_subtitle_x_offset, 195 + vid_subtitle_y_offset - 240, 0.5 * vid_subtitle_zoom, 0.5 * vid_subtitle_zoom, DEF_DRAW_WHITE, 
								DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 320, 40, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
							break;
						}
					}
				}

				if(vid_menu_mode != DEF_VID_MENU_NONE)
					Draw_texture(&vid_menu_background, DEF_DRAW_WEAK_GREEN, 0, 50, 320, 130);

				if(vid_menu_mode == DEF_VID_MENU_SETTINGS_0)
				{
					//scroll bar
					Draw_texture(&vid_scroll_bar, vid_scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_ui_y_offset / vid_ui_y_max * 120) + 50, 7, 10);

					y_offset = 60;
					//playback mode
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_PLAY_METHOD_MSG] + vid_msg[DEF_VID_NO_REPEAT_MSG + vid_playback_mode], 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_playback_mode_button, vid_playback_mode_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_playback_mode_button.x_size = -1;
						vid_playback_mode_button.y_size = -1;
					}

					y_offset += 25;
					//volume
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_VOLUME_MSG] + std::to_string(vid_volume) + "%", 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, vid_too_big ? DEF_DRAW_RED : color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_volume_button, vid_volume_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_volume_button.x_size = -1;
						vid_volume_button.y_size = -1;
					}

					y_offset += 25;
					//select audio track
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_AUDIO_TRACK_MSG], 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_select_audio_track_button, vid_select_audio_track_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_select_audio_track_button.x_size = -1;
						vid_select_audio_track_button.y_size = -1;
					}

					y_offset += 25;
					//select subtitle track
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SUBTITLE_TRACK_MSG], 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_select_subtitle_track_button, vid_select_subtitle_track_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_select_subtitle_track_button.x_size = -1;
						vid_select_subtitle_track_button.y_size = -1;
					}

					y_offset += 25;
					//seek duration
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SEEK_MSG] + std::to_string(vid_seek_duration) + "s", 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_seek_duration_button, vid_seek_duration_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_seek_duration_button.x_size = -1;
						vid_seek_duration_button.y_size = -1;
					}

					y_offset += 25;
					//remember video pos
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_REMEMBER_POS_MSG] + (vid_remember_video_pos_mode ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_remember_video_pos_button, vid_remember_video_pos_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_remember_video_pos_button.x_size = -1;
						vid_remember_video_pos_button.y_size = -1;
					}

					y_offset += 25;
					//texture filter
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_TEX_FILTER_MSG] + (vid_linear_filter ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_texture_filter_button, vid_texture_filter_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_texture_filter_button.x_size = -1;
						vid_texture_filter_button.y_size = -1;
					}

					y_offset += 25;
					//correct aspect ratio
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_ASPECT_RATIO_MSG] + (vid_correct_aspect_ratio_mode ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_correct_aspect_ratio_button, vid_correct_aspect_ratio_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_correct_aspect_ratio_button.x_size = -1;
						vid_correct_aspect_ratio_button.y_size = -1;
					}
				
					y_offset += 25;
					//move content mode
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_MOVE_MODE_MSG] + vid_msg[DEF_VID_MOVE_MODE_EDIABLE_MSG + vid_move_content_mode], 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_move_content_button, vid_move_content_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_move_content_button.x_size = -1;
						vid_move_content_button.y_size = -1;
					}

					y_offset += 25;
					//allow skip frames
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SKIP_FRAME_MSG] + (vid_allow_skip_frames ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_allow_skip_frames_button, vid_allow_skip_frames_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_allow_skip_frames_button.x_size = -1;
						vid_allow_skip_frames_button.y_size = -1;
					}

					y_offset += 25;
					//allow skip key frames
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SKIP_KEY_FRAME_MSG] + (vid_allow_skip_key_frames ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.5, 0.5, vid_allow_skip_frames ? color : disabled_color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 15,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_allow_skip_key_frames_button, vid_allow_skip_key_frames_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_allow_skip_key_frames_button.x_size = -1;
						vid_allow_skip_key_frames_button.y_size = -1;
					}

					Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 0, 180, 100, 8);
					Draw_texture(&vid_menu_button[1], vid_menu_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
					Draw_texture(&vid_menu_button[2], vid_menu_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
				}
				else if(vid_menu_mode == DEF_VID_MENU_SETTINGS_1)
				{
					//scroll bar
					Draw_texture(&vid_scroll_bar, vid_scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_ui_y_offset / vid_ui_y_max * 120) + 50, 7, 10);

					y_offset = 60;
					//disable audio
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_DISABLE_AUDIO_MSG] + (vid_disable_audio_mode ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_play_request ? disabled_color : color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 20,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_disable_audio_button, vid_disable_audio_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_disable_audio_button.x_size = -1;
						vid_disable_audio_button.y_size = -1;
					}

					y_offset += 30;
					//disable video
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_DISABLE_VIDEO_MSG] + (vid_disable_video_mode ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_play_request ? disabled_color : color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 20,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_disable_video_button, vid_disable_video_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_disable_video_button.x_size = -1;
						vid_disable_video_button.y_size = -1;
					}

					y_offset += 30;
					//disable subtitle
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_DISABLE_SUBTITLE_MSG] + (vid_disable_subtitle_mode ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_play_request ? disabled_color : color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 20,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_disable_subtitle_button, vid_disable_subtitle_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_disable_subtitle_button.x_size = -1;
						vid_disable_subtitle_button.y_size = -1;
					}

					y_offset += 30;
					//use hw decoding
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_HW_DECODER_MSG] + (vid_use_hw_decoding_request ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.425, 0.425, 
						(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL || vid_play_request) ? disabled_color : color,
						DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 20, DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_use_hw_decoding_button, vid_use_hw_decoding_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_use_hw_decoding_button.x_size = -1;
						vid_use_hw_decoding_button.y_size = -1;
					}

					y_offset += 30;
					//use hw color conversion
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_HW_CONVERTER_MSG] + (vid_use_hw_color_conversion_request ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_play_request ? disabled_color : color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 20,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_use_hw_color_conversion_button, vid_use_hw_color_conversion_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_use_hw_color_conversion_button.x_size = -1;
						vid_use_hw_color_conversion_button.y_size = -1;
					}

					y_offset += 30;
					//use multi-threaded decoding (in software decoding)
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_MULTI_THREAD_MSG] + (vid_use_multi_threaded_decoding_request ? "ON" : "OFF"), 12.5, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_play_request ? disabled_color : color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 20,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_use_multi_threaded_decoding_button, vid_use_multi_threaded_decoding_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_use_multi_threaded_decoding_button.x_size = -1;
						vid_use_multi_threaded_decoding_button.y_size = -1;
					}

					y_offset += 30;
					//lower resolution
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_LOWER_RESOLUTION_MSG] + lower_resolution_mode[vid_lower_resolution], 12.5, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_play_request ? disabled_color : color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 20,
						DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_lower_resolution_button, vid_lower_resolution_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_lower_resolution_button.x_size = -1;
						vid_lower_resolution_button.y_size = -1;
					}

					Draw_texture(&vid_menu_button[0], vid_menu_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
					Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 110, 180, 100, 8);
					Draw_texture(&vid_menu_button[2], vid_menu_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
				}
				else if(vid_menu_mode == DEF_VID_MENU_INFO)
				{
					//scroll bar
					Draw_texture(&vid_scroll_bar, vid_scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_ui_y_offset / vid_ui_y_max * 120) + 50, 7, 10);

					y_offset = 160;
					//color conversion time
					if(vid_show_color_conversion_graph_mode)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_time[1][i] + vid_ui_y_offset, DEF_DRAW_BLUE, i + 1, y_offset - vid_time[1][i + 1] + vid_ui_y_offset, DEF_DRAW_BLUE, 1);//Thread 1
					}
					//decoding time
					if(vid_show_decode_graph_mode)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_time[0][i] + vid_ui_y_offset, DEF_DRAW_RED, i + 1, y_offset - vid_time[0][i + 1] + vid_ui_y_offset, DEF_DRAW_RED, 1);//Thread 0		
					}
					//compressed buffer
					if(vid_show_packet_buffer_graph_mode)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_packet_buffer[i] / 4 + vid_ui_y_offset, 0xFFFF00FF, i + 1, y_offset - vid_packet_buffer[i + 1] / 4 + vid_ui_y_offset, 0xFFFF00FF, 1);//Packet buffer
					}
					//raw video buffer
					if(vid_show_raw_video_buffer_graph_mode)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_raw_video_buffer[i] / 2 + vid_ui_y_offset, 0xFF2060FF, i + 1, y_offset - vid_raw_video_buffer[i + 1] / 2 + vid_ui_y_offset, 0xFF2060FF, 1);//Raw image buffer
					}
					//raw audio buffer
					if(vid_show_raw_audio_buffer_graph_mode)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_raw_audio_buffer[i] / 3 + vid_ui_y_offset, 0xFF00A000, i + 1, y_offset - vid_raw_audio_buffer[i + 1] / 3 + vid_ui_y_offset, 0xFF00A000, 1);//Raw audio buffer
					}

					//bottom line
					Draw_line(0, y_offset + vid_ui_y_offset, color, 320, y_offset + vid_ui_y_offset, color, 1);
					//deadline
					Draw_line(0, y_offset + vid_ui_y_offset - vid_frametime, 0xFFD0D000, 320, y_offset - vid_frametime + vid_ui_y_offset, 0xFFD0D000, 1);

					//key frame
					if(vid_show_decode_graph_mode)
					{
						for(int i = 0; i < 319; i++)
						{
							if(vid_key_frame_list[i])
								Draw_line(i, y_offset + vid_ui_y_offset, disabled_color, i, y_offset - 110 + vid_ui_y_offset, disabled_color, 2);
						}
					}

					//compressed buffer button 
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 170)
					{
						Draw_texture(&vid_show_packet_buffer_graph_button, vid_show_packet_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_ui_y_offset, 160, 10);
						Draw("Compressed buffer : " + std::to_string(Util_decoder_get_available_packet_num(0)), 0, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_show_packet_buffer_graph_mode ? 0xFFFF00FF : color);
					}
					else
					{
						vid_show_packet_buffer_graph_button.x_size = -1;
						vid_show_packet_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					//raw video and audio buffer button
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 170)
					{
						Draw_texture(&vid_show_raw_video_buffer_graph_button, vid_show_raw_video_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_ui_y_offset, 160, 10);
						if(vid_frametime != 0)
						{
							Draw("Raw video buffer : " + std::to_string(vid_hw_decoding_mode ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0))
							+ "(" + std::to_string(int((vid_hw_decoding_mode ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0)) * vid_frametime)) + "ms)",
							0, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_show_raw_video_buffer_graph_mode ? 0xFF2060FF : color);
						}
						else
						{
							Draw("Raw video buffer : " + std::to_string(vid_hw_decoding_mode ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0)), 
							0, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_show_raw_video_buffer_graph_mode ? 0xFF2060FF : color);
						}
					}
					else
					{
						vid_show_raw_video_buffer_graph_button.x_size = -1;
						vid_show_raw_video_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 170)
					{
						Draw_texture(&vid_show_raw_audio_buffer_graph_button, vid_show_raw_audio_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_ui_y_offset, 160, 10);
						if(vid_audio_info.ch != 0 && vid_audio_info.sample_rate != 0)
						{
							Draw("Raw audio buffer : " + std::to_string(Util_speaker_get_available_buffer_num(0)) + "(" 
							+ std::to_string((int)(Util_speaker_get_available_buffer_size(0) / 2.0 / vid_audio_info.ch / vid_audio_info.sample_rate * 1000)) + "ms)",
							0, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_show_raw_audio_buffer_graph_mode ? 0xFF00A000 : color);
						}
						else
							Draw("Raw audio buffer : " + std::to_string(Util_speaker_get_available_buffer_num(0)), 0, y_offset + vid_ui_y_offset, 0.425, 0.425, vid_show_raw_audio_buffer_graph_mode ? 0xFF00A000 : color);
					}
					else
					{
						vid_show_raw_audio_buffer_graph_button.x_size = -1;
						vid_show_raw_audio_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					//decoding speed
					if(vid_total_frames != 0 && vid_min_time != 0  && vid_recent_total_time != 0 && y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 170)
					{
						if(!vid_hw_decoding_mode && vid_video_info.thread_type == DEF_DECODER_THREAD_TYPE_FRAME)
							Draw("*When thread_type == frame, the red graph is unusable", 0, y_offset + vid_ui_y_offset, 0.425, 0.425, color);
						else
							Draw("avg/min/max/recent avg " + std::to_string(1000 / (vid_total_time / vid_total_frames)).substr(0, 5) + "/" + std::to_string(1000 / vid_max_time).substr(0, 5) 
							+  "/" + std::to_string(1000 / vid_min_time).substr(0, 5) + "/" + std::to_string(1000 / (vid_recent_total_time / 90)).substr(0, 5) +  " fps", 0, y_offset + vid_ui_y_offset, 0.425, 0.425, color);
					}

					y_offset += 10;
					//decoding mode
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 170)
						Draw((std::string)"Hw decoding : " + (vid_hw_decoding_mode ? "yes" : "no"), 0, y_offset + vid_ui_y_offset, 0.45, 0.45, color);

					y_offset += 10;
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 170)
						Draw((std::string)"Hw color conversion : " + ((vid_hw_decoding_mode || vid_hw_color_conversion_mode) ? "yes" : "no"), 0, y_offset + vid_ui_y_offset, 0.45, 0.45, color);

					y_offset += 10;
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 170)
						Draw((std::string)"Thread type : " + thread_mode[vid_hw_decoding_mode ? 0 : vid_video_info.thread_type], 0, y_offset + vid_ui_y_offset, 0.45, 0.45, color);


					y_offset += 10;
					//color conversion button
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 136)
						Draw_texture(&vid_show_color_conversion_graph_button, vid_show_color_conversion_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 160, y_offset + vid_ui_y_offset, 160, 44);
					else
					{
						vid_show_color_conversion_graph_button.x_size = -1;
						vid_show_color_conversion_graph_button.y_size = -1;
					}

					//deadline and color conversion text
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw("Deadline : " + std::to_string(vid_frametime).substr(0, 5) + "ms", 0, y_offset + vid_ui_y_offset, 0.45, 0.45, 0xFFD0D000);
						Draw("Data copy 0 : " + std::to_string(vid_copy_time[0]).substr(0, 5) + "ms", 160, y_offset + vid_ui_y_offset, 0.45, 0.45, vid_show_color_conversion_graph_mode ? DEF_DRAW_BLUE : color);
					}	

					y_offset += 10;
					//decoding time button
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 146)
						Draw_texture(&vid_show_decode_graph_button, vid_show_decode_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_ui_y_offset, 160, 34);
					else
					{
						vid_show_decode_graph_button.x_size = -1;
						vid_show_decode_graph_button.y_size = -1;
					}

					//decoding time text and color conversion text
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw("Video decode : " + std::to_string(vid_video_time).substr(0, 5) + "ms", 0, y_offset + vid_ui_y_offset, 0.45, 0.45, vid_show_decode_graph_mode ? DEF_DRAW_RED : color);
						Draw("Color conversion : " + std::to_string(vid_convert_time).substr(0, 5) + "ms", 160, y_offset + vid_ui_y_offset, 0.45, 0.45, vid_show_color_conversion_graph_mode ? DEF_DRAW_BLUE : color);
					}

					y_offset += 10;
					//decoding time text and color conversion text
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw("Audio decode : " + std::to_string(vid_audio_time).substr(0, 5) + "ms", 0, y_offset + vid_ui_y_offset, 0.45, 0.45, vid_show_decode_graph_mode ? DEF_DRAW_RED : color);
						Draw("Data copy 1 : " + std::to_string(vid_copy_time[1]).substr(0, 5) + "ms", 160, y_offset + vid_ui_y_offset, 0.45, 0.45, vid_show_color_conversion_graph_mode ? DEF_DRAW_BLUE : color);
					}

					y_offset += 10;
					//total time
					if(y_offset + vid_ui_y_offset >= 50 && y_offset + vid_ui_y_offset <= 165)
					{
						Draw("Thread 0 : " + std::to_string(vid_time[0][319]).substr(0, 6) + "ms", 0, y_offset + vid_ui_y_offset, 0.5, 0.5, vid_show_decode_graph_mode ? DEF_DRAW_RED : color);
						Draw("Thread 1 : " + std::to_string(vid_time[1][319]).substr(0, 6) + "ms", 160, y_offset + vid_ui_y_offset, 0.5, 0.5, vid_show_color_conversion_graph_mode ? DEF_DRAW_BLUE : color);
					}

					Draw_texture(&vid_menu_button[0], vid_menu_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
					Draw_texture(&vid_menu_button[1], vid_menu_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
					Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 220, 180, 100, 8);
				}

				//controls
				Draw(vid_msg[DEF_VID_CONTROLS_MSG], 167.5, 195, 0.425, 0.425, color, DEF_DRAW_X_ALIGN_CENTER, DEF_DRAW_Y_ALIGN_CENTER, 145, 10,
				DEF_DRAW_BACKGROUND_ENTIRE_BOX, &vid_control_button, vid_control_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);

				//time bar
				Draw(Util_convert_seconds_to_time(vid_current_pos / 1000) + "/" + Util_convert_seconds_to_time(vid_duration / 1000), 10, 192.5, 0.5, 0.5, color);
				Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 5, 210, 310, 10);
				if(vid_duration != 0)
					Draw_texture(var_square_image[0], 0xFF800080, 5, 210, 310 * ((vid_current_pos / 1000) / (vid_duration / 1000)), 10);

				if(vid_show_controls_request)
				{
					Draw_texture(vid_control[var_night_mode], 80, 20, 160, 160);
					if(var_lang == "ro")
					{
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG], 122.5, 47.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 1], 122.5, 62.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 2], 122.5, 77.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 3], 122.5, 92.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 4], 135, 107.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 5], 122.5, 122.5, 0.425, 0.425, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 6], 132.5, 137.5, 0.425, 0.425, DEF_DRAW_BLACK);
					}
					else
					{
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG], 122.5, 47.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 1], 122.5, 62.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 2], 122.5, 77.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 3], 122.5, 92.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 4], 135, 107.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 5], 122.5, 122.5, 0.45, 0.45, DEF_DRAW_BLACK);
						Draw(vid_msg[DEF_VID_CONTROL_DESCRIPTION_MSG + 6], 132.5, 137.5, 0.45, 0.45, DEF_DRAW_BLACK);
					}
				}

				if(vid_select_audio_track_request)
				{
					Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 40, 20, 240, 140);
					Draw(vid_msg[DEF_VID_AUDIO_TRACK_DESCRIPTION_MSG], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

					for(int i = 0; i < vid_num_of_audio_tracks; i++)
					{
						Draw_texture(&vid_audio_track_button[i], vid_audio_track_button[i].selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 40, 40 + (i * 12), 240, 12);
						Draw("Track " + std::to_string(i) + "(" + vid_audio_track_lang[i] + ")", 42.5, 40 + (i * 12), 0.475, 0.475, i == vid_selected_audio_track ? DEF_DRAW_RED : color);
					}

					Draw_texture(&vid_ok_button, vid_ok_button.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
					Draw("OK", 152.5, 140, 0.425, 0.425, DEF_DRAW_BLACK);
				}

				if(vid_select_subtitle_track_request)
				{
					Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 40, 20, 240, 140);
					Draw(vid_msg[DEF_VID_SUBTITLE_TRACK_DESCRIPTION_MSG], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

					for(int i = 0; i < vid_num_of_subtitle_tracks; i++)
					{
						Draw_texture(&vid_subtitle_track_button[i], vid_subtitle_track_button[i].selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 40, 40 + (i * 12), 240, 12);
						Draw("Track " + std::to_string(i) + "(" + vid_subtitle_track_lang[i] + ")", 42.5, 40 + (i * 12), 0.475, 0.475, i == vid_selected_subtitle_track ? DEF_DRAW_RED : color);
					}

					Draw_texture(&vid_ok_button, vid_ok_button.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
					Draw("OK", 152.5, 140, 0.425, 0.425, DEF_DRAW_BLACK);
				}

				if(Util_expl_query_show_flag())
					Util_expl_draw();

				if(Util_err_query_error_show_flag())
					Util_err_draw();

				Draw_bot_ui();
			}
		}

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();

	if(vid_set_volume_request)
	{
		vid_pause_request = true;
		Util_swkbd_init(SWKBD_TYPE_NUMPAD, SWKBD_NOTEMPTY, 1, 3, "", std::to_string(vid_volume));
		Util_swkbd_launch(&swkbd_input);
		vid_volume = atoi((char*)swkbd_input.c_str());
		vid_pause_request = false;
		var_need_reflesh = true;
		Util_swkbd_exit();
		vid_set_volume_request = false;
	}
	else if(vid_set_seek_duration_request)
	{
		vid_pause_request = true;
		Util_swkbd_init(SWKBD_TYPE_NUMPAD, SWKBD_NOTEMPTY, 1, 2, "", std::to_string(vid_seek_duration));
		Util_swkbd_launch(&swkbd_input);
		vid_seek_duration = atoi((char*)swkbd_input.c_str());
		if(vid_seek_duration == 0)
			vid_seek_duration = 1;
		vid_pause_request = false;
		var_need_reflesh = true;
		Util_swkbd_exit();
		vid_set_seek_duration_request = false;
	}
}
