#if !defined(DEF_WATCH_TYPES_H)
#define DEF_WATCH_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/log_enum_types.h"

#define DEF_WATCH_MAX_VARIABLES		(uint32_t)(512)

typedef enum
{
	WATCH_HANDLE_INVALID = -1,

	WATCH_HANDLE_GLOBAL,			//Watch handle for global data.
	WATCH_HANDLE_ERR,				//(err.c) Watch handle for error API.
	WATCH_HANDLE_EXPL,				//(expl.c) Watch handle for file explorer API.
	WATCH_HANDLE_LOG,				//(log.c) Watch handle for log API.
	WATCH_HANDLE_MAIN_MENU,			//(menu.c) Watch handle for main menu.
	WATCH_HANDLE_SETTINGS_MENU,		//(setting_menu.c) Watch handle for settings menu.
	WATCH_HANDLE_VIDEO_PLAYER,		//(video_player.cpp) Watch handle for video player.

	WATCH_HANDLE_MAX,
	WATCH_HANDLE_FORCE_8BIT = INT8_MAX,
} Watch_handle;

DEF_LOG_ENUM_DEBUG
(
	Watch_handle,
	WATCH_HANDLE_INVALID,
	WATCH_HANDLE_GLOBAL,
	WATCH_HANDLE_ERR,
	WATCH_HANDLE_EXPL,
	WATCH_HANDLE_LOG,
	WATCH_HANDLE_MAIN_MENU,
	WATCH_HANDLE_SETTINGS_MENU,
	WATCH_HANDLE_VIDEO_PLAYER,
	WATCH_HANDLE_MAX,
	WATCH_HANDLE_FORCE_8BIT
)

typedef uint16_t Watch_handle_bit;
#define	DEF_WATCH_HANDLE_BIT_NONE			(Watch_handle_bit)(0 << 0)							//No watch handles.
#define	DEF_WATCH_HANDLE_BIT_GLOBAL			(Watch_handle_bit)(1 << WATCH_HANDLE_GLOBAL)		//Watch handle bit for WATCH_HANDLE_GLOBAL.
#define	DEF_WATCH_HANDLE_BIT_ERR			(Watch_handle_bit)(1 << WATCH_HANDLE_ERR)			//Watch handle bit for WATCH_HANDLE_ERR.
#define	DEF_WATCH_HANDLE_BIT_EXPL			(Watch_handle_bit)(1 << WATCH_HANDLE_EXPL)			//Watch handle bit for WATCH_HANDLE_EXPL.
#define	DEF_WATCH_HANDLE_BIT_LOG			(Watch_handle_bit)(1 << WATCH_HANDLE_LOG)			//Watch handle bit for WATCH_HANDLE_LOG.
#define	DEF_WATCH_HANDLE_BIT_MAIN_MENU		(Watch_handle_bit)(1 << WATCH_HANDLE_MAIN_MENU)		//Watch handle bit for WATCH_HANDLE_MAIN_MENU.
#define	DEF_WATCH_HANDLE_BIT_SETTINGS_MENU	(Watch_handle_bit)(1 << WATCH_HANDLE_SETTINGS_MENU)	//Watch handle bit for WATCH_HANDLE_SETTINGS_MENU.
#define	DEF_WATCH_HANDLE_BIT_VIDEO_PLAYER	(Watch_handle_bit)(1 << WATCH_HANDLE_VIDEO_PLAYER)	//Watch handle bit for WATCH_HANDLE_VIDEO_PLAYER.

typedef struct
{
	const void* original_address;	//Original data address.
	void* previous_data;			//Previous data.
	uint32_t data_length;			//Data length for this data.
	Watch_handle handle;			//Watch handle.
} Watch_data;

#endif //!defined(DEF_WATCH_TYPES_H)
