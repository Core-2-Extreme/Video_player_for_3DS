#include "system/headers.hpp"

bool util_log_show_flag = false;
int util_log_current_index = 0;
int util_log_y = 0;
double util_log_x = 0.0;
double util_log_uptime_ms = 0.0;
double util_log_spend_time[DEF_LOG_BUFFER_LINES];
std::string util_log_logs[DEF_LOG_BUFFER_LINES];
TickCounter util_log_uptime_stopwatch;

void Util_log_init(void)
{
	osTickCounterStart(&util_log_uptime_stopwatch);
	util_log_uptime_ms = 0;
	for(int i = 0; i < DEF_LOG_BUFFER_LINES; i++)
	{
		util_log_spend_time[i] = 0;
		util_log_logs[i] = "";
	}
}

bool Util_log_query_log_show_flag(void)
{
	return util_log_show_flag;
}

void Util_log_set_log_show_flag(bool flag)
{
	util_log_show_flag = flag;
	var_need_reflesh = true;
}

int Util_log_save(std::string type, std::string text)
{
	return Util_log_save(type, text, 1234567890);
}

int Util_log_save(std::string type, std::string text, int result)
{
	int return_log_num = 0;
	char app_log_cache[2048];
	memset(app_log_cache, 0x0, 2048);

	osTickCounterUpdate(&util_log_uptime_stopwatch);
	util_log_uptime_ms += osTickCounterRead(&util_log_uptime_stopwatch);
	util_log_spend_time[util_log_current_index] = util_log_uptime_ms;

	if (result == 1234567890)
		sprintf(app_log_cache, "[%.5f][%s] %s", util_log_uptime_ms / 1000, type.c_str(), text.c_str());
	else
		sprintf(app_log_cache, "[%.5f][%s] %s 0x%x", util_log_uptime_ms / 1000, type.c_str(), text.c_str(), result);

	util_log_logs[util_log_current_index] = app_log_cache;
	util_log_current_index++;
	return_log_num = util_log_current_index;
	if (util_log_current_index >= DEF_LOG_BUFFER_LINES)
		util_log_current_index = 0;

	if (util_log_current_index < DEF_LOG_DISPLAYED_LINES)
		util_log_y = 0;
	else
		util_log_y = util_log_current_index - DEF_LOG_DISPLAYED_LINES;
	
	if(util_log_show_flag)
		var_need_reflesh = true;

	return (return_log_num - 1);
}

void Util_log_add(int add_log_num, std::string add_text)
{
	Util_log_add(add_log_num, add_text, 1234567890);
}

void Util_log_add(int add_log_num, std::string add_text, int result)
{
	char app_log_add_cache[2048];
	memset(app_log_add_cache, 0x0, 2048);

	osTickCounterUpdate(&util_log_uptime_stopwatch);
	util_log_uptime_ms += osTickCounterRead(&util_log_uptime_stopwatch);

	if (result != 1234567890)
		sprintf(app_log_add_cache, "%s0x%x (%.2fms)", add_text.c_str(), result, (util_log_uptime_ms - util_log_spend_time[add_log_num]));
	else
		sprintf(app_log_add_cache, "%s (%.2fms)", add_text.c_str(), (util_log_uptime_ms - util_log_spend_time[add_log_num]));

	util_log_logs[add_log_num] += app_log_add_cache;
	if(util_log_show_flag)
		var_need_reflesh = true;
}

void Util_log_main(Hid_info key)
{
	if (key.h_c_up)
	{
		if (util_log_y - 1 > 0)
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
	for (int i = 0; i < DEF_LOG_DISPLAYED_LINES; i++)
		Draw(util_log_logs[util_log_y + i], util_log_x, 10.0 + (i * 10), 0.4, 0.4, DEF_LOG_COLOR);
}
