#include "system/headers.hpp"

bool util_cam_init = false;
u32 util_cam_buffer_size = 0;
int util_cam_width = 640;
int util_cam_height = 480;

Result_with_string Util_cam_init(std::string color_format)
{
	Result_with_string result;
	CAMU_OutputFormat color;

	if(util_cam_init)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] Camera is already initialized. ";
		return result;
	}

	if(color_format == "bgr565")
		color = OUTPUT_RGB_565;
	else if(color_format == "yuv422")
		color = OUTPUT_YUV_422;
	else
	{
		result.code = DEF_ERR_INVALID_ARG;
		result.string = DEF_ERR_INVALID_ARG_STR;
		return result;
	}

	result.code = camInit();
	if (result.code != 0)
	{
		result.string = "[Error] camInit() failed. ";
		return result;
	}

	result.code = CAMU_SetOutputFormat(SELECT_ALL, color, CONTEXT_BOTH);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetOutputFormat() failed. ";
		return result;
	}

	result.code = CAMU_SetNoiseFilter(SELECT_ALL, true);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetNoiseFilter() failed. ";
		return result;
	}

	result.code = CAMU_SetAutoExposure(SELECT_ALL, true);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetAutoExposure() failed. ";
		return result;
	}

	result.code = CAMU_SetWhiteBalance(SELECT_ALL, WHITE_BALANCE_AUTO);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetWhiteBalance() failed. ";
		return result;
	}

	result.code = CAMU_SetPhotoMode(SELECT_ALL, PHOTO_MODE_NORMAL);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetPhotoMode() failed. ";
		return result;
	}

	result.code = CAMU_SetTrimming(PORT_BOTH, false);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetTrimming() failed. ";
		return result;
	}

	result.code = CAMU_SetFrameRate(SELECT_ALL, FRAME_RATE_30);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetFrameRate() failed. ";
		return result;
	}

	result.code = CAMU_SetContrast(SELECT_ALL, CONTRAST_NORMAL);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetContrast() failed. ";
		return result;
	}

	result.code = CAMU_SetLensCorrection(SELECT_ALL, LENS_CORRECTION_NORMAL);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetLensCorrection() failed. ";
		return result;
	}

	util_cam_width = 640;
	util_cam_height = 480;
	result.code = CAMU_SetSize(SELECT_ALL, SIZE_VGA, CONTEXT_BOTH);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetSize() failed. ";
		return result;
	}

	result.code = CAMU_GetMaxBytes(&util_cam_buffer_size, 640, 480);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_GetMaxBytes() failed. ";
		return result;
	}

	result.code = CAMU_SetTransferBytes(PORT_BOTH, util_cam_buffer_size, 640, 480);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_SetTransferBytes() failed. ";
		return result;
	}

	result.code = CAMU_Activate(SELECT_OUT1);
	if (result.code != 0)
	{
		camExit();
		result.string = "[Error] CAMU_Activate() failed. ";
		return result;
	}

	util_cam_init = true;
	return result;
}

Result_with_string Util_cam_take_a_picture(u8** raw_data, int* width, int* height, bool shutter_sound)
{
	Result_with_string result;
	Handle receive = 0;
	if(!util_cam_init)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] camera is not initialized. ";
		return result;
	}

	*raw_data = (u8*)malloc(util_cam_width * util_cam_height * 2);
	if(*raw_data == NULL)
	{
		result.code = DEF_ERR_OUT_OF_MEMORY;
		result.string = DEF_ERR_OUT_OF_MEMORY_STR;
		return result;
	}

	result.code = CAMU_StartCapture(PORT_BOTH);
	if(result.code != 0)
	{
		result.string = "[Error] CAMU_StartCapture failed. ";
		return result;
	}

	result.code = CAMU_SetReceiving(&receive, *raw_data, PORT_CAM1, util_cam_width * util_cam_height * 2, (s16)util_cam_buffer_size);
	if(result.code != 0)
	{
		result.string = "[Error] CAMU_SetReceiving failed. ";
		return result;
	}

	result.code = svcWaitSynchronization(receive, 1000000000);
	if(result.code != 0)
	{
		svcCloseHandle(receive);
		result.string = "[Error] svcWaitSynchronization failed. ";
		return result;
	}
	*width = util_cam_width;
	*height = util_cam_height;

	if(shutter_sound)
	{
		result.code = CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_NORMAL);
		if(result.code != 0)
		{
			svcCloseHandle(receive);
			result.string = "[Error] CAMU_PlayShutterSound failed. ";
			return result;
		}
	}

	svcCloseHandle(receive);

	return result;
}

Result_with_string Util_cam_set_resolution(int width, int height)
{
	CAMU_Size size;
	Result_with_string result;
	if(!util_cam_init)
	{
		result.code = DEF_ERR_OTHER;
		result.string = "[Error] camera is not initialized. ";
		return result;
	}
	
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
	{
		result.code = DEF_ERR_INVALID_ARG;
		result.string = DEF_ERR_INVALID_ARG_STR;
		return result;
	}

	result.code = CAMU_SetSize(SELECT_ALL, size, CONTEXT_BOTH);
	if (result.code != 0)
	{
		result.string = "[Error] CAMU_SetSize() failed. ";
		return result;
	}
	util_cam_width = width;
	util_cam_height = height;

	result.code = CAMU_GetMaxBytes(&util_cam_buffer_size, width, height);
	if (result.code != 0)
	{
		result.string = "[Error] CAMU_GetMaxBytes() failed. ";
		return result;
	}

	result.code = CAMU_SetTransferBytes(PORT_BOTH, util_cam_buffer_size, width, height);
	if (result.code != 0)
	{
		result.string = "[Error] CAMU_SetTransferBytes() failed. ";
		return result;
	}

	return result;
}

void Util_cam_exit(void)
{
	camExit();
	util_cam_init = false;
}