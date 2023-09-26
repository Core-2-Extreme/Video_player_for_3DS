/**
 * @file sprite.h
 * @brief Stateful sprite API
 */
#pragma once
#include "spritesheet.h"

typedef struct
{
	C2D_Image image;
	C2D_DrawParams params;
} C2D_Sprite;

/** @defgroup Sprite Sprite functions
 *  @{
 */

/** @brief Initializes a sprite from an image
 *  @param[in] Pointer to sprite
 *  @param[in] image Image to use
 */
static inline void C2D_SpriteFromImage(C2D_Sprite* sprite, C2D_Image image)
{
	sprite->image           = image;
	sprite->params.pos.x    = 0.0f;
	sprite->params.pos.y    = 0.0f;
	sprite->params.pos.w    = image.subtex->width;
	sprite->params.pos.h    = image.subtex->height;
	sprite->params.center.x = 0.0f;
	sprite->params.center.y = 0.0f;
	sprite->params.angle    = 0.0f;
	sprite->params.depth    = 0.0f;
}

/** @brief Initializes a sprite from an image stored in a sprite sheet
 *  @param[in] Pointer to sprite
 *  @param[in] sheet Sprite sheet handle
 *  @param[in] index Index of the image inside the sprite sheet
 */
static inline void C2D_SpriteFromSheet(C2D_Sprite* sprite, C2D_SpriteSheet sheet, size_t index)
{
	C2D_SpriteFromImage(sprite, C2D_SpriteSheetGetImage(sheet, index));
}

/** @brief Scale sprite (relative)
 *  @param[in] sprite Pointer to sprite
 *  @param[in] x      X scale (negative values flip the sprite horizontally)
 *  @param[in] y      Y scale (negative values flip the sprite vertically)
 */
static inline void C2D_SpriteScale(C2D_Sprite* sprite, float x, float y)
{
	sprite->params.pos.w *= x;
	sprite->params.pos.h *= y;
	sprite->params.center.x *= x;
	sprite->params.center.y *= y;
}

/** @brief Rotate sprite (relative)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] radians Amount to rotate in radians
 */
static inline void C2D_SpriteRotate(C2D_Sprite* sprite, float radians)
{
	sprite->params.angle += radians;
}

/** @brief Rotate sprite (relative)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] degrees Amount to rotate in degrees
 */
static inline void C2D_SpriteRotateDegrees(C2D_Sprite* sprite, float degrees)
{
	C2D_SpriteRotate(sprite, C3D_AngleFromDegrees(degrees));
}

/** @brief Move sprite (relative)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] x       X translation
 *  @param[in] y       Y translation
 */
static inline void C2D_SpriteMove(C2D_Sprite* sprite, float x, float y)
{
	sprite->params.pos.x += x;
	sprite->params.pos.y += y;
}

/** @brief Scale sprite (absolute)
 *  @param[in] sprite Pointer to sprite
 *  @param[in] x      X scale (negative values flip the sprite horizontally)
 *  @param[in] y      Y scale (negative values flip the sprite vertically)
 */
static inline void C2D_SpriteSetScale(C2D_Sprite* sprite, float x, float y)
{
	float oldCenterX = sprite->params.center.x / sprite->params.pos.w;
	float oldCenterY = sprite->params.center.y / sprite->params.pos.h;
	sprite->params.pos.w = x*sprite->image.subtex->width;
	sprite->params.pos.h = y*sprite->image.subtex->height;
	sprite->params.center.x = fabsf(oldCenterX*sprite->params.pos.w);
	sprite->params.center.y = fabsf(oldCenterY*sprite->params.pos.h);
}

/** @brief Rotate sprite (absolute)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] radians Amount to rotate in radians
 */
static inline void C2D_SpriteSetRotation(C2D_Sprite* sprite, float radians)
{
	sprite->params.angle = radians;
}

/** @brief Rotate sprite (absolute)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] degrees Amount to rotate in degrees
 */
static inline void C2D_SpriteSetRotationDegrees(C2D_Sprite* sprite, float degrees)
{
	C2D_SpriteSetRotation(sprite, C3D_AngleFromDegrees(degrees));
}

/** @brief Set the center of a sprite in values independent of the sprite size (absolute)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] x       X position of the center (0.0 through 1.0)
 *  @param[in] y       Y position of the center (0.0 through 1.0)
 */
static inline void C2D_SpriteSetCenter(C2D_Sprite* sprite, float x, float y)
{
	sprite->params.center.x = x*sprite->params.pos.w;
	sprite->params.center.y = y*sprite->params.pos.h;
}

/** @brief Set the center of a sprite in terms of pixels (absolute)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] x       X position of the center (in pixels)
 *  @param[in] y       Y position of the center (in pixels)
 */
static inline void C2D_SpriteSetCenterRaw(C2D_Sprite* sprite, float x, float y)
{
	sprite->params.center.x = x;
	sprite->params.center.y = y;
}

/** @brief Move sprite (absolute)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] x       X position
 *  @param[in] y       Y position
 */
static inline void C2D_SpriteSetPos(C2D_Sprite* sprite, float x, float y)
{
	sprite->params.pos.x = x;
	sprite->params.pos.y = y;
}

/** @brief Sets the depth level of a sprite (absolute)
 *  @param[in] sprite  Pointer to sprite
 *  @param[in] depth   Depth value
 */
static inline void C2D_SpriteSetDepth(C2D_Sprite* sprite, float depth)
{
	sprite->params.depth = depth;
}

/** @brief Draw sprite
 *  @param[in] sprite Sprite to draw
 */
static inline bool C2D_DrawSprite(const C2D_Sprite* sprite)
{
	return C2D_DrawImage(sprite->image, &sprite->params, NULL);
}

/** @brief Draw sprite with color tinting
 *  @param[in] sprite Sprite to draw
 *  @param[in] tint Color tinting parameters to apply to the sprite
 */
static inline bool C2D_DrawSpriteTinted(const C2D_Sprite* sprite, const C2D_ImageTint* tint)
{
	return C2D_DrawImage(sprite->image, &sprite->params, tint);
}

/** @} */
