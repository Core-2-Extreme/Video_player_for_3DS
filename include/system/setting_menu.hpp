#ifndef SETTING_MENU_HPP
#define SETTING_MENU_HPP

#include "types.hpp"

bool Sem_query_init_flag(void);

bool Sem_query_running_flag(void);

void Sem_resume(void);

void Sem_suspend(void);

Result_with_string Sem_load_msg(std::string lang);

void Sem_init(void);

void Sem_draw_init(void);

void Sem_exit(void);

void Sem_main(void);

void Sem_hid(Hid_info key);

#endif
