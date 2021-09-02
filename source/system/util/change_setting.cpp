#include "system/headers.hpp"

Result_with_string Util_cset_set_screen_brightness(bool top_screen, bool bottom_screen, int brightness)
{
	gspLcdInit();
	Result_with_string result;
	int screen = -1;

	if (top_screen && bottom_screen)
		screen = GSPLCD_SCREEN_BOTH;
	else if (top_screen)
		screen = GSPLCD_SCREEN_TOP;
	else if (bottom_screen)
		screen = GSPLCD_SCREEN_BOTTOM;
	else
		result.code = DEF_ERR_INVALID_ARG;

	if(result.code != DEF_ERR_INVALID_ARG)
	{
		result.code = GSPLCD_SetBrightnessRaw(screen, brightness);
		if(result.code != 0)
			result.string = "GSPLCD_SetBrightnessRaw() failed.";
	}

	gspLcdExit();
	return result;
}

Result_with_string Util_cset_set_wifi_state(bool wifi_state)
{
	nwmExtInit();
	Result_with_string result;

	result.code = NWMEXT_ControlWirelessEnabled(wifi_state);
	if(result.code != 0)
		result.string = "NWMEXT_ControlWirelessEnabled() failed.";

	nwmExtExit();
	return result;
}

Result_with_string Util_cset_set_screen_state(bool top_screen, bool bottom_screen, bool state)
{
	gspLcdInit();
	Result_with_string result;
	int screen = -1;

	if (top_screen && bottom_screen)
		screen = GSPLCD_SCREEN_BOTH;
	else if (top_screen)
		screen = GSPLCD_SCREEN_TOP;
	else if (bottom_screen)
		screen = GSPLCD_SCREEN_BOTTOM;
	else
		result.code = DEF_ERR_INVALID_ARG;

	if(result.code != DEF_ERR_INVALID_ARG)
	{
		if (state)
		{
			result.code = GSPLCD_PowerOnBacklight(screen);
			if(result.code != 0)
				result.string = "GSPLCD_PowerOnBacklight() failed.";
		}
		else
		{
			result.code = GSPLCD_PowerOffBacklight(screen);
			if(result.code != 0)
				result.string = "GSPLCD_PowerOffBacklight() failed.";
		}
	}

	gspLcdExit();
	return result;
}
