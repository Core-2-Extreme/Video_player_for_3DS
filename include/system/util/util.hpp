#pragma once

Result_with_string Util_parse_file(std::string source_data, int num_of_items, std::string out_data[]);

std::string Util_convert_seconds_to_time(double input_seconds);

std::string Util_encode_to_escape(std::string in_data);

Result_with_string Util_load_msg(std::string file_name, std::string out_msg[], int num_of_msg);

void Util_init(void);

void Util_exit(void);

void* Util_safe_linear_alloc(int size);

void Util_safe_linear_free(void* pointer);
