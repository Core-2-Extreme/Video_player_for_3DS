#include "headers.hpp"

bool log_show_logs = false;
int log_current_log_num = 0;
int log_y = 0;
double log_x = 0.0;
double log_up_time_ms = 0.0;
double log_spend_time[DEF_LOG_BUFFER_LINES];
std::string log_logs[DEF_LOG_BUFFER_LINES];
TickCounter log_up_time_stopwatch;

void Util_log_init(void)
{
	osTickCounterStart(&log_up_time_stopwatch);
	log_up_time_ms = 0;
	for(int i = 0; i < DEF_LOG_BUFFER_LINES; i++)
	{
		log_spend_time[i] = 0;
		log_logs[i] = "";
	}
}

bool Util_log_query_log_show_flag(void)
{
	return log_show_logs;
}

void Util_log_set_log_show_flag(bool flag)
{
	log_show_logs = flag;
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

	osTickCounterUpdate(&log_up_time_stopwatch);
	log_up_time_ms += osTickCounterRead(&log_up_time_stopwatch);
	log_spend_time[log_current_log_num] = log_up_time_ms;

	if (result == 1234567890)
		sprintf(app_log_cache, "[%.5f][%s] %s", log_up_time_ms / 1000, type.c_str(), text.c_str());
	else
		sprintf(app_log_cache, "[%.5f][%s] %s 0x%x", log_up_time_ms / 1000, type.c_str(), text.c_str(), result);

	log_logs[log_current_log_num] = app_log_cache;
	log_current_log_num++;
	return_log_num = log_current_log_num;
	if (log_current_log_num >= DEF_LOG_BUFFER_LINES)
		log_current_log_num = 0;

	if (log_current_log_num < DEF_LOG_DISPLAYED_LINES)
		log_y = 0;
	else
		log_y = log_current_log_num - DEF_LOG_DISPLAYED_LINES;
	
	if(log_show_logs)
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

	osTickCounterUpdate(&log_up_time_stopwatch);
	log_up_time_ms += osTickCounterRead(&log_up_time_stopwatch);

	if (result != 1234567890)
		sprintf(app_log_add_cache, "%s0x%x (%.2fms)", add_text.c_str(), result, (log_up_time_ms - log_spend_time[add_log_num]));
	else
		sprintf(app_log_add_cache, "%s (%.2fms)", add_text.c_str(), (log_up_time_ms - log_spend_time[add_log_num]));

	log_logs[add_log_num] += app_log_add_cache;
	if(log_show_logs)
		var_need_reflesh = true;
}

void Util_log_main(Hid_info key)
{
	if (key.h_c_up)
	{
		if (log_y - 1 > 0)
		{
			var_need_reflesh = true;
			log_y--;
		}
	}
	if (key.h_c_down)
	{
		if (log_y + 1 <= DEF_LOG_BUFFER_LINES - DEF_LOG_DISPLAYED_LINES)
		{
			var_need_reflesh = true;
			log_y++;
		}
	}
	if (key.h_c_left)
	{
		if (log_x + 5.0 < 0.0)
			log_x += 5.0;
		else
			log_x = 0.0;

		var_need_reflesh = true;
	}
	if (key.h_c_right)
	{
		if (log_x - 5.0 > -1000.0)
			log_x -= 5.0;
		else
			log_x = -1000.0;

		var_need_reflesh = true;
	}
}

void Util_log_draw(void)
{
	for (int i = 0; i < DEF_LOG_DISPLAYED_LINES; i++)
		Draw(log_logs[log_y + i], log_x, 10.0 + (i * 10), 0.4, 0.4, DEF_LOG_COLOR);
}
