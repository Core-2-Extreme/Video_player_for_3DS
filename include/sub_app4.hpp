#pragma once
#include "system/types.hpp"

bool Sapp4_query_init_flag(void);

bool Sapp4_query_running_flag(void);

void Sapp4_resume(void);

void Sapp4_suspend(void);

Result_with_string Sapp4_load_msg(std::string lang);

void Sapp4_init(void);

void Sapp4_exit(void);

void Sapp4_main(void);
