#include "definitions.hpp"
#include "system/types.hpp"

#include "system/menu.hpp"
#include "system/variables.hpp"

#include "system/draw/draw.hpp"

#include "system/util/change_setting.hpp"
#include "system/util/converter.hpp"
#include "system/util/decoder.hpp"
#include "system/util/error.hpp"
#include "system/util/explorer.hpp"
#include "system/util/file.hpp"
#include "system/util/hid.hpp"
#include "system/util/log.hpp"
#include "system/util/speaker.hpp"
#include "system/util/swkbd.hpp"
#include "system/util/queue.hpp"
#include "system/util/util.hpp"

//Include myself.
#include "video_player.hpp"

#define DEF_VID_QUEUE_ADD_WITH_LOG(thread_name, queue, event, msg, timeout_us, option) \
{ \
	char log_buffer[256]; \
	Result_with_string queue_result = Util_queue_add(queue, event, msg, timeout_us, option); \
	snprintf(log_buffer, sizeof(log_buffer), "Util_queue_add() %s...%s%s", #event, queue_result.string.c_str(), queue_result.error_description.c_str()); \
	Util_log_save(thread_name, log_buffer, queue_result.code); \
}

void Vid_init_hidden_settings(void);
void Vid_init_debug_view_mode(void);
void Vid_init_debug_view_data(void);
void Vid_init_player_data(void);
void Vid_init_media_data(void);
void Vid_init_video_data(void);
void Vid_init_audio_data(void);
void Vid_init_subtitle_data(void);
void Vid_init_ui_data(void);

struct Large_image
{
	int image_width;
	int image_height;
	int num_of_images;
	Image_data* images;
};

enum Vid_command
{
	NONE_REQUEST,

	DECODE_THREAD_PLAY_REQUEST,						//Start playback request.
	DECODE_THREAD_PAUSE_REQUEST,					//Pause playback request.
	DECODE_THREAD_RESUME_REQUEST,					//Resume playback request.
	DECODE_THREAD_CHANGE_AUDIO_TRACK_REQUEST,		//Change audio track request.
	DECODE_THREAD_CHANGE_SUBTITLE_TRACK_REQUEST,	//Change subtitle track request.
	DECODE_THREAD_SEEK_REQUEST,						//Seek request (from hid thread).
	DECODE_THREAD_PLAY_NEXT_REQUEST,				//Change file (or repeat) request.
	DECODE_THREAD_ABORT_REQUEST,					//Stop request.
	DECODE_THREAD_SHUTDOWN_REQUEST,					//Exit request.

	READ_PACKET_THREAD_READ_PACKET_REQUEST,			//Read a file request.
	READ_PACKET_THREAD_SEEK_REQUEST,				//Seek request (from decode theread).
	READ_PACKET_THREAD_ABORT_REQUEST,				//Stop request.

	DECODE_VIDEO_THREAD_DECODE_REQUEST,				//Decode a frame request.
	DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST,		//Clear cache request (in seek process).
	DECODE_VIDEO_THREAD_ABORT_REQUEST,				//Stop request.

	CONVERT_THREAD_CONVERT_REQUEST,					//Start converting request. 
	CONVERT_THREAD_ABORT_REQUEST,					//Stop request.

	MAX_REQUEST = 0xFFFFFFFF,
};

enum Vid_notification
{
	NONE_NOTIFICATION,

	DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION,				//Done buffering because of memory limit or eof.

	READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION,			//Done reading a file because of buffer limit.
	READ_PACKET_THREAD_FINISHED_READING_EOF_NOTIFICATION,		//Done reading a file because of eof.
	READ_PACKET_THREAD_FINISHED_SEEKING_NOTIFICATION,			//Done seeking.
	READ_PACKET_THREAD_FINISHED_ABORTING_NOTIFICATION,			//Done aborting.

	DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION,	//Done copying a video packet.
	DECODE_VIDEO_THREAD_FINISHED_CLEARING_CACHE,				//Done clearing cache.
	DECODE_VIDEO_THREAD_FINISHED_ABORTING_NOTIFICATION,			//Done aborting.

	CONVERT_THREAD_OUT_OF_BUFFER_NOTIFICATION,					//Run out of video buffer.
	CONVERT_THREAD_FINISHED_BUFFERING_NOTIFICATION,				//Done buffering because of threshold or buffer limit.
	CONVERT_THREAD_FINISHED_ABORTING_NOTIFICATION,				//Done aborting.

	MAX_NOTIFICATION = 0xFFFFFFFF,
};

enum Vid_player_main_state
{
	PLAYER_STATE_IDLE,				//File is not opened.
	PLAYER_STATE_PREPATE_PLAYING,	//File is not opened, but will be opened soon.
	PLAYER_STATE_PLAYING,			//File is opened and playing.
	PLAYER_STATE_PAUSE,				//File is opened but not playing.
	PLAYER_STATE_BUFFERING,			//File is opened but it runs out of video buffer, buffering is in progress.
	PLAYER_STATE_PREPARE_SEEKING,	//File is opened and seek request has been received.
	PLAYER_STATE_SEEKING,			//File is opened and seek is in progress.
};

enum Vid_player_sub_state
{
	PLAYER_SUB_STATE_NONE		 			= 0,		//Nothing special.
	PLAYER_SUB_STATE_HW_DECODING			= (1 << 0),	//Player is using hardware decoding.
	PLAYER_SUB_STATE_HW_CONVERSION			= (1 << 1),	//Player is using hardware color converter.
	PLAYER_SUB_STATE_TOO_BIG				= (1 << 2),	//Volume is too big.
	PLAYER_SUB_STATE_RESUME_LATER			= (1 << 3),	//After buffering or seeking, playback should be resumed.
	PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT		= (1 << 4),	//Waiting for seek backward.
};

struct Vid_file
{
	char name[DEF_VID_MAX_FILE_NAME_LENGTH];	//File name.
	char directory[DEF_VID_MAX_PATH_LENGTH];	//Directory path for this file.
	int index = 0;								//File index of this file.
};

struct Vid_video_packet_data
{
	bool is_key_frame = false;	//Whether current packet is key frame.
	int packet_index = -1;		//Packet index for current packet.
};

struct Vid_player
{
	//Settings (can be changed while playing videos).
	bool remember_video_pos = false;				//Whether save playback position so that we can resume from there later.
	bool use_linear_texture_filter = true;			//Whether apply linear texture filter.
	bool correct_aspect_ratio = true;				//Whether correct aspect ratio based on SAR values.
	bool allow_skip_frames = false;					//Whether allow skip non-keyframes when it can't keep up.
	bool allow_skip_key_frames = false;				//Whether allow skip keyframes when it can't keep up.
	int playback_mode = DEF_VID_NO_REPEAT;			//Playback mode.
	int volume = 100;								//Software controlled volume level in %.
	int seek_duration = 5;							//Seek duration for DPAD (direction pad) left and right in seconds.
	int move_content_mode = DEF_VID_MOVE_BOTH;		//Move content mode.
	int restart_playback_threshold = 48;			//Restart playback threshold after out of video buffer.

	//Settings (can NOT be changed while playing videos).
	bool disable_audio = false;						//Whether disable audio.
	bool disable_video = false;						//Whether disable video.
	bool disable_subtitle = false;					//Whether disable subtitle.
	bool use_hw_decoding = true;					//Whether use hardware decoder if available.
	bool use_hw_color_conversion = true;			//Whether use hardware color conversion if available.
	bool use_multi_threaded_decoding = true;		//Whether use multi-threaded software decoding if available.
	int lower_resolution = 0;						//Downscale level on supported codecs, 0 = 100% (no downscale), 1 = 50%, 2 = 25%.

	//Other settings (user doesn't have permission to change).
	bool show_full_screen_msg = true;				//Whether show how to exit full screen msg.
	int num_of_threads = 1;							//Number of threads to use for multi-threaded software decoding.
	Multi_thread_type thread_mode = THREAD_TYPE_AUTO;	//Thread mode to use for multi-threaded software decoding.

	//Debug view.
	bool show_decoding_graph = true;				//Whether show decode time graph in debug view.
	bool show_color_conversion_graph = true;		//Whether show color conversion time graph in debug view.
	bool show_packet_buffer_graph = true;			//Whether show packet buffer health graph in debug view.
	bool show_raw_video_buffer_graph = true;		//Whether show video buffer health graph in debug view.
	bool show_raw_audio_buffer_graph = true;		//Whether show audio buffer health graph in debug view.
	int total_frames = 0;							//Total number of decoded frames.
	u64 previous_ts = 0;							//Time stamp for last graph update (only for packet, video and audio buffers).
	double decoding_min_time = 0;					//Minimum video decoding time in ms.
	double decoding_max_time = 0;					//Maximum video decoding time in ms.
	double decoding_total_time = 0;					//Total video decoding time in ms.
	double decoding_recent_total_time = 0;			//Same as above, but only for recent 90 frames.
	bool keyframe_list[320];						//List for keyframe.
	int packet_buffer_list[320];					//List for packet buffer health.
	int raw_video_buffer_list[320];					//List for video buffer health.
	int raw_audio_buffer_list[320];					//List for audio buffer health.
	double video_decoding_time_list[320];			//List for video decoding time in ms.
	double audio_decoding_time_list[320];			//List for audio decoding time in ms.
	double conversion_time_list[320];				//List for color conversion time in ms.

	//Player.
	Vid_player_main_state state = PLAYER_STATE_IDLE;		//Main state.
	Vid_player_sub_state sub_state = PLAYER_SUB_STATE_NONE;	//Sub state.
	Vid_file file;											//File info.

	//Media.
	double media_duration = 0;						//Media duration in ms, if the file contains both video and audio, then the longest one's duration.
	double media_current_pos = 0;					//Current media position in ms, if the file contains both video and audio,
													//then the longest one's position.
	double seek_pos_cache = 0;						//Seek destination position cache, this is used when user is holding the seek bar.
	double seek_pos = 0;							//Seek destination position in ms.

	//Video.
	int num_of_video_tracks = 0;					//Number of video tracks for current file.
	int image_index = 0;							//Next texture buffer index to store converted image.
	int image_index_3d = 0;							//Next 3d video texture buffer (for right eye) index to store converted image.
	int vps = 0;									//Actual video playback framerate.
	double video_current_pos = 0;					//Current video position in ms.
	double video_x_offset = 0;						//X (horizontal) offset for video.
	double video_y_offset = 15;						//Y (vertical) offset for video.
	double video_zoom = 1;							//Zoom level for video.
	double video_frametime = 0;						//Video frametime in ms, if file contains 2 video tracks, then this will be (actual_frametime / 2).
	Video_info video_info[DEF_DECODER_MAX_VIDEO_TRACKS];	//Video info.
	Large_image large_image[DEF_VID_VIDEO_BUFFERS][2];		//Texture for video images.

	//Audio.
	int num_of_audio_tracks = 0;					//Number of audio tracks for current file.
	int selected_audio_track_cache = 0;				//Selected audio track cache, this is used when user is selecting audio track.
	int selected_audio_track = 0;					//Selected audio track.
	double audio_current_pos = 0;					//Current audio position in ms.
	double last_decoded_audio_pos = 0;				//Latest decoded audio position in ms.
	Audio_info audio_info[DEF_DECODER_MAX_AUDIO_TRACKS];	//Audio info.

	//Subtitle.
	int num_of_subtitle_tracks = 0;					//Number of subtitle tracks for current file.
	int selected_subtitle_track_cache = 0;			//Selected subtitle track cache, this is used when user is selecting audio track.
	int selected_subtitle_track = 0;				//Selected subtitle track.
	int subtitle_index = 0;							//Next subtitle index for subtitle_data.
	double subtitle_x_offset = 0;					//X (horizontal) offset for subtitle.
	double subtitle_y_offset = 0;					//Y (vertical) offset for subtitle.
	double subtitle_zoom = 1;						//Zoom level for subtitle.
	Subtitle_info subtitle_info[DEF_DECODER_MAX_SUBTITLE_TRACKS];	//Subtitle info.
	Subtitle_data subtitle_data[DEF_VID_SUBTITLE_BUFFERS];			//Subtitle data.

	//UI.
	bool is_full_screen = false;					//Whether player is full screen.
	bool is_pause_for_home_menu = false;			//Whether player state is pause because of nintendo home menu.
	bool is_selecting_audio_track = false;			//Whether user is selecting a audio track.
	bool is_selecting_subtitle_track = false;		//Whether user is selecting a subtitle track.
	bool is_setting_volume = false;					//Whether user is setting volume level.
	bool is_setting_seek_duration = false;			//Whether user is setting seek duration.
	bool is_displaying_controls = false;			//Whether user is checking how to control.
	int turn_off_bottom_screen_count = 0;			//Turn bottom screen off after this count in full screen mode.
	int menu_mode = DEF_VID_MENU_NONE;				//Current menu tab.
	int lr_held_count = 0;							//Hold count for L or R button.
	int cpad_held_count = 0;						//Hold count for CPAD (circle pad).
	int dpad_held_count = 0;						//Hold count for DPAD (direction pad).
	u64 show_screen_brightness_until = 0;			//Display screen brightness message until this time.
	u64 show_current_pos_until = 0;					//Display current position message until this time.
	double ui_y_offset_max = 0;						//Max Y (vertical) offset for UI.
	double ui_y_offset = 0;							//Y (vertical) offset for UI.
	double ui_y_move = 0;							//Remaining Y (vertical) direction movement, this is used for scrolling.

	int banner_texture_handle = -1;					//Banner texture handle used for freeing texture.
	int control_texture_handle = -1;				//Control texture handle used for freeing texture.
	C2D_Image banner[2];							//Banner texture.
	C2D_Image control[2];							//Control texture.

	//Buttons.
	Image_data select_audio_track_button, texture_filter_button, allow_skip_frames_button, allow_skip_key_frames_button,
	volume_button, seek_duration_button, use_hw_decoding_button, use_hw_color_conversion_button, use_multi_threaded_decoding_button,
	lower_resolution_button, menu_button[3], control_button, ok_button, audio_track_button[DEF_DECODER_MAX_AUDIO_TRACKS],
	correct_aspect_ratio_button, move_content_button, remember_video_pos_button, show_decode_graph_button,
	show_color_conversion_graph_button, show_packet_buffer_graph_button, show_raw_video_buffer_graph_button, 
	show_raw_audio_buffer_graph_button, menu_background, scroll_bar, playback_mode_button,
	subtitle_track_button[DEF_DECODER_MAX_SUBTITLE_TRACKS], select_subtitle_track_button, disable_audio_button,
	disable_video_button, disable_subtitle_button, restart_playback_threshold_bar, seek_bar;

	//Threads and queues.
	Thread decode_thread;							//Decode thread handle.
	Queue decode_thread_command_queue;				//Decode thread command queue.
	Queue decode_thread_notification_queue;			//Decode thread notification queue.

	Thread decode_video_thread;						//Decode video thread handle.
	Queue decode_video_thread_command_queue;		//Decode video thread command queue.

	Thread convert_thread;							//Convert thread handle.
	Queue convert_thread_command_queue;				//Convert thread command queue.

	Thread read_packet_thread;						//Read packet thread handle.
	Queue read_packet_thread_command_queue;			//Read packet thread command queue.
};

bool vid_main_run = false;
bool vid_thread_run = false;
bool vid_already_init = false;
bool vid_thread_suspend = true;
std::string vid_status = "";
std::string vid_msg[DEF_VID_NUM_OF_MSG];
Thread vid_init_thread, vid_exit_thread;
Vid_player vid_player;

void Vid_suspend(void);

void Vid_callback(std::string file, std::string dir)
{
	Vid_file* file_data = (Vid_file*)malloc(sizeof(Vid_file));

	//Create a message.
	if(file_data)
	{
		file_data->index = Util_expl_query_current_file_index();
		snprintf(file_data->name, sizeof(file_data->name), "%s", file.c_str());
		snprintf(file_data->directory, sizeof(file_data->directory), "%s", dir.c_str());
	}

	DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_EXPL_CALLBACK_STR, &vid_player.decode_thread_command_queue,
	DECODE_THREAD_PLAY_REQUEST, file_data, 100000, QUEUE_OPTION_NONE)
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

int Vid_get_min_texture_size(int width_or_height)
{
	if(width_or_height <= 16)
		return 16;
	else if(width_or_height <= 32)
		return 32;
	else if(width_or_height <= 64)
		return 64;
	else if(width_or_height <= 128)
		return 128;
	else if(width_or_height <= 256)
		return 256;
	else if(width_or_height <= 512)
		return 512;
	else
		return 1024;
}

void Vid_large_texture_free(Large_image* large_image_data)
{
	if(!large_image_data || !large_image_data->images)
		return;

	for(int i = 0; i < large_image_data->num_of_images; i++)
		Draw_texture_free(&large_image_data->images[i]);

	large_image_data->image_width = 0;
	large_image_data->image_height = 0;
	large_image_data->num_of_images = 0;
	free(large_image_data->images);
	large_image_data->images = NULL;
}

void Vid_large_texture_set_filter(Large_image* large_image_data, bool filter)
{
	if(!large_image_data || !large_image_data->images)
		return;

	for(int i = 0; i < large_image_data->num_of_images; i++)
		Draw_set_texture_filter(&large_image_data->images[i], filter);
}

void Vid_large_texture_crop(Large_image* large_image_data, int width, int height)
{
	int width_offset = 0;
	int height_offset = 0;

	if(!large_image_data || !large_image_data->images
	|| (width == large_image_data->image_width && height == large_image_data->image_height))
		return;

	for(int i = 0; i < large_image_data->num_of_images; i++)
	{
		float texture_width = large_image_data->images[i].c2d.tex->width;
		float texture_height = large_image_data->images[i].c2d.tex->height;

		if(width_offset + texture_width >= width)
		{
			//Crop for X direction.
			int new_width = (width - width_offset);
			if(new_width < 0)
				new_width = 0;

			large_image_data->images[i].subtex->width = new_width;
			large_image_data->images[i].subtex->right = new_width / texture_width;
		}

		if(height_offset + texture_height >= height)
		{
			//Crop for Y direction.
			int new_height = (height - height_offset);
			if(new_height < 0)
				new_height = 0;

			large_image_data->images[i].subtex->height = new_height;
			large_image_data->images[i].subtex->bottom = (texture_height - new_height) / texture_height;
		}

		//Update offset.
		width_offset += texture_width;
		if(width_offset >= large_image_data->image_width)
		{
			width_offset = 0;
			height_offset += texture_height;
		}
	}
}

Result_with_string Vid_large_texture_init(Large_image* large_image_data, int width, int height, Pixel_format color_format, bool zero_initialize)
{
	int loop = 0;
	int width_offset = 0;
	int height_offset = 0;
	Result_with_string result;

	if(!large_image_data || width <= 0 || height <= 0)
		goto invalid_arg;

	//Calculate how many textures we need.
	if(width % 1024 > 0)
		loop = (width / 1024) + 1;
	else
		loop = (width / 1024);

	if(height % 1024 > 0)
		loop *= ((height / 1024) + 1);
	else
		loop *= (height / 1024);

	//Init parameters.
	large_image_data->image_width = 0;
	large_image_data->image_height = 0;
	large_image_data->num_of_images = 0;
	large_image_data->images = (Image_data*)malloc(sizeof(Image_data) * loop);
	if(!large_image_data->images)
		goto out_of_memory;

	for(int i = 0; i < loop; i++)
	{
		int texture_width = Vid_get_min_texture_size(width - width_offset);
		int texture_height = Vid_get_min_texture_size(height - height_offset);

		result = Draw_texture_init(&large_image_data->images[i], texture_width, texture_height, color_format);
		if(result.code != 0)
			goto error_other;

		if(zero_initialize)
		{
			int pixel_size = 0;

			if(color_format == PIXEL_FORMAT_RGB565LE)
				pixel_size = 2;
			else if(color_format == PIXEL_FORMAT_BGR888)
				pixel_size = 3;
			else if(color_format == PIXEL_FORMAT_ABGR8888)
				pixel_size = 4;

			memset(large_image_data->images[i].c2d.tex->data, 0x0, texture_width * texture_height * pixel_size);
		}
		large_image_data->num_of_images++;

		//Update offset.
		width_offset += 1024;
		if(width_offset >= width)
		{
			width_offset = 0;
			height_offset += 1024;
		}
	}

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	Vid_large_texture_free(large_image_data);
	result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
	result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
	return result;

	error_other:
	Vid_large_texture_free(large_image_data);
	return result;
}

Result_with_string Vid_large_texture_set_data(Large_image* large_image_data, u8* raw_image, int width, int height, bool use_direct)
{
	int loop = 0;
	int width_offset = 0;
	int height_offset = 0;
	Result_with_string result;

	if(!large_image_data || !large_image_data->images || large_image_data->num_of_images <= 0 || !raw_image || width <= 0 || height <= 0)
		goto invalid_arg;

	large_image_data->image_width = width;
	large_image_data->image_height = height;

	if(use_direct)
	{
		result = Draw_set_texture_data_direct(&large_image_data->images[0], raw_image, width, height);

		if(result.code != 0)
			goto error_other;
	}
	else
	{
		//Calculate how many textures we need.
		if(width % 1024 > 0)
			loop = (width / 1024) + 1;
		else
			loop = (width / 1024);

		if(height % 1024 > 0)
			loop *= ((height / 1024) + 1);
		else
			loop *= (height / 1024);

		if(loop > large_image_data->num_of_images)
			loop = large_image_data->num_of_images;

		for(int i = 0; i < loop; i++)
		{
			result = Draw_set_texture_data(&large_image_data->images[i], raw_image, width, height, width_offset, height_offset);

			if(result.code != 0)
				goto error_other;

			//Update offset.
			width_offset += 1024;
			if(width_offset >= width)
			{
				width_offset = 0;
				height_offset += 1024;
			}
		}
	}

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	error_other:
	return result;
}

void Vid_large_texture_draw(Large_image* large_image_data, double x_offset, double y_offset, double pic_width, double pic_height)
{
	int width_offset = 0;
	int height_offset = 0;
	double width_factor = 0;
	double height_factor = 0;

	if(!large_image_data || !large_image_data->images || large_image_data->num_of_images <= 0
	|| large_image_data->image_width <= 0 || large_image_data->image_height <= 0)
		return;

	width_factor = pic_width / large_image_data->image_width;
	height_factor = pic_height / large_image_data->image_height;

	for(int i = 0; i < large_image_data->num_of_images; i++)
	{
		if(large_image_data->images[i].subtex)
		{
			double texture_x_offset = x_offset + (width_offset * width_factor);
			double texture_y_offset = y_offset + (height_offset * height_factor);
			double texture_width = large_image_data->images[i].subtex->width * width_factor;
			double texture_height = large_image_data->images[i].subtex->height * height_factor;
			Draw_texture(&large_image_data->images[i], texture_x_offset, texture_y_offset, texture_width, texture_height);

			//Update offset.
			width_offset += large_image_data->images[i].c2d.tex->width;
			if(width_offset >= large_image_data->image_width)
			{
				width_offset = 0;
				height_offset += large_image_data->images[i].c2d.tex->height;
			}
		}
	}
}

void Vid_fit_to_screen(int screen_width, int screen_height)
{
	if(vid_player.video_info[0].width != 0 && vid_player.video_info[0].height != 0 && vid_player.video_info[0].sar_width != 0 && vid_player.video_info[0].sar_height != 0)
	{
		//fit to screen size
		if((((double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1)) / screen_width) >= (((double)vid_player.video_info[0].height * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1)) / screen_height))
			vid_player.video_zoom = 1.0 / (((double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1)) / screen_width);
		else
			vid_player.video_zoom = 1.0 / (((double)vid_player.video_info[0].height * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1)) / screen_height);

		vid_player.video_x_offset = (screen_width - (vid_player.video_info[0].width * vid_player.video_zoom * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1))) / 2;
		vid_player.video_y_offset = (screen_height - (vid_player.video_info[0].height * vid_player.video_zoom * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1))) / 2;
		vid_player.video_y_offset += 240 - screen_height;
		vid_player.video_x_offset += 400 - screen_width;
	}
	vid_player.subtitle_x_offset = 0;
	vid_player.subtitle_y_offset = 0;
	vid_player.subtitle_zoom = 1;
}

void Vid_change_video_size(double change_px)
{
	double current_width = (double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) * vid_player.video_zoom;
	if(vid_player.video_info[0].width != 0 && vid_player.video_info[0].height != 0 && vid_player.video_info[0].sar_width != 0 && vid_player.video_info[0].sar_height != 0)
		vid_player.video_zoom = 1.0 / ((double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) / (current_width + change_px));
}

