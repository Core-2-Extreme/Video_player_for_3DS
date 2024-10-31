#if !defined(DEF_CONVERTER_TYPES_H)
#define DEF_CONVERTER_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "raw_types.h"

#define DEF_CONVERTER_HW_API_ENABLE					/*(bool)(*/true/*)*/	//Enable hardware color converter API.
#define DEF_CONVERTER_SW_ASM_API_ENABLE				/*(bool)(*/true/*)*/	//Enable assembly optimized software color converter API.
#define DEF_CONVERTER_SW_API_ENABLE					/*(bool)(*/true/*)*/	//Enable software color converter API.
#define DEF_CONVERTER_SW_FFMPEG_AUDIO_API_ENABLE	/*(bool)(*/true/*)*/	//Enable software audio samples converter API. This will use ffmpeg functions.
#define DEF_CONVERTER_SW_FFMPEG_COLOR_API_ENABLE	/*(bool)(*/true/*)*/	//Enable software color converter API. This will use ffmpeg functions.

typedef struct
{
	uint8_t* source;				//(in)  Source raw image data, user must allocate the buffer.
	uint8_t* converted;				//(out) Converted raw image data, this buffer will be allocated inside of function.
	uint32_t in_width;				//(in)  Source image width.
	uint32_t in_height;				//(in)  Source image height.
	uint32_t out_width;				//(in)  Converted image width.
	uint32_t out_height;			//(in)  Converted image height.
	Raw_pixel in_color_format;		//(in) Source image pixel format.
	Raw_pixel out_color_format;		//(in) Converted image pixel format.
} Converter_color_parameters;

typedef struct
{
	uint8_t* source;					//(in)  Source raw audio data, user must allocate the buffer.
	uint8_t* converted;					//(out) Converted raw audio data, this buffer will be allocated inside of function.
	uint8_t in_ch;						//(in)  Source audio ch.
	uint32_t in_samples;				//(in)  Number of source audio samples per channel.
	uint32_t in_sample_rate;			//(in)  Source audio saple rate in Hz.
	uint8_t out_ch;						//(in)  Converted audio ch.
	uint32_t out_samples;				//(out) Number of converted audio samples per channel.
	uint32_t out_sample_rate;			//(in)  Converted audio saple rate in Hz.
	Raw_sample in_sample_format;		//(in) Source audio sample format.
	Raw_sample out_sample_format;		//(in) Converted audio sample format.
} Converter_audio_parameters;

#endif //!defined(DEF_CONVERTER_TYPES_H)
