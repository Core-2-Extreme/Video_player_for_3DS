#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>

#include <3ds.h>
#include <citro2d.h>

#include "system_definitions.hpp"

//These definitions are based on (stolen from) library\ffmpeg\include\libavutil\pixfmt.h see it for more information.
enum Pixel_format
{
	PIXEL_FORMAT_INVALID			= -1,
	//YUV*
	PIXEL_FORMAT_YUV410P			= 0,
	PIXEL_FORMAT_YUV411P			= 1,
	PIXEL_FORMAT_YUV420P			= 2,
	PIXEL_FORMAT_YUV420P9BE			= 3,
	PIXEL_FORMAT_YUV420P9LE			= 4,
	PIXEL_FORMAT_YUV420P10BE		= 5,
	PIXEL_FORMAT_YUV420P10LE		= 6,
	PIXEL_FORMAT_YUV420P12BE		= 7,
	PIXEL_FORMAT_YUV420P12LE		= 8,
	PIXEL_FORMAT_YUV420P14BE		= 9,
	PIXEL_FORMAT_YUV420P14LE		= 10,
	PIXEL_FORMAT_YUV420P16BE		= 11,
	PIXEL_FORMAT_YUV420P16LE		= 12,
	PIXEL_FORMAT_YUV422P			= 13,
	PIXEL_FORMAT_YUV422P9BE			= 14,
	PIXEL_FORMAT_YUV422P9LE			= 15,
	PIXEL_FORMAT_YUV422P10BE		= 16,
	PIXEL_FORMAT_YUV422P10LE		= 17,
	PIXEL_FORMAT_YUV422P12BE		= 18,
	PIXEL_FORMAT_YUV422P12LE		= 19,
	PIXEL_FORMAT_YUV422P14BE		= 20,
	PIXEL_FORMAT_YUV422P14LE		= 21,
	PIXEL_FORMAT_YUV422P16BE		= 22,
	PIXEL_FORMAT_YUV422P16LE		= 23,
	PIXEL_FORMAT_YUV440P			= 24,
	PIXEL_FORMAT_YUV440P10BE		= 25,
	PIXEL_FORMAT_YUV440P10LE		= 26,
	PIXEL_FORMAT_YUV440P12BE		= 27,
	PIXEL_FORMAT_YUV440P12LE		= 28,
	PIXEL_FORMAT_YUV444P			= 29,
	PIXEL_FORMAT_YUV444P9BE			= 30,
	PIXEL_FORMAT_YUV444P9LE			= 31,
	PIXEL_FORMAT_YUV444P10BE		= 32,
	PIXEL_FORMAT_YUV444P10LE		= 33,
	PIXEL_FORMAT_YUV444P12BE		= 34,
	PIXEL_FORMAT_YUV444P12LE		= 35,
	PIXEL_FORMAT_YUV444P14BE		= 36,
	PIXEL_FORMAT_YUV444P14LE		= 37,
	PIXEL_FORMAT_YUV444P16BE		= 38,
	PIXEL_FORMAT_YUV444P16LE		= 39,
	//YUVJ*
	PIXEL_FORMAT_YUVJ411P			= 40,
	PIXEL_FORMAT_YUVJ420P			= 41,
	PIXEL_FORMAT_YUVJ422P			= 42,
	PIXEL_FORMAT_YUVJ440P			= 43,
	PIXEL_FORMAT_YUVJ444P			= 44,
	//YUVA*
	PIXEL_FORMAT_YUVA420P			= 45,
	PIXEL_FORMAT_YUVA420P9BE		= 46,
	PIXEL_FORMAT_YUVA420P9LE		= 47,
	PIXEL_FORMAT_YUVA420P10BE		= 48,
	PIXEL_FORMAT_YUVA420P10LE		= 49,
	PIXEL_FORMAT_YUVA420P16BE		= 50,
	PIXEL_FORMAT_YUVA420P16LE		= 51,
	PIXEL_FORMAT_YUVA422P			= 52,
	PIXEL_FORMAT_YUVA422P9BE		= 53,
	PIXEL_FORMAT_YUVA422P9LE		= 54,
	PIXEL_FORMAT_YUVA422P10BE		= 55,
	PIXEL_FORMAT_YUVA422P10LE		= 56,
	PIXEL_FORMAT_YUVA422P16BE		= 57,
	PIXEL_FORMAT_YUVA422P16LE		= 58,
	PIXEL_FORMAT_YUVA444P			= 59,
	PIXEL_FORMAT_YUVA444P9BE		= 60,
	PIXEL_FORMAT_YUVA444P9LE		= 61,
	PIXEL_FORMAT_YUVA444P10BE		= 62,
	PIXEL_FORMAT_YUVA444P10LE		= 63,
	PIXEL_FORMAT_YUVA444P16BE		= 64,
	PIXEL_FORMAT_YUVA444P16LE		= 65,
	//UYVY*
	PIXEL_FORMAT_UYVY422			= 66,
	//YUYV*
	PIXEL_FORMAT_YUYV422			= 67,
	//YVYU*
	PIXEL_FORMAT_YVYU422			= 68,
	//UYYVYY*
	PIXEL_FORMAT_UYYVYY411			 = 69,
	//RGB* (exclude RGBA)
	PIXEL_FORMAT_RGB121				= 70,
	PIXEL_FORMAT_RGB121_BYTE		= 71,
	PIXEL_FORMAT_RGB332				= 72,
	PIXEL_FORMAT_RGB444BE			= 73,
	PIXEL_FORMAT_RGB444LE			= 74,
	PIXEL_FORMAT_RGB555BE			= 75,
	PIXEL_FORMAT_RGB555LE			= 76,
	PIXEL_FORMAT_RGB565BE			= 77,
	PIXEL_FORMAT_RGB565LE			= 78,
	PIXEL_FORMAT_RGB888				= 79,
	PIXEL_FORMAT_RGB161616BE		= 80,
	PIXEL_FORMAT_RGB161616LE		= 81,
	//BGR* (exclude BGRA)
	PIXEL_FORMAT_BGR121				= 82,
	PIXEL_FORMAT_BGR121_BYTE		= 83,
	PIXEL_FORMAT_BGR332				= 84,
	PIXEL_FORMAT_BGR444BE			= 85,
	PIXEL_FORMAT_BGR444LE			= 86,
	PIXEL_FORMAT_BGR555BE			= 87,
	PIXEL_FORMAT_BGR555LE			= 88,
	PIXEL_FORMAT_BGR565BE			= 89,
	PIXEL_FORMAT_BGR565LE			= 90,
	PIXEL_FORMAT_BGR888				= 91,
	PIXEL_FORMAT_BGR161616BE		= 92,
	PIXEL_FORMAT_BGR161616LE		= 93,
	//GBR* (exclude GRBA)
	PIXEL_FORMAT_GBR888P			= 94,
	PIXEL_FORMAT_GBR999PBE			= 95,
	PIXEL_FORMAT_GBR999PLE			= 96,
	PIXEL_FORMAT_GBR101010PBE		= 97,
	PIXEL_FORMAT_GBR101010PLE		= 98,
	PIXEL_FORMAT_GBR121212PBE		= 99,
	PIXEL_FORMAT_GBR121212PLE		= 100,
	PIXEL_FORMAT_GBR141414PBE		= 101,
	PIXEL_FORMAT_GBR141414PLE		= 102,
	PIXEL_FORMAT_GBR161616PBE		= 103,
	PIXEL_FORMAT_GBR161616PLE		= 104,
	//ARGB*
	PIXEL_FORMAT_ARGB8888			= 105,
	//ABGR*
	PIXEL_FORMAT_ABGR8888			= 106,
	//RGBA*
	PIXEL_FORMAT_RGBA8888			= 107,
	PIXEL_FORMAT_RGBA16161616BE		= 108,
	PIXEL_FORMAT_RGBA16161616LE		= 109,
	//BGRA*
	PIXEL_FORMAT_BGRA8888			= 110,
	PIXEL_FORMAT_BGRA16161616BE		= 111,
	PIXEL_FORMAT_BGRA16161616LE		= 112,
	//GBRA*
	PIXEL_FORMAT_GBRA8888P			= 113,
	PIXEL_FORMAT_GBRA10101010PBE	= 114,
	PIXEL_FORMAT_GBRA10101010PLE	= 115,
	PIXEL_FORMAT_GBRA12121212PBE	= 116,
	PIXEL_FORMAT_GBRA12121212PLE	= 117,
	PIXEL_FORMAT_GBRA16161616PBE	= 118,
	PIXEL_FORMAT_GBRA16161616PLE	= 119,
	//GRAY*
	PIXEL_FORMAT_GRAY8				= 120,
	PIXEL_FORMAT_GRAY10BE			= 121,
	PIXEL_FORMAT_GRAY10LE			= 122,
	PIXEL_FORMAT_GRAY12BE			= 123,
	PIXEL_FORMAT_GRAY12LE			= 124,
	PIXEL_FORMAT_GRAY16BE			= 125,
	PIXEL_FORMAT_GRAY16LE			= 126,
	//GRAYALPHA*
	PIXEL_FORMAT_GRAYALPHA88		= 127,
	PIXEL_FORMAT_GRAYALPHA1616BE	= 128,
	PIXEL_FORMAT_GRAYALPHA1616LE	= 129,

