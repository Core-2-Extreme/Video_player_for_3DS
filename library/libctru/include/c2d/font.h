/**
 * @file font.h
 * @brief Font loading and management
 */
#pragma once
#include "base.h"

struct C2D_Font_s;
typedef struct C2D_Font_s* C2D_Font;

/** @defgroup Font Font functions
 * @{
 */

/** @brief Load a font from a file
 * @param[in] filename Name of the font file (.bcfnt)
 * @returns Font handle
 * @retval NULL Error
 */
C2D_Font C2D_FontLoad(const char* filename);

/** @brief Load a font from memory
 * @param[in] data Data to load
 * @param[in] size Size of the data to load
 * @returns Font handle
 * @retval NULL Error
 */
C2D_Font C2D_FontLoadFromMem(const void* data, size_t size);

/** @brief Load a font from file descriptor
 * @param[in] fd File descriptor used to load data
 * @returns Font handle
 * @retval NULL Error
 */
C2D_Font C2D_FontLoadFromFD(int fd);

/** @brief Load font from stdio file handle
 *  @param[in] f File handle used to load data
 *  @returns Font handle
 *  @retval NULL Error
 */
C2D_Font C2D_FontLoadFromHandle(FILE* f);

/** @brief Load corresponding font from system archive
 *  @param[in] region Region to get font from
 *  @returns Font handle
 *  @retval NULL Error
 *  @remark JPN, USA, EUR, and AUS all use the same font.
 */
C2D_Font C2D_FontLoadSystem(CFG_Region region);

/** @brief Free a font
 * @param[in] font Font handle
 */
void C2D_FontFree(C2D_Font font);

/** @brief Set a font's texture filter
 * @param[in] font Font handle
 * @param[in] magFilter the magnification filter
 * @param[in] minFilter the minification filter
 */
void C2D_FontSetFilter(C2D_Font font, GPU_TEXTURE_FILTER_PARAM magFilter, GPU_TEXTURE_FILTER_PARAM minFilter);

/** @brief Find the glyph index of a codepoint, or returns the default
 * @param[in] font Font to search, or NULL for system font
 * @param[in] codepoint Codepoint to search for
 * @returns Glyph index
 * @retval font->cfnt->finf.alterCharIndex The codepoint does not exist in the font
 */
int C2D_FontGlyphIndexFromCodePoint(C2D_Font font, u32 codepoint);

/** @brief Get character width info for a given index
 * @param[in] font Font to read from, or NULL for system font
 * @param[in] glyphIndex Index to get the width of
 * @returns Width info for glyph
 */
charWidthInfo_s* C2D_FontGetCharWidthInfo(C2D_Font font, int glyphIndex);

/** @brief Calculate glyph position of given index
 * @param[in] font Font to read from, or NULL for system font
 * @param[out] out Glyph position
 * @param[in] glyphIndex Index to get position of
 * @param[in] flags Misc flags
 * @param[in] scaleX Size to scale in X
 * @param[in] scaleY Size to scale in Y
 */
void C2D_FontCalcGlyphPos(C2D_Font font, fontGlyphPos_s* out, int glyphIndex, u32 flags, float scaleX, float scaleY);

/** @brief Get the font info structure associated with the font
 * @param[in] font Font to read from, or NULL for the system font
 * @returns FINF associated with the font
 */
FINF_s* C2D_FontGetInfo(C2D_Font font);

/** @} */
