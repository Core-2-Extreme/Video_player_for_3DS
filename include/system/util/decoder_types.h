#if !defined(DEF_DECODER_TYPES_H)
#define DEF_DECODER_TYPES_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_DECODER_DMA_ENABLE					/*(bool)(*/true/*)*/	//Enable DMA in video decoder module for faster processing.
#define DEF_DECODER_IMAGE_API_ENABLE			/*(bool)(*/true/*)*/	//Enable image decoder API. This will use stb_image functions.
#define DEF_DECODER_VIDEO_AUDIO_API_ENABLE		/*(bool)(*/true/*)*/	//Enable video/audio decoder API. This will use ffmpeg functions.

#define DEF_DECODER_MAX_AUDIO_TRACKS			(uint8_t)(8)
#define DEF_DECODER_MAX_VIDEO_TRACKS			(uint8_t)(2)
#define DEF_DECODER_MAX_SUBTITLE_TRACKS			(uint8_t)(8)
#define DEF_DECODER_MAX_SESSIONS				(uint8_t)(2)
#define DEF_DECODER_MAX_CACHE_PACKETS			(uint16_t)(256)
#define DEF_DECODER_MAX_RAW_IMAGE				(uint16_t)(128)

#endif //!defined(DEF_DECODER_TYPES_H)
