#pragma once

void Util_hid_init(void);

void Util_hid_exit(void);

bool Util_hid_is_pressed(Hid_info hid_state, Image_data image);

bool Util_hid_is_held(Hid_info hid_state, Image_data image);

bool Util_hid_is_released(Hid_info hid_state, Image_data image);

void Util_hid_query_key_state(Hid_info* out_key_state);

void Util_hid_key_flag_reset(void);

void Util_hid_scan_hid_thread(void* arg);
