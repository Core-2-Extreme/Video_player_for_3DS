//Includes.
#include "system/util/speaker.h"

#if DEF_SPEAKER_API_ENABLE
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/util.h"

//Defines.
#define DEF_SPEAKER_NUM_OF_CH		(uint8_t)(24)

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
static bool util_speaker_init = false;
static uint32_t util_speaker_music_ch[DEF_SPEAKER_NUM_OF_CH] = { 0, };
static ndspWaveBuf util_ndsp_buffer[DEF_SPEAKER_NUM_OF_CH][DEF_SPEAKER_MAX_BUFFERS] = { 0, };

//Code.
uint32_t Util_speaker_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_speaker_init)
		goto already_inited;

	for(uint8_t i = 0; i < DEF_SPEAKER_NUM_OF_CH; i++)
		util_speaker_music_ch[i] = UINT32_MAX;

	result = ndspInit();//0xD880A7FA.
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(ndspInit, false, result);
		goto nintendo_api_failed;
	}

	util_speaker_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed:
	return result;
}

uint32_t Util_speaker_set_audio_info(uint8_t play_ch, uint8_t music_ch, uint32_t sample_rate)
{
	float mix[12] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

	if(!util_speaker_init)
		goto not_inited;

	if(play_ch >= DEF_SPEAKER_NUM_OF_CH || music_ch == 0 || music_ch > 2 || sample_rate == 0)
		goto invalid_arg;

	ndspChnReset(play_ch);
	ndspChnWaveBufClear(play_ch);
	ndspChnSetMix(play_ch, mix);
	if(music_ch == 2)
	{
		ndspChnSetFormat(play_ch, NDSP_FORMAT_STEREO_PCM16);
		ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	}
	else
	{
		ndspChnSetFormat(play_ch, NDSP_FORMAT_MONO_PCM16);
		ndspSetOutputMode(NDSP_OUTPUT_MONO);
	}

	util_speaker_music_ch[play_ch] = music_ch;

	ndspChnSetInterp(play_ch, NDSP_INTERP_LINEAR);
	ndspChnSetRate(play_ch, sample_rate);
	for(uint32_t i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
	{
		free(util_ndsp_buffer[play_ch][i].data_vaddr);
		util_ndsp_buffer[play_ch][i].data_vaddr = NULL;
	}
	memset(util_ndsp_buffer[play_ch], 0, sizeof(util_ndsp_buffer[play_ch]));

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_speaker_add_buffer(uint8_t play_ch, const uint8_t* buffer, uint32_t size)
{
	uint32_t free_queue = UINT32_MAX;

	if(!util_speaker_init)
		goto not_inited;

	if(play_ch >= DEF_SPEAKER_NUM_OF_CH || !buffer || size == 0)
		goto invalid_arg;

	if(util_speaker_music_ch[play_ch] != 1 && util_speaker_music_ch[play_ch] != 2)
		goto not_inited;

	//Search for free queue.
	for(uint32_t i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
	{
		if(util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_FREE || util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_DONE)
		{
			//Free unused data if exist.
			free(util_ndsp_buffer[play_ch][i].data_vaddr);
			util_ndsp_buffer[play_ch][i].data_vaddr = NULL;

			if(free_queue == UINT32_MAX)
				free_queue = i;
		}
	}

	if(free_queue == UINT32_MAX)
	{
		//DEF_LOG_FORMAT("Queues are full!!!!!");
		goto try_again;
	}

	util_ndsp_buffer[play_ch][free_queue].data_vaddr = (uint8_t*)linearAlloc(size);
	if(!util_ndsp_buffer[play_ch][free_queue].data_vaddr)
		goto out_of_linear_memory;

	memcpy(util_ndsp_buffer[play_ch][free_queue].data_vaddr, buffer, size);

	util_ndsp_buffer[play_ch][free_queue].nsamples = size / (2 * util_speaker_music_ch[play_ch]);
	ndspChnWaveBufAdd(play_ch, &util_ndsp_buffer[play_ch][free_queue]);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	out_of_linear_memory:
	return DEF_ERR_OUT_OF_LINEAR_MEMORY;
}

uint32_t Util_speaker_get_available_buffer_num(uint8_t play_ch)
{
	uint32_t available_buffers = 0;

	if(!util_speaker_init)
		return 0;
	if(play_ch >= DEF_SPEAKER_NUM_OF_CH)
		return 0;
	if(util_speaker_music_ch[play_ch] != 1 && util_speaker_music_ch[play_ch] != 2)
		return 0;

	for(uint32_t i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
	{
		if(util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_PLAYING || util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_QUEUED)
			available_buffers++;
	}

	return available_buffers;
}

uint32_t Util_speaker_get_available_buffer_size(uint8_t play_ch)
{
	uint32_t buffer_size = 0;

	if(!util_speaker_init)
		return 0;
	if(play_ch >= DEF_SPEAKER_NUM_OF_CH)
		return 0;
	if(util_speaker_music_ch[play_ch] != 1 && util_speaker_music_ch[play_ch] != 2)
		return 0;

	for(uint32_t i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
	{
		if(util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_QUEUED)
			buffer_size += util_ndsp_buffer[play_ch][i].nsamples * 2 * util_speaker_music_ch[play_ch];
		else if(util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_PLAYING)
			buffer_size += (util_ndsp_buffer[play_ch][i].nsamples - ndspChnGetSamplePos(play_ch)) * 2 * util_speaker_music_ch[play_ch];
	}

	return buffer_size;
}

void Util_speaker_clear_buffer(uint8_t play_ch)
{
	if(!util_speaker_init)
		return;
	if(play_ch >= DEF_SPEAKER_NUM_OF_CH)
		return;

	ndspChnWaveBufClear(play_ch);
	for(uint32_t i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
	{
		free(util_ndsp_buffer[play_ch][i].data_vaddr);
		util_ndsp_buffer[play_ch][i].data_vaddr = NULL;
	}
}

void Util_speaker_pause(uint8_t play_ch)
{
	if(!util_speaker_init)
		return;
	if(play_ch >= DEF_SPEAKER_NUM_OF_CH)
		return;

	ndspChnSetPaused(play_ch, true);
}

void Util_speaker_resume(uint8_t play_ch)
{
	if(!util_speaker_init)
		return;
	if(play_ch >= DEF_SPEAKER_NUM_OF_CH)
		return;

	ndspChnSetPaused(play_ch, false);
}

bool Util_speaker_is_paused(uint8_t play_ch)
{
	if(!util_speaker_init)
		return false;
	else if(play_ch >= DEF_SPEAKER_NUM_OF_CH)
		return false;
	else
		return ndspChnIsPaused(play_ch);
}

bool Util_speaker_is_playing(uint8_t play_ch)
{
	if(!util_speaker_init)
		return false;
	else if(play_ch >= DEF_SPEAKER_NUM_OF_CH)
		return false;
	else
		return ndspChnIsPlaying(play_ch);
}

void Util_speaker_exit(void)
{
	if(!util_speaker_init)
		return;

	for(uint8_t i = 0; i < DEF_SPEAKER_NUM_OF_CH; i++)
		Util_speaker_clear_buffer(i);

	util_speaker_init = false;
	ndspExit();
	for(uint8_t i = 0; i < DEF_SPEAKER_NUM_OF_CH; i++)
		util_speaker_music_ch[i] = UINT32_MAX;
}
#endif //DEF_SPEAKER_API_ENABLE