	PIXEL_FORMAT_MAX,
};

//These definitions are based on (stolen from) library\ffmpeg\include\libavutil\smaplefmt.h see it for more information.
enum Sample_format
{
	SAMPLE_FORMAT_INVALID	= -1,
	//Integer
	SAMPLE_FORMAT_U8		= 0,
	SAMPLE_FORMAT_U8P		= 1,
	SAMPLE_FORMAT_S16		= 2,
	SAMPLE_FORMAT_S16P		= 3,
	SAMPLE_FORMAT_S32		= 4,
	SAMPLE_FORMAT_S32P		= 5,
	SAMPLE_FORMAT_S64		= 6,
	SAMPLE_FORMAT_S64P		= 7,
	//Float
	SAMPLE_FORMAT_FLOAT32	= 8,
	SAMPLE_FORMAT_FLOAT32P	= 9,
	//Double
	SAMPLE_FORMAT_DOUBLE64	= 10,
	SAMPLE_FORMAT_DOUBLE64P	= 11,

	SAMPLE_FORMAT_MAX,
};

enum Screen
{
	SCREEN_INVALID		= -1,

	SCREEN_TOP_LEFT		= 0,	//Top screen for left eye.
	SCREEN_BOTTOM		= 1,	//Bottom screen.
	SCREEN_TOP_RIGHT	= 2,	//Top screen for right eye, this is used when 3D mode is enabled.

