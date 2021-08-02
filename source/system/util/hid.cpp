#include "headers.hpp"

bool hid_scan_hid_thread_run = false;
bool hid_key_A_press = false;
bool hid_key_B_press = false;
bool hid_key_X_press = false;
bool hid_key_Y_press = false;
bool hid_key_C_UP_press = false;
bool hid_key_C_RIGHT_press = false;
bool hid_key_C_DOWN_press = false;
bool hid_key_C_LEFT_press = false;
bool hid_key_D_UP_press = false;
bool hid_key_D_RIGHT_press = false;
bool hid_key_D_DOWN_press = false;
bool hid_key_D_LEFT_press = false;
bool hid_key_L_press = false;
bool hid_key_R_press = false;
bool hid_key_ZL_press = false;
bool hid_key_ZR_press = false;
bool hid_key_START_press = false;
bool hid_key_SELECT_press = false;
bool hid_key_CS_UP_press = false;
bool hid_key_CS_DOWN_press = false;
bool hid_key_CS_RIGHT_press = false;
bool hid_key_CS_LEFT_press = false;
bool hid_key_touch_press = false;
bool hid_key_any_press = false;
bool hid_key_A_held = false;
bool hid_key_B_held = false;
bool hid_key_X_held = false;
bool hid_key_Y_held = false;
bool hid_key_C_UP_held = false;
bool hid_key_C_DOWN_held = false;
bool hid_key_C_RIGHT_held = false;
bool hid_key_C_LEFT_held = false;
bool hid_key_D_UP_held = false;
bool hid_key_D_DOWN_held = false;
bool hid_key_D_RIGHT_held = false;
bool hid_key_D_LEFT_held = false;
bool hid_key_L_held = false;
bool hid_key_R_held = false;
bool hid_key_ZL_held = false;
bool hid_key_ZR_held = false;
bool hid_key_START_held = false;
bool hid_key_SELECT_held = false;
bool hid_key_CS_UP_held = false;
bool hid_key_CS_DOWN_held = false;
bool hid_key_CS_RIGHT_held = false;
bool hid_key_CS_LEFT_held = false;
bool hid_key_touch_held = false;
bool hid_key_any_held = false;
int hid_cpad_pos_x = 0;
int hid_cpad_pos_y = 0;
int hid_touch_pos_x = 0;
int hid_pre_touch_pos_x = 0;
int hid_touch_pos_x_moved = 0;
int hid_touch_pos_y = 0;
int hid_pre_touch_pos_y = 0;
int hid_touch_pos_y_moved = 0;
int hid_held_time = 0;
int hid_count = 0;
std::string hid_scan_hid_thread_string = "Hid/Scan hid thread";
Thread hid_scan_hid_thread;

void Util_hid_init(void)
{
	hid_scan_hid_thread_run = true;
	hid_scan_hid_thread = threadCreate(Util_hid_scan_hid_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, -1, false);
}

void Util_hid_exit(void)
{
	hid_scan_hid_thread_run = false;
	threadJoin(hid_scan_hid_thread, 10000000000);
	threadFree(hid_scan_hid_thread);
}

