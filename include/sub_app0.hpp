#pragma once
#include "system/types.hpp"

bool Sapp0_query_init_flag(void);

bool Sapp0_query_running_flag(void);

void Sapp0_resume(void);

void Sapp0_suspend(void);

Result_with_string Sapp0_load_msg(std::string lang);

void Sapp0_init(void);

void Sapp0_exit(void);

void Sapp0_main(void);
