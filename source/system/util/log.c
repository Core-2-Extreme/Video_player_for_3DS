//Includes.
#include "system/util/log.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "system/draw/draw.h"
#include "system/util/err_types.h"
#include "system/util/file.h"
#include "system/util/hid.h"
#include "system/util/str.h"
#include "system/util/sync.h"
#include "system/util/watch.h"

//Defines.
#define DISPLAYED_LINES					(uint8_t)(23)

//Close.
#define HID_NOT_INITED_CLOSE_CFM(k)		(bool)(DEF_HID_PR_EM((k).a, 1) || DEF_HID_HD((k).a))
//Next item.
#define HID_NEXT_ITEM_CFM(k)			(bool)(DEF_HID_PHY_PR((k).c_up) || DEF_HID_PHY_HE((k).c_up))
//Previous item.
#define HID_PRE_ITEM_CFM(k)				(bool)(DEF_HID_PHY_PR((k).c_down) || DEF_HID_PHY_HE((k).c_down))
//Pan left.
#define HID_PAN_LEFT_CFM(k)				(bool)(DEF_HID_PHY_PR((k).c_left) || DEF_HID_PHY_HE((k).c_left))
//Pan Right.
#define HID_PAN_RIGHT_CFM(k)			(bool)(DEF_HID_PHY_PR((k).c_right) || DEF_HID_PHY_HE((k).c_right))

#define FONT_SIZE_ERROR					(float)(13.50)	//Font size for API error.
#define FONT_SIZE_LOG					(float)(12.75)	//Font size for logs.

//Typedefs.
//N/A.

//Prototypes.
static uint32_t Util_log_add_internal(uint32_t log_index, bool append_time, const char* caller, const char* format_string, va_list args);

//Variables.
static bool util_log_show_flag = false;
static bool util_log_init = false;
static uint32_t util_log_current_index = 0;
static uint32_t util_log_y = 0;
static double util_log_x = 0;
static double util_log_uptime_ms = 0;
static double util_log_spend_time[DEF_LOG_BUFFER_LINES] = { 0, };
static Str_data util_log_logs[DEF_LOG_BUFFER_LINES] = { 0, };
static Sync_data util_log_mutex = { 0, };
static TickCounter util_log_uptime_stopwatch = { 0, };

//Code.
uint32_t Util_log_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_log_init)
		goto already_inited;

	osTickCounterStart(&util_log_uptime_stopwatch);
	util_log_current_index = 0;
	util_log_y = 0;
	util_log_x = 0.0;
	util_log_uptime_ms = 0;

	result = Util_sync_create(&util_log_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	for(uint32_t i = 0; i < DEF_LOG_BUFFER_LINES; i++)
	{
		util_log_spend_time[i] = 0;

		result = Util_str_init(&util_log_logs[i]);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto error_other;
		}
	}

	Util_watch_add(WATCH_HANDLE_GLOBAL, &util_log_show_flag, sizeof(util_log_show_flag));
	Util_watch_add(WATCH_HANDLE_LOG, &util_log_x, sizeof(util_log_x));
	Util_watch_add(WATCH_HANDLE_LOG, &util_log_y, sizeof(util_log_y));

	util_log_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	error_other:
	for(uint32_t i = 0; i < DEF_LOG_BUFFER_LINES; i++)
		Util_str_free(&util_log_logs[i]);

	Util_sync_destroy(&util_log_mutex);
	return result;
}

void Util_log_exit(void)
{
	if(!util_log_init)
		return;

	util_log_init = false;

	for(uint32_t i = 0; i < DEF_LOG_BUFFER_LINES; i++)
		Util_str_free(&util_log_logs[i]);

	Util_watch_remove(WATCH_HANDLE_GLOBAL, &util_log_show_flag);
	Util_watch_remove(WATCH_HANDLE_LOG, &util_log_x);
	Util_watch_remove(WATCH_HANDLE_LOG, &util_log_y);
	Util_sync_destroy(&util_log_mutex);
}

uint32_t Util_log_dump(const char* file_name, const char* dir_path)
{
	uint32_t result = DEF_ERR_OTHER;
	Str_data log = { 0, };

	if(!util_log_init)
		goto not_inited;

	if(!file_name || !dir_path)
		goto invalid_arg;

	result = Util_str_init(&log);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error_other;
	}

	for(uint32_t i = 0; i < DEF_LOG_BUFFER_LINES; i++)
	{
		if(!Util_str_has_data(&util_log_logs[i]))
			continue;

		Util_str_add(&log, util_log_logs[i].buffer);
		Util_str_add(&log, "\n");
	}

	if(Util_str_has_data(&log))
	{
		result = Util_file_save_to_file(file_name, dir_path, (uint8_t*)log.buffer, log.length, true);
		if(result != DEF_SUCCESS)
			DEF_LOG_RESULT(Util_file_save_to_file, false, result);
	}

	Util_str_free(&log);

	return result;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error_other:
	return result;
}

