#pragma once
#include "system/types.hpp"

bool Sapp2_query_init_flag(void);

bool Sapp2_query_running_flag(void);

void Sapp2_hid(Hid_info key);

void Sapp2_resume(void);

void Sapp2_suspend(void);

Result_with_string Sapp2_load_msg(std::string lang);

void Sapp2_init(void);

void Sapp2_exit(void);

void Sapp2_main(void);
