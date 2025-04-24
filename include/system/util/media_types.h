#if !defined(DEF_MEDIA_TYPES_H)
#define DEF_MEDIA_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "raw_types.h"

typedef uint8_t Media_seek_flag;
#define MEDIA_SEEK_FLAG_NONE		(Media_seek_flag)(0 << 0)	//No seek flag.
#define MEDIA_SEEK_FLAG_BACKWARD	(Media_seek_flag)(1 << 0)	//Seek backward.
#define MEDIA_SEEK_FLAG_BYTE		(Media_seek_flag)(1 << 1)	//Seek to given byte offset instead of time.
#define MEDIA_SEEK_FLAG_ANY			(Media_seek_flag)(1 << 2)	//Seek to any location including non-key frame.
#define MEDIA_SEEK_FLAG_FRAME		(Media_seek_flag)(1 << 3)	//Seek to given frame number instead of time.

typedef enum
{
	MEDIA_V_CODEC_INVALID = -1,

	MEDIA_V_CODEC_MJPEG,		//Motion jpeg.
	MEDIA_V_CODEC_H264,			//Advanced video coding.
	MEDIA_V_CODEC_MPEG4,		//MPEG4 part 2.
	MEDIA_V_CODEC_MPEG2VIDEO,	//MPEG2 video.

	MEDIA_V_CODEC_MAX,
} Media_v_codec;

DEF_LOG_ENUM_DEBUG
(
	Media_v_codec,
	MEDIA_V_CODEC_INVALID,
	MEDIA_V_CODEC_MJPEG,
	MEDIA_V_CODEC_H264,
	MEDIA_V_CODEC_MPEG4,
	MEDIA_V_CODEC_MPEG2VIDEO,
	MEDIA_V_CODEC_MAX
)

typedef enum
{
	MEDIA_A_CODEC_INVALID = -1,

	MEDIA_A_CODEC_AAC,	//Advanced audio coding.
	MEDIA_A_CODEC_AC3,	//Audio codec 3.
	MEDIA_A_CODEC_MP2,	//MPEG audio layer 2.
	MEDIA_A_CODEC_MP3,	//MPEG audio layer 3.

	MEDIA_A_CODEC_MAX,
} Media_a_codec;

DEF_LOG_ENUM_DEBUG
(
	Media_a_codec,
	MEDIA_A_CODEC_INVALID,
	MEDIA_A_CODEC_AAC,
	MEDIA_A_CODEC_AC3,
	MEDIA_A_CODEC_MP2,
	MEDIA_A_CODEC_MP3,
	MEDIA_A_CODEC_MAX
)

typedef enum
{
	MEDIA_I_CODEC_INVALID = -1,

	MEDIA_I_CODEC_PNG,	//Portable network graphics.
	MEDIA_I_CODEC_JPG,	//Joint photographic experts group.
	MEDIA_I_CODEC_BMP,	//Bitmap.
	MEDIA_I_CODEC_TGA,	//Truevision TGA.

	MEDIA_I_CODEC_MAX,
} Media_i_codec;

DEF_LOG_ENUM_DEBUG
(
	Media_i_codec,
	MEDIA_I_CODEC_INVALID,
	MEDIA_I_CODEC_PNG,
	MEDIA_I_CODEC_JPG,
	MEDIA_I_CODEC_BMP,
	MEDIA_I_CODEC_TGA,
	MEDIA_I_CODEC_MAX
)

typedef enum
{
	MEDIA_PACKET_TYPE_INVALID = -1,

	MEDIA_PACKET_TYPE_UNKNOWN,		//This packet contains unknown data.
	MEDIA_PACKET_TYPE_AUDIO,		//This packet contains audio data.
	MEDIA_PACKET_TYPE_VIDEO,		//This packet contains video data.
	MEDIA_PACKET_TYPE_SUBTITLE,		//This packet contains subtitle data.

	MEDIA_PACKET_TYPE_MAX,
} Media_packet_type;

