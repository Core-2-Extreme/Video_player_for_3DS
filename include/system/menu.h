#if !defined(DEF_MENU_H)
#define DEF_MENU_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_MENU_MAIN_DIR				/*(const char*)(*/"/3ds/Video_player/"/*)*/
#define DEF_MENU_APP_INFO				/*(const char*)(*/"Video_player_for_3ds/" DEF_MENU_CURRENT_APP_VER/*)*/
#define DEF_MENU_CURRENT_APP_VER		/*(const char*)(*/"1.6.0"/*)*/
#define DEF_MENU_CHECK_INTERNET_URL		/*(const char*)(*/"http://connectivitycheck.gstatic.com/generate_204"/*)*/
#define DEF_MENU_SEND_APP_INFO_URL		/*(const char*)(*/"https://script.google.com/macros/s/AKfycbyn_blFyKWXCgJr6NIF8x6ETs7CHRN5FXKYEAAIrzV6jPYcCkI/exec"/*)*/
#define DEF_MENU_CURRENT_APP_VER_INT	(uint32_t)(1120)

#define DEF_MENU_NUM_OF_CALLBACKS		(uint16_t)(32)
#define DEF_MENU_SOCKET_BUFFER_SIZE		(uint32_t)(0x40000)
#define DEF_MENU_HTTP_POST_BUFFER_SIZE	(uint32_t)(0x80000)

#define DEF_MENU_NUM_OF_MSG				(uint16_t)(5)
#define DEF_MENU_EXIST_MSG				(uint16_t)(0)
#define DEF_MENU_CONFIRM_MSG			(uint16_t)(1)
#define DEF_MENU_CANCEL_MSG				(uint16_t)(2)
#define DEF_MENU_NEW_VERSION_MSG		(uint16_t)(3)
#define DEF_MENU_HOW_TO_UPDATE_MSG		(uint16_t)(4)

bool Menu_query_must_exit_flag(void);

void Menu_set_must_exit_flag(bool flag);

void Menu_resume(void);

void Menu_suspend(void);

uint32_t Menu_load_msg(const char* lang);

void Menu_init(void);

bool Menu_add_worker_thread_callback(void (*const callback)(void));

void Menu_remove_worker_thread_callback(void (*const callback)(void));

void Menu_exit(void);

void Menu_main(void);

#endif //!defined(DEF_MENU_H)
