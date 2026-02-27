//Includes.
#include "system/util/expl.h"

#if DEF_EXPL_API_ENABLE
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "system/menu.h"
#include "system/draw/draw.h"
#include "system/util/err_types.h"
#include "system/util/file.h"
#include "system/util/hid.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/util.h"
#include "system/util/watch.h"

//Defines.
#define SORT_TYPE_UNDEFINED					(uint8_t)(0)	//Unknown.
#define SORT_TYPE_SPECIAL_CHAR				(uint8_t)(1)	//Other than 0-9,a-z,A-Z.
#define SORT_TYPE_NUMBER					(uint8_t)(2)	//0-9.
#define SORT_TYPE_ALPHABET					(uint8_t)(3)	//a-z or A-Z.

#define NUM_OF_DISPLAYED_ITEMS				(uint8_t)(16)	//Number of displayed files on the screen.

#define BOX_X								(double)(10)	//Box X offset for explorer screen in px.
#define BOX_Y								(double)(20)	//Box Y offset for explorer screen in px.
#define BOX_WIDTH							(double)(300)	//Box width for explorer screen in px.
#define BOX_HEIGHT							(double)(190)	//Box height for explorer screen in px.

#define ITEM_SPACE_X						(double)(2.5)	//Element spacing for explorer screen (for X direction) in px.
#define ITEM_SPACE_Y						(double)(0)		//Element spacing for explorer screen (for Y direction) in px.
#define ITEM_WIDTH							(double)(295)	//Element width for explorer screen in px.
#define ITEM_HEIGHT							(double)(160.0 / NUM_OF_DISPLAYED_ITEMS)	//Element height for explorer screen in px.

#define CONTROL_SPACE_Y						(double)(5)		//Control element spacing for explorer screen (for Y direction) in px.

//Close.
#define HID_NOT_INITED_CLOSE_CFM(k)			(bool)(DEF_HID_PR_EM((k).a, 1) || DEF_HID_HD((k).a))
//Cancel (close).
#define HID_CANCEL_CFM(k)					(bool)(DEF_HID_PR_EM((k).y, 1) || DEF_HID_HD((k).y))
//File selection.
#define HID_FILE_SELECTED_TOUCH(k, id)		(bool)((DEF_HID_PHY_PR((k).touch) && DEF_HID_INIT_IN(util_expl_file_button[id], (k))))
#define HID_FILE_CONFIRMED_BUTTON(k)		(bool)(DEF_HID_PR_EM((k).a, 1) || DEF_HID_HD((k).a))
#define HID_FILE_CONFIRMED_TOUCH(k, id)		(bool)((DEF_HID_PR_EM((k).touch, 1) || DEF_HID_HD((k).touch)) && DEF_HID_INIT_LAST_IN(util_expl_file_button[id], (k)))
#define HID_FILE_DESELECTED_TOUCH(k)		(bool)(DEF_HID_PHY_NP((k).touch))
//Back to parent directory.
#define HID_PARENT_DIR_CFM(k)				(bool)(DEF_HID_PR_EM((k).b, 1) || DEF_HID_HD((k).b))
//Scroll mode.
#define HID_SCROLL_MODE_SEL(k)				(bool)(DEF_HID_PHY_HE((k).touch) && ((abs((k).touch_x_initial - (k).touch_x) > 6) || (abs((k).touch_y_initial - (k).touch_y) > 6)))
#define HID_SCROLL_MODE_DESEL(k)			(bool)(DEF_HID_PHY_NP((k).touch))
//Go to next file.
#define HID_NEXT_FILE_PRE_CFM(k)			(bool)(DEF_HID_PHY_PR((k).d_down) || DEF_HID_PHY_PR((k).c_down) || DEF_HID_PHY_PR((k).d_right) \
|| DEF_HID_PHY_PR((k).c_right) || DEF_HID_HE((k).d_down) || DEF_HID_HE((k).c_down) || DEF_HID_PHY_HE((k).d_right) || DEF_HID_PHY_HE((k).c_right))

