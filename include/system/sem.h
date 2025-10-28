#if !defined(DEF_SEM_H)
#define DEF_SEM_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/hid_types.h"

#define DEF_SEM_UPDATE_FILE_PREFIX		/*(const char*)(*/"Vid_"/*)*/
#define DEF_SEM_CHECK_UPDATE_URL		/*(const char*)(*/"https://script.google.com/macros/s/AKfycbwTd3jzV0npUE9MNKmZIv3isazVR5D9_7A8rexsG1vr9SE7iavDBxgtzlph8dZipwu9/exec"/*)*/

#define DEF_SEM_NUM_OF_MSG				(uint16_t)(71)
#define DEF_SEM_ENABLE_ICON
//#define DEF_SEM_ENABLE_NAME
#define DEF_SEM_ICON_PATH				(const char*)"romfs:/gfx/draw/icon/sem_icon.t3x"
#define DEF_SEM_NAME					(const char*)"Settings"

#define DEF_SEM_MENU_TOP				(int8_t)(-1)
#define DEF_SEM_MENU_UPDATE				(int8_t)(0)
#define DEF_SEM_MENU_LANGAGES			(int8_t)(1)
#define DEF_SEM_MENU_LCD				(int8_t)(2)
#define DEF_SEM_MENU_CONTROL			(int8_t)(3)
#define DEF_SEM_MENU_FONT				(int8_t)(4)
#define DEF_SEM_MENU_WIFI				(int8_t)(5)
#define DEF_SEM_MENU_ADVANCED			(int8_t)(6)
#define DEF_SEM_MENU_BATTERY			(int8_t)(7)
#define DEF_SEM_MENU_RECORDING			(int8_t)(8)

#define DEF_SEM_RECORD_BOTH				(uint8_t)(0)
#define DEF_SEM_RECORD_TOP				(uint8_t)(1)
#define DEF_SEM_RECORD_BOTTOM			(uint8_t)(2)

#define DEF_SEM_EDTION_NONE				(int8_t)(-1)
#define DEF_SEM_EDTION_3DSX				(int8_t)(0)
#define DEF_SEM_EDTION_CIA				(int8_t)(1)

typedef uint8_t Sem_screen_mode;
#define DEF_SEM_SCREEN_MODE_AUTO		(Sem_screen_mode)(0)
#define DEF_SEM_SCREEN_MODE_400PX		(Sem_screen_mode)(1)
#define DEF_SEM_SCREEN_MODE_800PX		(Sem_screen_mode)(2)
#define DEF_SEM_SCREEN_MODE_3D			(Sem_screen_mode)(3)
#define DEF_SEM_SCREEN_MODE_MAX			(Sem_screen_mode)(DEF_SEM_SCREEN_MODE_3D + 1)

typedef uint8_t Sem_wifi_signal;
#define DEF_SEM_WIFI_SIGNAL_BAD_WITH_INTERNET			(uint8_t)(0)
#define DEF_SEM_WIFI_SIGNAL_NORMAL_WITH_INTERNET		(uint8_t)(1)
#define DEF_SEM_WIFI_SIGNAL_GOOD_WITH_INTERNET			(uint8_t)(2)
#define DEF_SEM_WIFI_SIGNAL_EXCELLENT_WITH_INTERNET		(uint8_t)(3)
#define DEF_SEM_WIFI_SIGNAL_BAD_WITHOUT_INTERNET		(uint8_t)(4)
#define DEF_SEM_WIFI_SIGNAL_NORMAL_WITHOUT_INTERNET		(uint8_t)(5)
#define DEF_SEM_WIFI_SIGNAL_GOOD_WITHOUT_INTERNET		(uint8_t)(6)
#define DEF_SEM_WIFI_SIGNAL_EXCELLENT_WITHOUT_INTERNET	(uint8_t)(7)
#define DEF_SEM_WIFI_SIGNAL_DISABLED					(uint8_t)(8)

typedef uint8_t Sem_model;
#define DEF_SEM_MODEL_OLD3DS				(Sem_model)(0)
#define DEF_SEM_MODEL_OLD3DSXL				(Sem_model)(1)
#define DEF_SEM_MODEL_OLD2DS				(Sem_model)(2)
#define DEF_SEM_MODEL_NEW3DS				(Sem_model)(3)
#define DEF_SEM_MODEL_NEW3DSXL				(Sem_model)(4)
#define DEF_SEM_MODEL_NEW2DSXL				(Sem_model)(5)
#define DEF_SEM_MODEL_MAX					(Sem_model)(DEF_SEM_MODEL_NEW2DSXL + 1)
#define DEF_SEM_MODEL_IS_NEW(model)			(bool)(model == DEF_SEM_MODEL_NEW3DS || model == DEF_SEM_MODEL_NEW3DSXL || model == DEF_SEM_MODEL_NEW2DSXL)

