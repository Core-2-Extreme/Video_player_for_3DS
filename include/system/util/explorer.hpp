#pragma once

std::string Util_expl_query_current_patch(void);

std::string Util_expl_query_file_name(int file_num);

int Util_expl_query_num_of_file(void);

int Util_expl_query_size(int file_num);

std::string Util_expl_query_type(int file_num);

bool Util_expl_query_show_flag(void);

void Util_expl_set_callback(void (*callback)(std::string file, std::string dir));

void Util_expl_set_cancel_callback(void (*callback)(void));

void Util_expl_set_current_patch(std::string patch);

void Util_expl_set_show_flag(bool flag);

void Util_expl_init(void);

void Util_expl_exit(void);

void Util_expl_draw(void);

void Util_expl_main(Hid_info key);
