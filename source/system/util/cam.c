//Includes.
#include "system/util/cam.h"

#if DEF_CAM_API_ENABLE
#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/raw_types.h"
#include "system/util/log.h"
#include "system/util/util.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
static bool util_cam_init = false;
static uint16_t util_cam_width = 640;
static uint16_t util_cam_height = 480;
static uint32_t util_cam_buffer_size = 0;
static Cam_resolution util_cam_resolution = CAM_RES_640x480;
static Cam_port util_cam_port = CAM_PORT_OUT_RIGHT;

//Code.
uint32_t Util_cam_init(Raw_pixel color_format)
{
	uint16_t width = 0;
	uint16_t height = 0;
	uint32_t result = DEF_ERR_OTHER;
	CAMU_OutputFormat color = OUTPUT_RGB_565;

	if(util_cam_init)
		goto already_inited;

	if(color_format == RAW_PIXEL_RGB565LE)
		color = OUTPUT_RGB_565;
	else if(color_format == RAW_PIXEL_YUV422P)
		color = OUTPUT_YUV_422;
	else
		goto invalid_arg;

	result = camInit();
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(camInit, false, result);
		goto nintendo_api_failed_0;
	}

	result = CAMU_SetOutputFormat(SELECT_ALL, color, CONTEXT_BOTH);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetOutputFormat, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetNoiseFilter(SELECT_ALL, true);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetNoiseFilter, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetAutoExposure(SELECT_ALL, true);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetAutoExposure, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetWhiteBalance(SELECT_ALL, WHITE_BALANCE_AUTO);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetWhiteBalance, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetPhotoMode(SELECT_ALL, PHOTO_MODE_NORMAL);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetPhotoMode, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetTrimming(PORT_BOTH, false);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetTrimming, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetFrameRate(SELECT_ALL, FRAME_RATE_30);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetFrameRate, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetContrast(SELECT_ALL, CONTRAST_NORMAL);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetContrast, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetLensCorrection(SELECT_ALL, LENS_CORRECTION_NORMAL);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetLensCorrection, false, result);
		goto nintendo_api_failed;
	}

	width = 640;
	height = 480;
	result = CAMU_SetSize(SELECT_ALL, SIZE_VGA, CONTEXT_BOTH);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetSize, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_GetMaxBytes(&util_cam_buffer_size, width, height);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_GetMaxBytes, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetTransferBytes(PORT_BOTH, util_cam_buffer_size, width, height);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetTransferBytes, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_Activate(SELECT_OUT1);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_Activate, false, result);
		goto nintendo_api_failed;
	}

	util_cam_port = CAM_PORT_OUT_RIGHT;
	util_cam_width = width;
	util_cam_height = height;
	util_cam_init = true;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed:
	camExit();
	//Fallthrough.

	nintendo_api_failed_0:
	return result;
}

uint32_t Util_cam_take_a_picture(uint8_t** raw_data, uint16_t* width, uint16_t* height, bool shutter_sound)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t size = 0;
	Handle receive = 0;

	if(!util_cam_init)
		goto not_inited;

	if(!raw_data || !width || !height)
		goto invalid_arg;

	size = (util_cam_width * util_cam_height * 2);
	*raw_data = (uint8_t*)linearAlloc(size);
	if(*raw_data == NULL)
		goto out_of_memory;

	result = CAMU_StartCapture(PORT_BOTH);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_StartCapture, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetReceiving(&receive, *raw_data, (util_cam_port == CAM_PORT_OUT_LEFT ? PORT_CAM2 : PORT_CAM1), size, (int16_t)util_cam_buffer_size);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetReceiving, false, result);
		goto nintendo_api_failed;
	}

	result = svcWaitSynchronization(receive, 1000000000);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(svcWaitSynchronization, false, result);
		goto nintendo_api_failed;
	}

	if(shutter_sound)
	{
		result = CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_NORMAL);
		if (result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(CAMU_PlayShutterSound, false, result);
			goto nintendo_api_failed;
		}
	}

	*width = util_cam_width;
	*height = util_cam_height;
	svcCloseHandle(receive);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	nintendo_api_failed:
	svcCloseHandle(receive);
	return result;
}

