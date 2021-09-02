#pragma once
#include "system/types.hpp"

bool Sapp6_query_init_flag(void);

bool Sapp6_query_running_flag(void);

void Sapp6_resume(void);

void Sapp6_suspend(void);

Result_with_string Sapp6_load_msg(void);

void Sapp6_init(void);

void Sapp6_exit(void);

void Sapp6_main(void);