void Util_hid_query_key_state(Hid_info* out_key_state)
{
	out_key_state->p_a = hid_key_A_press;
	out_key_state->p_b = hid_key_B_press;
	out_key_state->p_x = hid_key_X_press;
	out_key_state->p_y = hid_key_Y_press;
	out_key_state->p_c_up = hid_key_C_UP_press;
	out_key_state->p_c_down = hid_key_C_DOWN_press;
	out_key_state->p_c_left = hid_key_C_LEFT_press;
	out_key_state->p_c_right = hid_key_C_RIGHT_press;
	out_key_state->p_d_up = hid_key_D_UP_press;
	out_key_state->p_d_down = hid_key_D_DOWN_press;
	out_key_state->p_d_left = hid_key_D_LEFT_press;
	out_key_state->p_d_right = hid_key_D_RIGHT_press;
	out_key_state->p_l = hid_key_L_press;
	out_key_state->p_r = hid_key_R_press;
	out_key_state->p_zl = hid_key_ZL_press;
	out_key_state->p_zr = hid_key_ZR_press;
	out_key_state->p_start = hid_key_START_press;
	out_key_state->p_select = hid_key_SELECT_press;
	out_key_state->p_cs_up = hid_key_CS_UP_press;
	out_key_state->p_cs_down = hid_key_CS_DOWN_press;
	out_key_state->p_cs_left = hid_key_CS_LEFT_press;
	out_key_state->p_cs_right = hid_key_CS_RIGHT_press;
	out_key_state->p_touch = hid_key_touch_press;
	out_key_state->p_any = hid_key_any_press;
	out_key_state->h_a = hid_key_A_held;
	out_key_state->h_b = hid_key_B_held;
	out_key_state->h_x = hid_key_X_held;
	out_key_state->h_y = hid_key_Y_held;
	out_key_state->h_c_up = hid_key_C_UP_held;
	out_key_state->h_c_down = hid_key_C_DOWN_held;
	out_key_state->h_c_left = hid_key_C_LEFT_held;
	out_key_state->h_c_right = hid_key_C_RIGHT_held;
	out_key_state->h_d_up = hid_key_D_UP_held;
	out_key_state->h_d_down = hid_key_D_DOWN_held;
	out_key_state->h_d_left = hid_key_D_LEFT_held;
	out_key_state->h_d_right = hid_key_D_RIGHT_held;
	out_key_state->h_l = hid_key_L_held;
	out_key_state->h_r = hid_key_R_held;
	out_key_state->h_zl = hid_key_ZL_held;
	out_key_state->h_zr = hid_key_ZR_held;
	out_key_state->h_start = hid_key_START_held;
	out_key_state->h_select = hid_key_SELECT_held;
	out_key_state->h_cs_up = hid_key_CS_UP_held;
	out_key_state->h_cs_down = hid_key_CS_DOWN_held;
	out_key_state->h_cs_left = hid_key_CS_LEFT_held;
	out_key_state->h_cs_right = hid_key_CS_RIGHT_held;
	out_key_state->h_touch = hid_key_touch_held;
	out_key_state->h_any = hid_key_any_held;
	out_key_state->cpad_x = hid_cpad_pos_x;
	out_key_state->cpad_y = hid_cpad_pos_y;
	out_key_state->touch_x = hid_touch_pos_x;
	out_key_state->touch_y = hid_touch_pos_y;
	out_key_state->touch_x_move = hid_touch_pos_x_moved;
	out_key_state->touch_y_move = hid_touch_pos_y_moved;
	out_key_state->held_time = hid_held_time;
	out_key_state->count = hid_count;
}

void Util_hid_key_flag_reset(void)
{
	hid_key_A_press = false;
	hid_key_B_press = false;
	hid_key_X_press = false;
	hid_key_Y_press = false;
	hid_key_C_UP_press = false;
	hid_key_C_DOWN_press = false;
	hid_key_C_RIGHT_press = false;
	hid_key_C_LEFT_press = false;
	hid_key_D_UP_press = false;
	hid_key_D_DOWN_press = false;
	hid_key_D_RIGHT_press = false;
	hid_key_D_LEFT_press = false;
	hid_key_L_press = false;
	hid_key_R_press = false;
	hid_key_ZL_press = false;
	hid_key_ZR_press = false;
	hid_key_START_press = false;
	hid_key_SELECT_press = false;
	hid_key_CS_UP_press = false;
	hid_key_CS_DOWN_press = false;
	hid_key_CS_RIGHT_press = false;
	hid_key_CS_LEFT_press = false;
	hid_key_touch_press = false;
	hid_key_any_press = false;
	hid_key_A_held = false;
	hid_key_B_held = false;
	hid_key_X_held = false;
	hid_key_Y_held = false;
	hid_key_C_UP_held = false;
	hid_key_C_DOWN_held = false;
	hid_key_C_RIGHT_held = false;
	hid_key_C_LEFT_held = false;
	hid_key_D_UP_held = false;
	hid_key_D_DOWN_held = false;
	hid_key_D_RIGHT_held = false;
	hid_key_D_LEFT_held = false;
	hid_key_L_held = false;
	hid_key_R_held = false;
	hid_key_ZL_held = false;
	hid_key_ZR_held = false;
	hid_key_START_held = false;
	hid_key_SELECT_held = false;
	hid_key_CS_UP_held = false;
	hid_key_CS_DOWN_held = false;
	hid_key_CS_RIGHT_held = false;
	hid_key_CS_LEFT_held = false;
	hid_key_touch_held = false;
	hid_key_any_held = false;
	hid_touch_pos_x = 0;
	hid_touch_pos_y = 0;
	hid_count = 0;
}