void Vid_enter_full_screen(int bottom_screen_timeout)
{
	vid_player.turn_off_bottom_screen_count = bottom_screen_timeout;
	vid_player.is_full_screen = true;
	var_top_lcd_brightness = var_lcd_brightness;
	var_bottom_lcd_brightness = var_lcd_brightness;
}

void Vid_exit_full_screen(void)
{
	vid_player.turn_off_bottom_screen_count = 0;
	vid_player.is_full_screen = false;
	var_turn_on_bottom_lcd = true;
	var_top_lcd_brightness = var_lcd_brightness;
	var_bottom_lcd_brightness = var_lcd_brightness;
}

void Vid_increase_screen_brightness(void)
{
	if(var_lcd_brightness + 1 <= 180)
	{
		var_lcd_brightness++;
		var_top_lcd_brightness = var_lcd_brightness;
		if(!vid_player.is_full_screen)
			var_bottom_lcd_brightness = var_lcd_brightness;

		vid_player.show_screen_brightness_until = osGetTime() + 2500;
	}
}

void Vid_decrease_screen_brightness(void)
{
	if(var_lcd_brightness - 1 >= 0)
	{
		var_lcd_brightness--;
		var_top_lcd_brightness = var_lcd_brightness;
		if(!vid_player.is_full_screen)
			var_bottom_lcd_brightness = var_lcd_brightness;

		vid_player.show_screen_brightness_until = osGetTime() + 2500;
	}
}

void Vid_control_full_screen(void)
{
	if(vid_player.turn_off_bottom_screen_count > 0)
	{
		vid_player.turn_off_bottom_screen_count--;
		if(vid_player.turn_off_bottom_screen_count == 0 && var_model != CFG_MODEL_2DS)
			var_turn_on_bottom_lcd = false;
		if(var_bottom_lcd_brightness > 10 && var_model != CFG_MODEL_2DS)
			var_bottom_lcd_brightness--;
	}
}

void Vid_update_sleep_policy(void)
{
	//Disallow sleep if headset is connected and media is playing.
	if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PAUSE && osIsHeadsetConnected())
	{
		if(aptIsSleepAllowed())
			aptSetSleepAllowed(false);

		return;
	}
	else//Allow sleep if headset is not connected or no media is playing.
	{
		if(!aptIsSleepAllowed())
			aptSetSleepAllowed(true);

		return;
	}
}

void Vid_update_performance_graph(double decoding_time, bool is_key_frame, double* total_delay)
{
	if(vid_player.video_frametime - decoding_time < 0 && vid_player.allow_skip_frames && vid_player.video_frametime != 0)
		*total_delay -= vid_player.video_frametime - decoding_time;

	if(vid_player.decoding_min_time > decoding_time)
		vid_player.decoding_min_time = decoding_time;
	if(vid_player.decoding_max_time < decoding_time)
		vid_player.decoding_max_time = decoding_time;

	vid_player.decoding_total_time += decoding_time;

	for(int i = 1; i < 320; i++)
	{
		vid_player.keyframe_list[i - 1] = vid_player.keyframe_list[i];
		vid_player.video_decoding_time_list[i - 1] = vid_player.video_decoding_time_list[i];
	}

	vid_player.total_frames++;
	vid_player.keyframe_list[319] = is_key_frame;
	vid_player.video_decoding_time_list[319] = decoding_time;
	vid_player.decoding_recent_total_time = 0;

	for(int i = 230; i < 320; i++)
		vid_player.decoding_recent_total_time += vid_player.video_decoding_time_list[i];
}

void Vid_init_variable(void)
{
	Vid_init_hidden_settings();
	Vid_init_debug_view_mode();
	Vid_init_debug_view_data();
	Vid_init_player_data();
	Vid_init_media_data();
	Vid_init_video_data();
	Vid_init_audio_data();
	Vid_init_subtitle_data();
	Vid_init_ui_data();
}

void Vid_init_hidden_settings(void)
{
	bool frame_cores[4] = { false, false, false, false, };
	bool slice_cores[4] = { false, false, false, false, };

	vid_player.show_full_screen_msg = true;
	vid_player.thread_mode = THREAD_TYPE_AUTO;

	if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
	{
		vid_player.num_of_threads = 2;
		frame_cores[0] = true;
		frame_cores[1] = true;
		frame_cores[2] = var_core_2_available;
		frame_cores[3] = var_core_3_available;
		slice_cores[0] = true;
		slice_cores[1] = true;
		slice_cores[3] = var_core_3_available;

		vid_player.num_of_threads += var_core_2_available;
		vid_player.num_of_threads += var_core_3_available;
	}
	else
	{
		vid_player.num_of_threads = 2;
		frame_cores[0] = true;
		frame_cores[1] = true;
		slice_cores[1] = true;
	}
	Util_video_decoder_set_enabled_cores(frame_cores, slice_cores);
}

void Vid_init_debug_view_mode(void)
{
	vid_player.show_decoding_graph = true;
	vid_player.show_color_conversion_graph = true;
	vid_player.show_packet_buffer_graph = true;
	vid_player.show_raw_video_buffer_graph = true;
	vid_player.show_raw_audio_buffer_graph = true;
}

void Vid_init_debug_view_data(void)
{
	vid_player.total_frames = 0;
	vid_player.previous_ts = 0;
	vid_player.decoding_min_time = 0xFFFFFFFF;
	vid_player.decoding_max_time = 0;
	vid_player.decoding_total_time = 0;
	vid_player.decoding_recent_total_time = 0;

	for(int i = 0 ; i < 320; i++)
	{
		vid_player.keyframe_list[i] = 0;
		vid_player.packet_buffer_list[i] = 0;
		vid_player.raw_video_buffer_list[i] = 0;
		vid_player.raw_audio_buffer_list[i] = 0;
		vid_player.video_decoding_time_list[i] = 0;
		vid_player.audio_decoding_time_list[i] = 0;
		vid_player.conversion_time_list[i] = 0;
	}
}

void Vid_init_player_data(void)
{
	vid_player.state = PLAYER_STATE_IDLE;
	vid_player.sub_state = PLAYER_SUB_STATE_NONE;
	memset(vid_player.file.name, 0x0, sizeof(vid_player.file.name));
	memset(vid_player.file.directory, 0x0, sizeof(vid_player.file.directory));
	vid_player.file.index = 0;
}

void Vid_init_media_data(void)
{
	vid_player.media_duration = 0;
	vid_player.media_current_pos = 0;
	vid_player.seek_pos_cache = 0;
	vid_player.seek_pos = 0;
}

void Vid_init_video_data(void)
{
	vid_player.num_of_video_tracks = 0;
	vid_player.image_index = 0;
	vid_player.image_index_3d = 0;
	vid_player.vps = 0;
	vid_player.video_current_pos = 0;
	vid_player.video_x_offset = 0;
	vid_player.video_y_offset = 15;
	vid_player.video_zoom = 1;
	vid_player.video_frametime = 0;

	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		vid_player.video_info[i].width = 0;
		vid_player.video_info[i].height = 0;
		vid_player.video_info[i].codec_width = 0;
		vid_player.video_info[i].codec_height = 0;
		vid_player.video_info[i].framerate = 0;
		vid_player.video_info[i].format_name = "n/a";
		vid_player.video_info[i].short_format_name = "n/a";
		vid_player.video_info[i].duration = 0;
		vid_player.video_info[i].thread_type = THREAD_TYPE_NONE;
		vid_player.video_info[i].sar_width = 1;
		vid_player.video_info[i].sar_height = 1;
		vid_player.video_info[i].pixel_format = PIXEL_FORMAT_INVALID;
	}

	for(int i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
	{
		for(int k = 0; k < 2; k++)
			Vid_large_texture_free(&vid_player.large_image[i][k]);
	}
}

void Vid_init_audio_data(void)
{
	vid_player.num_of_audio_tracks = 0;
	vid_player.selected_audio_track_cache = 0;
	vid_player.selected_audio_track = 0;
	vid_player.audio_current_pos = 0;
	vid_player.last_decoded_audio_pos = 0;

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
	{
		vid_player.audio_info[i].bitrate = 0;
		vid_player.audio_info[i].sample_rate = 0;
		vid_player.audio_info[i].ch = 0;
		vid_player.audio_info[i].format_name = "n/a";
		vid_player.audio_info[i].short_format_name = "n/a";
		vid_player.audio_info[i].duration = 0;
		vid_player.audio_info[i].track_lang = "n/a";
		vid_player.audio_info[i].sample_format = SAMPLE_FORMAT_INVALID;
	}
}

void Vid_init_subtitle_data(void)
{
	vid_player.num_of_subtitle_tracks = 0;
	vid_player.selected_subtitle_track_cache = 0;
	vid_player.selected_subtitle_track = 0;
	vid_player.subtitle_index = 0;
	vid_player.subtitle_x_offset = 0;
	vid_player.subtitle_y_offset = 0;
	vid_player.subtitle_zoom = 1;

	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
	{
		vid_player.subtitle_info[i].format_name = "n/a";
		vid_player.subtitle_info[i].short_format_name = "n/a";
		vid_player.subtitle_info[i].track_lang = "n/a";
	}

	for(int i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)
	{
		vid_player.subtitle_data[i].text = "";
		vid_player.subtitle_data[i].start_time = 0;
		vid_player.subtitle_data[i].end_time = 0;
	}
}

void Vid_init_ui_data(void)
{
	vid_player.is_full_screen = false;
	vid_player.is_pause_for_home_menu = false;
	vid_player.is_selecting_audio_track = false;
	vid_player.is_selecting_subtitle_track = false;
	vid_player.is_setting_volume = false;
	vid_player.is_setting_seek_duration = false;
	vid_player.is_displaying_controls = false;
	vid_player.turn_off_bottom_screen_count = 0;
	vid_player.menu_mode = DEF_VID_MENU_NONE;
	vid_player.lr_held_count = 0;
	vid_player.cpad_held_count = 0;
	vid_player.dpad_held_count = 0;
	vid_player.show_screen_brightness_until = 0;
	vid_player.show_current_pos_until = 0;
	vid_player.ui_y_offset_max = 0;
	vid_player.ui_y_offset = 0;
	vid_player.ui_y_move = 0;
}