uint32_t Util_cam_set_resolution(Cam_resolution resolution_mode)
{
	int16_t width = 0;
	int16_t height = 0;
	uint32_t result = DEF_ERR_OTHER;
	CAMU_Size size = SIZE_VGA;

	if(!util_cam_init)
		goto not_inited;

	if(resolution_mode <= CAM_RES_INVALID || resolution_mode >= CAM_RES_MAX)
		goto invalid_arg;

	if (resolution_mode == CAM_RES_640x480)
	{
		size = SIZE_VGA;
		width = 640;
		height = 480;
	}
	else if (resolution_mode == CAM_RES_512x384)
	{
		size = SIZE_DS_LCDx4;
		width = 512;
		height = 384;
	}
	else if (resolution_mode == CAM_RES_400x240)
	{
		size = SIZE_CTR_TOP_LCD;
		width = 400;
		height = 240;
	}
	else if (resolution_mode == CAM_RES_352x288)
	{
		size = SIZE_CIF;
		width = 352;
		height = 288;
	}
	else if (resolution_mode == CAM_RES_320x240)
	{
		size = SIZE_QVGA;
		width = 320;
		height = 240;
	}
	else if (resolution_mode == CAM_RES_256x192)
	{
		size = SIZE_DS_LCD;
		width = 256;
		height = 192;
	}
	else if (resolution_mode == CAM_RES_176x144)
	{
		size = SIZE_QCIF;
		width = 176;
		height = 144;
	}
	else if (resolution_mode == CAM_RES_160x120)
	{
		size = SIZE_QQVGA;
		width = 160;
		height = 120;
	}

	result = CAMU_SetSize(SELECT_ALL, size, CONTEXT_BOTH);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetSize, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_GetMaxBytes(&util_cam_buffer_size, width, height);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_GetMaxBytes, false, result);
		goto nintendo_api_failed;
	}

	result = CAMU_SetTransferBytes(PORT_BOTH, util_cam_buffer_size, width, height);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetTransferBytes, false, result);
		goto nintendo_api_failed;
	}

	util_cam_width = width;
	util_cam_height = height;
	util_cam_resolution = resolution_mode;

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	return result;
}

uint32_t Util_cam_set_fps(Cam_framerate fps_mode)
{
	uint32_t result = DEF_ERR_OTHER;
	CAMU_FrameRate framerate = FRAME_RATE_15;

	if(!util_cam_init)
		goto not_inited;

	if (fps_mode <= CAM_FPS_INVALID || fps_mode >= CAM_FPS_MAX)
		goto invalid_arg;

	if(fps_mode == CAM_FPS_5)
		framerate = FRAME_RATE_5;
	else if(fps_mode == CAM_FPS_8_5)
		framerate = FRAME_RATE_8_5;
	else if(fps_mode == CAM_FPS_10)
		framerate = FRAME_RATE_10;
	else if(fps_mode == CAM_FPS_15)
		framerate = FRAME_RATE_15;
	else if(fps_mode == CAM_FPS_20)
		framerate = FRAME_RATE_20;
	else if(fps_mode == CAM_FPS_30)
		framerate = FRAME_RATE_30;
	else if(fps_mode == CAM_FPS_15_TO_2)
		framerate = FRAME_RATE_15_TO_2;
	else if(fps_mode == CAM_FPS_15_TO_5)
		framerate = FRAME_RATE_15_TO_5;
	else if(fps_mode == CAM_FPS_15_TO_10)
		framerate = FRAME_RATE_15_TO_10;
	else if(fps_mode == CAM_FPS_20_TO_10)
		framerate = FRAME_RATE_20_TO_10;
	else if(fps_mode == CAM_FPS_30_TO_10)
		framerate = FRAME_RATE_30_TO_10;
	else if(fps_mode == CAM_FPS_30_TO_5)
		framerate = FRAME_RATE_30_TO_5;

	result = CAMU_SetFrameRate(SELECT_ALL, framerate);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetFrameRate, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	return result;
}

uint32_t Util_cam_set_contrast(Cam_contrast contrast_mode)
{
	uint32_t result = DEF_ERR_OTHER;
	CAMU_Contrast contrast = CONTRAST_PATTERN_01;

	if(!util_cam_init)
		goto not_inited;

	if (contrast_mode <= CAM_CONTRAST_INVALID || contrast_mode >= CAM_CONTRAST_MAX)
		goto invalid_arg;

	if(contrast_mode == CAM_CONTRAST_01)
		contrast = CONTRAST_PATTERN_01;
	else if(contrast_mode == CAM_CONTRAST_02)
		contrast = CONTRAST_PATTERN_02;
	else if(contrast_mode == CAM_CONTRAST_03)
		contrast = CONTRAST_PATTERN_03;
	else if(contrast_mode == CAM_CONTRAST_04)
		contrast = CONTRAST_PATTERN_04;
	else if(contrast_mode == CAM_CONTRAST_05)
		contrast = CONTRAST_PATTERN_05;
	else if(contrast_mode == CAM_CONTRAST_06)
		contrast = CONTRAST_PATTERN_06;
	else if(contrast_mode == CAM_CONTRAST_07)
		contrast = CONTRAST_PATTERN_07;
	else if(contrast_mode == CAM_CONTRAST_08)
		contrast = CONTRAST_PATTERN_08;
	else if(contrast_mode == CAM_CONTRAST_09)
		contrast = CONTRAST_PATTERN_09;
	else if(contrast_mode == CAM_CONTRAST_10)
		contrast = CONTRAST_PATTERN_10;
	else if(contrast_mode == CAM_CONTRAST_11)
		contrast = CONTRAST_PATTERN_11;

	result = CAMU_SetContrast(SELECT_ALL, contrast);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetContrast, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	return result;
}

