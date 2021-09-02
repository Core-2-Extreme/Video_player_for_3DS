#include "system/headers.hpp"

ndspWaveBuf util_ndsp_buffer[24][DEF_SPEAKER_MAX_BUFFERS];

void Util_speaker_init(int play_ch, int music_ch, int sample_rate)
{
	float mix[12] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

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
	
	ndspChnSetInterp(play_ch, NDSP_INTERP_LINEAR);
	ndspChnSetRate(play_ch, sample_rate);
	memset(util_ndsp_buffer[play_ch], 0, sizeof(util_ndsp_buffer[play_ch]));
	for(int i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
		util_ndsp_buffer[play_ch][i].data_vaddr = NULL;
}

Result_with_string Util_speaker_add_buffer(int play_ch, int music_ch, u8* buffer, int size)
{
	Result_with_string result;
	int free_queue = -1;

	for(int i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
	{
		if(util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_FREE || util_ndsp_buffer[play_ch][i].status == NDSP_WBUF_DONE)
		{
			free_queue = i;
			break;
		}
	}

	if(free_queue == -1)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] Queues are full ";
		return result;
	}

	linearFree((void*)util_ndsp_buffer[play_ch][free_queue].data_vaddr);
	util_ndsp_buffer[play_ch][free_queue].data_vaddr = NULL;
	util_ndsp_buffer[play_ch][free_queue].data_vaddr = (u8*)linearAlloc(size * music_ch);
	if(util_ndsp_buffer[play_ch][free_queue].data_vaddr == NULL)
	{
		result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
		result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
		return result;
	}

	memcpy((void*)util_ndsp_buffer[play_ch][free_queue].data_vaddr, buffer, size * music_ch);

	util_ndsp_buffer[play_ch][free_queue].nsamples = size / 2;
	ndspChnWaveBufAdd(play_ch, &util_ndsp_buffer[play_ch][free_queue]);

	return result;
}

void Util_speaker_clear_buffer(int play_ch)
{
	ndspChnWaveBufClear(play_ch);
}

void Util_speaker_pause(int play_ch)
{
	ndspChnSetPaused(play_ch, true);
}

void Util_speaker_resume(int play_ch)
{
	ndspChnSetPaused(play_ch, false);
}

bool Util_speaker_is_paused(int play_ch)
{
	return ndspChnIsPaused(play_ch);
}

bool Util_speaker_is_playing(int play_ch)
{
	return ndspChnIsPlaying(play_ch);
}

void Util_speaker_exit(int play_ch)
{
	ndspChnWaveBufClear(play_ch);
	ndspChnSetPaused(play_ch, false);
	for(int i = 0; i < DEF_SPEAKER_MAX_BUFFERS; i++)
	{
		linearFree((void*)util_ndsp_buffer[play_ch][i].data_vaddr);
		util_ndsp_buffer[play_ch][i].data_vaddr = NULL;
	}
}