#define HID_NEXT_FILE_UPDATE_RANGE(k)		DEF_HID_HE_NEW_INTERVAL((k).d_down, 100, is_new_range[0]); DEF_HID_HE_NEW_INTERVAL((k).c_down, 200, is_new_range[1])
#define HID_NEXT_FILE_CFM(k)				(bool)(DEF_HID_PHY_PR((k).d_down) || DEF_HID_PHY_PR((k).c_down) || DEF_HID_PHY_PR((k).d_right) \
|| DEF_HID_PHY_PR((k).c_right) || DEF_HID_PHY_HE((k).d_right) || DEF_HID_PHY_HE((k).c_right) || is_new_range[0] || is_new_range[1])

//Go to previous file.
#define HID_PRE_FILE_PRE_CFM(k)				(bool)(DEF_HID_PHY_PR((k).d_up) || DEF_HID_PHY_PR((k).c_up) || DEF_HID_PHY_PR((k).d_left) \
|| DEF_HID_PHY_PR((k).c_left) || DEF_HID_HE((k).d_up) || DEF_HID_HE((k).c_up) || DEF_HID_PHY_HE((k).d_left) || DEF_HID_PHY_HE((k).c_left))

#define HID_PRE_FILE_UPDATE_RANGE(k)		DEF_HID_HE_NEW_INTERVAL((k).d_up, 100, is_new_range[0]); DEF_HID_HE_NEW_INTERVAL((k).c_up, 200, is_new_range[1])
#define HID_PRE_FILE_CFM(k)					(bool)(DEF_HID_PHY_PR((k).d_up) || DEF_HID_PHY_PR((k).c_up) || DEF_HID_PHY_PR((k).d_left) \
|| DEF_HID_PHY_PR((k).c_left) || DEF_HID_PHY_HE((k).d_left) || DEF_HID_PHY_HE((k).c_left) || is_new_range[0] || is_new_range[1])

#define FONT_SIZE_ERROR						(float)(13.50)	//Font size for API error in px.
#define FONT_SIZE_CONTENT					(float)(ITEM_HEIGHT * 1.25)	//Font size for content in px.
#define FONT_SIZE_CONTROL					(float)(12.75)	//Font size for UI controls in px.
#define FONT_SIZE_CURRENT_DIR				(float)(13.50)	//Font size for current directory in px.

//Typedefs.
typedef struct
{
	uint32_t size[DEF_EXPL_MAX_FILES];
	Str_data name[DEF_EXPL_MAX_FILES];
	Expl_file_type type[DEF_EXPL_MAX_FILES];
} Util_expl_files;

typedef struct
{
	uint32_t size;
	Str_data name;
	Expl_file_type type;
} Util_expl_file_compare;

//Prototypes.
static void Util_expl_generate_file_type_string(Expl_file_type type, Str_data* type_string);
//We can't get rid of this "int" because library uses "int" type as return value.
static int Util_expl_compare_name(const void* a, const void* b);
static void Util_expl_read_dir_callback(void);

//Variables.
static void (*util_expl_callback)(Str_data*, Str_data*) = NULL;
static void (*util_expl_cancel_callback)(void) = NULL;
static bool util_expl_read_dir_request = false;
static bool util_expl_show_flag = false;
static bool util_expl_scroll_mode = false;
static bool util_expl_init = false;
static double util_expl_move_remaining = 0;
static uint8_t util_expl_active_index = 0;
static uint32_t util_expl_num_of_files = 0;
static uint32_t util_expl_check_file_size_index = 0;
static uint32_t util_expl_y_offset = 0;
static Str_data util_expl_current_dir = { 0, };
static Draw_image_data util_expl_file_button[NUM_OF_DISPLAYED_ITEMS] = { 0, };
static Util_expl_files util_expl_files = { 0, };