#define DEF_SEM_UPDATE_MSG					(uint16_t)(0)
#define DEF_SEM_LANGAGES_MSG				(uint16_t)(1)
#define DEF_SEM_LCD_MSG						(uint16_t)(2)
#define DEF_SEM_CONTROL_MSG					(uint16_t)(3)
#define DEF_SEM_FONT_MSG					(uint16_t)(4)
#define DEF_SEM_WIFI_MSG					(uint16_t)(5)
#define DEF_SEM_ADVANCED_MSG				(uint16_t)(6)
#define DEF_SEM_BATTERY_MSG					(uint16_t)(7)
#define DEF_SEM_CHECK_UPDATE_MSG			(uint16_t)(8)
#define DEF_SEM_ENGLISH_MSG					(uint16_t)(9)
#define DEF_SEM_JAPANESE_MSG				(uint16_t)(10)
#define DEF_SEM_NIGHT_MODE_MSG				(uint16_t)(11)
#define DEF_SEM_ON_MSG						(uint16_t)(12)
#define DEF_SEM_OFF_MSG						(uint16_t)(13)
#define DEF_SEM_FLASH_MSG					(uint16_t)(14)
#define DEF_SEM_BRIGHTNESS_MSG				(uint16_t)(15)
#define DEF_SEM_LCD_OFF_TIME_0_MSG			(uint16_t)(16)
#define DEF_SEM_LCD_OFF_TIME_1_MSG			(uint16_t)(17)
#define DEF_SEM_SCROLL_SPEED_MSG			(uint16_t)(18)
#define DEF_SEM_JAPANESE_FONT_MSG			(uint16_t)(19)
#define DEF_SEM_CHINESE_FONT_MSG			(uint16_t)(20)
#define DEF_SEM_KOREAN_FONT_MSG				(uint16_t)(21)
#define DEF_SEM_TAIWANESE_FONT_MSG			(uint16_t)(22)
#define DEF_SEM_LOAD_ALL_FONT_MSG			(uint16_t)(23)
#define DEF_SEM_UNLOAD_ALL_FONT_MSG			(uint16_t)(24)
#define DEF_SEM_SEND_INFO_MODE_MSG			(uint16_t)(25)
#define DEF_SEM_ALLOW_MSG					(uint16_t)(26)
#define DEF_SEM_DENY_MSG					(uint16_t)(27)
#define DEF_SEM_DEBUG_MODE_MSG				(uint16_t)(28)
#define DEF_SEM_ECO_MODE_MSG				(uint16_t)(29)
#define DEF_SEM_BACK_MSG					(uint16_t)(30)
#define DEF_SEM_CHECKING_UPDATE_MSG			(uint16_t)(31)
#define DEF_SEM_CHECKING_UPDATE_FAILED_MSG	(uint16_t)(32)
#define DEF_SEM_UP_TO_DATE_MSG				(uint16_t)(33)
#define DEF_SEM_NEW_VERSION_AVAILABLE_MSG	(uint16_t)(34)
#define DEF_SEM_CLOSE_UPDATER_MSG			(uint16_t)(35)
#define DEF_SEM_BACK_TO_PATCH_NOTE_MSG		(uint16_t)(35)	//Same as CLOSE_UPDATER_MSG.
#define DEF_SEM_SELECT_EDITION_MSG			(uint16_t)(36)
#define DEF_SEM_3DSX_MSG					(uint16_t)(37)
#define DEF_SEM_CIA_MSG						(uint16_t)(38)
#define DEF_SEM_FILE_PATH_MSG				(uint16_t)(39)
#define DEF_SEM_DOWNLOADING_MSG				(uint16_t)(40)
#define DEF_SEM_INSTALLING_MSG				(uint16_t)(41)
#define DEF_SEM_SUCCESS_MSG					(uint16_t)(42)
#define DEF_SEM_FAILURE_MSG					(uint16_t)(43)
#define DEF_SEM_RESTART_MSG					(uint16_t)(44)
#define DEF_SEM_DL_INSTALL_MSG				(uint16_t)(45)
#define DEF_SEM_CLOSE_APP_MSG				(uint16_t)(46)
#define DEF_SEM_WIFI_MODE_MSG				(uint16_t)(47)
#define DEF_SEM_CONNECTED_SSID_MSG			(uint16_t)(48)
#define DEF_SEM_RECORDING_MSG				(uint16_t)(49)
#define DEF_SEM_RECORD_BOTH_LCD_MSG			(uint16_t)(50)
#define DEF_SEM_RECORD_TOP_LCD_MSG			(uint16_t)(51)
#define DEF_SEM_RECORD_BOTTOM_LCD_MSG		(uint16_t)(52)
#define DEF_SEM_STOP_RECORDING_MSG			(uint16_t)(53)
#define DEF_SEM_LCD_MODE_MSG				(uint16_t)(54)
#define DEF_SEM_CANNOT_RECORD_MSG			(uint16_t)(55)
#define DEF_SEM_800PX_MSG					(uint16_t)(56)
#define DEF_SEM_3D_MSG						(uint16_t)(57)
#define DEF_SEM_400PX_MSG					(uint16_t)(58)
#define DEF_SEM_HUNGARIAN_MSG				(uint16_t)(59)
#define DEF_SEM_CHINESE_MSG					(uint16_t)(60)
#define DEF_SEM_ITALIAN_MSG					(uint16_t)(61)
#define DEF_SEM_FAKE_MODEL_MSG				(uint16_t)(62)
#define DEF_SEM_SPANISH_MSG					(uint16_t)(63)
#define DEF_SEM_ROMANIAN_MSG				(uint16_t)(64)
#define DEF_SEM_POLISH_MSG					(uint16_t)(65)
#define DEF_SEM_CPU_USAGE_MONITOR_MSG		(uint16_t)(66)
#define DEF_SEM_DUMP_LOGS_MSG				(uint16_t)(67)
#define DEF_SEM_RYUKYUAN_MSG				(uint16_t)(68)
#define DEF_SEM_AUTO_MSG					(uint16_t)(69)
#define DEF_SEM_SLEEP_TIME_MSG				(uint16_t)(70)

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
