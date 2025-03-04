#if !defined(DEF_CAM_TYPES_H)
#define DEF_CAM_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/log_enum_types.h"

#define DEF_CAM_API_ENABLE				/*(bool)(*/true/*)*/	//Enable camera API.

typedef enum
{
	CAM_RES_INVALID = -1,

	CAM_RES_640x480,
	CAM_RES_512x384,
	CAM_RES_400x240,
	CAM_RES_352x288,
	CAM_RES_320x240,
	CAM_RES_256x192,
	CAM_RES_176x144,
	CAM_RES_160x120,

	CAM_RES_MAX,
} Cam_resolution;

DEF_LOG_ENUM_DEBUG
(
	Cam_resolution,
	CAM_RES_INVALID,
	CAM_RES_640x480,
	CAM_RES_512x384,
	CAM_RES_400x240,
	CAM_RES_352x288,
	CAM_RES_320x240,
	CAM_RES_256x192,
	CAM_RES_176x144,
	CAM_RES_160x120,
	CAM_RES_MAX
)

typedef enum
{
	CAM_FPS_INVALID = -1,

	CAM_FPS_15,
	CAM_FPS_15_TO_5,
	CAM_FPS_15_TO_2,
	CAM_FPS_10,
	CAM_FPS_8_5,
	CAM_FPS_5,
	CAM_FPS_20,
	CAM_FPS_20_TO_5,
	CAM_FPS_30,
	CAM_FPS_30_TO_5,
	CAM_FPS_15_TO_10,
	CAM_FPS_20_TO_10,
	CAM_FPS_30_TO_10,

	CAM_FPS_MAX,
} Cam_framerate;

DEF_LOG_ENUM_DEBUG
(
	Cam_framerate,
	CAM_FPS_INVALID,
	CAM_FPS_15,
	CAM_FPS_15_TO_5,
	CAM_FPS_15_TO_2,
	CAM_FPS_10,
	CAM_FPS_8_5,
	CAM_FPS_5,
	CAM_FPS_20,
	CAM_FPS_20_TO_5,
	CAM_FPS_30,
	CAM_FPS_30_TO_5,
	CAM_FPS_15_TO_10,
	CAM_FPS_20_TO_10,
	CAM_FPS_30_TO_10,
	CAM_FPS_MAX
)

typedef enum
{
	CAM_LENS_CORRECTION_INVALID = -1,

	CAM_LENS_CORRECTION_OFF,
	CAM_LENS_CORRECTION_70,
	CAM_LENS_CORRECTION_90,

	CAM_LENS_CORRECTION_MAX,
} Cam_lens_correction;

DEF_LOG_ENUM_DEBUG
(
	Cam_lens_correction,
	CAM_LENS_CORRECTION_INVALID,
	CAM_LENS_CORRECTION_OFF,
	CAM_LENS_CORRECTION_70,
	CAM_LENS_CORRECTION_90,
	CAM_LENS_CORRECTION_MAX
)

typedef enum
{
	CAM_PORT_INVALID = -1,

	CAM_PORT_OUT_LEFT,
	CAM_PORT_OUT_RIGHT,
	CAM_PORT_IN,

	CAM_PORT_MAX,
} Cam_port;

DEF_LOG_ENUM_DEBUG
(
	Cam_port,
	CAM_PORT_INVALID,
	CAM_PORT_OUT_LEFT,
	CAM_PORT_OUT_RIGHT,
	CAM_PORT_IN,
	CAM_PORT_MAX
)

typedef enum
{
	CAM_CONTRAST_INVALID = -1,

	CAM_CONTRAST_01,
	CAM_CONTRAST_02,
	CAM_CONTRAST_03,
	CAM_CONTRAST_04,
	CAM_CONTRAST_05,
	CAM_CONTRAST_06,
	CAM_CONTRAST_07,
	CAM_CONTRAST_08,
	CAM_CONTRAST_09,
	CAM_CONTRAST_10,
	CAM_CONTRAST_11,

	CAM_CONTRAST_MAX,
} Cam_contrast;

DEF_LOG_ENUM_DEBUG
(
	Cam_contrast,
	CAM_CONTRAST_INVALID,
	CAM_CONTRAST_01,
	CAM_CONTRAST_02,
	CAM_CONTRAST_03,
	CAM_CONTRAST_04,
	CAM_CONTRAST_05,
	CAM_CONTRAST_06,
	CAM_CONTRAST_07,
	CAM_CONTRAST_08,
	CAM_CONTRAST_09,
	CAM_CONTRAST_10,
	CAM_CONTRAST_11,
	CAM_CONTRAST_MAX
)

typedef enum
{
	CAM_WHITE_BALANCE_INVALID = -1,

	CAM_WHITE_BALANCE_AUTO,
	CAM_WHITE_BALANCE_3200K,
	CAM_WHITE_BALANCE_4150K,
	CAM_WHITE_BALANCE_5200K,
	CAM_WHITE_BALANCE_6000K,
	CAM_WHITE_BALANCE_7000K,

	CAM_WHITE_BALANCE_MAX,
} Cam_white_balance;

DEF_LOG_ENUM_DEBUG
(
	Cam_white_balance,
	CAM_WHITE_BALANCE_INVALID,
	CAM_WHITE_BALANCE_AUTO,
	CAM_WHITE_BALANCE_3200K,
	CAM_WHITE_BALANCE_4150K,
	CAM_WHITE_BALANCE_5200K,
	CAM_WHITE_BALANCE_6000K,
	CAM_WHITE_BALANCE_7000K,
	CAM_WHITE_BALANCE_MAX
)

typedef enum
{
	CAM_EXPOSURE_INVALID = -1,

	CAM_EXPOSURE_0,
	CAM_EXPOSURE_1,
	CAM_EXPOSURE_2,
	CAM_EXPOSURE_3,
	CAM_EXPOSURE_4,
	CAM_EXPOSURE_5,

	CAM_EXPOSURE_MAX,
} Cam_exposure;

DEF_LOG_ENUM_DEBUG
(
	Cam_exposure,
	CAM_EXPOSURE_INVALID,
	CAM_EXPOSURE_0,
	CAM_EXPOSURE_1,
	CAM_EXPOSURE_2,
	CAM_EXPOSURE_3,
	CAM_EXPOSURE_4,
	CAM_EXPOSURE_5,
	CAM_EXPOSURE_MAX
)

#endif //!defined(DEF_CAM_TYPES_H)
