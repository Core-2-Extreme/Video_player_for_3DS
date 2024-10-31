//Includes.
#include "system/util/mic.h"

#if DEF_MIC_API_ENABLE
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/util.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
static bool util_mic_init = false;
static uint8_t* util_mic_buffer = NULL;
static uint32_t util_mic_last_pos = 0;
static uint32_t util_mic_sample_rate = 0;

//Code.
uint32_t Util_mic_init(uint32_t buffer_size)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_mic_init)
		goto already_inited;

	if(buffer_size < 0x2000)
		goto invalid_arg;

	util_mic_last_pos = 0;
	buffer_size -= buffer_size % 0x1000;
	//Mic module requires memory allocated on heap (precisely svcCreateMemoryBlock() requires it).
	util_mic_buffer = (uint8_t*)memalign_heap(0x1000, buffer_size);
	if(!util_mic_buffer)
		goto out_of_memory;

	memset(util_mic_buffer, 0x0, buffer_size);
	result = micInit(util_mic_buffer, buffer_size);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(micInit, false, result);
		goto nintendo_api_failed;
	}

	result = MICU_SetAllowShellClosed(true);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(MICU_SetAllowShellClosed, false, result);
		goto nintendo_api_failed_0;
	}

	result = MICU_SetPower(true);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(MICU_SetPower, false, result);
		goto nintendo_api_failed_0;
	}

	util_mic_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed_0:
	micExit();
	//Fallthrough.

	nintendo_api_failed:
	free(util_mic_buffer);
	util_mic_buffer = NULL;
	return result;
}

uint32_t Util_mic_start_recording(Mic_sample_rate sample_rate_mode)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t sample_rate = 0;
	MICU_SampleRate mic_sample_rate = MICU_SAMPLE_RATE_8180;

	if(!util_mic_init)
		goto not_inited;

	if (sample_rate_mode <= MIC_SAMPLE_RATE_INVALID || sample_rate_mode >= MIC_SAMPLE_RATE_MAX)
		goto invalid_arg;

	if(Util_mic_is_recording())
		goto already_inited;

	if(sample_rate_mode == MIC_SAMPLE_RATE_32728HZ)
	{
		sample_rate = 32728;
		mic_sample_rate = MICU_SAMPLE_RATE_32730;
	}
	else if(sample_rate_mode == MIC_SAMPLE_RATE_16364HZ)
	{
		sample_rate = 16364;
		mic_sample_rate = MICU_SAMPLE_RATE_16360;
	}
	else if(sample_rate_mode == MIC_SAMPLE_RATE_10909HZ)
	{
		sample_rate = 10909;
		mic_sample_rate = MICU_SAMPLE_RATE_10910;
	}
	else if(sample_rate_mode == MIC_SAMPLE_RATE_8182HZ)
	{
		sample_rate = 8182;
		mic_sample_rate = MICU_SAMPLE_RATE_8180;
	}

	result = MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, mic_sample_rate, 0, micGetSampleDataSize() - 4, true);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(MICU_StartSampling, false, result);
		goto nintendo_api_failed;
	}

	util_mic_last_pos = 0;
	util_mic_sample_rate = sample_rate;
	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
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

uint32_t Util_mic_query_remaining_buffer_time(void)
{
	uint32_t current_pos = 0;
	uint32_t remaining_time = 0;
	if(!util_mic_init || !Util_mic_is_recording())
		return 0;

	current_pos = micGetLastSampleOffset();
	if(util_mic_last_pos < current_pos)
		remaining_time = ((double)((micGetSampleDataSize() - 4) - (current_pos - util_mic_last_pos)) / (util_mic_sample_rate * 2)) * 1000;
	else
		remaining_time = ((double)(util_mic_last_pos - current_pos) / (util_mic_sample_rate * 2)) * 1000;

	return remaining_time;
}

uint32_t Util_mic_get_audio_data(uint8_t** raw_data, uint32_t* size)
{
	uint32_t buffer_offset = 0;
	uint32_t last_pos = 0;
	uint32_t buffer_size = 0;

	if(!util_mic_init)
		goto not_inited;

	if(!raw_data || !size)
		goto invalid_arg;

	*size = 0;

	last_pos = micGetLastSampleOffset();
	if(last_pos == util_mic_last_pos)
	{
		DEF_LOG_STRING("No raw audio available.");
		goto try_again;
	}

	if(util_mic_last_pos < last_pos)
		buffer_size = last_pos - util_mic_last_pos;
	else
		buffer_size = (micGetSampleDataSize() - 4) - (util_mic_last_pos - last_pos);

	*raw_data = (uint8_t*)linearAlloc(buffer_size);
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
	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	try_again:
	return DEF_ERR_TRY_AGAIN;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

void Util_mic_exit(void)
{
	if(!util_mic_init)
		return;

	Util_mic_stop_recording();
	MICU_SetAllowShellClosed(false);
	MICU_SetPower(false);
	micExit();
	free(util_mic_buffer);
	util_mic_buffer = NULL;
	util_mic_init = false;
}
#endif //DEF_MIC_API_ENABLE
