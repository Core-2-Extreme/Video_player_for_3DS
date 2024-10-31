//Includes.
#include "system/util/hw_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
//N/A.

//Variables.
//N/A.

//Code.
uint32_t Util_hw_config_set_screen_brightness(bool top_screen, bool bottom_screen, uint8_t brightness)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t screen = GSPLCD_SCREEN_BOTH;

	if (top_screen && bottom_screen)
		screen = GSPLCD_SCREEN_BOTH;
	else if (top_screen)
		screen = GSPLCD_SCREEN_TOP;
	else if (bottom_screen)
		screen = GSPLCD_SCREEN_BOTTOM;
	else
		goto invalid_arg;

	if(brightness > 200)
		goto invalid_arg;

	result = gspLcdInit();
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(gspLcdInit, false, result);
		goto nintendo_api_failed_0;
	}

	result = GSPLCD_SetBrightnessRaw(screen, brightness);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(GSPLCD_SetBrightnessRaw, false, result);
		goto nintendo_api_failed;
	}

	gspLcdExit();
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	gspLcdExit();
	//Fallthrough.

	nintendo_api_failed_0:
	return result;
}

uint32_t Util_hw_config_set_wifi_state(bool wifi_state)
{
	uint32_t result = DEF_ERR_OTHER;

	result = nwmExtInit();
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(nwmExtInit, false, result);
		goto nintendo_api_failed_0;
	}

	result = NWMEXT_ControlWirelessEnabled(wifi_state);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(NWMEXT_ControlWirelessEnabled, false, result);
		goto nintendo_api_failed;
	}

	nwmExtExit();
	return DEF_SUCCESS;

	nintendo_api_failed:
	nwmExtExit();
	//Fallthrough.

	nintendo_api_failed_0:
	return result;
}

uint32_t Util_hw_config_set_screen_state(bool top_screen, bool bottom_screen, bool state)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t screen = GSPLCD_SCREEN_BOTH;

	if (top_screen && bottom_screen)
		screen = GSPLCD_SCREEN_BOTH;
	else if (top_screen)
		screen = GSPLCD_SCREEN_TOP;
	else if (bottom_screen)
		screen = GSPLCD_SCREEN_BOTTOM;
	else
		goto invalid_arg;

	result = gspLcdInit();
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(gspLcdInit, false, result);
		goto nintendo_api_failed_0;
	}

	if(state)
	{
		result = GSPLCD_PowerOnBacklight(screen);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(GSPLCD_PowerOnBacklight, false, result);
			goto nintendo_api_failed;
		}
	}
	else
	{
		result = GSPLCD_PowerOffBacklight(screen);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(GSPLCD_PowerOffBacklight, false, result);
			goto nintendo_api_failed;
		}
	}

	gspLcdExit();
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	nintendo_api_failed:
	gspLcdExit();
	//Fallthrough.

	nintendo_api_failed_0:
	return result;
}

uint32_t Util_hw_config_sleep_system(Hw_config_wakeup_bit wakeup_events)
{
	uint32_t result = DEF_ERR_OTHER;
	PtmWakeEvents wake_up_event_mask = { 0, };

	if(!(wakeup_events & HW_CONFIG_WAKEUP_BIT_PRESS_HOME_BUTTON) && !(wakeup_events & HW_CONFIG_WAKEUP_BIT_OPEN_SHELL))
		goto invalid_arg;

	if(!aptIsSleepAllowed())
	{
		result = DEF_ERR_OTHER;
		DEF_LOG_FORMAT("Sleep is NOT allowed!!!!!");
		goto not_allowed;
	}

	wake_up_event_mask.mcu_interupt_mask = (uint32_t)wakeup_events;
	wake_up_event_mask.pdn_wake_events = 0;

	result = APT_SleepSystem(&wake_up_event_mask);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(APT_SleepSystem, false, result);
		goto nintendo_api_failed;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	not_allowed:
	nintendo_api_failed:
	return result;
}