//Code.
uint32_t Util_expl_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_expl_init)
		goto already_inited;

	util_expl_callback = NULL;
	util_expl_cancel_callback = NULL;
	util_expl_read_dir_request = false;
	util_expl_show_flag = false;
	util_expl_scroll_mode = false;
	util_expl_num_of_files = 0;
	util_expl_check_file_size_index = 0;
	util_expl_y_offset = 0;
	util_expl_active_index = 0;

	result = Util_str_init(&util_expl_current_dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto other;
	}

	result = Util_str_set(&util_expl_current_dir, "/");
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto other;
	}

	for(uint32_t i = 0; i < DEF_EXPL_MAX_FILES; i++)
	{
		util_expl_files.size[i] = 0;
		util_expl_files.type[i] = EXPL_FILE_TYPE_NONE;

		result = Util_str_init(&util_expl_files.name[i]);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto other;
		}
	}

	for(uint8_t i = 0; i < NUM_OF_DISPLAYED_ITEMS; i++)
		util_expl_file_button[i] = Draw_get_empty_image();

	if(!Menu_add_worker_thread_callback(Util_expl_read_dir_callback))
	{
		result = DEF_ERR_OTHER;
		DEF_LOG_RESULT(Menu_add_worker_thread_callback, false, result);
		goto other;
	}

	Util_watch_add(WATCH_HANDLE_GLOBAL, &util_expl_show_flag, sizeof(util_expl_show_flag));
	Util_watch_add(WATCH_HANDLE_EXPL, &util_expl_y_offset, sizeof(util_expl_y_offset));
	Util_watch_add(WATCH_HANDLE_EXPL, &util_expl_active_index, sizeof(util_expl_active_index));
	for (uint8_t i = 0; i < NUM_OF_DISPLAYED_ITEMS; i++)
		Util_watch_add(WATCH_HANDLE_EXPL, &util_expl_file_button[i].selected, sizeof(util_expl_file_button[i].selected));

	util_expl_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	other:
	Util_expl_exit();
	return result;
}

void Util_expl_exit(void)
{
	if(!util_expl_init)
		return;

	util_expl_init = false;
	Menu_remove_worker_thread_callback(Util_expl_read_dir_callback);

	Util_str_free(&util_expl_current_dir);
	for(uint32_t i = 0; i < DEF_EXPL_MAX_FILES; i++)
		Util_str_free(&util_expl_files.name[i]);

	Util_watch_remove(WATCH_HANDLE_GLOBAL, &util_expl_show_flag);
	Util_watch_remove(WATCH_HANDLE_EXPL, &util_expl_y_offset);
	Util_watch_remove(WATCH_HANDLE_EXPL, &util_expl_active_index);
	for (uint8_t i = 0; i < NUM_OF_DISPLAYED_ITEMS; i++)
		Util_watch_remove(WATCH_HANDLE_EXPL, &util_expl_file_button[i].selected);
}

uint32_t Util_expl_query_current_dir(Str_data* dir_name)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_expl_init)
		goto not_inited;

	if(!dir_name)
		goto invalid_arg;

	result = Util_str_init(dir_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto other;
	}

	//Copy directory name.
	result = Util_str_set(dir_name, util_expl_current_dir.buffer);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto other;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	other:
	return result;
}

uint32_t Util_expl_query_num_of_file(void)
{
	if(!util_expl_init)
		return 0;

	return util_expl_num_of_files;
}

uint32_t Util_expl_query_current_file_index(void)
{
	if(!util_expl_init)
		return DEF_EXPL_INVALID_INDEX;

	return (util_expl_active_index + util_expl_y_offset);
}

uint32_t Util_expl_query_file_name(uint32_t index, Str_data* file_name)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!util_expl_init)
		goto not_inited;

	if(index >= DEF_EXPL_MAX_FILES || !file_name)
		goto invalid_arg;

	result = Util_str_init(file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto other;
	}

	//Copy file name.
	result = Util_str_set(file_name, util_expl_files.name[index].buffer);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto other;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	other:
	return result;
}

uint32_t Util_expl_query_size(uint32_t index)
{
	if(!util_expl_init)
		return 0;
	else if (index < DEF_EXPL_MAX_FILES)
		return util_expl_files.size[index];
	else
		return 0;
}

Expl_file_type Util_expl_query_type(uint32_t index)
{
	if(!util_expl_init)
		return EXPL_FILE_TYPE_NONE;
	if (index < DEF_EXPL_MAX_FILES)
		return util_expl_files.type[index];
	else
		return EXPL_FILE_TYPE_NONE;
}

bool Util_expl_query_show_flag(void)
{
	if(!util_expl_init)
		return false;

	return util_expl_show_flag;
}

