#if !defined(DEF_HID_TYPES_H)
#define DEF_HID_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/log_enum_types.h"

#define DEF_HID_NUM_OF_CALLBACKS			(uint16_t)(4)
#define DEF_HID_HELD_THRESHOLD_MS			(uint32_t)(200)
#define DEF_HID_RELEASE_THRESHOLD_MS		(uint32_t)(200)

typedef uint32_t Hid_key_bit;
#define HID_KEY_BIT_NONE		(Hid_key_bit)(0 <<  0)
#define HID_KEY_BIT_A			(Hid_key_bit)(1 <<  0)
#define HID_KEY_BIT_B			(Hid_key_bit)(1 <<  1)
#define HID_KEY_BIT_X			(Hid_key_bit)(1 <<  2)
#define HID_KEY_BIT_Y			(Hid_key_bit)(1 <<  3)
#define HID_KEY_BIT_L			(Hid_key_bit)(1 <<  4)
#define HID_KEY_BIT_R			(Hid_key_bit)(1 <<  5)
#define HID_KEY_BIT_ZL			(Hid_key_bit)(1 <<  6)
#define HID_KEY_BIT_ZR			(Hid_key_bit)(1 <<  7)
#define HID_KEY_BIT_START		(Hid_key_bit)(1 <<  8)
#define HID_KEY_BIT_SELECT		(Hid_key_bit)(1 <<  9)
#define HID_KEY_BIT_C_UP		(Hid_key_bit)(1 << 10)
#define HID_KEY_BIT_C_DOWN		(Hid_key_bit)(1 << 11)
#define HID_KEY_BIT_C_LEFT		(Hid_key_bit)(1 << 12)
#define HID_KEY_BIT_C_RIGHT		(Hid_key_bit)(1 << 13)
#define HID_KEY_BIT_D_UP		(Hid_key_bit)(1 << 14)
#define HID_KEY_BIT_D_DOWN		(Hid_key_bit)(1 << 15)
#define HID_KEY_BIT_D_LEFT		(Hid_key_bit)(1 << 16)
#define HID_KEY_BIT_D_RIGHT		(Hid_key_bit)(1 << 17)
#define HID_KEY_BIT_CS_UP		(Hid_key_bit)(1 << 18)
#define HID_KEY_BIT_CS_DOWN		(Hid_key_bit)(1 << 19)
#define HID_KEY_BIT_CS_LEFT		(Hid_key_bit)(1 << 20)
#define HID_KEY_BIT_CS_RIGHT	(Hid_key_bit)(1 << 21)
#define HID_KEY_BIT_TOUCH		(Hid_key_bit)(1 << 22)
#define HID_KEY_BIT_ALL			(Hid_key_bit)(HID_KEY_BIT_A | HID_KEY_BIT_B | HID_KEY_BIT_X | HID_KEY_BIT_Y \
| HID_KEY_BIT_L | HID_KEY_BIT_R | HID_KEY_BIT_ZL | HID_KEY_BIT_ZR | HID_KEY_BIT_START | HID_KEY_BIT_SELECT \
| HID_KEY_BIT_C_UP | HID_KEY_BIT_C_DOWN | HID_KEY_BIT_C_LEFT | HID_KEY_BIT_C_RIGHT | HID_KEY_BIT_D_UP \
| HID_KEY_BIT_D_DOWN | HID_KEY_BIT_D_LEFT | HID_KEY_BIT_D_RIGHT | HID_KEY_BIT_CS_UP | HID_KEY_BIT_CS_DOWN \
| HID_KEY_BIT_CS_LEFT | HID_KEY_BIT_CS_RIGHT | HID_KEY_BIT_TOUCH)

#define DEF_HID_PHY_NP(key)				(bool)(!key.was_active && !key.is_active)	//Whether key is **physically** NOT pressed.
#define DEF_HID_PHY_PR(key)				(bool)(!key.was_active && key.is_active)	//Whether key is **physically** pressed.
#define DEF_HID_PHY_HE(key)				(bool)(key.was_active && key.is_active)		//Whether key is **physically** being held.
#define DEF_HID_PHY_RE(key)				(bool)(key.was_active && !key.is_active)	//Whether key is **physically** released.

#define DEF_HID_NP(key)					(bool)(key.state == HID_STATE_NOT_PRESSED)	//Whether key state is NOT_PRESSED.
#define DEF_HID_PE(key)					(bool)(key.state == HID_STATE_PENDING)		//Whether key state is PENDING.
#define DEF_HID_PR(key)					(bool)(key.state == HID_STATE_PRESSED)		//Whether key state is PRESSED.
#define DEF_HID_PD(key)					(bool)(key.state == HID_STATE_PRESSED_DONE)	//Whether key state is PRESSED_DONE.
#define DEF_HID_HE(key)					(bool)(key.state == HID_STATE_HELD)			//Whether key state is HELD.
#define DEF_HID_HD(key)					(bool)(key.state == HID_STATE_HELD_DONE)	//Whether key state is HELD_DONE.

