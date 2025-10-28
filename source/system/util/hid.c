//Includes.
#include "system/util/hid.h"

#include <stdbool.h>
#include <stdint.h>

#include "system/draw/draw.h"
#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/sync.h"
#include "system/util/thread_types.h"

//Defines.
#define DEF_HID_UPDATE_STATE_ARG(KEY, key)	(key_pressed & KEY), (key_held & KEY), (key_released & KEY), util_hid_info.ts, &util_hid_internal_keys.key, &util_hid_info.key

//Typedefs.
typedef enum
{
	HID_INTERNAL_STATE_INVALID = -1,

	HID_INTERNAL_STATE_NONE,
	HID_INTERNAL_STATE_PRESSED_WAIT_HOLD_THRESHOLD,
	HID_INTERNAL_STATE_PRESSED_WAIT_RELEASE,
	HID_INTERNAL_STATE_HELD_WAIT_RELEASE,
	HID_INTERNAL_STATE_RELEASED_WAIT_RELEASE_THRESHOLD,
	HID_INTERNAL_STATE_HELD,

	HID_INTERNAL_STATE_MAX,
} Hid_internal_state;

typedef struct
{
	bool wait_release;
	uint16_t click_count;
	uint32_t held_ms;
	uint64_t ts;
	Hid_internal_state state;
} Hid_internal_info;

typedef struct
{
	Hid_internal_info a;
	Hid_internal_info b;
	Hid_internal_info x;
	Hid_internal_info y;
	Hid_internal_info l;
	Hid_internal_info r;
	Hid_internal_info zl;
	Hid_internal_info zr;
	Hid_internal_info start;
	Hid_internal_info select;
	Hid_internal_info c_up;
	Hid_internal_info c_down;
	Hid_internal_info c_left;
	Hid_internal_info c_right;
	Hid_internal_info d_up;
	Hid_internal_info d_down;
	Hid_internal_info d_left;
	Hid_internal_info d_right;
	Hid_internal_info cs_up;
	Hid_internal_info cs_down;
	Hid_internal_info cs_left;
	Hid_internal_info cs_right;
	Hid_internal_info touch;
} Hid_internal_keys;

//Prototypes.
static void Util_hid_update_key_state(bool is_pressed, bool is_held, bool is_released, uint64_t current_ts, Hid_internal_info* internal_key, Hid_key* key);
void Util_hid_scan_hid_thread(void* arg);

//Variables.
static bool util_hid_thread_run = false;
static bool util_hid_init = false;
static Hid_info util_hid_info = { 0, };
static Hid_internal_keys util_hid_internal_keys = { 0, };
static Sync_data util_hid_callback_mutex = { 0, }, util_hid_data_mutex = { 0, };
static void (*util_hid_callbacks[DEF_HID_NUM_OF_CALLBACKS])(void) = { 0, };
static Thread util_hid_scan_thread = NULL;

//Code.
uint32_t Util_hid_init(void)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_hid_init)
		goto already_inited;

	for(uint16_t i = 0; i < DEF_HID_NUM_OF_CALLBACKS; i++)
		util_hid_callbacks[i] = NULL;

	result = Util_sync_create(&util_hid_callback_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	result = Util_sync_create(&util_hid_data_mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_sync_create, false, result);
		goto error_other;
	}

	result = hidInit();
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(hidInit, false, result);
		goto nintendo_api_failed;
	}

	util_hid_thread_run = true;
	util_hid_scan_thread = threadCreate(Util_hid_scan_hid_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_REALTIME, 0, false);
	if(!util_hid_scan_thread)
	{
		result = DEF_ERR_OTHER;
		DEF_LOG_RESULT(threadCreate, false, result);
		goto nintendo_api_failed_0;
	}

	util_hid_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	nintendo_api_failed_0:
	hidExit();
	//Fallthrough.

	nintendo_api_failed:
	error_other:
	Util_sync_destroy(&util_hid_callback_mutex);
	Util_sync_destroy(&util_hid_data_mutex);
	return result;
}

void Util_hid_exit(void)
{
	if(!util_hid_init)
		return;

	util_hid_init = false;
	util_hid_thread_run = false;
	threadJoin(util_hid_scan_thread, DEF_THREAD_WAIT_TIME);
	threadFree(util_hid_scan_thread);
	hidExit();
	Util_sync_destroy(&util_hid_callback_mutex);
	Util_sync_destroy(&util_hid_data_mutex);
}