void Util_hid_scan_hid_thread(void* arg)
{
	Util_log_save(hid_scan_hid_thread_string, "Thread started.");

	u32 kDown;
	u32 kHeld;
	touchPosition touch_pos;
	circlePosition circle_pos;
	Result_with_string result;

	while (hid_scan_hid_thread_run)
	{
		hidScanInput();
		hidTouchRead(&touch_pos);
		hidCircleRead(&circle_pos);
		kHeld = hidKeysHeld();
		kDown = hidKeysDown();

		if (kDown & KEY_A)
			hid_key_A_press = true;
		else
			hid_key_A_press = false;

		if (kDown & KEY_B)
			hid_key_B_press = true;
		else
			hid_key_B_press = false;

		if (kDown & KEY_X)
			hid_key_X_press = true;
		else
			hid_key_X_press = false;

		if (kDown & KEY_Y)
			hid_key_Y_press = true;
		else
			hid_key_Y_press = false;

		if (kDown & KEY_DUP)
			hid_key_D_UP_press = true;
		else
			hid_key_D_UP_press = false;

		if (kDown & KEY_DDOWN)
			hid_key_D_DOWN_press = true;
		else
			hid_key_D_DOWN_press = false;

		if (kDown & KEY_DRIGHT)
			hid_key_D_RIGHT_press = true;
		else
			hid_key_D_RIGHT_press = false;

		if (kDown & KEY_DLEFT)
			hid_key_D_LEFT_press = true;
		else
			hid_key_D_LEFT_press = false;

		if (kDown & KEY_CPAD_UP)
			hid_key_C_UP_press = true;
		else
			hid_key_C_UP_press = false;

		if (kDown & KEY_CPAD_DOWN)
			hid_key_C_DOWN_press = true;
		else
			hid_key_C_DOWN_press = false;

		if (kDown & KEY_CPAD_RIGHT)
			hid_key_C_RIGHT_press = true;
		else
			hid_key_C_RIGHT_press = false;

		if (kDown & KEY_CPAD_LEFT)
			hid_key_C_LEFT_press = true;
		else
			hid_key_C_LEFT_press = false;

		if (kDown & KEY_SELECT)
			hid_key_SELECT_press = true;
		else
			hid_key_SELECT_press = false;

		if (kDown & KEY_START)
			hid_key_START_press = true;
		else
			hid_key_START_press = false;

		if (kDown & KEY_L)
			hid_key_L_press = true;
		else
			hid_key_L_press = false;

		if (kDown & KEY_R)
			hid_key_R_press = true;
		else
			hid_key_R_press = false;

		if (kDown & KEY_ZL)
			hid_key_ZL_press = true;
		else
			hid_key_ZL_press = false;

		if (kDown & KEY_ZR)
			hid_key_ZR_press = true;
		else
			hid_key_ZR_press = false;

		if (kHeld & KEY_DRIGHT)
			hid_key_D_RIGHT_held = true;
		else
			hid_key_D_RIGHT_held = false;

		if (kHeld & KEY_DLEFT)
			hid_key_D_LEFT_held = true;
		else
			hid_key_D_LEFT_held = false;

		if (kHeld & KEY_DDOWN)
			hid_key_D_DOWN_held = true;
		else
			hid_key_D_DOWN_held = false;

		if (kHeld & KEY_DUP)
			hid_key_D_UP_held = true;
		else
			hid_key_D_UP_held = false;

		if (kHeld & KEY_A)
			hid_key_A_held = true;
		else
			hid_key_A_held = false;

		if (kHeld & KEY_B)
			hid_key_B_held = true;
		else
			hid_key_B_held = false;

		if (kHeld & KEY_Y)
			hid_key_Y_held = true;
		else
			hid_key_Y_held = false;

		if (kHeld & KEY_X)
			hid_key_X_held = true;
		else
			hid_key_X_held = false;

		if (kHeld & KEY_CPAD_UP)
			hid_key_C_UP_held = true;
		else
			hid_key_C_UP_held = false;

		if (kHeld & KEY_CPAD_DOWN)
			hid_key_C_DOWN_held = true;
		else
			hid_key_C_DOWN_held = false;

		if (kHeld & KEY_CPAD_RIGHT)
			hid_key_C_RIGHT_held = true;
		else
			hid_key_C_RIGHT_held = false;

		if (kHeld & KEY_CPAD_LEFT)
			hid_key_C_LEFT_held = true;
		else
			hid_key_C_LEFT_held = false;

		if (kHeld & KEY_L)
			hid_key_L_held = true;
		else
			hid_key_L_held = false;

		if (kHeld & KEY_R)
			hid_key_R_held = true;
		else
			hid_key_R_held = false;

		if (kHeld & KEY_ZL)
			hid_key_ZL_held = true;
		else
			hid_key_ZL_held = false;

		if (kHeld & KEY_ZR)
			hid_key_ZR_held = true;
		else
			hid_key_ZR_held = false;

		if (kDown & KEY_TOUCH || kHeld & KEY_TOUCH)
		{
			if (kDown & KEY_TOUCH)
			{
				hid_key_touch_press = true;
				hid_key_touch_held = false;
				hid_pre_touch_pos_x = touch_pos.px;
				hid_pre_touch_pos_y = touch_pos.py;
				hid_touch_pos_x = touch_pos.px;
				hid_touch_pos_y = touch_pos.py;
			}
			else if (kHeld & KEY_TOUCH)
			{
				hid_key_touch_held = true;
				hid_key_touch_press = false;
				hid_touch_pos_x = touch_pos.px;
				hid_touch_pos_y = touch_pos.py;
				hid_touch_pos_x_moved = hid_pre_touch_pos_x - hid_touch_pos_x;
				hid_touch_pos_y_moved = hid_pre_touch_pos_y - hid_touch_pos_y;
				hid_pre_touch_pos_x = touch_pos.px;
				hid_pre_touch_pos_y = touch_pos.py;
			}
		}
		else
		{
			hid_key_touch_press = false;
			hid_key_touch_held = false;
			hid_touch_pos_x = -1;
			hid_touch_pos_y = -1;
			hid_touch_pos_x_moved = 0;
			hid_touch_pos_y_moved = 0;
			hid_pre_touch_pos_x = 0;
			hid_pre_touch_pos_y = 0;
		}

		hid_cpad_pos_x = circle_pos.dx;
		hid_cpad_pos_y = circle_pos.dy;

		if (hid_key_A_press || hid_key_B_press || hid_key_X_press || hid_key_Y_press || 
			hid_key_C_UP_press || hid_key_C_RIGHT_press || hid_key_C_DOWN_press || hid_key_C_LEFT_press || 
			hid_key_D_UP_press || hid_key_D_RIGHT_press || hid_key_D_DOWN_press || hid_key_D_LEFT_press || 
			hid_key_L_press || hid_key_R_press || hid_key_ZL_press || hid_key_ZR_press || 
			hid_key_START_press || hid_key_SELECT_press || hid_key_CS_UP_press || hid_key_CS_DOWN_press || 
			hid_key_CS_RIGHT_press || hid_key_CS_LEFT_press || hid_key_touch_press ||  hid_key_A_held || 
			hid_key_B_held || hid_key_X_held || hid_key_Y_held || hid_key_C_UP_held || hid_key_C_DOWN_held || 
			hid_key_C_RIGHT_held || hid_key_C_LEFT_held || hid_key_D_UP_held || hid_key_D_DOWN_held || 
			hid_key_D_RIGHT_held || hid_key_D_LEFT_held || hid_key_L_held || hid_key_R_held || hid_key_ZL_held || 
			hid_key_ZR_held || hid_key_START_held || hid_key_SELECT_held || hid_key_CS_UP_held || 
			hid_key_CS_DOWN_held || hid_key_CS_RIGHT_held || hid_key_CS_LEFT_held || hid_key_touch_held)
		{
			if(hid_key_A_press || hid_key_B_press || hid_key_X_press || hid_key_Y_press || 
			hid_key_C_UP_press || hid_key_C_RIGHT_press || hid_key_C_DOWN_press || hid_key_C_LEFT_press || 
			hid_key_D_UP_press || hid_key_D_RIGHT_press || hid_key_D_DOWN_press || hid_key_D_LEFT_press || 
			hid_key_L_press || hid_key_R_press || hid_key_ZL_press || hid_key_ZR_press || 
			hid_key_START_press || hid_key_SELECT_press || hid_key_CS_UP_press || hid_key_CS_DOWN_press || 
			hid_key_CS_RIGHT_press || hid_key_CS_LEFT_press || hid_key_touch_press)
				hid_key_any_press = true;
			if(hid_key_A_held || 
			hid_key_B_held || hid_key_X_held || hid_key_Y_held || hid_key_C_UP_held || hid_key_C_DOWN_held || 
			hid_key_C_RIGHT_held || hid_key_C_LEFT_held || hid_key_D_UP_held || hid_key_D_DOWN_held || 
			hid_key_D_RIGHT_held || hid_key_D_LEFT_held || hid_key_L_held || hid_key_R_held || hid_key_ZL_held || 
			hid_key_ZR_held || hid_key_START_held || hid_key_SELECT_held || hid_key_CS_UP_held || 
			hid_key_CS_DOWN_held || hid_key_CS_RIGHT_held || hid_key_CS_LEFT_held || hid_key_touch_held)
				hid_key_any_held = true;

			var_afk_time = 0;
		}

		if (hid_key_D_UP_held || hid_key_D_DOWN_held || hid_key_D_RIGHT_held || hid_key_D_LEFT_held
			|| hid_key_C_UP_held || hid_key_C_DOWN_held || hid_key_C_RIGHT_held || hid_key_C_LEFT_held
			|| hid_key_CS_UP_held || hid_key_CS_DOWN_held || hid_key_CS_RIGHT_held || hid_key_CS_LEFT_held
			|| hid_key_touch_held)
			hid_held_time++;
		else
			hid_held_time = 0;

		hid_count++;

		gspWaitForVBlank();
	}
	Util_log_save(hid_scan_hid_thread_string, "Thread exit");
	threadExit(0);
}