void Util_expl_set_callback(void (*const callback)(Str_data*, Str_data*))
{
	if(!util_expl_init)
		return;

	util_expl_callback = callback;
}

void Util_expl_set_cancel_callback(void (*const callback)(void))
{
	if(!util_expl_init)
		return;

	util_expl_cancel_callback = callback;
}

void Util_expl_set_current_dir(const Str_data* dir_name)
{
	if(!util_expl_init)
		return;

	if(!Util_str_has_data(dir_name))
		return;

	Util_str_set(&util_expl_current_dir, dir_name->buffer);
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
	Draw_image_data background = Draw_get_empty_image();
	uint32_t color = DEF_DRAW_BLACK;
	double draw_x = 0;
	double draw_y = 0;

	if(!util_expl_init)
	{
		Draw_with_background_c("Explorer API is not initialized.\nPress A to close.", BOX_X, BOX_Y, FONT_SIZE_ERROR, DEF_DRAW_RED,
		DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER, BOX_WIDTH, BOX_HEIGHT, DRAW_BACKGROUND_ENTIRE_BOX, &background, DEF_DRAW_AQUA);
		return;
	}

	Draw_texture(&background, DEF_DRAW_AQUA, BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT);

	draw_x = (BOX_X + ITEM_SPACE_X);
	draw_y = (BOX_Y + ITEM_SPACE_Y);

	for (uint8_t i = 0; i < NUM_OF_DISPLAYED_ITEMS; i++)
	{
		Str_data message = { 0, };
		Str_data type = { 0, };
		uint32_t index = (i + util_expl_y_offset);
		uint32_t text_color = DEF_DRAW_NO_COLOR;
		uint32_t texture_color = DEF_DRAW_NO_COLOR;

		if(Util_str_init(&message) != DEF_SUCCESS
		|| Util_str_init(&type) != DEF_SUCCESS)
		{
			Util_str_free(&message);
			Util_str_free(&type);
			continue;
		}

		Util_expl_generate_file_type_string(util_expl_files.type[index], &type);

		if(util_expl_files.type[index] & EXPL_FILE_TYPE_DIR)
			Util_str_format(&message, "%s (%s)", DEF_STR_NEVER_NULL(&util_expl_files.name[index]), DEF_STR_NEVER_NULL(&type));
		else
		{
			double size = util_expl_files.size[index];

			if(size < 1000)
				Util_str_format(&message, "%s(%" PRIu32 "B) (%s)", DEF_STR_NEVER_NULL(&util_expl_files.name[index]), (uint32_t)size, DEF_STR_NEVER_NULL(&type));
			else
			{
				size /= 1000.0;
				if(size < 1000)
					Util_str_format(&message, "%s(%.1fKB) (%s)", DEF_STR_NEVER_NULL(&util_expl_files.name[index]), size, DEF_STR_NEVER_NULL(&type));
				else
				{
					size /= 1000.0;
					if(size < 1000)
						Util_str_format(&message, "%s(%.1fMB) (%s)", DEF_STR_NEVER_NULL(&util_expl_files.name[index]), size, DEF_STR_NEVER_NULL(&type));
					else
					{
						size /= 1000.0;
						Util_str_format(&message, "%s(%.1fGB) (%s)", DEF_STR_NEVER_NULL(&util_expl_files.name[index]), size, DEF_STR_NEVER_NULL(&type));
					}
				}
			}
		}
		Util_str_free(&type);

		text_color = (i == util_expl_active_index ? DEF_DRAW_RED : color);
		texture_color = (util_expl_file_button[i].selected ? DEF_DRAW_GREEN : DEF_DRAW_AQUA);

		Draw_with_background(&message, draw_x, draw_y, FONT_SIZE_CONTENT, text_color, DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_CENTER,
		ITEM_WIDTH, ITEM_HEIGHT, DRAW_BACKGROUND_ENTIRE_BOX, &util_expl_file_button[i], texture_color);

		draw_y += (ITEM_HEIGHT + ITEM_SPACE_Y);
		Util_str_free(&message);
	}

	draw_y += CONTROL_SPACE_Y;
	Draw_c("A : OK, B : Back, Y : Close, ↑↓→← : Move", draw_x, draw_y, FONT_SIZE_CONTROL, DEF_DRAW_BLACK);

	draw_y += (ITEM_HEIGHT + ITEM_SPACE_Y);
	Draw(&util_expl_current_dir, draw_x, draw_y, FONT_SIZE_CURRENT_DIR, DEF_DRAW_BLACK);
}

