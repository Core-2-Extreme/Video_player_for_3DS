#include "system/headers.hpp"

bool util_cam_init = false;
u32 util_cam_buffer_size = 0;
int util_cam_width = 640;
int util_cam_height = 480;

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

	result.code = CAMU_SetReceiving(&receive, *raw_data, PORT_CAM1, util_cam_width * util_cam_height * 2, (s16)util_cam_buffer_size);
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

void Util_cam_exit(void)
{
	if(!util_cam_init)
		return;
	
	camExit();
	util_cam_init = false;
}