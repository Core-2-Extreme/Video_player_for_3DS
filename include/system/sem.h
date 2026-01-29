#if !defined(DEF_SEM_H)
#define DEF_SEM_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/hid_types.h"

#define DEF_SEM_CHECK_UPDATE_URL		/*(const char*)(*/"https://script.google.com/macros/s/AKfycbwTd3jzV0npUE9MNKmZIv3isazVR5D9_7A8rexsG1vr9SE7iavDBxgtzlph8dZipwu9/exec"/*)*/

#define DEF_SEM_ENABLE_ICON
//#define DEF_SEM_ENABLE_NAME
#define DEF_SEM_ICON_PATH				(const char*)"romfs:/gfx/draw/icon/sem_icon.t3x"
#define DEF_SEM_NAME					(const char*)"Settings"

typedef uint8_t Sem_screen_mode;
#define DEF_SEM_SCREEN_MODE_AUTO		(Sem_screen_mode)(0)
#define DEF_SEM_SCREEN_MODE_400PX		(Sem_screen_mode)(1)
#define DEF_SEM_SCREEN_MODE_800PX		(Sem_screen_mode)(2)
#define DEF_SEM_SCREEN_MODE_3D			(Sem_screen_mode)(3)
#define DEF_SEM_SCREEN_MODE_MAX			(Sem_screen_mode)(DEF_SEM_SCREEN_MODE_3D + 1)

typedef uint8_t Sem_wifi_signal;
#define DEF_SEM_WIFI_SIGNAL_BAD_WITH_INTERNET			(Sem_wifi_signal)(0)
#define DEF_SEM_WIFI_SIGNAL_NORMAL_WITH_INTERNET		(Sem_wifi_signal)(1)
#define DEF_SEM_WIFI_SIGNAL_GOOD_WITH_INTERNET			(Sem_wifi_signal)(2)
#define DEF_SEM_WIFI_SIGNAL_EXCELLENT_WITH_INTERNET		(Sem_wifi_signal)(3)
#define DEF_SEM_WIFI_SIGNAL_BAD_WITHOUT_INTERNET		(Sem_wifi_signal)(4)
#define DEF_SEM_WIFI_SIGNAL_NORMAL_WITHOUT_INTERNET		(Sem_wifi_signal)(5)
#define DEF_SEM_WIFI_SIGNAL_GOOD_WITHOUT_INTERNET		(Sem_wifi_signal)(6)
#define DEF_SEM_WIFI_SIGNAL_EXCELLENT_WITHOUT_INTERNET	(Sem_wifi_signal)(7)
#define DEF_SEM_WIFI_SIGNAL_DISABLED					(Sem_wifi_signal)(8)

typedef uint8_t Sem_model;
#define DEF_SEM_MODEL_OLD3DS				(Sem_model)(0)
#define DEF_SEM_MODEL_OLD3DSXL				(Sem_model)(1)
#define DEF_SEM_MODEL_OLD2DS				(Sem_model)(2)
#define DEF_SEM_MODEL_NEW3DS				(Sem_model)(3)
#define DEF_SEM_MODEL_NEW3DSXL				(Sem_model)(4)
#define DEF_SEM_MODEL_NEW2DSXL				(Sem_model)(5)
#define DEF_SEM_MODEL_MAX					(Sem_model)(DEF_SEM_MODEL_NEW2DSXL + 1)
#define DEF_SEM_MODEL_IS_NEW(model)			(bool)(model == DEF_SEM_MODEL_NEW3DS || model == DEF_SEM_MODEL_NEW3DSXL || model == DEF_SEM_MODEL_NEW2DSXL)

//You need to enable DEF_CONVERTER_SW_API_ENABLE **and** DEF_ENCODER_VIDEO_AUDIO_API_ENABLE as well to use screen recorder.
#define DEF_SEM_ENABLE_SCREEN_RECORDER		/*(bool)(*/false/*)*/
//You need to enable DEF_HTTPC_API_ENABLE **or** DEF_CURL_API_ENABLE as well to use updater.
#define DEF_SEM_ENABLE_UPDATER				/*(bool)(*/true/*)*/

typedef struct
{
	bool is_debug;					//Whether debug mode is enabled.
	bool is_send_info_allowed;		//Whether send application information is allowed.
	bool is_night;					//Whether night mode is enabled.
	bool is_flash;					//Whether flash mode is enabled.
	bool is_eco;					//Whether eco mode is enabled.
	bool is_wifi_on;				//Whether Wifi is enabled.
	bool is_top_lcd_on;				//Whether top is enabled.
	bool is_bottom_lcd_on;			//Whether bottom is enabled.
	uint8_t top_lcd_brightness;		//Brightness for top LCD.
	uint8_t bottom_lcd_brightness;	//Brightness for bottom LCD.
	uint16_t time_to_turn_off_lcd;	//Screen timeout in seconds, 0 to disable it.
	uint16_t time_to_enter_sleep;	//Sleep timeout in seconds, 0 to disable it.
	double scroll_speed;			//Scroll sensitivity.
	char lang[8];					//Language.
	Sem_screen_mode screen_mode;	//Current screen mode.
} Sem_config;

typedef struct
{
	uint16_t years;			//Year, e.g. 2011.
	uint8_t months;			//Month, e.g. 2.
	uint8_t days;			//Day, e.g. 26.
	uint8_t hours;			//Hour, e.g. 23.
	uint8_t minutes;		//Minute, e.g. 59.
	uint8_t seconds;		//Second, e.g. 59.
} Sem_time;

typedef struct
{
	bool is_charging;				//Whether charger is active.
	uint8_t battery_level;			//Battery level in %.
	uint8_t battery_temp;			//Battery temperature in degrees celsius (℃).
	uint32_t free_ram;				//Free heap in bytes.
	uint32_t free_linear_ram;		//Free linear RAM in bytes.
	uint32_t num_of_launch;			//Number of application launches.
	double battery_voltage;			//Battery voltage in volts.
	char connected_wifi[33];		//Connected network (access point) name (empty string if not connected).
	char msg[128];					//Preformatted status message.
	Sem_wifi_signal wifi_signal;	//Wifi signal strength and whether connected to the Internet.
	Sem_model console_model;		//Console model ID.
	Sem_time time;					//The time.
} Sem_state;

bool Sem_query_init_flag(void);

bool Sem_query_running_flag(void);

void Sem_resume(void);

void Sem_suspend(void);

uint32_t Sem_load_msg(const char* lang);

void Sem_get_config(Sem_config* config);

void Sem_set_config(Sem_config* new_config);

void Sem_get_state(Sem_state* state);

void Sem_init(void);

void Sem_draw_init(void);

void Sem_exit(void);

void Sem_main(void);

void Sem_hid(const Hid_info* key);

#endif //!defined(DEF_SEM_H)
