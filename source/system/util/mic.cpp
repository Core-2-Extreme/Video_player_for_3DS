#include "system/headers.hpp"

bool util_mic_init = false;
u8* util_mic_buffer = NULL;
int util_mic_last_pos = 0;
int util_mic_sample_rate = 0;

Result_with_string Util_mic_init(int buffer_size)
{
	Result_with_string result;

	if(util_mic_init)
		goto already_inited;

	if(buffer_size < 0x2000)
		goto invalid_arg;

	util_mic_last_pos = 0;
	buffer_size -= buffer_size % 0x1000;
	//mic module requires memory allocated on heap (precisely svcCreateMemoryBlock() requires it)
	util_mic_buffer = (u8*)__real_memalign(0x1000, buffer_size);
	if(!util_mic_buffer)
		goto out_of_memory;
	
	memset(util_mic_buffer, 0x0, buffer_size);
	result.code = micInit(util_mic_buffer, buffer_size);
	if(result.code != 0)
	{
		result.error_description = "[Error] micInit() failed. ";
		goto nintendo_api_failed;
	}

	result.code = MICU_SetAllowShellClosed(true);
	if(result.code != 0)
	{
		result.error_description = "[Error] MICU_SetAllowShellClosed(() failed. ";
		goto nintendo_api_failed_0;
	}

	result.code = MICU_SetPower(true);
	if(result.code != 0)
	{
		result.error_description = "[Error] MICU_SetPower() failed. ";
		goto nintendo_api_failed_0;
	}

	util_mic_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed_0:
	micExit();

	nintendo_api_failed:
	Util_safe_linear_free(util_mic_buffer);
	util_mic_buffer = NULL;
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_mic_start_recording(int sample_rate)
{
	Result_with_string result;
	MICU_SampleRate mic_sample_rate;

	if(!util_mic_init)
		goto not_inited;

	if(Util_mic_is_recording())
		goto already_inited;

	if(sample_rate == 32728)
		mic_sample_rate = MICU_SAMPLE_RATE_32730;
	else if(sample_rate == 16364)
		mic_sample_rate = MICU_SAMPLE_RATE_16360;
	else if(sample_rate == 10909)
		mic_sample_rate = MICU_SAMPLE_RATE_10910;
	else if(sample_rate == 8182)
		mic_sample_rate = MICU_SAMPLE_RATE_8180;
	else
		goto invalid_arg;

	result.code = MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, mic_sample_rate, 0, micGetSampleDataSize() - 4, true);
	if(result.code != 0)
	{
		result.error_description = "[Error] MICU_StartSampling() failed. ";
		goto nintendo_api_failed;
	}
	util_mic_last_pos = 0;
	util_mic_sample_rate = sample_rate;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_mic_stop_recording(void)
{
	if(!util_mic_init)
		return;

	MICU_StopSampling();
}

bool Util_mic_is_recording(void)
{
	bool recording = false;
	if(!util_mic_init)
		return false;

	MICU_IsSampling(&recording);
	return recording;
}

int Util_mic_query_remaining_buffer_time(void)
{
	int current_pos = 0;
	int remaining_time = 0;
	if(!util_mic_init || !Util_mic_is_recording())
		return 0;

	current_pos = micGetLastSampleOffset();
	if(util_mic_last_pos < current_pos)
		remaining_time = ((double)((micGetSampleDataSize() - 4) - (current_pos - util_mic_last_pos)) / (util_mic_sample_rate * 2)) * 1000;
	else
		remaining_time = ((double)(util_mic_last_pos - current_pos) / (util_mic_sample_rate * 2)) * 1000;

	return remaining_time;
}

Result_with_string Util_mic_get_audio_data(u8** raw_data, int* size)
{
	Result_with_string result;
	int buffer_offset = 0;
	int last_pos = 0;
	int buffer_size = 0;
	*size = 0;

	if(!util_mic_init)
		goto not_inited;

	if(!raw_data || !size)
		goto invalid_arg;

	last_pos = micGetLastSampleOffset();
	if(last_pos == util_mic_last_pos)
	{
		result.error_description = "[Error] No raw audio available. ";
		goto try_again;
	}

	if(util_mic_last_pos < last_pos)
		buffer_size = last_pos - util_mic_last_pos;
	else
		buffer_size = (micGetSampleDataSize() - 4) - (util_mic_last_pos - last_pos);

	*raw_data = (u8*)Util_safe_linear_alloc(buffer_size);
	if(!*raw_data)
		goto out_of_memory;
	
	*size = buffer_size;
	if(util_mic_last_pos < last_pos)
		memcpy(*raw_data, util_mic_buffer + util_mic_last_pos, buffer_size);
	else
	{
		memcpy(*raw_data, util_mic_buffer + util_mic_last_pos, micGetSampleDataSize() - 4 - util_mic_last_pos);
		buffer_offset += micGetSampleDataSize() - 4 - util_mic_last_pos;
		memcpy((*raw_data) + buffer_offset, util_mic_buffer, last_pos);
	}

	util_mic_last_pos = last_pos;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;
	
	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	try_again:
	result.code = DEF_ERR_TRY_AGAIN;
	result.string = DEF_ERR_TRY_AGAIN_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

void Util_mic_exit(void)
{
	if(!util_mic_init)
		return;
	
	Util_mic_stop_recording();
	MICU_SetAllowShellClosed(false);
	MICU_SetPower(false);
	micExit();
	Util_safe_linear_free(util_mic_buffer);
	util_mic_buffer = NULL;
	util_mic_init = false;
}
