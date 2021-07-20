#include "headers.hpp"

#include "sub_app1.hpp"

bool vid_mvd_main_run = false;
bool vid_mvd_thread_run = false;
bool vid_mvd_already_init = false;
bool vid_mvd_thread_suspend = true;
bool vid_mvd_play_request = false;
bool vid_mvd_change_video_request = false;
bool vid_mvd_convert_request = false;
bool vid_mvd_convert_wait_request = false;
bool vid_mvd_decode_video_request = false;
bool vid_mvd_decode_video_wait_request = false;
bool vid_mvd_detail_mode = false;
bool vid_mvd_pause_request = false;
bool vid_mvd_seek_request = false;
bool vid_mvd_linear_filter = true;
bool vid_mvd_show_controls = false;
bool vid_mvd_allow_skip_frames = false;
bool vid_mvd_image_enabled[8];
double vid_mvd_time[2][320];
double vid_mvd_copy_time[2] = { 0, 0, };
double vid_mvd_audio_time = 0;
double vid_mvd_video_time = 0;
double vid_mvd_frametime = 0;
double vid_mvd_framerate = 0;
double vid_mvd_duration = 0;
double vid_mvd_zoom = 1;
double vid_mvd_x = 0;
double vid_mvd_y = 15;
double vid_mvd_current_pos = 0;
double vid_mvd_seek_pos = 0;
double vid_mvd_min_time = 0;
double vid_mvd_max_time = 0;
double vid_mvd_total_time = 0;
double vid_mvd_recent_time[90];
double vid_mvd_recent_total_time = 0;
int vid_mvd_packet_index = 0;
int vid_mvd_total_frames = 0;
int vid_mvd_width = 0;
int vid_mvd_height = 0;
int vid_mvd_tex_width[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };
int vid_mvd_tex_height[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };
int vid_mvd_lr_count = 0;
int vid_mvd_cd_count = 0;
int vid_mvd_image_num = 0;
std::string vid_mvd_file = "";
std::string vid_mvd_dir = "";
std::string vid_mvd_video_format = "n/a";
std::string vid_mvd_audio_format = "n/a";
std::string vid_mvd_msg[DEF_SAPP1_NUM_OF_MSG];
Image_data vid_mvd_image[8];
C2D_Image vid_mvd_banner[2];
C2D_Image vid_mvd_control[2];
Thread vid_mvd_decode_thread, vid_mvd_decode_video_thread, vid_mvd_convert_thread;

void Sapp1_callback(std::string file, std::string dir)
{
	vid_mvd_file = file;
	vid_mvd_dir = dir;
	vid_mvd_change_video_request = true;
}

void Sapp1_cancel(void)
{

}

bool Sapp1_query_init_flag(void)
{
	return vid_mvd_already_init;
}

bool Sapp1_query_running_flag(void)
{
	return vid_mvd_main_run;
}

