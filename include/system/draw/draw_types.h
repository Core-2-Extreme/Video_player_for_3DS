#if !defined(DEF_DRAW_TYPES_H)
#define DEF_DRAW_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include "citro2d.h"
#include "citro3d.h"
#include "system/util/log_enum_types.h"

#define DEF_DRAW_DMA_ENABLE					/*(bool)(*/true/*)*/	//Enable DMA in draw module for faster processing.
#define DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS	(uint32_t)(128)
#define DEF_DRAW_MAX_TEXTURE_SIZE			(uint16_t)(1024)

#define DEF_DRAW_RED						(uint32_t)(0xFF0000FF)
#define DEF_DRAW_GREEN						(uint32_t)(0xFF00FF00)
#define DEF_DRAW_BLUE						(uint32_t)(0xFFFF0000)
#define DEF_DRAW_BLACK						(uint32_t)(0xFF000000)
#define DEF_DRAW_WHITE						(uint32_t)(0xFFFFFFFF)
#define DEF_DRAW_AQUA						(uint32_t)(0xFFFFFF00)
#define DEF_DRAW_YELLOW						(uint32_t)(0xFF00C5FF)
#define DEF_DRAW_WEAK_RED					(uint32_t)(0x500000FF)
#define DEF_DRAW_WEAK_GREEN					(uint32_t)(0x5000FF00)
#define DEF_DRAW_WEAK_BLUE					(uint32_t)(0x50FF0000)
#define DEF_DRAW_WEAK_BLACK					(uint32_t)(0x50000000)
#define DEF_DRAW_WEAK_WHITE					(uint32_t)(0x50FFFFFF)
#define DEF_DRAW_WEAK_AQUA					(uint32_t)(0x50FFFF00)
#define DEF_DRAW_WEAK_YELLOW				(uint32_t)(0x5000C5FF)
#define DEF_DRAW_NO_COLOR					(uint32_t)(0x00000000)

#define DEF_DRAW_NORMAL_SCALE_AND_SPACE		1.0, 1.0, 0.04, 0.01

typedef enum
{
	DRAW_SCREEN_INVALID	= -1,

	DRAW_SCREEN_TOP_LEFT,	//Top screen for left eye.
	DRAW_SCREEN_BOTTOM,		//Bottom screen.
	DRAW_SCREEN_TOP_RIGHT,	//Top screen for right eye, this is used when 3D mode is enabled.

	DRAW_SCREEN_MAX,
} Draw_screen;

DEF_LOG_ENUM_DEBUG
(
	Draw_screen,
	DRAW_SCREEN_INVALID,
	DRAW_SCREEN_TOP_LEFT,
	DRAW_SCREEN_BOTTOM,
	DRAW_SCREEN_TOP_RIGHT,
	DRAW_SCREEN_MAX
)

typedef enum
{
	DRAW_X_ALIGN_INVALID = -1,

	DRAW_X_ALIGN_LEFT,		//Align text left (default).
	DRAW_X_ALIGN_CENTER,	//Align text center.
	DRAW_X_ALIGN_RIGHT,		//Align text right.

	DRAW_X_ALIGN_MAX,
} Draw_text_align_x;

DEF_LOG_ENUM_DEBUG
(
	Draw_text_align_x,
	DRAW_X_ALIGN_INVALID,
	DRAW_X_ALIGN_LEFT,
	DRAW_X_ALIGN_CENTER,
	DRAW_X_ALIGN_RIGHT,
	DRAW_X_ALIGN_MAX
)

typedef enum
{
	DRAW_Y_ALIGN_INVALID = -1,

	DRAW_Y_ALIGN_TOP,		//Align text top (default).
	DRAW_Y_ALIGN_CENTER,	//Align text center.
	DRAW_Y_ALIGN_BOTTOM,	//Align text bottom.

	DRAW_Y_ALIGN_MAX,
} Draw_text_align_y;

DEF_LOG_ENUM_DEBUG
(
	Draw_text_align_y,
	DRAW_Y_ALIGN_INVALID,
	DRAW_Y_ALIGN_TOP,
	DRAW_Y_ALIGN_CENTER,
	DRAW_Y_ALIGN_BOTTOM,
	DRAW_Y_ALIGN_MAX
)

typedef enum
{
	DRAW_BACKGROUND_INVALID = -1,

	DRAW_BACKGROUND_NONE,			//No background texture (default).
	DRAW_BACKGROUND_ENTIRE_BOX,		//Draw background texture entire box.
	DRAW_BACKGROUND_UNDER_TEXT,		//Only draw background texture under text.

	DRAW_BACKGROUND_MAX,
} Draw_background;

DEF_LOG_ENUM_DEBUG
(
	Draw_background,
	DRAW_BACKGROUND_INVALID,
	DRAW_BACKGROUND_NONE,
	DRAW_BACKGROUND_ENTIRE_BOX,
	DRAW_BACKGROUND_UNDER_TEXT,
	DRAW_BACKGROUND_MAX
)

typedef struct
{
	C2D_Image c2d;				//Texture data.
	Tex3DS_SubTexture* subtex;	//Subtexture data.
	bool selected;				//Whether this texture is selected.
	double x;					//X (horizontal) position.
	double y;					//Y (vertical) position.
	double x_size;				//Texture drawn width.
	double y_size;				//Texture drawn height.
} Draw_image_data;

#endif //!defined(DEF_DRAW_TYPES_H)
