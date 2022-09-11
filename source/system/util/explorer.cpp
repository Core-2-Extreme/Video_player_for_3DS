#include "system/headers.hpp"

#if DEF_ENABLE_EXPL_API

void (*util_expl_callback)(std::string, std::string) = NULL;
void (*util_expl_cancel_callback)(void) = NULL;
bool util_expl_thread_run = false;
bool util_expl_read_dir_request = false;
bool util_expl_show_flag = false;
bool util_expl_scroll_mode = false;
bool util_expl_init = false;
int util_expl_num_of_file = 0;
int util_expl_size[DEF_EXPL_MAX_FILES];
int util_expl_type[DEF_EXPL_MAX_FILES];
double util_expl_y_offset = 0.0;
double util_expl_selected_file_num = 0.0;
std::string util_expl_current_dir = "/";
std::string util_expl_files[DEF_EXPL_MAX_FILES];
Thread util_expl_read_dir_thread;
Image_data util_expl_file_button[16];

void Util_expl_read_dir_thread(void* arg);

Result_with_string Util_expl_init(void)
{
	Result_with_string result;
	if(util_expl_init)
		goto already_inited;

	util_expl_callback = NULL;
	util_expl_cancel_callback = NULL;
	util_expl_read_dir_request = false;
	util_expl_show_flag = false;
	util_expl_scroll_mode = false;
	util_expl_num_of_file = 0;
	util_expl_y_offset = 0.0;
	util_expl_selected_file_num = 0.0;
	util_expl_current_dir = "/";

	for(int i = 0; i < DEF_EXPL_MAX_FILES; i++)
	{
		util_expl_size[i] = 0;
		util_expl_type[i] = 0;
		util_expl_files[i] = "";
	}

	for(int i = 0; i < 16; i++)
		util_expl_file_button[i].c2d = var_square_image[0];

	util_expl_thread_run = true;
	util_expl_read_dir_thread = threadCreate(Util_expl_read_dir_thread, (void*)(""), DEF_STACKSIZE * 2, DEF_THREAD_PRIORITY_HIGH, 0, false);
	if(!util_expl_read_dir_thread)
	{
		result.code = DEF_ERR_OTHER;
		result.error_description = "[Error] threadCreate failed. ";
		goto nintendo_api_failed;
	}

	util_expl_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Util_expl_exit(void)
{
	if(!util_expl_init)
		return;

	util_expl_init = false;
	util_expl_thread_run = false;
	threadJoin(util_expl_read_dir_thread, DEF_THREAD_WAIT_TIME);
	threadFree(util_expl_read_dir_thread);
}

std::string Util_expl_generate_file_type_string(int type)
{
	std::string type_string = "";
	if(type == DEF_EXPL_TYPE_UNKNOWN)
		type_string += "unknown,";
	if(type & DEF_EXPL_TYPE_FILE)
		type_string += "file,";
	if(type & DEF_EXPL_TYPE_DIR)
		type_string += "dir,";
	if(type & DEF_EXPL_TYPE_READ_ONLY)
		type_string += "read only,";
	if(type & DEF_EXPL_TYPE_HIDDEN)
		type_string += "hidden,";
	
	if(type_string.length() > 0)
		return type_string.substr(0, type_string.length() - 1);
	else
		return type_string;
}

std::string Util_expl_query_current_dir(void)
{
	if(!util_expl_init)
		return "";
	
	return util_expl_current_dir;
}

int Util_expl_query_num_of_file(void)
{
	if(!util_expl_init)
		return 0;
	
	return util_expl_num_of_file;
}

int Util_expl_query_current_file_index(void)
{
	if(!util_expl_init)
		return -1;
	
	return (int)util_expl_selected_file_num + (int)util_expl_y_offset;
}

std::string Util_expl_query_file_name(int index)
{
	if(!util_expl_init)
		return "";
	else if (index >= 0 && index < DEF_EXPL_MAX_FILES)
		return util_expl_files[index];
	else
		return "";
}

int Util_expl_query_size(int index)
{
	if(!util_expl_init)
		return 0;
	else if (index >= 0 && index < DEF_EXPL_MAX_FILES)
		return util_expl_size[index];
	else
		return 0;
}

int Util_expl_query_type(int index)
{
	if(!util_expl_init)
		return DEF_EXPL_TYPE_UNKNOWN;
	if (index >= 0 && index < DEF_EXPL_MAX_FILES)
		return util_expl_type[index];
	else
		return DEF_EXPL_TYPE_UNKNOWN;
}

bool Util_expl_query_show_flag(void)
{
	if(!util_expl_init)
		return false;
	
	return util_expl_show_flag;
}

void Util_expl_set_callback(void (*callback)(std::string, std::string))
{
	if(!util_expl_init)
		return;
	
	util_expl_callback = callback;
}

void Util_expl_set_cancel_callback(void (*callback)(void))
{
	if(!util_expl_init)
		return;

	util_expl_cancel_callback = callback;
}

void Util_expl_set_current_dir(std::string dir)
{
	if(!util_expl_init)
		return;

	util_expl_current_dir = dir;
	util_expl_read_dir_request = true;
}

void Util_expl_set_show_flag(bool flag)
{
	if(!util_expl_init)
		return;

	util_expl_show_flag = flag;
	if(flag)
		util_expl_read_dir_request = true;
}

void Util_expl_draw(void)
{
	int color = DEF_DRAW_BLACK;
	if(!util_expl_init)
	{
		Draw_texture(var_square_image[0], DEF_DRAW_AQUA, 10.0, 20.0, 300.0, 190.0);
		Draw("Explorer api is not initialized.\nPress A to close.", 12.5, 30.0, 0.45, 0.45, DEF_DRAW_RED);
		return;
	}

	Draw_texture(var_square_image[0], DEF_DRAW_AQUA, 10.0, 20.0, 300.0, 190.0);
	Draw("A : OK, B : Back, Y : Close, ↑↓→← : Move", 12.5, 185.0, 0.425, 0.425, DEF_DRAW_BLACK);
	Draw(util_expl_current_dir, 12.5, 195.0, 0.45, 0.45, DEF_DRAW_BLACK);
	for (int i = 0; i < 16; i++)
	{
		Draw_texture(&util_expl_file_button[i], util_expl_file_button[i].selected ? DEF_DRAW_GREEN : DEF_DRAW_AQUA, 10, 20 + (i * 10), 290, 10);
		if(util_expl_type[i + (int)util_expl_y_offset] & DEF_EXPL_TYPE_DIR)
		{
			Draw(util_expl_files[i + (int)util_expl_y_offset] + "(" + Util_expl_generate_file_type_string(util_expl_type[i + (int)util_expl_y_offset])
			 + ")" , 12.5, 19 + (i * 10), 0.425, 0.425, i == (int)util_expl_selected_file_num ? DEF_DRAW_RED : color);
		}
		else
		{
			Draw(util_expl_files[i + (int)util_expl_y_offset] + "(" + std::to_string(util_expl_size[i + (int)util_expl_y_offset] / 1024.0 / 1024.0).substr(0, 4)
			+ "MB) (" + Util_expl_generate_file_type_string(util_expl_type[i + (int)util_expl_y_offset]) + ")"
			, 12.5, 19 + (i * 10), 0.425, 0.425, i == (int)util_expl_selected_file_num ? DEF_DRAW_RED : color);
		}
	}
}

void Util_expl_main(Hid_info key)
{
	size_t cut_pos;

	if(!util_expl_init)
	{
		if (key.p_a)
		{
			util_expl_show_flag = false;
			var_need_reflesh = true;
		}
		return;
	}

	if (key.p_y)
	{
		if(util_expl_cancel_callback)
			util_expl_cancel_callback();

		util_expl_show_flag = false;
		var_need_reflesh = true;
	}
	else if (!util_expl_read_dir_request)
	{
		if(util_expl_scroll_mode)
		{
			if (key.touch_y_move > 0)
			{
				if ((util_expl_y_offset + (key.touch_y_move * var_scroll_speed / 8)) < util_expl_num_of_file - 15)
					util_expl_y_offset += (key.touch_y_move * var_scroll_speed / 8);

				var_need_reflesh = true;
			}
			else if (key.touch_y_move < 0)
			{
				if ((util_expl_y_offset + (key.touch_y_move * var_scroll_speed / 8)) > -1.0)
					util_expl_y_offset += (key.touch_y_move * var_scroll_speed / 8);

				var_need_reflesh = true;
			}
		}
		else
		{
			for (int i = 0; i < 16; i++)
			{
				if(Util_hid_is_pressed(key, util_expl_file_button[i]) && util_expl_num_of_file > (i + (int)util_expl_y_offset))
				{
					util_expl_file_button[i].selected = true;
					var_need_reflesh = true;
				}
				else if (key.p_a || (Util_hid_is_released(key, util_expl_file_button[i]) && util_expl_file_button[i].selected))
				{
					if (key.p_a || i == (int)util_expl_selected_file_num)
					{
						if (((int)util_expl_y_offset + (int)util_expl_selected_file_num) == 0 && !(Util_expl_query_current_dir() == "/"))
						{
							util_expl_current_dir = util_expl_current_dir.substr(0, util_expl_current_dir.length() - 1);
							cut_pos = util_expl_current_dir.find_last_of("/");
							if (!(cut_pos == std::string::npos))
								util_expl_current_dir = util_expl_current_dir.substr(0, cut_pos + 1);

							util_expl_y_offset = 0.0;
							util_expl_selected_file_num = 0.0;
							util_expl_read_dir_request = true;
						}
						else if (util_expl_type[(int)util_expl_y_offset + (int)util_expl_selected_file_num] & DEF_EXPL_TYPE_DIR)
						{
							util_expl_current_dir = util_expl_current_dir + util_expl_files[(int)util_expl_selected_file_num + (int)util_expl_y_offset] + "/";
							util_expl_y_offset = 0.0;
							util_expl_selected_file_num = 0.0;
							util_expl_read_dir_request = true;
						}
						else
						{
							if(util_expl_callback)
								util_expl_callback(Util_expl_query_file_name((int)util_expl_selected_file_num + (int)util_expl_y_offset), util_expl_current_dir);
							
							util_expl_show_flag = false;
							var_need_reflesh = true;
						}

						break;
					}
					else
					{
						if (util_expl_num_of_file > (i + (int)util_expl_y_offset))
							util_expl_selected_file_num = i;
						
						var_need_reflesh = true;
					}
				}
				else if(!Util_hid_is_held(key, util_expl_file_button[i]) &&  util_expl_file_button[i].selected)
				{
					util_expl_file_button[i].selected = false;
					util_expl_scroll_mode = true;
					var_need_reflesh = true;
				}
			}
		}
		if (key.p_b)
		{
			if (util_expl_current_dir != "/")
			{
				util_expl_current_dir = util_expl_current_dir.substr(0, util_expl_current_dir.length() - 1);
				cut_pos = util_expl_current_dir.find_last_of("/");
				if (!(cut_pos == std::string::npos))
					util_expl_current_dir = util_expl_current_dir.substr(0, cut_pos + 1);

				util_expl_y_offset = 0.0;
				util_expl_selected_file_num = 0.0;
				util_expl_read_dir_request = true;
				var_need_reflesh = true;
			}
		}
		else if (key.p_d_down || key.h_d_down || key.p_c_down || key.h_c_down || key.p_d_right || key.h_d_right || key.p_c_right || key.h_c_right)
		{
			if ((util_expl_selected_file_num + 1.0) < 16.0 && (util_expl_selected_file_num + 1.0) < util_expl_num_of_file)
			{
				if (key.p_d_down || key.h_d_down || key.p_c_down || key.h_c_down)
					util_expl_selected_file_num += 0.125;
				else if (key.p_d_right || key.h_d_right || key.p_c_right || key.h_c_right)
					util_expl_selected_file_num += 1.0;
			}
			else if ((util_expl_y_offset + util_expl_selected_file_num + 1.0) < util_expl_num_of_file)
			{
				if (key.p_d_down || key.h_d_down || key.p_c_down || key.h_c_down)
					util_expl_y_offset += 0.125;
				else if (key.p_d_right || key.h_d_right || key.p_c_right || key.h_c_right)
					util_expl_y_offset += 1.0;
			}
			var_need_reflesh = true;
		}
		else if (key.p_d_up || key.h_d_up || key.p_c_up || key.h_c_up || key.p_d_left || key.h_d_left || key.p_c_left || key.h_c_left)
		{
			if ((util_expl_selected_file_num - 1.0) > -1.0)
			{
				if (key.p_d_up || key.h_d_up || key.p_c_up || key.h_c_up)
					util_expl_selected_file_num -= 0.125;
				else if (key.p_d_left || key.h_d_left || key.p_c_left || key.h_c_left)
					util_expl_selected_file_num -= 1.0;
			}
			else if ((util_expl_y_offset - 1.0) > -1.0)
			{
				if (key.p_d_up || key.h_d_up || key.p_c_up || key.h_c_up)
					util_expl_y_offset -= 0.125;
				else if (key.p_d_left || key.h_d_left || key.p_c_left || key.h_c_left)
					util_expl_y_offset -= 1.0;
			}
			var_need_reflesh = true;
		}

		if(!key.p_touch && !key.h_touch)
		{
			for(int i = 0; i < 16; i++)
			{
				if(util_expl_file_button[i].selected)
					var_need_reflesh = true;
				
				util_expl_file_button[i].selected = false;
			}
			util_expl_scroll_mode = false;
		}

		if (util_expl_selected_file_num <= -1)
			util_expl_selected_file_num = 0;
		else if (util_expl_selected_file_num >= 16)
			util_expl_selected_file_num = 15;
		else if (util_expl_selected_file_num >= util_expl_num_of_file)
			util_expl_selected_file_num = util_expl_num_of_file - 1;
		if (util_expl_y_offset <= -1)
			util_expl_y_offset = 0;
		else if (util_expl_y_offset + util_expl_selected_file_num >= util_expl_num_of_file)
			util_expl_y_offset = util_expl_num_of_file - 16;
	}
}

void Util_expl_read_dir_thread(void* arg)
{
	Util_log_save(DEF_EXPL_READ_DIR_THREAD_STR, "Thread started.");
	int log_num;
	int num_of_dir = 0;
	int num_of_file = 0;
	int num_of_unknown = 0;
	int num_offset = 0;
	int index = 0;
	int dir_type[DEF_EXPL_MAX_FILES];
	int file_type[DEF_EXPL_MAX_FILES];
	u64 file_size;
	std::string name_of_dir[DEF_EXPL_MAX_FILES];
	std::string name_of_file[DEF_EXPL_MAX_FILES];
	std::string name_of_unknown[DEF_EXPL_MAX_FILES];
	std::string name_cache[DEF_EXPL_MAX_FILES];
	Result_with_string result;

	for (int i = 0; i < DEF_EXPL_MAX_FILES; i++)
	{
		util_expl_files[i] = "";
		util_expl_type[i] = 0;
		util_expl_size[i] = 0;
	}

	while (util_expl_thread_run)
	{
		if (util_expl_read_dir_request)
		{
			var_need_reflesh = true;
			for (int i = 0; i < DEF_EXPL_MAX_FILES; i++)
			{
				util_expl_files[i] = "";
				util_expl_type[i] = DEF_EXPL_TYPE_UNKNOWN;
				util_expl_size[i] = 0;
			}
			index = 0;

			log_num = Util_log_save(DEF_EXPL_READ_DIR_THREAD_STR, "Util_file_read_dir()...");
			result = Util_file_read_dir(util_expl_current_dir, &util_expl_num_of_file, util_expl_files, util_expl_type, DEF_EXPL_MAX_FILES);
			Util_log_add(log_num, result.string + result.error_description, result.code);

			if (result.code == 0)
			{
				num_of_dir = 0;
				num_of_file = 0;
				num_of_unknown = 0;
				for (int i = 0; i < DEF_EXPL_MAX_FILES; i++)
				{
					name_of_dir[i] = "";
					name_of_file[i] = "";
					name_of_unknown[i] = "";
					name_cache[i] = "";
					dir_type[DEF_EXPL_MAX_FILES] = DEF_EXPL_TYPE_UNKNOWN;
					file_type[DEF_EXPL_MAX_FILES] = DEF_EXPL_TYPE_UNKNOWN;
				}

				for (int i = 0; i < util_expl_num_of_file; i++)
				{
					if (util_expl_type[i] & DEF_FILE_TYPE_DIR)
					{
						name_of_dir[num_of_dir] = util_expl_files[i];
						dir_type[num_of_dir] = DEF_EXPL_TYPE_DIR;
						if(util_expl_type[i] & DEF_FILE_TYPE_HIDDEN)
							dir_type[num_of_dir] |= DEF_EXPL_TYPE_HIDDEN;
						if(util_expl_type[i] & DEF_FILE_TYPE_READ_ONLY)
							dir_type[num_of_dir] |= DEF_EXPL_TYPE_READ_ONLY;

						num_of_dir++;
					}
					else if (util_expl_type[i] & DEF_FILE_TYPE_FILE)
					{
						name_of_file[num_of_file] = util_expl_files[i];
						file_type[num_of_file] = DEF_EXPL_TYPE_FILE;
						if(util_expl_type[i] & DEF_FILE_TYPE_HIDDEN)
							file_type[num_of_file] |= DEF_EXPL_TYPE_HIDDEN;
						if(util_expl_type[i] & DEF_FILE_TYPE_READ_ONLY)
							file_type[num_of_file] |= DEF_EXPL_TYPE_READ_ONLY;

						num_of_file++;
					}
					else
					{
						name_of_unknown[num_of_unknown] = util_expl_files[i];
						num_of_unknown++;
					}
				}

				for (int i = 0; i < DEF_EXPL_MAX_FILES; i++)
				{
					util_expl_files[i] = "";
					util_expl_type[i] = DEF_EXPL_TYPE_UNKNOWN;
				}

				if (!(util_expl_current_dir == "/"))
				{
					num_offset = 1;
					util_expl_num_of_file += 1;
					util_expl_type[0] = DEF_EXPL_TYPE_DIR;
					util_expl_files[0] = ".. (Move to parent directory)";
				}
				else
					num_offset = 0;

				//Directories
				for(int i = 0; i < num_of_dir; i++)
					name_cache[i] = name_of_dir[i];

				std::sort(begin(name_of_dir), begin(name_of_dir) + num_of_dir);
				for (int i = 0; i < num_of_dir; i++)
				{
					index = i + num_offset;
					util_expl_files[index] = name_of_dir[i];
					for(int k = 0; k < num_of_dir; k++)
					{
						if(name_of_dir[i] == name_cache[k])
						{
							util_expl_type[index] = dir_type[k];
							break;
						}
					}
				}

				//Files
				for(int i = 0; i < num_of_file; i++)
					name_cache[i] = name_of_file[i];

				std::sort(begin(name_of_file), begin(name_of_file) + num_of_file);
				for (int i = 0; i < num_of_file; i++)
				{
					index = i + num_of_dir + num_offset;
					util_expl_files[index] = name_of_file[i];
					for(int k = 0; k < num_of_file; k++)
					{
						if(name_of_file[i] == name_cache[k])
						{
							util_expl_type[index] = file_type[k];
							break;
						}
					}
				}

				//Unknowns
				for(int i = 0; i < num_of_unknown; i++)
					name_cache[i] = name_of_unknown[i];

				std::sort(begin(name_of_unknown), begin(name_of_unknown) + num_of_unknown);
				for (int i = 0; i < num_of_unknown; i++)
				{
					index = i + num_of_dir + num_of_file + num_offset;
					util_expl_files[index] = name_of_unknown[i];
					util_expl_type[index] = DEF_EXPL_TYPE_UNKNOWN;
				}
			}
			else
			{
				util_expl_type[0] = DEF_EXPL_TYPE_DIR;
				util_expl_files[0] = ".. (Move to parent directory)";
				util_expl_num_of_file = 1;
			}
			
			var_need_reflesh = true;
			util_expl_read_dir_request = false;

			for (int i = 0; i <= index; i++)
			{
				if (util_expl_read_dir_request)
					break;

				if(util_expl_type[i] & DEF_EXPL_TYPE_FILE || util_expl_type[i] & DEF_EXPL_TYPE_UNKNOWN)
				{
					result = Util_file_check_file_size(util_expl_files[i], util_expl_current_dir, &file_size);
					if (result.code == 0)
					{
						util_expl_size[i] = (int)file_size;
						var_need_reflesh = true;
					}
					else
						Util_log_save(DEF_EXPL_READ_DIR_THREAD_STR, "Util_file_check_file_size()..." + result.string + result.error_description, result.code);
				}
			}
		}
		else
			usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);
	}
	Util_log_save(DEF_EXPL_READ_DIR_THREAD_STR, "Thread exit.");
	threadExit(0);
}

#endif
