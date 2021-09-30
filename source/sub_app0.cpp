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
bool vid_use_hw_decoding_request = true;
bool vid_use_hw_color_conversion_request = true;
bool vid_use_multi_threaded_decoding_request = true;
bool vid_pause_request = false;
bool vid_seek_request = false;
bool vid_seek_adjust_request = false;
bool vid_linear_filter = true;
bool vid_show_controls = false;
bool vid_allow_skip_frames = false;
bool vid_allow_skip_key_frames = false;
bool vid_key_frame = false;
bool vid_full_screen_mode = false;
bool vid_hw_decoding_mode = false;
bool vid_hw_color_conversion_mode = false;
bool vid_correct_aspect_ratio_mode = false;
bool vid_disable_resize_move_mode = false;
bool vid_remember_video_pos_mode = false;
bool vid_show_full_screen_msg = true;
bool vid_too_big = false;
bool vid_image_enabled[4][DEF_SAPP0_BUFFERS][2];
bool vid_enabled_cores[4] = { false, false, false, false, };
bool vid_key_frame_list[320];
bool vid_select_audio_track_button_selected, vid_texture_filter_button_selected, vid_allow_skip_frames_button_selected,
vid_allow_skip_key_frames_button_selected, vid_volume_button_selected, vid_seek_duration_button_selected, vid_use_hw_decoding_button_selected,
vid_use_hw_color_conversion_button_selected, vid_use_multi_threaded_decoding_button_selected, vid_lower_resolution_button_selected,
vid_menu_button_selected[3], vid_control_button_selected, vid_ok_button_selected, vid_audio_track_button_selected[DEF_DECODER_MAX_AUDIO_TRACKS],
vid_correct_aspect_ratio_button_selected, vid_disable_resize_move_button_selected, vid_remember_video_pos_button_selected;
double vid_time[2][320];
double vid_copy_time[2] = { 0, 0, };
double vid_audio_time = 0;
double vid_video_time = 0;
double vid_convert_time = 0;
double vid_frametime = 0;
double vid_framerate = 0;
double vid_duration = 0;//in ms
double vid_zoom = 1;
double vid_x = 0;
double vid_y = 15;
double vid_current_pos = 0;//in ms
double vid_seek_pos = 0;//in ms
double vid_min_time = 0;
double vid_max_time = 0;
double vid_total_time = 0;
double vid_recent_time[90];
double vid_recent_total_time = 0;
int vid_sar_width = 1;
int vid_sar_height = 1;
int vid_seek_duration = 5;
int vid_volume = 100;
int vid_lower_resolution = 0;
int vid_menu_mode = 0;
int vid_num_of_threads = 1;
int vid_request_thread_mode = DEF_DECODER_THREAD_TYPE_AUTO;
int vid_thread_mode = DEF_DECODER_THREAD_TYPE_NONE;
int vid_turn_off_bottom_screen_count = 0;
int vid_num_of_audio_tracks = 0;
int vid_selected_audio_track = 0;
int vid_total_frames = 0;
int vid_width = 0;
int vid_height = 0;
int vid_codec_width = 0;
int vid_codec_height = 0;
int vid_tex_width[4] = { 0, 0, 0, 0, };
int vid_tex_height[4] = { 0, 0, 0, 0, };
int vid_lr_count = 0;
int vid_cd_count = 0;
int vid_image_num = 0;
int vid_image_num_3d = 0;
int vid_packet_index[2] = { 0, 0, };
int vid_control_texture_num = -1;
int	vid_banner_texture_num = -1;
std::string vid_file = "";
std::string vid_dir = "";
std::string vid_video_format = "n/a";
std::string vid_audio_format = "n/a";
std::string vid_audio_track_lang[DEF_DECODER_MAX_AUDIO_TRACKS];
std::string vid_msg[DEF_SAPP0_NUM_OF_MSG];
Image_data vid_image[4][DEF_SAPP0_BUFFERS][2];
C2D_Image vid_banner[2];
C2D_Image vid_control[2];
Thread vid_decode_thread, vid_decode_video_thread, vid_convert_thread;
Image_data vid_select_audio_track_button, vid_texture_filter_button, vid_allow_skip_frames_button, vid_allow_skip_key_frames_button,
vid_volume_button, vid_seek_duration_button, vid_use_hw_decoding_button, vid_use_hw_color_conversion_button, vid_use_multi_threaded_decoding_button,
vid_lower_resolution_button, vid_menu_button[3], vid_control_button, vid_ok_button, vid_audio_track_button[DEF_DECODER_MAX_AUDIO_TRACKS],
vid_correct_aspect_ratio_button, vid_disable_resize_move_button, vid_remember_video_pos_button;

void Sapp0_callback(std::string file, std::string dir)
{
	vid_file = file;
	vid_dir = dir;
	vid_change_video_request = true;
}

void Sapp0_cancel(void)
{

}

bool Sapp0_query_init_flag(void)
{
	return vid_already_init;
}

bool Sapp0_query_running_flag(void)
{
	return vid_main_run;
}