uint32_t Util_hid_query_key_state(Hid_info* out_key_state)
{
	if(!util_hid_init)
		goto not_inited;

	if(!out_key_state)
		goto invalid_arg;

	Util_sync_lock(&util_hid_data_mutex, UINT64_MAX);
	*out_key_state = util_hid_info;
	Util_sync_unlock(&util_hid_data_mutex);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;
}

void Util_hid_reset_afk_time(void)
{
	if(!util_hid_init)
		return;

	Util_sync_lock(&util_hid_data_mutex, UINT64_MAX);
	util_hid_info.afk_time_ms = 0;
	Util_sync_unlock(&util_hid_data_mutex);
}

void Util_hid_reset_key_state(Hid_key_bit keys)
{
	Hid_key clean = { 0, };
	Hid_internal_info clean_internal = { 0, };

	if(!util_hid_init)
		return;

	//Set wait release flag so that user have to release the key first.
	clean_internal.wait_release = true;

	Util_sync_lock(&util_hid_data_mutex, UINT64_MAX);
	if(keys & HID_KEY_BIT_A)
	{
		util_hid_info.a = clean;
		util_hid_internal_keys.a = clean_internal;
	}
	if(keys & HID_KEY_BIT_B)
	{
		util_hid_info.b = clean;
		util_hid_internal_keys.b = clean_internal;
	}
	if(keys & HID_KEY_BIT_X)
	{
		util_hid_info.x = clean;
		util_hid_internal_keys.x = clean_internal;
	}
	if(keys & HID_KEY_BIT_Y)
	{
		util_hid_info.y = clean;
		util_hid_internal_keys.y = clean_internal;
	}
	if(keys & HID_KEY_BIT_L)
	{
		util_hid_info.l = clean;
		util_hid_internal_keys.l = clean_internal;
	}
	if(keys & HID_KEY_BIT_R)
	{
		util_hid_info.r = clean;
		util_hid_internal_keys.r = clean_internal;
	}
	if(keys & HID_KEY_BIT_ZL)
	{
		util_hid_info.zl = clean;
		util_hid_internal_keys.zl = clean_internal;
	}
	if(keys & HID_KEY_BIT_ZR)
	{
		util_hid_info.zr = clean;
		util_hid_internal_keys.zr = clean_internal;
	}
	if(keys & HID_KEY_BIT_START)
	{
		util_hid_info.start = clean;
		util_hid_internal_keys.start = clean_internal;
	}
	if(keys & HID_KEY_BIT_SELECT)
	{
		util_hid_info.select = clean;
		util_hid_internal_keys.select = clean_internal;
	}
	if(keys & HID_KEY_BIT_C_UP)
	{
		util_hid_info.c_up = clean;
		util_hid_internal_keys.c_up = clean_internal;
	}
	if(keys & HID_KEY_BIT_C_DOWN)
	{
		util_hid_info.c_down = clean;
		util_hid_internal_keys.c_down = clean_internal;
	}
	if(keys & HID_KEY_BIT_C_LEFT)
	{
		util_hid_info.c_left = clean;
		util_hid_internal_keys.c_left = clean_internal;
	}
	if(keys & HID_KEY_BIT_C_RIGHT)
	{
		util_hid_info.c_right = clean;
		util_hid_internal_keys.c_right = clean_internal;
	}
	if(keys & HID_KEY_BIT_D_UP)
	{
		util_hid_info.d_up = clean;
		util_hid_internal_keys.d_up = clean_internal;
	}
	if(keys & HID_KEY_BIT_D_DOWN)
	{
		util_hid_info.d_down = clean;
		util_hid_internal_keys.d_down = clean_internal;
	}
	if(keys & HID_KEY_BIT_D_LEFT)
	{
		util_hid_info.d_left = clean;
		util_hid_internal_keys.d_left = clean_internal;
	}
	if(keys & HID_KEY_BIT_D_RIGHT)
	{
		util_hid_info.d_right = clean;
		util_hid_internal_keys.d_right = clean_internal;
	}
	if(keys & HID_KEY_BIT_CS_UP)
	{
		util_hid_info.cs_up = clean;
		util_hid_internal_keys.cs_up = clean_internal;
	}
	if(keys & HID_KEY_BIT_CS_DOWN)
	{
		util_hid_info.cs_down = clean;
		util_hid_internal_keys.cs_down = clean_internal;
	}
	if(keys & HID_KEY_BIT_CS_LEFT)
	{
		util_hid_info.cs_left = clean;
		util_hid_internal_keys.cs_left = clean_internal;
	}
	if(keys & HID_KEY_BIT_CS_RIGHT)
	{
		util_hid_info.cs_right = clean;
		util_hid_internal_keys.cs_right = clean_internal;
	}
	if(keys & HID_KEY_BIT_TOUCH)
	{
		util_hid_info.touch = clean;
		util_hid_internal_keys.touch = clean_internal;
	}

	Util_sync_unlock(&util_hid_data_mutex);
}