DEF_LOG_ENUM_DEBUG
(
	Media_packet_type,
	MEDIA_PACKET_TYPE_INVALID,
	MEDIA_PACKET_TYPE_UNKNOWN,
	MEDIA_PACKET_TYPE_AUDIO,
	MEDIA_PACKET_TYPE_VIDEO,
	MEDIA_PACKET_TYPE_SUBTITLE,
	MEDIA_PACKET_TYPE_MAX
)

typedef enum
{
	MEDIA_THREAD_TYPE_INVALID = -1,

	MEDIA_THREAD_TYPE_NONE,		//No multi-threading, using single thread.
	MEDIA_THREAD_TYPE_FRAME,	//Frame level multi-threading.
	MEDIA_THREAD_TYPE_SLICE,	//Slice level multi-threading.
	MEDIA_THREAD_TYPE_AUTO,		//Auto (only used when requesting multi-threading mode).

	MEDIA_THREAD_TYPE_MAX,
} Media_thread_type;

DEF_LOG_ENUM_DEBUG
(
	Media_thread_type,
	MEDIA_THREAD_TYPE_INVALID,
	MEDIA_THREAD_TYPE_NONE,
	MEDIA_THREAD_TYPE_FRAME,
	MEDIA_THREAD_TYPE_SLICE,
	MEDIA_THREAD_TYPE_AUTO,
	MEDIA_THREAD_TYPE_MAX
)

typedef struct
{
	uint32_t bitrate;				//(out) Audio bitrate in bps.
	uint32_t sample_rate;			//(out) Audio sample rate in Hz.
	uint8_t ch;						//(out) Number of audio channels.
	double duration;				//(out) Audio track duration in seconds.
	char format_name[96];			//(out) Audio codec name.
	char short_format_name[16];		//(out) Audio short codec name.
	char track_lang[16];			//(out) Audio track language.
	Raw_sample sample_format;		//(out) Audio sample format.
} Media_a_info;

typedef struct
{
	uint32_t width;					//(out) Video width.
	uint32_t height;				//(out) Video height.
	uint32_t codec_width;			//(out) Video codec width (actual image width).
	uint32_t codec_height;			//(out) Video codec height (actual image height).
	double framerate;				//(out) Video framerate.
	double duration;				//(out) Video track duration in seconds.
	double sar_width;				//(out) Sample aspect ratio for width.
	double sar_height;				//(out) Sample aspect ratio for height.
	char format_name[96];			//(out) Video codec name.
	char short_format_name[16];		//(out) Video short codec name.
	Media_thread_type thread_type;	//(out) Threading mode.
	Raw_pixel pixel_format;			//(out) Video pixel format.
} Media_v_info;

typedef struct
{
	char format_name[96];			//(out) Subtitle codec name.
	char short_format_name[16];		//(out) Subtitle short codec name.
	char track_lang[16];			//(out) Subtitle track language.
} Media_s_info;

typedef struct
{
	uint8_t* bitmap;			//(out) Subtitle bitmap, this may be NULL.
	uint32_t bitmap_x;			//(out) X (horizontal) position, this field will be set if bitmap is not NULL.
	uint32_t bitmap_y;			//(out) Y (vertical) position, this field will be set if bitmap is not NULL.
	uint32_t bitmap_width;		//(out) Bitmap width, this field will be set if bitmap is not NULL.
	uint32_t bitmap_height;		//(out) Bitmap height, this field will be set if bitmap is not NULL.
	double start_time;			//(out) Start time in ms for this subtitle data, subtitle should be displayed if (start_time <= current_time <= end_time).
	double end_time;			//(out) End time in ms for this subtitle data, subtitle should be displayed if (start_time <= current_time <= end_time).
	char* text;					//(out) Subtitle text.
} Media_s_data;

#endif //!defined(DEF_MEDIA_TYPES_H)
