/**
 * @file text.h
 * @brief Text rendering API
 */
#pragma once
#include "base.h"
#include "font.h"

struct C2D_TextBuf_s;
typedef struct C2D_TextBuf_s* C2D_TextBuf;

/** @defgroup Text Text drawing functions
 *  @{
 */

/// Text object.
typedef struct
{
	C2D_TextBuf buf;   ///< Buffer associated with the text.
	size_t      begin; ///< Reserved for internal use.
	size_t      end;   ///< Reserved for internal use.
	float       width; ///< Width of the text in pixels, according to 1x scale metrics.
	u32         lines; ///< Number of lines in the text.
	u32         words; ///< Number of words in the text.
	C2D_Font    font;  ///< Font used to draw the text, or NULL for system font
} C2D_Text;

enum
{
	C2D_AtBaseline       = BIT(0), ///< Matches the Y coordinate with the baseline of the font.
	C2D_WithColor        = BIT(1), ///< Draws text with color. Requires a u32 color value.
	C2D_AlignLeft        = 0 << 2, ///< Draws text aligned to the left. This is the default.
	C2D_AlignRight       = 1 << 2, ///< Draws text aligned to the right.
	C2D_AlignCenter      = 2 << 2, ///< Draws text centered.
	C2D_AlignJustified   = 3 << 2, ///< Draws text justified. When C2D_WordWrap is not specified, right edge is x + scaleX*text->width. Otherwise, right edge is x + the width specified for those values.
	C2D_AlignMask        = 3 << 2, ///< Bitmask for alignment values.
	C2D_WordWrap         = BIT(4), ///< Draws text with wrapping of full words before specified width. Requires a float value, passed after color if C2D_WithColor is specified.
};

/** @brief Creates a new text buffer.
 *  @param[in] maxGlyphs Maximum number of glyphs that can be stored in the buffer.
 *  @returns Text buffer handle (or NULL on failure).
 */
C2D_TextBuf C2D_TextBufNew(size_t maxGlyphs);

/** @brief Resizes a text buffer.
 *  @param[in] buf Text buffer to resize.
 *  @param[in] maxGlyphs Maximum number of glyphs that can be stored in the buffer.
 *  @returns New text buffer handle (or NULL on failure).
 *  @remarks If successful, old text buffer handle becomes invalid.
 */
C2D_TextBuf C2D_TextBufResize(C2D_TextBuf buf, size_t maxGlyphs);


/** @brief Deletes a text buffer.
 *  @param[in] buf Text buffer handle.
 *  @remarks This also invalidates all text objects previously created with this buffer.
 */
void C2D_TextBufDelete(C2D_TextBuf buf);

/** @brief Clears all stored text in a buffer.
 *  @param[in] buf Text buffer handle.
 */
void C2D_TextBufClear(C2D_TextBuf buf);

/** @brief Retrieves the number of glyphs stored in a text buffer.
 *  @param[in] buf Text buffer handle.
 *  @returns The number of glyphs.
 */
size_t C2D_TextBufGetNumGlyphs(C2D_TextBuf buf);

/** @brief Parses and adds a single line of text to a text buffer.
 *  @param[out] text Pointer to text object to store information in.
 *  @param[in] buf Text buffer handle.
 *  @param[in] str String to parse.
 *  @param[in] lineNo Line number assigned to the text (used to calculate vertical position).
 *  @remarks Whitespace doesn't add any glyphs to the text buffer and is thus "free".
 *  @returns On success, a pointer to the character on which string processing stopped, which
 *           can be a newline ('\n'; indicating that's where the line ended), the null character
 *           ('\0'; indicating the end of the string was reached), or any other character
 *           (indicating the text buffer is full and no more glyphs can be added).
 *           On failure, NULL.
 */
const char* C2D_TextParseLine(C2D_Text* text, C2D_TextBuf buf, const char* str, u32 lineNo);