bool Util_hid_add_callback(void (*const callback)(void))
{
	if(!util_hid_init)
		return false;

	Util_sync_lock(&util_hid_callback_mutex, UINT64_MAX);

	for(uint16_t i = 0; i < DEF_HID_NUM_OF_CALLBACKS; i++)
	{
		if(util_hid_callbacks[i] == callback)
			goto success;//Already exist.
	}

	for(uint16_t i = 0; i < DEF_HID_NUM_OF_CALLBACKS; i++)
	{
		if(!util_hid_callbacks[i])
		{
			util_hid_callbacks[i] = callback;
			goto success;
		}
	}

	//No free spaces left.
	Util_sync_unlock(&util_hid_callback_mutex);
	return false;

	success:
	Util_sync_unlock(&util_hid_callback_mutex);
	return true;
}

void Util_hid_remove_callback(void (*const callback)(void))
{
	if(!util_hid_init)
		return;

	Util_sync_lock(&util_hid_callback_mutex, UINT64_MAX);

	for(uint16_t i = 0; i < DEF_HID_NUM_OF_CALLBACKS; i++)
	{
		if(util_hid_callbacks[i] == callback)
		{
			util_hid_callbacks[i] = NULL;
			break;
		}
	}

	Util_sync_unlock(&util_hid_callback_mutex);
}