	SCREEN_MAX,
};

enum Text_align_x
{
	X_ALIGN_INVALID = -1,

	X_ALIGN_LEFT,	//Align text left (default).
	X_ALIGN_CENTER,	//Align text center.
	X_ALIGN_RIGHT,	//Align text right.

	X_ALIGN_MAX,
};

enum Text_align_y
{
	Y_ALIGN_INVALID = -1,

	Y_ALIGN_TOP,	//Align text top (default).
	Y_ALIGN_CENTER,	//Align text center.
	Y_ALIGN_BOTTOM,	//Align text bottom.

	Y_ALIGN_MAX,
};

enum Background
{
	BACKGROUND_INVALID = -1,

	BACKGROUND_NONE,		//No background texture (default).
	BACKGROUND_ENTIRE_BOX,	//Draw background texture entire box.
	BACKGROUND_UNDER_TEXT,	//Only draw background texture under text.

	BACKGROUND_MAX,
};

enum Camera_resolution
{
	CAM_RES_INVALID = -1,

	CAM_RES_640x480,
	CAM_RES_512x384,
	CAM_RES_400x240,
	CAM_RES_352x288,
	CAM_RES_320x240,
	CAM_RES_256x192,
	CAM_RES_176x144,
	CAM_RES_160x120,

	CAM_RES_MAX,
};

enum Camera_framerate
{
	CAM_FPS_INVALID = -1,