void Sapp1_decode_thread(void* arg)
{
	Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Thread started.");

	Result_with_string result;
	int bitrate = 0;
	int sample_rate = 0;
	int ch = 0;
	int audio_size = 0;
	int packet_index = 0;
	double pos = 0;
	bool has_audio = false;
	bool has_video = false;
	bool has_3d = false;
	u8* audio = NULL;
	std::string format = "";
	std::string type = "";
	TickCounter counter;
	osTickCounterStart(&counter);

	while (vid_mvd_thread_run)
	{
		if(vid_mvd_play_request || vid_mvd_change_video_request)
		{
			packet_index = 0;
			vid_mvd_x = 0;
			vid_mvd_y = 15;
			vid_mvd_frametime = 0;
			vid_mvd_framerate = 0;
			vid_mvd_current_pos = 0;
			vid_mvd_duration = 0;
			vid_mvd_zoom = 1;
			vid_mvd_width = 0;
			vid_mvd_height = 0;
			vid_mvd_image_num = 0;
			vid_mvd_video_format = "n/a";
			vid_mvd_audio_format = "n/a";
			vid_mvd_change_video_request = false;
			vid_mvd_play_request = true;
			vid_mvd_decode_video_request = false;
			vid_mvd_convert_request = false;
			vid_mvd_linear_filter = true;
			vid_mvd_total_time = 0;
			vid_mvd_total_frames = 0;
			vid_mvd_min_time = 99999999;
			vid_mvd_max_time = 0;
			vid_mvd_recent_total_time = 0;
			for(int i = 0; i < 90; i++)
				vid_mvd_recent_time[i] = 0;

			for(int i = 0; i < 8; i++)
			{
				vid_mvd_tex_width[i] = 0;
				vid_mvd_tex_height[i] = 0;
			}

			for(int i = 0 ; i < 320; i++)
			{
				vid_mvd_time[0][i] = 0;
				vid_mvd_time[1][i] = 0;
			}

			vid_mvd_audio_time = 0;
			vid_mvd_video_time = 0;
			vid_mvd_copy_time[0] = 0;
			vid_mvd_copy_time[1] = 0;

			result = Util_decoder_open_file(vid_mvd_dir + vid_mvd_file, &has_audio, &has_video, &has_3d, 1);
			Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_decoder_open_file()..." + result.string + result.error_description, result.code);
			if(result.code != 0)
				vid_mvd_play_request = false;

			if(has_audio && vid_mvd_play_request)
			{
				result = Util_audio_decoder_init(1);
				Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_audio_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_mvd_play_request = false;

				if(vid_mvd_play_request)
				{
					Util_audio_decoder_get_info(&bitrate, &sample_rate, &ch, &vid_mvd_audio_format, &vid_mvd_duration, 1);
					Util_speaker_init(1, ch, sample_rate);
				}
			}
			if(has_video && vid_mvd_play_request)
			{
				result = Util_video_decoder_init(0, has_3d, 1);
				Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_video_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_mvd_play_request = false;

				result = Util_mvd_video_decoder_init(1);
				Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_mvd_video_decoder_init()..." + result.string + result.error_description, result.code);
				if(result.code != 0)
					vid_mvd_play_request = false;
				
				if(vid_mvd_play_request)
				{
					Util_video_decoder_get_info(&vid_mvd_width, &vid_mvd_height, &vid_mvd_framerate, &vid_mvd_video_format, &vid_mvd_duration, 1);
					vid_mvd_frametime = (1000.0 / vid_mvd_framerate);

					//fit to screen size
					while(((vid_mvd_width * vid_mvd_zoom) > 400 || (vid_mvd_height * vid_mvd_zoom) > 225) && vid_mvd_zoom > 0.05)
						vid_mvd_zoom -= 0.001;

					vid_mvd_x = (400 - (vid_mvd_width * vid_mvd_zoom)) / 2;
					vid_mvd_y = (225 - (vid_mvd_height * vid_mvd_zoom)) / 2;
					vid_mvd_y += 15;

					if(vid_mvd_width % 16 != 0)
						vid_mvd_width += 16 - vid_mvd_width % 16;
					if(vid_mvd_height % 16 != 0)
						vid_mvd_height += 16 - vid_mvd_height % 16;
					
					if(vid_mvd_video_format != "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10")
					{
						result.code = DEF_ERR_OTHER;
						result.string = "[Error] H264 only";
						vid_mvd_play_request = false;
					}
					else
					{
						for(int i = 0; i < 1; i++)
						{
							result = Draw_c2d_image_init(&vid_mvd_image[0], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_mvd_image_enabled[0] = true;

							result = Draw_c2d_image_init(&vid_mvd_image[4], 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								break;
							else
								vid_mvd_image_enabled[4] = true;

							if(vid_mvd_width > 1024)
							{
								result = Draw_c2d_image_init(&vid_mvd_image[1], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									break;
								else
									vid_mvd_image_enabled[1] = true;

								result = Draw_c2d_image_init(&vid_mvd_image[5], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									break;
								else
									vid_mvd_image_enabled[5] = true;
							}

							if(vid_mvd_height > 1024)
							{
								result = Draw_c2d_image_init(&vid_mvd_image[2], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									break;
								else
									vid_mvd_image_enabled[2] = true;

								result = Draw_c2d_image_init(&vid_mvd_image[6], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									break;
								else
									vid_mvd_image_enabled[6] = true;
							}

							if(vid_mvd_width > 1024 && vid_mvd_height > 1024)
							{
								result = Draw_c2d_image_init(&vid_mvd_image[3], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									break;
								else
									vid_mvd_image_enabled[3] = true;

								result = Draw_c2d_image_init(&vid_mvd_image[7], 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									break;
								else
									vid_mvd_image_enabled[7] = true;
							}
						}
					}

					if(result.code != 0 && vid_mvd_play_request)
					{
						result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
						result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
						vid_mvd_play_request = false;
					}
					else
					{
						for(int i = 0; i < 2; i++)
						{
							if(vid_mvd_width > 1024 && vid_mvd_height > 1024)
							{
								vid_mvd_tex_width[i * 4] = 1024;
								vid_mvd_tex_width[i * 4 + 1] = vid_mvd_width - 1024;
								vid_mvd_tex_width[i * 4 + 2] = 1024;
								vid_mvd_tex_width[i * 4 + 3] = vid_mvd_width - 1024;
								vid_mvd_tex_height[i * 4] = 1024;
								vid_mvd_tex_height[i * 4 + 1] = 1024;
								vid_mvd_tex_height[i * 4 + 2] = vid_mvd_height - 1024;
								vid_mvd_tex_height[i * 4 + 3] = vid_mvd_height - 1024;
							}
							else if(vid_mvd_width > 1024)
							{
								vid_mvd_tex_width[i * 4] = 1024;
								vid_mvd_tex_width[i * 4 + 1] = vid_mvd_width - 1024;
								vid_mvd_tex_height[i * 4] = vid_mvd_height;
								vid_mvd_tex_height[i * 4 + 1] = vid_mvd_height;
							}
							else if(vid_mvd_height > 1024)
							{
								vid_mvd_tex_width[i * 4] = vid_mvd_width;
								vid_mvd_tex_width[i * 4 + 1] = vid_mvd_width;
								vid_mvd_tex_height[i * 4] = 1024;
								vid_mvd_tex_height[i * 4 + 1] = vid_mvd_height - 1024;
							}
							else
							{
								vid_mvd_tex_width[i * 4] = vid_mvd_width;
								vid_mvd_tex_height[i * 4] = vid_mvd_height;
							}
						}
					}
				}
			}

			if(result.code != 0)
			{
				Util_err_set_error_message(result.string, result.error_description, DEF_SAPP1_DECODE_THREAD_STR, result.code);
				Util_err_set_error_show_flag(true);
				var_need_reflesh = true;
			}

			while(vid_mvd_play_request)
			{
				while(vid_mvd_decode_video_request && vid_mvd_play_request)
					usleep(1000);

				if(!vid_mvd_play_request)
					break;
				
				result = Util_decoder_read_packet(&type, &packet_index, 1);
				if(result.code != 0)
					Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_decoder_read_packet()..." + result.string + result.error_description, result.code);
				
				var_afk_time = 0;

				if(vid_mvd_pause_request)
				{
					Util_speaker_pause(1);
					while(vid_mvd_pause_request && vid_mvd_play_request && !vid_mvd_seek_request && !vid_mvd_change_video_request)
						usleep(20000);
					
					Util_speaker_resume(1);
				}

				if(vid_mvd_seek_request)
				{
					//µs
					result = Util_decoder_seek(vid_mvd_seek_pos * 1000, 8, 1);
					Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_decoder_seek()..." + result.string + result.error_description, result.code);
					Util_speaker_clear_buffer(1);
					vid_mvd_seek_request = false;
				}

				if(!vid_mvd_play_request || vid_mvd_change_video_request || result.code != 0)
					break;

				if(type == "audio")
				{
					result = Util_decoder_ready_audio_packet(1);
					if(result.code == 0)
					{
						osTickCounterUpdate(&counter);
						result = Util_audio_decoder_decode(&audio_size, &audio, &pos, 1);
						osTickCounterUpdate(&counter);
						vid_mvd_audio_time = osTickCounterRead(&counter);

						if(!has_video && !std::isnan(pos) && !std::isinf(pos))
							vid_mvd_current_pos = pos;

						if(result.code == 0)
						{
							while(true)
							{
								result = Util_speaker_add_buffer(1, ch, audio, audio_size);
								if(result.code == 0 || !vid_mvd_play_request || vid_mvd_seek_request || vid_mvd_change_video_request)
									break;
								
								usleep(3000);
							}
						}
						else
							Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_audio_decoder_decode()..." + result.string + result.error_description, result.code);

						free(audio);
						audio = NULL;
					}
					else
						Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Util_decoder_ready_audio_packet()..." + result.string + result.error_description, result.code);
				}
				else if(type == "video")
				{
					vid_mvd_packet_index = packet_index;
					vid_mvd_decode_video_request = true;
				}
			}
			
			vid_mvd_decode_video_request = false;
			vid_mvd_convert_request = false;
			while(vid_mvd_convert_wait_request || vid_mvd_decode_video_wait_request)
				usleep(10000);

			if(has_audio)
			{
				Util_audio_decoder_exit(1);
				while(Util_speaker_is_playing(1) && vid_mvd_play_request)
					usleep(10000);
				
				Util_speaker_exit(1);
			}
			if(has_video)
			{
				Util_video_decoder_exit(1);
				Util_mvd_video_decoder_exit();
			}

			Util_decoder_close_file(1);

			for(int i = 0; i < 8; i++)
			{
				if(vid_mvd_image_enabled[i])
				{
					Draw_c2d_image_free(vid_mvd_image[i]);
					vid_mvd_image_enabled[i] = false;
				}
			}
			
			vid_mvd_pause_request = false;
			vid_mvd_seek_request = false;
			if(!vid_mvd_change_video_request)
				vid_mvd_play_request = false;
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_mvd_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}

	Util_log_save(DEF_SAPP1_DECODE_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp1_decode_video_thread(void* arg)
{
	Util_log_save(DEF_SAPP1_DECODE_VIDEO_THREAD_STR, "Thread started.");
	bool key = false;
	int w = 0;
	int h = 0;
	int skip = 0;
	int packet_index = 0;
	double pos = 0;
	TickCounter counter;
	Result_with_string result;

	osTickCounterStart(&counter);

	while (vid_mvd_thread_run)
	{	
		if(vid_mvd_play_request)
		{
			key = false;
			w = 0;
			h = 0;
			skip = 0;
			pos = 0;
			
			while(vid_mvd_play_request)
			{
				if(vid_mvd_decode_video_request)
				{
					vid_mvd_decode_video_wait_request = true;
					packet_index = vid_mvd_packet_index;
					if(vid_mvd_allow_skip_frames && skip > vid_mvd_frametime)
					{
						skip -= vid_mvd_frametime;
						Util_decoder_skip_video_packet(packet_index, 1);
						vid_mvd_decode_video_wait_request = false;
						vid_mvd_decode_video_request = false;
					}
					else
					{
						result = Util_decoder_ready_video_packet(packet_index, 1);
						vid_mvd_decode_video_request = false;

						if(result.code == 0)
						{
							osTickCounterUpdate(&counter);
							result = Util_mvd_video_decoder_decode(&w, &h, &key, &pos, 1);
							osTickCounterUpdate(&counter);
							vid_mvd_video_time = osTickCounterRead(&counter);

							if(!std::isnan(pos) && !std::isinf(pos))
								vid_mvd_current_pos = pos;

							if(result.code == 0)
								vid_mvd_convert_request = true;
							else
								Util_log_save(DEF_SAPP1_DECODE_VIDEO_THREAD_STR, "Util_video_decoder_decode()..." + result.string + result.error_description, result.code);

							vid_mvd_decode_video_wait_request = false;

							vid_mvd_time[0][319] = vid_mvd_video_time;

							if(vid_mvd_frametime - vid_mvd_video_time > 0)
								usleep((vid_mvd_frametime - vid_mvd_video_time) * 1000);
							else if(vid_mvd_allow_skip_frames)
								skip -= vid_mvd_frametime - vid_mvd_video_time;

							if(vid_mvd_min_time > vid_mvd_video_time)
								vid_mvd_min_time = vid_mvd_video_time;
							if(vid_mvd_max_time < vid_mvd_video_time)
								vid_mvd_max_time = vid_mvd_video_time;
							
							vid_mvd_total_time += vid_mvd_video_time;
							vid_mvd_total_frames++;

							vid_mvd_recent_time[89] = vid_mvd_video_time;
							vid_mvd_recent_total_time = 0;
							for(int i = 0; i < 90; i++)
								vid_mvd_recent_total_time += vid_mvd_recent_time[i];

							for(int i = 1; i < 90; i++)
								vid_mvd_recent_time[i - 1] = vid_mvd_recent_time[i];

							for(int i = 1; i < 320; i++)
								vid_mvd_time[0][i - 1] = vid_mvd_time[0][i];
						}
						else
						{
							vid_mvd_decode_video_wait_request = false;
							Util_log_save(DEF_SAPP1_DECODE_VIDEO_THREAD_STR, "Util_decoder_ready_video_packet()..." + result.string + result.error_description, result.code);
						}
					}
				}
				else
					usleep(1000);
			}
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_mvd_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}
	
	Util_log_save(DEF_SAPP1_DECODE_VIDEO_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp1_convert_thread(void* arg)
{
	Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Thread started.");
	u8* yuv_video = NULL;
	u8* video = NULL;
	TickCounter counter[3];
	Result_with_string result;

	APT_SetAppCpuTimeLimit(5);
	osTickCounterStart(&counter[0]);
	osTickCounterStart(&counter[1]);
	osTickCounterStart(&counter[2]);

	while (vid_mvd_thread_run)
	{	
		if(vid_mvd_play_request)
		{
			while(vid_mvd_play_request)
			{
				if(vid_mvd_convert_request)
				{
					vid_mvd_convert_wait_request = true;
					osTickCounterUpdate(&counter[2]);
					osTickCounterUpdate(&counter[0]);
					result = Util_mvd_video_decoder_get_image(&video, vid_mvd_width, vid_mvd_height, 1);
					osTickCounterUpdate(&counter[0]);
					vid_mvd_copy_time[0] = osTickCounterRead(&counter[0]);
					
					vid_mvd_convert_request = false;
					if(result.code == 0)
					{
						//result = Util_converter_yuv420p_to_bgr565(yuv_video, &video, vid_mvd_width, vid_mvd_height);

						if(result.code == 0)
						{
							osTickCounterUpdate(&counter[1]);

							result = Draw_set_texture_data(&vid_mvd_image[vid_mvd_image_num * 4], video, vid_mvd_width, vid_mvd_height, 1024, 1024, GPU_RGB565);
							if(result.code != 0)
								Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);

							if(vid_mvd_width > 1024)
							{
								result = Draw_set_texture_data(&vid_mvd_image[vid_mvd_image_num * 4 + 1], video, vid_mvd_width, vid_mvd_height, 1024, 0, 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
							}
							if(vid_mvd_height > 1024)
							{
								result = Draw_set_texture_data(&vid_mvd_image[vid_mvd_image_num * 4 + 2], video, vid_mvd_width, vid_mvd_height, 0, 1024, 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
							}
							if(vid_mvd_width > 1024 && vid_mvd_height > 1024)
							{
								result = Draw_set_texture_data(&vid_mvd_image[vid_mvd_image_num * 4 + 3], video, vid_mvd_width, vid_mvd_height, 1024, 1024, 1024, 1024, GPU_RGB565);
								if(result.code != 0)
									Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Draw_set_texture_data()..." + result.string + result.error_description, result.code);
							}

							if(vid_mvd_image_num == 0)
								vid_mvd_image_num = 1;
							else
								vid_mvd_image_num = 0;

							osTickCounterUpdate(&counter[1]);
							vid_mvd_copy_time[1] = osTickCounterRead(&counter[1]);
							
							var_need_reflesh = true;
						}
						else
							Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Util_converter_yuv420p_to_bgr565()..." + result.string + result.error_description, result.code);
					}
					else
						Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Util_video_decoder_get_image()..." + result.string + result.error_description, result.code);

					free(yuv_video);
					free(video);
					yuv_video = NULL;
					video = NULL;

					vid_mvd_convert_wait_request = false;

					osTickCounterUpdate(&counter[2]);
					vid_mvd_time[1][319] = osTickCounterRead(&counter[2]);
					for(int i = 1; i < 320; i++)
						vid_mvd_time[1][i - 1] = vid_mvd_time[1][i];
				}
				else
					usleep(1000);
			}
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);

		while (vid_mvd_thread_suspend)
			usleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
	}
	
	Util_log_save(DEF_SAPP1_CONVERT_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Sapp1_resume(void)
{
	vid_mvd_thread_suspend = false;
	vid_mvd_main_run = true;
	var_need_reflesh = true;
	Menu_suspend();
}

void Sapp1_suspend(void)
{
	vid_mvd_thread_suspend = true;
	vid_mvd_main_run = false;
	Menu_resume();
}

void Sapp1_init(void)
{
	Util_log_save(DEF_SAPP1_INIT_STR, "Initializing...");
	bool new_3ds = false;
	Result_with_string result;
	
	APT_CheckNew3DS(&new_3ds);
	vid_mvd_thread_run = true;
	if(new_3ds)
	{
		vid_mvd_decode_thread = threadCreate(Sapp1_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 0, false);
		vid_mvd_decode_video_thread = threadCreate(Sapp1_decode_video_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_mvd_convert_thread = threadCreate(Sapp1_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	}
	else
	{
		vid_mvd_thread_run = false;
		Util_err_set_error_message("[Error] New 3DS only", "", DEF_SAPP1_INIT_STR, DEF_ERR_OTHER);
		Util_err_set_error_show_flag(true);
	}

	vid_mvd_total_time = 0;
	vid_mvd_total_frames = 0;
	vid_mvd_min_time = 99999999;
	vid_mvd_max_time = 0;
	vid_mvd_recent_total_time = 0;
	for(int i = 0; i < 90; i++)
		vid_mvd_recent_time[i] = 0;

	for(int i = 0; i < 8; i++)
	{
		vid_mvd_tex_width[i] = 0;
		vid_mvd_tex_height[i] = 0;
	}

	for(int i = 0 ; i < 320; i++)
	{
		vid_mvd_time[0][i] = 0;
		vid_mvd_time[1][i] = 0;
	}

	for(int i = 0; i < 8; i++)
		vid_mvd_image_enabled[i] = false;

	vid_mvd_audio_time = 0;
	vid_mvd_video_time = 0;
	vid_mvd_copy_time[0] = 0;
	vid_mvd_copy_time[1] = 0;

	result = Draw_load_texture("romfs:/gfx/draw/video_player/banner.t3x", 63, vid_mvd_banner, 0, 2);
	Util_log_save(DEF_SAPP1_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	result = Draw_load_texture("romfs:/gfx/draw/video_player/control.t3x", 64, vid_mvd_control, 0, 2);
	Util_log_save(DEF_SAPP1_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	vid_mvd_detail_mode = false;
	vid_mvd_show_controls = false;
	vid_mvd_allow_skip_frames = false;
	vid_mvd_lr_count = 0;
	vid_mvd_cd_count = 0;
	vid_mvd_x = 0;
	vid_mvd_y = 15;
	vid_mvd_frametime = 0;
	vid_mvd_framerate = 0;
	vid_mvd_current_pos = 0;
	vid_mvd_duration = 0;
	vid_mvd_zoom = 1;
	vid_mvd_width = 0;
	vid_mvd_height = 0;
	vid_mvd_file = "";
	vid_mvd_dir = "";
	vid_mvd_video_format = "n/a";
	vid_mvd_audio_format = "n/a";

	result = Util_load_msg("sapp1_" + var_lang + ".txt", vid_mvd_msg, DEF_SAPP1_NUM_OF_MSG);
	Util_log_save(DEF_SAPP1_INIT_STR, "Util_load_msg()..." + result.string + result.error_description, result.code);

	Sapp1_resume();
	vid_mvd_already_init = true;
	Util_log_save(DEF_SAPP1_INIT_STR, "Initialized.");
}

void Sapp1_exit(void)
{
	Util_log_save(DEF_SAPP1_EXIT_STR, "Exiting...");
	u64 time_out = 10000000000;
	Result_with_string result;

	vid_mvd_already_init = false;
	vid_mvd_thread_suspend = false;
	vid_mvd_thread_run = false;
	vid_mvd_convert_request = false;
	vid_mvd_play_request = false;

	Util_log_save(DEF_SAPP1_EXIT_STR, "threadJoin()...", threadJoin(vid_mvd_decode_thread, time_out));
	Util_log_save(DEF_SAPP1_EXIT_STR, "threadJoin()...", threadJoin(vid_mvd_decode_video_thread, time_out));
	Util_log_save(DEF_SAPP1_EXIT_STR, "threadJoin()...", threadJoin(vid_mvd_convert_thread, time_out));
	threadFree(vid_mvd_decode_thread);
	threadFree(vid_mvd_decode_video_thread);
	threadFree(vid_mvd_convert_thread);

	Draw_free_texture(63);
	Draw_free_texture(64);

	
	Util_log_save(DEF_SAPP1_EXIT_STR, "Exited.");
}

void Sapp1_main(void)
{
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;
	int image_num = 0;
	Hid_info key;
	Util_hid_query_key_state(&key);
	Util_hid_key_flag_reset();

	if (var_night_mode)
	{
		color = DEF_DRAW_WHITE;
		back_color = DEF_DRAW_BLACK;
	}
	if(vid_mvd_image_num == 0)
		image_num = 1;
	else
		image_num = 0;

	if(var_need_reflesh || !var_eco_mode)
	{
		var_need_reflesh = false;
		Draw_frame_ready();
		Draw_screen_ready(0, back_color);

		if(vid_mvd_play_request)
		{
			//video
			Draw_texture(vid_mvd_image[image_num * 4].c2d, vid_mvd_x, vid_mvd_y, vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4] * vid_mvd_zoom);
			if(vid_mvd_width > 1024)
				Draw_texture(vid_mvd_image[image_num * 4 + 1].c2d, (vid_mvd_x + vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom), vid_mvd_y, vid_mvd_tex_width[image_num * 4 + 1] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4 + 1] * vid_mvd_zoom);
			if(vid_mvd_height > 1024)
				Draw_texture(vid_mvd_image[image_num * 4 + 2].c2d, vid_mvd_x, (vid_mvd_y + vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom), vid_mvd_tex_width[image_num * 4 + 2] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4 + 2] * vid_mvd_zoom);
			if(vid_mvd_width > 1024 && vid_mvd_height > 1024)
				Draw_texture(vid_mvd_image[image_num * 4 + 3].c2d, (vid_mvd_x + vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom), (vid_mvd_y + vid_mvd_tex_height[image_num * 4] * vid_mvd_zoom), vid_mvd_tex_width[image_num * 4 + 3] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4 + 3] * vid_mvd_zoom);
		}
		else
		{
			Draw_texture(vid_mvd_banner[var_night_mode], 0, 15, 400, 225);
			Draw(vid_mvd_msg[12], 0, 15, 0.5, 0.5, DEF_DRAW_RED);
		}

		if(Util_log_query_log_show_flag())
			Util_log_draw();

		Draw_top_ui();

		Draw_screen_ready(1, back_color);

		Draw(DEF_SAPP1_VER, 0, 0, 0.4, 0.4, DEF_DRAW_GREEN);

		//codec info
		Draw(vid_mvd_video_format, 0, 10, 0.5, 0.5, color);
		Draw(vid_mvd_audio_format, 0, 20, 0.5, 0.5, color);
		Draw(std::to_string(vid_mvd_width) + "x" + std::to_string(vid_mvd_height) + "@" + std::to_string(vid_mvd_framerate).substr(0, 5) + "fps", 0, 30, 0.5, 0.5, color);

		if(vid_mvd_play_request)
		{
			//video
			Draw_texture(vid_mvd_image[image_num * 4].c2d, vid_mvd_x - 40, vid_mvd_y - 240, vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4] * vid_mvd_zoom);
			if(vid_mvd_width > 1024)
				Draw_texture(vid_mvd_image[image_num * 4 + 1].c2d, (vid_mvd_x + vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom) - 40, vid_mvd_y - 240, vid_mvd_tex_width[image_num * 4 + 1] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4 + 1] * vid_mvd_zoom);
			if(vid_mvd_height > 1024)
				Draw_texture(vid_mvd_image[image_num * 4 + 2].c2d, vid_mvd_x - 40, (vid_mvd_y + vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom) - 240, vid_mvd_tex_width[image_num * 4 + 2] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4 + 2] * vid_mvd_zoom);
			if(vid_mvd_width > 1024 && vid_mvd_height > 1024)
				Draw_texture(vid_mvd_image[image_num * 4 + 3].c2d, (vid_mvd_x + vid_mvd_tex_width[image_num * 4] * vid_mvd_zoom) - 40, (vid_mvd_y + vid_mvd_tex_height[image_num * 4] * vid_mvd_zoom) - 240, vid_mvd_tex_width[image_num * 4 + 3] * vid_mvd_zoom, vid_mvd_tex_height[image_num * 4 + 3] * vid_mvd_zoom);
		}

		//controls
		Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 165, 165, 145, 10);
		Draw(vid_mvd_msg[2], 167.5, 165, 0.4, 0.4, color);

		//texture filter
		Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 10, 180, 145, 10);
		Draw(vid_mvd_msg[vid_mvd_linear_filter], 12.5, 180, 0.4, 0.4, color);

		//allow skip frames
		Draw_texture(var_square_image[0], DEF_DRAW_WEAK_AQUA, 165, 180, 145, 10);
		Draw(vid_mvd_msg[3 + vid_mvd_allow_skip_frames], 167.5, 180, 0.4, 0.4, color);

		//time bar
		Draw(Util_convert_seconds_to_time(vid_mvd_current_pos / 1000) + "/" + Util_convert_seconds_to_time(vid_mvd_duration), 110, 210, 0.5, 0.5, color);
		Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 5, 195, 310, 10);
		if(vid_mvd_duration != 0)
			Draw_texture(var_square_image[0], 0xFF800080, 5, 195, 310 * ((vid_mvd_current_pos / 1000) / vid_mvd_duration), 10);

		if(vid_mvd_detail_mode)
		{
			//decoding detail
			for(int i = 0; i < 319; i++)
			{
				Draw_line(i, 110 - vid_mvd_time[1][i], DEF_DRAW_BLUE, i + 1, 110 - vid_mvd_time[1][i + 1], DEF_DRAW_BLUE, 1);//Thread 1
				Draw_line(i, 110 - vid_mvd_time[0][i], DEF_DRAW_RED, i + 1, 110 - vid_mvd_time[0][i + 1], DEF_DRAW_RED, 1);//Thread 0
			}

			Draw_line(0, 110, color, 320, 110, color, 2);
			Draw_line(0, 110 - vid_mvd_frametime, 0xFFFFFF00, 320, 110 - vid_mvd_frametime, 0xFFFFFF00, 2);
			if(vid_mvd_total_frames != 0 && vid_mvd_min_time != 0  && vid_mvd_recent_total_time != 0)
			{
				Draw("avg " + std::to_string(1000 / (vid_mvd_total_time / vid_mvd_total_frames)).substr(0, 5) + " min " + std::to_string(1000 / vid_mvd_max_time).substr(0, 5) 
				+ " max " + std::to_string(1000 / vid_mvd_min_time).substr(0, 5) + " recent avg " + std::to_string(1000 / (vid_mvd_recent_total_time / 90)).substr(0, 5) +  " fps", 0, 110, 0.4, 0.4, color);
			}

			Draw("Deadline : " + std::to_string(vid_mvd_frametime).substr(0, 5) + "ms", 0, 120, 0.4, 0.4, 0xFFFFFF00);			
			Draw("Video decode : " + std::to_string(vid_mvd_video_time).substr(0, 5) + "ms", 0, 130, 0.4, 0.4, DEF_DRAW_RED);
			Draw("Audio decode : " + std::to_string(vid_mvd_audio_time).substr(0, 5) + "ms", 0, 140, 0.4, 0.4, DEF_DRAW_RED);
			Draw("Data copy 0 : " + std::to_string(vid_mvd_copy_time[0]).substr(0, 5) + "ms", 160, 130, 0.4, 0.4, DEF_DRAW_BLUE);
			Draw("Data copy 1 : " + std::to_string(vid_mvd_copy_time[1]).substr(0, 5) + "ms", 160, 140, 0.4, 0.4, DEF_DRAW_BLUE);
			Draw("Thread 0 : " + std::to_string(vid_mvd_time[0][319]).substr(0, 6) + "ms", 0, 150, 0.5, 0.5, DEF_DRAW_RED);
			Draw("Thread 1 : " + std::to_string(vid_mvd_time[1][319]).substr(0, 6) + "ms", 160, 150, 0.5, 0.5, DEF_DRAW_BLUE);
			Draw("Zoom : x" + std::to_string(vid_mvd_zoom).substr(0, 5) + " X : " + std::to_string((int)vid_mvd_x) + " Y : " + std::to_string((int)vid_mvd_y), 0, 160, 0.5, 0.5, color);
		}

		if(vid_mvd_show_controls)
		{
			Draw_texture(vid_mvd_control[var_night_mode], 80, 20, 160, 160);
			Draw(vid_mvd_msg[5], 122.5, 47.5, 0.45, 0.45, DEF_DRAW_BLACK);
			Draw(vid_mvd_msg[6], 122.5, 62.5, 0.45, 0.45, DEF_DRAW_BLACK);
			Draw(vid_mvd_msg[7], 122.5, 77.5, 0.45, 0.45, DEF_DRAW_BLACK);
			Draw(vid_mvd_msg[8], 122.5, 92.5, 0.45, 0.45, DEF_DRAW_BLACK);
			Draw(vid_mvd_msg[9], 135, 107.5, 0.45, 0.45, DEF_DRAW_BLACK);
			Draw(vid_mvd_msg[10], 122.5, 122.5, 0.45, 0.45, DEF_DRAW_BLACK);
			Draw(vid_mvd_msg[11], 132.5, 137.5, 0.45, 0.45, DEF_DRAW_BLACK);
		}

		if(Util_expl_query_show_flag())
			Util_expl_draw();

		if(Util_err_query_error_show_flag())
			Util_err_draw();

		Draw_bot_ui();
		Draw_touch_pos();

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
			Sapp1_suspend();
		else if(key.p_a)
		{
			if(vid_mvd_play_request)
				vid_mvd_pause_request = !vid_mvd_pause_request;
			else
				vid_mvd_play_request = true;

			var_need_reflesh = true;
		}
		else if(key.p_b)
		{
			vid_mvd_play_request = false;
			var_need_reflesh = true;
		}
		else if(key.p_x)
		{
			Util_expl_set_show_flag(true);
			Util_expl_set_callback(Sapp1_callback);
			Util_expl_set_cancel_callback(Sapp1_cancel);
		}
		else if(key.p_y)
		{
			vid_mvd_detail_mode = !vid_mvd_detail_mode;
			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 5 && key.touch_x <= 314 && key.touch_y >= 195 && key.touch_y <= 204)
		{
			vid_mvd_seek_pos = (vid_mvd_duration * 1000) * (((double)key.touch_x - 5) / 310);
			vid_mvd_seek_request = true;
			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 165 && key.touch_x <= 309 && key.touch_y >= 165 && key.touch_y <= 174)
		{
			vid_mvd_show_controls = !vid_mvd_show_controls;
			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 10 && key.touch_x <= 154 && key.touch_y >= 180 && key.touch_y <= 189)
		{
			vid_mvd_linear_filter = !vid_mvd_linear_filter;
			for(int i = 0; i < 8; i++)
			{
				if(vid_mvd_image_enabled[i])
					Draw_c2d_image_set_filter(&vid_mvd_image[i], vid_mvd_linear_filter);
			}

			var_need_reflesh = true;
		}
		else if(key.p_touch && key.touch_x >= 165 && key.touch_x <= 309 && key.touch_y >= 180 && key.touch_y <= 189)
		{
			vid_mvd_allow_skip_frames = !vid_mvd_allow_skip_frames;
			var_need_reflesh = true;
		}
		else if(key.h_touch || key.p_touch)
			var_need_reflesh = true;
		
		if(key.p_c_down || key.p_c_up || key.p_c_right || key.p_c_left || key.h_c_down || key.h_c_up || key.h_c_right || key.h_c_left
		|| key.p_d_down || key.p_d_up || key.p_d_right || key.p_d_left || key.h_d_down || key.h_d_up || key.h_d_right || key.h_d_left)
		{
			if(key.p_c_down || key.p_d_down)
				vid_mvd_y -= 1 * var_scroll_speed * key.count;
			else if(key.h_c_down || key.h_d_down)
			{
				if(vid_mvd_cd_count > 600)
					vid_mvd_y -= 10 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 240)
					vid_mvd_y -= 7.5 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 5)
					vid_mvd_y -= 5 * var_scroll_speed * key.count;
			}

			if(key.p_c_up || key.p_d_up)
				vid_mvd_y += 1 * var_scroll_speed * key.count;
			else if(key.h_c_up || key.h_d_up)
			{
				if(vid_mvd_cd_count > 600)
					vid_mvd_y += 10 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 240)
					vid_mvd_y += 7.5 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 5)
					vid_mvd_y += 5 * var_scroll_speed * key.count;
			}

			if(key.p_c_right || key.p_d_right)
				vid_mvd_x -= 1 * var_scroll_speed * key.count;
			else if(key.h_c_right || key.h_d_right)
			{
				if(vid_mvd_cd_count > 600)
					vid_mvd_x -= 10 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 240)
					vid_mvd_x -= 7.5 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 5)
					vid_mvd_x -= 5 * var_scroll_speed * key.count;
			}

			if(key.p_c_left || key.p_d_left)
				vid_mvd_x += 1 * var_scroll_speed * key.count;
			else if(key.h_c_left || key.h_d_left)
			{
				if(vid_mvd_cd_count > 600)
					vid_mvd_x += 10 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 240)
					vid_mvd_x += 7.5 * var_scroll_speed * key.count;
				else if(vid_mvd_cd_count > 5)
					vid_mvd_x += 5 * var_scroll_speed * key.count;
			}

			if(vid_mvd_x > 400)
				vid_mvd_x = 400;
			else if(vid_mvd_x < -vid_mvd_width * vid_mvd_zoom)
				vid_mvd_x = -vid_mvd_width * vid_mvd_zoom;

			if(vid_mvd_y > 480)
				vid_mvd_y = 480;
			else if(vid_mvd_y < -vid_mvd_height * vid_mvd_zoom)
				vid_mvd_y = -vid_mvd_height * vid_mvd_zoom;

			vid_mvd_cd_count++;
			var_need_reflesh = true;
		}
		else
			vid_mvd_cd_count = 0;

		if(key.p_l || key.p_r || key.h_l || key.h_r)
		{
			if(key.p_l)
				vid_mvd_zoom -= 0.005 * var_scroll_speed * key.count;
			else if(key.h_l)
			{
				if(vid_mvd_lr_count > 360)
					vid_mvd_zoom -= 0.05 * var_scroll_speed * key.count;
				else if(vid_mvd_lr_count > 120)
					vid_mvd_zoom -= 0.01 * var_scroll_speed * key.count;
				else if(vid_mvd_lr_count > 5)
					vid_mvd_zoom -= 0.005 * var_scroll_speed * key.count;
			}

			if(key.p_r)
				vid_mvd_zoom += 0.005 * var_scroll_speed * key.count;
			else if(key.h_r)
			{
				if(vid_mvd_lr_count > 360)
					vid_mvd_zoom += 0.05 * var_scroll_speed * key.count;
				else if(vid_mvd_lr_count > 120)
					vid_mvd_zoom += 0.01 * var_scroll_speed * key.count;
				else if(vid_mvd_lr_count > 5)
					vid_mvd_zoom += 0.005 * var_scroll_speed * key.count;
			}

			if(vid_mvd_zoom < 0.05)
				vid_mvd_zoom = 0.05;
			else if(vid_mvd_zoom > 10)
				vid_mvd_zoom = 10;

			vid_mvd_lr_count++;
			var_need_reflesh = true;
		}
		else
			vid_mvd_lr_count = 0;
	}

	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}
