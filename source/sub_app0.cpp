#include "headers.hpp"

#include "sub_app0.hpp"

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
bool vid_detail_mode = false;
bool vid_pause_request = false;
bool vid_seek_request = false;
bool vid_linear_filter = true;
bool vid_show_controls = false;
bool vid_allow_skip_frames = false;
bool vid_do_not_skip_request = false;
bool vid_full_screen_mode = false;
bool vid_image_enabled[8][2];
double vid_time[2][320];
double vid_copy_time[2] = { 0, 0, };
double vid_audio_time = 0;
double vid_video_time = 0;
double vid_convert_time = 0;
double vid_frametime = 0;
double vid_framerate = 0;
double vid_duration = 0;
double vid_zoom = 1;
double vid_x = 0;
double vid_y = 15;
double vid_3d_x_offset = 0;
double vid_current_pos = 0;
double vid_seek_pos = 0;
double vid_min_time = 0;
double vid_max_time = 0;
double vid_total_time = 0;
double vid_recent_time[90];
double vid_recent_total_time = 0;
int vid_turn_off_bottom_screen_count = 0;
int vid_num_of_audio_tracks = 0;
int vid_selected_audio_track = 0;
int vid_total_frames = 0;
int vid_width = 0;
int vid_height = 0;
int vid_codec_width = 0;
int vid_codec_height = 0;
int vid_tex_width[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };
int vid_tex_height[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };
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
Image_data vid_image[8][2];
C2D_Image vid_banner[2];
C2D_Image vid_control[2];
Thread vid_decode_thread, vid_decode_video_thread, vid_convert_thread;

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

	Result_with_string result;
	int audio_track = 0;
	int bitrate = 0;
	int sample_rate = 0;
	int ch = 0;
	int audio_size = 0;
	int packet_index = 0;
	int num_of_audio_tracks = 0;
	int num_of_video_tracks = 0;
	bool key_frame = false;
	double pos = 0;
	u8* audio = NULL;
	std::string format = "";
	std::string type = "";
	TickCounter counter;
	osTickCounterStart(&counter);

	while (vid_thread_run)
	{
		if(vid_play_request || vid_change_video_request)
		{
			packet_index = 0;
			vid_num_of_audio_tracks = 0;
			vid_selected_audio_track = 0;
			vid_x = 0;
			vid_y = 15;
			vid_3d_x_offset = 0;
			vid_frametime = 0;
			vid_framerate = 0;
			vid_current_pos = 0;
			vid_duration = 0;
			vid_zoom = 1;
			vid_width = 0;
			vid_height = 0;
			vid_codec_width = 0;
			vid_codec_height = 0;
			vid_video_format = "n/a";
			vid_audio_format = "n/a";
			vid_change_video_request = false;
			vid_play_request = true;
			vid_decode_video_request = false;
			vid_convert_request = false;
			vid_linear_filter = true;
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
			
			for(int i = 0; i < 8; i++)
			{
				vid_tex_width[i] = 0;
				vid_tex_height[i] = 0;
			}

			for(int i = 0 ; i < 320; i++)
			{
				vid_time[0][i] = 0;
				vid_time[1][i] = 0;
			}

			vid_audio_time = 0;
			vid_video_time = 0;
			vid_copy_time[0] = 0;
			vid_copy_time[1] = 0;
			vid_convert_time = 0;

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
					Util_speaker_init(0, ch, sample_rate);
				}
			}
			if(num_of_video_tracks && vid_play_request)
			{
				result = Util_video_decoder_init(0, num_of_video_tracks, 0);
				Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_video_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_play_request = false;
				
				if(vid_play_request)
				{
					Util_video_decoder_get_info(&vid_width, &vid_height, &vid_framerate, &vid_video_format, &vid_duration, 0, 0);
					vid_frametime = (1000.0 / vid_framerate);
					if(num_of_video_tracks == 2)
						vid_frametime /= 2;

					vid_codec_width = vid_width;
					vid_codec_height = vid_height;
					//fit to screen size
					while(((vid_width * vid_zoom) > 400 || (vid_height * vid_zoom) > 240) && vid_zoom > 0.05)
						vid_zoom -= 0.001;

					vid_x = (400 - (vid_width * vid_zoom)) / 2;
					vid_y = (240 - (vid_height * vid_zoom)) / 2;

					if(vid_codec_width % 16 != 0)
						vid_codec_width += 16 - vid_codec_width % 16;
					if(vid_codec_height % 16 != 0)
						vid_codec_height += 16 - vid_codec_height % 16;

					for(int k = 0; k < num_of_video_tracks; k++)
					{
						result = Draw_c2d_image_init(&vid_image[0][k], 1024, 1024, GPU_RGB565);
						if(result.code != 0)
							break;
						else
							vid_image_enabled[0][k] = true;

						result = Draw_c2d_image_init(&vid_image[4][k], 1024, 1024, GPU_RGB565);
						if(result.code != 0)
							break;
						else
							vid_image_enabled[4][k] = true;

						if(vid_codec_width > 1024)
						{
							result = Draw_c2d_image_init(&vid_image[1][k], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_image_enabled[1][k] = true;

							result = Draw_c2d_image_init(&vid_image[5][k], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_image_enabled[5][k] = true;
						}

						if(vid_codec_height > 1024)
						{
							result = Draw_c2d_image_init(&vid_image[2][k], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_image_enabled[2][k] = true;

							result = Draw_c2d_image_init(&vid_image[6][k], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_image_enabled[6][k] = true;
						}

						if(vid_codec_width > 1024 && vid_codec_height > 1024)
						{
							result = Draw_c2d_image_init(&vid_image[3][k], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_image_enabled[3][k] = true;

							result = Draw_c2d_image_init(&vid_image[7][k], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_image_enabled[7][k] = true;
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
						for(int i = 0; i < 2; i++)
						{
							if(vid_codec_width > 1024 && vid_codec_height > 1024)
							{
								vid_tex_width[i * 4] = 1024;
								vid_tex_width[i * 4 + 1] = vid_codec_width - 1024;
								vid_tex_width[i * 4 + 2] = 1024;
								vid_tex_width[i * 4 + 3] = vid_codec_width - 1024;
								vid_tex_height[i * 4] = 1024;
								vid_tex_height[i * 4 + 1] = 1024;
								vid_tex_height[i * 4 + 2] = vid_codec_height - 1024;
								vid_tex_height[i * 4 + 3] = vid_codec_height - 1024;
							}
							else if(vid_codec_width > 1024)
							{
								vid_tex_width[i * 4] = 1024;
								vid_tex_width[i * 4 + 1] = vid_codec_width - 1024;
								vid_tex_height[i * 4] = vid_codec_height;
								vid_tex_height[i * 4 + 1] = vid_codec_height;
							}
							else if(vid_codec_height > 1024)
							{
								vid_tex_width[i * 4] = vid_codec_width;
								vid_tex_width[i * 4 + 1] = vid_codec_width;
								vid_tex_height[i * 4] = 1024;
								vid_tex_height[i * 4 + 1] = vid_codec_height - 1024;
							}
							else
							{
								vid_tex_width[i * 4] = vid_codec_width;
								vid_tex_height[i * 4] = vid_codec_height;
							}
						}
						if(vid_codec_width <= 1024 && vid_codec_height <= 1024)
							result = Util_converter_y2r_init();
					}
				}
			}
			vid_full_screen_mode = true;
			vid_turn_off_bottom_screen_count = 300;

			if(result.code != 0)
			{
				Util_err_set_error_message(result.string, result.error_description, DEF_SAPP0_DECODE_THREAD_STR, result.code);
				Util_err_set_error_show_flag(true);
				var_need_reflesh = true;
			}

			while(vid_play_request)
			{
				while(vid_decode_video_request && vid_play_request)
					usleep(1000);

				if(!vid_play_request)
					break;

				result = Util_decoder_read_packet(&type, &packet_index, &key_frame, 0);
				if(result.code != 0)
					Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_decoder_read_packet()..." + result.string + result.error_description, result.code);

				var_afk_time = 0;

				if(!vid_select_audio_track_request && audio_track != vid_selected_audio_track)
				{
					audio_track = vid_selected_audio_track;
					Util_speaker_clear_buffer(0);
					Util_audio_decoder_get_info(&bitrate, &sample_rate, &ch, &vid_audio_format, &vid_duration, audio_track, &vid_audio_track_lang[audio_track], 0);
					Util_speaker_init(0, ch, sample_rate);
				}

				if(vid_pause_request)
				{
					Util_speaker_pause(0);
					while(vid_pause_request && vid_play_request && !vid_seek_request && !vid_change_video_request)
						usleep(20000);
					
					Util_speaker_resume(0);
				}

				if(vid_seek_request)
				{
					//µs
					result = Util_decoder_seek(vid_seek_pos * 1000, 8, 0);
					Util_log_save(DEF_SAPP0_DECODE_THREAD_STR, "Util_decoder_seek()..." + result.string + result.error_description, result.code);
					Util_speaker_clear_buffer(0);
					vid_seek_request = false;
				}

				if(!vid_play_request || vid_change_video_request || result.code != 0)
					break;

				if(type == "audio" && packet_index == audio_track)
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
						
						if(result.code == 0)
						{
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
					vid_do_not_skip_request = key_frame;
					vid_decode_video_request = true;
				}
			}

			vid_decode_video_request = false;
			vid_convert_request = false;
			while(vid_convert_wait_request || vid_decode_video_wait_request)
				usleep(10000);

			if(num_of_audio_tracks > 0)
			{
				Util_audio_decoder_exit(0);
				while(Util_speaker_is_playing(0) && vid_play_request)
					usleep(10000);
				
				Util_speaker_exit(0);
			}
			if(num_of_video_tracks > 0)
				Util_video_decoder_exit(0);

			Util_decoder_close_file(0);
			if(vid_codec_width <= 1024 && vid_codec_height <= 1024)
				Util_converter_y2r_exit();
			
			for(int k = 0; k < 2; k++)
			{
				for(int i = 0; i < 8; i++)
				{
					if(vid_image_enabled[i][k])
					{
						Draw_c2d_image_free(vid_image[i][k]);
						vid_image_enabled[i][k] = false;
					}
				}
			}

			var_top_lcd_brightness = var_lcd_brightness;
			var_bottom_lcd_brightness = var_lcd_brightness;
			vid_turn_off_bottom_screen_count = 0;
			var_turn_on_bottom_lcd = true;
			vid_full_screen_mode = false;
			vid_pause_request = false;
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
					if(vid_allow_skip_frames && skip > vid_frametime && !vid_do_not_skip_request)
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
							result = Util_video_decoder_decode(&w, &h, &pos, packet_index, 0);
							osTickCounterUpdate(&counter);
							vid_video_time = osTickCounterRead(&counter);

							if(!std::isnan(pos) && !std::isinf(pos))
								vid_current_pos = pos;

							if(result.code == 0)
							{
								vid_packet_index[1] = packet_index;
								vid_convert_request = true;
							}
							else
								Util_log_save(DEF_SAPP0_DECODE_VIDEO_THREAD_STR, "Util_video_decoder_decode()..." + result.string + result.error_description, result.code);

							vid_decode_video_wait_request = false;

							vid_time[0][319] = vid_video_time;

							if(vid_frametime - vid_video_time > 0)
								usleep((vid_frametime - vid_video_time) * 1000);
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
								vid_time[0][i - 1] = vid_time[0][i];
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
	std::string type = (char*)arg;
	if(type == "1")
		APT_SetAppCpuTimeLimit(80);
	else
		APT_SetAppCpuTimeLimit(5);

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
				result = Util_video_decoder_get_image(&yuv_video, vid_codec_width, vid_codec_height, packet_index, 0);
				osTickCounterUpdate(&counter[2]);
				vid_copy_time[0] = osTickCounterRead(&counter[2]);

				vid_convert_request = false;
				if(result.code == 0)
				{
					osTickCounterUpdate(&counter[0]);
					if(vid_codec_width <= 1024 && vid_codec_height <= 1024)
						result = Util_converter_y2r_yuv420p_to_bgr565(yuv_video, &video, vid_codec_width, vid_codec_height);
					else
						result = Util_converter_yuv420p_to_bgr565(yuv_video, &video, vid_codec_width, vid_codec_height);
					osTickCounterUpdate(&counter[0]);
					vid_convert_time = osTickCounterRead(&counter[0]);

					if(result.code == 0)
					{
						osTickCounterUpdate(&counter[1]);
						if(packet_index == 0)
							image_num = vid_image_num;
						else if(packet_index == 1)
							image_num = vid_image_num_3d;

						result = Draw_set_texture_data(&vid_image[image_num * 4][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, GPU_RGB565);
						if(result.code != 0)
							Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);

						if(vid_codec_width > 1024)
						{
							result = Draw_set_texture_data(&vid_image[image_num * 4 + 1][packet_index], video, vid_codec_width, vid_codec_height, 1024, 0, 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
						}
						if(vid_codec_height > 1024)
						{
							result = Draw_set_texture_data(&vid_image[image_num * 4 + 2][packet_index], video, vid_codec_width, vid_codec_height, 0, 1024, 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
						}
						if(vid_codec_width > 1024 && vid_codec_height > 1024)
						{
							result = Draw_set_texture_data(&vid_image[image_num * 4 + 3][packet_index], video, vid_codec_width, vid_codec_height, 1024, 1024, 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
						}

						if(packet_index == 0)
						{
							if(image_num == 0)
								vid_image_num = 1;
							else
								vid_image_num = 0;
						}
						else if(packet_index == 1)
						{
							if(image_num == 0)
								vid_image_num_3d = 1;
							else
								vid_image_num_3d = 0;
						}

						osTickCounterUpdate(&counter[1]);
						vid_copy_time[1] = osTickCounterRead(&counter[1]);
						
						if(packet_index == 0)
							var_need_reflesh = true;
					}
					else
						Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Util_converter_yuv420p_to_bgr565()..." + result.string + result.error_description, result.code);
				}
				else
					Util_log_save(DEF_SAPP0_CONVERT_THREAD_STR, "Util_video_decoder_get_image()..." + result.string + result.error_description, result.code);

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

	APT_SetAppCpuTimeLimit(5);

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

void Sapp0_init(void)
{
	Util_log_save(DEF_SAPP0_INIT_STR, "Initializing...");
	bool new_3ds = false;
	Result_with_string result;
	
	APT_CheckNew3DS(&new_3ds);
	vid_thread_run = true;
	if(new_3ds)
	{
		vid_decode_thread = threadCreate(Sapp0_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 0, false);
		vid_decode_video_thread = threadCreate(Sapp0_decode_video_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 2, false);
		vid_convert_thread = threadCreate(Sapp0_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
	}
	else
	{
		vid_decode_thread = threadCreate(Sapp0_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 0, false);
		vid_decode_video_thread = threadCreate(Sapp0_decode_video_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 1, false);
		vid_convert_thread = threadCreate(Sapp0_convert_thread, (void*)("1"), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
	}

	vid_full_screen_mode = false;
	vid_detail_mode = false;
	vid_show_controls = false;
	vid_allow_skip_frames = false;
	vid_num_of_audio_tracks = 0;
	vid_selected_audio_track = 0;
	vid_lr_count = 0;
	vid_cd_count = 0;
	vid_x = 0;
	vid_y = 15;
	vid_3d_x_offset = 0;
	vid_frametime = 0;
	vid_framerate = 0;
	vid_current_pos = 0;
	vid_duration = 0;
	vid_zoom = 1;
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

	for(int i = 0; i < 8; i++)
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
		for(int i = 0; i < 8; i++)
			vid_image_enabled[i][k] = false;
	}

	vid_banner_texture_num = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/banner.t3x", vid_banner_texture_num, vid_banner, 0, 2);
	Util_log_save(DEF_SAPP0_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	vid_control_texture_num = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/control.t3x", vid_control_texture_num, vid_control, 0, 2);
	Util_log_save(DEF_SAPP0_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	result = Util_load_msg("sapp0_" + var_lang + ".txt", vid_msg, DEF_SAPP0_NUM_OF_MSG);
	Util_log_save(DEF_SAPP0_INIT_STR, "Util_load_msg()..." + result.string + result.error_description, result.code);

	Sapp0_resume();
	vid_already_init = true;
	Util_log_save(DEF_SAPP0_INIT_STR, "Initialized.");
}

void Sapp0_exit(void)
{
	Util_log_save(DEF_SAPP0_EXIT_STR, "Exiting...");
	u64 time_out = 10000000000;
	Result_with_string result;

	vid_already_init = false;
	vid_thread_suspend = false;
	vid_thread_run = false;
	vid_convert_request = false;
	vid_decode_video_request = false;
	vid_play_request = false;
	vid_select_audio_track_request = false;

	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(vid_decode_thread, time_out));
	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(vid_decode_video_thread, time_out));
	Util_log_save(DEF_SAPP0_EXIT_STR, "threadJoin()...", threadJoin(vid_convert_thread, time_out));
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
	int back_color = DEF_DRAW_WHITE;
	int image_num = 0;
	int image_num_3d = 0;
	Hid_info key;
	Util_hid_query_key_state(&key);
	Util_hid_key_flag_reset();

	if (var_night_mode)
	{
		color = DEF_DRAW_WHITE;
		back_color = DEF_DRAW_BLACK;
	}
	if(vid_image_num == 0)
		image_num = 1;

	if(vid_image_num_3d == 0)
		image_num_3d = 1;

	if(vid_turn_off_bottom_screen_count > 0)
	{
		vid_turn_off_bottom_screen_count--;
		if(vid_turn_off_bottom_screen_count == 0)
			var_turn_on_bottom_lcd = false;
		if(var_bottom_lcd_brightness > 10)
			var_bottom_lcd_brightness--;
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
				Draw_texture(vid_image[image_num * 4][0].c2d, vid_x, vid_y, vid_tex_width[image_num * 4] * vid_zoom, vid_tex_height[image_num * 4] * vid_zoom);
				if(vid_codec_width > 1024)
					Draw_texture(vid_image[image_num * 4 + 1][0].c2d, (vid_x + vid_tex_width[image_num * 4] * vid_zoom), vid_y, vid_tex_width[image_num * 4 + 1] * vid_zoom, vid_tex_height[image_num * 4 + 1] * vid_zoom);
				if(vid_codec_height > 1024)
					Draw_texture(vid_image[image_num * 4 + 2][0].c2d, vid_x, (vid_y + vid_tex_width[image_num * 4] * vid_zoom), vid_tex_width[image_num * 4 + 2] * vid_zoom, vid_tex_height[image_num * 4 + 2] * vid_zoom);
				if(vid_codec_width > 1024 && vid_codec_height > 1024)
					Draw_texture(vid_image[image_num * 4 + 3][0].c2d, (vid_x + vid_tex_width[image_num * 4] * vid_zoom), (vid_y + vid_tex_height[image_num * 4] * vid_zoom), vid_tex_width[image_num * 4 + 3] * vid_zoom, vid_tex_height[image_num * 4 + 3] * vid_zoom);
			}
			else
				Draw_texture(vid_banner[var_night_mode], 0, 15, 400, 225);
			
			if(vid_full_screen_mode && vid_turn_off_bottom_screen_count > 0)
			{
				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_BLACK, 40, 20, 320, 20);
				Draw(vid_msg[14], 42.5, 20, 0.5, 0.5, DEF_DRAW_WHITE);
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
					//video
					Draw_texture(vid_image[image_num_3d * 4][1].c2d, vid_x + vid_3d_x_offset, vid_y, vid_tex_width[image_num_3d * 4] * vid_zoom, vid_tex_height[image_num_3d * 4] * vid_zoom);
					if(vid_codec_width > 1024)
						Draw_texture(vid_image[image_num_3d * 4 + 1][1].c2d, (vid_x  + vid_3d_x_offset + vid_tex_width[image_num_3d * 4] * vid_zoom), vid_y, vid_tex_width[image_num_3d * 4 + 1] * vid_zoom, vid_tex_height[image_num_3d * 4 + 1] * vid_zoom);
					if(vid_codec_height > 1024)
						Draw_texture(vid_image[image_num_3d * 4 + 2][1].c2d, vid_x + vid_3d_x_offset, (vid_y + vid_tex_width[image_num_3d * 4] * vid_zoom), vid_tex_width[image_num_3d * 4 + 2] * vid_zoom, vid_tex_height[image_num_3d * 4 + 2] * vid_zoom);
					if(vid_codec_width > 1024 && vid_codec_height > 1024)
						Draw_texture(vid_image[image_num_3d * 4 + 3][1].c2d, (vid_x + vid_3d_x_offset + vid_tex_width[image_num_3d * 4] * vid_zoom), (vid_y + vid_tex_height[image_num_3d * 4] * vid_zoom), vid_tex_width[image_num_3d * 4 + 3] * vid_zoom, vid_tex_height[image_num_3d * 4 + 3] * vid_zoom);
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
			Draw(std::to_string(vid_width) + "(" + std::to_string(vid_codec_width) + ")" + "x" + std::to_string(vid_height) + "(" + std::to_string(vid_codec_height) + ")" + "@" + std::to_string(vid_framerate).substr(0, 5) + "fps", 0, 30, 0.5, 0.5, color);

			if(vid_play_request)
			{
				//video
				Draw_texture(vid_image[image_num * 4][0].c2d, vid_x - 40, vid_y - 240, vid_tex_width[image_num * 4] * vid_zoom, vid_tex_height[image_num * 4] * vid_zoom);
				if(vid_codec_width > 1024)
					Draw_texture(vid_image[image_num * 4 + 1][0].c2d, (vid_x + vid_tex_width[image_num * 4] * vid_zoom) - 40, vid_y - 240, vid_tex_width[image_num * 4 + 1] * vid_zoom, vid_tex_height[image_num * 4 + 1] * vid_zoom);
				if(vid_codec_height > 1024)
					Draw_texture(vid_image[image_num * 4 + 2][0].c2d, vid_x - 40, (vid_y + vid_tex_width[image_num * 4] * vid_zoom) - 240, vid_tex_width[image_num * 4 + 2] * vid_zoom, vid_tex_height[image_num * 4 + 2] * vid_zoom);
				if(vid_codec_width > 1024 && vid_codec_height > 1024)
					Draw_texture(vid_image[image_num * 4 + 3][0].c2d, (vid_x + vid_tex_width[image_num * 4] * vid_zoom) - 40, (vid_y + vid_tex_height[image_num * 4] * vid_zoom) - 240, vid_tex_width[image_num * 4 + 3] * vid_zoom, vid_tex_height[image_num * 4 + 3] * vid_zoom);
			}

			//select audio track
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 165, 145, 10);
			Draw(vid_msg[13], 12.5, 165, 0.4, 0.4, color);

			//controls
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 165, 165, 145, 10);
			Draw(vid_msg[2], 167.5, 165, 0.4, 0.4, color);

			//texture filter
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 180, 145, 10);
			Draw(vid_msg[vid_linear_filter], 12.5, 180, 0.4, 0.4, color);

			//allow skip frames
			Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 165, 180, 145, 10);
			Draw(vid_msg[3 + vid_allow_skip_frames], 167.5, 180, 0.4, 0.4, color);

			//time bar
			Draw(Util_convert_seconds_to_time(vid_current_pos / 1000) + "/" + Util_convert_seconds_to_time(vid_duration), 110, 210, 0.5, 0.5, color);
			Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 5, 195, 310, 10);
			if(vid_duration != 0)
				Draw_texture(var_square_image[0], 0xFF800080, 5, 195, 310 * ((vid_current_pos / 1000) / vid_duration), 10);

			if(vid_detail_mode)
			{
				//decoding detail
				for(int i = 0; i < 319; i++)
				{
					Draw_line(i, 110 - vid_time[1][i], DEF_DRAW_BLUE, i + 1, 110 - vid_time[1][i + 1], DEF_DRAW_BLUE, 1);//Thread 1
					Draw_line(i, 110 - vid_time[0][i], DEF_DRAW_RED, i + 1, 110 - vid_time[0][i + 1], DEF_DRAW_RED, 1);//Thread 0
				}

				Draw_line(0, 110, color, 320, 110, color, 2);
				Draw_line(0, 110 - vid_frametime, 0xFFFFFF00, 320, 110 - vid_frametime, 0xFFFFFF00, 2);
				if(vid_total_frames != 0 && vid_min_time != 0  && vid_recent_total_time != 0)
				{
					Draw("avg " + std::to_string(1000 / (vid_total_time / vid_total_frames)).substr(0, 5) + " min " + std::to_string(1000 / vid_max_time).substr(0, 5) 
					+  " max " + std::to_string(1000 / vid_min_time).substr(0, 5) + " recent avg " + std::to_string(1000 / (vid_recent_total_time / 90)).substr(0, 5) +  " fps", 0, 110, 0.4, 0.4, color);
				}

				Draw("Deadline : " + std::to_string(vid_frametime).substr(0, 5) + "ms", 0, 120, 0.4, 0.4, 0xFFFFFF00);
				Draw("Video decode : " + std::to_string(vid_video_time).substr(0, 5) + "ms", 0, 130, 0.4, 0.4, DEF_DRAW_RED);
				Draw("Audio decode : " + std::to_string(vid_audio_time).substr(0, 5) + "ms", 0, 140, 0.4, 0.4, DEF_DRAW_RED);
				Draw("Data copy 0 : " + std::to_string(vid_copy_time[0]).substr(0, 5) + "ms", 160, 120, 0.4, 0.4, DEF_DRAW_BLUE);
				Draw("Color convert : " + std::to_string(vid_convert_time).substr(0, 5) + "ms", 160, 130, 0.4, 0.4, DEF_DRAW_BLUE);
				Draw("Data copy 1 : " + std::to_string(vid_copy_time[1]).substr(0, 5) + "ms", 160, 140, 0.4, 0.4, DEF_DRAW_BLUE);
				Draw("Thread 0 : " + std::to_string(vid_time[0][319]).substr(0, 6) + "ms", 0, 150, 0.5, 0.5, DEF_DRAW_RED);
				Draw("Thread 1 : " + std::to_string(vid_time[1][319]).substr(0, 6) + "ms", 160, 150, 0.5, 0.5, DEF_DRAW_BLUE);
			}

			if(vid_show_controls)
			{
				Draw_texture(vid_control[var_night_mode], 80, 20, 160, 160);
				Draw(vid_msg[5], 122.5, 47.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[6], 122.5, 62.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[7], 122.5, 77.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[8], 122.5, 92.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[9], 135, 107.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[10], 122.5, 122.5, 0.45, 0.45, DEF_DRAW_BLACK);
				Draw(vid_msg[11], 132.5, 137.5, 0.45, 0.45, DEF_DRAW_BLACK);
			}

			if(vid_select_audio_track_request)
			{
				Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 40, 20, 240, 140);
				Draw(vid_msg[12], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

				for(int i = 0; i < vid_num_of_audio_tracks; i++)
					Draw("Track " + std::to_string(i) + "(" + vid_audio_track_lang[i] + ")", 42.5, 40 + (i * 10), 0.5, 0.5, i == vid_selected_audio_track ? DEF_DRAW_RED : color);

				Draw_texture(var_square_image[0], DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
				Draw("OK", 152.5, 140, 0.4, 0.4, DEF_DRAW_BLACK);
			}

			if(Util_expl_query_show_flag())
				Util_expl_draw();

			if(Util_err_query_error_show_flag())
				Util_err_draw();

			Draw_bot_ui();
			Draw_touch_pos();
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
		if (key.p_start || (key.p_touch && key.touch_x >= 110 && key.touch_x <= 230 && key.touch_y >= 220 && key.touch_y <= 240))
			Sapp0_suspend();
		else if(key.p_select)
		{
			if(!vid_full_screen_mode)
			{
				//fit to screen size
				vid_zoom = 1;
				while(((vid_width * vid_zoom) > 400 || (vid_height * vid_zoom) > 240) && vid_zoom > 0.05)
					vid_zoom -= 0.001;

				vid_x = (400 - (vid_width * vid_zoom)) / 2;
				vid_y = (240 - (vid_height * vid_zoom)) / 2;
				vid_turn_off_bottom_screen_count = 300;
				vid_full_screen_mode = true;
				var_top_lcd_brightness = var_lcd_brightness;
				var_bottom_lcd_brightness = var_lcd_brightness;
				key.p_any = false;
				var_need_reflesh = true;
			}
		}
		else if(key.p_a)
		{
			if(vid_select_audio_track_request)
				vid_select_audio_track_request = false;
			else if(vid_play_request)
				vid_pause_request = !vid_pause_request;
			else
				vid_play_request = true;

			var_need_reflesh = true;
		}
		else if(key.p_b)
		{
			if(!vid_select_audio_track_request)
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
			vid_detail_mode = !vid_detail_mode;
			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 40 && key.touch_x <= 279 && key.touch_y >= 40 && key.touch_y <= 109)
		{
			if(vid_select_audio_track_request)
			{
				for(int i = 0; i < vid_num_of_audio_tracks; i++)
				{
					if(key.touch_y >= 40 + (i * 10) && key.touch_y <= 49 + (i * 10))
					{
						vid_selected_audio_track = i;
						var_need_reflesh = true;
						break;
					}
				}
			}
		}
		else if(key.p_touch && key.touch_x >= 150 && key.touch_x <= 169 && key.touch_y >= 140 && key.touch_y <= 149)
		{
			if(vid_select_audio_track_request)
				vid_select_audio_track_request = false;

			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 10 && key.touch_x <= 154 && key.touch_y >= 165 && key.touch_y <= 174)
		{
			vid_select_audio_track_request = !vid_select_audio_track_request;
			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 165 && key.touch_x <= 309 && key.touch_y >= 165 && key.touch_y <= 174)
		{
			vid_show_controls = !vid_show_controls;
			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 10 && key.touch_x <= 154 && key.touch_y >= 180 && key.touch_y <= 189)
		{
			vid_linear_filter = !vid_linear_filter;
			for(int k = 0; k < 2; k++)
			{
				for(int i = 0; i < 8; i++)
				{
					if(vid_image_enabled[i][k])
						Draw_c2d_image_set_filter(&vid_image[i][k], vid_linear_filter);
				}
			}

			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 165 && key.touch_x <= 309 && key.touch_y >= 180 && key.touch_y <= 189)
		{
			vid_allow_skip_frames = !vid_allow_skip_frames;
			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 5 && key.touch_x <= 314 && key.touch_y >= 195 && key.touch_y <= 204)
		{
			vid_seek_pos = (vid_duration * 1000) * (((double)key.touch_x - 5) / 310);
			vid_seek_request = true;
			var_need_reflesh = true;
		}
		else if(key.h_touch || key.p_touch)
			var_need_reflesh = true;
		
		if(key.p_c_down || key.p_c_up || key.p_c_right || key.p_c_left || key.h_c_down || key.h_c_up || key.h_c_right || key.h_c_left
		|| key.p_d_down || key.p_d_up || key.p_d_right || key.p_d_left || key.h_d_down || key.h_d_up || key.h_d_right || key.h_d_left)
		{
			if(key.p_c_down || key.p_d_down)
			{
				if(vid_select_audio_track_request && vid_selected_audio_track + 1 < vid_num_of_audio_tracks)
					vid_selected_audio_track++;
				else if(!vid_select_audio_track_request)
					vid_y -= 1 * var_scroll_speed * key.count;
			}
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
			{
				if(vid_select_audio_track_request && vid_selected_audio_track - 1 > -1)
					vid_selected_audio_track--;
				else if(!vid_select_audio_track_request)
					vid_y += 1 * var_scroll_speed * key.count;
			}
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

			if(key.p_d_right)
				vid_3d_x_offset -= 1 * var_scroll_speed * key.count;
			else if(key.h_d_right)
				vid_3d_x_offset -= 2.5 * var_scroll_speed * key.count;

			if(key.p_d_left)
				vid_3d_x_offset += 1 * var_scroll_speed * key.count;
			else if(key.h_d_left)
				vid_3d_x_offset += 2.5 * var_scroll_speed * key.count;

			if(vid_x > 400)
				vid_x = 400;
			else if(vid_x < -vid_codec_width * vid_zoom)
				vid_x = -vid_codec_width * vid_zoom;

			if(vid_y > 480)
				vid_y = 480;
			else if(vid_y < -vid_codec_height * vid_zoom)
				vid_y = -vid_codec_height * vid_zoom;

			if(vid_3d_x_offset > 400)
				vid_3d_x_offset = 400;
			else if(vid_3d_x_offset < -400)
				vid_3d_x_offset = -400;

			vid_cd_count++;
			var_need_reflesh = true;
		}
		else
			vid_cd_count = 0;

		if(key.p_l || key.p_r || key.h_l || key.h_r)
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

	if(key.p_any)
	{
		if(vid_full_screen_mode)
		{
			//fit to screen size
			vid_zoom = 1;
			while(((vid_width * vid_zoom) > 400 || (vid_height * vid_zoom) > 225) && vid_zoom > 0.05)
				vid_zoom -= 0.001;

			vid_x = (400 - (vid_width * vid_zoom)) / 2;
			vid_y = (225 - (vid_height * vid_zoom)) / 2;
			vid_y += 15;
			vid_turn_off_bottom_screen_count = 0;
			vid_full_screen_mode = false;
			var_turn_on_bottom_lcd = true;
			var_top_lcd_brightness = var_lcd_brightness;
			var_bottom_lcd_brightness = var_lcd_brightness;
			var_need_reflesh = true;
		}
	}
	
	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}
