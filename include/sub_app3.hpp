#pragma once
#include "system/types.hpp"

bool Sapp3_query_init_flag(void);

bool Sapp3_query_running_flag(void);

void Sapp3_resume(void);

void Sapp3_suspend(void);

Result_with_string Sapp3_load_msg(void);

void Sapp3_init(void);

void Sapp3_exit(void);

void Sapp3_main(void);