void Util_expl_main(const Hid_info* key, double scroll_speed)
{
	if(!key)
		return;

	if(!util_expl_init)
	{
		//Execute functions if conditions are satisfied.
		if (HID_NOT_INITED_CLOSE_CFM(*key))
		{
			util_expl_show_flag = false;
			//Reset key state on scene change.
			Util_hid_reset_key_state(HID_KEY_BIT_ALL);
		}
		return;
	}

	//Notify user that button is being pressed.
	if (!util_expl_read_dir_request)
	{
		if(!util_expl_scroll_mode)
		{
			for (uint8_t i = 0; i < NUM_OF_DISPLAYED_ITEMS; i++)
			{
				if(HID_FILE_SELECTED_TOUCH(*key, i) && util_expl_num_of_files > (i + util_expl_y_offset))
					util_expl_file_button[i].selected = true;
			}
		}
	}
	if(HID_SCROLL_MODE_SEL(*key))
		util_expl_scroll_mode = true;

	//Execute functions if conditions are satisfied.
	if (HID_CANCEL_CFM(*key))
	{
		if(util_expl_cancel_callback)
			util_expl_cancel_callback();

		util_expl_show_flag = false;
		//Reset key state on scene change.
		Util_hid_reset_key_state(HID_KEY_BIT_ALL);
	}
	else if (!util_expl_read_dir_request)
	{
		if(util_expl_scroll_mode)
		{
			if(util_expl_num_of_files > NUM_OF_DISPLAYED_ITEMS && key->touch_y_move != 0)
			{
				uint32_t new_offset = util_expl_y_offset;
				int32_t changes = 0;

				//Store value less than integer (e.g. if input value is 4.6
				//remaining will be 0.6 and changes will be 4).
				util_expl_move_remaining += (key->touch_y_move * scroll_speed / 8);
				changes = (int32_t)util_expl_move_remaining;
				util_expl_move_remaining -= changes;

				if(changes >= 0)
				{
					if((new_offset + changes) > (util_expl_num_of_files - NUM_OF_DISPLAYED_ITEMS))
						new_offset = (util_expl_num_of_files - NUM_OF_DISPLAYED_ITEMS);
					else
						new_offset += changes;
				}
				else
				{
					if(new_offset >= (uint32_t)(-changes))
						new_offset -= (uint32_t)(-changes);
					else
						new_offset = 0;
				}

				util_expl_y_offset = new_offset;
			}
		}
		else
		{
			for (uint8_t i = 0; i < NUM_OF_DISPLAYED_ITEMS; i++)
			{
				if (HID_FILE_CONFIRMED_TOUCH(*key, i) || HID_FILE_CONFIRMED_BUTTON(*key))
				{
					if (HID_FILE_CONFIRMED_BUTTON(*key) || (i == util_expl_active_index))
					{
						//User has released the button on the active item (i.e. file or dir), open it.
						uint32_t selected_index = (util_expl_y_offset + util_expl_active_index);
						Str_data dir = { 0, };

						Util_expl_query_current_dir(&dir);
						if(Util_str_has_data(&dir))
						{
							bool is_root_dir = false;

							if(strcmp(dir.buffer, "/") == 0)
								is_root_dir = true;

							if (selected_index == 0 && !is_root_dir)
							{
								//Back to parent directory.
								char* last_slash_pos = strrchr(dir.buffer, '/');

								if(last_slash_pos)
								{
									//Remove last slash first.
									uint32_t new_length = (last_slash_pos - dir.buffer);

									Util_str_resize(&dir, new_length);

									last_slash_pos = strrchr(dir.buffer, '/');
									if(last_slash_pos)
									{
										//Then remove until next slash.
										new_length = (last_slash_pos - dir.buffer) + 1;
										Util_str_resize(&dir, new_length);
									}
								}
								else
									Util_str_set(&dir, "/");

								Util_str_set(&util_expl_current_dir, dir.buffer);
								util_expl_y_offset = 0;
								util_expl_active_index = 0;
								util_expl_read_dir_request = true;
							}
							else if (util_expl_files.type[selected_index] & EXPL_FILE_TYPE_DIR)
							{
								//Go to selected sub directory.
								Util_str_format_append(&dir, "%s/", DEF_STR_NEVER_NULL(&util_expl_files.name[selected_index]));

								Util_str_set(&util_expl_current_dir, dir.buffer);
								util_expl_y_offset = 0;
								util_expl_active_index = 0;
								util_expl_read_dir_request = true;
							}
							else
							{
								//Notify file selection.
								Str_data file = { 0, };

								Util_expl_query_file_name(selected_index, &file);

								if(Util_str_has_data(&file) && Util_str_has_data(&dir) && util_expl_callback)
									util_expl_callback(&file, &dir);

								Util_str_free(&file);
								util_expl_show_flag = false;
							}
						}

						Util_str_free(&dir);
					}
					else
					{
						//User has released the button on inactive item, mark selected item as active.
						if (util_expl_num_of_files > (i + util_expl_y_offset))
							util_expl_active_index = i;
					}

					//Reset key state on scene change.
					Util_hid_reset_key_state(HID_KEY_BIT_ALL);

					break;
				}
			}
		}
		if (HID_PARENT_DIR_CFM(*key))
		{
			bool is_root_dir = false;
			Str_data dir = { 0, };

			Util_expl_query_current_dir(&dir);
			if(Util_str_has_data(&dir) && strcmp(dir.buffer, "/") == 0)
				is_root_dir = true;

			if (!is_root_dir)
			{
				if(Util_str_has_data(&dir))
				{
					//Back to parent directory.
					char* last_slash_pos = strrchr(dir.buffer, '/');

					if(last_slash_pos)
					{
						//Remove last slash first.
						uint32_t new_length = (last_slash_pos - dir.buffer);

						Util_str_resize(&dir, new_length);

						last_slash_pos = strrchr(dir.buffer, '/');
						if(last_slash_pos)
						{
							//Then remove until next slash.
							new_length = (last_slash_pos - dir.buffer) + 1;
							Util_str_resize(&dir, new_length);
						}
					}
					else
						Util_str_set(&dir, "/");
				}
				else
					Util_str_set(&dir, "/");

				Util_str_set(&util_expl_current_dir, dir.buffer);
				util_expl_y_offset = 0;
				util_expl_active_index = 0;
				util_expl_read_dir_request = true;
			}

			Util_str_free(&dir);
		}
		else if (HID_NEXT_FILE_PRE_CFM(*key))
		{
			bool is_new_range[2] = { 0, };//Used by UPDATE_RANGE and CONFIRMED macro.

			HID_NEXT_FILE_UPDATE_RANGE(*key);

			if (HID_NEXT_FILE_CFM(*key))
			{
				if (util_expl_active_index < (NUM_OF_DISPLAYED_ITEMS - 1) && ((uint32_t)util_expl_active_index + 1) < util_expl_num_of_files)
					util_expl_active_index++; 
				else if ((util_expl_y_offset + util_expl_active_index + 1) < util_expl_num_of_files)
					util_expl_y_offset++;
			}
		}
		else if (HID_PRE_FILE_PRE_CFM(*key))
		{
			bool is_new_range[2] = { 0, };//Used by UPDATE_RANGE and CONFIRMED macro.

			HID_PRE_FILE_UPDATE_RANGE(*key);

			if (HID_PRE_FILE_CFM(*key))
			{
				if (util_expl_active_index > 0)
					util_expl_active_index--;
				else if (util_expl_y_offset > 0)
					util_expl_y_offset--;
			}
		}
	}

	//Notify user that button is NOT being pressed anymore.
	if(HID_FILE_DESELECTED_TOUCH(*key) || util_expl_scroll_mode)
	{
		for(uint8_t i = 0; i < NUM_OF_DISPLAYED_ITEMS; i++)
			util_expl_file_button[i].selected = false;
	}
	if(HID_SCROLL_MODE_DESEL(*key))
		util_expl_scroll_mode = false;
}