void Vid_hid(Hid_info key)
{
	double video_width = 0;

	if(vid_player.is_setting_volume || vid_player.is_setting_seek_duration || (aptShouldJumpToHome() && vid_player.is_pause_for_home_menu))
		return;

	if(Util_err_query_error_show_flag())
		Util_err_main(key);
	else if(Util_expl_query_show_flag())
		Util_expl_main(key);
	else
	{
		if(vid_player.is_pause_for_home_menu)
		{
			vid_player.is_pause_for_home_menu = false;
			//Resume the video.
			DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
			DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
		}

		if(aptShouldJumpToHome())
		{
			vid_player.is_pause_for_home_menu = true;
			//Pause the video.
			DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
			DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)

			while(vid_player.state == PLAYER_STATE_PLAYING)
				Util_sleep(10000);
		}

		if(vid_player.is_full_screen)
		{
			if(key.p_select || key.p_touch || aptShouldJumpToHome())
			{
				Vid_fit_to_screen(400, 225);
				Vid_exit_full_screen();
				vid_player.show_full_screen_msg = false;
			}
			else if(key.p_a)
			{
				if(vid_player.state == PLAYER_STATE_IDLE)//Play the video.
				{
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_PLAY_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
				}
				//If player state is one of them, pause the video.
				//1. State is playing.
				//2. State is buffering and sub state contains resume later flag.
				//3. State is prepare seeking and sub state contains resume later flag.
				//4. State is seeking and sub state contains resume later flag.
				else if(vid_player.state == PLAYER_STATE_PLAYING
				|| ((vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
				|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING)))
				{
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
				}
				//If player state is one of them, resume the video.
				//1. State is pause.
				//2. State is buffering and sub state doesn't contain resume later flag.
				//3. State is prepare seeking and sub state doesn't contain resume later flag.
				//4. State is seeking and sub state doesn't contain resume later flag.
				else if(vid_player.state == PLAYER_STATE_PAUSE
				|| (!(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
				|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING))
				)
				{
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
				}
			}
		}
		else if(vid_player.is_selecting_audio_track)//Select audio track.
		{
			if(key.p_d_down || key.p_c_down)
			{
				if(vid_player.selected_audio_track_cache + 1 < vid_player.num_of_audio_tracks)
					vid_player.selected_audio_track_cache++;
			}
			else if(key.p_d_up || key.p_c_up)
			{
				if(vid_player.selected_audio_track_cache - 1 > -1)
					vid_player.selected_audio_track_cache--;
			}
			else if(Util_hid_is_pressed(key, vid_player.ok_button))
				vid_player.ok_button.selected = true;
			else if(key.p_a || (Util_hid_is_released(key, vid_player.ok_button) && vid_player.ok_button.selected))
			{
				vid_player.is_selecting_audio_track = false;
				//Apply the track selection.
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
				DECODE_THREAD_CHANGE_AUDIO_TRACK_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
			}

			for(int i = 0; i < vid_player.num_of_audio_tracks; i++)
			{
				if(Util_hid_is_pressed(key, vid_player.audio_track_button[i]))
				{
					vid_player.audio_track_button[i].selected = true;
					break;
				}
				else if(Util_hid_is_released(key, vid_player.audio_track_button[i]) && vid_player.audio_track_button[i].selected)
				{
					vid_player.selected_audio_track_cache = i;
					break;
				}
			}
		}
		else if(vid_player.is_selecting_subtitle_track)//Select subtitle.
		{
			if(key.p_d_down || key.p_c_down)
			{
				if(vid_player.selected_subtitle_track_cache + 1 < vid_player.num_of_subtitle_tracks)
					vid_player.selected_subtitle_track_cache++;
			}
			else if(key.p_d_up || key.p_c_up)
			{
				if(vid_player.selected_subtitle_track_cache - 1 > -1)
					vid_player.selected_subtitle_track_cache--;
			}
			else if(Util_hid_is_pressed(key, vid_player.ok_button))
				vid_player.ok_button.selected = true;
			else if(key.p_a || (Util_hid_is_released(key, vid_player.ok_button) && vid_player.ok_button.selected))
			{
				vid_player.is_selecting_subtitle_track = false;
				//Apply the track selection.
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
				DECODE_THREAD_CHANGE_SUBTITLE_TRACK_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
			}

			for(int i = 0; i < vid_player.num_of_subtitle_tracks; i++)
			{
				if(Util_hid_is_pressed(key, vid_player.subtitle_track_button[i]))
				{
					vid_player.subtitle_track_button[i].selected = true;
					break;
				}
				else if(Util_hid_is_released(key, vid_player.subtitle_track_button[i]) && vid_player.subtitle_track_button[i].selected)
				{
					vid_player.selected_subtitle_track_cache = i;
					break;
				}
			}
		}
		else if(vid_player.is_displaying_controls)
		{
			if(Util_hid_is_pressed(key, vid_player.control_button))
				vid_player.control_button.selected = true;
			else if((key.p_a || Util_hid_is_released(key, vid_player.control_button)) && vid_player.control_button.selected)
				vid_player.is_displaying_controls = false;
		}
		else
		{
			if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_0)
			{
				//scroll
				if(vid_player.menu_background.selected)
					vid_player.ui_y_move = key.touch_y_move * var_scroll_speed;
				else if(Util_hid_is_pressed(key, vid_player.scroll_bar))
					vid_player.scroll_bar.selected = true;
				else if(vid_player.scroll_bar.selected)
				{
					if(key.touch_y < 50)
						vid_player.ui_y_offset = 0;
					else if(key.touch_y > 175)
						vid_player.ui_y_offset = vid_player.ui_y_offset_max;
					else
						vid_player.ui_y_offset = vid_player.ui_y_offset_max * (50 - key.touch_y) / -125;
				}
				else if(vid_player.restart_playback_threshold_bar.selected)
				{
					if(key.touch_x <= 12)
						vid_player.restart_playback_threshold = 0;
					else if(key.touch_x >= 302)
						vid_player.restart_playback_threshold = DEF_DECODER_MAX_RAW_IMAGE - 1;
					else
						vid_player.restart_playback_threshold = (DEF_DECODER_MAX_RAW_IMAGE - 1) * (key.touch_x - 12) / 290;
				}
				//audio track button
				else if(Util_hid_is_pressed(key, vid_player.select_audio_track_button))
					vid_player.select_audio_track_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.select_audio_track_button) && vid_player.select_audio_track_button.selected)
					vid_player.is_selecting_audio_track = !vid_player.is_selecting_audio_track;
				else if(!Util_hid_is_held(key, vid_player.select_audio_track_button) && vid_player.select_audio_track_button.selected)
				{
					vid_player.select_audio_track_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//subtitle track button
				else if(Util_hid_is_pressed(key, vid_player.select_subtitle_track_button))
					vid_player.select_subtitle_track_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.select_subtitle_track_button) && vid_player.select_subtitle_track_button.selected)
					vid_player.is_selecting_subtitle_track = !vid_player.is_selecting_subtitle_track;
				else if(!Util_hid_is_held(key, vid_player.select_subtitle_track_button) && vid_player.select_subtitle_track_button.selected)
				{
					vid_player.select_subtitle_track_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//texture filter button
				else if(Util_hid_is_pressed(key, vid_player.texture_filter_button))
					vid_player.texture_filter_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.texture_filter_button) && vid_player.texture_filter_button.selected)
				{
					vid_player.use_linear_texture_filter = !vid_player.use_linear_texture_filter;

					//Update texture filter.
					for(int i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
					{
						for(int k = 0; k < 2; k++)
							Vid_large_texture_set_filter(&vid_player.large_image[i][k], vid_player.use_linear_texture_filter);
					}
				}
				else if(!Util_hid_is_held(key, vid_player.texture_filter_button) && vid_player.texture_filter_button.selected)
				{
					vid_player.texture_filter_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//allow skip frames button
				else if(Util_hid_is_pressed(key, vid_player.allow_skip_frames_button))
					vid_player.allow_skip_frames_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.allow_skip_frames_button) && vid_player.allow_skip_frames_button.selected)
				{
					vid_player.allow_skip_frames = !vid_player.allow_skip_frames;
					if(vid_player.allow_skip_key_frames)
						vid_player.allow_skip_key_frames = false;
				}
				else if(!Util_hid_is_held(key, vid_player.allow_skip_frames_button) && vid_player.allow_skip_frames_button.selected)
				{
					vid_player.allow_skip_frames_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//allow skip key frames button
				else if(Util_hid_is_pressed(key, vid_player.allow_skip_key_frames_button) && vid_player.allow_skip_frames)
					vid_player.allow_skip_key_frames_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.allow_skip_key_frames_button) && vid_player.allow_skip_key_frames_button.selected && vid_player.allow_skip_frames)
					vid_player.allow_skip_key_frames = !vid_player.allow_skip_key_frames;
				else if(!Util_hid_is_held(key, vid_player.allow_skip_key_frames_button) && vid_player.allow_skip_key_frames_button.selected)
				{
					vid_player.allow_skip_key_frames_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//change volume button
				else if(Util_hid_is_pressed(key, vid_player.volume_button))
					vid_player.volume_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.volume_button) && vid_player.volume_button.selected)
					vid_player.is_setting_volume = true;
				else if(!Util_hid_is_held(key, vid_player.volume_button) && vid_player.volume_button.selected)
				{
					vid_player.volume_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//change seek duration button
				else if(Util_hid_is_pressed(key, vid_player.seek_duration_button))
					vid_player.seek_duration_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.seek_duration_button) && vid_player.seek_duration_button.selected)
					vid_player.is_setting_seek_duration = true;
				else if(!Util_hid_is_held(key, vid_player.seek_duration_button) && vid_player.seek_duration_button.selected)
				{
					vid_player.seek_duration_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//correct aspect ratio button
				else if(Util_hid_is_pressed(key, vid_player.correct_aspect_ratio_button))
					vid_player.correct_aspect_ratio_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.correct_aspect_ratio_button) && vid_player.correct_aspect_ratio_button.selected)
				{
					vid_player.correct_aspect_ratio = !vid_player.correct_aspect_ratio;
					Vid_fit_to_screen(400, 225);
				}
				else if(!Util_hid_is_held(key, vid_player.correct_aspect_ratio_button) && vid_player.correct_aspect_ratio_button.selected)
				{
					vid_player.correct_aspect_ratio_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//disable resize and move button
				else if(Util_hid_is_pressed(key, vid_player.move_content_button))
					vid_player.move_content_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.move_content_button) && vid_player.move_content_button.selected)
				{
					if(vid_player.move_content_mode + 1 > DEF_VID_MOVE_SUBTITLE)
						vid_player.move_content_mode = DEF_VID_MOVE_DISABLE;
					else
						vid_player.move_content_mode++;
				}
				else if(!Util_hid_is_held(key, vid_player.move_content_button) && vid_player.move_content_button.selected)
				{
					vid_player.move_content_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//remember video pos button
				else if(Util_hid_is_pressed(key, vid_player.remember_video_pos_button))
					vid_player.remember_video_pos_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.remember_video_pos_button) && vid_player.remember_video_pos_button.selected)
					vid_player.remember_video_pos = !vid_player.remember_video_pos;
				else if(!Util_hid_is_held(key, vid_player.remember_video_pos_button) && vid_player.remember_video_pos_button.selected)
				{
					vid_player.remember_video_pos_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//playback mode button
				else if(Util_hid_is_pressed(key, vid_player.playback_mode_button))
					vid_player.playback_mode_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.playback_mode_button) && vid_player.playback_mode_button.selected)
				{
					if(vid_player.playback_mode + 1 > DEF_VID_RANDOM)
						vid_player.playback_mode = DEF_VID_NO_REPEAT;
					else
						vid_player.playback_mode++;
				}
				else if(!Util_hid_is_held(key, vid_player.playback_mode_button) && vid_player.playback_mode_button.selected)
				{
					vid_player.playback_mode_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//restart playback threshold button
				else if(Util_hid_is_pressed(key, vid_player.restart_playback_threshold_bar))
					vid_player.restart_playback_threshold_bar.selected = true;
				//background
				else if(Util_hid_is_pressed(key, vid_player.menu_background))
					vid_player.menu_background.selected = true;
				//menu mode button
				else if(Util_hid_is_pressed(key, vid_player.menu_button[1]))
					vid_player.menu_button[1].selected = true;
				else if(Util_hid_is_released(key, vid_player.menu_button[1]) && vid_player.menu_button[1].selected)
				{
					vid_player.ui_y_offset_max = -100;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_SETTINGS_1;
				}
				else if(Util_hid_is_pressed(key, vid_player.menu_button[2]))
					vid_player.menu_button[2].selected = true;
				else if(Util_hid_is_released(key, vid_player.menu_button[2]) && vid_player.menu_button[2].selected)
				{
					vid_player.ui_y_offset_max = -65;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_INFO;
				}

				if (vid_player.menu_background.selected || vid_player.ui_y_move != 0)
				{
					if(!vid_player.menu_background.selected)
						vid_player.ui_y_move *= 0.95;

					if(vid_player.ui_y_move < 0.75 && vid_player.ui_y_move > -0.75)
						vid_player.ui_y_move = 0;

					if(vid_player.ui_y_offset - vid_player.ui_y_move > 0)
						vid_player.ui_y_offset = 0;
					else if(vid_player.ui_y_offset - vid_player.ui_y_move < vid_player.ui_y_offset_max)
						vid_player.ui_y_offset = vid_player.ui_y_offset_max;
					else
						vid_player.ui_y_offset -= vid_player.ui_y_move;
				}
			}
			if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_1)
			{
				//Scroll.
				if(vid_player.menu_background.selected)
					vid_player.ui_y_move = key.touch_y_move * var_scroll_speed;
				else if(Util_hid_is_pressed(key, vid_player.scroll_bar))
					vid_player.scroll_bar.selected = true;
				else if(vid_player.scroll_bar.selected)
				{
					if(key.touch_y < 50)
						vid_player.ui_y_offset = 0;
					else if(key.touch_y > 175)
						vid_player.ui_y_offset = vid_player.ui_y_offset_max;
					else
						vid_player.ui_y_offset = vid_player.ui_y_offset_max * (50 - key.touch_y) / -125;
				}

				else if(vid_player.state == PLAYER_STATE_IDLE)
				{
					//Disable audio button.
					if(Util_hid_is_pressed(key, vid_player.disable_audio_button))
						vid_player.disable_audio_button.selected = true;
					else if(Util_hid_is_released(key, vid_player.disable_audio_button) && vid_player.disable_audio_button.selected)
						vid_player.disable_audio = !vid_player.disable_audio;
					else if(!Util_hid_is_held(key, vid_player.disable_audio_button) && vid_player.disable_audio_button.selected)
					{
						vid_player.disable_audio_button.selected = false;
						vid_player.menu_background.selected = true;
					}
					//Disable video button.
					else if(Util_hid_is_pressed(key, vid_player.disable_video_button))
						vid_player.disable_video_button.selected = true;
					else if(Util_hid_is_released(key, vid_player.disable_video_button) && vid_player.disable_video_button.selected)
						vid_player.disable_video = !vid_player.disable_video;
					else if(!Util_hid_is_held(key, vid_player.disable_video_button) && vid_player.disable_video_button.selected)
					{
						vid_player.disable_video_button.selected = false;
						vid_player.menu_background.selected = true;
					}
					//Disable subtitle button.
					else if(Util_hid_is_pressed(key, vid_player.disable_subtitle_button))
						vid_player.disable_subtitle_button.selected = true;
					else if(Util_hid_is_released(key, vid_player.disable_subtitle_button) && vid_player.disable_subtitle_button.selected)
						vid_player.disable_subtitle = !vid_player.disable_subtitle;
					else if(!Util_hid_is_held(key, vid_player.disable_subtitle_button) && vid_player.disable_subtitle_button.selected)
					{
						vid_player.disable_subtitle_button.selected = false;
						vid_player.menu_background.selected = true;
					}
					//Hardware decoding button.
					else if(Util_hid_is_pressed(key, vid_player.use_hw_decoding_button) && !(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL))
						vid_player.use_hw_decoding_button.selected = true;
					else if(Util_hid_is_released(key, vid_player.use_hw_decoding_button) && vid_player.use_hw_decoding_button.selected
					&& !(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL))
						vid_player.use_hw_decoding = !vid_player.use_hw_decoding;
					else if(!Util_hid_is_held(key, vid_player.use_hw_decoding_button) && vid_player.use_hw_decoding_button.selected)
					{
						vid_player.use_hw_decoding_button.selected = false;
						vid_player.menu_background.selected = true;
					}
					//Hardware color conversion button.
					else if(Util_hid_is_pressed(key, vid_player.use_hw_color_conversion_button))
						vid_player.use_hw_color_conversion_button.selected = true;
					else if(Util_hid_is_released(key, vid_player.use_hw_color_conversion_button) && vid_player.use_hw_color_conversion_button.selected)
						vid_player.use_hw_color_conversion = !vid_player.use_hw_color_conversion;
					else if(!Util_hid_is_held(key, vid_player.use_hw_color_conversion_button) && vid_player.use_hw_color_conversion_button.selected)
					{
						vid_player.use_hw_color_conversion_button.selected = false;
						vid_player.menu_background.selected = true;
					}
					//Multi-threaded decoding button.
					else if(Util_hid_is_pressed(key, vid_player.use_multi_threaded_decoding_button))
						vid_player.use_multi_threaded_decoding_button.selected = true;
					else if(Util_hid_is_released(key, vid_player.use_multi_threaded_decoding_button) && vid_player.use_multi_threaded_decoding_button.selected)
						vid_player.use_multi_threaded_decoding = !vid_player.use_multi_threaded_decoding;
					else if(!Util_hid_is_held(key, vid_player.use_multi_threaded_decoding_button) && vid_player.use_multi_threaded_decoding_button.selected)
					{
						vid_player.use_multi_threaded_decoding_button.selected = false;
						vid_player.menu_background.selected = true;
					}
					//Lower video resolution button.
					else if(Util_hid_is_pressed(key, vid_player.lower_resolution_button))
						vid_player.lower_resolution_button.selected = true;
					else if(Util_hid_is_released(key, vid_player.lower_resolution_button) && vid_player.lower_resolution_button.selected)
					{
						if(vid_player.lower_resolution + 1 > 2)
							vid_player.lower_resolution = 0;
						else
							vid_player.lower_resolution++;
					}
					else if(!Util_hid_is_held(key, vid_player.lower_resolution_button) && vid_player.lower_resolution_button.selected)
					{
						vid_player.lower_resolution_button.selected = false;
						vid_player.menu_background.selected = true;
					}
				}
				else//These settings can't be changed while playing a media.
				{
					vid_player.disable_audio_button.selected = false;
					vid_player.disable_video_button.selected = false;
					vid_player.disable_subtitle_button.selected = false;
					vid_player.use_hw_decoding_button.selected = false;
					vid_player.use_hw_color_conversion_button.selected = false;
					vid_player.use_multi_threaded_decoding_button.selected = false;
					vid_player.lower_resolution_button.selected = false;
				}

				//Background.
				if(Util_hid_is_pressed(key, vid_player.menu_background))
				{
					if(!vid_player.disable_audio_button.selected && !vid_player.disable_video_button.selected && !vid_player.disable_subtitle_button.selected
					&& !vid_player.use_hw_decoding_button.selected && !vid_player.use_hw_color_conversion_button.selected
					&& !vid_player.use_multi_threaded_decoding_button.selected && !vid_player.lower_resolution_button.selected)
						vid_player.menu_background.selected = true;
				}
				//Menu mode button.
				else if(Util_hid_is_pressed(key, vid_player.menu_button[0]))
					vid_player.menu_button[0].selected = true;
				else if(Util_hid_is_released(key, vid_player.menu_button[0]) && vid_player.menu_button[0].selected)
				{
					vid_player.ui_y_offset_max = -200;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_SETTINGS_0;
				}
				else if(Util_hid_is_pressed(key, vid_player.menu_button[2]))
					vid_player.menu_button[2].selected = true;
				else if(Util_hid_is_released(key, vid_player.menu_button[2]) && vid_player.menu_button[2].selected)
				{
					vid_player.ui_y_offset_max = -65;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_INFO;
				}

				if (vid_player.menu_background.selected || vid_player.ui_y_move != 0)
				{
					if(!vid_player.menu_background.selected)
						vid_player.ui_y_move *= 0.95;
					
					if(vid_player.ui_y_move < 0.75 && vid_player.ui_y_move > -0.75)
						vid_player.ui_y_move = 0;

					if(vid_player.ui_y_offset - vid_player.ui_y_move > 0)
						vid_player.ui_y_offset = 0;
					else if(vid_player.ui_y_offset - vid_player.ui_y_move < vid_player.ui_y_offset_max)
						vid_player.ui_y_offset = vid_player.ui_y_offset_max;
					else
						vid_player.ui_y_offset -= vid_player.ui_y_move;
				}
			}
			else if(vid_player.menu_mode == DEF_VID_MENU_INFO)
			{
				//scroll
				if(vid_player.menu_background.selected)
					vid_player.ui_y_move = key.touch_y_move * var_scroll_speed;
				else if(Util_hid_is_pressed(key, vid_player.scroll_bar))
					vid_player.scroll_bar.selected = true;
				else if(vid_player.scroll_bar.selected)
				{
					if(key.touch_y < 50)
						vid_player.ui_y_offset = 0;
					else if(key.touch_y > 175)
						vid_player.ui_y_offset = vid_player.ui_y_offset_max;
					else
						vid_player.ui_y_offset = vid_player.ui_y_offset_max * (50 - key.touch_y) / -125;
				}
				//decoding graph button
				else if(Util_hid_is_pressed(key, vid_player.show_decode_graph_button))
					vid_player.show_decode_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.show_decode_graph_button) && vid_player.show_decode_graph_button.selected)
					vid_player.show_decoding_graph = !vid_player.show_decoding_graph;
				else if(!Util_hid_is_held(key, vid_player.show_decode_graph_button) && vid_player.show_decode_graph_button.selected)
				{
					vid_player.show_decode_graph_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//color conversion graph button
				else if(Util_hid_is_pressed(key, vid_player.show_color_conversion_graph_button))
					vid_player.show_color_conversion_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.show_color_conversion_graph_button) && vid_player.show_color_conversion_graph_button.selected)
					vid_player.show_color_conversion_graph = !vid_player.show_color_conversion_graph;
				else if(!Util_hid_is_held(key, vid_player.show_color_conversion_graph_button) && vid_player.show_color_conversion_graph_button.selected)
				{
					vid_player.show_color_conversion_graph_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//packet buffer graph button
				else if(Util_hid_is_pressed(key, vid_player.show_packet_buffer_graph_button))
					vid_player.show_packet_buffer_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.show_packet_buffer_graph_button) && vid_player.show_packet_buffer_graph_button.selected)
					vid_player.show_packet_buffer_graph = !vid_player.show_packet_buffer_graph;
				else if(!Util_hid_is_held(key, vid_player.show_packet_buffer_graph_button) && vid_player.show_packet_buffer_graph_button.selected)
				{
					vid_player.show_packet_buffer_graph_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//raw video buffer graph button
				else if(Util_hid_is_pressed(key, vid_player.show_raw_video_buffer_graph_button))
					vid_player.show_raw_video_buffer_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.show_raw_video_buffer_graph_button) && vid_player.show_raw_video_buffer_graph_button.selected)
					vid_player.show_raw_video_buffer_graph = !vid_player.show_raw_video_buffer_graph;
				else if(!Util_hid_is_held(key, vid_player.show_raw_video_buffer_graph_button) && vid_player.show_raw_video_buffer_graph_button.selected)
				{
					vid_player.show_raw_video_buffer_graph_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//raw audio buffer graph button
				else if(Util_hid_is_pressed(key, vid_player.show_raw_audio_buffer_graph_button))
					vid_player.show_raw_audio_buffer_graph_button.selected = true;
				else if(Util_hid_is_released(key, vid_player.show_raw_audio_buffer_graph_button) && vid_player.show_raw_audio_buffer_graph_button.selected)
					vid_player.show_raw_audio_buffer_graph = !vid_player.show_raw_audio_buffer_graph;
				else if(!Util_hid_is_held(key, vid_player.show_raw_audio_buffer_graph_button) && vid_player.show_raw_audio_buffer_graph_button.selected)
				{
					vid_player.show_raw_audio_buffer_graph_button.selected = false;
					vid_player.menu_background.selected = true;
				}
				//background
				else if(Util_hid_is_pressed(key, vid_player.menu_background))
					vid_player.menu_background.selected = true;
				//menu mode button
				else if(Util_hid_is_pressed(key, vid_player.menu_button[0]))
					vid_player.menu_button[0].selected = true;
				else if(Util_hid_is_released(key, vid_player.menu_button[0]) && vid_player.menu_button[0].selected)
				{
					vid_player.ui_y_offset_max = -200;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_SETTINGS_0;
				}
				else if(Util_hid_is_pressed(key, vid_player.menu_button[1]))
					vid_player.menu_button[1].selected = true;
				else if(Util_hid_is_released(key, vid_player.menu_button[1]) && vid_player.menu_button[1].selected)
				{
					vid_player.ui_y_offset_max = -100;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_SETTINGS_1;
				}

				if (vid_player.menu_background.selected || vid_player.ui_y_move != 0)
				{
					if(!vid_player.menu_background.selected)
						vid_player.ui_y_move *= 0.95;
					
					if(vid_player.ui_y_move < 0.75 && vid_player.ui_y_move > -0.75)
						vid_player.ui_y_move = 0;

					if(vid_player.ui_y_offset - vid_player.ui_y_move > 0)
						vid_player.ui_y_offset = 0;
					else if(vid_player.ui_y_offset - vid_player.ui_y_move < vid_player.ui_y_offset_max)
						vid_player.ui_y_offset = vid_player.ui_y_offset_max;
					else
						vid_player.ui_y_offset -= vid_player.ui_y_move;
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
				if(vid_player.state == PLAYER_STATE_IDLE)//Play the video.
				{
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_PLAY_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
				}
				//If player state is one of them, pause the video.
				//1. State is playing.
				//2. State is buffering and sub state contains resume later flag.
				//3. State is prepare seeking and sub state contains resume later flag.
				//4. State is seeking and sub state contains resume later flag.
				else if(vid_player.state == PLAYER_STATE_PLAYING
				|| ((vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
				|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING)))
				{
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
				}
				//If player state is one of them, resume the video.
				//1. State is pause.
				//2. State is buffering and sub state doesn't contain resume later flag.
				//3. State is prepare seeking and sub state doesn't contain resume later flag.
				//4. State is seeking and sub state doesn't contain resume later flag.
				else if(vid_player.state == PLAYER_STATE_PAUSE
				|| (!(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER) && (vid_player.state == PLAYER_STATE_BUFFERING
				|| vid_player.state == PLAYER_STATE_PREPARE_SEEKING || vid_player.state == PLAYER_STATE_SEEKING))
				)
				{
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
				}
			}
			else if(key.p_b)//Abort the playback.
			{
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
				DECODE_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_SEND_TO_FRONT)
			}
			else if(key.p_x)
			{
				Util_expl_set_show_flag(true);
				Util_expl_set_callback(Vid_callback);
				Util_expl_set_cancel_callback(Vid_cancel);
			}
			else if(key.p_y)
			{
				if(vid_player.menu_mode == DEF_VID_MENU_NONE)
				{
					vid_player.ui_y_offset_max = -200;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_SETTINGS_0;
				}
				else
				{
					vid_player.ui_y_offset_max = 0;
					vid_player.ui_y_offset = 0;
					vid_player.menu_mode = DEF_VID_MENU_NONE;
				}
			}
			else if(Util_hid_is_pressed(key, vid_player.control_button))
				vid_player.control_button.selected = true;
			else if(Util_hid_is_released(key, vid_player.control_button) && vid_player.control_button.selected)
				vid_player.is_displaying_controls = true;
			else if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PREPATE_PLAYING)
			{
				if(Util_hid_is_pressed(key, vid_player.seek_bar))
					vid_player.seek_bar.selected = true;
				else if(key.h_touch && vid_player.seek_bar.selected)
				{
					vid_player.seek_pos_cache = vid_player.media_duration * (((double)key.touch_x - 5) / 310);
					var_need_reflesh = true;
				}
				else if(key.r_touch && vid_player.seek_bar.selected)
				{
					vid_player.seek_pos = vid_player.media_duration * (((double)key.touch_x - 5) / 310);

					//Seek the video.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
				}
			}

			if((key.p_c_down || key.p_c_up || key.p_c_right || key.p_c_left || key.h_c_down || key.h_c_up || key.h_c_right || key.h_c_left)
			&& vid_player.move_content_mode)
			{
				if(vid_player.move_content_mode == DEF_VID_MOVE_VIDEO || vid_player.move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.p_c_down)
						vid_player.video_y_offset -= 1 * var_scroll_speed;
					else if(key.h_c_down)
					{
						if(!vid_player.is_selecting_audio_track && !vid_player.is_selecting_subtitle_track)
						{
							if(vid_player.cpad_held_count > 600)
								vid_player.video_y_offset -= 10 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 240)
								vid_player.video_y_offset -= 7.5 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 5)
								vid_player.video_y_offset -= 5 * var_scroll_speed;
						}
					}

					if(key.p_c_up)
						vid_player.video_y_offset += 1 * var_scroll_speed;
					else if(key.h_c_up)
					{
						if(!vid_player.is_selecting_audio_track && !vid_player.is_selecting_subtitle_track)
						{
							if(vid_player.cpad_held_count > 600)
								vid_player.video_y_offset += 10 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 240)
								vid_player.video_y_offset += 7.5 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 5)
								vid_player.video_y_offset += 5 * var_scroll_speed;
						}
					}

					if(key.p_c_right)
						vid_player.video_x_offset -= 1 * var_scroll_speed;
					else if(key.h_c_right)
					{
						if(vid_player.cpad_held_count > 600)
							vid_player.video_x_offset -= 10 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 240)
							vid_player.video_x_offset -= 7.5 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 5)
							vid_player.video_x_offset -= 5 * var_scroll_speed;
					}

					if(key.p_c_left)
						vid_player.video_x_offset += 1 * var_scroll_speed;
					else if(key.h_c_left)
					{
						if(vid_player.cpad_held_count > 600)
							vid_player.video_x_offset += 10 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 240)
							vid_player.video_x_offset += 7.5 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 5)
							vid_player.video_x_offset += 5 * var_scroll_speed;
					}

					if(vid_player.video_x_offset > 400)
						vid_player.video_x_offset = 400;
					else if(vid_player.video_x_offset < -vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) * vid_player.video_zoom)
						vid_player.video_x_offset = -vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) * vid_player.video_zoom;

					if(vid_player.video_y_offset > 480)
						vid_player.video_y_offset = 480;
					else if(vid_player.video_y_offset < -vid_player.video_info[0].height * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1) * vid_player.video_zoom)
						vid_player.video_y_offset = -vid_player.video_info[0].height * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_height : 1) * vid_player.video_zoom;
				}
				if(vid_player.move_content_mode == DEF_VID_MOVE_SUBTITLE || vid_player.move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.p_c_down)
						vid_player.subtitle_y_offset -= 1 * var_scroll_speed;
					else if(key.h_c_down)
					{
						if(!vid_player.is_selecting_audio_track && !vid_player.is_selecting_subtitle_track)
						{
							if(vid_player.cpad_held_count > 600)
								vid_player.subtitle_y_offset -= 10 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 240)
								vid_player.subtitle_y_offset -= 7.5 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 5)
								vid_player.subtitle_y_offset -= 5 * var_scroll_speed;
						}
					}

					if(key.p_c_up)
						vid_player.subtitle_y_offset += 1 * var_scroll_speed;
					else if(key.h_c_up)
					{
						if(!vid_player.is_selecting_audio_track && !vid_player.is_selecting_subtitle_track)
						{
							if(vid_player.cpad_held_count > 600)
								vid_player.subtitle_y_offset += 10 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 240)
								vid_player.subtitle_y_offset += 7.5 * var_scroll_speed;
							else if(vid_player.cpad_held_count > 5)
								vid_player.subtitle_y_offset += 5 * var_scroll_speed;
						}
					}

					if(key.p_c_right)
						vid_player.subtitle_x_offset -= 1 * var_scroll_speed;
					else if(key.h_c_right)
					{
						if(vid_player.cpad_held_count > 600)
							vid_player.subtitle_x_offset -= 10 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 240)
							vid_player.subtitle_x_offset -= 7.5 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 5)
							vid_player.subtitle_x_offset -= 5 * var_scroll_speed;
					}

					if(key.p_c_left)
						vid_player.subtitle_x_offset += 1 * var_scroll_speed;
					else if(key.h_c_left)
					{
						if(vid_player.cpad_held_count > 600)
							vid_player.subtitle_x_offset += 10 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 240)
							vid_player.subtitle_x_offset += 7.5 * var_scroll_speed;
						else if(vid_player.cpad_held_count > 5)
							vid_player.subtitle_x_offset += 5 * var_scroll_speed;
					}

					if(vid_player.subtitle_x_offset > 400)
						vid_player.subtitle_x_offset = 400;
					else if(vid_player.subtitle_x_offset < -vid_player.video_info[0].codec_width * vid_player.video_zoom)
						vid_player.subtitle_x_offset = -vid_player.video_info[0].codec_width * vid_player.video_zoom;

					if(vid_player.subtitle_y_offset > 480)
						vid_player.subtitle_y_offset = 480;
					else if(vid_player.subtitle_y_offset < -vid_player.video_info[0].codec_height * vid_player.video_zoom)
						vid_player.subtitle_y_offset = -vid_player.video_info[0].codec_height * vid_player.video_zoom;
				}

				vid_player.cpad_held_count++;
			}
			else
				vid_player.cpad_held_count = 0;

			if((key.p_l || key.p_r || key.h_l || key.h_r) && vid_player.move_content_mode)
			{
				if(vid_player.move_content_mode == DEF_VID_MOVE_VIDEO || vid_player.move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.h_l || key.p_l)
					{
						if(vid_player.lr_held_count > 360)
							Vid_change_video_size(-5);
						else if(vid_player.lr_held_count > 120)
							Vid_change_video_size(-3);
						else if(vid_player.lr_held_count > 5)
							Vid_change_video_size(-1);
					}

					if(key.h_r || key.p_r)
					{
						if(vid_player.lr_held_count > 360)
							Vid_change_video_size(5);
						else if(vid_player.lr_held_count > 120)
							Vid_change_video_size(3);
						else if(vid_player.lr_held_count > 5)
							Vid_change_video_size(1);
					}

					video_width = (double)vid_player.video_info[0].width * (vid_player.correct_aspect_ratio ? vid_player.video_info[0].sar_width : 1) * vid_player.video_zoom;
					//If video is too large or small, don't enlarge or make it small anymore.
					if(video_width < 20)
					{
						if(vid_player.lr_held_count > 360)
							Vid_change_video_size(5);
						else if(vid_player.lr_held_count > 120)
							Vid_change_video_size(3);
						else if(vid_player.lr_held_count > 5)
							Vid_change_video_size(1);
					}
					else if(video_width > 2000)
					{
						if(vid_player.lr_held_count > 360)
							Vid_change_video_size(-5);
						else if(vid_player.lr_held_count > 120)
							Vid_change_video_size(-3);
						else if(vid_player.lr_held_count > 5)
							Vid_change_video_size(-1);
					}
				}
				if(vid_player.move_content_mode == DEF_VID_MOVE_SUBTITLE || vid_player.move_content_mode == DEF_VID_MOVE_BOTH)
				{
					if(key.h_l || key.p_l)
					{
						if(vid_player.lr_held_count > 360)
							vid_player.subtitle_zoom -= 0.05 * var_scroll_speed;
						else if(vid_player.lr_held_count > 120)
							vid_player.subtitle_zoom -= 0.01 * var_scroll_speed;
						else if(vid_player.lr_held_count > 5)
							vid_player.subtitle_zoom -= 0.005 * var_scroll_speed;
					}

					if(key.h_r || key.p_r)
					{
						if(vid_player.lr_held_count > 360)
							vid_player.subtitle_zoom += 0.05 * var_scroll_speed;
						else if(vid_player.lr_held_count > 120)
							vid_player.subtitle_zoom += 0.01 * var_scroll_speed;
						else if(vid_player.lr_held_count > 5)
							vid_player.subtitle_zoom += 0.005 * var_scroll_speed;
					}

					if(vid_player.subtitle_zoom < 0.05)
						vid_player.subtitle_zoom = 0.05;
					else if(vid_player.subtitle_zoom > 10)
						vid_player.subtitle_zoom = 10;
				}

				vid_player.lr_held_count++;
			}
			else
				vid_player.lr_held_count = 0;
		}

		if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PREPATE_PLAYING)
		{
			if(key.p_d_right && !vid_player.seek_bar.selected)
			{
				double current_bar_pos = 0;
				if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					current_bar_pos = vid_player.seek_pos;
				else
					current_bar_pos = vid_player.media_current_pos;

				if(current_bar_pos + (vid_player.seek_duration * 1000) > vid_player.media_duration)
					vid_player.seek_pos = vid_player.media_duration;
				else
					vid_player.seek_pos = current_bar_pos + (vid_player.seek_duration * 1000);

				//Seek the video.
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
				DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
			}
			else if(key.p_d_left && !vid_player.seek_bar.selected)
			{
				double current_bar_pos = 0;
				if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					current_bar_pos = vid_player.seek_pos;
				else
					current_bar_pos = vid_player.media_current_pos;

				if(current_bar_pos - (vid_player.seek_duration * 1000) < 0)
					vid_player.seek_pos = 0;
				else
					vid_player.seek_pos = current_bar_pos - (vid_player.seek_duration * 1000);

				//Seek the video.
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_HID_CALLBACK_STR, &vid_player.decode_thread_command_queue,
				DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
			}
		}

		//Control screen brightness.
		if(key.p_d_up || key.h_d_up || key.p_d_down || key.h_d_down)
		{
			if(vid_player.dpad_held_count % 15 == 0 || (vid_player.dpad_held_count >= 75 && vid_player.dpad_held_count % 5 == 0)
			|| vid_player.dpad_held_count >= 180)
			{
				if(key.p_d_up || key.h_d_up)
					Vid_increase_screen_brightness();
				else if(key.p_d_down || key.h_d_down)
					Vid_decrease_screen_brightness();
			}

			vid_player.dpad_held_count++;
		}
		else
			vid_player.dpad_held_count = 0;

		if(!key.p_touch && !key.h_touch)
		{
			vid_player.select_audio_track_button.selected = vid_player.texture_filter_button.selected = vid_player.allow_skip_frames_button.selected
			= vid_player.allow_skip_key_frames_button.selected = vid_player.volume_button.selected = vid_player.seek_duration_button.selected
			= vid_player.use_hw_decoding_button.selected = vid_player.use_hw_color_conversion_button.selected = vid_player.use_multi_threaded_decoding_button.selected
			= vid_player.lower_resolution_button.selected = vid_player.menu_button[0].selected = vid_player.menu_button[1].selected = vid_player.menu_button[2].selected 
			= vid_player.control_button.selected = vid_player.ok_button.selected = vid_player.correct_aspect_ratio_button.selected 
			= vid_player.move_content_button.selected = vid_player.remember_video_pos_button.selected = Draw_get_bot_ui_button()->selected
			= vid_player.show_decode_graph_button.selected = vid_player.show_color_conversion_graph_button.selected = vid_player.show_packet_buffer_graph_button.selected
			= vid_player.show_raw_video_buffer_graph_button.selected = vid_player.show_raw_audio_buffer_graph_button.selected = vid_player.menu_background.selected 
			= vid_player.scroll_bar.selected = vid_player.playback_mode_button.selected = vid_player.select_subtitle_track_button.selected 
			= vid_player.restart_playback_threshold_bar.selected = vid_player.seek_bar.selected = false;

			for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
				vid_player.audio_track_button[i].selected = false;

			for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
				vid_player.subtitle_track_button[i].selected = false;
		}
	}
	
	if(Util_log_query_log_show_flag())
		Util_log_main(key);
}

