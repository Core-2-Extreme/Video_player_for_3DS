#include "system/headers.hpp"

#if DEF_ENABLE_CAM_API

bool util_cam_init = false;
u32 util_cam_buffer_size = 0;
int util_cam_width = 640;
int util_cam_height = 480;
int util_cam_mode = DEF_CAM_OUT_RIGHT;

Result_with_string Util_cam_init(int color_format)
{
	Result_with_string result;
	CAMU_OutputFormat color;
	int width = 0;
	int height = 0;

	if(util_cam_init)
		goto already_inited;

	if(color_format == DEF_CAM_OUT_RGB565)
		color = OUTPUT_RGB_565;
	else if(color_format == DEF_CAM_OUT_YUV422)
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
	util_cam_mode = DEF_CAM_OUT_RIGHT;

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

	result.code = CAMU_SetReceiving(&receive, *raw_data, util_cam_mode == DEF_CAM_OUT_LEFT ? PORT_CAM2 : PORT_CAM1, util_cam_width * util_cam_height * 2, (s16)util_cam_buffer_size);
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

Result_with_string Util_cam_set_resolution(int width, int height)
{
	CAMU_Size size;
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;
	
	if (width == 640 && height == 480)
		size = SIZE_VGA;
	else if (width == 512 && height == 384)
		size = SIZE_DS_LCDx4;
	else if (width == 400 && height == 240)
		size = SIZE_CTR_TOP_LCD;
	else if (width == 352 && height == 288)
		size = SIZE_CIF;
	else if (width == 320 && height == 240)
		size = SIZE_QVGA;
	else if (width == 256 && height == 192)
		size = SIZE_DS_LCD;
	else if (width == 176 && height == 144)
		size = SIZE_QCIF;
	else if (width == 160 && height == 120)
		size = SIZE_QQVGA;
	else
		goto invalid_arg;

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

Result_with_string Util_cam_set_fps(int fps_mode)
{
	CAMU_FrameRate framerate = FRAME_RATE_15;
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;
	
	if (fps_mode < 0 || fps_mode > 12)
		goto invalid_arg;

	if(fps_mode == DEF_CAM_FPS_5)
		framerate = FRAME_RATE_5;
	else if(fps_mode == DEF_CAM_FPS_8_5)
		framerate = FRAME_RATE_8_5;
	else if(fps_mode == DEF_CAM_FPS_10)
		framerate = FRAME_RATE_10;
	else if(fps_mode == DEF_CAM_FPS_15)
		framerate = FRAME_RATE_15;
	else if(fps_mode == DEF_CAM_FPS_20)
		framerate = FRAME_RATE_20;
	else if(fps_mode == DEF_CAM_FPS_30)
		framerate = FRAME_RATE_30;
	else if(fps_mode == DEF_CAM_FPS_15_TO_2)
		framerate = FRAME_RATE_15_TO_2;
	else if(fps_mode == DEF_CAM_FPS_15_TO_5)
		framerate = FRAME_RATE_15_TO_5;
	else if(fps_mode == DEF_CAM_FPS_15_TO_10)
		framerate = FRAME_RATE_15_TO_10;
	else if(fps_mode == DEF_CAM_FPS_20_TO_10)
		framerate = FRAME_RATE_20_TO_10;
	else if(fps_mode == DEF_CAM_FPS_30_TO_10)
		framerate = FRAME_RATE_30_TO_10;
	else if(fps_mode == DEF_CAM_FPS_30_TO_5)
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

Result_with_string Util_cam_set_contrast(int contrast_mode)
{
	CAMU_Contrast contrast = CONTRAST_PATTERN_01;
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;
	
	if (contrast_mode < 0 || contrast_mode > 10)
		goto invalid_arg;

	if(contrast_mode == DEF_CAM_CONTRAST_01)
		contrast = CONTRAST_PATTERN_01;
	else if(contrast_mode == DEF_CAM_CONTRAST_02)
		contrast = CONTRAST_PATTERN_02;
	else if(contrast_mode == DEF_CAM_CONTRAST_03)
		contrast = CONTRAST_PATTERN_03;
	else if(contrast_mode == DEF_CAM_CONTRAST_04)
		contrast = CONTRAST_PATTERN_04;
	else if(contrast_mode == DEF_CAM_CONTRAST_05)
		contrast = CONTRAST_PATTERN_05;
	else if(contrast_mode == DEF_CAM_CONTRAST_06)
		contrast = CONTRAST_PATTERN_06;
	else if(contrast_mode == DEF_CAM_CONTRAST_07)
		contrast = CONTRAST_PATTERN_07;
	else if(contrast_mode == DEF_CAM_CONTRAST_08)
		contrast = CONTRAST_PATTERN_08;
	else if(contrast_mode == DEF_CAM_CONTRAST_09)
		contrast = CONTRAST_PATTERN_09;
	else if(contrast_mode == DEF_CAM_CONTRAST_10)
		contrast = CONTRAST_PATTERN_10;
	else if(contrast_mode == DEF_CAM_CONTRAST_11)
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

Result_with_string Util_cam_set_white_balance(int white_balance_mode)
{
	CAMU_WhiteBalance white_balance = WHITE_BALANCE_AUTO;
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;
	
	if (white_balance_mode < 0 || white_balance_mode > 5)
		goto invalid_arg;

	if (white_balance_mode == DEF_CAM_WHITE_BALANCE_AUTO)
		white_balance = WHITE_BALANCE_AUTO;
	else if (white_balance_mode == DEF_CAM_WHITE_BALANCE_3200K)
		white_balance = WHITE_BALANCE_3200K;
	else if (white_balance_mode == DEF_CAM_WHITE_BALANCE_4150K)
		white_balance = WHITE_BALANCE_4150K;
	else if (white_balance_mode == DEF_CAM_WHITE_BALANCE_5200K)
		white_balance = WHITE_BALANCE_5200K;
	else if (white_balance_mode == DEF_CAM_WHITE_BALANCE_6000K)
		white_balance = WHITE_BALANCE_6000K;
	else if (white_balance_mode == DEF_CAM_WHITE_BALANCE_7000K)
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

Result_with_string Util_cam_set_lens_correction(int lens_correction_mode)
{
	CAMU_LensCorrection lens_correction = LENS_CORRECTION_OFF;
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;

	if (lens_correction_mode < 0 || lens_correction_mode > 2)
		goto invalid_arg;

	if (lens_correction_mode == DEF_CAM_LENS_CORRECTION_OFF)
		lens_correction = LENS_CORRECTION_OFF;
	else if (lens_correction_mode == DEF_CAM_LENS_CORRECTION_70)
		lens_correction = LENS_CORRECTION_ON_70;
	else if (lens_correction_mode == DEF_CAM_LENS_CORRECTION_90)
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

Result_with_string Util_cam_set_camera(int camera_mode)
{
	int camera = 0;
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;
	
	if (camera_mode < 0 || camera_mode > 2)
		goto invalid_arg;
	
	if(camera_mode == DEF_CAM_OUT_LEFT)
		camera = SELECT_OUT2;
	else if(camera_mode == DEF_CAM_OUT_RIGHT)
		camera = SELECT_OUT1;
	else if(camera_mode == DEF_CAM_IN)
		camera = SELECT_IN1;

	result = Util_cam_set_resolution(util_cam_width, util_cam_height);
	if (result.code != 0)
		return result;

	result.code = CAMU_Activate(camera);
	if (result.code != 0)
	{
		result.error_description = "[Error] CAMU_Activate() failed. ";
		goto nintendo_api_failed;
	}
	util_cam_mode = camera_mode;

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

Result_with_string Util_cam_set_exposure(int exposure_mode)
{
	int exposure = 0;
	Result_with_string result;
	if(!util_cam_init)
		goto not_inited;
	
	if(exposure < 0 || exposure > 5)
		goto invalid_arg;

	result.code = CAMU_SetExposure(SELECT_ALL, (s8)exposure);
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
