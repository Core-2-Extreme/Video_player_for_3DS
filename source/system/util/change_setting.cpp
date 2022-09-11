#include "system/headers.hpp"

Result_with_string Util_cset_set_screen_brightness(bool top_screen, bool bottom_screen, int brightness)
{
	Result_with_string result;
	int screen = -1;

	if (top_screen && bottom_screen)
		screen = GSPLCD_SCREEN_BOTH;
	else if (top_screen)
		screen = GSPLCD_SCREEN_TOP;
	else if (bottom_screen)
		screen = GSPLCD_SCREEN_BOTTOM;
	else
		goto invalid_arg;

	if(brightness < 0 || brightness > 200)
		goto invalid_arg;

	result.code = gspLcdInit();
	if(result.code != 0)
	{
		result.error_description = "[Error] gspLcdInit() failed. ";
		goto nintendo_api_failed_0;
	}

	result.code = GSPLCD_SetBrightnessRaw(screen, brightness);
	if(result.code != 0)
	{
		result.error_description = "GSPLCD_SetBrightnessRaw() failed. ";
		goto nintendo_api_failed;
	}

	gspLcdExit();
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	gspLcdExit();

	nintendo_api_failed_0:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cset_set_wifi_state(bool wifi_state)
{
	Result_with_string result;
	result.code = nwmExtInit();
	if(result.code != 0)
	{
		result.error_description = "[Error] nwmExtInit() failed. ";
		goto nintendo_api_failed_0;
	}

	result.code = NWMEXT_ControlWirelessEnabled(wifi_state);
	if(result.code != 0)
	{
		result.error_description = "[Error] NWMEXT_ControlWirelessEnabled() failed. ";
		goto nintendo_api_failed;
	}

	nwmExtExit();
	return result;

	nintendo_api_failed:
	nwmExtExit();

	nintendo_api_failed_0:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cset_set_screen_state(bool top_screen, bool bottom_screen, bool state)
{
	Result_with_string result;
	int screen = -1;

	if (top_screen && bottom_screen)
		screen = GSPLCD_SCREEN_BOTH;
	else if (top_screen)
		screen = GSPLCD_SCREEN_TOP;
	else if (bottom_screen)
		screen = GSPLCD_SCREEN_BOTTOM;
	else
		goto invalid_arg;

	result.code = gspLcdInit();
	if(result.code != 0)
	{
		result.error_description = "[Error] gspLcdInit() failed. ";
		goto nintendo_api_failed_0;
	}

	if(state)
	{
		result.code = GSPLCD_PowerOnBacklight(screen);
		if(result.code != 0)
		{
			result.error_description = "[Error] GSPLCD_PowerOnBacklight() failed. ";
			goto nintendo_api_failed;
		}
	}
	else
	{
		result.code = GSPLCD_PowerOffBacklight(screen);
		if(result.code != 0)
		{
			result.error_description = "[Error] GSPLCD_PowerOffBacklight() failed. ";
			goto nintendo_api_failed;
		}
	}

	gspLcdExit();
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	gspLcdExit();

	nintendo_api_failed_0:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

Result_with_string Util_cset_sleep_system(int wake_up_event)
{
	PtmWakeEvents wake_up_event_mask;
	Result_with_string result;

	if(!(wake_up_event & DEF_CSET_WAKE_UP_PRESS_HOME_BUTTON) && !(wake_up_event & DEF_CSET_WAKE_UP_OPEN_SHELL)) 
		goto invalid_arg;

	wake_up_event_mask.mcu_interupt_mask = wake_up_event;
	wake_up_event_mask.pdn_wake_events = 0;

	result.code = APT_SleepSystem(&wake_up_event_mask);
	if(result.code != 0)
	{
		result.error_description = "[Error] APT_SleepSystem() failed. ";
		goto nintendo_api_failed;
	}

	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}