static void Util_expl_generate_file_type_string(Expl_file_type type, Str_data* type_string)
{
	if(type == EXPL_FILE_TYPE_NONE)
	{
		if(Util_str_add(type_string, "unknown,") != DEF_SUCCESS)
			return;
	}
	if(type & EXPL_FILE_TYPE_FILE)
	{
		if(Util_str_add(type_string, "file,") != DEF_SUCCESS)
			return;
	}
	if(type & EXPL_FILE_TYPE_DIR)
	{
		if(Util_str_add(type_string, "dir,") != DEF_SUCCESS)
			return;
	}
	if(type & EXPL_FILE_TYPE_READ_ONLY)
	{
		if(Util_str_add(type_string, "read only,") != DEF_SUCCESS)
			return;
	}
	if(type & EXPL_FILE_TYPE_HIDDEN)
	{
		if(Util_str_add(type_string, "hidden,") != DEF_SUCCESS)
			return;
	}

	if(type_string->length > 0)//Remove last comma.
		Util_str_resize(type_string, (type_string->length - 1));
}

//We can't get rid of this "int" because library uses "int" type as return value.
static int Util_expl_compare_name(const void* a, const void* b)
{
	const Util_expl_file_compare* file_a = (const Util_expl_file_compare*)a;
	const Util_expl_file_compare* file_b = (const Util_expl_file_compare*)b;
	bool is_a_dir = (file_a->type & EXPL_FILE_TYPE_DIR);
	bool is_b_dir = (file_b->type & EXPL_FILE_TYPE_DIR);

	if((is_a_dir && is_b_dir)
	|| (!is_a_dir && !is_b_dir))
	{
		//Both elements have the same type, compare name.
		int32_t result = 0;
		uint32_t loop = Util_max(file_a->name.length, file_b->name.length);

		for(uint32_t i = 0; i < loop; i++)
		{
			char char_a = 0x00;
			char char_b = 0x00;
			uint8_t a_type = SORT_TYPE_UNDEFINED;
			uint8_t b_type = SORT_TYPE_UNDEFINED;

			if(i < file_a->name.length)
				char_a = file_a->name.buffer[i];
			if(i < file_b->name.length)
				char_b = file_b->name.buffer[i];

			if(char_a == char_b)
				continue;

			if(char_a >= '0' && char_a <= '9')
				a_type = SORT_TYPE_NUMBER;
			else if((char_a >= 'a' && char_a <= 'z') || (char_a >= 'A' && char_a <= 'Z'))
				a_type = SORT_TYPE_ALPHABET;
			else
				a_type = SORT_TYPE_SPECIAL_CHAR;

			if(char_b >= '0' && char_b <= '9')
				b_type = SORT_TYPE_NUMBER;
			else if((char_b >= 'a' && char_b <= 'z') || (char_b >= 'A' && char_b <= 'Z'))
				b_type = SORT_TYPE_ALPHABET;
			else
				b_type = SORT_TYPE_SPECIAL_CHAR;

			if(a_type == SORT_TYPE_NUMBER && b_type == SORT_TYPE_NUMBER)
			{
				//Both characters are numbers, just compare with ASCII values.
				result = ((int16_t)char_a - (int16_t)char_b);
				break;
			}
			else if(a_type == SORT_TYPE_ALPHABET && b_type == SORT_TYPE_ALPHABET)
			{
				//Both characters are alphabets, compare with ASCII values after lowering them.
				result = ((int16_t)tolower(char_a) - (int16_t)tolower(char_b));
				break;
			}
			else if(a_type == SORT_TYPE_SPECIAL_CHAR && b_type == SORT_TYPE_SPECIAL_CHAR)
			{
				//Both characters are special characters, just compare with ASCII values.
				result = ((int16_t)char_a - (int16_t)char_b);
				break;
			}
			else
			{
				//Both characters have the different type.
				//Special characters should go first, then numbers, finally alphabets.
				if(a_type == SORT_TYPE_SPECIAL_CHAR)
					result = -1;
				else if(b_type == SORT_TYPE_SPECIAL_CHAR)
					result = 1;
				else if(a_type == SORT_TYPE_NUMBER)
					result = -1;
				else if(b_type == SORT_TYPE_NUMBER)
					result = 1;
				else if(a_type == SORT_TYPE_ALPHABET)
					result = -1;
				else if(b_type == SORT_TYPE_ALPHABET)
					result = 1;

				break;
			}
		}

		return result;
	}
	else
	{
		if(is_a_dir)//Directories should go first.
			return -1;
		else//Files should go after directories.
			return 1;
	}
}

