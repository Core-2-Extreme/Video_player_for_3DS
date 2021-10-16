#pragma once

void Util_log_init(void);

void Util_log_exit(void);

bool Util_log_query_log_show_flag(void);

void Util_log_set_log_show_flag(bool flag);

int Util_log_save(std::string type, std::string text);

int Util_log_save(std::string type, std::string text, int result);

void Util_log_add(int add_log_num, std::string add_text);

void Util_log_add(int add_log_num, std::string add_text, int result);

void Util_log_main(Hid_info key);

void Util_log_draw(void);