void Vid_decode_thread(void* arg)
{
	Util_log_save(DEF_VID_DECODE_THREAD_STR, "Thread started.");

	bool is_eof = false;
	bool is_read_packet_thread_active = false;
	bool is_waiting_video_decoder = false;
	int backward_timeout = 20;
	int wait_count = 0;
	double seek_start_pos = 0;
	u8 dummy = 0;
	std::string cache_file_name = "";
	std::string bar_pos = "";
	Result_with_string result;

	Util_file_save_to_file(".", DEF_MAIN_DIR + "saved_pos/", &dummy, 1, true);//Create directory.

	while (vid_thread_run)
	{
		u64 timeout_us = DEF_ACTIVE_THREAD_SLEEP_TIME;
		void* message = NULL;
		Vid_command event = NONE_REQUEST;
		Vid_notification notification = NONE_NOTIFICATION;

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
		}

		//If player state is not idle, don't wait for commands
		//as we want to decode audio/videos/subtitles as quick as possible.
		if(vid_player.state != PLAYER_STATE_IDLE)
			timeout_us = 0;

		//Reset afk count so that system won't go sleep.
		if(vid_player.state != PLAYER_STATE_IDLE && vid_player.state != PLAYER_STATE_PAUSE)
			var_afk_time = 0;

		result = Util_queue_get(&vid_player.decode_thread_command_queue, (u32*)&event, &message, timeout_us);
		if(result.code == 0)
		{
			switch (event)
			{
				case DECODE_THREAD_PLAY_REQUEST:
				{
					Vid_file* new_file = (Vid_file*)message;

					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
					{
						int num_of_audio_tracks = 0;
						int num_of_video_tracks = 0;
						int num_of_subtitle_tracks = 0;
						std::string path = "";

						Vid_init_debug_view_data();
						Vid_init_media_data();
						Vid_init_video_data();
						Vid_init_audio_data();
						Vid_init_subtitle_data();
						vid_player.state = PLAYER_STATE_PLAYING;
						vid_player.sub_state = PLAYER_SUB_STATE_NONE;
						is_eof = false;
						is_read_packet_thread_active = false;
						is_waiting_video_decoder = false;

						//Update the file.
						if(new_file)
						{
							snprintf(vid_player.file.name, sizeof(vid_player.file.name), "%s", new_file->name);
							snprintf(vid_player.file.directory, sizeof(vid_player.file.directory), "%s", new_file->directory);
							vid_player.file.index = new_file->index;

							free(new_file);
							new_file = NULL;
						}

						seek_start_pos = 0;
						wait_count = 0;
						cache_file_name = "";
						path = (std::string)vid_player.file.directory + vid_player.file.name;
						bar_pos = "";

						//Generate cache file name.
						for(int i = 0; i < (int)path.length(); i++)
						{
							if(path.substr(i, 1) == "/")
								cache_file_name += "_";
							else
								cache_file_name += path.substr(i, 1);
						}

						result = Util_decoder_open_file(path, &num_of_audio_tracks, &num_of_video_tracks, &num_of_subtitle_tracks, 0);
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_open_file()..." + result.string + result.error_description, result.code);
						if(result.code != 0)
							goto error;

						//Overwirte number of tracks if disable flag is set.
						if(vid_player.disable_audio)
						{
							num_of_audio_tracks = 0;
							vid_player.audio_info[0].format_name = "disabled";
						}
						if(vid_player.disable_video)
						{
							num_of_video_tracks = 0;
							vid_player.video_info[0].format_name = "disabled";
						}
						if(vid_player.disable_subtitle)
						{
							num_of_subtitle_tracks = 0;
							vid_player.subtitle_info[0].format_name = "disabled";
						}

						if(num_of_audio_tracks > 0)
						{
							result = Util_speaker_init();
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_speaker_init()..." + result.string + result.error_description, result.code);
							if(result.code != 0)
							{
								Util_err_set_error_message(result.string, "You have to run dsp1 in order to listen to audio.\n(https://github.com/zoogie/DSP1/releases)", DEF_VID_DECODE_THREAD_STR, result.code);
								Util_err_set_error_show_flag(true);
							}

							result = Util_audio_decoder_init(num_of_audio_tracks, 0);
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_audio_decoder_init()..." + result.string + result.error_description, result.code);

							if(result.code == 0)
							{
								for(int i = 0; i < num_of_audio_tracks; i++)
									Util_audio_decoder_get_info(&vid_player.audio_info[i], i, 0);

								vid_player.selected_audio_track = 0;

								//3DS only supports up to 2ch.
								result = Util_speaker_set_audio_info(0, (vid_player.audio_info[vid_player.selected_audio_track].ch > 2 ? 2 : vid_player.audio_info[vid_player.selected_audio_track].ch), vid_player.audio_info[vid_player.selected_audio_track].sample_rate);
								Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_speaker_set_audio_info()..." + result.string + result.error_description, result.code);
							}

							if(result.code != 0)//If audio format is not supported, disable audio so that video can be played without audio.
							{
								Util_speaker_exit();
								vid_player.num_of_audio_tracks = 0;
								vid_player.audio_info[0].format_name = "Unsupported format";
								//Ignore the error.
								result.code = 0;
							}
							else
								vid_player.num_of_audio_tracks = num_of_audio_tracks;
						}

						if(num_of_video_tracks > 0)
						{
							int request_threads = vid_player.use_multi_threaded_decoding ? vid_player.num_of_threads : 1;
							int loop = (num_of_video_tracks > 2 ? 2 : num_of_video_tracks);
							Multi_thread_type request_thread_type = vid_player.use_multi_threaded_decoding ? vid_player.thread_mode : THREAD_TYPE_NONE;

							result = Util_video_decoder_init(vid_player.lower_resolution, num_of_video_tracks, request_threads, request_thread_type, 0);
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_video_decoder_init()..." + result.string + result.error_description, result.code);

							if(result.code == 0)
							{
								for(int i = 0; i < num_of_video_tracks; i++)
									Util_video_decoder_get_info(&vid_player.video_info[i], i, 0);

								//Use sar 1:2 if 800x240 and no sar value is set.
								if(vid_player.video_info[0].width == 800 && vid_player.video_info[0].height == 240
								&& vid_player.video_info[0].sar_width == 1 && vid_player.video_info[0].sar_height == 1)
									vid_player.video_info[0].sar_height = 2;

								if(vid_player.video_info[0].framerate == 0)
									vid_player.video_frametime = 0;
								else
									vid_player.video_frametime = (1000.0 / vid_player.video_info[0].framerate);

								if(num_of_video_tracks >= 2 && vid_player.video_frametime != 0)
									vid_player.video_frametime /= 2;

								if(vid_player.use_hw_decoding && vid_player.video_info[0].format_name == "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10"
								&& vid_player.video_info[0].pixel_format == PIXEL_FORMAT_YUV420P)
								{
									//We can use hw decoding for this video.
									vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_HW_DECODING);

									result = Util_mvd_video_decoder_init(0);
									Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_mvd_video_decoder_init()..." + result.string + result.error_description, result.code);
									if(result.code != 0)
										goto error;
								}

								if(!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) && vid_player.use_hw_color_conversion)
								{
									if(vid_player.video_info[0].codec_width <= 1024 && vid_player.video_info[0].codec_height <= 1024
									&& (vid_player.video_info[0].pixel_format == PIXEL_FORMAT_YUV420P || vid_player.video_info[0].pixel_format == PIXEL_FORMAT_YUVJ420P))
									{
										//We can use hw color converter for this video.
										vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_HW_CONVERSION);

										result = Util_converter_y2r_init();
										Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_converter_y2r_init()..." + result.string + result.error_description, result.code);
										if(result.code != 0)
											goto error;
									}
								}

								//Allocate texture buffers.
								for(int i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
								{
									for(int k = 0; k < loop; k++)
									{
										result = Vid_large_texture_init(&vid_player.large_image[i][k], vid_player.video_info[k].codec_width, vid_player.video_info[k].codec_height, PIXEL_FORMAT_RGB565LE, true);
										if(result.code != 0)
										{
											result.error_description = "[Error] Vid_large_texture_init() failed. ";
											goto error;
										}
									}
								}

								//Apply texture filter.
								for(int i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
								{
									for(int k = 0; k < 2; k++)
										Vid_large_texture_set_filter(&vid_player.large_image[i][k], vid_player.use_linear_texture_filter);
								}

								if((var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
								&& (vid_player.video_info[0].thread_type == THREAD_TYPE_NONE || (vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)))
									APT_SetAppCpuTimeLimit(20);
								else
								{
									if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
										APT_SetAppCpuTimeLimit(80);
									else
										APT_SetAppCpuTimeLimit(70);
								}

								vid_player.num_of_video_tracks = num_of_video_tracks;
							}
							else//If video format is not supported, disable video so that audio can be played without video.
							{
								vid_player.num_of_video_tracks = 0;
								vid_player.video_info[0].format_name = "Unsupported format";
								//Ignore the error.
								result.code = 0;
							}
						}

						if(num_of_subtitle_tracks > 0)
						{
							result = Util_subtitle_decoder_init(num_of_subtitle_tracks, 0);
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_subtitle_decoder_init()..." + result.string + result.error_description, result.code);

							if(result.code == 0)
							{
								for(int i = 0; i < num_of_subtitle_tracks; i++)
									Util_subtitle_decoder_get_info(&vid_player.subtitle_info[i], i, 0);

								vid_player.selected_subtitle_track = 0;
								vid_player.num_of_subtitle_tracks = num_of_subtitle_tracks;
							}
							else//If subtitle format is not supported, disable subtitle so that audio or video can be played without subtitle.
							{
								vid_player.num_of_subtitle_tracks = 0;
								vid_player.subtitle_info[0].format_name = "Unsupported format";
								//Ignore the error.
								result.code = 0;
							}
						}

						//Use the longest duration as duration for this file.
						if(num_of_video_tracks > 0)
							vid_player.media_duration = vid_player.video_info[0].duration * 1000;
						if(num_of_audio_tracks > 0 && vid_player.audio_info[vid_player.selected_audio_track].duration * 1000 > vid_player.media_duration)
							vid_player.media_duration = vid_player.audio_info[vid_player.selected_audio_track].duration * 1000;

						//Can't play subtitle alone.
						if(num_of_audio_tracks == 0 && num_of_video_tracks == 0)
						{
							result.code = DEF_ERR_OTHER;
							result.string = DEF_ERR_OTHER_STR;
							result.error_description = "[Error] No playable media. ";
							goto error;
						}

						//Enter full screen mode if file has video tracks.
						if(num_of_video_tracks > 0 && !Util_err_query_error_show_flag() && !Util_expl_query_show_flag())
						{
							Vid_fit_to_screen(400, 240);
							if(!vid_player.is_full_screen)
								Vid_enter_full_screen(300);
						}
						else
						{
							Vid_fit_to_screen(400, 225);
							Vid_exit_full_screen();
						}

						//If remember video pos is set, try to find previous playback position.
						if(vid_player.remember_video_pos)
						{
							u8* saved_data = NULL;
							u32 read_size = 0;

							result = Util_file_load_from_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/", &saved_data, sizeof(double), &read_size);
							if(result.code == 0 && read_size >= sizeof(sizeof(double)))
							{
								double saved_pos = *(double*)saved_data;
								if(saved_pos > 0 && saved_pos < vid_player.media_duration)
								{
									vid_player.seek_pos = saved_pos;
									DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_thread_command_queue,
									DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
								}
							}

							Util_safe_linear_free(saved_data);
							saved_data = NULL;
						}

						//If we have video tracks, buffer some data before starting/resuming playback.
						//If there is seek event in queue, don't do it because it will automatically
						//enter buffering state after seeking.
						if(!Util_queue_check_event_exist(&vid_player.decode_thread_command_queue, DECODE_THREAD_SEEK_REQUEST)
						&& num_of_video_tracks > 0 && vid_player.video_frametime != 0)
						{
							//Pause the playback.
							Util_speaker_pause(0);
							//Add resume later bit.
							vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
							vid_player.state = PLAYER_STATE_BUFFERING;
						}

						//Start reading packets.
						is_read_packet_thread_active = true;
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.read_packet_thread_command_queue,
						READ_PACKET_THREAD_READ_PACKET_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)

						//Start converting color, this thread keeps running unless we send abort request.
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.convert_thread_command_queue,
						CONVERT_THREAD_CONVERT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)

						if(num_of_video_tracks == 0 || vid_player.video_frametime == 0)
							Util_add_watch(&bar_pos);

						break;

						error:
						//An error occurred, reset everything.
						Util_err_set_error_message(result.string, result.error_description, DEF_VID_DECODE_THREAD_STR, result.code);
						Util_err_set_error_show_flag(true);
						var_need_reflesh = true;

						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_thread_command_queue,
						DECODE_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_SEND_TO_FRONT)

						continue;
					}
					else
					{
						//If currently player state is not idle, abort current playback first.
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_thread_command_queue,
						DECODE_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)

						//Then play new one, pass the received new_file again.
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_thread_command_queue,
						DECODE_THREAD_PLAY_REQUEST, new_file, 100000, QUEUE_OPTION_NONE)
					}

					break;
				}

				case DECODE_THREAD_PAUSE_REQUEST:
				{
					//Do nothing if player state is idle, prepare playing or pause.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
					|| vid_player.state == PLAYER_STATE_PAUSE)
						break;

					if(vid_player.state == PLAYER_STATE_PLAYING)
					{
						Util_speaker_pause(0);
						vid_player.state = PLAYER_STATE_PAUSE;
					}
					else
					{
						//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);
					}

					break;
				}

				case DECODE_THREAD_RESUME_REQUEST:
				{
					//Do nothing if player state is idle, prepare playing or playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
					|| vid_player.state == PLAYER_STATE_PLAYING)
						break;

					if(vid_player.state == PLAYER_STATE_PAUSE)
					{
						Util_speaker_resume(0);
						vid_player.state = PLAYER_STATE_PLAYING;
					}
					else
					{
						//Add resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
					}

					break;
				}

				case DECODE_THREAD_SEEK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					if(vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					{
						//Currently threre is on going seek request, retry later.
						//Add seek request if threre is no another seek request.
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_thread_command_queue,
						DECODE_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)

						Util_sleep(1000);
						break;
					}

					if(vid_player.state == PLAYER_STATE_PLAYING)//Add resume later bit.
					{
						Util_speaker_pause(0);
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
					}
					else if(vid_player.state == PLAYER_STATE_PAUSE)//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

					//Handle seek request.
					vid_player.state = PLAYER_STATE_PREPARE_SEEKING;
					vid_player.show_current_pos_until = U64_MAX;
					seek_start_pos = vid_player.media_current_pos;

					if(seek_start_pos > vid_player.seek_pos)//Add seek backward wait bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);
					else//Remove seek backward wait bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);

					Util_speaker_clear_buffer(0);

					//Reset EOF flag.
					is_eof = false;

					//Seek the video.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.read_packet_thread_command_queue,
					READ_PACKET_THREAD_SEEK_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)

					break;
				}

				case DECODE_THREAD_CHANGE_AUDIO_TRACK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					//Change audio track.
					if(!vid_player.is_selecting_audio_track && vid_player.selected_audio_track != vid_player.selected_audio_track_cache
					&& vid_player.selected_audio_track_cache < vid_player.num_of_audio_tracks)
					{
						vid_player.selected_audio_track = vid_player.selected_audio_track_cache;
						Util_speaker_clear_buffer(0);
						//3DS only supports up to 2ch.
						Util_speaker_set_audio_info(0, (vid_player.audio_info[vid_player.selected_audio_track].ch > 2 ? 2 : vid_player.audio_info[vid_player.selected_audio_track].ch), vid_player.audio_info[vid_player.selected_audio_track].sample_rate);

						//Use the longest duration as duration for this file.
						if(vid_player.num_of_video_tracks > 0)
							vid_player.media_duration = vid_player.video_info[0].duration * 1000;
						if(vid_player.num_of_audio_tracks > 0 && vid_player.audio_info[vid_player.selected_audio_track].duration * 1000 > vid_player.media_duration)
							vid_player.media_duration = vid_player.audio_info[vid_player.selected_audio_track].duration * 1000;
					}

					break;
				}

				case DECODE_THREAD_CHANGE_SUBTITLE_TRACK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					//Change subtitle track.
					if(!vid_player.is_selecting_subtitle_track && vid_player.selected_subtitle_track != vid_player.selected_subtitle_track_cache
					&& vid_player.selected_subtitle_track_cache < vid_player.num_of_subtitle_tracks)
					{
						vid_player.selected_subtitle_track = vid_player.selected_subtitle_track_cache;
						for(int i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)
						{
							vid_player.subtitle_data[i].text = "";
							vid_player.subtitle_data[i].start_time = 0;
							vid_player.subtitle_data[i].end_time = 0;
						}
					}

					break;
				}

				case DECODE_THREAD_PLAY_NEXT_REQUEST:
				case DECODE_THREAD_ABORT_REQUEST:
				case DECODE_THREAD_SHUTDOWN_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
					{
						vid_player.state = PLAYER_STATE_IDLE;
						vid_player.sub_state = PLAYER_SUB_STATE_NONE;

						if(event == DECODE_THREAD_SHUTDOWN_REQUEST)
						{
							//Exit the threads.
							vid_thread_run = false;
							continue;
						}

						break;
					}

					if(vid_player.remember_video_pos)
					{
						if(event == DECODE_THREAD_PLAY_NEXT_REQUEST)//If playback has finished, remove saved time.
							Util_file_delete_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/");
						else//If remember video pos is set, save playback position.
						{
							double saved_pos = vid_player.media_current_pos;
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "last pos : " + Util_convert_seconds_to_time(saved_pos / 1000));
							Util_file_save_to_file(cache_file_name, DEF_MAIN_DIR + "saved_pos/", (u8*)(&saved_pos), sizeof(double), true);
						}
					}

					//Wait for read packet thread (also flush queues).
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.read_packet_thread_command_queue,
					READ_PACKET_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE);
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_thread_notification_queue, (u32*)&notification, NULL, 100000);
						if(result.code == 0 && notification == READ_PACKET_THREAD_FINISHED_ABORTING_NOTIFICATION)
							break;
					}

					//Wait for convert thread (also flush queues).
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.convert_thread_command_queue,
					CONVERT_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE);
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_thread_notification_queue, (u32*)&notification, NULL, 100000);
						if(result.code == 0 && notification == CONVERT_THREAD_FINISHED_ABORTING_NOTIFICATION)
							break;
					}

					//Wait for video decoding thread (also flush queues).
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_video_thread_command_queue,
					DECODE_VIDEO_THREAD_ABORT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE);
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_thread_notification_queue, (u32*)&notification, NULL, 100000);
						if(result.code == 0 && notification == DECODE_VIDEO_THREAD_FINISHED_ABORTING_NOTIFICATION)
							break;
					}

					if(vid_player.num_of_audio_tracks > 0)
						Util_speaker_exit();

					Util_decoder_close_file(0);
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION)
						Util_converter_y2r_exit();

					for(int i = 0; i < DEF_VID_VIDEO_BUFFERS; i++)
					{
						for(int k = 0; k < 2; k++)
							Vid_large_texture_free(&vid_player.large_image[i][k]);
					}

					if(vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0)
						Util_remove_watch(&bar_pos);

					vid_player.show_full_screen_msg = false;
					vid_player.seek_bar.selected = false;
					vid_player.show_current_pos_until = 0;

					if(event == DECODE_THREAD_PLAY_NEXT_REQUEST && vid_player.playback_mode != DEF_VID_NO_REPEAT)
					{
						Vid_file* file_data = (Vid_file*)malloc(sizeof(Vid_file));

						if(vid_player.playback_mode == DEF_VID_IN_ORDER || vid_player.playback_mode == DEF_VID_RANDOM)
						{
							for(int i = 0; i < (vid_player.playback_mode == DEF_VID_RANDOM ? Util_expl_query_num_of_file() + 256 : Util_expl_query_num_of_file()); i++)
							{
								if(vid_player.file.index + 1 >= Util_expl_query_num_of_file())
									vid_player.file.index = 0;
								else
									vid_player.file.index++;

								if(Util_expl_query_type(vid_player.file.index) & FILE_TYPE_FILE)
								{
									srand(time(NULL) + i);
									if(vid_player.playback_mode == DEF_VID_IN_ORDER)
										break;
									else if(rand() % 5 == 1)//1 in 5
										break;
								}
							}
						}

						if(file_data)
						{
							//Create a message.
							file_data->index = Util_expl_query_current_file_index();
							snprintf(file_data->name, sizeof(file_data->name), "%s", Util_expl_query_file_name(file_data->index).c_str());
							snprintf(file_data->directory, sizeof(file_data->directory), "%s", Util_expl_query_current_dir().c_str());
						}

						//Play next video.
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_thread_command_queue,
						DECODE_THREAD_PLAY_REQUEST, file_data, 100000, QUEUE_OPTION_NONE)

						vid_player.state = PLAYER_STATE_PREPATE_PLAYING;
					}
					else
					{
						Vid_exit_full_screen();
						vid_player.state = PLAYER_STATE_IDLE;
					}

					vid_player.sub_state = PLAYER_SUB_STATE_NONE;

					if(event == DECODE_THREAD_SHUTDOWN_REQUEST)
					{
						//Exit the threads.
						vid_thread_run = false;
						continue;
					}

					break;
				}

				default:
					break;
			}
		}

		result = Util_queue_get(&vid_player.decode_thread_notification_queue, (u32*)&notification, NULL, 0);
		if(result.code == 0)
		{
			switch (notification)
			{
				case READ_PACKET_THREAD_FINISHED_READING_EOF_NOTIFICATION:
				{
					is_eof = true;
					//Fall through.
				}

				case READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION:
				{
					is_read_packet_thread_active = false;
					break;
				}

				case READ_PACKET_THREAD_FINISHED_SEEKING_NOTIFICATION:
				{
					//Do nothing if player state is not prepare seeking.
					if(vid_player.state != PLAYER_STATE_PREPARE_SEEKING)
						break;

					if(vid_player.num_of_video_tracks <= 0)
					{
						//If there are no video tracks, start seeking.
						//Sometimes library caches previous frames even after clearing packet, 
						//so ignore first 5 (+ num_of_threads if frame threading is used) frames.
						wait_count = 5 + (vid_player.video_info[0].thread_type == THREAD_TYPE_FRAME ? vid_player.num_of_threads : 0);
						backward_timeout = 20;
						vid_player.state = PLAYER_STATE_SEEKING;
					}
					else
					{
						//If there are video tracks, clear the video cache first.
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_video_thread_command_queue,
						DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
					}

					break;
				}

				case DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION:
				{
					is_waiting_video_decoder = false;
					break;
				}

				case DECODE_VIDEO_THREAD_FINISHED_CLEARING_CACHE:
				{
					//Do nothing if player state is not prepare seeking.
					if(vid_player.state != PLAYER_STATE_PREPARE_SEEKING)
						break;

					//After clearing cache start seeking.
					//Sometimes library caches previous frames even after clearing packet, 
					//so ignore first 5 (+ num_of_threads if frame threading is used) frames.
					wait_count = 5 + (vid_player.video_info[0].thread_type == THREAD_TYPE_FRAME ? vid_player.num_of_threads : 0);
					backward_timeout = 20;
					vid_player.state = PLAYER_STATE_SEEKING;

					break;
				}

				case DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION:
				case CONVERT_THREAD_FINISHED_BUFFERING_NOTIFICATION:
				{
					//Do nothing if player state is not buffering.
					if(vid_player.state != PLAYER_STATE_BUFFERING)
						break;

					if(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER)
					{
						//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

						//Resume the playback.
						Util_speaker_resume(0);
						vid_player.state = PLAYER_STATE_PLAYING;
					}
					else
					{
						//Don't resume.
						vid_player.state = PLAYER_STATE_PAUSE;
					}

					break;
				}

				case CONVERT_THREAD_OUT_OF_BUFFER_NOTIFICATION:
				{
					//Do nothing if player state is not playing nor pause.
					if(vid_player.state != PLAYER_STATE_PLAYING && vid_player.state != PLAYER_STATE_PAUSE)
						break;

					//Do nothing if we completely reached EOF.
					if(is_eof && Util_decoder_get_available_packet_num(0) <= 0)
						break;

					if(vid_player.state == PLAYER_STATE_PLAYING)
					{
						//Pause the playback.
						Util_speaker_pause(0);
						//Add resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_RESUME_LATER);
					}
					else//Remove resume later bit.
						vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

					vid_player.state = PLAYER_STATE_BUFFERING;

					break;
				}

				default:
					break;
			}
		}

		if(vid_player.state == PLAYER_STATE_PLAYING || vid_player.state == PLAYER_STATE_SEEKING
		|| vid_player.state == PLAYER_STATE_PAUSE || vid_player.state == PLAYER_STATE_BUFFERING)
		{
			bool key_frame = false;
			int packet_index = 0;
			int num_of_cached_packets = 0;
			int num_of_audio_buffers = 0;
			int num_of_video_buffers = 0;
			int audio_buffers_size = 0;
			int required_free_ram = 0;
			double audio_buffer_health_ms = 0;
			Packet_type type = PACKET_TYPE_UNKNOWN;

			//Calculate how much free RAM we need and check buffer health.
			//To prevent out of memory on other tasks, make sure we have at least : 
			//6MB + (raw image size * 2) for hardware decoding.
			//7MB + (raw image size * (1 + num_of_threads)) for software decoding.
			if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
			{
				//Hardware decoder always returns raw image in RGB565LE.
				required_free_ram = (1024 * 1024 * 6) + (vid_player.video_info[0].width * vid_player.video_info[0].height * 2 * 2);
				num_of_video_buffers = Util_mvd_video_decoder_get_available_raw_image_num(0);
			}
			else
			{
				//We are assuming raw image format is YUV420P because it is the most common format.
				required_free_ram = (1024 * 1024 * 7) + (vid_player.video_info[0].width * vid_player.video_info[0].height * 1.5 * (1 + (vid_player.video_info[0].thread_type == THREAD_TYPE_FRAME ? vid_player.num_of_threads : 1)));
				num_of_video_buffers = Util_video_decoder_get_available_raw_image_num(0, 0);

				if(vid_player.num_of_video_tracks > 1 && Util_video_decoder_get_available_raw_image_num(1, 0) > num_of_video_buffers)
					num_of_video_buffers = Util_video_decoder_get_available_raw_image_num(1, 0);
			}
			num_of_cached_packets = Util_decoder_get_available_packet_num(0);
			num_of_audio_buffers = Util_speaker_get_available_buffer_num(0);
			audio_buffers_size = Util_speaker_get_available_buffer_size(0);

			//Audio buffer health (in ms) is ((buffer_size / bytes_per_sample / num_of_ch / sample_rate) * 1000).
			audio_buffer_health_ms = (audio_buffers_size / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000);

			//Update audio position.
			if(vid_player.num_of_audio_tracks > 0)
				vid_player.audio_current_pos = vid_player.last_decoded_audio_pos - audio_buffer_health_ms;

			//Get position from audio track if duration is longer than video track.
			if(vid_player.num_of_video_tracks == 0 || vid_player.audio_info[vid_player.selected_audio_track].duration > vid_player.video_info[0].duration)
				vid_player.media_current_pos = vid_player.audio_current_pos;

			//If file does not have video tracks, update bar pos to see if the position has changed
			//so that we can update the screen when it gets changed.
			if(vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0)
				bar_pos = Util_convert_seconds_to_time(vid_player.media_current_pos / 1000);

			//If we only have half number of packets in buffer, we have not reached eof
			//and read packet thread is inactive, send a read packet request.
			if((num_of_cached_packets < (DEF_DECODER_MAX_CACHE_PACKETS / 2)) && !is_read_packet_thread_active && !is_eof)
			{
				is_read_packet_thread_active = true;

				//Start reading packets.
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.read_packet_thread_command_queue,
				READ_PACKET_THREAD_READ_PACKET_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
			}

			if(num_of_cached_packets == 0 && is_eof)
			{
				//We consumed all of packets.
				bool is_audio_done = true;
				bool is_video_done = true;

				//Wait for finish playback audio.
				if(vid_player.num_of_audio_tracks > 0)
				{
					if(audio_buffers_size > 0)
						is_audio_done = false;
				}

				//Wait for finish playback video. 
				if(vid_player.num_of_video_tracks > 0 && vid_player.video_frametime != 0)
				{
					if(num_of_video_buffers > 0)
						is_video_done = false;
				}

				//Stop buffering if we completely reached EOF.
				if(vid_player.state == PLAYER_STATE_BUFFERING)
				{
					//Notify we've done buffering (can't buffer anymore).
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_VIDEO_THREAD_STR, &vid_player.decode_thread_notification_queue,
					DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)
					continue;
				}

				if(is_audio_done && is_video_done)
				{
					//We've finished playing.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_thread_command_queue,
					DECODE_THREAD_PLAY_NEXT_REQUEST, NULL, 100000, QUEUE_OPTION_NONE)
					continue;
				}

				Util_sleep(10000);
				continue;
			}

			if(is_waiting_video_decoder || (num_of_video_buffers + 1 >= DEF_DECODER_MAX_RAW_IMAGE)
			|| (num_of_audio_buffers + 1 >= DEF_SPEAKER_MAX_BUFFERS) || (int)Util_check_free_linear_space() < required_free_ram)
			{
				//If one of them is true, wait and try again later.
				//1. Video decoder thread has not completed copying the packet yet.
				//2. Video buffer is full.
				//3. Speaker buffer is full.
				//4. No enough free RAM.

				if(is_waiting_video_decoder)
				{
					//Wait for video decoder thread.
					Util_sleep(1000);
					continue;
				}
				else if(vid_player.num_of_video_tracks <= 0)
				{
					//No video tracks, speaker buffer must be full.
					Util_sleep(10000);
					continue;
				}
				else if(num_of_video_buffers > 0)
				{
					//We don't have enough free RAM but have cached raw pictures,
					//so wait frametime ms for them to get playbacked and freed.
					//If framerate is unknown, sleep 10ms.
					int sleep = (vid_player.video_info[0].framerate > 0 ? (1000000 / vid_player.video_info[0].framerate) : 10000);

					if(vid_player.state == PLAYER_STATE_BUFFERING)
					{
						//Notify we've done buffering (can't buffer anymore).
						DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_VIDEO_THREAD_STR, &vid_player.decode_thread_notification_queue,
						DECODE_THREAD_FINISHED_BUFFERING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)
						continue;
					}

					Util_sleep(sleep);
					continue;
				}
				else
				{
					//If we don't have enough RAM and there are no cached raw pictures,
					//it is unlikely to increase amount of free RAM by waiting since
					//no video buffer get freed, so we have to try to decode it anyway.
				}
			}

			result = Util_decoder_parse_packet(&type, &packet_index, &key_frame, 0);
			if(result.code == DEF_ERR_TRY_AGAIN)//Packet is not ready, try again later.
			{
				Util_sleep(5000);
				continue;
			}
			else if(result.code != 0)//For other errors, log error detail.
			{
				Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_parse_packet()..." + result.string + result.error_description, result.code);
				Util_sleep(10000);
				continue;
			}

			//Handle seek request.
			if(vid_player.state == PLAYER_STATE_SEEKING && (vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0 || type == PACKET_TYPE_VIDEO))
			{
				//Make sure we went back.
				if((wait_count <= 0 && vid_player.video_current_pos < seek_start_pos)
				|| vid_player.num_of_video_tracks == 0 || vid_player.video_frametime == 0)//Remove seek backward wait bit.
					vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);

				if(!(vid_player.sub_state & PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT) && vid_player.media_current_pos >= vid_player.seek_pos)
				{
					//Seek has finished.
					vid_player.show_current_pos_until = osGetTime() + 4000;

					if(vid_player.num_of_video_tracks > 0 && vid_player.video_frametime != 0)//Buffer some data before resuming playback if file contains video tracks.
						vid_player.state = PLAYER_STATE_BUFFERING;
					else
					{
						if(vid_player.sub_state & PLAYER_SUB_STATE_RESUME_LATER)
						{
							//Remove resume later bit.
							vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_RESUME_LATER);

							//Resume the playback.
							Util_speaker_resume(0);
							vid_player.state = PLAYER_STATE_PLAYING;
						}
						else
						{
							//Don't resume.
							vid_player.state = PLAYER_STATE_PAUSE;
						}
					}
				}

				if(wait_count > 0)
					wait_count--;
				else if(backward_timeout <= 0)//Timeout, remove seek backward wait bit.
					vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_SEEK_BACKWARD_WAIT);
				else
					backward_timeout--;
			}

			if(type == PACKET_TYPE_UNKNOWN)
			{
				Util_log_save(DEF_VID_DECODE_THREAD_STR, "Unknown packet type!!!!!");
				Util_sleep(10000);
				continue;
			}
			else if(type == PACKET_TYPE_AUDIO)
			{
				if(vid_player.num_of_audio_tracks != 0 && packet_index == vid_player.selected_audio_track)
				{
					result = Util_decoder_ready_audio_packet(packet_index, 0);
					if(result.code == 0)
					{
						u8* audio = NULL;
						int audio_samples = 0;
						double pos = 0;
						TickCounter counter;
						Audio_converter_parameters parameters;

						parameters.converted = NULL;

						osTickCounterStart(&counter);
						result = Util_audio_decoder_decode(&audio_samples, &audio, &pos, packet_index, 0);
						osTickCounterUpdate(&counter);
						vid_player.audio_decoding_time_list[319] = osTickCounterRead(&counter);

						for(int i = 1; i < 320; i++)
							vid_player.audio_decoding_time_list[i - 1] = vid_player.audio_decoding_time_list[i];

						if(result.code == 0 && vid_player.state != PLAYER_STATE_SEEKING)
						{
							parameters.source = audio;
							parameters.in_ch = vid_player.audio_info[packet_index].ch;
							parameters.in_sample_rate = vid_player.audio_info[packet_index].sample_rate;
							parameters.in_sample_format = vid_player.audio_info[packet_index].sample_format;
							parameters.in_samples = audio_samples;
							parameters.converted = NULL;
							parameters.out_ch = (vid_player.audio_info[packet_index].ch > 2 ? 2 : vid_player.audio_info[packet_index].ch);
							parameters.out_sample_rate = vid_player.audio_info[packet_index].sample_rate;
							parameters.out_sample_format = SAMPLE_FORMAT_S16;

							result = Util_converter_convert_audio(&parameters);
							if(result.code == 0)
							{
								bool too_big = false;

								//Change volume.
								if(vid_player.volume != 100)
								{
									for(int i = 0; i < (parameters.out_samples * parameters.out_ch * 2); i += 2)
									{
										if(*(s16*)(parameters.converted + i) * ((double)vid_player.volume / 100) > INT16_MAX)
										{
											*(s16*)(parameters.converted + i) = INT16_MAX;
											too_big = true;
										}
										else
											*(s16*)(parameters.converted + i) = *(s16*)(parameters.converted + i) * ((double)vid_player.volume / 100);
									}

									if(too_big)
										vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state | PLAYER_SUB_STATE_TOO_BIG);
									else
										vid_player.sub_state = (Vid_player_sub_state)(vid_player.sub_state & ~PLAYER_SUB_STATE_TOO_BIG);
								}

								//Add audio to speaker buffer, wait up to 250ms.
								for(int i = 0; i < 125; i++)
								{
									result = Util_speaker_add_buffer(0, parameters.converted, (parameters.out_samples * parameters.out_ch * 2));
									if(result.code != DEF_ERR_TRY_AGAIN)
										break;

									Util_sleep(2000);
								}
							}
							else
								Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_converter_convert_audio()..." + result.string + result.error_description, result.code);
						}
						else if(result.code != 0)
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_audio_decoder_decode()..." + result.string + result.error_description, result.code);

						//Update audio position.
						vid_player.last_decoded_audio_pos = pos;

						Util_safe_linear_free(parameters.converted);
						Util_safe_linear_free(audio);
						parameters.converted = NULL;
						audio = NULL;
					}
					else
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_ready_audio_packet()..." + result.string + result.error_description, result.code);
				}
				else//This packet is not what we are looking for now, just skip it.
					Util_decoder_skip_audio_packet(packet_index, 0);
			}
			else if(type == PACKET_TYPE_SUBTITLE)
			{
				if(vid_player.num_of_subtitle_tracks != 0 && packet_index == vid_player.selected_subtitle_track)
				{
					result = Util_decoder_ready_subtitle_packet(packet_index, 0);
					if(result.code == 0)
					{
						result = Util_subtitle_decoder_decode(&vid_player.subtitle_data[vid_player.subtitle_index], packet_index, 0);
						if(result.code == 0)
						{
							if(vid_player.subtitle_index + 1 >= DEF_VID_SUBTITLE_BUFFERS)
								vid_player.subtitle_index = 0;
							else
								vid_player.subtitle_index++;
						}
						else
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_subtitle_decoder_decode()..." + result.string + result.error_description, result.code);
					}
					else
						Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_decoder_ready_subtitle_packet()..." + result.string + result.error_description, result.code);

				}
				else//This packet is not what we are looking for now, just skip it.
					Util_decoder_skip_subtitle_packet(packet_index, 0);
			}
			else if(type == PACKET_TYPE_VIDEO)
			{
				if(vid_player.num_of_video_tracks != 0 && (!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
				|| ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) && packet_index == 0)))
				{
					Vid_video_packet_data* packet_info = (Vid_video_packet_data*)malloc(sizeof(Vid_video_packet_data));

					if(packet_info)
					{
						packet_info->is_key_frame = key_frame;
						packet_info->packet_index = packet_index;

						is_waiting_video_decoder = true;
						//Decode the next frame.
						//Too noisy.
						// DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_THREAD_STR, &vid_player.decode_video_thread_command_queue,
						// DECODE_VIDEO_THREAD_DECODE_REQUEST, packet_info, 100000, QUEUE_OPTION_NONE);
						result = Util_queue_add(&vid_player.decode_video_thread_command_queue,
						DECODE_VIDEO_THREAD_DECODE_REQUEST, packet_info, 100000, QUEUE_OPTION_NONE);
						if(result.code != 0)
							Util_log_save(DEF_VID_DECODE_THREAD_STR, "Util_queue_add()..." + result.string + result.error_description, result.code);
					}
					else
						Util_decoder_skip_video_packet(packet_index, 0);
				}
				else if(type == PACKET_TYPE_VIDEO)//This packet is not what we are looking for now, just skip it.
					Util_decoder_skip_video_packet(packet_index, 0);
			}
		}
		else
			Util_sleep(1000);
	}

	Util_log_save(DEF_VID_DECODE_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Vid_decode_video_thread(void* arg)
{
	Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Thread started.");
	double skip = 0;
	TickCounter counter;
	Result_with_string result;

	osTickCounterStart(&counter);

	while (vid_thread_run)
	{
		void* message = NULL;
		Vid_command event = NONE_REQUEST;

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
		}

		result = Util_queue_get(&vid_player.decode_video_thread_command_queue, (u32*)&event, &message, DEF_ACTIVE_THREAD_SLEEP_TIME);
		if(result.code == 0)
		{
			switch (event)
			{
				case DECODE_VIDEO_THREAD_DECODE_REQUEST:
				{
					bool key_frame = false;
					int packet_index = 0;
					Vid_video_packet_data* packet_info = (Vid_video_packet_data*)message;

					//Do nothing if player state is idle or prepare playing or message is NULL.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING || !packet_info)
						break;

					key_frame = packet_info->is_key_frame;
					packet_index = packet_info->packet_index;

					free(packet_info);
					packet_info = NULL;

					if(vid_player.allow_skip_frames && skip > vid_player.video_frametime && (!key_frame || vid_player.allow_skip_key_frames) && vid_player.video_frametime != 0)
					{
						skip -= vid_player.video_frametime;
						Util_decoder_skip_video_packet(packet_index, 0);

						//Notify we've done copying packet to video decoder buffer (we skipped the frame here)
						//so that decode thread can read the next packet.
						//Too noisy.
						// DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_VIDEO_THREAD_STR, &vid_player.decode_thread_notification_queue,
						// DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE);
						result = Util_queue_add(&vid_player.decode_thread_notification_queue,
						DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE);
						if(result.code != 0)
							Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_queue_add()..." + result.string + result.error_description, result.code);
					}
					else
					{
						result = Util_decoder_ready_video_packet(packet_index, 0);

						//Notify we've done copying packet to video decoder buffer
						//so that decode thread can read the next packet.
						//Too noisy.
						// DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_VIDEO_THREAD_STR, &vid_player.decode_thread_notification_queue,
						// DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE);
						result = Util_queue_add(&vid_player.decode_thread_notification_queue,
						DECODE_VIDEO_THREAD_FINISHED_COPYING_PACKET_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE);
						if(result.code != 0)
							Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_queue_add()..." + result.string + result.error_description, result.code);

						if(result.code == 0)
						{
							while(true)
							{
								osTickCounterUpdate(&counter);

								if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
									result = Util_mvd_video_decoder_decode(0);
								else
									result = Util_video_decoder_decode(packet_index, 0);

								osTickCounterUpdate(&counter);

								if(result.code == DEF_ERR_MVD_TRY_AGAIN || result.code == DEF_ERR_MVD_TRY_AGAIN_NO_OUTPUT
								|| result.code == DEF_ERR_TRY_AGAIN)
								{
									if(result.code == DEF_ERR_MVD_TRY_AGAIN)//Got a frame.
									{
										Vid_update_performance_graph(osTickCounterRead(&counter), key_frame, &skip);
										key_frame = false;
									}
									else if(result.code == DEF_ERR_TRY_AGAIN)//Buffer is full.
									{
										if(vid_player.video_frametime <= 0)
											Util_sleep(10000);
										else
											Util_sleep(vid_player.video_frametime * 1000);

										//If we get clear cache or abort request while waiting, break the loop.
										if(Util_queue_check_event_exist(&vid_player.decode_video_thread_command_queue, DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST)
										|| Util_queue_check_event_exist(&vid_player.decode_video_thread_command_queue, DECODE_VIDEO_THREAD_ABORT_REQUEST))
											break;
									}

									continue;
								}
								else
									break;
							}

							if(result.code == 0)
								Vid_update_performance_graph(osTickCounterRead(&counter), key_frame, &skip);
							else if(result.code != DEF_ERR_NEED_MORE_INPUT)
							{
								if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
									Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_mvd_video_decoder_decode()..." + result.string + result.error_description, result.code);
								else
									Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_video_decoder_decode()..." + result.string + result.error_description, result.code);
							}
						}
						else
						{
							Util_decoder_skip_video_packet(packet_index, 0);
							Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Util_decoder_ready_video_packet()..." + result.string + result.error_description, result.code);
						}
					}

					break;
				}

				case DECODE_VIDEO_THREAD_CLEAR_CACHE_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					//Clear cache.
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
						Util_mvd_video_decoder_clear_raw_image(0);
					else
					{
						for(int i = 0; i < vid_player.num_of_video_tracks; i++)
							Util_video_decoder_clear_raw_image(i, 0);
					}

					//Flush the decoder.
					while(true)
					{
						if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
							result = Util_mvd_video_decoder_decode(0);
						else
						{
							result = Util_video_decoder_decode(0, 0);
							if(vid_player.num_of_video_tracks > 1)
								result = Util_video_decoder_decode(1, 0);
						}

						if(result.code != 0 && result.code != DEF_ERR_MVD_TRY_AGAIN_NO_OUTPUT && result.code != DEF_ERR_MVD_TRY_AGAIN)
							break;
					}

					//Notify we've done clearing cache.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_VIDEO_THREAD_STR, &vid_player.decode_thread_notification_queue,
					DECODE_VIDEO_THREAD_FINISHED_CLEARING_CACHE, NULL, 100000, QUEUE_OPTION_NONE)

					break;
				}

				case DECODE_VIDEO_THREAD_ABORT_REQUEST:
				{
					skip = 0;

					//Clear cache.
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
						Util_mvd_video_decoder_clear_raw_image(0);
					else
					{
						for(int i = 0; i < vid_player.num_of_video_tracks; i++)
							Util_video_decoder_clear_raw_image(i, 0);
					}

					//Flush the command queue.
					while(true)
					{
						result = Util_queue_get(&vid_player.decode_video_thread_command_queue, (u32*)&event, NULL, 0);
						if(result.code != 0)
							break;
					}

					//Notify we've done aborting.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_DECODE_VIDEO_THREAD_STR, &vid_player.decode_thread_notification_queue,
					DECODE_VIDEO_THREAD_FINISHED_ABORTING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

					break;
				}

				default:
					break;
			}
		}
	}
	
	Util_log_save(DEF_VID_DECODE_VIDEO_THREAD_STR, "Thread exit.");
	threadExit(0);
}

