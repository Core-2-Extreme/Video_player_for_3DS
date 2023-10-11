#include "definitions.hpp"
#include "system/types.hpp"

#include "system/variables.hpp"

#include "system/draw/draw.hpp"

#include "system/util/file.hpp"

//Include myself.
#include "system/util/log.hpp"

bool util_log_show_flag = false;
bool util_log_init = false;
int util_log_current_index = 0;
int util_log_y = 0;
double util_log_x = 0.0;
double util_log_uptime_ms = 0.0;
double util_log_spend_time[DEF_LOG_BUFFER_LINES];
std::string util_log_logs[DEF_LOG_BUFFER_LINES];
TickCounter util_log_uptime_stopwatch;
LightLock util_log_mutex = 1;//Initially unlocked state.

Result_with_string Util_log_init(void)
{
	Result_with_string result;
	if(util_log_init)
		goto already_inited;

	osTickCounterStart(&util_log_uptime_stopwatch);
	util_log_current_index = 0;
	util_log_y = 0;
	util_log_x = 0.0;
	util_log_uptime_ms = 0;
	for(int i = 0; i < DEF_LOG_BUFFER_LINES; i++)
	{
		util_log_spend_time[i] = 0;
		util_log_logs[i] = "";
	}

	LightLock_Init(&util_log_mutex);

	util_log_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;
}

void Util_log_exit(void)
{
	if(!util_log_init)
		return;

	util_log_init = false;
}

Result_with_string Util_log_dump(std::string file_name, std::string dir_path)
{
	Result_with_string result;
	std::string log = "";
	if(!util_log_init)
		goto not_inited;

	for(int i = 0; i < DEF_LOG_BUFFER_LINES; i++)
	{
		if(util_log_logs[i] == "")
			continue;

		log += util_log_logs[i] + "\n";
	}

	return Util_file_save_to_file(file_name, dir_path, (u8*)log.c_str(), log.length(), true);

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;
}

bool Util_log_query_log_show_flag(void)
{
	if(!util_log_init)
		return false;
	else
		return util_log_show_flag;
}

void Util_log_set_log_show_flag(bool flag)
{
	if(!util_log_init)
		return;

	util_log_show_flag = flag;
	var_need_reflesh = true;
}

int Util_log_save(std::string place, std::string text)
{
	return Util_log_save(place, text, 1234567890);
}

int Util_log_save(std::string place, std::string text, int result)
{
	int return_log_num = 0;
	char* app_log_cache = NULL;
	if(!util_log_init)
		return -1;

	app_log_cache = (char*)malloc(place.length() + text.length() + 32);
	if(!app_log_cache)
		return -1;

	memset(app_log_cache, 0x0, place.length() + text.length() + 32);

	LightLock_Lock(&util_log_mutex);
	osTickCounterUpdate(&util_log_uptime_stopwatch);
	util_log_uptime_ms += osTickCounterRead(&util_log_uptime_stopwatch);

	if (result == 1234567890)
		sprintf(app_log_cache, "[%.5f][%s] %s", util_log_uptime_ms / 1000, place.c_str(), text.c_str());
	else
		sprintf(app_log_cache, "[%.5f][%s] %s 0x%x", util_log_uptime_ms / 1000, place.c_str(), text.c_str(), result);

	util_log_spend_time[util_log_current_index] = util_log_uptime_ms;
	util_log_logs[util_log_current_index] = app_log_cache;
	util_log_current_index++;
	return_log_num = util_log_current_index;
	if (util_log_current_index >= DEF_LOG_BUFFER_LINES)
		util_log_current_index = 0;

	if (util_log_current_index < DEF_LOG_DISPLAYED_LINES)
		util_log_y = 0;
	else
		util_log_y = util_log_current_index - DEF_LOG_DISPLAYED_LINES;

	LightLock_Unlock(&util_log_mutex);
	free(app_log_cache);
	app_log_cache = NULL;

	if(util_log_show_flag)
		var_need_reflesh = true;

	return (return_log_num - 1);
}

void Util_log_add(int log_index, std::string text)
{
	Util_log_add(log_index, text, 1234567890);
}

void Util_log_add(int log_index, std::string text, int result)
{
	char* app_log_add_cache = NULL;
	if(!util_log_init)
		return;

	if(log_index < 0 || log_index >= DEF_LOG_BUFFER_LINES)
		return;

	app_log_add_cache = (char*)malloc(text.length() + 32);
	memset(app_log_add_cache, 0x0, text.length() + 32);

	LightLock_Lock(&util_log_mutex);
	osTickCounterUpdate(&util_log_uptime_stopwatch);
	util_log_uptime_ms += osTickCounterRead(&util_log_uptime_stopwatch);
	LightLock_Unlock(&util_log_mutex);

	if (result != 1234567890)
		sprintf(app_log_add_cache, "%s0x%x (%.2fms)", text.c_str(), result, (util_log_uptime_ms - util_log_spend_time[log_index]));
	else
		sprintf(app_log_add_cache, "%s (%.2fms)", text.c_str(), (util_log_uptime_ms - util_log_spend_time[log_index]));

	util_log_logs[log_index] += app_log_add_cache;
	free(app_log_add_cache);
	app_log_add_cache = NULL;
	if(util_log_show_flag)
		var_need_reflesh = true;
}

void Util_log_main(Hid_info key)
{
	if(!util_log_init)
	{
		if (key.p_a)
		{
			util_log_show_flag = false;
			var_need_reflesh = true;
		}
		return;
	}

	if (key.h_c_up)
	{
		if (util_log_y - 1 >= 0)
		{
			var_need_reflesh = true;
			util_log_y--;
		}
	}
	if (key.h_c_down)
	{
		if (util_log_y + 1 <= DEF_LOG_BUFFER_LINES - DEF_LOG_DISPLAYED_LINES)
		{
			var_need_reflesh = true;
			util_log_y++;
		}
	}
	if (key.h_c_left)
	{
		if (util_log_x + 5.0 < 0.0)
			util_log_x += 5.0;
		else
			util_log_x = 0.0;

		var_need_reflesh = true;
	}
	if (key.h_c_right)
	{
		if (util_log_x - 5.0 > -1000.0)
			util_log_x -= 5.0;
		else
			util_log_x = -1000.0;

		var_need_reflesh = true;
	}
}

void Util_log_draw(void)
{
	if(!util_log_init)
	{
		Draw("Log api is not initialized.\nPress A to close.", 0, 10, 0.5, 0.5, DEF_DRAW_RED);
		return;
	}

	for (int i = 0; i < DEF_LOG_DISPLAYED_LINES; i++)
		Draw(util_log_logs[util_log_y + i], util_log_x, 10.0 + (i * 10), 0.425, 0.425, DEF_LOG_COLOR);
}