static void Util_hid_update_key_state(bool is_pressed, bool is_held, bool is_released, uint64_t current_ts, Hid_internal_info* internal_key, Hid_key* key)
{
	if(!internal_key || !key)
		return;

	if(is_released)
	{
		is_pressed = false;
		is_held = false;
	}
	else if(is_pressed)
		is_held = false;

	//Reset state except was_active.
	key->was_active = key->is_active;
	key->state = HID_STATE_NOT_PRESSED;
	key->click_count = 0;
	key->held_ms = 0;
	key->is_active = false;

	if(is_released || (!is_pressed && !is_held))
		internal_key->wait_release = false;

	if(internal_key->wait_release)
		return;

	if(is_pressed || is_held)
		key->is_active = true;

	if(internal_key->state == HID_INTERNAL_STATE_NONE)
	{
		if(is_pressed)
		{
			//The button has been pressed (first time).
			internal_key->state = HID_INTERNAL_STATE_PRESSED_WAIT_HOLD_THRESHOLD;
			internal_key->click_count = 0;
			internal_key->held_ms = 0;
			internal_key->ts = current_ts;

			key->state = HID_STATE_PENDING;
			key->click_count = internal_key->click_count;
			key->held_ms = internal_key->held_ms;
		}
		else
		{
			//Nothing.
			internal_key->state = HID_INTERNAL_STATE_NONE;
			internal_key->click_count = 0;
			internal_key->held_ms = 0;
			internal_key->ts = 0;
		}
	}
	else if(internal_key->state == HID_INTERNAL_STATE_PRESSED_WAIT_HOLD_THRESHOLD)
	{
		if(is_held)
		{
			uint64_t elapsed_ms = (current_ts - internal_key->ts);

			if(elapsed_ms > DEF_HID_HELD_THRESHOLD_MS)
			{
				//User keeps holding the button, make it HELD event.
				internal_key->state = HID_INTERNAL_STATE_HELD_WAIT_RELEASE;
				internal_key->click_count = 0;
				internal_key->held_ms = (elapsed_ms > UINT32_MAX ? UINT32_MAX : elapsed_ms);
				internal_key->ts = current_ts;

				key->state = HID_STATE_HELD;
				key->click_count = internal_key->click_count;
				key->held_ms = internal_key->held_ms;
			}
			else
			{
				key->state = HID_STATE_PENDING;
				key->click_count = internal_key->click_count;
				key->held_ms = internal_key->held_ms;
			}
		}
		else if(is_released)
		{
			//The button has been released, make it PRESSED event
			//and wait for release timeout to check if user presses again. 
			internal_key->state = HID_INTERNAL_STATE_RELEASED_WAIT_RELEASE_THRESHOLD;
			internal_key->click_count = 1;
			internal_key->held_ms = 0;
			internal_key->ts = current_ts;

			key->state = HID_STATE_PRESSED;
			key->click_count = internal_key->click_count;
			key->held_ms = internal_key->held_ms;
		}
		else
		{
			//Shouldn't happen, reset everything.
			internal_key->state = HID_INTERNAL_STATE_NONE;
			internal_key->click_count = 0;
			internal_key->held_ms = 0;
			internal_key->ts = 0;
		}
	}
	else if(internal_key->state == HID_INTERNAL_STATE_RELEASED_WAIT_RELEASE_THRESHOLD)
	{
		if(is_released || is_held)
		{
			//Shouldn't happen, reset everything.
			internal_key->state = HID_INTERNAL_STATE_NONE;
			internal_key->click_count = 0;
			internal_key->held_ms = 0;
			internal_key->ts = 0;
		}
		else if(is_pressed)
		{
			internal_key->state = HID_INTERNAL_STATE_PRESSED_WAIT_RELEASE;
			//click_count will be incremented on release event.
			internal_key->held_ms = 0;
			internal_key->ts = current_ts;

			key->state = HID_STATE_PENDING;
			key->click_count = internal_key->click_count;
			key->held_ms = internal_key->held_ms;
		}
		else
		{
			uint64_t elapsed_ms = (current_ts - internal_key->ts);

			if(elapsed_ms > DEF_HID_RELEASE_THRESHOLD_MS)
			{
				//No more presses, copy state and reset everything.
				key->state = HID_STATE_PRESSED_DONE;
				key->click_count = internal_key->click_count;
				key->held_ms = internal_key->held_ms;

				internal_key->state = HID_INTERNAL_STATE_NONE;
				internal_key->click_count = 0;
				internal_key->held_ms = 0;
				internal_key->ts = 0;
			}
			else
			{
				key->state = HID_STATE_PENDING;
				key->click_count = internal_key->click_count;
				key->held_ms = internal_key->held_ms;
			}
		}
	}
	else if(internal_key->state == HID_INTERNAL_STATE_HELD_WAIT_RELEASE)
	{
		if(is_held)
		{
			uint64_t elapsed_ms = (current_ts - internal_key->ts);

			//User keeps holding the button.
			internal_key->state = HID_INTERNAL_STATE_HELD_WAIT_RELEASE;
			internal_key->click_count = 0;
			if(elapsed_ms > (UINT32_MAX - internal_key->held_ms))
				internal_key->held_ms = UINT32_MAX;
			else
				internal_key->held_ms += elapsed_ms;

			internal_key->ts = current_ts;

			key->state = HID_STATE_HELD;
			key->click_count = internal_key->click_count;
			key->held_ms = internal_key->held_ms;
		}
		else if(is_released)
		{
			//User has finally released the button, copy state and reset everything.
			key->state = HID_STATE_HELD_DONE;
			key->click_count = internal_key->click_count;
			key->held_ms = internal_key->held_ms;

			internal_key->state = HID_INTERNAL_STATE_NONE;
			internal_key->click_count = 0;
			internal_key->held_ms = 0;
			internal_key->ts = 0;
		}
		else
		{
			//Shouldn't happen, reset everything.
			internal_key->state = HID_INTERNAL_STATE_NONE;
			internal_key->click_count = 0;
			internal_key->held_ms = 0;
			internal_key->ts = 0;
		}
	}
	else if(internal_key->state == HID_INTERNAL_STATE_PRESSED_WAIT_RELEASE)
	{
		if(is_held)
		{
			uint64_t elapsed_ms = (current_ts - internal_key->ts);

			if(elapsed_ms > DEF_HID_RELEASE_THRESHOLD_MS)
			{
				//No more presses (discard last press), copy state and reset everything.
				key->state = HID_STATE_PRESSED_DONE;
				key->click_count = internal_key->click_count;
				key->held_ms = internal_key->held_ms;

				internal_key->state = HID_INTERNAL_STATE_NONE;
				internal_key->click_count = 0;
				internal_key->held_ms = 0;
				internal_key->ts = 0;
			}
			else
			{
				key->state = HID_STATE_PENDING;
				key->click_count = internal_key->click_count;
				key->held_ms = internal_key->held_ms;
			}
		}
		else if(is_released)
		{
			//The button has been released, wait for release timeout to check if user presses again. 
			internal_key->state = HID_INTERNAL_STATE_RELEASED_WAIT_RELEASE_THRESHOLD;
			if(internal_key->click_count < UINT16_MAX)
				internal_key->click_count++;

			internal_key->held_ms = 0;
			internal_key->ts = current_ts;

			key->state = HID_STATE_PRESSED;
			key->click_count = internal_key->click_count;
			key->held_ms = internal_key->held_ms;
		}
		else
		{
			//Shouldn't happen, reset everything.
			internal_key->state = HID_INTERNAL_STATE_NONE;
			internal_key->click_count = 0;
			internal_key->held_ms = 0;
			internal_key->ts = 0;
		}
	}
}