void Vid_convert_thread(void* arg)
{
	Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Thread started.");

	bool should_convert = false;
	int packet_index = 0;
	int vfps_cache = 0;
	u64 next_vfps_update = 0;
	double video_delay_ms = 0;
	TickCounter conversion_time_counter;
	Result_with_string result;

	osTickCounterStart(&conversion_time_counter);

	while (vid_thread_run)
	{
		int num_of_cached_raw_images = 0;
		u64 timeout_us = DEF_ACTIVE_THREAD_SLEEP_TIME;
		Vid_command event = NONE_REQUEST;

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
		}

		//If player state is playing or seeking, don't wait for commands.
		if((vid_player.state == PLAYER_STATE_PLAYING || vid_player.state == PLAYER_STATE_SEEKING)
		&& should_convert)
			timeout_us = 0;

		result = Util_queue_get(&vid_player.convert_thread_command_queue, (u32*)&event, NULL, timeout_us);
		if(result.code == 0)
		{
			switch (event)
			{
				case CONVERT_THREAD_CONVERT_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing or file doesn't have video tracks.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
					|| vid_player.num_of_video_tracks <= 0)
						break;

					should_convert = true;
					video_delay_ms = 0;
					packet_index = 0;
					vfps_cache = 0;
					next_vfps_update = osGetTime() + 1000;

					break;
				}

				case CONVERT_THREAD_ABORT_REQUEST:
				{
					should_convert = false;
					video_delay_ms = 0;
					packet_index = 0;
					vfps_cache = 0;
					next_vfps_update = 0;

					//Flush the command queue.
					while(true)
					{
						result = Util_queue_get(&vid_player.convert_thread_command_queue, (u32*)&event, NULL, 0);
						if(result.code != 0)
							break;
					}

					//Notify we've done aborting.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_CONVERT_THREAD_STR, &vid_player.decode_thread_notification_queue,
					CONVERT_THREAD_FINISHED_ABORTING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

					break;
				}

				default:
					break;
			}
		}

		//Update vps (video playback framerate).
		if(osGetTime() >= next_vfps_update)
		{
			if(osGetTime() >= next_vfps_update + 1000)
				next_vfps_update = osGetTime() + 1000;
			else
				next_vfps_update += 1000;

			vid_player.vps = vfps_cache;
			vfps_cache = 0;
		}

		//Do nothing if player state is idle, prepare playing, pause, prepare seeking or should_convert flag is not set.
		if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING
		|| vid_player.state == PLAYER_STATE_PAUSE || vid_player.state == PLAYER_STATE_PREPARE_SEEKING || !should_convert)
		{
			video_delay_ms = 0;
			continue;
		}

		//Check if we have cached raw images.
		if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
			num_of_cached_raw_images = Util_mvd_video_decoder_get_available_raw_image_num(0);
		else
			num_of_cached_raw_images = Util_video_decoder_get_available_raw_image_num(packet_index, 0);

		//Skip video frame if we can't keep up or we are seeking.
		if((video_delay_ms > vid_player.video_frametime || vid_player.state == PLAYER_STATE_SEEKING) && vid_player.video_frametime != 0)
		{
			if(num_of_cached_raw_images > 0)
			{
				double pos = 0;

				if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
					Util_mvd_video_decoder_skip_image(&pos, 0);
				else
				{
					if(vid_player.num_of_video_tracks >= 2)
					{
						Util_video_decoder_skip_image(&pos, 0, 0);
						Util_video_decoder_skip_image(&pos, 1, 0);
					}
					else
						Util_video_decoder_skip_image(&pos, packet_index, 0);
				}

				vid_player.video_current_pos = pos;
				if(vid_player.num_of_video_tracks >= 2)
					packet_index = (packet_index == 0 ? 1 : 0);
			}
			else if(vid_player.state == PLAYER_STATE_SEEKING)
				Util_sleep(3000);

			osTickCounterUpdate(&conversion_time_counter);

			//Get position from video track if duration is longer than ot equal to audio track.
			if(vid_player.video_frametime != 0 && (vid_player.num_of_audio_tracks == 0
			|| vid_player.video_info[0].duration >= vid_player.audio_info[vid_player.selected_audio_track].duration))
				vid_player.media_current_pos = vid_player.video_current_pos;

			if(vid_player.state == PLAYER_STATE_SEEKING)
				video_delay_ms = 0;
			else
				video_delay_ms -= vid_player.video_frametime - osTickCounterRead(&conversion_time_counter);
		}
		else if(vid_player.state == PLAYER_STATE_PLAYING)
		{
			osTickCounterUpdate(&conversion_time_counter);

			if(num_of_cached_raw_images <= 0 && vid_player.video_frametime != 0 && (Util_speaker_get_available_buffer_num(0) + 1) < DEF_SPEAKER_MAX_BUFFERS)
			{
				//Notify we've run out of buffer.
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_CONVERT_THREAD_STR, &vid_player.decode_thread_notification_queue,
				CONVERT_THREAD_OUT_OF_BUFFER_NOTIFICATION, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)
				Util_sleep(1000);
			}
			else
			{
				u8* yuv_video = NULL;
				u8* video = NULL;
				int width = vid_player.video_info[packet_index].codec_width;
				int height = vid_player.video_info[packet_index].codec_height;
				double pos = 0;

				if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)//Hardware decoder only supports 1 track at a time.
					result = Util_mvd_video_decoder_get_image(&video, &pos, vid_player.video_info[0].codec_width, vid_player.video_info[0].codec_height, 0);
				else
					result = Util_video_decoder_get_image(&yuv_video, &pos, width, height, packet_index, 0);

				if(result.code == 0)
				{
					//Audio buffer health (in ms) is ((buffer_size / bytes_per_sample / num_of_ch / sample_rate) * 1000).
					double audio_buffer_health_ms = (Util_speaker_get_available_buffer_size(0) / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000);

					//Get position from video track if duration is longer than ot equal to audio track.
					if(vid_player.video_frametime != 0 && (vid_player.num_of_audio_tracks == 0
					|| vid_player.video_info[0].duration >= vid_player.audio_info[vid_player.selected_audio_track].duration))
					{
						vid_player.video_current_pos = pos - vid_player.video_frametime;
						vid_player.media_current_pos = vid_player.video_current_pos;
					}

					//Update audio position.
					if(vid_player.num_of_audio_tracks > 0)
						vid_player.audio_current_pos = vid_player.last_decoded_audio_pos - audio_buffer_health_ms;

					//Hardware decoder returns BGR565, so we don't have to convert it.
					if(!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING))
					{
						if(vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION)//Use hardware color converter.
							result = Util_converter_y2r_yuv420p_to_rgb565le(yuv_video, &video, width, height, true);
						else//Use software color converter.
						{
							Color_converter_parameters color_converter_parameters;
							color_converter_parameters.source = yuv_video;
							color_converter_parameters.converted = NULL;
							color_converter_parameters.in_width = width;
							color_converter_parameters.in_height = height;
							color_converter_parameters.in_color_format = vid_player.video_info[packet_index].pixel_format;
							color_converter_parameters.out_width = width;
							color_converter_parameters.out_height = height;
							color_converter_parameters.out_color_format = PIXEL_FORMAT_RGB565LE;
							result = Util_converter_convert_color(&color_converter_parameters);
							video = color_converter_parameters.converted;
						}
					}

					//Set texture data.
					if(result.code == 0)
					{
						int image_num = (packet_index == 0 ? vid_player.image_index : vid_player.image_index_3d);

						if(!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) && (vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION))//Raw image is texture format.
							result = Vid_large_texture_set_data(&vid_player.large_image[image_num][packet_index], video, width, height, true);
						else//Raw image is NOT texture format.
							result = Vid_large_texture_set_data(&vid_player.large_image[image_num][packet_index], video, width, height, false);

						if(result.code != 0)
							Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Vid_large_texture_set_data()..." + result.string + result.error_description, result.code);

						//Crop the image so that user won't see glitch on videos.
						Vid_large_texture_crop(&vid_player.large_image[image_num][packet_index], vid_player.video_info[packet_index].width, vid_player.video_info[packet_index].height);

						if(packet_index == 0)
						{
							if(image_num + 1 < DEF_VID_VIDEO_BUFFERS)
								vid_player.image_index++;
							else
								vid_player.image_index = 0;
						}
						else if(packet_index == 1)
						{
							if(image_num + 1 < DEF_VID_VIDEO_BUFFERS)
								vid_player.image_index_3d++;
							else
								vid_player.image_index_3d = 0;
						}

						if(packet_index == 0)
						{
							vfps_cache++;
							var_need_reflesh = true;
						}

						if(vid_player.num_of_video_tracks >= 2)
							packet_index = !packet_index;
					}
					else
						Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Util_converter_yuv420p_to_bgr565()..." + result.string + result.error_description, result.code);
				}
				else
				{
					if(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING)
						Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Util_mvd_video_decoder_get_image()..." + result.string + result.error_description, result.code);
					else
						Util_log_save(DEF_VID_CONVERT_THREAD_STR, "Util_video_decoder_get_image()..." + result.string + result.error_description, result.code);

					Util_sleep(5000);
				}

				Util_safe_linear_free(yuv_video);
				Util_safe_linear_free(video);
				yuv_video = NULL;
				video = NULL;

				for(int i = 1; i < 320; i++)
					vid_player.conversion_time_list[i - 1] = vid_player.conversion_time_list[i];

				osTickCounterUpdate(&conversion_time_counter);
				vid_player.conversion_time_list[319] = osTickCounterRead(&conversion_time_counter);

				if(packet_index == 0 && vid_player.video_frametime != 0)
				{
					//Calculate how long we should sleep before converting the next frame.
					double sleep_time_ms = (vid_player.video_frametime - vid_player.conversion_time_list[319]);

					if(vid_player.num_of_audio_tracks > 0 && Util_speaker_get_available_buffer_num(0) > 0)
					{
						float av_difference_ms = vid_player.video_current_pos - vid_player.audio_current_pos;

						if(av_difference_ms >= 0)
						{
							//If audio is late, add extra sleep (up to 100ms) time to sync with audio.
							sleep_time_ms += av_difference_ms > 100 ? 100 : av_difference_ms;
						}
						else
						{
							//If video is late, reduce sleep time to sync with audio
							sleep_time_ms += av_difference_ms < -100 ? -100 : av_difference_ms;
						}
					}

					if(sleep_time_ms > 0)
					{
						if(video_delay_ms > 0)
						{
							//If we are delayed, try to reduce delay time before sleeping.
							if(sleep_time_ms > video_delay_ms)
							{
								sleep_time_ms -= video_delay_ms;
								video_delay_ms = 0;
							}
							else
							{
								video_delay_ms -= sleep_time_ms;
								sleep_time_ms = 0;
							}
						}

						if(sleep_time_ms > 0)
						{
							//We still go ahead, so we can sleep.
							TickCounter counter;

							osTickCounterStart(&counter);
							Util_sleep(sleep_time_ms * 1000);
							osTickCounterUpdate(&counter);

							//Sleep function may sleep more than the specified duration, so check the actual sleep duration.
							video_delay_ms += osTickCounterRead(&counter) - sleep_time_ms;
						}
					}
					else
						video_delay_ms -= sleep_time_ms;//We are delayed.
				}
			}
		}
		else if(vid_player.state == PLAYER_STATE_BUFFERING)
		{
			if(num_of_cached_raw_images >= vid_player.restart_playback_threshold
			|| Util_speaker_get_available_buffer_num(0) + 1 >= DEF_SPEAKER_MAX_BUFFERS)
			{
				//Notify we've finished buffering.
				DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_CONVERT_THREAD_STR, &vid_player.decode_thread_notification_queue,
				CONVERT_THREAD_FINISHED_BUFFERING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)
			}

			video_delay_ms = 0;
		}
		else
			Util_sleep(1000);
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
		Vid_command event = NONE_REQUEST;

		result = Util_queue_get(&vid_player.read_packet_thread_command_queue, (u32*)&event, NULL, DEF_ACTIVE_THREAD_SLEEP_TIME);
		if(result.code == 0)
		{
			switch (event)
			{
				case READ_PACKET_THREAD_READ_PACKET_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					while(true)
					{
						result = Util_decoder_read_packet(0);
						if(result.code == 0)
						{
							if(Util_decoder_get_available_packet_num(0) + 1 >= DEF_DECODER_MAX_CACHE_PACKETS)
							{
								//Notify we've done reading.
								DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_READ_PACKET_THREAD_STR, &vid_player.decode_thread_notification_queue,
								READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

								break;
							}
						}
						if(result.code == DEF_ERR_FFMPEG_RETURNED_NOT_SUCCESS)
						{
							//Notify we've reached EOF.
							DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_READ_PACKET_THREAD_STR, &vid_player.decode_thread_notification_queue,
							READ_PACKET_THREAD_FINISHED_READING_EOF_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

							break;
						}
						else if(result.code != 0)
						{
							if(result.code == DEF_ERR_TRY_AGAIN)
							{
								//Wait and try again.
								Util_sleep(4000);
							}
							else
							{
								Util_log_save(DEF_VID_READ_PACKET_THREAD_STR, "Util_decoder_read_packet()..." + result.string + result.error_description, result.code);
								//Notify we've done reading.
								DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_READ_PACKET_THREAD_STR, &vid_player.decode_thread_notification_queue,
								READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

								break;
							}
						}

						//If we get seek or abort request while reading, break the loop.
						if(Util_queue_check_event_exist(&vid_player.read_packet_thread_command_queue, READ_PACKET_THREAD_SEEK_REQUEST)
						|| Util_queue_check_event_exist(&vid_player.read_packet_thread_command_queue, READ_PACKET_THREAD_ABORT_REQUEST))
						{
							//Notify we've done reading.
							DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_READ_PACKET_THREAD_STR, &vid_player.decode_thread_notification_queue,
							READ_PACKET_THREAD_FINISHED_READING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

							break;
						}
					}

					break;
				}

				case READ_PACKET_THREAD_SEEK_REQUEST:
				{
					//Do nothing if player state is idle or prepare playing.
					if(vid_player.state == PLAYER_STATE_IDLE || vid_player.state == PLAYER_STATE_PREPATE_PLAYING)
						break;

					Util_decoder_seek(vid_player.seek_pos, SEEK_FLAG_BACKWARD, 0);
					Util_log_save(DEF_VID_READ_PACKET_THREAD_STR, "Util_decoder_seek()..." + result.string + result.error_description, result.code);
					Util_decoder_clear_cache_packet(0);

					//Notify we've done seeking.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_READ_PACKET_THREAD_STR, &vid_player.decode_thread_notification_queue,
					READ_PACKET_THREAD_FINISHED_SEEKING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

					break;
				}

				case READ_PACKET_THREAD_ABORT_REQUEST:
				{
					//Flush the command queue.
					while(true)
					{
						result = Util_queue_get(&vid_player.read_packet_thread_command_queue, (u32*)&event, NULL, 0);
						if(result.code != 0)
							break;
					}

					//Notify we've done aborting.
					DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_READ_PACKET_THREAD_STR, &vid_player.decode_thread_notification_queue,
					READ_PACKET_THREAD_FINISHED_ABORTING_NOTIFICATION, NULL, 100000, QUEUE_OPTION_NONE)

					break;
				}

				default:
					break;
			}
		}

		if(vid_player.state == PLAYER_STATE_IDLE)
		{
			while (vid_thread_suspend)
				Util_sleep(DEF_INACTIVE_THREAD_SLEEP_TIME);
		}
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

