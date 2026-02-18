#if !defined(DEF_MENU_H)
#define DEF_MENU_H
#include <stdbool.h>
#include <stdint.h>

#define DEF_MENU_MAIN_DIR				/*(const char*)(*/"/3ds/Video_player/"/*)*/
#define DEF_MENU_CURRENT_APP_VER		/*(const char*)(*/"1.6.2 (nightly build)"/*)*/
#define DEF_MENU_CURRENT_APP_VER_INT	(uint32_t)(1)

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