#define DEF_HID_PR_LT(key, count)		(bool)(DEF_HID_PR(key) && (key.click_count < count))	//Whether key state is PRESSED and pressed for less than specified times.
#define DEF_HID_PR_EL(key, count)		(bool)(DEF_HID_PR(key) && (key.click_count <= count))	//Whether key state is PRESSED and pressed for equals to or less than specified times.
#define DEF_HID_PR_EQ(key, count)		(bool)(DEF_HID_PR(key) && (key.click_count == count))	//Whether key state is PRESSED and pressed for equals to specified times.
#define DEF_HID_PR_EM(key, count)		(bool)(DEF_HID_PR(key) && (key.click_count >= count))	//Whether key state is PRESSED and pressed for equals to or more than specified times.
#define DEF_HID_PR_MT(key, count)		(bool)(DEF_HID_PR(key) && (key.click_count > count))	//Whether key state is PRESSED and pressed for more than specified times.

#define DEF_HID_PD_LT(key, count)		(bool)(DEF_HID_PD(key) && (key.click_count < count))	//Whether key state is PRESSED_DONE and pressed for less than specified times.
#define DEF_HID_PD_EL(key, count)		(bool)(DEF_HID_PD(key) && (key.click_count <= count))	//Whether key state is PRESSED_DONE and pressed for equals to or less than specified times.
#define DEF_HID_PD_EQ(key, count)		(bool)(DEF_HID_PD(key) && (key.click_count == count))	//Whether key state is PRESSED_DONE and pressed for equals to specified times.
#define DEF_HID_PD_EM(key, count)		(bool)(DEF_HID_PD(key) && (key.click_count >= count))	//Whether key state is PRESSED_DONE and pressed for equals to or more than specified times.
#define DEF_HID_PD_MT(key, count)		(bool)(DEF_HID_PD(key) && (key.click_count > count))	//Whether key state is PRESSED_DONE and pressed for more than specified times.

#define DEF_HID_HE_LT(key, time_ms)		(bool)(DEF_HID_HE(key) && (key.held_ms < time_ms))		//Whether key state is HELD and held for less than specified duration in ms.
#define DEF_HID_HE_EL(key, time_ms)		(bool)(DEF_HID_HE(key) && (key.held_ms <= time_ms))		//Whether key state is HELD and held for equals to or less than specified duration in ms.
#define DEF_HID_HE_EQ(key, time_ms)		(bool)(DEF_HID_HE(key) && (key.held_ms == time_ms))		//Whether key state is HELD and held for equals to specified duration in ms.
#define DEF_HID_HE_EM(key, time_ms)		(bool)(DEF_HID_HE(key) && (key.held_ms >= time_ms))		//Whether key state is HELD and held for equals to or more than specified duration in ms.
#define DEF_HID_HE_MT(key, time_ms)		(bool)(DEF_HID_HE(key) && (key.held_ms > time_ms))		//Whether key state is HELD and held for more than specified duration in ms.

#define DEF_HID_HD_LT(key, time_ms)		(bool)(DEF_HID_HD(key) && (key.held_ms < time_ms))		//Whether key state is HELD_DONE and held for less than specified duration in ms.
#define DEF_HID_HD_EL(key, time_ms)		(bool)(DEF_HID_HD(key) && (key.held_ms <= time_ms))		//Whether key state is HELD_DONE and held for equals to or less than specified duration in ms.
#define DEF_HID_HD_EQ(key, time_ms)		(bool)(DEF_HID_HD(key) && (key.held_ms == time_ms))		//Whether key state is HELD_DONE and held for equals to specified duration in ms.
#define DEF_HID_HD_EM(key, time_ms)		(bool)(DEF_HID_HD(key) && (key.held_ms >= time_ms))		//Whether key state is HELD_DONE and held for equals to or more than specified duration in ms.
#define DEF_HID_HD_MT(key, time_ms)		(bool)(DEF_HID_HD(key) && (key.held_ms > time_ms))		//Whether key state is HELD_DONE and held for more than specified duration in ms.