void Vid_init_thread(void* arg)
{
	Util_log_save(DEF_VID_INIT_STR, "Thread started.");
	u8* cache = NULL;
	u32 read_size = 0;
	std::string out_data[17];
	Result_with_string result;

	vid_status += "Initializing variables...";
	Vid_init_variable();
	Vid_exit_full_screen();

	vid_player.select_audio_track_button.c2d = var_square_image[0];
	vid_player.texture_filter_button.c2d = var_square_image[0];
	vid_player.allow_skip_frames_button.c2d = var_square_image[0];
	vid_player.allow_skip_key_frames_button.c2d = var_square_image[0];
	vid_player.volume_button.c2d = var_square_image[0];
	vid_player.seek_duration_button.c2d = var_square_image[0];
	vid_player.use_hw_decoding_button.c2d = var_square_image[0];
	vid_player.use_hw_color_conversion_button.c2d = var_square_image[0];
	vid_player.use_multi_threaded_decoding_button.c2d = var_square_image[0];
	vid_player.lower_resolution_button.c2d = var_square_image[0];
	vid_player.menu_button[0].c2d = var_square_image[0];
	vid_player.menu_button[1].c2d = var_square_image[0];
	vid_player.menu_button[2].c2d = var_square_image[0];
	vid_player.control_button.c2d = var_square_image[0];
	vid_player.ok_button.c2d = var_square_image[0];
	vid_player.correct_aspect_ratio_button.c2d = var_square_image[0];
	vid_player.move_content_button.c2d = var_square_image[0];
	vid_player.remember_video_pos_button.c2d = var_square_image[0];
	vid_player.show_decode_graph_button.c2d = var_square_image[0];
	vid_player.show_color_conversion_graph_button.c2d = var_square_image[0];
	vid_player.show_packet_buffer_graph_button.c2d = var_square_image[0];
	vid_player.show_raw_video_buffer_graph_button.c2d = var_square_image[0];
	vid_player.show_raw_audio_buffer_graph_button.c2d = var_square_image[0];
	vid_player.menu_background.c2d = var_square_image[0];
	vid_player.scroll_bar.c2d = var_square_image[0];
	vid_player.playback_mode_button.c2d = var_square_image[0];
	vid_player.select_subtitle_track_button.c2d = var_square_image[0];
	vid_player.disable_audio_button.c2d = var_square_image[0];
	vid_player.disable_video_button.c2d = var_square_image[0];
	vid_player.disable_subtitle_button.c2d = var_square_image[0];
	vid_player.restart_playback_threshold_bar.c2d = var_square_image[0];
	vid_player.seek_bar.c2d = var_square_image[0];

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		vid_player.audio_track_button[i].c2d = var_square_image[0];

	for(int i = 0; i < DEF_DECODER_MAX_SUBTITLE_TRACKS; i++)
		vid_player.subtitle_track_button[i].c2d = var_square_image[0];

	Util_add_watch((int*)&vid_player.state);
	Util_add_watch(&vid_player.is_selecting_audio_track);
	Util_add_watch(&vid_player.is_selecting_subtitle_track);
	Util_add_watch(&vid_player.is_displaying_controls);
	Util_add_watch(&vid_player.is_setting_seek_duration);
	Util_add_watch(&vid_player.is_setting_volume);
	Util_add_watch(&vid_player.use_hw_decoding);
	Util_add_watch(&vid_player.use_hw_color_conversion);
	Util_add_watch(&vid_player.use_multi_threaded_decoding);

	Util_add_watch(&vid_player.use_linear_texture_filter);
	Util_add_watch(&vid_player.allow_skip_frames);
	Util_add_watch(&vid_player.allow_skip_key_frames);
	Util_add_watch(&vid_player.is_full_screen);
	Util_add_watch(&vid_player.correct_aspect_ratio);
	Util_add_watch(&vid_player.move_content_mode);
	Util_add_watch(&vid_player.remember_video_pos);
	Util_add_watch(&vid_player.playback_mode);
	Util_add_watch(&vid_player.menu_mode);
	Util_add_watch(&vid_player.lower_resolution);
	Util_add_watch(&vid_player.show_decoding_graph);
	Util_add_watch(&vid_player.show_color_conversion_graph);
	Util_add_watch(&vid_player.show_packet_buffer_graph);
	Util_add_watch(&vid_player.show_raw_video_buffer_graph);
	Util_add_watch(&vid_player.show_raw_audio_buffer_graph);

	Util_add_watch(&vid_player.seek_pos);
	Util_add_watch(&vid_player.ui_y_offset);
	Util_add_watch(&vid_player.ui_y_offset_max);
	Util_add_watch(&vid_player.subtitle_x_offset);
	Util_add_watch(&vid_player.subtitle_y_offset);
	Util_add_watch(&vid_player.subtitle_zoom);
	Util_add_watch(&vid_player.video_x_offset);
	Util_add_watch(&vid_player.video_y_offset);
	Util_add_watch(&vid_player.video_zoom);
	Util_add_watch(&vid_player.selected_audio_track_cache);
	Util_add_watch(&vid_player.selected_subtitle_track_cache);
	Util_add_watch(&vid_player.restart_playback_threshold);

	Util_add_watch(&vid_player.ok_button.selected);
	Util_add_watch(&vid_player.control_button.selected);
	Util_add_watch(&vid_player.scroll_bar.selected);
	Util_add_watch(&vid_player.select_audio_track_button.selected);
	Util_add_watch(&vid_player.select_subtitle_track_button.selected);
	Util_add_watch(&vid_player.texture_filter_button.selected);
	Util_add_watch(&vid_player.allow_skip_frames_button.selected);
	Util_add_watch(&vid_player.allow_skip_key_frames_button.selected);
	Util_add_watch(&vid_player.volume_button.selected);
	Util_add_watch(&vid_player.seek_duration_button.selected);
	Util_add_watch(&vid_player.correct_aspect_ratio_button.selected);
	Util_add_watch(&vid_player.move_content_button.selected);
	Util_add_watch(&vid_player.remember_video_pos_button.selected);
	Util_add_watch(&vid_player.playback_mode_button.selected);
	Util_add_watch(&vid_player.menu_background.selected);
	Util_add_watch(&vid_player.disable_audio_button.selected);
	Util_add_watch(&vid_player.disable_video_button.selected);
	Util_add_watch(&vid_player.disable_subtitle_button.selected);
	Util_add_watch(&vid_player.use_hw_decoding_button.selected);
	Util_add_watch(&vid_player.use_hw_color_conversion_button.selected);
	Util_add_watch(&vid_player.use_multi_threaded_decoding_button.selected);
	Util_add_watch(&vid_player.lower_resolution_button.selected);
	Util_add_watch(&vid_player.show_decode_graph_button.selected);
	Util_add_watch(&vid_player.show_color_conversion_graph_button.selected);
	Util_add_watch(&vid_player.show_packet_buffer_graph_button.selected);
	Util_add_watch(&vid_player.show_raw_video_buffer_graph_button.selected);
	Util_add_watch(&vid_player.show_raw_audio_buffer_graph_button.selected);
	Util_add_watch(&vid_player.restart_playback_threshold_bar.selected);
	Util_add_watch(&vid_player.seek_bar.selected);

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		Util_add_watch(&vid_player.audio_track_button[i].selected);

	for(int i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)
		Util_add_watch(&vid_player.subtitle_track_button[i].selected);

	for(int i = 0; i < 3; i++)
		Util_add_watch(&vid_player.menu_button[i].selected);

	result = Util_queue_create(&vid_player.decode_thread_command_queue, 200);
	Util_log_save(DEF_VID_INIT_STR, "Util_queue_create()..." + result.string + result.error_description, result.code);

	result = Util_queue_create(&vid_player.decode_thread_notification_queue, 100);
	Util_log_save(DEF_VID_INIT_STR, "Util_queue_create()..." + result.string + result.error_description, result.code);

	result = Util_queue_create(&vid_player.read_packet_thread_command_queue, 200);
	Util_log_save(DEF_VID_INIT_STR, "Util_queue_create()..." + result.string + result.error_description, result.code);

	result = Util_queue_create(&vid_player.decode_video_thread_command_queue, 200);
	Util_log_save(DEF_VID_INIT_STR, "Util_queue_create()..." + result.string + result.error_description, result.code);

	result = Util_queue_create(&vid_player.convert_thread_command_queue, 200);
	Util_log_save(DEF_VID_INIT_STR, "Util_queue_create()..." + result.string + result.error_description, result.code);

	vid_status += "\nStarting threads...";
	vid_thread_run = true;
	if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
	{
		vid_player.decode_thread = threadCreate(Vid_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_player.decode_video_thread = threadCreate(Vid_decode_video_thread, (void*)(""), 1024 * 1024, DEF_THREAD_PRIORITY_HIGH, var_core_2_available ? 2 : 0, false);
		vid_player.convert_thread = threadCreate(Vid_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 0, false);
		vid_player.read_packet_thread = threadCreate(Vid_read_packet_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 1, false);
	}
	else
	{
		vid_player.decode_thread = threadCreate(Vid_decode_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_player.decode_video_thread = threadCreate(Vid_decode_video_thread, (void*)(""), 1024 * 1024, DEF_THREAD_PRIORITY_HIGH, 0, false);
		vid_player.convert_thread = threadCreate(Vid_convert_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
		vid_player.read_packet_thread = threadCreate(Vid_read_packet_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_HIGH, 1, false);
	}

	vid_status += "\nLoading settings...";
	result = Util_file_load_from_file("vid_settings.txt", DEF_MAIN_DIR, &cache, 0x1000, &read_size);
	Util_log_save(DEF_VID_INIT_STR, "Util_file_load_from_file()..." + result.string + result.error_description, result.code);

	if(result.code == 0)
	{
		result = Util_parse_file((char*)cache, 17, out_data);//settings file for v1.5.1
		Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);

		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 16, out_data);//settings file for v1.5.0
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[16] = "48";
		}
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 13, out_data);//settings file for v1.4.2
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
			out_data[16] = "48";
		}
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 12, out_data);//settings file for v1.3.2, v1.3.3, v1.4.0 and v1.4.1
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[12] = "0";
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
			out_data[16] = "48";
		}
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 9, out_data);//settings file for v1.3.1
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[9] = "1";
			out_data[10] = "0";
			out_data[11] = "1";
			out_data[12] = "0";
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
			out_data[16] = "48";
		}
		if(result.code != 0)
		{
			result = Util_parse_file((char*)cache, 7, out_data);//settings file for v1.3.0
			Util_log_save(DEF_VID_INIT_STR , "Util_parse_file()..." + result.string + result.error_description, result.code);
			out_data[7] = "100";
			out_data[8] = "10";
			out_data[9] = "1";
			out_data[10] = "0";
			out_data[11] = "1";
			out_data[12] = "0";
			out_data[13] = "0";
			out_data[14] = "0";
			out_data[15] = "0";
			out_data[16] = "48";
		}
	}

	if(result.code == 0)
	{
		vid_player.use_linear_texture_filter = (out_data[0] == "1");
		vid_player.allow_skip_frames = (out_data[1] == "1");
		vid_player.allow_skip_key_frames = (out_data[2] == "1");
		vid_player.use_hw_decoding = (out_data[3] == "1");
		vid_player.use_hw_color_conversion = (out_data[4] == "1");
		vid_player.use_multi_threaded_decoding = (out_data[5] == "1");
		vid_player.lower_resolution = atoi((char*)out_data[6].c_str());
		vid_player.volume = atoi((char*)out_data[7].c_str());
		vid_player.seek_duration = atoi((char*)out_data[8].c_str());
		vid_player.correct_aspect_ratio = (out_data[9] == "1");
		vid_player.move_content_mode = atoi((char*)out_data[10].c_str());
		vid_player.remember_video_pos = (out_data[11] == "1");
		vid_player.playback_mode = atoi((char*)out_data[12].c_str());
		vid_player.disable_audio = (out_data[13] == "1");
		vid_player.disable_video = (out_data[14] == "1");
		vid_player.disable_subtitle = (out_data[15] == "1");
		vid_player.restart_playback_threshold = atoi((char*)out_data[16].c_str());
	}

	if(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL)
		vid_player.use_hw_decoding = false;

	if(!vid_player.allow_skip_frames)
		vid_player.allow_skip_key_frames = false;

	if(vid_player.lower_resolution > 2 || vid_player.lower_resolution < 0)
		vid_player.lower_resolution = 0;

	if(vid_player.volume > 999 || vid_player.volume < 0)
		vid_player.volume = 100;

	if(vid_player.seek_duration > 99 || vid_player.seek_duration < 1)
		vid_player.seek_duration = 10;

	if(vid_player.playback_mode != DEF_VID_NO_REPEAT && vid_player.playback_mode != DEF_VID_REPEAT 
		&& vid_player.playback_mode != DEF_VID_IN_ORDER && vid_player.playback_mode != DEF_VID_RANDOM)
		vid_player.playback_mode = DEF_VID_NO_REPEAT;

	if(vid_player.move_content_mode != DEF_VID_MOVE_BOTH && vid_player.move_content_mode != DEF_VID_MOVE_VIDEO
	&& vid_player.move_content_mode != DEF_VID_MOVE_SUBTITLE && vid_player.move_content_mode != DEF_VID_MOVE_DISABLE)
		vid_player.move_content_mode = DEF_VID_MOVE_BOTH;

	if(vid_player.seek_duration >= DEF_DECODER_MAX_RAW_IMAGE || vid_player.restart_playback_threshold < 0)
		vid_player.restart_playback_threshold = 48 >= DEF_DECODER_MAX_RAW_IMAGE ? DEF_DECODER_MAX_RAW_IMAGE : 48;

	Util_safe_linear_free(cache);
	cache = NULL;

	vid_status += "\nLoading textures...";
	vid_player.banner_texture_handle = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/banner.t3x", vid_player.banner_texture_handle, vid_player.banner, 0, 2);
	Util_log_save(DEF_VID_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	vid_player.control_texture_handle = Draw_get_free_sheet_num();
	result = Draw_load_texture("romfs:/gfx/draw/video_player/control.t3x", vid_player.control_texture_handle, vid_player.control, 0, 2);
	Util_log_save(DEF_VID_INIT_STR, "Draw_load_texture()..." + result.string + result.error_description, result.code);

	vid_already_init = true;

	Util_log_save(DEF_VID_INIT_STR, "Thread exit.");
	threadExit(0);
}