static void Util_expl_read_dir_callback(void)
{
	if (util_expl_init)
	{
		if (util_expl_read_dir_request)
		{
			bool is_root_dir = false;
			uint32_t detected_files = 0;
			uint32_t result = DEF_ERR_OTHER;
			//We don't want to fly our stack.
			static Util_expl_files files = { 0, };

			for (uint32_t i = 0; i < DEF_EXPL_MAX_FILES; i++)
			{
				Util_str_clear(&util_expl_files.name[i]);
				util_expl_files.type[i] = EXPL_FILE_TYPE_NONE;
				util_expl_files.size[i] = 0;
			}
			Draw_set_refresh_needed(true);

			memset(&files, 0x00, sizeof(files));

			if(strcmp(util_expl_current_dir.buffer, "/") == 0)
				is_root_dir = true;

			if (!is_root_dir)
			{
				Util_str_set(&util_expl_files.name[0], ".. (Move to parent directory)");
				util_expl_files.type[0] = EXPL_FILE_TYPE_DIR;
			}

			//Read files in directory.
			DEF_LOG_RESULT_SMART(result, Util_file_read_dir(util_expl_current_dir.buffer, &detected_files, files.name, files.type, DEF_EXPL_MAX_FILES), (result == DEF_SUCCESS), result);

			if (result == DEF_SUCCESS)
			{
				//Non-root directory has a directory named "Go to parent directory".
				uint8_t offset = (is_root_dir ? 0 : 1);
				uint32_t loop = 0;
				//We don't want to fly our stack.
				static Util_expl_file_compare sort_cache[DEF_EXPL_MAX_FILES] = { 0, };

				memset(sort_cache, 0x00, sizeof(sort_cache));
				for(uint32_t i = 0; i < detected_files; i++)
				{
					sort_cache[i].name = files.name[i];
					sort_cache[i].type = files.type[i];
				}

				qsort(sort_cache, detected_files, sizeof(Util_expl_file_compare), Util_expl_compare_name);

				loop = (uint32_t)Util_min((offset + detected_files), DEF_EXPL_MAX_FILES);
				for(uint32_t i = offset, source_index = 0; i < loop; i++)
				{
					Util_str_set(&util_expl_files.name[i], sort_cache[source_index].name.buffer);
					util_expl_files.type[i] = sort_cache[source_index].type;
					source_index++;
				}

				util_expl_num_of_files = (detected_files + offset);
			}
			else
				util_expl_num_of_files = 1;

			for(uint32_t i = 0; i < DEF_EXPL_MAX_FILES; i++)
				Util_str_free(&files.name[i]);

			util_expl_check_file_size_index = 0;
			Draw_set_refresh_needed(true);
			util_expl_read_dir_request = false;
		}
		else if(util_expl_check_file_size_index < util_expl_num_of_files)
		{
			while(util_expl_check_file_size_index < util_expl_num_of_files)
			{
				if(util_expl_files.type[util_expl_check_file_size_index] & EXPL_FILE_TYPE_FILE || util_expl_files.type[util_expl_check_file_size_index] & EXPL_FILE_TYPE_NONE)
				{
					uint64_t file_size = 0;
					uint32_t result = DEF_ERR_OTHER;

					result = Util_file_check_file_size(util_expl_files.name[util_expl_check_file_size_index].buffer, util_expl_current_dir.buffer, &file_size);
					if (result == DEF_SUCCESS)
					{
						util_expl_files.size[util_expl_check_file_size_index] = file_size;
						Draw_set_refresh_needed(true);
					}
					else
						DEF_LOG_RESULT(Util_file_check_file_size, (result == DEF_SUCCESS), result);

					util_expl_check_file_size_index++;
					//Don't check all files once as it locks worker thread too long.
					break;
				}
				else
					util_expl_check_file_size_index++;
			}
		}
	}
}
#endif //DEF_EXPL_API_ENABLE