void Sapp0_decode_thread(void* arg)
{
	Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Thread started.");

	bool key_frame = false;
	int wait_count = 0;
	int audio_track = 0;
	int bitrate = 0;
	int sample_rate = 0;
	int ch = 0;
	int audio_size = 0;
	int packet_index = 0;
	int num_of_audio_tracks = 0;
	int num_of_video_tracks = 0;
	double pos = 0;
	double saved_pos = 0;
	u8* audio = NULL;
	u32 read_size = 0;
	std::string format = "";
	std::string type = "";
	std::string cache_file_name = "";
	std::string cache = "";
	TickCounter counter;
	Result_with_string result;
	osTickCounterStart(&counter);
	
	Util_file_save_to_file(".", DEF_MAIN_DIR + "saved_pos/", NULL, 0, false);//create directory

	while (vid_thread_run)
	{
		if(vid_play_request || vid_change_video_request)
		{
			packet_index = 0;
			wait_count = 0;
			vid_num_of_audio_tracks = 0;
			vid_selected_audio_track = 0;
			vid_x = 0;
			vid_y = 15;
			vid_frametime = 0;
			vid_framerate = 0;
			vid_current_pos = 0;
			vid_duration = 0;
			vid_zoom = 1;
			vid_sar_width = 1;
			vid_sar_height = 1;
			vid_width = 0;
			vid_height = 0;
			vid_codec_width = 0;
			vid_codec_height = 0;
			vid_video_format = "n/a";
			vid_audio_format = "n/a";
			cache_file_name = "";
			cache = vid_dir + vid_file;
			vid_seek_request = false;
			vid_seek_adjust_request = false;
			vid_change_video_request = false;
			vid_play_request = true;
			vid_decode_video_request = false;
			vid_convert_request = false;
			vid_hw_decoding_mode = false;
			vid_hw_color_conversion_mode = false;
			vid_total_time = 0;
			vid_total_frames = 0;
			vid_min_time = 99999999;
			vid_max_time = 0;
			vid_recent_total_time = 0;
			vid_image_num = 0;
			vid_selected_audio_track = 0;

			for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
				vid_audio_track_lang[i] = "";

			for(int i = 0; i < 90; i++)
				vid_recent_time[i] = 0;
			
			for(int i = 0; i < 4; i++)
			{
				vid_tex_width[i] = 0;
				vid_tex_height[i] = 0;
			}

			for(int i = 0 ; i < 320; i++)
			{
				vid_time[0][i] = 0;
				vid_time[1][i] = 0;
				vid_key_frame_list[i] = false;
			}

			vid_audio_time = 0;
			vid_video_time = 0;
			vid_copy_time[0] = 0;
			vid_copy_time[1] = 0;
			vid_convert_time = 0;
			
			for(int i = 0; i < (int)cache.length(); i++)
			{
				if(cache.substr(i, 1) == "/")
					cache_file_name += "_";
				else
					cache_file_name += cache.substr(i, 1);
			}

			result = Util_decoder_open_file(vid_dir + vid_file, &num_of_audio_tracks, &num_of_video_tracks, 0);
			Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_decoder_open_file()..." + result.string + result.error_description, result.code);
			if(result.code != 0)
				vid_play_request = false;

			if(num_of_audio_tracks > 0 && vid_play_request)
			{
				vid_num_of_audio_tracks = num_of_audio_tracks;
				result = Util_audio_decoder_init(num_of_audio_tracks, 0);
				Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_audio_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_play_request = false;
				
				if(vid_play_request)
				{
					for(int i = 0; i < num_of_audio_tracks; i++)
						Util_audio_decoder_get_info(&bitrate, &sample_rate, &ch, &vid_audio_format, &vid_duration, i, &vid_audio_track_lang[i], 0);

					if(num_of_audio_tracks > 1)
					{
						vid_select_audio_track_request = true;
						var_need_reflesh = true;
						while(vid_select_audio_track_request)
							usleep(16600);
					}

					audio_track = vid_selected_audio_track;
					Util_audio_decoder_get_info(&bitrate, &sample_rate, &ch, &vid_audio_format, &vid_duration, audio_track, &vid_audio_track_lang[audio_track], 0);
					vid_duration *= 1000;
					Util_speaker_init(0, ch, sample_rate);
				}
			}
			if(num_of_video_tracks && vid_play_request)
			{
				Util_fake_pthread_set_enabled_core(vid_enabled_cores);
				result = Util_video_decoder_init(vid_lower_resolution, num_of_video_tracks, vid_use_multi_threaded_decoding_request ? vid_num_of_threads : 1, vid_use_multi_threaded_decoding_request ? vid_request_thread_mode : 0, 0);
				Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_video_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_play_request = false;
				
				if(vid_play_request)
				{
					Util_video_decoder_get_info(&vid_width, &vid_height, &vid_framerate, &vid_video_format, &vid_duration, &vid_thread_mode, &vid_sar_width, &vid_sar_height, 0, 0);
					
					//use sar 1:2 if 800x240 and no sar value is set
					if(vid_width == 800 && vid_height == 240 && vid_sar_width == 1 && vid_sar_height == 1)
						vid_sar_height = 2;
					
					vid_duration *= 1000;
					vid_frametime = (1000.0 / vid_framerate);
					if(num_of_video_tracks == 2)
						vid_frametime /= 2;

					vid_codec_width = vid_width;
					vid_codec_height = vid_height;
					//fit to screen size
					if((((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400) >= (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 240))
						vid_zoom = 1.0 / (((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400);
					else 
						vid_zoom = 1.0 / (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 240);
					
					vid_x = (400 - (vid_width * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1))) / 2;
					vid_y = (240 - (vid_height * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1))) / 2;

					if(vid_codec_width % 16 != 0)
						vid_codec_width += 16 - vid_codec_width % 16;
					if(vid_codec_height % 16 != 0)
						vid_codec_height += 16 - vid_codec_height % 16;

					if(vid_use_hw_decoding_request && vid_video_format == "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10")
					{
						vid_hw_decoding_mode = true;
						result = Util_mvd_video_decoder_init(0);
						Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_mvd_video_decoder_init()..." + result.string + result.error_description, result.code);
						if(result.code != 0)
							vid_play_request = false;
					}

					if(!vid_use_hw_decoding_request && vid_use_hw_color_conversion_request)
					{
						if(vid_codec_width <= 1024 && vid_codec_height <= 1024)
						{
							vid_hw_color_conversion_mode = true;
							result = Util_converter_y2r_init();
							Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_converter_y2r_init()..." + result.string + result.error_description, result.code);
							if(result.code != 0)
								vid_play_request = false;
						}
					}
				}

				if(vid_play_request)
				{
					for(int k = 0; k < num_of_video_tracks; k++)
					{
						for(int s = 0; s < DEF_SAPP0_BUFFERS; s++)
						{
							result = Draw_c2d_image_init(&vid_image[0][s][k], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
							{
								k = num_of_video_tracks;
								s = DEF_SAPP0_BUFFERS;
								break;
							}
							else
								vid_image_enabled[0][s][k] = true;

							if(vid_codec_width > 1024)
							{
								result = Draw_c2d_image_init(&vid_image[1][s][k], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
								{
									k = num_of_video_tracks;
									s = DEF_SAPP0_BUFFERS;
									break;
								}
								else
									vid_image_enabled[1][s][k] = true;
							}

							if(vid_codec_height > 1024)
							{
								result = Draw_c2d_image_init(&vid_image[2][s][k], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
								{
									k = num_of_video_tracks;
									s = DEF_SAPP0_BUFFERS;
									break;
								}
								else
									vid_image_enabled[2][s][k] = true;
							}

							if(vid_codec_width > 1024 && vid_codec_height > 1024)
							{
								result = Draw_c2d_image_init(&vid_image[3][s][k], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
								{
									k = num_of_video_tracks;
									s = DEF_SAPP0_BUFFERS;
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
								for(int s = 0; s < DEF_SAPP0_BUFFERS; s++)
								{
									if(vid_image_enabled[i][s][k])
										Draw_c2d_image_set_filter(&vid_image[i][s][k], vid_linear_filter);
								}
							}
						}

						if(vid_hw_decoding_mode)
							APT_SetAppCpuTimeLimit(5);
						else
							APT_SetAppCpuTimeLimit(80);

						if(vid_codec_width > 1024 && vid_codec_height > 1024)
						{
							vid_tex_width[0] = 1024;
							vid_tex_width[1] = vid_codec_width - 1024;
							vid_tex_width[2] = 1024;
							vid_tex_width[3] = vid_codec_width - 1024;
							vid_tex_height[0] = 1024;
							vid_tex_height[1] = 1024;
							vid_tex_height[2] = vid_codec_height - 1024;
							vid_tex_height[3] = vid_codec_height - 1024;
						}
						else if(vid_codec_width > 1024)
						{
							vid_tex_width[0] = 1024;
							vid_tex_width[1] = vid_codec_width - 1024;
							vid_tex_height[0] = vid_codec_height;
							vid_tex_height[1] = vid_codec_height;
						}
						else if(vid_codec_height > 1024)
						{
							vid_tex_width[0] = vid_codec_width;
							vid_tex_width[1] = vid_codec_width;
							vid_tex_height[0] = 1024;
							vid_tex_height[1] = vid_codec_height - 1024;
						}
						else
						{
							vid_tex_width[0] = vid_codec_width;
							vid_tex_height[0] = vid_codec_height;
						}
					}
				}
			}

			if(num_of_video_tracks > 0)
			{
				vid_full_screen_mode = true;
				vid_turn_off_bottom_screen_count = 300;
			}

			if(result.code != 0)
			{
				Util_err_set_error_message(result.string, result.error_description, DEF_SAPP0_DECODE_THREAD_STR, result.code);
				Util_err_set_error_show_flag(true);
				var_need_reflesh = true;
			}

			if(vid_remember_video_pos_mode && vid_play_request)
			{
				result = Util_file_load_from_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/", (u8*)(&saved_pos), sizeof(double), &read_size);
				if(result.code == 0 && saved_pos > 0 && saved_pos < vid_duration)
				{
					Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "saved pos : " + Util_convert_seconds_to_time(saved_pos / 1000));
					vid_seek_pos = saved_pos;
					vid_seek_request = true;
				}
			}

			while(vid_play_request)
			{
				while(vid_decode_video_request && vid_play_request)
					usleep(1000);

				result = Util_decoder_read_packet(&type, &packet_index, &key_frame, 0);
				if(result.code != 0)
					Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_decoder_read_packet()..." + result.string + result.error_description, result.code);

				var_afk_time = 0;

				if(!vid_select_audio_track_request && audio_track != vid_selected_audio_track)
				{
					audio_track = vid_selected_audio_track;
					Util_speaker_clear_buffer(0);
					Util_audio_decoder_get_info(&bitrate, &sample_rate, &ch, &vid_audio_format, &vid_duration, audio_track, &vid_audio_track_lang[audio_track], 0);
					vid_duration *= 1000;
					Util_speaker_init(0, ch, sample_rate);
				}

				if(vid_pause_request)
				{
					Util_speaker_pause(0);
					while(vid_pause_request && vid_play_request && !vid_seek_request && !vid_change_video_request)
						usleep(20000);
					
					Util_speaker_resume(0);
				}

				if(vid_seek_adjust_request && (num_of_video_tracks == 0 || type == "video"))
				{
					//Util_log_save("", std::to_string(vid_current_pos) + " " +std::to_string(vid_seek_pos) + " " + std::to_string(wait_count));
					if(wait_count <= 0)
					{
						if(vid_current_pos >= vid_seek_pos)
							vid_seek_adjust_request = false;
					}
					else
						wait_count--;
				}

				if(vid_seek_request)
				{
					//µs
					result = Util_decoder_seek(vid_seek_pos * 1000, 1, 0);
					Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_decoder_seek()..." + result.string + result.error_description, result.code);
					Util_speaker_clear_buffer(0);
					if(result.code == 0)
					{
						vid_seek_adjust_request = true;
						wait_count = 3;//sometimes cached previous frames so ignore first 3 frames 
					}

					vid_seek_request = false;
				}

				if(!vid_play_request || vid_change_video_request || result.code != 0)
				{
					if((!vid_play_request || vid_change_video_request) && vid_remember_video_pos_mode)
					{
						saved_pos = vid_current_pos;
						Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "last pos : " + Util_convert_seconds_to_time(saved_pos / 1000));
						Util_file_save_to_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/", (u8*)(&saved_pos), sizeof(double), true);
					}
					else if(result.code != 0 && vid_remember_video_pos_mode)//if playback end due to read packet error(EOF), remove saved time
						Util_file_delete_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/");

					break;
				}

				if(type == "audio" && packet_index == audio_track && (!vid_seek_adjust_request || num_of_video_tracks == 0))
				{
					result = Util_decoder_ready_audio_packet(audio_track, 0);
					if(result.code == 0)
					{
						osTickCounterUpdate(&counter);
						result = Util_audio_decoder_decode(&audio_size, &audio, &pos, audio_track, 0);
						osTickCounterUpdate(&counter);
						vid_audio_time = osTickCounterRead(&counter);
						
						if(num_of_video_tracks == 0 && !std::isnan(pos) && !std::isinf(pos))
							vid_current_pos = pos;
						
						if(result.code == 0 && !vid_seek_adjust_request)
						{
							vid_too_big = false;
							if(vid_volume != 100)
							{
								for(int i = 0; i < audio_size * ch; i += 2)
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
									Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Volume is too big!!!");
							}

							while(true)
							{
								result = Util_speaker_add_buffer(0, ch, audio, audio_size);
								if(result.code == 0 || !vid_play_request || vid_seek_request || vid_change_video_request)
									break;
								
								usleep(3000);
							}
						}
						else
							Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_audio_decoder_decode()..." + result.string + result.error_description, result.code);

						free(audio);
						audio = NULL;
					}
					else
						Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_decoder_ready_audio_packet()..." + result.string + result.error_description, result.code);
				}
				else if(type == "audio")
					Util_decoder_skip_audio_packet(packet_index, 0);
				else if(type == "video")
				{
					vid_packet_index[0] = packet_index;
					vid_key_frame = key_frame;
					vid_decode_video_request = true;
				}
			}

			vid_decode_video_request = false;
			vid_convert_request = false;
			while(vid_convert_wait_request || vid_decode_video_wait_request)
				usleep(10000);

			vid_decode_video_request = false;
			vid_convert_request = false;

			if(num_of_audio_tracks > 0)
			{
				Util_audio_decoder_exit(0);
				while(Util_speaker_is_playing(0) && vid_play_request)
					usleep(10000);
				
				Util_speaker_exit(0);
			}
			if(num_of_video_tracks > 0)
			{
				Util_video_decoder_exit(0);
				if(vid_hw_decoding_mode)
					Util_mvd_video_decoder_exit();
			}

			Util_decoder_close_file(0);
			if(vid_hw_color_conversion_mode)
				Util_converter_y2r_exit();
			
			for(int k = 0; k < 2; k++)
			{
				for(int i = 0; i < 4; i++)
				{
					for(int s = 0; s < DEF_SAPP0_BUFFERS; s++)
					{
						if(vid_image_enabled[i][s][k])
						{
							Draw_c2d_image_free(vid_image[i][s][k]);
							vid_image_enabled[i][s][k] = false;
						}
					}
				}
			}

			var_top_lcd_brightness = var_lcd_brightness;
			var_bottom_lcd_brightness = var_lcd_brightness;
			vid_turn_off_bottom_screen_count = 0;
			var_turn_on_bottom_lcd = true;
			vid_full_screen_mode = false;
			vid_pause_request = false;
			vid_seek_adjust_request = false;
			vid_seek_request = false;
			if(!vid_change_video_request)
				vid_play_request = false;
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp0_decode_video_thread(void* arg)
{
	Util_log_save(DEF_SAPP0_DECODE_VIDEO_THREAD_STR, "Thread started.");
	bool key_frame = false;
	int packet_index = 0;
	int w = 0;
	int h = 0;
	int skip = 0;
	double pos = 0;
	TickCounter counter;
	Result_with_string result;

	osTickCounterStart(&counter);

	while (vid_thread_run)
	{
		if(vid_play_request)
		{
			w = 0;
			h = 0;
			skip = 0;
			pos = 0;

			while(vid_play_request)
			{
				if(vid_decode_video_request)
				{
					vid_decode_video_wait_request = true;
					packet_index = vid_packet_index[0];
					key_frame = vid_key_frame;
					if(vid_allow_skip_frames && skip > vid_frametime && (!key_frame || vid_allow_skip_key_frames))
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
							osTickCounterUpdate(&counter);
							if(vid_hw_decoding_mode)
								result = Util_mvd_video_decoder_decode(&w, &h, &pos, 0);
							else
								result = Util_video_decoder_decode(&w, &h, &pos, packet_index, 0);
							osTickCounterUpdate(&counter);
							vid_video_time = osTickCounterRead(&counter);

							if(!std::isnan(pos) && !std::isinf(pos))
								vid_current_pos = pos;

							if(result.code == 0)
							{
								if(!vid_seek_adjust_request)
								{
									vid_packet_index[1] = packet_index;
									vid_convert_request = true;
								}
								else
									var_need_reflesh = true;
							}
							else
							{
								if(vid_hw_decoding_mode)
									Util_log_save(DEF_SAPP0_DECODE_VIDEO_THREAD_STR, "Util_video_decoder_decode()..." + result.string + result.error_description, result.code);
								else
									Util_log_save(DEF_SAPP0_DECODE_VIDEO_THREAD_STR, "Util_mvd_video_decoder_decode()..." + result.string + result.error_description, result.code);
							}

							vid_decode_video_wait_request = false;

							vid_key_frame_list[319] = key_frame;
							vid_time[0][319] = vid_video_time;

							if(vid_frametime - vid_video_time > 0)
							{
								if(!vid_seek_adjust_request)
									usleep((vid_frametime - vid_video_time) * 1000);
							}
							else if(vid_allow_skip_frames)
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
							Util_log_save(DEF_SAPP0_DECODE_VIDEO_THREAD_STR, "Util_decoder_ready_video_packet()..." + result.string + result.error_description, result.code);
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
	
	Util_log_save(DEF_SAPP0_DECODE_VIDEO_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp0_convert_thread(void* arg)
{
	Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Thread started.");
	u8* yuv_video = NULL;
	u8* video = NULL;
	int packet_index = 0;
	int image_num = 0;

	TickCounter counter[4];
	Result_with_string result;

	osTickCounterStart(&counter[0]);
	osTickCounterStart(&counter[1]);
	osTickCounterStart(&counter[2]);
	osTickCounterStart(&counter[3]);

	while (vid_thread_run)
	{	
		while(vid_play_request)
		{
			if(vid_convert_request)
			{
				vid_convert_wait_request = true;
				packet_index = vid_packet_index[1];
				osTickCounterUpdate(&counter[3]);
				osTickCounterUpdate(&counter[2]);
				if(vid_hw_decoding_mode)
					result = Util_mvd_video_decoder_get_image(&video, vid_codec_width, vid_codec_height, 0);
				else
					result = Util_video_decoder_get_image(&yuv_video, vid_codec_width, vid_codec_height, packet_index, 0);
				osTickCounterUpdate(&counter[2]);
				vid_copy_time[0] = osTickCounterRead(&counter[2]);

				vid_convert_request = false;
				if(result.code == 0)
				{
					osTickCounterUpdate(&counter[0]);
					if(!vid_hw_decoding_mode && vid_hw_color_conversion_mode)
						result = Util_converter_y2r_yuv420p_to_bgr565(yuv_video, &video, vid_codec_width, vid_codec_height, true);
					else if(!vid_hw_decoding_mode)
						result = Util_converter_yuv420p_to_bgr565_asm(yuv_video, &video, vid_codec_width, vid_codec_height);
					osTickCounterUpdate(&counter[0]);
					vid_convert_time = osTickCounterRead(&counter[0]);

					if(result.code == 0)
					{
						osTickCounterUpdate(&counter[1]);
						if(packet_index == 0)
							image_num = vid_image_num;
						else if(packet_index == 1)
							image_num = vid_image_num_3d;

						//Util_file_save_to_file("unknown.bin", DEF_MAIN_DIR + "debug/", video, vid_codec_width * vid_codec_height * 2, true);

						if(!vid_hw_decoding_mode && vid_hw_color_conversion_mode)
							result = Draw_set_texture_data_direct(&vid_image[0][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, GPU_RGB565);
						else
							result = Draw_set_texture_data(&vid_image[0][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, GPU_RGB565);
						if(result.code != 0)
							Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);

						if(vid_codec_width > 1024)
						{
							result = Draw_set_texture_data(&vid_image[1][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 0, 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
						}
						if(vid_codec_height > 1024)
						{
							result = Draw_set_texture_data(&vid_image[2][image_num][packet_index], video, vid_codec_width, vid_codec_height, 0, 1024, 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
						}
						if(vid_codec_width > 1024 && vid_codec_height > 1024)
						{
							result = Draw_set_texture_data(&vid_image[3][image_num][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
						}

						osTickCounterUpdate(&counter[1]);
						vid_copy_time[1] = osTickCounterRead(&counter[1]);

						if(packet_index == 0)
						{
							if(image_num + 1 < DEF_SAPP0_BUFFERS)
								vid_image_num++;
							else
								vid_image_num = 0;
						}
						else if(packet_index == 1)
						{
							if(image_num + 1 < DEF_SAPP0_BUFFERS)
								vid_image_num_3d++;
							else
								vid_image_num_3d = 0;
						}

						if(packet_index == 0)
							var_need_reflesh = true;
					}
					else
						Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Util_converter_yuv420p_to_bgr565()..." + result.string + result.error_description, result.code);
				}
				else
				{
					if(vid_hw_decoding_mode)
						Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Util_mvd_video_decoder_get_image()..." + result.string + result.error_description, result.code);
					else
						Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Util_video_decoder_get_image()..." + result.string + result.error_description, result.code);
				}

				free(yuv_video);
				free(video);
				yuv_video = NULL;
				video = NULL;

				vid_convert_wait_request = false;
				osTickCounterUpdate(&counter[3]);
				vid_time[1][319] = osTickCounterRead(&counter[3]);
				for(int i = 1; i < 320; i++)
					vid_time[1][i - 1] = vid_time[1][i];
			}
			else
				usleep(1000);
		}
		
		usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp0_resume(void)
{
	vid_thread_suspend = false;
	vid_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Sapp0_suspend(void)
{
	vid_thread_suspend = true;
	vid_main_run = false;
	Menu_resume();
}

Result_with_string Sapp0_load_msg(std::string lang)
{
	return Util_load_msg("sapp0_" + lang + ".txt", vid_msg, DEF_SAPP0_NUM_OF_MSG);
}

void Sapp0_init(void)
{
	Util_log_save(DEF_SAPP0_INIT_STR, "Initializing...");
	bool new_3ds = false;
	u8* cache = NULL;
	u32 read_size = 0;
	std::string out_data[12];
	Result_with_string result;
	
	APT_CheckNew3DS(&new_3ds);
	vid_thread_run = true;
	if(new_3ds)
	{
		vid_num_of_threads = 2;
		vid_enabled_cores[0] = true;
		vid_enabled_cores[1] = true;
		vid_enabled_cores[2] = var_core_2_available;
		vid_enabled_cores[3] = var_core_3_available;
		if(var_core_2_available)
			vid_num_of_threads++;
		if(var_core_3_available)
			vid_num_of_threads++;
		
		vid_decode_thread = threadCreate(Sapp0_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 0, false);
		vid_decode_video_thread = threadCreate(Sapp0_decode_video_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, var_core_2_available ? 2 : 0, false);
		vid_convert_thread = threadCreate(Sapp0_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_LOW, var_core_3_available ? 3 : 0, false);
	}
	else
	{
		vid_num_of_threads = 2;
		vid_enabled_cores[0] = true;
		vid_enabled_cores[1] = true;
		vid_enabled_cores[2] = false;
		vid_enabled_cores[3] = false;
		vid_decode_thread = threadCreate(Sapp0_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 0, false);
		vid_decode_video_thread = threadCreate(Sapp0_decode_video_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_convert_thread = threadCreate(Sapp0_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_LOW, 1, false);
	}

	vid_full_screen_mode = false;
	vid_show_controls = false;
	vid_allow_skip_frames = false;
	vid_show_full_screen_msg = true;
	vid_hw_decoding_mode = false;
	vid_hw_color_conversion_mode = false;
	vid_seek_request = false;
	vid_seek_adjust_request = false;
	vid_use_hw_decoding_request = true;
	vid_use_hw_color_conversion_request = true;
	vid_use_multi_threaded_decoding_request = true;
	vid_request_thread_mode = DEF_DECODER_THREAD_TYPE_AUTO;
	vid_thread_mode = DEF_DECODER_THREAD_TYPE_NONE;
	vid_menu_mode = DEF_SAPP0_MENU_NONE;
	vid_volume = 100;
	vid_seek_duration = 10;
	vid_lower_resolution = 0;
	vid_num_of_audio_tracks = 0;
	vid_selected_audio_track = 0;
	vid_lr_count = 0;
	vid_cd_count = 0;
	vid_x = 0;
	vid_y = 15;
	vid_frametime = 0;
	vid_framerate = 0;
	vid_current_pos = 0;
	vid_duration = 0;
	vid_zoom = 1;
	vid_sar_width = 1;
	vid_sar_height = 1;
	vid_codec_width = 0;
	vid_codec_height = 0;
	vid_width = 0;
	vid_height = 0;
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
	vid_file = "";
	vid_dir = "";
	vid_video_format = "n/a";
	vid_audio_format = "n/a";

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		vid_audio_track_lang[i] = "";

	for(int i = 0; i < 90; i++)
		vid_recent_time[i] = 0;

	for(int i = 0; i < 4; i++)
	{
		vid_tex_width[i] = 0;
		vid_tex_height[i] = 0;
	}

	for(int i = 0 ; i < 320; i++)
	{
		vid_time[0][i] = 0;
		vid_time[1][i] = 0;
	}

	for(int k = 0; k < 2; k++)
	{
		for(int i = 0; i < 4; i++)
		{
			for(int s = 0; s < DEF_SAPP0_BUFFERS; s++)
				vid_image_enabled[i][s][k] = false;
		}
	}

	cache = (u8*)malloc(0x1000);
	result = Util_file_load_from_file("vid_settings.txt", DEF_MAIN_DIR, cache, 0x1000, &read_size);
	Util_log_save(DEF_SAPP0_INIT_STR, "Util_file_load_from_file()..." + result.string + result.error_description, result.code);

	result = Util_parse_file((char*)cache, 12, out_data);//settings file for v1.3.2
	Util_log_save(DEF_SAPP0_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
	if(result.code != 0)
	{
		result = Util_parse_file((char*)cache, 9, out_data);//settings file for v1.3.1
		Util_log_save(DEF_SAPP0_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
		out_data[9] = "0";
		out_data[10] = "0";
		out_data[11] = "1";
	}
	if(result.code != 0)
	{
		result = Util_parse_file((char*)cache, 7, out_data);//settings file for v1.3.0
		Util_log_save(DEF_SAPP0_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
		out_data[7] = "100";
		out_data[8] = "10";
		out_data[9] = "0";
		out_data[10] = "0";
		out_data[11] = "1";
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
		vid_disable_resize_move_mode = (out_data[10] == "1");
		vid_remember_video_pos_mode = (out_data[11] == "1");
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

	free(cache);
	cache = NULL;

	vid_banner_texture_num = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/banner.t3x", vid_banner_texture_num, vid_banner, 0, 2);
	Util_log_save(DEF_SAPP0_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	vid_control_texture_num = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/control.t3x", vid_control_texture_num, vid_control, 0, 2);
	Util_log_save(DEF_SAPP0_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

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
	vid_disable_resize_move_button.c2d = var_square_image[0];
	vid_remember_video_pos_button.c2d = var_square_image[0];

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		vid_audio_track_button[i].c2d = var_square_image[0];

	Sapp0_resume();
	vid_already_init = true;
	Util_log_save(DEF_SAPP0_INIT_STR, "Initialized.");
}

void Sapp0_exit(void)
{
	Util_log_save(DEF_SAPP0_EXIT_STR, "Exiting...");
	std::string data = "";
	Result_with_string result;

	vid_already_init = false;
	vid_thread_suspend = false;
	vid_thread_run = false;
	vid_convert_request = false;
	vid_decode_video_request = false;
	vid_play_request = false;
	vid_select_audio_track_request = false;

	data = "<0>" + std::to_string(vid_linear_filter) + "</0><1>" + std::to_string(vid_allow_skip_frames) + "</1><2>"
	+ std::to_string(vid_allow_skip_key_frames) + "</2><3>" + std::to_string(vid_use_hw_decoding_request) + "</3><4>"
	+ std::to_string(vid_use_hw_color_conversion_request) + "</4><5>" + std::to_string(vid_use_multi_threaded_decoding_request) + "</5><6>"
	+ std::to_string(vid_lower_resolution) + "</6><7>"	+ std::to_string(vid_volume) + "</7><8>"
	+ std::to_string(vid_seek_duration) + "</8><9>" + std::to_string(vid_correct_aspect_ratio_mode) + "</9><10>"
	+ std::to_string(vid_disable_resize_move_mode) + "</10><11>" + std::to_string(vid_remember_video_pos_mode) + "</11>";
	Util_file_save_to_file("vid_settings.txt", DEF_MAIN_DIR, (u8*)data.c_str(), data.length(), true);

	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(vid_decode_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(vid_decode_video_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(vid_convert_thread, DEF_THREAD_WAIT_TIME));
	threadFree(vid_decode_thread);
	threadFree(vid_decode_video_thread);
	threadFree(vid_convert_thread);

	Draw_free_texture(vid_banner_texture_num);
	Draw_free_texture(vid_control_texture_num);
	
	Util_log_save(DEF_SAPP0_EXIT_STR, "Exited.");
}

void Sapp0_main(void)
{
	int color = DEF_DRAW_BLACK;
	int disabled_color = DEF_DRAW_WEAK_BLACK;
	int back_color = DEF_DRAW_WHITE;
	int image_num = 0;
	int image_num_3d = 0;
	double image_width[4] = { 0, 0, 0, 0, };
	double image_height[4] = { 0, 0, 0, 0, };
	std::string thread_mode[3] = { "none", "frame", "slice" };
	std::string lower_resolution_mode[3] = { "OFF (x1.0)", "ON (x0.5)", "ON (x0.25)" };
	std::string swkbd_input = "";
	Hid_info key;
	Util_hid_query_key_state(&key);
	Util_hid_key_flag_reset();

	if (var_night_mode)
	{
		color = DEF_DRAW_WHITE;
		disabled_color = DEF_DRAW_WEAK_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	if(vid_image_num - 1 >= 0)
		image_num = vid_image_num - 1;
	else
		image_num = DEF_SAPP0_BUFFERS - 1;

	if(vid_image_num_3d - 1 >= 0)
		image_num_3d = vid_image_num_3d - 1;
	else
		image_num_3d = DEF_SAPP0_BUFFERS - 1;

	if(vid_turn_off_bottom_screen_count > 0)
	{
		vid_turn_off_bottom_screen_count--;
		if(vid_turn_off_bottom_screen_count == 0)
			var_turn_on_bottom_lcd = false;
		if(var_bottom_lcd_brightness > 10)
			var_bottom_lcd_brightness--;
	}

	for(int i = 0; i < 4; i++)
	{
		image_width[i] = vid_tex_width[i] * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1) * vid_zoom;
		image_height[i] = vid_tex_height[i] * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1) * vid_zoom;
	}
	
	if(var_need_reflesh || !var_eco_mode)
	{
		var_need_reflesh = false;
		Draw_frame_ready();

		if(var_turn_on_top_lcd)
		{
			Draw_screen_ready(0, vid_full_screen_mode ? DEF_DRAW_BLACK : back_color);

			if(vid_play_request)
			{
				//video
				Draw_texture(vid_image[0][image_num][0].c2d, vid_x, vid_y, image_width[0], image_height[0]);
				if(vid_codec_width > 1024)
					Draw_texture(vid_image[1][image_num][0].c2d, (vid_x + image_width[0]), vid_y, image_width[1], image_height[1]);
				if(vid_codec_height > 1024)
					Draw_texture(vid_image[2][image_num][0].c2d, vid_x, (vid_y + image_height[0]), image_width[2], image_height[2]);
				if(vid_codec_width > 1024 && vid_codec_height > 1024)
					Draw_texture(vid_image[3][image_num][0].c2d, (vid_x + image_width[0]), (vid_y + image_height[0]), image_width[3], image_height[3]);
			}
			else
				Draw_texture(vid_banner[var_night_mode], 0, 15, 400, 225);
			
			if(vid_full_screen_mode && vid_turn_off_bottom_screen_count > 0 && vid_show_full_screen_msg)
			{
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_BLACK, 40, 20, 320, 30);
				Draw(vid_msg[DEF_SAPP0_FULL_SCREEN_MSG], 42.5, 20, 0.45, 0.45, DEF_DRAW_WHITE);
			}

			if(vid_seek_request || vid_seek_adjust_request)
			{
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_BLACK, 150, 220, 100, 20);
				Draw(vid_msg[DEF_SAPP0_SEEKING_MSG], 152.5, 220, 0.5, 0.5, DEF_DRAW_WHITE);
			}

			if(Util_log_query_log_show_flag())
				Util_log_draw();

			if(!vid_full_screen_mode)
				Draw_top_ui();

			if(var_3d_mode)
			{
				Draw_screen_ready(2, vid_full_screen_mode ? DEF_DRAW_BLACK : back_color);

				if(vid_play_request)
				{
					//3d video (right eye)
					Draw_texture(vid_image[0][image_num_3d][1].c2d, vid_x, vid_y, image_width[0], image_height[0]);
					if(vid_codec_width > 1024)
						Draw_texture(vid_image[1][image_num_3d][1].c2d, (vid_x + image_width[0]), vid_y, image_width[1], image_height[1]);
					if(vid_codec_height > 1024)
						Draw_texture(vid_image[2][image_num_3d][1].c2d, vid_x, (vid_y + image_height[0]), image_width[2], image_height[2]);
					if(vid_codec_width > 1024 && vid_codec_height > 1024)
						Draw_texture(vid_image[3][image_num_3d][1].c2d, (vid_x + image_width[0]), (vid_y + image_height[0]), image_width[3], image_height[3]);
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
			Draw_screen_ready(1, back_color);

			Draw(DEF_SAPP0_VER, 0, 0, 0.4, 0.4, DEF_DRAW_GREEN);

			//codec info
			Draw(vid_video_format, 0, 10, 0.5, 0.5, color);
			Draw(vid_audio_format, 0, 20, 0.5, 0.5, color);
			Draw(std::to_string(vid_width) + "x" + std::to_string(vid_height) + "(" + std::to_string(vid_codec_width) + "x" + std::to_string(vid_codec_height) + ")" + "@" + std::to_string(vid_framerate).substr(0, 5) + "fps", 0, 30, 0.5, 0.5, color);

			if(vid_play_request)
			{
				//video
				Draw_texture(vid_image[0][image_num][0].c2d, vid_x - 40, vid_y - 240, image_width[0], image_height[0]);
				if(vid_codec_width > 1024)
					Draw_texture(vid_image[1][image_num][0].c2d, (vid_x + image_width[0] - 40), vid_y - 240, image_width[1], image_height[1]);
				if(vid_codec_height > 1024)
					Draw_texture(vid_image[2][image_num][0].c2d, vid_x - 40, (vid_y + image_height[0] - 240), image_width[2], image_height[2]);
				if(vid_codec_width > 1024 && vid_codec_height > 1024)
					Draw_texture(vid_image[3][image_num][0].c2d, (vid_x + image_width[0] - 40), (vid_y + image_height[0] - 240), image_width[3], image_height[3]);
			}

			if(vid_menu_mode != DEF_SAPP0_MENU_NONE)
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_GREEN, 0, 50, 320, 130);

			if(vid_menu_mode == DEF_SAPP0_MENU_SETTINGS_0)
			{
				//select audio track
				Draw_texture(&vid_select_audio_track_button, vid_select_audio_track_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 60, 145, 10);
				Draw(vid_msg[DEF_SAPP0_AUDIO_TRACK_MSG], 12.5, 60, 0.4, 0.4, color);

				//texture filter
				Draw_texture(&vid_texture_filter_button, vid_texture_filter_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 165, 60, 145, 10);
				Draw(vid_msg[DEF_SAPP0_TEX_FILTER_MSG] + (vid_linear_filter ? "ON" : "OFF"), 167.5, 60, 0.4, 0.4, color);

				//allow skip frames
				Draw_texture(&vid_allow_skip_frames_button, vid_allow_skip_frames_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 80, 145, 10);
				if(var_lang == "it")
					Draw(vid_msg[DEF_SAPP0_SKIP_FRAME_MSG] + (vid_allow_skip_frames ? "ON" : "OFF"), 12.5, 80, 0.375, 0.375, color);
				else
					Draw(vid_msg[DEF_SAPP0_SKIP_FRAME_MSG] + (vid_allow_skip_frames ? "ON" : "OFF"), 12.5, 80, 0.4, 0.4, color);

				//allow skip key frames
				Draw_texture(&vid_allow_skip_key_frames_button, vid_allow_skip_key_frames_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 165, 80, 145, 10);
				Draw(vid_msg[DEF_SAPP0_SKIP_KEY_FRAME_MSG] + (vid_allow_skip_key_frames ? "ON" : "OFF"), 167.5, 80, 0.35, 0.35, vid_allow_skip_frames ? color : disabled_color);

				//volume
				Draw_texture(&vid_volume_button, vid_volume_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 100, 145, 10);
				Draw(vid_msg[DEF_SAPP0_VOLUME_MSG] + std::to_string(vid_volume) + "%", 12.5, 100, 0.4, 0.4, vid_too_big ? DEF_DRAW_RED : color);

				//seek duration
				Draw_texture(&vid_seek_duration_button, vid_seek_duration_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 165, 100, 145, 10);
				Draw(vid_msg[DEF_SAPP0_SEEK_MSG] + std::to_string(vid_seek_duration) + "s", 167.5, 100, 0.4, 0.4, color);

				//correct aspect ratio
				Draw_texture(&vid_correct_aspect_ratio_button, vid_correct_aspect_ratio_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 120, 145, 10);
				Draw(vid_msg[DEF_SAPP0_ASPECT_RATIO_MSG] + (vid_correct_aspect_ratio_mode ? "ON" : "OFF"), 12.5, 120, 0.4, 0.4, color);

				//disable resize and move video
				Draw_texture(&vid_disable_resize_move_button, vid_disable_resize_move_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 165, 120, 145, 10);
				if(var_lang == "zh-cn" || var_lang == "it")
					Draw(vid_msg[DEF_SAPP0_DISABLE_RESIZE_MOVE_MSG] + (vid_disable_resize_move_mode ? "ON" : "OFF"), 167.5, 120, 0.4, 0.4, color);
				else
					Draw(vid_msg[DEF_SAPP0_DISABLE_RESIZE_MOVE_MSG] + (vid_disable_resize_move_mode ? "ON" : "OFF"), 167.5, 120, 0.35, 0.35, color);

				//remember video pos
				Draw_texture(&vid_remember_video_pos_button, vid_remember_video_pos_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 140, 145, 10);
				Draw(vid_msg[DEF_SAPP0_REMEMBER_POS_MSG] + (vid_remember_video_pos_mode ? "ON" : "OFF"), 12.5, 140, 0.4, 0.4, color);

				Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 0, 180, 100, 8);
				Draw_texture(&vid_menu_button[1], vid_menu_button_selected[1] ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
				Draw_texture(&vid_menu_button[2], vid_menu_button_selected[2] ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
			}
			else if(vid_menu_mode == DEF_SAPP0_MENU_SETTINGS_1)
			{
				//use hw decoding
				Draw_texture(&vid_use_hw_decoding_button, vid_use_hw_decoding_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 60, 300, 10);
				Draw(vid_msg[DEF_SAPP0_HW_DECODER_MSG] + (vid_use_hw_decoding_request ? "ON" : "OFF"), 12.5, 60, 0.4, 0.4, 
				(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL || vid_play_request) ? disabled_color : color);

				//use hw color conversion
				Draw_texture(&vid_use_hw_color_conversion_button, vid_use_hw_color_conversion_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 80, 300, 10);
				Draw(vid_msg[DEF_SAPP0_HW_CONVERTER_MSG] + (vid_use_hw_color_conversion_request ? "ON" : "OFF"), 12.5, 80, 0.4, 0.4, vid_play_request ? disabled_color : color);

				//use multi-threaded decoding (in software decoding)
				Draw_texture(&vid_use_multi_threaded_decoding_button, vid_use_multi_threaded_decoding_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 100, 300, 10);
				Draw(vid_msg[DEF_SAPP0_MULTI_THREAD_MSG] + (vid_use_multi_threaded_decoding_request ? "ON" : "OFF"), 12.5, 100, 0.4, 0.4, vid_play_request ? disabled_color : color);

				//lower resolution
				Draw_texture(&vid_lower_resolution_button, vid_lower_resolution_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 10, 120, 300, 10);
				if(var_lang == "it")
					Draw(vid_msg[DEF_SAPP0_LOWER_RESOLUTION_MSG] + lower_resolution_mode[vid_lower_resolution], 12.5, 120, 0.375, 0.375, vid_play_request ? disabled_color : color);
				else
					Draw(vid_msg[DEF_SAPP0_LOWER_RESOLUTION_MSG] + lower_resolution_mode[vid_lower_resolution], 12.5, 120, 0.4, 0.4, vid_play_request ? disabled_color : color);

				Draw_texture(&vid_menu_button[0], vid_menu_button_selected[0] ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
				Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 110, 180, 100, 8);
				Draw_texture(&vid_menu_button[2], vid_menu_button_selected[2] ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
			}
			else if(vid_menu_mode == DEF_SAPP0_MENU_INFO)
			{
				//decoding detail
				for(int i = 0; i < 319; i++)
				{
					Draw_line(i, 110 - vid_time[1][i], DEF_DRAW_BLUE, i + 1, 110 - vid_time[1][i + 1], DEF_DRAW_BLUE, 1);//Thread 1
					Draw_line(i, 110 - vid_time[0][i], DEF_DRAW_RED, i + 1, 110 - vid_time[0][i + 1], DEF_DRAW_RED, 1);//Thread 0
				}

				Draw_line(0, 110, color, 320, 110, color, 2);
				Draw_line(0, 110 - vid_frametime, 0xFFFFFF00, 320, 110 - vid_frametime, 0xFFFFFF00, 2);
				for(int i = 0; i < 319; i++)
				{
					if(vid_key_frame_list[i])
						Draw_line(i, 110, disabled_color, i, 50, disabled_color, 2);
				}

				if(vid_total_frames != 0 && vid_min_time != 0  && vid_recent_total_time != 0)
				{
					if(!vid_hw_decoding_mode && vid_thread_mode == DEF_DECODER_THREAD_TYPE_FRAME)
						Draw("*When thread_type == frame, the red graph is unusable", 0, 110, 0.4, 0.4, color);
					else
						Draw("avg " + std::to_string(1000 / (vid_total_time / vid_total_frames)).substr(0, 5) + " min " + std::to_string(1000 / vid_max_time).substr(0, 5) 
						+  " max " + std::to_string(1000 / vid_min_time).substr(0, 5) + " recent avg " + std::to_string(1000 / (vid_recent_total_time / 90)).substr(0, 5) +  " fps", 0, 110, 0.4, 0.4, color);
				}

				Draw("Deadline : " + std::to_string(vid_frametime).substr(0, 5) + "ms", 0, 120, 0.4, 0.4, DEF_DRAW_AQUA);
				Draw("Video decode : " + std::to_string(vid_video_time).substr(0, 5) + "ms", 0, 130, 0.4, 0.4, DEF_DRAW_RED);
				Draw("Audio decode : " + std::to_string(vid_audio_time).substr(0, 5) + "ms", 0, 140, 0.4, 0.4, DEF_DRAW_RED);
				Draw("Data copy 0 : " + std::to_string(vid_copy_time[0]).substr(0, 5) + "ms", 160, 120, 0.4, 0.4, DEF_DRAW_BLUE);
				Draw("Color convert : " + std::to_string(vid_convert_time).substr(0, 5) + "ms", 160, 130, 0.4, 0.4, DEF_DRAW_BLUE);
				Draw("Data copy 1 : " + std::to_string(vid_copy_time[1]).substr(0, 5) + "ms", 160, 140, 0.4, 0.4, DEF_DRAW_BLUE);
				Draw("Thread 0 : " + std::to_string(vid_time[0][319]).substr(0, 6) + "ms", 0, 150, 0.5, 0.5, DEF_DRAW_RED);
				Draw("Thread 1 : " + std::to_string(vid_time[1][319]).substr(0, 6) + "ms", 160, 150, 0.5, 0.5, DEF_DRAW_BLUE);
				Draw((std::string)"Hw decoding : " + (vid_hw_decoding_mode ? "yes" : "no"), 0, 160, 0.4, 0.4, color);
				Draw((std::string)"Hw color conversion : " + ((vid_hw_decoding_mode || vid_hw_color_conversion_mode) ? "yes" : "no"), 160, 160, 0.4, 0.4, color);
				Draw((std::string)"Thread type : " + thread_mode[vid_hw_decoding_mode ? 0 : vid_thread_mode], 0, 170, 0.4, 0.4, color);

				Draw_texture(&vid_menu_button[0], vid_menu_button_selected[0] ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
				Draw_texture(&vid_menu_button[1], vid_menu_button_selected[1] ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
				Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 220, 180, 100, 8);
			}

			//controls
			Draw_texture(&vid_control_button, vid_control_button_selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 165, 195, 145, 10);
			Draw(vid_msg[DEF_SAPP0_CONTROLS_MSG], 167.5, 195, 0.4, 0.4, color);

			//time bar
			Draw(Util_convert_seconds_to_time(vid_current_pos / 1000) + "/" + Util_convert_seconds_to_time(vid_duration / 1000), 10, 192.5, 0.5, 0.5, color);
			Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 5, 210, 310, 10);
			if(vid_duration != 0)
				Draw_texture(var_square_image[0], 0xFF800080, 5, 210, 310 * ((vid_current_pos / 1000) / (vid_duration / 1000)), 10);

			if(vid_show_controls)
			{
				Draw_texture(vid_control[var_night_mode], 80, 20, 160, 160);
				Draw(vid_msg[DEF_SAPP0_CONTROL_DESCRIPTION_MSG], 122.5, 47.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[DEF_SAPP0_CONTROL_DESCRIPTION_MSG + 1], 122.5, 62.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[DEF_SAPP0_CONTROL_DESCRIPTION_MSG + 2], 122.5, 77.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[DEF_SAPP0_CONTROL_DESCRIPTION_MSG + 3], 122.5, 92.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[DEF_SAPP0_CONTROL_DESCRIPTION_MSG + 4], 135, 107.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[DEF_SAPP0_CONTROL_DESCRIPTION_MSG + 5], 122.5, 122.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[DEF_SAPP0_CONTROL_DESCRIPTION_MSG + 6], 132.5, 137.5, 0.45, 0.45, DEF_DRAW_BLACK);
			}

			if(vid_select_audio_track_request)
			{
				Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 40, 20, 240, 140);
				Draw(vid_msg[DEF_SAPP0_AUDIO_TRACK_DESCRIPTION_MSG], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

				for(int i = 0; i < vid_num_of_audio_tracks; i++)
				{
					Draw_texture(&vid_audio_track_button[i], vid_audio_track_button_selected[i] ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 40, 40 + (i * 12), 240, 12);
					Draw("Track " + std::to_string(i) + "(" + vid_audio_track_lang[i] + ")", 42.5, 40 + (i * 12), 0.475, 0.475, i == vid_selected_audio_track ? DEF_DRAW_RED : color);
				}

				Draw_texture(&vid_ok_button, vid_ok_button_selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
				Draw("OK", 152.5, 140, 0.4, 0.4, DEF_DRAW_BLACK);
			}

			if(Util_expl_query_show_flag())
				Util_expl_draw();

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
	else if(Util_expl_query_show_flag())
		Util_expl_main(key);
	else
	{
		if(vid_full_screen_mode)
		{
			if(key.p_select || key.p_touch || aptShouldJumpToHome())
			{
				if(vid_width > 0 && vid_height > 0)
				{
					//fit to screen size
					if((((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400) >= (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 225))
						vid_zoom = 1.0 / (((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400);
					else
						vid_zoom = 1.0 / (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 225);

					vid_x = (400 - (vid_width * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1))) / 2;
					vid_y = (225 - (vid_height * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1))) / 2;
					vid_y += 15;
				}
				vid_turn_off_bottom_screen_count = 0;
				vid_full_screen_mode = false;
				var_turn_on_bottom_lcd = true;
				var_top_lcd_brightness = var_lcd_brightness;
				var_bottom_lcd_brightness = var_lcd_brightness;
				vid_show_full_screen_msg = false;
				var_need_reflesh = true;
			}
			else if(key.p_a)
			{
				if(vid_play_request)
					vid_pause_request = !vid_pause_request;
				else
					vid_play_request = true;
				var_need_reflesh = true;
			}
			else if(key.p_d_right)
			{
				if(vid_current_pos + (vid_seek_duration * 1000) > vid_duration)
					vid_seek_pos = vid_duration;
				else
					vid_seek_pos = vid_current_pos + (vid_seek_duration * 1000);

				vid_seek_request = true;
				var_need_reflesh = true;
			}
			if(key.p_d_left)
			{
				if(vid_current_pos - (vid_seek_duration * 1000) < 0)
					vid_seek_pos = 0;
				else
					vid_seek_pos = vid_current_pos - (vid_seek_duration * 1000);
				
				vid_seek_request = true;
				var_need_reflesh = true;
			}
		}
		else if(vid_select_audio_track_request)
		{
			if(key.p_d_down || key.p_c_down)
			{
				if(vid_selected_audio_track + 1 < vid_num_of_audio_tracks)
					vid_selected_audio_track++;
				var_need_reflesh = true;
			}
			else if(key.p_d_up || key.p_c_up)
			{
				if(vid_selected_audio_track - 1 > -1)
					vid_selected_audio_track--;
				var_need_reflesh = true;
			}
			else if(Util_hid_is_pressed(key, vid_ok_button))
			{
				vid_ok_button_selected = true;
				var_need_reflesh = true;
			}
			else if(key.p_a || (Util_hid_is_released(key, vid_ok_button) && vid_ok_button_selected))
			{
				vid_select_audio_track_request = false;
				var_need_reflesh = true;
			}

			for(int i = 0; i < vid_num_of_audio_tracks; i++)
			{
				if(Util_hid_is_pressed(key, vid_audio_track_button[i]))
				{
					vid_audio_track_button_selected[i] = true;
					var_need_reflesh = true;
					break;
				}
				else if(Util_hid_is_released(key, vid_audio_track_button[i]) && vid_audio_track_button_selected[i])
				{
					vid_selected_audio_track = i;
					var_need_reflesh = true;
					break;
				}
			}
		}
		else if(vid_show_controls)
		{
			if(Util_hid_is_pressed(key, vid_control_button))
			{
				vid_control_button_selected = true;
				var_need_reflesh = true;
			}
			else if((key.p_a || Util_hid_is_released(key, vid_control_button)) && vid_control_button_selected)
			{
				vid_show_controls = false;
				var_need_reflesh = true;
			}
		}
		else
		{
			if(vid_menu_mode == DEF_SAPP0_MENU_SETTINGS_0)
			{
				if(Util_hid_is_pressed(key, vid_select_audio_track_button))
				{
					vid_select_audio_track_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_select_audio_track_button) && vid_select_audio_track_button_selected)
				{
					vid_select_audio_track_request = !vid_select_audio_track_request;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_texture_filter_button))
				{
					vid_texture_filter_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_texture_filter_button) && vid_texture_filter_button_selected)
				{
					vid_linear_filter = !vid_linear_filter;
					for(int k = 0; k < 2; k++)
					{
						for(int i = 0; i < 4; i++)
						{
							for(int s = 0; s < DEF_SAPP0_BUFFERS; s++)
							{
								if(vid_image_enabled[i][s][k])
									Draw_c2d_image_set_filter(&vid_image[i][s][k], vid_linear_filter);
							}
						}
					}
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_allow_skip_frames_button))
				{
					vid_allow_skip_frames_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_allow_skip_frames_button) && vid_allow_skip_frames_button_selected)
				{
					vid_allow_skip_frames = !vid_allow_skip_frames;
					if(vid_allow_skip_key_frames)
						vid_allow_skip_key_frames = false;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_allow_skip_key_frames_button) && vid_allow_skip_frames)
				{
					vid_allow_skip_key_frames_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_allow_skip_key_frames_button) && vid_allow_skip_key_frames_button_selected && vid_allow_skip_frames)
				{
					vid_allow_skip_key_frames = !vid_allow_skip_key_frames;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_volume_button))
				{
					vid_volume_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_volume_button) && vid_volume_button_selected)
				{
					vid_pause_request = true;
					Util_swkbd_init(SWKBD_TYPE_NUMPAD, SWKBD_NOTEMPTY, 1, 3, "", std::to_string(vid_volume));
					Util_swkbd_launch(3, &swkbd_input);
					vid_volume = atoi((char*)swkbd_input.c_str());
					vid_pause_request = false;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_seek_duration_button))
				{
					vid_seek_duration_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_seek_duration_button) && vid_seek_duration_button_selected)
				{
					vid_pause_request = true;
					Util_swkbd_init(SWKBD_TYPE_NUMPAD, SWKBD_NOTEMPTY, 1, 2, "", std::to_string(vid_seek_duration));
					Util_swkbd_launch(2, &swkbd_input);
					vid_seek_duration = atoi((char*)swkbd_input.c_str());
					if(vid_seek_duration == 0)
						vid_seek_duration = 1;
					vid_pause_request = false;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_correct_aspect_ratio_button))
				{
					vid_correct_aspect_ratio_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_correct_aspect_ratio_button) && vid_correct_aspect_ratio_button_selected)
				{
					vid_correct_aspect_ratio_mode = !vid_correct_aspect_ratio_mode;
					if(vid_width > 0 && vid_height > 0)
					{
						//fit to screen size
						if((((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400) >= (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 225))
							vid_zoom = 1.0 / (((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400);
						else
							vid_zoom = 1.0 / (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 225);

						vid_x = (400 - (vid_width * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1))) / 2;
						vid_y = (225 - (vid_height * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1))) / 2;
						vid_y += 15;
					}
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_disable_resize_move_button))
				{
					vid_disable_resize_move_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_disable_resize_move_button) && vid_disable_resize_move_button_selected)
				{
					vid_disable_resize_move_mode = !vid_disable_resize_move_mode;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_remember_video_pos_button))
				{
					vid_remember_video_pos_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_remember_video_pos_button) && vid_remember_video_pos_button_selected)
				{
					vid_remember_video_pos_mode = !vid_remember_video_pos_mode;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[1]))
				{
					vid_menu_button_selected[1] = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_menu_button[1]) && vid_menu_button_selected[1])
				{
					vid_menu_mode = DEF_SAPP0_MENU_SETTINGS_1;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[2]))
				{
					vid_menu_button_selected[2] = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_menu_button[2]) && vid_menu_button_selected[2])
				{
					vid_menu_mode = DEF_SAPP0_MENU_INFO;
					var_need_reflesh = true;
				}
			}
			if(vid_menu_mode == DEF_SAPP0_MENU_SETTINGS_1)
			{
				if(Util_hid_is_pressed(key, vid_use_hw_decoding_button) && !(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL) && !vid_play_request)
				{
					vid_use_hw_decoding_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_use_hw_decoding_button) && !(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL) && !vid_play_request && vid_use_hw_decoding_button_selected)
				{
					vid_use_hw_decoding_request = !vid_use_hw_decoding_request;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_use_hw_color_conversion_button) && !vid_play_request)
				{
					vid_use_hw_color_conversion_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_use_hw_color_conversion_button) && !vid_play_request && vid_use_hw_color_conversion_button_selected)
				{
					vid_use_hw_color_conversion_request = !vid_use_hw_color_conversion_request;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_use_multi_threaded_decoding_button) && !vid_play_request)
				{
					vid_use_multi_threaded_decoding_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_use_multi_threaded_decoding_button) && !vid_play_request && vid_use_multi_threaded_decoding_button_selected)
				{
					vid_use_multi_threaded_decoding_request = !vid_use_multi_threaded_decoding_request;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_lower_resolution_button) && !vid_play_request)
				{
					vid_lower_resolution_button_selected = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_lower_resolution_button) && !vid_play_request && vid_lower_resolution_button_selected)
				{
					if(vid_lower_resolution + 1 > 2)
						vid_lower_resolution = 0;
					else
						vid_lower_resolution++;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[0]))
				{
					vid_menu_button_selected[0] = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_menu_button[0]) && vid_menu_button_selected[0])
				{
					vid_menu_mode = DEF_SAPP0_MENU_SETTINGS_0;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[2]))
				{
					vid_menu_button_selected[2] = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_menu_button[2]) && vid_menu_button_selected[2])
				{
					vid_menu_mode = DEF_SAPP0_MENU_INFO;
					var_need_reflesh = true;
				}
			}
			else if(vid_menu_mode == DEF_SAPP0_MENU_INFO)
			{
				if(Util_hid_is_pressed(key, vid_menu_button[0]))
				{
					vid_menu_button_selected[0] = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_menu_button[0]) && vid_menu_button_selected[0])
				{
					vid_menu_mode = DEF_SAPP0_MENU_SETTINGS_0;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_pressed(key, vid_menu_button[1]))
				{
					vid_menu_button_selected[1] = true;
					var_need_reflesh = true;
				}
				else if(Util_hid_is_released(key, vid_menu_button[1]) && vid_menu_button_selected[1])
				{
					vid_menu_mode = DEF_SAPP0_MENU_SETTINGS_1;
					var_need_reflesh = true;
				}
			}

			if (key.p_start || (key.p_touch && key.touch_x >= 110 && key.touch_x <= 230 && key.touch_y >= 220 && key.touch_y <= 240))
				Sapp0_suspend();
			else if(key.p_select)
			{
				if(vid_width > 0 && vid_height > 0)
				{
					//fit to screen size
					if((((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400) >= (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 240))
						vid_zoom = 1.0 / (((double)vid_width * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1)) / 400);
					else 
						vid_zoom = 1.0 / (((double)vid_height * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1)) / 240);

					vid_x = (400 - (vid_width * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_width : 1))) / 2;
					vid_y = (240 - (vid_height * vid_zoom * (vid_correct_aspect_ratio_mode ? vid_sar_height : 1))) / 2;
				}
				vid_turn_off_bottom_screen_count = 300;
				vid_full_screen_mode = true;
				var_top_lcd_brightness = var_lcd_brightness;
				var_bottom_lcd_brightness = var_lcd_brightness;
				var_need_reflesh = true;
			}
			else if(key.p_a)
			{
				if(vid_play_request)
					vid_pause_request = !vid_pause_request;
				else
					vid_play_request = true;

				var_need_reflesh = true;
			}
			else if(key.p_b)
			{
				vid_play_request = false;
				var_need_reflesh = true;
			}
			else if(key.p_x)
			{
				Util_expl_set_show_flag(true);
				Util_expl_set_callback(Sapp0_callback);
				Util_expl_set_cancel_callback(Sapp0_cancel);
			}
			else if(key.p_y)
			{
				if(vid_menu_mode == DEF_SAPP0_MENU_NONE)
					vid_menu_mode = DEF_SAPP0_MENU_SETTINGS_0;
				else
					vid_menu_mode = DEF_SAPP0_MENU_NONE;
				
				var_need_reflesh = true;
			}
			else if(Util_hid_is_pressed(key, vid_control_button))
			{
				vid_control_button_selected = true;
				var_need_reflesh = true;
			}
			else if(Util_hid_is_released(key, vid_control_button) && vid_control_button_selected)
			{
				vid_show_controls = true;
				var_need_reflesh = true;
			}
			else if(key.p_touch && key.touch_x >= 5 && key.touch_x <= 314 && key.touch_y >= 210 && key.touch_y <= 219)
			{
				vid_seek_pos = vid_duration * (((double)key.touch_x - 5) / 310);
				vid_seek_request = true;
				var_need_reflesh = true;
			}
			
			if((key.p_c_down || key.p_c_up || key.p_c_right || key.p_c_left || key.h_c_down || key.h_c_up || key.h_c_right || key.h_c_left
			|| key.p_d_down || key.p_d_up || key.h_d_down || key.h_d_up || key.h_d_right || key.h_d_left) && !vid_disable_resize_move_mode)
			{
				if(key.p_c_down || key.p_d_down)
					vid_y -= 1 * var_scroll_speed * key.count;
				else if(key.h_c_down || key.h_d_down)
				{
					if(!vid_select_audio_track_request)
					{
						if(vid_cd_count > 600)
							vid_y -= 10 * var_scroll_speed * key.count;
						else if(vid_cd_count > 240)
							vid_y -= 7.5 * var_scroll_speed * key.count;
						else if(vid_cd_count > 5)
							vid_y -= 5 * var_scroll_speed * key.count;
					}
				}

				if(key.p_c_up || key.p_d_up)
					vid_y += 1 * var_scroll_speed * key.count;
				else if(key.h_c_up || key.h_d_up)
				{
					if(!vid_select_audio_track_request)
					{
						if(vid_cd_count > 600)
							vid_y += 10 * var_scroll_speed * key.count;
						else if(vid_cd_count > 240)
							vid_y += 7.5 * var_scroll_speed * key.count;
						else if(vid_cd_count > 5)
							vid_y += 5 * var_scroll_speed * key.count;
					}
				}

				if(key.p_c_right)
					vid_x -= 1 * var_scroll_speed * key.count;
				else if(key.h_c_right)
				{
					if(vid_cd_count > 600)
						vid_x -= 10 * var_scroll_speed * key.count;
					else if(vid_cd_count > 240)
						vid_x -= 7.5 * var_scroll_speed * key.count;
					else if(vid_cd_count > 5)
						vid_x -= 5 * var_scroll_speed * key.count;
				}

				if(key.p_c_left)
					vid_x += 1 * var_scroll_speed * key.count;
				else if(key.h_c_left)
				{
					if(vid_cd_count > 600)
						vid_x += 10 * var_scroll_speed * key.count;
					else if(vid_cd_count > 240)
						vid_x += 7.5 * var_scroll_speed * key.count;
					else if(vid_cd_count > 5)
						vid_x += 5 * var_scroll_speed * key.count;
				}

				if(vid_x > 400)
					vid_x = 400;
				else if(vid_x < -vid_codec_width * vid_zoom)
					vid_x = -vid_codec_width * vid_zoom;

				if(vid_y > 480)
					vid_y = 480;
				else if(vid_y < -vid_codec_height * vid_zoom)
					vid_y = -vid_codec_height * vid_zoom;

				vid_cd_count++;
				var_need_reflesh = true;
			}
			else
				vid_cd_count = 0;

			if((key.p_l || key.p_r || key.h_l || key.h_r) && !vid_disable_resize_move_mode)
			{
				if(key.p_l)
					vid_zoom -= 0.005 * var_scroll_speed * key.count;
				else if(key.h_l)
				{
					if(vid_lr_count > 360)
						vid_zoom -= 0.05 * var_scroll_speed * key.count;
					else if(vid_lr_count > 120)
						vid_zoom -= 0.01 * var_scroll_speed * key.count;
					else if(vid_lr_count > 5)
						vid_zoom -= 0.005 * var_scroll_speed * key.count;
				}

				if(key.p_r)
					vid_zoom += 0.005 * var_scroll_speed * key.count;
				else if(key.h_r)
				{
					if(vid_lr_count > 360)
						vid_zoom += 0.05 * var_scroll_speed * key.count;
					else if(vid_lr_count > 120)
						vid_zoom += 0.01 * var_scroll_speed * key.count;
					else if(vid_lr_count > 5)
						vid_zoom += 0.005 * var_scroll_speed * key.count;
				}

				if(vid_zoom < 0.05)
					vid_zoom = 0.05;
				else if(vid_zoom > 10)
					vid_zoom = 10;

				vid_lr_count++;
				var_need_reflesh = true;
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
			var_need_reflesh = true;
		}
		if(key.p_d_left)
		{
			if(vid_current_pos - (vid_seek_duration * 1000) < 0)
				vid_seek_pos = 0;
			else
				vid_seek_pos = vid_current_pos - (vid_seek_duration * 1000);
			
			vid_seek_request = true;
			var_need_reflesh = true;
		}

		if(!key.p_touch && !key.h_touch)
		{
			if(vid_select_audio_track_button_selected || vid_texture_filter_button_selected || vid_allow_skip_frames_button_selected
			|| vid_allow_skip_key_frames_button_selected || vid_volume_button_selected || vid_seek_duration_button_selected
			|| vid_use_hw_decoding_button_selected || vid_use_hw_color_conversion_button_selected || vid_use_multi_threaded_decoding_button_selected
			|| vid_lower_resolution_button_selected || vid_menu_button_selected[0] || vid_menu_button_selected[1] || vid_menu_button_selected[2]
			|| vid_control_button_selected || vid_ok_button_selected || vid_correct_aspect_ratio_button_selected 
			|| vid_disable_resize_move_button_selected || vid_remember_video_pos_button_selected)
				var_need_reflesh = true;

			vid_select_audio_track_button_selected = vid_texture_filter_button_selected = vid_allow_skip_frames_button_selected
			= vid_allow_skip_key_frames_button_selected = vid_volume_button_selected = vid_seek_duration_button_selected
			= vid_use_hw_decoding_button_selected = vid_use_hw_color_conversion_button_selected = vid_use_multi_threaded_decoding_button_selected
			= vid_lower_resolution_button_selected = vid_menu_button_selected[0] = vid_menu_button_selected[1] = vid_menu_button_selected[2] 
			= vid_control_button_selected = vid_ok_button_selected = vid_correct_aspect_ratio_button_selected 
			= vid_disable_resize_move_button_selected = vid_remember_video_pos_button_selected = false;

			for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
			{
				if(vid_audio_track_button_selected[i])
					var_need_reflesh = true;

				vid_audio_track_button_selected[i] = false;
			}
		}
	}
	
	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}
