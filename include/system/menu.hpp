#pragma once

bool Menu_query_must_exit_flag(void);

void Menu_set_must_exit_flag(bool flag);

void Menu_resume(void);

void Menu_suspend(void);

Result_with_string Menu_load_msg(std::string lang);

void Menu_init(void);

void Menu_exit(void);

void Menu_main(void);