void Vid_exit_thread(void* arg)
{
	Util_log_save(DEF_VID_EXIT_STR, "Thread started.");
	std::string data = "";
	Result_with_string result;

	vid_already_init = false;
	vid_thread_suspend = false;
	vid_player.is_selecting_audio_track = false;
	vid_player.is_selecting_subtitle_track = false;

	DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_EXIT_STR, &vid_player.decode_thread_command_queue,
	DECODE_THREAD_SHUTDOWN_REQUEST, NULL, 100000, QUEUE_OPTION_SEND_TO_FRONT)

	vid_status = "Saving settings...";
	data = "<0>" + std::to_string(vid_player.use_linear_texture_filter) + "</0><1>" + std::to_string(vid_player.allow_skip_frames) + "</1><2>"
	+ std::to_string(vid_player.allow_skip_key_frames) + "</2><3>" + std::to_string(vid_player.use_hw_decoding) + "</3><4>"
	+ std::to_string(vid_player.use_hw_color_conversion) + "</4><5>" + std::to_string(vid_player.use_multi_threaded_decoding) + "</5><6>"
	+ std::to_string(vid_player.lower_resolution) + "</6><7>"	+ std::to_string(vid_player.volume) + "</7><8>"
	+ std::to_string(vid_player.seek_duration) + "</8><9>" + std::to_string(vid_player.correct_aspect_ratio) + "</9><10>"
	+ std::to_string(vid_player.move_content_mode) + "</10><11>" + std::to_string(vid_player.remember_video_pos) + "</11><12>"
	+ std::to_string(vid_player.playback_mode) + "</12><13>" + std::to_string(vid_player.disable_audio) + "</13><14>"
	+ std::to_string(vid_player.disable_video) + "</14><15>" + std::to_string(vid_player.disable_subtitle) + "</15><16>"
	+ std::to_string(vid_player.restart_playback_threshold) + "</16>";
	Util_file_save_to_file("vid_settings.txt", DEF_MAIN_DIR, (u8*)data.c_str(), data.length(), true);

	vid_status += "\nExiting threads...";
	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_player.decode_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_player.decode_video_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_player.convert_thread, DEF_THREAD_WAIT_TIME));
	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_player.read_packet_thread, DEF_THREAD_WAIT_TIME));

	vid_status += "\nCleaning up...";
	threadFree(vid_player.decode_thread);
	threadFree(vid_player.decode_video_thread);
	threadFree(vid_player.convert_thread);
	threadFree(vid_player.read_packet_thread);

	Draw_free_texture(vid_player.banner_texture_handle);
	Draw_free_texture(vid_player.control_texture_handle);

	Util_remove_watch((int*)&vid_player.state);
	Util_remove_watch(&vid_player.is_selecting_audio_track);
	Util_remove_watch(&vid_player.is_selecting_subtitle_track);
	Util_remove_watch(&vid_player.is_displaying_controls);
	Util_remove_watch(&vid_player.is_setting_seek_duration);
	Util_remove_watch(&vid_player.is_setting_volume);
	Util_remove_watch(&vid_player.use_hw_decoding);
	Util_remove_watch(&vid_player.use_hw_color_conversion);
	Util_remove_watch(&vid_player.use_multi_threaded_decoding);

	Util_remove_watch(&vid_player.use_linear_texture_filter);
	Util_remove_watch(&vid_player.allow_skip_frames);
	Util_remove_watch(&vid_player.allow_skip_key_frames);
	Util_remove_watch(&vid_player.is_full_screen);
	Util_remove_watch(&vid_player.correct_aspect_ratio);
	Util_remove_watch(&vid_player.move_content_mode);
	Util_remove_watch(&vid_player.remember_video_pos);
	Util_remove_watch(&vid_player.playback_mode);
	Util_remove_watch(&vid_player.menu_mode);
	Util_remove_watch(&vid_player.lower_resolution);
	Util_remove_watch(&vid_player.show_decoding_graph);
	Util_remove_watch(&vid_player.show_color_conversion_graph);
	Util_remove_watch(&vid_player.show_packet_buffer_graph);
	Util_remove_watch(&vid_player.show_raw_video_buffer_graph);
	Util_remove_watch(&vid_player.show_raw_audio_buffer_graph);

	Util_remove_watch(&vid_player.seek_pos);
	Util_remove_watch(&vid_player.ui_y_offset);
	Util_remove_watch(&vid_player.ui_y_offset_max);
	Util_remove_watch(&vid_player.subtitle_x_offset);
	Util_remove_watch(&vid_player.subtitle_y_offset);
	Util_remove_watch(&vid_player.subtitle_zoom);
	Util_remove_watch(&vid_player.video_x_offset);
	Util_remove_watch(&vid_player.video_y_offset);
	Util_remove_watch(&vid_player.video_zoom);
	Util_remove_watch(&vid_player.selected_audio_track_cache);
	Util_remove_watch(&vid_player.selected_subtitle_track_cache);
	Util_remove_watch(&vid_player.restart_playback_threshold);

	Util_remove_watch(&vid_player.ok_button.selected);
	Util_remove_watch(&vid_player.control_button.selected);
	Util_remove_watch(&vid_player.scroll_bar.selected);
	Util_remove_watch(&vid_player.select_audio_track_button.selected);
	Util_remove_watch(&vid_player.select_subtitle_track_button.selected);
	Util_remove_watch(&vid_player.texture_filter_button.selected);
	Util_remove_watch(&vid_player.allow_skip_frames_button.selected);
	Util_remove_watch(&vid_player.allow_skip_key_frames_button.selected);
	Util_remove_watch(&vid_player.volume_button.selected);
	Util_remove_watch(&vid_player.seek_duration_button.selected);
	Util_remove_watch(&vid_player.correct_aspect_ratio_button.selected);
	Util_remove_watch(&vid_player.move_content_button.selected);
	Util_remove_watch(&vid_player.remember_video_pos_button.selected);
	Util_remove_watch(&vid_player.playback_mode_button.selected);
	Util_remove_watch(&vid_player.menu_background.selected);
	Util_remove_watch(&vid_player.disable_audio_button.selected);
	Util_remove_watch(&vid_player.disable_video_button.selected);
	Util_remove_watch(&vid_player.disable_subtitle_button.selected);
	Util_remove_watch(&vid_player.use_hw_decoding_button.selected);
	Util_remove_watch(&vid_player.use_hw_color_conversion_button.selected);
	Util_remove_watch(&vid_player.use_multi_threaded_decoding_button.selected);
	Util_remove_watch(&vid_player.lower_resolution_button.selected);
	Util_remove_watch(&vid_player.show_decode_graph_button.selected);
	Util_remove_watch(&vid_player.show_color_conversion_graph_button.selected);
	Util_remove_watch(&vid_player.show_packet_buffer_graph_button.selected);
	Util_remove_watch(&vid_player.show_raw_video_buffer_graph_button.selected);
	Util_remove_watch(&vid_player.show_raw_audio_buffer_graph_button.selected);
	Util_remove_watch(&vid_player.restart_playback_threshold_bar.selected);
	Util_remove_watch(&vid_player.seek_bar.selected);

	for(int i = 0; i < DEF_DECODER_MAX_AUDIO_TRACKS; i++)
		Util_remove_watch(&vid_player.audio_track_button[i].selected);

	for(int i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)
		Util_remove_watch(&vid_player.subtitle_track_button[i].selected);

	for(int i = 0; i < 3; i++)
		Util_remove_watch(&vid_player.menu_button[i].selected);

	Util_queue_delete(&vid_player.decode_thread_command_queue);
	Util_queue_delete(&vid_player.decode_thread_notification_queue);
	Util_queue_delete(&vid_player.read_packet_thread_command_queue);
	Util_queue_delete(&vid_player.decode_video_thread_command_queue);
	Util_queue_delete(&vid_player.convert_thread_command_queue);

	vid_already_init = false;

	Util_log_save(DEF_VID_EXIT_STR, "Thread exit.");
	threadExit(0);
}

void Vid_init(bool draw)
{
	Util_log_save(DEF_VID_INIT_STR, "Initializing...");
	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	Util_add_watch(&vid_status);
	vid_status = "";

	if((var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_N3DS) && var_core_2_available)
		vid_init_thread = threadCreate(Vid_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	else
	{
		if(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DS || var_model == CFG_MODEL_N3DSXL)
			APT_SetAppCpuTimeLimit(80);
		else
			APT_SetAppCpuTimeLimit(70);

		vid_init_thread = threadCreate(Vid_init_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	}

	while(!vid_already_init)
	{
		if(draw)
		{
			if (var_night_mode)
			{
				color = DEF_DRAW_WHITE;
				back_color = DEF_DRAW_BLACK;
			}

			if(Util_is_watch_changed() || var_need_reflesh || !var_eco_mode)
			{
				var_need_reflesh = false;
				Draw_frame_ready();
				Draw_screen_ready(SCREEN_TOP_LEFT, back_color);
				Draw_top_ui();
				if(var_monitor_cpu_usage)
					Draw_cpu_usage_info();

				Draw(vid_status, 0, 20, 0.65, 0.65, color);

				Draw_apply_draw();
			}
			else
				gspWaitForVBlank();
		}
		else
			Util_sleep(20000);
	}

	if(!(var_model == CFG_MODEL_N2DSXL || var_model == CFG_MODEL_N3DSXL || var_model == CFG_MODEL_N3DS) || !var_core_2_available)
		APT_SetAppCpuTimeLimit(10);

	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_init_thread, DEF_THREAD_WAIT_TIME));	
	threadFree(vid_init_thread);
	Vid_resume();

	Util_log_save(DEF_VID_INIT_STR, "Initialized.");
}