	CAM_FPS_15,
	CAM_FPS_15_TO_5,
	CAM_FPS_15_TO_2,
	CAM_FPS_10,
	CAM_FPS_8_5,
	CAM_FPS_5,
	CAM_FPS_20,
	CAM_FPS_20_TO_5,
	CAM_FPS_30,
	CAM_FPS_30_TO_5,
	CAM_FPS_15_TO_10,
	CAM_FPS_20_TO_10,
	CAM_FPS_30_TO_10,

	CAM_FPS_MAX,
};

enum Camera_lens_correction
{
	CAM_LENS_CORRECTION_INVALID = -1,

	CAM_LENS_CORRECTION_OFF,
	CAM_LENS_CORRECTION_70,
	CAM_LENS_CORRECTION_90,

	CAM_LENS_CORRECTION_MAX,
};

enum Camera_port
{
	CAM_PORT_INVALID = -1,

	CAM_PORT_OUT_LEFT,
	CAM_PORT_OUT_RIGHT,
	CAM_PORT_IN,

	CAM_PORT_MAX,
};

enum Camera_contrast
{
	CAM_CONTRAST_INVALID = -1,

	CAM_CONTRAST_01,
	CAM_CONTRAST_02,
	CAM_CONTRAST_03,
	CAM_CONTRAST_04,
	CAM_CONTRAST_05,
	CAM_CONTRAST_06,
	CAM_CONTRAST_07,
	CAM_CONTRAST_08,
	CAM_CONTRAST_09,
	CAM_CONTRAST_10,
	CAM_CONTRAST_11,

	CAM_CONTRAST_MAX,
};

enum Camera_white_balance
{
	CAM_WHITE_BALANCE_INVALID = -1,

	CAM_WHITE_BALANCE_AUTO,
	CAM_WHITE_BALANCE_3200K,
	CAM_WHITE_BALANCE_4150K,
	CAM_WHITE_BALANCE_5200K,
	CAM_WHITE_BALANCE_6000K,
	CAM_WHITE_BALANCE_7000K,

	CAM_WHITE_BALANCE_MAX,
};

enum Camera_exposure
{
	CAM_EXPOSURE_INVALID = -1,

	CAM_EXPOSURE_0,
	CAM_EXPOSURE_1,
	CAM_EXPOSURE_2,
	CAM_EXPOSURE_3,
	CAM_EXPOSURE_4,
	CAM_EXPOSURE_5,

	CAM_EXPOSURE_MAX,
};

enum Mic_sample_rate
{
	MIC_SAMPLE_RATE_INVALID = -1,

	MIC_SAMPLE_RATE_8182HZ,
	MIC_SAMPLE_RATE_10909HZ,
	MIC_SAMPLE_RATE_16364HZ,
	MIC_SAMPLE_RATE_32728HZ,

	MIC_SAMPLE_RATE_MAX,
};

enum Queue_option
{
	QUEUE_OPTION_NONE					= 0,		//Default.
	QUEUE_OPTION_DO_NOT_ADD_IF_EXIST	= (1 << 1), //Do not add the event if the same event id exist.
	QUEUE_OPTION_SEND_TO_FRONT			= (1 << 2), //Send an event to the front of the queue, use it for high priority event.
};

enum Wake_up_event
{
	WAKE_UP_EVENT_NONE				= 0,		//No wake up event.
	WAKE_UP_EVENT_PRESS_HOME_BUTTON	= (1 << 2), //Wake up if home button is pressed.
	WAKE_UP_EVENT_OPEN_SHELL		= (1 << 5), //Wake up if shell is opened.
};

enum File_type
{
	FILE_TYPE_NONE		= 0,	//File type is not set.
	FILE_TYPE_FILE		= 1,	//This entry is a file.
	FILE_TYPE_DIR		= 2,	//This entry is a directory.
	FILE_TYPE_READ_ONLY	= 4,	//This entry is read only.
	FILE_TYPE_HIDDEN	= 8,	//This entry is hidden.
};

enum Multi_thread_type
{
	THREAD_TYPE_INVALID = -1,

