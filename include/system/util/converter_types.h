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
	uint8_t* source;				//(in)  Input raw image data, user must allocate the buffer.
	uint8_t* converted;				//(out) Output raw image data, this buffer will be allocated inside of function.
	uint32_t in_width;				//(in)  Input image width.
	uint32_t in_height;				//(in)  Input image height.
	uint32_t out_width;				//(in)  Output image width.
	uint32_t out_height;			//(in)  Output image height.
	Raw_pixel in_color_format;		//(in)  Input image pixel format.
	Raw_pixel out_color_format;		//(in)  Output image pixel format.
} Converter_color_parameters;

typedef struct
{
	uint8_t* source;					//(in)  Input raw audio data, user must allocate the buffer.
	uint8_t* converted;					//(out) Output raw audio data, this buffer will be allocated inside of function.
	uint8_t in_ch;						//(in)  Input audio ch.
	uint32_t in_samples;				//(in)  Number of input samples per channel.
	uint32_t in_sample_rate;			//(in)  Input audio sample rate in Hz.
	uint8_t out_ch;						//(in)  Output audio ch.
	uint32_t out_samples;				//(out) Number of output samples per channel.
	uint32_t out_sample_rate;			//(in)  Output audio sample rate in Hz.
	Raw_sample in_sample_format;		//(in)  Input audio sample format.
	Raw_sample out_sample_format;		//(in)  Output audio sample format.
} Converter_audio_parameters;

#endif //!defined(DEF_CONVERTER_TYPES_H)