bool Util_log_query_show_flag(void)
{
	if(!util_log_init)
		return false;
	else
		return util_log_show_flag;
}

void Util_log_set_show_flag(bool flag)
{
	if(!util_log_init)
		return;

	util_log_show_flag = flag;
	Draw_set_refresh_needed(true);
}

uint32_t Util_log_format(const char* caller, const char* format_string, ...)
{
	uint32_t log_index = 0;
	va_list args;

	va_start(args, format_string);
	log_index = Util_log_add_internal(DEF_LOG_INDEX_AUTO, false, caller, format_string, args);
	va_end(args);

	return log_index;
}

uint32_t Util_log_vformat(const char* caller, const char* format_string, va_list args)
{
	return Util_log_add_internal(DEF_LOG_INDEX_AUTO, false, caller, format_string, args);
}

uint32_t Util_log_format_append(uint32_t log_index, bool append_elapsed_time, const char* format_string, ...)
{
	va_list args;

	va_start(args, format_string);
	log_index = Util_log_add_internal(log_index, append_elapsed_time, "", format_string, args);
	va_end(args);

	return log_index;
}

uint32_t Util_log_vformat_append(uint32_t log_index, bool append_elapsed_time, const char* format_string, va_list args)
{
	return Util_log_add_internal(log_index, append_elapsed_time, "", format_string, args);
}

uint32_t Util_log_save_result(const char* caller, const char* function_name, bool is_success, uint32_t result)
{
	if(!function_name || strlen(function_name) == 0)
	{
		if(is_success)
			return Util_log_format(caller, "xxxx()...0x%" PRIX32 " [Success]", result);
		else
			return Util_log_format(caller, "xxxx()...0x%" PRIX32 " [Error]", result);
	}
	else
	{
		if(is_success)
			return Util_log_format(caller, "%s()...0x%" PRIX32 " [Success]", function_name, result);
		else
			return Util_log_format(caller, "%s()...0x%" PRIX32 " [Error]", function_name, result);
	}
}

uint32_t Util_log_save_result_start(const char* caller, const char* function_name, bool is_smart_macro, bool omit_args)
{
	if(!function_name || strlen(function_name) == 0)
		return Util_log_format(caller, "xxxx()...");
	else if(omit_args)
	{
		const char* arg_start = strstr(function_name, "(");

		if(!arg_start)
			return Util_log_format(caller, "%s()...", function_name);
		else
		{
			uint32_t function_name_size = (arg_start - function_name);
			return Util_log_format(caller, "%.*s()...", function_name_size, function_name);
		}
	}
	else
	{
		if(is_smart_macro)//Smart macro already includes "()", so we don't append it here.
			return Util_log_format(caller, "%s...", function_name);
		else
			return Util_log_format(caller, "%s()...", function_name);
	}
}

uint32_t Util_log_save_result_end(uint32_t log_index, bool is_success, uint32_t result)
{
	if(is_success)
		return Util_log_format_append(log_index, true, "0x%" PRIX32 " [Success]", result);
	else
		return Util_log_format_append(log_index, true, "0x%" PRIX32 " [Error]", result);
}

uint32_t Util_log_save_bool(const char* caller, const char* symbol_name, bool value)
{
	if(!symbol_name || strlen(symbol_name) == 0)
		return Util_log_format(caller, "%s", (value ? "true" : "false"));
	else
		return Util_log_format(caller, "%s : %s", symbol_name, (value ? "true" : "false"));
}

uint32_t Util_log_save_int(const char* caller, const char* symbol_name, int64_t value)
{
	if(!symbol_name || strlen(symbol_name) == 0)
		return Util_log_format(caller, "%" PRIi64, value);
	else
		return Util_log_format(caller, "%s : %" PRIi64, symbol_name, value);
}

uint32_t Util_log_save_uint(const char* caller, const char* symbol_name, uint64_t value)
{
	if(!symbol_name || strlen(symbol_name) == 0)
		return Util_log_format(caller, "%" PRIu64, value);
	else
		return Util_log_format(caller, "%s : %" PRIu64, symbol_name, value);
}

uint32_t Util_log_save_hex(const char* caller, const char* symbol_name, uint64_t value)
{
	if(!symbol_name || strlen(symbol_name) == 0)
		return Util_log_format(caller, "0x%" PRIx64, value);
	else
		return Util_log_format(caller, "%s : 0x%" PRIx64, symbol_name, value);
}

uint32_t Util_log_save_double(const char* caller, const char* symbol_name, double value)
{
	if(!symbol_name || strlen(symbol_name) == 0)
		return Util_log_format(caller, "%f", value);
	else
		return Util_log_format(caller, "%s : %f", symbol_name, value);
}

uint32_t Util_log_save_string(const char* caller, const char* symbol_name, const char* text)
{
	if(!symbol_name || strlen(symbol_name) == 0 || symbol_name[0] == '"')
		return Util_log_format(caller, "%s", text);
	else
		return Util_log_format(caller, "%s : %s", symbol_name, text);
}