	THREAD_TYPE_NONE,	//No multi-threading, using single thread.
	THREAD_TYPE_FRAME,	//Frame level multi-threading.
	THREAD_TYPE_SLICE,	//Slice level multi-threading.
	THREAD_TYPE_AUTO,	//Auto (only used when request multi-threading mode).

	THREAD_TYPE_MAX,
};

enum Packet_type
{
	PACKET_TYPE_INVALID = -1,

	PACKET_TYPE_UNKNOWN,	//This packet contains unknown data.
	PACKET_TYPE_AUDIO,		//This packet contains audio data.
	PACKET_TYPE_VIDEO,		//This packet contains video data.
	PACKET_TYPE_SUBTITLE,	//This packet contains subtitle data.

	PACKET_TYPE_MAX,
};

enum Seek_flag
{
	SEEK_FLAG_NONE		= 0,		//No seek flag.
	SEEK_FLAG_BACKWARD	= (1 << 1),	//Seek backward.
	SEEK_FLAG_BYTE		= (1 << 2),	//Seek to given byte offset instead of time.
	SEEK_FLAG_ANY		= (1 << 3),	//Seek to any location including non key frame.
	SEEK_FLAG_FRAME		= (1 << 4),	//Seek to given frame number instead of time.
};

enum Audio_codec
{
	AUDIO_CODEC_INVALID = -1,

	AUDIO_CODEC_AAC,	//Advanced audio coding.
	AUDIO_CODEC_AC3,	//Audio codec 3.
	AUDIO_CODEC_MP2,	//Mpeg audio layer 2.
	AUDIO_CODEC_MP3,	//Mpeg audio layer 3.

	AUDIO_CODEC_MAX,
};

enum Video_codec
{
	VIDEO_CODEC_INVALID = -1,

	VIDEO_CODEC_MJPEG,		//Motion jpeg.
	VIDEO_CODEC_H264,		//Advanced video coding.
	VIDEO_CODEC_MPEG4,		//Mpeg4 part 2.
	VIDEO_CODEC_MPEG2VIDEO,	//Mpeg2 video.

	VIDEO_CODEC_MAX,
};

enum Image_codec
{
	IMAGE_CODEC_INVALID = -1,

	IMAGE_CODEC_PNG,	//Portable network graphic.
	IMAGE_CODEC_JPG,	//Joint photographic experts group.
	IMAGE_CODEC_BMP,	//Bitmap.
	IMAGE_CODEC_TGA,	//Truevision TGA.

	IMAGE_CODEC_MAX,
};

enum Keyboard_button
{
	KEYBOARD_BUTTON_INVALID = -1,

	KEYBOARD_BUTTON_NONE,	//No button was pressed.
	KEYBOARD_BUTTON_LEFT,	//Left button (usually cancel) was pressed.
	KEYBOARD_BUTTON_MIDDLE,	//Middle button (usually I forgot) was pressed.
	KEYBOARD_BUTTON_RIGHT,	//Right button (usually confirm) was pressed.

	KEYBOARD_BUTTON_MAX,
};

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
	int bitrate = 0;					//Audio bitrate in Bps.
	int sample_rate = 0;				//Audio smaple rate in Hz.
	int ch = 0;							//Number of audio channels.
	std::string format_name = "";		//Audio codec name.
	std::string short_format_name = "";	//Audio short codec name.
	double duration = 0;				//Audio track duration in seconds.
	std::string track_lang = "";		//Audio track language.
	Sample_format sample_format = SAMPLE_FORMAT_INVALID;	//Audio sample format.
};

struct Video_info
{
	int width = 0;						//Video width.
	int height = 0;						//Video height.
	int codec_width = 0;				//Video codec width (actual image width).
	int codec_height = 0;				//Video codec height (actual image height).
	double framerate = 0;				//Video framerate.
	std::string format_name = "";		//Video codec name.
	std::string short_format_name = "";	//Video short codec name.
	double duration = 0;				//Video track duration in seconds.
	Multi_thread_type thread_type = THREAD_TYPE_NONE;	//Threading mode.
	double sar_width = 1;				//Sample aspect ratio for width.
	double sar_height = 1;				//Sample aspect ratio for height.
	Pixel_format pixel_format = PIXEL_FORMAT_INVALID;	//Video pixel format.
};