/** @brief Parses and adds a single line of text to a text buffer.
 *  @param[out] text Pointer to text object to store information in.
 *  @param[in] font Font to get glyphs from, or null for system font
 *  @param[in] buf Text buffer handle.
 *  @param[in] str String to parse.
 *  @param[in] lineNo Line number assigned to the text (used to calculate vertical position).
 *  @remarks Whitespace doesn't add any glyphs to the text buffer and is thus "free".
 *  @returns On success, a pointer to the character on which string processing stopped, which
 *           can be a newline ('\n'; indicating that's where the line ended), the null character
 *           ('\0'; indicating the end of the string was reached), or any other character
 *           (indicating the text buffer is full and no more glyphs can be added).
 *           On failure, NULL.
 */
const char* C2D_TextFontParseLine(C2D_Text* text, C2D_Font font, C2D_TextBuf buf, const char* str, u32 lineNo);

/** @brief Parses and adds arbitrary text (including newlines) to a text buffer.
 *  @param[out] text Pointer to text object to store information in.
 *  @param[in] buf Text buffer handle.
 *  @param[in] str String to parse.
 *  @remarks Whitespace doesn't add any glyphs to the text buffer and is thus "free".
 *  @returns On success, a pointer to the character on which string processing stopped, which
 *           can be the null character ('\0'; indicating the end of the string was reached),
 *           or any other character (indicating the text buffer is full and no more glyphs can be added).
 *           On failure, NULL.
 */
const char* C2D_TextParse(C2D_Text* text, C2D_TextBuf buf, const char* str);

/** @brief Parses and adds arbitrary text (including newlines) to a text buffer.
 *  @param[out] text Pointer to text object to store information in.
 *  @param[in] font Font to get glyphs from, or null for system font
 *  @param[in] buf Text buffer handle.
 *  @param[in] str String to parse.
 *  @remarks Whitespace doesn't add any glyphs to the text buffer and is thus "free".
 *  @returns On success, a pointer to the character on which string processing stopped, which
 *           can be the null character ('\0'; indicating the end of the string was reached),
 *           or any other character (indicating the text buffer is full and no more glyphs can be added).
 *           On failure, NULL.
 */
const char* C2D_TextFontParse(C2D_Text* text, C2D_Font font, C2D_TextBuf buf, const char* str);

/** @brief Optimizes a text object in order to be drawn more efficiently.
 *  @param[in] text Pointer to text object.
 */
void C2D_TextOptimize(const C2D_Text* text);

/** @brief Retrieves the total dimensions of a text object.
 *  @param[in] text Pointer to text object.
 *  @param[in] scaleX Horizontal size of the font. 1.0f corresponds to the native size of the font.
 *  @param[in] scaleY Vertical size of the font. 1.0f corresponds to the native size of the font.
 *  @param[out] outWidth (optional) Variable in which to store the width of the text.
 *  @param[out] outHeight (optional) Variable in which to store the height of the text.
 */
void C2D_TextGetDimensions(const C2D_Text* text, float scaleX, float scaleY, float* outWidth, float* outHeight);

/** @brief Draws text using the GPU.
 *  @param[in] text Pointer to text object.
 *  @param[in] flags Text drawing flags.
 *  @param[in] x Horizontal position to draw the text on.
 *  @param[in] y Vertical position to draw the text on. If C2D_AtBaseline is not specified (default), this
 *               is the top left corner of the block of text; otherwise this is the position of the baseline
 *               of the first line of text.
 *  @param[in] z Depth value of the text. If unsure, pass 0.0f.
 *  @param[in] scaleX Horizontal size of the font. 1.0f corresponds to the native size of the font.
 *  @param[in] scaleY Vertical size of the font. 1.0f corresponds to the native size of the font.
 *  @remarks The default 3DS system font has a glyph height of 30px, and the baseline is at 25px.
 */
void C2D_DrawText(const C2D_Text* text, u32 flags, float x, float y, float z, float scaleX, float scaleY, ...);

/** @} */