void Vid_exit(bool draw)
{
	Util_log_save(DEF_VID_EXIT_STR, "Exiting...");

	int color = DEF_DRAW_BLACK;
	int back_color = DEF_DRAW_WHITE;

	vid_status = "";
	vid_exit_thread = threadCreate(Vid_exit_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	while(vid_already_init)
	{
		if(draw)
		{
			if (var_night_mode)
			{
				color = DEF_DRAW_WHITE;
				back_color = DEF_DRAW_BLACK;
			}

			if(Util_is_watch_changed() || var_need_reflesh || !var_eco_mode)
			{
				var_need_reflesh = false;
				Draw_frame_ready();
				Draw_screen_ready(SCREEN_TOP_LEFT, back_color);
				Draw_top_ui();
				if(var_monitor_cpu_usage)
					Draw_cpu_usage_info();

				Draw(vid_status, 0, 20, 0.65, 0.65, color);

				Draw_apply_draw();
			}
			else
				gspWaitForVBlank();
		}
		else
			Util_sleep(20000);
	}

	Util_log_save(DEF_VID_EXIT_STR, "threadJoin()...", threadJoin(vid_exit_thread, DEF_THREAD_WAIT_TIME));	
	threadFree(vid_exit_thread);
	Util_remove_watch(&vid_status);
	var_need_reflesh = true;

	Util_log_save(DEF_VID_EXIT_STR, "Exited.");
}

void Vid_main(void)
{
	int color = DEF_DRAW_BLACK;
	int disabled_color = DEF_DRAW_WEAK_BLACK;
	int back_color = DEF_DRAW_WHITE;
	int image_num = 0;
	int image_num_3d = 0;
	double image_width[DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, 0, };
	double image_height[DEF_DECODER_MAX_VIDEO_TRACKS] = { 0, 0, };
	double y_offset = 0;
	char msg_cache[128];
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

	if(vid_player.image_index - 1 >= 0)
		image_num = vid_player.image_index - 1;
	else
		image_num = DEF_VID_VIDEO_BUFFERS - 1;

	if(vid_player.image_index_3d - 1 >= 0)
		image_num_3d = vid_player.image_index_3d - 1;
	else
		image_num_3d = DEF_VID_VIDEO_BUFFERS - 1;

	Vid_control_full_screen();

	for(int i = 0; i < DEF_DECODER_MAX_VIDEO_TRACKS; i++)
	{
		if(i >= vid_player.num_of_video_tracks)
			break;

		image_width[i] = vid_player.video_info[i].codec_width * (vid_player.correct_aspect_ratio ? vid_player.video_info[i].sar_width : 1) * vid_player.video_zoom;
		image_height[i] = vid_player.video_info[i].codec_height * (vid_player.correct_aspect_ratio ? vid_player.video_info[i].sar_height : 1) * vid_player.video_zoom;
	}

	//Update performance data every 100ms.
	if(osGetTime() >= vid_player.previous_ts + 100)
	{
		vid_player.previous_ts = osGetTime();
		vid_player.packet_buffer_list[319] = Util_decoder_get_available_packet_num(0);
		vid_player.raw_audio_buffer_list[319] = Util_speaker_get_available_buffer_num(0);
		vid_player.raw_video_buffer_list[319] = (vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0);

		for(int i = 1; i < 320; i++)
			vid_player.packet_buffer_list[i - 1] = vid_player.packet_buffer_list[i];

		for(int i = 1; i < 320; i++)
			vid_player.raw_audio_buffer_list[i - 1] = vid_player.raw_audio_buffer_list[i];

		for(int i = 1; i < 320; i++)
			vid_player.raw_video_buffer_list[i - 1] = vid_player.raw_video_buffer_list[i];
	}


	if(Util_is_watch_changed() || var_need_reflesh || !var_eco_mode)
	{
		var_need_reflesh = false;
		Draw_frame_ready();

		if(var_turn_on_top_lcd)
		{
			std::string top_center_msg = "";
			std::string bottom_left_msg = "";
			std::string bottom_center_msg = "";
			Draw_screen_ready(SCREEN_TOP_LEFT, vid_player.is_full_screen ? DEF_DRAW_BLACK : back_color);

			if(!vid_player.is_full_screen)
				Draw_top_ui();

			if(vid_player.state != PLAYER_STATE_IDLE)
			{
				//Draw videos.
				Vid_large_texture_draw(&vid_player.large_image[image_num][0], vid_player.video_x_offset, vid_player.video_y_offset, image_width[0], image_height[0]);

				for(int i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)//Draw subtitles.
				{
					if(vid_player.video_current_pos >= vid_player.subtitle_data[i].start_time && vid_player.video_current_pos <= vid_player.subtitle_data[i].end_time)
					{
						Draw(vid_player.subtitle_data[i].text, vid_player.subtitle_x_offset, 195 + vid_player.subtitle_y_offset, 0.5 * vid_player.subtitle_zoom, 0.5 * vid_player.subtitle_zoom,
						DEF_DRAW_WHITE, X_ALIGN_CENTER, Y_ALIGN_CENTER, 400, 40, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);

						break;
					}
				}
			}
			else
				Draw_texture(vid_player.banner[var_night_mode], 0, 15, 400, 225);

			if(vid_player.is_full_screen && vid_player.turn_off_bottom_screen_count > 0 && vid_player.show_full_screen_msg)
			{
				//Display exit full screen message.
				top_center_msg += vid_msg[DEF_VID_FULL_SCREEN_MSG];
			}

			if(vid_player.show_screen_brightness_until >= osGetTime())
			{
				//Display current brightness.
				bottom_left_msg += vid_msg[DEF_VID_BRIGHTNESS_MSG] + std::to_string(var_lcd_brightness) + "/180";
			}
			if(vid_player.show_current_pos_until >= osGetTime())
			{
				double current_bar_pos = 0;

				if(bottom_left_msg != "")
					bottom_left_msg += "\n";

				if(vid_player.seek_bar.selected)
					current_bar_pos = vid_player.seek_pos_cache / 1000;
				else if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					current_bar_pos = vid_player.seek_pos / 1000;
				else
					current_bar_pos = vid_player.media_current_pos / 1000;

				//Display current video pos.
				bottom_left_msg += Util_convert_seconds_to_time(current_bar_pos) + "/" + Util_convert_seconds_to_time(vid_player.media_duration / 1000);
			}

			if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
			{
				//Display seeking message.
				bottom_center_msg += vid_msg[DEF_VID_SEEKING_MSG];
			}
			if(vid_player.state == PLAYER_STATE_BUFFERING)
			{
				if(bottom_center_msg != "")
					bottom_center_msg += "\n";

				//Display decoding message.
				bottom_center_msg += vid_msg[DEF_VID_PROCESSING_VIDEO_MSG];
			}

			if(top_center_msg != "")
			{
				Draw(top_center_msg, 0, 20, 0.45, 0.45, DEF_DRAW_WHITE, X_ALIGN_CENTER, Y_ALIGN_CENTER,
				400, 30, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
			}

			if(bottom_left_msg != "")
			{
				Draw(bottom_left_msg, 0, 200, 0.45, 0.45, DEF_DRAW_WHITE, X_ALIGN_LEFT, Y_ALIGN_BOTTOM,
				400, 40, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
			}

			if(bottom_center_msg != "")
			{
				Draw(bottom_center_msg, 0, 200, 0.5, 0.5, DEF_DRAW_WHITE, X_ALIGN_CENTER, Y_ALIGN_BOTTOM,
				400, 40, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
			}

			if(var_monitor_cpu_usage)
				Draw_cpu_usage_info();

			if(Util_log_query_log_show_flag())
				Util_log_draw();

			if(Draw_is_3d_mode())
			{
				Draw_screen_ready(SCREEN_TOP_RIGHT, vid_player.is_full_screen ? DEF_DRAW_BLACK : back_color);

				if(!vid_player.is_full_screen)
					Draw_top_ui();

				if(vid_player.state != PLAYER_STATE_IDLE)
				{
					//Draw 3d videos (right eye).
					Vid_large_texture_draw(&vid_player.large_image[image_num_3d][1], vid_player.video_x_offset, vid_player.video_y_offset, image_width[1], image_height[1]);

					for(int i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)//Draw subtitles.
					{
						if(vid_player.video_current_pos >= vid_player.subtitle_data[i].start_time && vid_player.video_current_pos <= vid_player.subtitle_data[i].end_time)
						{
							Draw(vid_player.subtitle_data[i].text, vid_player.subtitle_x_offset, 195 + vid_player.subtitle_y_offset, 0.5 * vid_player.subtitle_zoom, 0.5 * vid_player.subtitle_zoom, DEF_DRAW_WHITE, 
								X_ALIGN_CENTER, Y_ALIGN_CENTER, 400, 40, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
							break;
						}
					}
				}
				else
					Draw_texture(vid_player.banner[var_night_mode], 0, 15, 400, 225);

				if(top_center_msg != "")
				{
					Draw(top_center_msg, 0, 20, 0.45, 0.45, DEF_DRAW_WHITE, X_ALIGN_CENTER, Y_ALIGN_CENTER,
					400, 30, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
				}

				if(bottom_left_msg != "")
				{
					Draw(bottom_left_msg, 0, 200, 0.45, 0.45, DEF_DRAW_WHITE, X_ALIGN_LEFT, Y_ALIGN_BOTTOM,
					400, 40, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
				}

				if(bottom_center_msg != "")
				{
					Draw(bottom_center_msg, 0, 200, 0.5, 0.5, DEF_DRAW_WHITE, X_ALIGN_CENTER, Y_ALIGN_BOTTOM,
					400, 40, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
				}

				if(var_monitor_cpu_usage)
					Draw_cpu_usage_info();

				if(Util_log_query_log_show_flag())
					Util_log_draw();
			}
		}

		if(var_turn_on_bottom_lcd)
		{
			if(var_model == CFG_MODEL_2DS && vid_player.turn_off_bottom_screen_count == 0 && vid_player.is_full_screen)
				Draw_screen_ready(SCREEN_BOTTOM, DEF_DRAW_BLACK);//OLD2DS can't turn only bottom screen off, so just fill with black.
			else
			{
				double current_bar_pos = 0;

				Draw_screen_ready(SCREEN_BOTTOM, back_color);
				Draw(DEF_VID_VER, 0, 0, 0.425, 0.425, DEF_DRAW_GREEN);

				//Draw codec info.
				Draw("A : " + vid_player.audio_info[vid_player.selected_audio_track].format_name, 0, 10, 0.45, 0.45, color);
				Draw("V : " + vid_player.video_info[0].format_name, 0, 19, 0.45, 0.45, color);
				Draw("S : " + vid_player.subtitle_info[vid_player.selected_subtitle_track].format_name, 0, 28, 0.45, 0.45, color);
				Draw(std::to_string(vid_player.video_info[0].width) + "x" + std::to_string(vid_player.video_info[0].height) + "(" + std::to_string(vid_player.video_info[0].codec_width) + "x" + std::to_string(vid_player.video_info[0].codec_height) + ")" + "@" + std::to_string(vid_player.video_info[0].framerate).substr(0, 5) + "fps", 0, 37, 0.45, 0.45, color);

				if(vid_player.state != PLAYER_STATE_IDLE)
				{
					//Draw videos.
					Vid_large_texture_draw(&vid_player.large_image[image_num][0], vid_player.video_x_offset - 40, vid_player.video_y_offset - 240, image_width[0], image_height[0]);

					for(int i = 0; i < DEF_VID_SUBTITLE_BUFFERS; i++)//Draw subtitles.
					{
						if(vid_player.video_current_pos >= vid_player.subtitle_data[i].start_time && vid_player.video_current_pos <= vid_player.subtitle_data[i].end_time)
						{
							Draw(vid_player.subtitle_data[i].text, vid_player.subtitle_x_offset, 195 + vid_player.subtitle_y_offset - 240, 0.5 * vid_player.subtitle_zoom, 0.5 * vid_player.subtitle_zoom, DEF_DRAW_WHITE, 
								X_ALIGN_CENTER, Y_ALIGN_CENTER, 320, 40, BACKGROUND_UNDER_TEXT, var_square_image[0], 0xA0000000);
							break;
						}
					}
				}

				if(vid_player.menu_mode != DEF_VID_MENU_NONE)
					Draw_texture(&vid_player.menu_background, DEF_DRAW_WEAK_GREEN, 0, 50, 320, 130);

				if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_0)
				{
					//scroll bar
					Draw_texture(&vid_player.scroll_bar, vid_player.scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_player.ui_y_offset / vid_player.ui_y_offset_max * 120) + 50, 7, 10);

					y_offset = 60;
					//playback mode
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_PLAY_METHOD_MSG] + vid_msg[DEF_VID_NO_REPEAT_MSG + vid_player.playback_mode], 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.playback_mode_button, vid_player.playback_mode_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.playback_mode_button.x_size = -1;
						vid_player.playback_mode_button.y_size = -1;
					}

					y_offset += 25;
					//volume
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_VOLUME_MSG] + std::to_string(vid_player.volume) + "%", 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, (vid_player.sub_state & PLAYER_SUB_STATE_TOO_BIG) ? DEF_DRAW_RED : color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.volume_button, vid_player.volume_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.volume_button.x_size = -1;
						vid_player.volume_button.y_size = -1;
					}

					y_offset += 25;
					//select audio track
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_AUDIO_TRACK_MSG], 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.select_audio_track_button, vid_player.select_audio_track_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.select_audio_track_button.x_size = -1;
						vid_player.select_audio_track_button.y_size = -1;
					}

					y_offset += 25;
					//select subtitle track
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SUBTITLE_TRACK_MSG], 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.select_subtitle_track_button, vid_player.select_subtitle_track_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.select_subtitle_track_button.x_size = -1;
						vid_player.select_subtitle_track_button.y_size = -1;
					}

					y_offset += 25;
					//seek duration
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SEEK_MSG] + std::to_string(vid_player.seek_duration) + "s", 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.seek_duration_button, vid_player.seek_duration_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.seek_duration_button.x_size = -1;
						vid_player.seek_duration_button.y_size = -1;
					}

					y_offset += 25;
					//remember video pos
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_REMEMBER_POS_MSG] + (vid_player.remember_video_pos ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.remember_video_pos_button, vid_player.remember_video_pos_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.remember_video_pos_button.x_size = -1;
						vid_player.remember_video_pos_button.y_size = -1;
					}

					y_offset += 25;
					//texture filter
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_TEX_FILTER_MSG] + (vid_player.use_linear_texture_filter ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.texture_filter_button, vid_player.texture_filter_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.texture_filter_button.x_size = -1;
						vid_player.texture_filter_button.y_size = -1;
					}

					y_offset += 25;
					//correct aspect ratio
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_ASPECT_RATIO_MSG] + (vid_player.correct_aspect_ratio ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.correct_aspect_ratio_button, vid_player.correct_aspect_ratio_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.correct_aspect_ratio_button.x_size = -1;
						vid_player.correct_aspect_ratio_button.y_size = -1;
					}
				
					y_offset += 25;
					//move content mode
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_MOVE_MODE_MSG] + vid_msg[DEF_VID_MOVE_MODE_EDIABLE_MSG + vid_player.move_content_mode], 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.move_content_button, vid_player.move_content_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.move_content_button.x_size = -1;
						vid_player.move_content_button.y_size = -1;
					}

					y_offset += 25;
					//allow skip frames
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SKIP_FRAME_MSG] + (vid_player.allow_skip_frames ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.allow_skip_frames_button, vid_player.allow_skip_frames_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.allow_skip_frames_button.x_size = -1;
						vid_player.allow_skip_frames_button.y_size = -1;
					}

					y_offset += 25;
					//allow skip key frames
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_SKIP_KEY_FRAME_MSG] + (vid_player.allow_skip_key_frames ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.5, 0.5, vid_player.allow_skip_frames ? color : disabled_color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 15,
						BACKGROUND_ENTIRE_BOX, &vid_player.allow_skip_key_frames_button, vid_player.allow_skip_key_frames_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.allow_skip_key_frames_button.x_size = -1;
						vid_player.allow_skip_key_frames_button.y_size = -1;
					}

					y_offset += 35;
					//restart playback threshold
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						snprintf(msg_cache, sizeof(msg_cache), vid_msg[DEF_VID_RESTART_PLAYBACK_THRESHOLD_MSG].c_str(), vid_player.restart_playback_threshold);
						Draw(msg_cache, 12.5, y_offset + vid_player.ui_y_offset - 15, 0.5, 0.5, color);
						Draw_texture(var_square_image[0], DEF_DRAW_WEAK_BLACK, 12.5, y_offset + vid_player.ui_y_offset + 7.5, 300, 5);
						Draw_texture(&vid_player.restart_playback_threshold_bar, vid_player.restart_playback_threshold_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED,
						((double)vid_player.restart_playback_threshold / (DEF_DECODER_MAX_RAW_IMAGE - 1) * 290) + 12.5, y_offset + vid_player.ui_y_offset, 10, 20);
					}
					else
					{
						vid_player.restart_playback_threshold_bar.x_size = -1;
						vid_player.restart_playback_threshold_bar.y_size = -1;
					}

					Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 0, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[1], vid_player.menu_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[2], vid_player.menu_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
				}
				else if(vid_player.menu_mode == DEF_VID_MENU_SETTINGS_1)
				{
					//scroll bar
					Draw_texture(&vid_player.scroll_bar, vid_player.scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_player.ui_y_offset / vid_player.ui_y_offset_max * 120) + 50, 7, 10);

					y_offset = 60;
					//disable audio
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_DISABLE_AUDIO_MSG] + (vid_player.disable_audio ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 20,
						BACKGROUND_ENTIRE_BOX, &vid_player.disable_audio_button, vid_player.disable_audio_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.disable_audio_button.x_size = -1;
						vid_player.disable_audio_button.y_size = -1;
					}

					y_offset += 30;
					//disable video
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_DISABLE_VIDEO_MSG] + (vid_player.disable_video ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 20,
						BACKGROUND_ENTIRE_BOX, &vid_player.disable_video_button, vid_player.disable_video_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.disable_video_button.x_size = -1;
						vid_player.disable_video_button.y_size = -1;
					}

					y_offset += 30;
					//disable subtitle
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_DISABLE_SUBTITLE_MSG] + (vid_player.disable_subtitle ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 20,
						BACKGROUND_ENTIRE_BOX, &vid_player.disable_subtitle_button, vid_player.disable_subtitle_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.disable_subtitle_button.x_size = -1;
						vid_player.disable_subtitle_button.y_size = -1;
					}

					y_offset += 30;
					//use hw decoding
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_HW_DECODER_MSG] + (vid_player.use_hw_decoding ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, 
						(var_model == CFG_MODEL_2DS || var_model == CFG_MODEL_3DS || var_model == CFG_MODEL_3DSXL || vid_player.state != PLAYER_STATE_IDLE) ? disabled_color : color,
						X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 20, BACKGROUND_ENTIRE_BOX, &vid_player.use_hw_decoding_button, vid_player.use_hw_decoding_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.use_hw_decoding_button.x_size = -1;
						vid_player.use_hw_decoding_button.y_size = -1;
					}

					y_offset += 30;
					//use hw color conversion
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_HW_CONVERTER_MSG] + (vid_player.use_hw_color_conversion ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 20,
						BACKGROUND_ENTIRE_BOX, &vid_player.use_hw_color_conversion_button, vid_player.use_hw_color_conversion_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.use_hw_color_conversion_button.x_size = -1;
						vid_player.use_hw_color_conversion_button.y_size = -1;
					}

					y_offset += 30;
					//use multi-threaded decoding (in software decoding)
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_MULTI_THREAD_MSG] + (vid_player.use_multi_threaded_decoding ? "ON" : "OFF"), 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 20,
						BACKGROUND_ENTIRE_BOX, &vid_player.use_multi_threaded_decoding_button, vid_player.use_multi_threaded_decoding_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.use_multi_threaded_decoding_button.x_size = -1;
						vid_player.use_multi_threaded_decoding_button.y_size = -1;
					}

					y_offset += 30;
					//lower resolution
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 165)
					{
						Draw(vid_msg[DEF_VID_LOWER_RESOLUTION_MSG] + lower_resolution_mode[vid_player.lower_resolution], 12.5, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.state != PLAYER_STATE_IDLE ? disabled_color : color, X_ALIGN_LEFT, Y_ALIGN_CENTER, 300, 20,
						BACKGROUND_ENTIRE_BOX, &vid_player.lower_resolution_button, vid_player.lower_resolution_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);
					}
					else
					{
						vid_player.lower_resolution_button.x_size = -1;
						vid_player.lower_resolution_button.y_size = -1;
					}

					Draw_texture(&vid_player.menu_button[0], vid_player.menu_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
					Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 110, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[2], vid_player.menu_button[2].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 220, 180, 100, 8);
				}
				else if(vid_player.menu_mode == DEF_VID_MENU_INFO)
				{
					//scroll bar
					Draw_texture(&vid_player.scroll_bar, vid_player.scroll_bar.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 313, (vid_player.ui_y_offset / vid_player.ui_y_offset_max * 120) + 50, 7, 10);

					y_offset = 160;
					//color conversion time
					if(vid_player.show_color_conversion_graph)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - (vid_player.conversion_time_list[i] / 2) + vid_player.ui_y_offset, DEF_DRAW_BLUE, i + 1, y_offset - (vid_player.conversion_time_list[i + 1] / 2) + vid_player.ui_y_offset, DEF_DRAW_BLUE, 1);
					}
					//decoding time
					if(vid_player.show_decoding_graph)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - (vid_player.video_decoding_time_list[i] / 2) + vid_player.ui_y_offset, DEF_DRAW_RED, i + 1, y_offset - (vid_player.video_decoding_time_list[i + 1] / 2) + vid_player.ui_y_offset, DEF_DRAW_RED, 1);

						Draw_line(320 - ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0)),
						y_offset + vid_player.ui_y_offset - 110, DEF_DRAW_WEAK_RED, 320 - ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0)),
						y_offset + vid_player.ui_y_offset + 8, DEF_DRAW_WEAK_RED, 2);
					}
					//compressed buffer
					if(vid_player.show_packet_buffer_graph)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_player.packet_buffer_list[i] / 3.0 + vid_player.ui_y_offset, 0xFFFF00FF, i + 1, y_offset - vid_player.packet_buffer_list[i + 1] / 3.0 + vid_player.ui_y_offset, 0xFFFF00FF, 1);
					}
					//raw video buffer
					if(vid_player.show_raw_video_buffer_graph)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_player.raw_video_buffer_list[i] / 1.5 + vid_player.ui_y_offset, 0xFF2060FF, i + 1, y_offset - vid_player.raw_video_buffer_list[i + 1] / 1.5 + vid_player.ui_y_offset, 0xFF2060FF, 1);
					}
					//raw audio buffer
					if(vid_player.show_raw_audio_buffer_graph)
					{
						for(int i = 0; i < 319; i++)
							Draw_line(i, y_offset - vid_player.raw_audio_buffer_list[i] / 6.0 + vid_player.ui_y_offset, 0xFF00A000, i + 1, y_offset - vid_player.raw_audio_buffer_list[i + 1] / 6.0 + vid_player.ui_y_offset, 0xFF00A000, 1);
					}

					//bottom line
					Draw_line(0, y_offset + vid_player.ui_y_offset, color, 320, y_offset + vid_player.ui_y_offset, color, 1);
					//deadline
					Draw_line(0, y_offset + vid_player.ui_y_offset - (vid_player.video_frametime / 2), 0xFF606060, 320, y_offset - (vid_player.video_frametime / 2) + vid_player.ui_y_offset, 0xFF606060, 1);

					//key frame
					if(vid_player.show_decoding_graph)
					{
						for(int i = 0; i < 319; i++)
						{
							if(vid_player.keyframe_list[i])
								Draw_line(i, y_offset + vid_player.ui_y_offset, disabled_color, i, y_offset - 110 + vid_player.ui_y_offset, disabled_color, 2);
						}
					}

					//compressed buffer button 
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						Draw_texture(&vid_player.show_packet_buffer_graph_button, vid_player.show_packet_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 160, 10);
						Draw("Compressed buffer : " + std::to_string(Util_decoder_get_available_packet_num(0)), 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_packet_buffer_graph ? 0xFFFF00FF : color);
					}
					else
					{
						vid_player.show_packet_buffer_graph_button.x_size = -1;
						vid_player.show_packet_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					//raw video and audio buffer button
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						Draw_texture(&vid_player.show_raw_video_buffer_graph_button, vid_player.show_raw_video_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 160, 10);
						if(vid_player.video_frametime != 0)
						{
							Draw("Raw video buffer : " + std::to_string((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0))
							+ "(" + std::to_string(int(((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0)) * vid_player.video_frametime)) + "ms)",
							0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_raw_video_buffer_graph ? 0xFF2060FF : color);
						}
						else
						{
							Draw("Raw video buffer : " + std::to_string((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? Util_mvd_video_decoder_get_available_raw_image_num(0) : Util_video_decoder_get_available_raw_image_num(0, 0)), 
							0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_raw_video_buffer_graph ? 0xFF2060FF : color);
						}
						Draw("frames : " + std::to_string(vid_player.total_frames), 160, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color,
						X_ALIGN_CENTER, Y_ALIGN_CENTER, 160, 10);
					}
					else
					{
						vid_player.show_raw_video_buffer_graph_button.x_size = -1;
						vid_player.show_raw_video_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						Draw_texture(&vid_player.show_raw_audio_buffer_graph_button, vid_player.show_raw_audio_buffer_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 160, 10);
						if(vid_player.audio_info[vid_player.selected_audio_track].ch != 0 && vid_player.audio_info[vid_player.selected_audio_track].sample_rate != 0)
						{
							Draw("Raw audio buffer : " + std::to_string(Util_speaker_get_available_buffer_num(0)) + "(" 
							+ std::to_string((int)(Util_speaker_get_available_buffer_size(0) / 2.0 / vid_player.audio_info[vid_player.selected_audio_track].ch / vid_player.audio_info[vid_player.selected_audio_track].sample_rate * 1000)) + "ms)",
							0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_raw_audio_buffer_graph ? 0xFF00A000 : color);
						}
						else
							Draw("Raw audio buffer : " + std::to_string(Util_speaker_get_available_buffer_num(0)), 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_raw_audio_buffer_graph ? 0xFF00A000 : color);
					}
					else
					{
						vid_player.show_raw_audio_buffer_graph_button.x_size = -1;
						vid_player.show_raw_audio_buffer_graph_button.y_size = -1;
					}

					y_offset += 10;
					//Deadline text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
						Draw("Deadline : " + std::to_string(vid_player.video_frametime).substr(0, 5) + "ms", 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, 0xFF606060);

					y_offset += 10;
					//Decoding time button.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
						Draw_texture(&vid_player.show_decode_graph_button, vid_player.show_decode_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 200, 10);
					else
					{
						vid_player.show_decode_graph_button.x_size = -1;
						vid_player.show_decode_graph_button.y_size = -1;
					}

					//Video decoding time and decoding mode text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						double decoding_time_avg = 0;
						int divisor = 90;

						for(int i = 319; i >= 230; i--)
						{
							if(vid_player.video_decoding_time_list[i] == 0)
							{
								divisor = 320 - (i + 1);
								break;
							}

							decoding_time_avg += vid_player.video_decoding_time_list[i];
						}

						if(divisor != 0)
							decoding_time_avg /= divisor;

						snprintf(msg_cache, sizeof(msg_cache), "Video decoding (avg) : %.3fms", decoding_time_avg);
						Draw(msg_cache, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_decoding_graph ? DEF_DRAW_RED : color);

						snprintf(msg_cache, sizeof(msg_cache), "Hw decoding : %s", ((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? "yes" : "no"));
						Draw(msg_cache, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					y_offset += 10;
					//Audio decoding time and thread mode text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						double decoding_time_avg = 0;
						int divisor = 90;

						for(int i = 319; i >= 230; i--)
						{
							if(vid_player.audio_decoding_time_list[i] == 0)
							{
								divisor = 320 - (i + 1);
								break;
							}

							decoding_time_avg += vid_player.audio_decoding_time_list[i];
						}

						if(divisor != 0)
							decoding_time_avg /= divisor;

						snprintf(msg_cache, sizeof(msg_cache), "Audio decoding (avg) : %.3fms", decoding_time_avg);
						Draw(msg_cache, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_decoding_graph ? 0xFF800080 : color);

						snprintf(msg_cache, sizeof(msg_cache), "Thread type : %s", thread_mode[((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) ? 0 : vid_player.video_info[0].thread_type)].c_str());
						Draw(msg_cache, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					y_offset += 10;
					//Color conversion button.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
						Draw_texture(&vid_player.show_color_conversion_graph_button, vid_player.show_color_conversion_graph_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN, 0, y_offset + vid_player.ui_y_offset, 200, 10);
					else
					{
						vid_player.show_color_conversion_graph_button.x_size = -1;
						vid_player.show_color_conversion_graph_button.y_size = -1;
					}

					//Color conversion time and conversion mode text.
					if(y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						double conversion_time_avg = 0;
						int divisor = 90;

						for(int i = 319; i >= 230; i--)
						{
							if(vid_player.conversion_time_list[i] == 0)
							{
								divisor = 320 - (i + 1);
								break;
							}

							conversion_time_avg += vid_player.conversion_time_list[i];
						}

						if(divisor != 0)
							conversion_time_avg /= divisor;

						snprintf(msg_cache, sizeof(msg_cache), "Color conversion (avg) : %.3fms", conversion_time_avg);
						Draw(msg_cache, 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, vid_player.show_color_conversion_graph ? DEF_DRAW_BLUE : color);

						snprintf(msg_cache, sizeof(msg_cache), "Hw conversion : %s", (((vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) || (vid_player.sub_state & PLAYER_SUB_STATE_HW_CONVERSION)) ? "yes" : "no"));
						Draw(msg_cache, 200, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					y_offset += 10;
					//Decoding speed.
					if(vid_player.total_frames != 0 && vid_player.decoding_min_time != 0 && vid_player.decoding_recent_total_time != 0 && y_offset + vid_player.ui_y_offset >= 50 && y_offset + vid_player.ui_y_offset <= 170)
					{
						if(!(vid_player.sub_state & PLAYER_SUB_STATE_HW_DECODING) && vid_player.video_info[0].thread_type == THREAD_TYPE_FRAME)
							Draw("*When thread_type == frame, the red graph is unusable", 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
						else
							Draw("avg/min/max/recent avg " + std::to_string(1000 / (vid_player.decoding_total_time / vid_player.total_frames)).substr(0, 5) + "/" + std::to_string(1000 / vid_player.decoding_max_time).substr(0, 5) 
							+  "/" + std::to_string(1000 / vid_player.decoding_min_time).substr(0, 5) + "/" + std::to_string(1000 / (vid_player.decoding_recent_total_time/ 90)).substr(0, 5) +  " fps", 0, y_offset + vid_player.ui_y_offset, 0.425, 0.425, color);
					}

					Draw_texture(&vid_player.menu_button[0], vid_player.menu_button[0].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 0, 180, 100, 8);
					Draw_texture(&vid_player.menu_button[1], vid_player.menu_button[1].selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA, 110, 180, 100, 8);
					Draw_texture(var_square_image[0], DEF_DRAW_YELLOW, 220, 180, 100, 8);
				}

				//controls
				Draw(vid_msg[DEF_VID_CONTROLS_MSG], 167.5, 195, 0.425, 0.425, color, X_ALIGN_CENTER, Y_ALIGN_CENTER, 145, 10,
				BACKGROUND_ENTIRE_BOX, &vid_player.control_button, vid_player.control_button.selected ? DEF_DRAW_AQUA : DEF_DRAW_WEAK_AQUA);

				//Draw time bar.
				if(vid_player.seek_bar.selected)
					current_bar_pos = vid_player.seek_pos_cache / 1000;
				else if(vid_player.state == PLAYER_STATE_SEEKING || vid_player.state == PLAYER_STATE_PREPARE_SEEKING)
					current_bar_pos = vid_player.seek_pos / 1000;
				else
					current_bar_pos = vid_player.media_current_pos / 1000;

				Draw(Util_convert_seconds_to_time(current_bar_pos) + "/" + Util_convert_seconds_to_time(vid_player.media_duration / 1000), 10, 192.5, 0.5, 0.5, color);
				Draw_texture(&vid_player.seek_bar, DEF_DRAW_GREEN, 5, 210, 310, 10);
				if(vid_player.media_duration != 0)
					Draw_texture(var_square_image[0], 0xFF800080, 5, 210, 310 * (current_bar_pos / (vid_player.media_duration / 1000)), 10);

				if(vid_player.is_displaying_controls)
				{
					Draw_texture(vid_player.control[var_night_mode], 80, 20, 160, 160);
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

				if(vid_player.is_selecting_audio_track)
				{
					Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 40, 20, 240, 140);
					Draw(vid_msg[DEF_VID_AUDIO_TRACK_DESCRIPTION_MSG], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

					for(int i = 0; i < vid_player.num_of_audio_tracks; i++)
					{
						Draw_texture(&vid_player.audio_track_button[i], vid_player.audio_track_button[i].selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 40, 40 + (i * 12), 240, 12);
						Draw("Track " + std::to_string(i) + "(Lang:" + vid_player.audio_info[i].track_lang + ")", 42.5, 40 + (i * 12), 0.475, 0.475, i == vid_player.selected_audio_track_cache ? DEF_DRAW_RED : color);
					}

					Draw_texture(&vid_player.ok_button, vid_player.ok_button.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
					Draw("OK", 152.5, 140, 0.425, 0.425, DEF_DRAW_BLACK);
				}

				if(vid_player.is_selecting_subtitle_track)
				{
					Draw_texture(var_square_image[0], DEF_DRAW_GREEN, 40, 20, 240, 140);
					Draw(vid_msg[DEF_VID_SUBTITLE_TRACK_DESCRIPTION_MSG], 42.5, 25, 0.6, 0.6, DEF_DRAW_BLACK);

					for(int i = 0; i < vid_player.num_of_subtitle_tracks; i++)
					{
						Draw_texture(&vid_player.subtitle_track_button[i], vid_player.subtitle_track_button[i].selected ? DEF_DRAW_YELLOW : DEF_DRAW_WEAK_YELLOW, 40, 40 + (i * 12), 240, 12);
						Draw("Track " + std::to_string(i) + "(Lang:" + vid_player.subtitle_info[i].track_lang + ")", 42.5, 40 + (i * 12), 0.475, 0.475, i == vid_player.selected_subtitle_track_cache ? DEF_DRAW_RED : color);
					}

					Draw_texture(&vid_player.ok_button, vid_player.ok_button.selected ? DEF_DRAW_RED : DEF_DRAW_WEAK_RED, 150, 140, 20, 10);
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

	if(vid_player.is_setting_volume)
	{
		//Pause the video.
		DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_MAIN_STR, &vid_player.decode_thread_command_queue, 
		DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)

		Util_swkbd_init(SWKBD_TYPE_NUMPAD, SWKBD_NOTEMPTY, 1, 3, "", std::to_string(vid_player.volume));
		Util_swkbd_launch(&swkbd_input);
		vid_player.volume = atoi((char*)swkbd_input.c_str());

		//Resume the video.
		DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_MAIN_STR, &vid_player.decode_thread_command_queue,
		DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)

		var_need_reflesh = true;
		Util_swkbd_exit();
		vid_player.is_setting_volume = false;
	}
	else if(vid_player.is_setting_seek_duration)
	{
		//Pause the video.
		DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_MAIN_STR, &vid_player.decode_thread_command_queue,
		DECODE_THREAD_PAUSE_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)

		Util_swkbd_init(SWKBD_TYPE_NUMPAD, SWKBD_NOTEMPTY, 1, 2, "", std::to_string(vid_player.seek_duration));
		Util_swkbd_launch(&swkbd_input);
		vid_player.seek_duration = atoi((char*)swkbd_input.c_str());
		if(vid_player.seek_duration == 0)
			vid_player.seek_duration = 1;

		//Resume the video.
		DEF_VID_QUEUE_ADD_WITH_LOG(DEF_VID_MAIN_STR, &vid_player.decode_thread_command_queue,
		DECODE_THREAD_RESUME_REQUEST, NULL, 100000, QUEUE_OPTION_DO_NOT_ADD_IF_EXIST)

		var_need_reflesh = true;
		Util_swkbd_exit();
		vid_player.is_setting_seek_duration = false;
	}

	Vid_update_sleep_policy();
}
