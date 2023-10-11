#include "definitions.hpp"

#if DEF_ENABLE_CAM_API
#include "system/types.hpp"

#include "system/util/util.hpp"

//Include myself.
#include "system/util/camera.hpp"

bool util_cam_init = false;
u32 util_cam_buffer_size = 0;
int util_cam_width = 640;
int util_cam_height = 480;
Camera_resolution util_cam_resolution = CAM_RES_640x480;
Camera_port util_cam_port = CAM_PORT_OUT_RIGHT;

Result_with_string Util_cam_init(Pixel_format color_format)
{
	Result_with_string result;
	CAMU_OutputFormat color;
	int width = 0;
	int height = 0;

	if(util_cam_init)
		goto already_inited;

	if(color_format == PIXEL_FORMAT_RGB565LE)
		color = OUTPUT_RGB_565;
	else if(color_format == PIXEL_FORMAT_YUV422P)
		color = OUTPUT_YUV_422;
	else
		goto invalid_arg;

	result.code = camInit();
	if (result.code != 0)
	{
		result.error_description = "[Error] camInit() failed. ";
		goto nintendo_api_failed_0;
	}

	result.code = CAMU_SetOutputFormat(SELECT_ALL, color, CONTEXT_BOTH);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetOutputFormat() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetNoiseFilter(SELECT_ALL, true);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetNoiseFilter() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetAutoExposure(SELECT_ALL, true);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetAutoExposure() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetWhiteBalance(SELECT_ALL, WHITE_BALANCE_AUTO);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetWhiteBalance() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetPhotoMode(SELECT_ALL, PHOTO_MODE_NORMAL);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetPhotoMode() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetTrimming(PORT_BOTH, false);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetTrimming() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetFrameRate(SELECT_ALL, FRAME_RATE_30);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetFrameRate() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetContrast(SELECT_ALL, CONTRAST_NORMAL);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetContrast() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetLensCorrection(SELECT_ALL, LENS_CORRECTION_NORMAL);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetLensCorrection() failed. ";
		goto nintendo_api_failed;
	}

	width = 640;
	height = 480;
	result.code = CAMU_SetSize(SELECT_ALL, SIZE_VGA, CONTEXT_BOTH);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetSize() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_GetMaxBytes(&util_cam_buffer_size, width, height);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_GetMaxBytes() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetTransferBytes(PORT_BOTH, util_cam_buffer_size, width, height);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetTransferBytes() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_Activate(SELECT_OUT1);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_Activate() failed. ";
		goto nintendo_api_failed;
	}
	util_cam_port = CAM_PORT_OUT_RIGHT;

	util_cam_width = width;
	util_cam_height = height;
	util_cam_init = true;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	nintendo_api_failed:
	camExit();

	nintendo_api_failed_0:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_take_a_picture(u8** raw_data, int* width, int* height, bool shutter_sound)
{
	Result_with_string result;
	Handle receive = 0;
	if(!util_cam_init)
		goto not_inited;

	if(!raw_data || !width || !height)
		goto invalid_arg;

	Util_safe_linear_free(*raw_data);
	*raw_data = (u8*)Util_safe_linear_alloc(util_cam_width * util_cam_height * 2);
	if(*raw_data == NULL)
		goto out_of_memory;

	result.code = CAMU_StartCapture(PORT_BOTH);
	if(result.code != 0)
	{
		result.error_description = "[Error] CAMU_StartCapture() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetReceiving(&receive, *raw_data, util_cam_port == CAM_PORT_OUT_LEFT ? PORT_CAM2 : PORT_CAM1, util_cam_width * util_cam_height * 2, (s16)util_cam_buffer_size);
	if(result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetReceiving() failed. ";
		goto nintendo_api_failed;
	}

	result.code = svcWaitSynchronization(receive, 1000000000);
	if(result.code != 0)
	{
		result.error_description = "[Error] svcWaitSynchronization() failed. ";
		goto nintendo_api_failed;
	}

	if(shutter_sound)
	{
		result.code = CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_NORMAL);
		if(result.code != 0)
		{
			result.error_description = "[Error] CAMU_PlayShutterSound() failed. ";
			goto nintendo_api_failed;
		}
	}

	*width = util_cam_width;
	*height = util_cam_height;
	svcCloseHandle(receive);

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	svcCloseHandle(receive);
	return result;
}

Result_with_string Util_cam_set_resolution(Camera_resolution resolution_mode)
{
	s16 width = 0;
	s16 height = 0;
	CAMU_Size size;
	Result_with_string result;
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

	result.code = CAMU_SetSize(SELECT_ALL, size, CONTEXT_BOTH);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetSize() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_GetMaxBytes(&util_cam_buffer_size, width, height);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_GetMaxBytes() failed. ";
		goto nintendo_api_failed;
	}

	result.code = CAMU_SetTransferBytes(PORT_BOTH, util_cam_buffer_size, width, height);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetTransferBytes() failed. ";
		goto nintendo_api_failed;
	}

	util_cam_width = width;
	util_cam_height = height;
	util_cam_resolution = resolution_mode;

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_set_fps(Camera_framerate fps_mode)
{
	CAMU_FrameRate framerate = FRAME_RATE_15;
	Result_with_string result;
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

	result.code = CAMU_SetFrameRate(SELECT_ALL, framerate);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetFrameRate() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_set_contrast(Camera_contrast contrast_mode)
{
	CAMU_Contrast contrast = CONTRAST_PATTERN_01;
	Result_with_string result;
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

	result.code = CAMU_SetContrast(SELECT_ALL, contrast);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetContrast() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_set_white_balance(Camera_white_balance white_balance_mode)
{
	CAMU_WhiteBalance white_balance = WHITE_BALANCE_AUTO;
	Result_with_string result;
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

	result.code = CAMU_SetWhiteBalance(SELECT_ALL, white_balance);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetWhiteBalance() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_set_lens_correction(Camera_lens_correction lens_correction_mode)
{
	CAMU_LensCorrection lens_correction = LENS_CORRECTION_OFF;
	Result_with_string result;
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

	result.code = CAMU_SetLensCorrection(SELECT_ALL, lens_correction);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetLensCorrection() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_set_camera(Camera_port camera_port)
{
	int camera = 0;
	Result_with_string result;
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
	if (result.code != 0)
		return result;

	result.code = CAMU_Activate(camera);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_Activate() failed. ";
		goto nintendo_api_failed;
	}
	util_cam_port = camera_port;

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_set_exposure(Camera_exposure exposure_mode)
{
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;

	if(exposure_mode <= CAM_EXPOSURE_INVALID || exposure_mode >= CAM_EXPOSURE_MAX)
		goto invalid_arg;

	result.code = CAMU_SetExposure(SELECT_ALL, (s8)exposure_mode);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetExposure() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cam_set_noise_filter(bool noise_filter_mode)
{
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;

	result.code = CAMU_SetNoiseFilter(SELECT_ALL, noise_filter_mode);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_SetNoiseFilter() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_cam_exit(void)
{
	if(!util_cam_init)
		return;

	camExit();
	util_cam_init = false;
}

#endif
