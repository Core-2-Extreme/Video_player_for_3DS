#include "system/headers.hpp"

bool util_mic_init = false;
u8* util_mic_buffer = NULL;
int util_mic_last_pos = 0;

Result_with_string Util_mic_init(int buffer_size)
{
	Result_with_string result;

	if(util_mic_init)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] Mic is already initialized. ";
		return result;
	}

	util_mic_last_pos = 0;
	buffer_size -= buffer_size % 0x1000;
	util_mic_buffer = (u8*)memalign(0x1000, buffer_size);
	if(util_mic_buffer == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	MICU_SetAllowShellClosed(true);
	MICU_SetPower(true);
	result.code = micInit(util_mic_buffer, buffer_size);
	if(result.code != 0)
	{
		result.string = "[Error] micInit() failed. ";
		free(util_mic_buffer);
		util_mic_buffer = NULL;
		return result;
	}

	util_mic_init = true;
	return result;
}

Result_with_string Util_mic_start_recording(int sample_rate)
{
	Result_with_string result;
	MICU_SampleRate mic_sample_rate;

	if(!util_mic_init)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] Mic is not initialized. ";
		return result;
	}

	if(sample_rate == 32728)
		mic_sample_rate = MICU_SAMPLE_RATE_32730;
	else if(sample_rate == 16364)
		mic_sample_rate = MICU_SAMPLE_RATE_16360;
	else if(sample_rate == 10909)
		mic_sample_rate = MICU_SAMPLE_RATE_10910;
	else if(sample_rate == 8182)
		mic_sample_rate = MICU_SAMPLE_RATE_8180;
	else
	{
		result.code = DEF_ERR_INVALID_ARG;
		result.string = DEF_ERR_INVALID_ARG_STR;
		return result;
	}

	result.code = MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, mic_sample_rate, 0, micGetSampleDataSize(), false);
	if(result.code != 0)
	{
		result.string = "[Error] MICU_StartSampling() failed. ";
		return result;
	}
	util_mic_last_pos = 0;
	
	return result;
}

void Util_mic_stop_recording(void)
{
	MICU_StopSampling();
}

bool Util_mic_is_recording(void)
{
	bool recording = false;
	MICU_IsSampling(&recording);
	return recording;
}

Result_with_string Util_mic_get_audio_data(u8** raw_data, int* size)
{
	Result_with_string result;
	int last_pos;

	if(!util_mic_init)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] Mic is not initialized. ";
		return result;
	}

	last_pos = micGetLastSampleOffset();
	*raw_data = (u8*)Util_safe_linear_alloc(last_pos - util_mic_last_pos);
	if(*raw_data == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	*size = last_pos - util_mic_last_pos;
	memcpy(*raw_data, util_mic_buffer + util_mic_last_pos, *size);

	util_mic_last_pos = last_pos;
	return result;
}

void Util_mic_exit(void)
{
	MICU_SetAllowShellClosed(false);
	MICU_SetPower(false);
	micExit();
	free(util_mic_buffer);
	util_mic_buffer = NULL;
	util_mic_init = false;
}
