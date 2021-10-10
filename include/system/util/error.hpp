#pragma once

void Util_err_init(void);

void Util_err_exit(void);

bool Util_err_query_error_show_flag(void);

void Util_err_set_error_message(std::string summary, std::string description, std::string place);

void Util_err_set_error_message(std::string summary, std::string description, std::string place, int error_code);

void Util_err_set_error_show_flag(bool flag);

void Util_err_clear_error_message(void);

void Util_err_save_error(void);

void Util_err_main(Hid_info key);

void Util_err_draw(void);