void Util_log_main(const Hid_info* key)
{
	if(!key)
		return;

	if(!util_log_init)
	{
		//Execute functions if conditions are satisfied.
		if (HID_NOT_INITED_CLOSE_CFM(*key))
		{
			util_log_show_flag = false;
			//Reset key state on scene change.
			Util_hid_reset_key_state(HID_KEY_BIT_ALL);
		}
		return;
	}

	//Execute functions if conditions are satisfied.
	if(HID_NEXT_ITEM_CFM(*key))
	{
		if (util_log_y > 0)
			util_log_y--;
	}
	else if(HID_PRE_ITEM_CFM(*key))
	{
		if (util_log_y + 1 <= DEF_LOG_BUFFER_LINES - DISPLAYED_LINES)
			util_log_y++;
	}

	if(HID_PAN_LEFT_CFM(*key))
	{
		if (util_log_x + 5.0 < 0.0)
			util_log_x += 5.0;
		else
			util_log_x = 0.0;
	}
	else if (HID_PAN_RIGHT_CFM(*key))
	{
		if (util_log_x - 5.0 > -2000.0)
			util_log_x -= 5.0;
		else
			util_log_x = -2000.0;
	}
}

void Util_log_draw(void)
{
	if(!util_log_init)
	{
		Draw_c("Log API is not initialized.\nPress A to close.", 0, 10, FONT_SIZE_ERROR, DEF_DRAW_RED);
		return;
	}

	for (uint16_t i = 0; i < DISPLAYED_LINES; i++)
		Draw(&util_log_logs[util_log_y + i], util_log_x, 10.0 + (i * 10), FONT_SIZE_LOG, DEF_LOG_COLOR);
}

static uint32_t Util_log_add_internal(uint32_t log_index, bool append_time, const char* caller, const char* format_string, va_list args)
{
	bool is_append = true;
	char empty_char = 0x00;
	char error_msg[] = "(Couldn't format string, this is usually due to out of memory.)";
	Str_data temp_text = { 0, };

	if(!util_log_init)
		return DEF_LOG_INDEX_AUTO;

	if(log_index != DEF_LOG_INDEX_AUTO && log_index >= DEF_LOG_BUFFER_LINES)
		return DEF_LOG_INDEX_AUTO;

	if(!caller)
		caller = &empty_char;

	if(!format_string)
		format_string = &empty_char;

	Util_str_init(&temp_text);
	Util_str_vformat(&temp_text, format_string, args);

	Util_sync_lock(&util_log_mutex, UINT64_MAX);

	//Use next index if index is not specified.
	if(log_index == DEF_LOG_INDEX_AUTO)
	{
		log_index = util_log_current_index;
		//Overwrite the old log in this case.
		is_append = false;
	}

	osTickCounterUpdate(&util_log_uptime_stopwatch);
	util_log_uptime_ms += osTickCounterRead(&util_log_uptime_stopwatch);

	if(is_append)
	{
		//For append call, user may want to know the time difference
		//since first call to this function (with same log_index).
		//If caller want to do so, append it.
		if(append_time)
		{
			double time_diff_ms = (util_log_uptime_ms - util_log_spend_time[log_index]);
			Util_str_format_append(&util_log_logs[log_index], "%s (%.2fms)", (Util_str_is_valid(&temp_text) ? temp_text.buffer : error_msg), time_diff_ms);
		}
		else
			Util_str_format_append(&util_log_logs[log_index], "%s", (Util_str_is_valid(&temp_text) ? temp_text.buffer : error_msg));
	}
	else
	{
		bool auto_scroll = false;

		Util_str_format(&util_log_logs[log_index], "[%.5f][%s] %s", (util_log_uptime_ms / 1000), caller, (Util_str_is_valid(&temp_text) ? temp_text.buffer : error_msg));

		//Save timestamp for later append call to this log index.
		util_log_spend_time[log_index] = util_log_uptime_ms;

		//If user sees last log, auto scroll logs.
		auto_scroll = (util_log_y == (util_log_current_index - DISPLAYED_LINES));

		//Increment log index.
		util_log_current_index++;
		if (util_log_current_index >= DEF_LOG_BUFFER_LINES)
			util_log_current_index = 0;

		if(auto_scroll)
		{
			if (util_log_current_index < DISPLAYED_LINES)
				util_log_y = 0;
			else
				util_log_y = util_log_current_index - DISPLAYED_LINES;
		}
	}

	//Truncate logs if it's too long.
	if(util_log_logs[log_index].length > DEF_LOG_MAX_LENGTH)
		Util_str_resize(&util_log_logs[log_index], DEF_LOG_MAX_LENGTH);

	Util_sync_unlock(&util_log_mutex);
	Util_str_free(&temp_text);

	if(util_log_show_flag)
		Draw_set_refresh_needed(true);

	return log_index;
}
