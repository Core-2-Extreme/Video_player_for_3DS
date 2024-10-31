/**
 * @file spritesheet.h
 * @brief Spritesheet (texture atlas) loading and management
 */
#pragma once
#include "base.h"

struct C2D_SpriteSheet_s;
typedef struct C2D_SpriteSheet_s* C2D_SpriteSheet;

/** @defgroup SpriteSheet Sprite sheet functions
 *  @{
 */

/** @brief Load a sprite sheet from file
 *  @param[in] filename Name of the sprite sheet file (.t3x)
 *  @returns Sprite sheet handle
 *  @retval NULL Error
 */
C2D_SpriteSheet C2D_SpriteSheetLoad(const char* filename);

/** @brief Load a sprite sheet from memory
 *  @param[in] data Data to load
 *  @param[in] size Size of the data to load
 *  @returns Sprite sheet handle
 *  @retval NULL Error
 */
C2D_SpriteSheet C2D_SpriteSheetLoadFromMem(const void* data, size_t size);

/** @brief Load sprite sheet from file descriptor
 *  @param[in] fd File descriptor used to load data
 *  @returns Sprite sheet handle
 *  @retval NULL Error
 */
C2D_SpriteSheet C2D_SpriteSheetFromFD(int fd);

/** @brief Load sprite sheet from stdio file handle
 *  @param[in] f File handle used to load data
 *  @returns Sprite sheet handle
 *  @retval NULL Error
 */
C2D_SpriteSheet C2D_SpriteSheetLoadFromHandle(FILE* f);

/** @brief Free a sprite sheet
 *  @param[in] sheet Sprite sheet handle
 */
void C2D_SpriteSheetFree(C2D_SpriteSheet sheet);

/** @brief Retrieves the number of sprites in the specified sprite sheet
 *  @param[in] sheet Sprite sheet handle
 *  @returns Number of sprites
 */
size_t C2D_SpriteSheetCount(C2D_SpriteSheet sheet);

/** @brief Retrieves the specified image from the specified sprite sheet
 *  @param[in] sheet Sprite sheet handle
 *  @param[in] index Index of the image to retrieve
 *  @returns Image object
 */
C2D_Image C2D_SpriteSheetGetImage(C2D_SpriteSheet sheet, size_t index);

/** @} */
