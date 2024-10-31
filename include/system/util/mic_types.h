#if !defined(DEF_MIC_TYPES_H)
#define DEF_MIC_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/log_enum_types.h"

#define DEF_MIC_API_ENABLE				/*(bool)(*/true/*)*/	//Enable mic API.

typedef enum
{
	MIC_SAMPLE_RATE_INVALID = -1,

	MIC_SAMPLE_RATE_8182HZ,
	MIC_SAMPLE_RATE_10909HZ,
	MIC_SAMPLE_RATE_16364HZ,
	MIC_SAMPLE_RATE_32728HZ,

	MIC_SAMPLE_RATE_MAX,
} Mic_sample_rate;

DEF_LOG_ENUM_DEBUG
(
	Mic_sample_rate,
	MIC_SAMPLE_RATE_INVALID,
	MIC_SAMPLE_RATE_8182HZ,
	MIC_SAMPLE_RATE_10909HZ,
	MIC_SAMPLE_RATE_16364HZ,
	MIC_SAMPLE_RATE_32728HZ,
	MIC_SAMPLE_RATE_MAX
)

#endif //!defined(DEF_MIC_TYPES_H)