void Util_hid_scan_hid_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	TickCounter counter = { 0, };

	osTickCounterStart(&counter);
	osTickCounterUpdate(&counter);

	while (util_hid_thread_run)
	{
		uint32_t key_pressed = 0;
		uint32_t key_held = 0;
		uint32_t key_released = 0;
		touchPosition touch_pos = { 0, };
		circlePosition circle_pos = { 0, };

		hidScanInput();
		hidTouchRead(&touch_pos);
		hidCircleRead(&circle_pos);
		key_pressed = hidKeysDown();
		key_held = hidKeysHeld();
		key_released = hidKeysUp();

		Util_sync_lock(&util_hid_data_mutex, UINT64_MAX);

		util_hid_info.ts = osGetTime();
		osTickCounterUpdate(&counter);

		util_hid_info.is_any_pressed = (key_pressed != 0);
		util_hid_info.is_any_held = (key_held != 0);
		util_hid_info.is_any_released = (key_released != 0);

		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_A, a));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_B, b));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_X, x));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_Y, y));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_L, l));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_R, r));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_ZL, zl));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_ZR, zr));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_START, start));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_SELECT, select));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CPAD_UP, c_up));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CPAD_DOWN, c_down));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CPAD_LEFT, c_left));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CPAD_RIGHT, c_right));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_DUP, d_up));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_DDOWN, d_down));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_DLEFT, d_left));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_DRIGHT, d_right));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CSTICK_UP, cs_up));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CSTICK_DOWN, cs_down));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CSTICK_LEFT, cs_left));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_CSTICK_RIGHT, cs_right));
		Util_hid_update_key_state(DEF_HID_UPDATE_STATE_ARG(KEY_TOUCH, touch));

		//Process circle pad position and touch position.
		if(util_hid_info.c_up.was_active || util_hid_info.c_up.is_active
		|| util_hid_info.c_down.was_active || util_hid_info.c_down.is_active
		|| util_hid_info.c_left.was_active || util_hid_info.c_left.is_active
		|| util_hid_info.c_right.was_active || util_hid_info.c_right.is_active)
		{
			if(util_hid_info.c_up.is_active || util_hid_info.c_down.is_active
			|| util_hid_info.c_left.is_active || util_hid_info.c_right.is_active)
			{
				util_hid_info.cpad_x = circle_pos.dx;
				util_hid_info.cpad_y = circle_pos.dy;
			}
		}

		if (util_hid_info.touch.is_active)
		{
			if(util_hid_info.touch_x != -1 && util_hid_info.touch_y != -1)
			{
				util_hid_info.touch_x_move = (util_hid_info.touch_x - touch_pos.px);
				util_hid_info.touch_y_move = (util_hid_info.touch_y - touch_pos.py);
			}

			util_hid_info.touch_x = touch_pos.px;
			util_hid_info.touch_y = touch_pos.py;

			if(!util_hid_info.touch.was_active)
			{
				//Save initial tap position.
				util_hid_info.touch_x_initial = util_hid_info.touch_x;
				util_hid_info.touch_y_initial = util_hid_info.touch_y;
			}
		}
		else if(util_hid_info.touch.was_active)
		{
			//Save last tap position.
			util_hid_info.touch_x_last = util_hid_info.touch_x;
			util_hid_info.touch_y_last = util_hid_info.touch_y;

			util_hid_info.touch_x = -1;
			util_hid_info.touch_y = -1;
			util_hid_info.touch_x_move = 0;
			util_hid_info.touch_y_move = 0;
		}
		else
		{
			//Keep initial tap pos and last tap pos intact.
			util_hid_info.touch_x = -1;
			util_hid_info.touch_y = -1;
			util_hid_info.touch_x_move = 0;
			util_hid_info.touch_y_move = 0;
		}

		//Reset afk time if there are any activities, otherwise increment. 
		if (util_hid_info.is_any_pressed || util_hid_info.is_any_held || util_hid_info.is_any_released)
			util_hid_info.afk_time_ms = 0;
		else
			util_hid_info.afk_time_ms += osTickCounterRead(&counter);

		Util_sync_unlock(&util_hid_data_mutex);

		Util_sync_lock(&util_hid_callback_mutex, UINT64_MAX);

		//Call callback functions.
		for(uint16_t i = 0; i < DEF_HID_NUM_OF_CALLBACKS; i++)
		{
			if(util_hid_callbacks[i])
				util_hid_callbacks[i]();
		}

		Util_sync_unlock(&util_hid_callback_mutex);

		gspWaitForVBlank();
	}
	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}