struct Color_converter_parameters
{
	u8* source = NULL;		//(in)  Source raw image data, user must allocate the buffer.
	u8* converted = NULL;	//(out) Converted raw image data, this buffer will be allocated inside of function.
	int in_width = 0;		//(in)  Source image width.
	int in_height = 0;		//(in)  Source image height.
	int out_width = 0;		//(in)  Converted image width.
	int out_height = 0;		//(in)  Converted image height.
	Pixel_format in_color_format = PIXEL_FORMAT_INVALID;	//(in) Source image pixel format.
	Pixel_format out_color_format = PIXEL_FORMAT_INVALID;	//(in) Converted image pixel format.
};

struct Audio_converter_parameters
{
	u8* source = NULL;			//(in)  Source raw audio data, user must allocate the buffer.
	u8* converted = NULL;		//(out) Converted raw audio data, this buffer will be allocated inside of function.
	int in_samples = 0;			//(in)  Number of source audio samples per channel.
	int in_ch = 0;				//(in)  Source audio ch.
	int in_sample_rate = 0;		//(in)  Source audio saple rate in Hz.
	int out_samples = 0;		//(out) Number of converted audio samples per channel.
	int out_ch = 0;				//(in)  Converted audio ch.
	int out_sample_rate = 0;	//(in)  Converted audio saple rate in Hz.
	Sample_format in_sample_format = SAMPLE_FORMAT_INVALID;		//(in) Source audio sample format.
	Sample_format out_sample_format = SAMPLE_FORMAT_INVALID;	//(in) Converted audio sample format.
};

struct Subtitle_info
{
	std::string format_name = "";		//Subtitle codec name.
	std::string short_format_name = "";	//Subtitle short codec name.
	std::string track_lang = "";		//Subtitle track language.
};

struct Subtitle_data
{
	u8* bitmap = NULL;		//Subtitle bitmap, this may NULL.
	int bitmap_x = 0;		//X (horizontal) position, this field will be set if bitmap is not NULL.
	int bitmap_y = 0;		//Y (vertical) position, this field will be set if bitmap is not NULL.
	int bitmap_width = 0;	//Bitmap width, this field will be set if bitmap is not NULL.
	int bitmap_height = 0;	//Bitmap height, this field will be set if bitmap is not NULL.
	double start_time = 0;	//Start time in ms for this subtitle data. subtitle should be displayed if (start_time <= current_time <= end_time).
	double end_time = 0;	//End time in ms for this subtitle data. subtitle should be displayed if (start_time <= current_time <= end_time).
	std::string text = "";	//Subtitle text.
};

struct Image_data
{
	C2D_Image c2d = { .tex = NULL, .subtex = NULL, };	//Texture data.
	Tex3DS_SubTexture* subtex = NULL;					//Subtexture data.
	bool selected = false;	//Whether this texture is selected.
	double x = -1;			//X (horizontal) position.
	double y = -1;			//Y (vertical) position.
	double x_size = -1;		//Texture drawn width.
	double y_size = -1;		//Texture drawn height.
};

struct Hid_info
{
	//Is button pressed.
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
	//Is button held.
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
	//Is button released.
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
	//CPAD and touch position.
	int cpad_x = 0;
	int cpad_y = 0;
	int touch_x = 0;
	int touch_y = 0;
	int touch_x_move = 0;
	int touch_y_move = 0;
	int held_time = 0;
	//Timestamp for this data.
	u64 ts = 0;
};

struct Queue
{
	bool deleting;					//Whether this queue is being deleted.
	u32* data;						//Data list.
	u32* event_id;					//Event id list.
	s32 max_items;					//Queue capacity.
	s32 next_index;					//Next free index.
	s32 reference_count;			//Reference count for this queue.
	LightEvent receive_wait_event;	//If timeout is not 0, this is used to wait for new message from this queue.
	LightEvent send_wait_event;		//If timeout is not 0, this is used to wait for available space to send data to this queue.
};

#endif
