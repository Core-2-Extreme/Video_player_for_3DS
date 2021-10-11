#pragma once
#include "system/types.hpp"

bool Vid_query_init_flag(void);

bool Vid_query_running_flag(void);

void Vid_resume(void);

void Vid_suspend(void);

Result_with_string Vid_load_msg(std::string lang);

void Vid_init(void);

void Vid_exit(void);

void Vid_main(void);