uint32_t Util_cam_set_white_balance(Cam_white_balance white_balance_mode)
{
	uint32_t result = DEF_ERR_OTHER;
	CAMU_WhiteBalance white_balance = WHITE_BALANCE_AUTO;

	if(!util_cam_init)
		goto not_inited;

	if (white_balance_mode <= CAM_WHITE_BALANCE_INVALID || white_balance_mode >= CAM_WHITE_BALANCE_MAX)
		goto invalid_arg;

	if (white_balance_mode == CAM_WHITE_BALANCE_AUTO)
		white_balance = WHITE_BALANCE_AUTO;
	else if (white_balance_mode == CAM_WHITE_BALANCE_3200K)
		white_balance = WHITE_BALANCE_3200K;
	else if (white_balance_mode == CAM_WHITE_BALANCE_4150K)
		white_balance = WHITE_BALANCE_4150K;
	else if (white_balance_mode == CAM_WHITE_BALANCE_5200K)
		white_balance = WHITE_BALANCE_5200K;
	else if (white_balance_mode == CAM_WHITE_BALANCE_6000K)
		white_balance = WHITE_BALANCE_6000K;
	else if (white_balance_mode == CAM_WHITE_BALANCE_7000K)
		white_balance = WHITE_BALANCE_7000K;

	result = CAMU_SetWhiteBalance(SELECT_ALL, white_balance);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetWhiteBalance, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	return result;
}

uint32_t Util_cam_set_lens_correction(Cam_lens_correction lens_correction_mode)
{
	uint32_t result = DEF_ERR_OTHER;
	CAMU_LensCorrection lens_correction = LENS_CORRECTION_OFF;

	if(!util_cam_init)
		goto not_inited;

	if (lens_correction_mode <= CAM_LENS_CORRECTION_INVALID || lens_correction_mode >= CAM_LENS_CORRECTION_MAX)
		goto invalid_arg;

	if (lens_correction_mode == CAM_LENS_CORRECTION_OFF)
		lens_correction = LENS_CORRECTION_OFF;
	else if (lens_correction_mode == CAM_LENS_CORRECTION_70)
		lens_correction = LENS_CORRECTION_ON_70;
	else if (lens_correction_mode == CAM_LENS_CORRECTION_90)
		lens_correction = LENS_CORRECTION_ON_90;

	result = CAMU_SetLensCorrection(SELECT_ALL, lens_correction);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetLensCorrection, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	return result;
}

uint32_t Util_cam_set_camera(Cam_port camera_port)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t camera = SELECT_OUT2;

	if(!util_cam_init)
		goto not_inited;

	if (camera_port <= CAM_PORT_INVALID || camera_port >= CAM_PORT_MAX)
		goto invalid_arg;

	if(camera_port == CAM_PORT_OUT_LEFT)
		camera = SELECT_OUT2;
	else if(camera_port == CAM_PORT_OUT_RIGHT)
		camera = SELECT_OUT1;
	else if(camera_port == CAM_PORT_IN)
		camera = SELECT_IN1;

	result = Util_cam_set_resolution(util_cam_resolution);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_cam_set_resolution, false, result);
		goto error_other;
	}

	result = CAMU_Activate(camera);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_Activate, false, result);
		goto nintendo_api_failed;
	}
	util_cam_port = camera_port;

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	error_other:
	return result;
}

uint32_t Util_cam_set_exposure(Cam_exposure exposure_mode)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_cam_init)
		goto not_inited;

	if(exposure_mode <= CAM_EXPOSURE_INVALID || exposure_mode >= CAM_EXPOSURE_MAX)
		goto invalid_arg;

	result = CAMU_SetExposure(SELECT_ALL, (int8_t)exposure_mode);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetExposure, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	return result;
}

uint32_t Util_cam_set_noise_filter(bool noise_filter_mode)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_cam_init)
		goto not_inited;

	result = CAMU_SetNoiseFilter(SELECT_ALL, noise_filter_mode);
	if (result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(CAMU_SetNoiseFilter, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	nintendo_api_failed:
	return result;
}

void Util_cam_exit(void)
{
	if(!util_cam_init)
		return;

	camExit();
	util_cam_init = false;
}
#endif //DEF_CAM_API_ENABLE