#define DEF_HID_HE_NEW_INTERVAL(key, interval, is_new_interval) \
{ \
	static uint32_t last_range = 0; \
	static uint32_t last_interval = 0; \
	\
	is_new_interval = false; \
	if(last_interval != interval) \
	{ \
		/* Reset last_range if interval has been changed. */ \
		last_interval = interval; \
		last_range = 0; \
	} \
	\
	if(interval != 0 && DEF_HID_HE(key)) \
	{ \
		uint32_t range = (key.held_ms / interval); \
		is_new_interval = (range != last_range); \
		last_range = range; \
	} \
	else \
		last_range = 0; \
}

//Whether texture position and touch position are overlapping.
#define DEF_HID_IN(tex_x, tex_x_end, tex_y, tex_y_end, touch_x, touch_y)	(bool)((touch_x >= tex_x) && (touch_x <= tex_x_end) && (touch_y >= tex_y) && (touch_y <= tex_y_end))

//Whether texture position and initial touch position are overlapping.
#define DEF_HID_INIT_IN(img, key)		(bool)(DEF_HID_IN(img.x, (img.x + img.x_size - 1), img.y, (img.y + img.y_size - 1), key.touch_x_initial, key.touch_y_initial))

//Whether texture position and current touch position are overlapping.
#define DEF_HID_CUR_IN(img, key)		(bool)(DEF_HID_IN(img.x, (img.x + img.x_size - 1), img.y, (img.y + img.y_size - 1), key.touch_x, key.touch_y))

//Whether texture position and last touch position are overlapping.
#define DEF_HID_LAST_IN(img, key)		(bool)(DEF_HID_IN(img.x, (img.x + img.x_size - 1), img.y, (img.y + img.y_size - 1), key.touch_x_last, key.touch_y_last))

//Whether texture position, initial touch position and current touch position are overlapping.
#define DEF_HID_INIT_CUR_IN(img, key)	(bool)(DEF_HID_INIT_IN(img, key) && DEF_HID_CUR_IN(img, key))

//Whether texture position, initial touch position and last touch position are overlapping.
#define DEF_HID_INIT_LAST_IN(img, key)	(bool)(DEF_HID_INIT_IN(img, key) && DEF_HID_LAST_IN(img, key))

typedef enum
{
	HID_STATE_INVALID = -1,

	HID_STATE_NOT_PRESSED,	//Nothing.
	HID_STATE_PENDING,		//Button has some activities, but there are no enough information to decide which state.
	HID_STATE_PRESSED,		//Button was pressed and released, may be pressed more.
	HID_STATE_PRESSED_DONE,	//Button was pressed and released, no more presses.
	HID_STATE_HELD,			//Button is being held.
	HID_STATE_HELD_DONE,	//Button was being held and released.

	HID_STATE_MAX,
} Hid_state;

DEF_LOG_ENUM_DEBUG
(
	Hid_state,
	HID_STATE_INVALID,
	HID_STATE_NOT_PRESSED,
	HID_STATE_PRESSED,
	HID_STATE_PRESSED_DONE,
	HID_STATE_HELD,
	HID_STATE_HELD_DONE,
	HID_STATE_PENDING,
	HID_STATE_MAX
)

typedef struct
{
	bool was_active;		//Whether button was physically pressed on previous event.
	bool is_active;			//Whether button is physically pressed now, this may not match with the current "state".
	uint32_t held_ms;		//Held time in ms.
	uint16_t click_count;	//Number of clicks/taps.
	Hid_state state;		//Button state.
} Hid_key;

typedef struct
{
	bool is_any_pressed;	//Whether at least 1 key is PRESSED state.
	bool is_any_held;		//Whether at least 1 key is HELD state.
	bool is_any_released;	//Whether at least 1 key is RELEASED state.
	Hid_key a;
	Hid_key b;
	Hid_key x;
	Hid_key y;
	Hid_key l;
	Hid_key r;
	Hid_key zl;
	Hid_key zr;
	Hid_key start;
	Hid_key select;
	Hid_key c_up;
	Hid_key c_down;
	Hid_key c_left;
	Hid_key c_right;
	Hid_key d_up;
	Hid_key d_down;
	Hid_key d_left;
	Hid_key d_right;
	Hid_key cs_up;
	Hid_key cs_down;
	Hid_key cs_left;
	Hid_key cs_right;
	Hid_key touch;
	//CPAD and touch position.
	int16_t cpad_x;
	int16_t cpad_y;
	int16_t touch_x_initial;
	int16_t touch_y_initial;
	int16_t touch_x;
	int16_t touch_y;
	int16_t touch_x_last;
	int16_t touch_y_last;
	int16_t touch_x_move;
	int16_t touch_y_move;
	double afk_time_ms;		//Afk (i.e. no activities) time in ms.
	uint64_t ts;			//Timestamp for this data.
} Hid_info;

#endif //!defined(DEF_HID_TYPES_H)
