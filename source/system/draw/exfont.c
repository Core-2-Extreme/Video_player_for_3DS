//Includes.
#include "system/draw/exfont.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "system/menu.h"
#include "system/draw/draw.h"
#include "system/util/err_types.h"
#include "system/util/file.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/util.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
static void Exfont_draw_external_fonts_internal(Exfont_one_char* character, float texture_x, float texture_y, float base_size,
float x_scale, float y_scale, uint32_t abgr8888, float* out_width, float* out_height, bool size_only);
static uint32_t Exfont_load_exfont(uint16_t exfont_id);
static void Exfont_unload_exfont(uint16_t exfont_id);
static void Exfont_load_font_callback(void);

//Variables.
static bool util_exfont_loaded_external_font[DEF_EXFONT_NUM_OF_FONT_NAME] = { 0, };
static bool util_exfont_request_external_font_state[DEF_EXFONT_NUM_OF_FONT_NAME] = { 0, };
static bool util_exfont_load_external_font_request = false;
static bool util_exfont_unload_external_font_request = false;
static bool util_exfont_init = false;
static uint32_t util_exfont_texture_num[DEF_EXFONT_NUM_OF_FONT_NAME] = { 0, };
static uint32_t util_exfont_cjk_unified_ideographs_texture_num[21] = { 0, };
static uint32_t util_exfont_hangul_syllables_texture_num[11] = { 0, };
static const uint16_t util_exfont_font_characters[DEF_EXFONT_NUM_OF_FONT_NAME] =
{
	128,  96, 128, 208,  96,  80, 112, 144, 256,  48,  96, 112, 256, 128, 128, 128,
	128, 128, 128, 128, 128, 256,  96, 640, 128,  64, 256, 112,  48,  48, 112, 256,
	256,  32, 160, 128,  32,  96, 256, 192, 128, 256,  64,  96,  96, 256, 20992, 1168,
	64, 11184, 32, 240, 768,  80,
};
static const uint16_t util_exfont_font_cjk_unified_ideographs_characters[21] =
{
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
	1024, 1024, 1024, 1024,  512,
};
static const uint16_t util_exfont_font_hangul_syllables_characters[11] =
{
	1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,  944,
};
static uint32_t util_exfont_num_of_right_left_characters = 0;
static uint32_t util_exfont_font_start_num[DEF_EXFONT_NUM_OF_FONT_NAME] = { 0, };
static Str_data util_exfont_font_name[DEF_EXFONT_NUM_OF_FONT_NAME] = { 0, };
static Exfont_one_char util_exfont_font_right_to_left_samples[257] = { 0, };
static const Exfont_one_char util_exfont_ignore_chars[] =
{
	{ .buffer = "\u2000", }, { .buffer = "\u2001", }, { .buffer = "\u2002", }, { .buffer = "\u2003", },
	{ .buffer = "\u2004", }, { .buffer = "\u2005", }, { .buffer = "\u2006", }, { .buffer = "\u2007", },
	{ .buffer = "\u2008", }, { .buffer = "\u2009", }, { .buffer = "\u200A", }, { .buffer = "\u200B", },
	{ .buffer = "\u200C", }, { .buffer = "\u200D", }, { .buffer = "\u200E", }, { .buffer = "\u200F", },
};
static const Exfont_one_char util_exfont_sample_one_byte[DEF_EXFONT_NUM_OF_ONE_BYTE_FONT] =
{
	//"\u0080"
	{ .buffer = { 0xC2, 0x80, }, },
};
static const Exfont_one_char util_exfont_samples_two_bytes[DEF_EXFONT_NUM_OF_TWO_BYTES_FONT] =
{
	{ .buffer = "\u0100", }, { .buffer = "\u0180", }, { .buffer = "\u0250", }, { .buffer = "\u02B0", },
	{ .buffer = "\u0300", }, { .buffer = "\u0370", }, { .buffer = "\u0400", }, { .buffer = "\u0500", },
	{ .buffer = "\u0530", }, { .buffer = "\u0590", }, { .buffer = "\u0600", }, { .buffer = "\u0700", },
};
static const Exfont_one_char util_exfont_samples_three_bytes[DEF_EXFONT_NUM_OF_THREE_BYTES_FONT] =
{
	{ .buffer = "\u0980", }, { .buffer = "\u0A80", }, { .buffer = "\u0C00", }, { .buffer = "\u0C80", },
	{ .buffer = "\u0D00", }, { .buffer = "\u0E00", }, { .buffer = "\u0E80", }, { .buffer = "\u0F00", },
	{ .buffer = "\u1000", }, { .buffer = "\u1100", }, { .buffer = "\u1680", }, { .buffer = "\u1D80", },
	{ .buffer = "\u1E00", }, { .buffer = "\u2000", }, { .buffer = "\u2070", }, { .buffer = "\u20A0", },
	{ .buffer = "\u2100", }, { .buffer = "\u2200", }, { .buffer = "\u2300", }, { .buffer = "\u2400", },
	{ .buffer = "\u2460", }, { .buffer = "\u2500", }, { .buffer = "\u2580", }, { .buffer = "\u25A0", },
	{ .buffer = "\u2600", }, { .buffer = "\u2700", }, { .buffer = "\u27C0", }, { .buffer = "\u2980", },
	{ .buffer = "\u2C00", }, { .buffer = "\u3040", }, { .buffer = "\u30A0", }, { .buffer = "\u3100", },
	{ .buffer = "\u3400", }, { .buffer = "\uA000", }, { .buffer = "\uA490", }, { .buffer = "\uA4D0", },
	{ .buffer = "\uD7B0", }, { .buffer = "\uFE50", }, { .buffer = "\uFFF0", },
};
static const Exfont_one_char util_exfont_samples_four_bytes[DEF_EXFONT_NUM_OF_FOUR_BYTES_FONT] =
{
	{ .buffer = "\U0001F600", }, { .buffer = "\U0001F650", },
};
static C2D_Image util_exfont_font_images[41376] = { 0, };

//Code.
uint32_t Exfont_init(void)
{
	uint32_t characters = 0;
	uint8_t* fs_buffer = NULL;
	uint32_t read_size = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(util_exfont_init)
		goto already_inited;

	result = Util_file_load_from_rom("font_name.txt", "romfs:/gfx/msg/", &fs_buffer, 0x2000, &read_size);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_file_load_from_rom, false, result);
		goto api_failed;
	}

	result = Util_parse_file((char*)fs_buffer, DEF_EXFONT_NUM_OF_FONT_NAME, util_exfont_font_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_parse_file, false, result);
		goto api_failed;
	}

	free(fs_buffer);
	fs_buffer = NULL;

	result = Util_file_load_from_rom("font_right_to_left_samples.txt", "romfs:/gfx/font/sample/", &fs_buffer, 0x8000, &read_size);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_file_load_from_rom, false, result);
		goto api_failed;
	}

	Exfont_text_parse_ignore_rtl((char*)fs_buffer, util_exfont_font_right_to_left_samples, 256, &characters);
	free(fs_buffer);
	fs_buffer = NULL;

	util_exfont_num_of_right_left_characters = characters - 1;

	util_exfont_font_start_num[0] = 0;
	for (uint16_t i = 1; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
		util_exfont_font_start_num[i] = util_exfont_font_start_num[i - 1] + util_exfont_font_characters[i - 1];

	for(uint8_t i = 0; i < 21; i++)
		util_exfont_cjk_unified_ideographs_texture_num[i] = UINT32_MAX;
	for(uint8_t i = 0; i < 11; i++)
		util_exfont_hangul_syllables_texture_num[i] = UINT32_MAX;

	for (uint16_t i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
	{
		util_exfont_loaded_external_font[i] = false;
		util_exfont_request_external_font_state[i] = false;
		util_exfont_texture_num[i] = UINT32_MAX;
	}

	if(!Menu_add_worker_thread_callback(Exfont_load_font_callback))
	{
		DEF_LOG_RESULT(Menu_add_worker_thread_callback, false, DEF_ERR_OTHER);
		goto other;
	}

	util_exfont_init = true;
	return DEF_SUCCESS;

	already_inited:
	return DEF_ERR_ALREADY_INITIALIZED;

	api_failed:
	free(fs_buffer);
	fs_buffer = NULL;
	return result;

	other:
	return DEF_ERR_OTHER;
}

void Exfont_exit(void)
{
	if(!util_exfont_init)
		return;

	util_exfont_init = false;
	Menu_remove_worker_thread_callback(Exfont_load_font_callback);
}

const char* Exfont_query_external_font_name(uint16_t exfont_id)
{
	if(!util_exfont_init)
		return "";
	else if (exfont_id < DEF_EXFONT_NUM_OF_FONT_NAME)
		return util_exfont_font_name[exfont_id].buffer;
	else
		return "";
}

bool Exfont_is_loaded_external_font(uint16_t exfont_id)
{
	if(!util_exfont_init)
		return false;
	else if (exfont_id < DEF_EXFONT_NUM_OF_FONT_NAME)
		return util_exfont_loaded_external_font[exfont_id];
	else
		return false;
}

bool Exfont_is_loading_external_font(void)
{
	if(!util_exfont_init)
		return false;

	return util_exfont_load_external_font_request;
}

bool Exfont_is_unloading_external_font(void)
{
	if(!util_exfont_init)
		return false;

	return util_exfont_unload_external_font_request;
}

void Exfont_set_external_font_request_state(uint16_t exfont_id, bool flag)
{
	if(!util_exfont_init)
		return;

	if (exfont_id < DEF_EXFONT_NUM_OF_FONT_NAME)
		util_exfont_request_external_font_state[exfont_id] = flag;
}

void Exfont_request_load_external_font(void)
{
	if(!util_exfont_init)
		return;

	util_exfont_load_external_font_request = true;
}

void Exfont_request_unload_external_font(void)
{
	if(!util_exfont_init)
		return;

	util_exfont_unload_external_font_request = true;
}

void Exfont_text_parse_ignore_rtl(const char* source_string, Exfont_one_char out_string[], uint32_t max_loop, uint32_t* out_element)
{
	uint32_t source_string_length = 0;
	uint32_t out_index = 0;
	uint32_t offset = 0;

	if(!source_string || !out_string || max_loop == 0 || !out_element)
		return;

	*out_element = 0;
	source_string_length = strlen(source_string);
	if(source_string_length == 0)
		return;

	for (uint32_t k = 0; k < max_loop; k++)
	{
		int32_t parse_string_length = mblen((source_string + offset), DEF_EXFONT_MAX_CHAR_LENGTH);

		if (offset >= source_string_length)
			break;
		else if (parse_string_length >= 1)
		{
			memcpy(out_string[out_index].buffer, (source_string + offset), parse_string_length);
			out_string[out_index].buffer[parse_string_length] = 0x00;
			out_index++;
			offset += parse_string_length;
		}
		else
			offset++;
	}
	out_string[out_index].buffer[0] = 0x00;
	*out_element = out_index;
}

void Exfont_text_parse(const char* source_string, Exfont_one_char out_string[], uint32_t max_loop, uint32_t* out_element)
{
	uint32_t* rtl_index_list = NULL;
	uint32_t source_string_length = 0;
	uint32_t out_index = 0;
	uint32_t rtl_index = 0;
	uint32_t offset = 0;
	Exfont_one_char right_to_left_sample[3] = { { .buffer = "\u05BD", }, { .buffer = "\u0700", }, { .buffer = "\u200F", }, };
	Exfont_one_char* cache_part_string = NULL;

	if(!source_string || !out_string || max_loop == 0 || !out_element)
		return;

	source_string_length = strlen(source_string);
	if(source_string_length == 0)
		return;

	cache_part_string = (Exfont_one_char*)malloc(sizeof(Exfont_one_char) * source_string_length);
	rtl_index_list = (uint32_t*)malloc(sizeof(uint32_t) * source_string_length);
	if(!cache_part_string || !rtl_index_list)
	{
		free(cache_part_string);
		free(rtl_index_list);
		cache_part_string = NULL;
		rtl_index_list = NULL;
		return;
	}

	*out_element = 0;

	for (uint32_t k = 0; k < max_loop; k++)
	{
		int32_t parse_string_length = 0;

		if(source_string_length <= offset)
			break;

		parse_string_length = mblen((source_string + offset), (source_string_length - offset >= 4 ? 4 : (source_string_length - offset)));
		if (parse_string_length >= 1)
		{
			bool rtl_found = false;
			Exfont_one_char one_char = { 0, };

			memcpy(one_char.buffer, (source_string + offset), parse_string_length);
			one_char.buffer[parse_string_length] = 0x0;

			//Sort for RTL characters, currently only Arabic and Armenian are supported.
			if((parse_string_length == 2 && strcmp(one_char.buffer, right_to_left_sample[0].buffer) > 0
			&& strcmp(one_char.buffer, right_to_left_sample[1].buffer) < 0))
			{
				int32_t increment = (util_exfont_num_of_right_left_characters / 10);
				uint32_t give_up_pos = 0;

				for(uint32_t i = 0; i < util_exfont_num_of_right_left_characters; i += increment)
				{
					int32_t strcmp_result = 0;
					if(increment == -1 && (i == give_up_pos))
						break;

					strcmp_result = strcmp(one_char.buffer, util_exfont_font_right_to_left_samples[i].buffer);
					if (strcmp_result == 0)
					{
						rtl_found = true;
						break;
					}
					else if(increment != -1 && strcmp_result < 0)
					{
						give_up_pos = i - increment;
						increment = -1;
						if(i == 0)
							break;
					}
				}
			}
			else if(strcmp(right_to_left_sample[2].buffer, one_char.buffer) == 0)
				rtl_found = true;

			if (rtl_found)
			{
				memcpy(cache_part_string[rtl_index].buffer, one_char.buffer, parse_string_length);
				cache_part_string[rtl_index].buffer[parse_string_length] = 0x00;
				rtl_index_list[rtl_index] = out_index;
				rtl_index++;
			}
			else
			{
				uint32_t length = 0;

				if(rtl_index > 0)
				{
					for(uint32_t i = 0, m = (rtl_index - 1); i < rtl_index; i++, m--)
					{
						length = strlen(cache_part_string[i].buffer);
						memcpy(out_string[rtl_index_list[m]].buffer, cache_part_string[i].buffer, length);
						out_string[rtl_index_list[m]].buffer[length] = 0x00;
					}

					rtl_index = 0;
				}

				length = strlen(one_char.buffer);
				memcpy(out_string[out_index].buffer, one_char.buffer, length);
				out_string[out_index].buffer[length] = 0x00;
			}

			offset += parse_string_length;
			out_index++;
		}
		else
			offset++;
	}

	if(rtl_index > 0)
	{
		for(uint32_t i = 0, m = (rtl_index - 1); i < rtl_index; i++, m--)
		{
			uint32_t length = strlen(cache_part_string[i].buffer);

			memcpy(out_string[rtl_index_list[m]].buffer, cache_part_string[i].buffer, length);
			out_string[rtl_index_list[m]].buffer[length] = 0x00;
		}

		rtl_index = 0;
	}

	free(rtl_index_list);
	free(cache_part_string);
	rtl_index_list = NULL;
	cache_part_string = NULL;
	out_string[out_index].buffer[0] = 0x00;
	*out_element = out_index;
}

void Exfont_draw_external_fonts(Exfont_one_char* character, float texture_x, float texture_y, float base_size,
float x_scale, float y_scale, uint32_t abgr8888, float* out_width, float* out_height)
{
	Exfont_draw_external_fonts_internal(character, texture_x, texture_y, base_size, x_scale, y_scale, abgr8888, out_width, out_height, false);
}

void Exfont_draw_get_text_size(Exfont_one_char* character, float base_size, float x_scale, float y_scale, float* out_width, float* out_height)
{
	Exfont_draw_external_fonts_internal(character, 0, 0, base_size, x_scale, y_scale, DEF_DRAW_NO_COLOR, out_width, out_height, true);
}

static void Exfont_draw_external_fonts_internal(Exfont_one_char* character, float texture_x, float texture_y, float base_size,
float x_scale, float y_scale, uint32_t abgr8888, float* out_width, float* out_height, bool size_only)
{
	bool unknown = true;
	uint16_t block = UINT16_MAX;
	uint32_t base_index = 0;
	uint32_t offset = 0;
	uint32_t length = 0;
	float width_rate = 1;
	float x_size = 0;
	float y_size = 0;

	if(!util_exfont_init)
		return;

	if(!util_exfont_loaded_external_font[DEF_EXFONT_BLOCK_BASIC_LATIN])
		return;

	if(!character || !out_width || !out_height)
		return;

	*out_width = 0;
	*out_height = 0;

	length = strlen(character->buffer);
	if(length == 0)
		return;

	if (length == 1)
	{
		if (strcmp(character->buffer, util_exfont_sample_one_byte[0].buffer) < 0)
			block = 0;
	}
	else if (length == 2)
	{
		for (uint8_t i = 0; i < DEF_EXFONT_NUM_OF_TWO_BYTES_FONT; i++)
		{
			if (strcmp(character->buffer, util_exfont_samples_two_bytes[i].buffer) < 0)
			{
				block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT;
				break;
			}
		}
	}
	else if (length == 3)
	{
		uint8_t start_pos = UINT8_MAX;
		uint8_t end_pos = 0;
		uint8_t loop = DEF_EXFONT_NUM_OF_THREE_BYTES_FONT / 10;

		for (uint8_t i = 1; i <= loop; i++)
		{
			uint8_t index = (i * 10) - 1;
			if (strcmp(character->buffer, util_exfont_samples_three_bytes[index].buffer) < 0)
			{
				start_pos = index - 9;
				end_pos = start_pos + 10;
				if(end_pos > DEF_EXFONT_NUM_OF_THREE_BYTES_FONT)
					end_pos = DEF_EXFONT_NUM_OF_THREE_BYTES_FONT;

				break;
			}
		}

		if(start_pos == UINT8_MAX)
		{
			start_pos = loop * 10;
			end_pos = DEF_EXFONT_NUM_OF_THREE_BYTES_FONT;
		}

		for (uint8_t i = start_pos; i < end_pos; i++)
		{
			if (strcmp(character->buffer, util_exfont_samples_three_bytes[i].buffer) < 0)
			{
				block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT + DEF_EXFONT_NUM_OF_TWO_BYTES_FONT;
				break;
			}
		}
	}
	else if (length == 4)
	{
		for (uint8_t i = 0; i < DEF_EXFONT_NUM_OF_FOUR_BYTES_FONT; i++)
		{
			if (strcmp(character->buffer, util_exfont_samples_four_bytes[i].buffer) < 0)
			{
				block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT + DEF_EXFONT_NUM_OF_TWO_BYTES_FONT + DEF_EXFONT_NUM_OF_THREE_BYTES_FONT;
				break;
			}
		}
	}

	if (block == DEF_EXFONT_BLOCK_GENERAL_PUNCTUATION)
	{
		for(uint32_t i = 0; i < DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(util_exfont_ignore_chars); i++)
		{
			if(strcmp(util_exfont_ignore_chars[i].buffer, character->buffer) == 0)
			{
				unknown = false;
				block = UINT16_MAX;
				break;
			}
		}
	}

	if (block != UINT16_MAX && util_exfont_loaded_external_font[block])
	{
		unknown = false;
		base_index = util_exfont_font_start_num[block];

		if(length == 1)
		{
			if(character->buffer[0] > 0x7F)
				unknown = true;
			else
			{
				//basic latin : U+0000[0x00]~U+007F[0x7F], passed
				if(block == DEF_EXFONT_BLOCK_BASIC_LATIN)
					offset = character->buffer[0];
				else
					unknown = true;
			}
		}
		else if(length == 2)
		{
			if(character->buffer[1] < 0x80 || character->buffer[1] > 0xBF)
				unknown = true;
			else
			{
				//latin 1 supplement : U+0080[0xC2, 0x80]~U+00FF[0xC3, 0xBF], passed
				//U+0080[0xC2, 0x80]~U+009F[0xC2, 0x9F] are invalid character.
				if(block == DEF_EXFONT_BLOCK_LATIN_1_SUPPLEMENT)
				{
					if(character->buffer[0] == 0xC2)
						offset = character->buffer[1] - 0xA0;
					else if(character->buffer[0] == 0xC3)
						offset = character->buffer[1] - 0x80 + 32;
					else
						unknown = true;
				}
				//latin extended a : U+0100[0xC4, 0x80]~U+017F[0xC5, 0xBF], passed
				else if(block == DEF_EXFONT_BLOCK_LATIN_EXTENDED_A)
				{
					if(character->buffer[0] == 0xC4)
						offset = character->buffer[1] - 0x80;
					else if(character->buffer[0] == 0xC5)
						offset = character->buffer[1] - 0x80 + 64;
					else
						unknown = true;
				}
				//latin extended b : U+0180[0xC6, 0x80]~U+024F[0xC9, 0x8F], passed
				else if(block == DEF_EXFONT_BLOCK_LATIN_EXTENDED_B)
				{
					if(character->buffer[0] == 0xC6)
						offset = character->buffer[1] - 0x80;
					else if(character->buffer[0] == 0xC7)
						offset = character->buffer[1] - 0x80 + 64;
					else if(character->buffer[0] == 0xC8)
						offset = character->buffer[1] - 0x80 + 128;
					else if(character->buffer[0] == 0xC9 && character->buffer[1] <= 0x8F)
						offset = character->buffer[1] - 0x80 + 192;
					else
						unknown = true;
				}
				//ipa extensions : U+0250[0xC9, 0x90]~U+02AF[0xCA, 0xAF], passed
				else if(block == DEF_EXFONT_BLOCK_IPA_EXTENSIONS)
				{
					if(character->buffer[0] == 0xC9 && character->buffer[1] >= 0x90)
						offset = character->buffer[1] - 0x90;
					else if(character->buffer[0] == 0xCA && character->buffer[1] <= 0xAF)
						offset = character->buffer[1] - 0x80 + 48;
					else
						unknown = true;
				}
				//spacing_modifier_letters : U+02B0[0xCA, 0xB0]~U+02FF[0xCB, 0xBF], passed
				else if(block == DEF_EXFONT_BLOCK_SPACING_MODIFIER_LETTERS)
				{
					if(character->buffer[0] == 0xCA && character->buffer[1] >= 0xB0)
						offset = character->buffer[1] - 0xB0;
					else if(character->buffer[0] == 0xCB)
						offset = character->buffer[1] - 0x80 + 16;
					else
						unknown = true;
				}
				//combining_diacritical_marks : U+0300[0xCC, 0x80]~U+036F[0xCD, 0xAF], passed
				else if(block == DEF_EXFONT_BLOCK_COMBINING_DIACRITICAL_MARKS)
				{
					if(character->buffer[0] == 0xCC)
						offset = character->buffer[1] - 0x80;
					else if(character->buffer[0] == 0xCD && character->buffer[1] <= 0xAF)
						offset = character->buffer[1] - 0x80 + 64;
					else
						unknown = true;
				}
				//greek and coptic : U+0370[0xCD, 0xB0]~U+03FF[0xCF, 0xBF], passed
				else if(block == DEF_EXFONT_BLOCK_GREEK_AND_COPTIC)
				{
					if(character->buffer[0] == 0xCD && character->buffer[1] >= 0xB0)
						offset = character->buffer[1] - 0xB0;
					else if(character->buffer[0] == 0xCE)
						offset = character->buffer[1] - 0x80 + 16;
					else if(character->buffer[0] == 0xCF)
						offset = character->buffer[1] - 0x80 + 80;
					else
						unknown = true;
				}
				//cyrillic : U+0400[0xD0, 0x80]~U+04FF[0xD3, 0xBF], passed
				else if(block == DEF_EXFONT_BLOCK_CYRILLIC)
				{
					if(character->buffer[0] == 0xD0)
						offset = character->buffer[1] - 0x80;
					else if(character->buffer[0] == 0xD1)
						offset = character->buffer[1] - 0x80 + 64;
					else if(character->buffer[0] == 0xD2)
						offset = character->buffer[1] - 0x80 + 128;
					else if(character->buffer[0] == 0xD3)
						offset = character->buffer[1] - 0x80 + 192;
					else
						unknown = true;
				}
				//cyrillic supplement : U+0500[0xD4, 0x80]~U+052F[0xD4, 0xAF], passed
				else if(block == DEF_EXFONT_BLOCK_CYRILLIC_SUPPLEMENT)
				{
					if(character->buffer[0] == 0xD4 && character->buffer[1] <= 0xAF)
						offset = character->buffer[1] - 0x80;
					else
						unknown = true;
				}
				//armenian : U+0530[0xD4, 0xB0]~U+058F[0xD6, 0x8F], passed
				else if(block == DEF_EXFONT_BLOCK_ARMENIAN)
				{
					if(character->buffer[0] == 0xD4 && character->buffer[1] >= 0xB0)
						offset = character->buffer[1] - 0xB0;
					else if(character->buffer[0] == 0xD5)
						offset = character->buffer[1] - 0x80 + 16;
					else if(character->buffer[0] == 0xD6 && character->buffer[1] <= 0x8F)
						offset = character->buffer[1] - 0x80 + 80;
					else
						unknown = true;
				}
				//hebrew : U+0590[0xD6, 0x90]~U+05FF[0xD7, 0xBF], passed
				else if(block == DEF_EXFONT_BLOCK_HEBREW)
				{
					if(character->buffer[0] == 0xD6 && character->buffer[1] >= 0x90)
						offset = character->buffer[1] - 0x90;
					else if(character->buffer[0] == 0xD7)
						offset = character->buffer[1] - 0x80 + 48;
					else
						unknown = true;
				}
				//arabic : U+0600[0xD8, 0x80]~U+06FF[0xDB, 0xBF], passed
				else if(block == DEF_EXFONT_BLOCK_ARABIC)
				{
					if(character->buffer[0] == 0xD8)
						offset = character->buffer[1] - 0x80;
					else if(character->buffer[0] == 0xD9)
						offset = character->buffer[1] - 0x80 + 64;
					else if(character->buffer[0] == 0xDA)
						offset = character->buffer[1] - 0x80 + 128;
					else if(character->buffer[0] == 0xDB)
						offset = character->buffer[1] - 0x80 + 192;
					else
						unknown = true;
				}
				else
					unknown = true;
			}
		}
		else if(length == 3)
		{
			if(character->buffer[0] < 0xE0 || character->buffer[0] > 0xEF
			|| character->buffer[2] < 0x80 || character->buffer[2] > 0xBF)
				unknown = true;
			else
			{
				if(block <= DEF_EXFONT_BLOCK_GEORGIAN)
				{
					//devanagari : U+0900[0xE0, 0xA4, 0x80]~U+097F[0xE0, 0xA5, 0xBF], passed
					if(block == DEF_EXFONT_BLOCK_DEVANAGARI)
					{
						if(character->buffer[1] == 0xA4)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xA5)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//gurmukhi : U+0A00[0xE0, 0xA8, 0x80]~U+0A7F[0xE0, 0xA9, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_GURMUKHI)
					{
						if(character->buffer[1] == 0xA8)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xA9)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//tamil : U+0B80[0xE0, 0xAE, 0x80]~U+0BFF[0xE0, 0xAF, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_TAMIL)
					{
						if(character->buffer[1] == 0xAE)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xAF)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//telugu : U+0C00[0xE0, 0xB0, 0x80]~U+0C7F[0xE0, 0xB1, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_TELUGU)
					{
						if(character->buffer[1] == 0xB0)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xB1)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//kannada : U+0C80[0xE0, 0xB2, 0x80]~U+0CFF[0xE0, 0xB3, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_KANNADA)
					{
						if(character->buffer[1] == 0xB2)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xB3)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//sinhala : U+0D80[0xE0, 0xB6, 0x80]~U+0DFF[0xE0, 0xB7, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_SINHALA)
					{
						if(character->buffer[1] == 0xB6)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xB7)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//thai : U+0E00[0xE0, 0xB8, 0x80]~U+0E7F[0xE0, 0xB9, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_THAI)
					{
						if(character->buffer[1] == 0xB8)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xB9)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//lao : U+0E80[0xE0, 0xBA, 0x80]~U+0EFF[0xE0, 0xBB, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_LAO)
					{
						if(character->buffer[1] == 0xBA)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xBB)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//tibetan : U+0F00[0xE0, 0xBC, 0x80]~U+0FFF[0xE0, 0xBF, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_TIBETAN)
					{
						if(character->buffer[1] == 0xBC)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xBD)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0xBE)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0xBF)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					//georgian : U+10A0[0xE1, 0x82, 0xA0]~U+10FF[0xE1, 0x83, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_GEORGIAN)
					{
						if(character->buffer[1] == 0x82 && character->buffer[2] >= 0xA0)
							offset = character->buffer[2] - 0xA0;
						else if(character->buffer[1] == 0x83)
							offset = character->buffer[2] - 0x80 + 32;
						else
							unknown = true;
					}
					else
						unknown = true;
				}
				else if(block <= DEF_EXFONT_BLOCK_MISCELLANEOUS_TECHNICAL)
				{
					//unified_canadian_aboriginal_syllabics : U+1400[0xE1, 0x90, 0x80]~U+167F[0xE1, 0x99, 0xBF], passed
					if(block == DEF_EXFONT_BLOCK_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS)
					{
						if(character->buffer[1] == 0x90)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x91)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0x92)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0x93)
							offset = character->buffer[2] - 0x80 + 192;
						else if(character->buffer[1] == 0x94)
							offset = character->buffer[2] - 0x80 + 256;
						else if(character->buffer[1] == 0x95)
							offset = character->buffer[2] - 0x80 + 320;
						else if(character->buffer[1] == 0x96)
							offset = character->buffer[2] - 0x80 + 384;
						else if(character->buffer[1] == 0x97)
							offset = character->buffer[2] - 0x80 + 448;
						else if(character->buffer[1] == 0x98)
							offset = character->buffer[2] - 0x80 + 512;
						else if(character->buffer[1] == 0x99)
							offset = character->buffer[2] - 0x80 + 576;
						else
							unknown = true;
					}
					//phonetic_extensions : U+1D00[0xE1, 0xB4, 0x80]~U+1D7F[0xE1, 0xB5, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_PHONETIC_EXTENSIONS)
					{
						if(character->buffer[1] == 0xB4)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xB5)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//combining_diacritical_marks_supplement : U+1DC0[0xE1, 0xB7, 0x80]~U+1DFF[0xE1, 0xB7, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT)
					{
						if(character->buffer[1] == 0xB7)
							offset = character->buffer[2] - 0x80;
						else
							unknown = true;
					}
					//greek_extended : U+1F00[0xE1, 0xBC, 0x80]~U+1FFF[0xE1, 0xBF, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_GREEK_EXTENDED)
					{
						if(character->buffer[1] == 0xBC)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xBD)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0xBE)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0xBF)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					//general_punctuation : U+2000[0xE2, 0x80, 0x80]~U+206F[0xE2, 0x81, 0xAF], passed
					else if(block == DEF_EXFONT_BLOCK_GENERAL_PUNCTUATION)
					{
						if(character->buffer[1] == 0x80)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x81 && character->buffer[2] <= 0xAF)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//superscripts_and_subscripts : U+2070[0xE2, 0x81, 0xB0]~U+209F[0xE2, 0x82, 0x9F], passed
					else if(block == DEF_EXFONT_BLOCK_SUPERSCRIPTS_AND_SUBSCRIPTS)
					{
						if(character->buffer[1] == 0x81 && character->buffer[2] >= 0xB0)
							offset = character->buffer[2] - 0xB0;
						else if(character->buffer[1] == 0x82 && character->buffer[2] <= 0x9F)
							offset = character->buffer[2] - 0x80 + 16;
						else
							unknown = true;
					}
					//combining_diacritical_marks_for_symbols : U+20D0[0xE2, 0x83, 0x90]~U+20FF[0xE2, 0x83, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS)
					{
						if(character->buffer[1] == 0x83 && character->buffer[2] >= 0x90)
							offset = character->buffer[2] - 0x90;
						else
							unknown = true;
					}
					//arrows : U+2190[0xE2, 0x86, 0x90]~U+21FF[0xE2, 0x87, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_ARROWS)
					{
						if(character->buffer[1] == 0x86 && character->buffer[2] >= 0x90)
							offset = character->buffer[2] - 0x90;
						else if(character->buffer[1] == 0x87)
							offset = character->buffer[2] - 0x80 + 48;
						else
							unknown = true;
					}
					//mathematical_operators : U+2200[0xE2, 0x88, 0x80]~U+22FF[0xE2, 0x8B, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_MATHEMATICAL_OPERATORS)
					{
						if(character->buffer[1] == 0x88)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x89)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0x8A)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0x8B)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					//miscellaneous_technical : U+2300[0xE2, 0x8C, 0x80]~U+23FF[0xE2, 0x8F, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_TECHNICAL)
					{
						if(character->buffer[1] == 0x8C)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x8D)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0x8E)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0x8F)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					else
						unknown = true;
				}
				else if(block <= DEF_EXFONT_BLOCK_CJK_SYMBOL_AND_PUNCTUATION)
				{
					//optical_character_recognition : U+2440[0xE2, 0x91, 0x80]~U+245F[0xE2, 0x91, 0x9F], passed
					if(block == DEF_EXFONT_BLOCK_OPTICAL_CHARACTER_RECOGNITION)
					{
						if(character->buffer[1] == 0x91 && character->buffer[2] <= 0x9F)
							offset = character->buffer[2] - 0x80;
						else
							unknown = true;
					}
					//enclosed_alphanumerics : U+2460[0xE2, 0x91, 0xA0]~U+24FF[0xE2, 0x93, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_ENCLOSED_ALPHANUMERICS)
					{
						if(character->buffer[1] == 0x91 && character->buffer[2] >= 0xA0)
							offset = character->buffer[2] - 0xA0;
						else if(character->buffer[1] == 0x92)
							offset = character->buffer[2] - 0x80 + 32;
						else if(character->buffer[1] == 0x93)
							offset = character->buffer[2] - 0x80 + 96;
						else
							unknown = true;
					}
					//box_drawing : U+2500[0xE2, 0x94, 0x80]~U+257F[0xE2, 0x95, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_BOX_DRAWING)
					{
						if(character->buffer[1] == 0x94)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x95)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//block_elements : U+2580[0xE2, 0x96, 0x80]~U+259F[0xE2, 0x96, 0x9F], passed
					else if(block == DEF_EXFONT_BLOCK_BLOCK_ELEMENTS)
					{
						if(character->buffer[1] == 0x96 && character->buffer[2] <= 0x9F)
							offset = character->buffer[2] - 0x80;
						else
							unknown = true;
					}
					//geometric_shapes : U+25A0[0xE2, 0x96, 0xA0]~U+25FF[0xE2, 0x97, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_GEOMETRIC_SHAPES)
					{
						if(character->buffer[1] == 0x96 && character->buffer[2] >= 0xA0)
							offset = character->buffer[2] - 0xA0;
						else if(character->buffer[1] == 0x97)
							offset = character->buffer[2] - 0x80 + 32;
						else
							unknown = true;
					}
					//miscellaneous_symbols : U+2600[0xE2, 0x98, 0x80]~U+26FF[0xE2, 0x9B, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_SYMBOLS)
					{
						if(character->buffer[1] == 0x98)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x99)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0x9A)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0x9B)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					//dingbats : U+2700[0xE2, 0x9C, 0x80]~U+27BF[0xE2, 0x9E, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_DINGBATS)
					{
						if(character->buffer[1] == 0x9C)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x9D)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0x9E)
							offset = character->buffer[2] - 0x80 + 128;
						else
							unknown = true;
					}
					//supplemental_arrows_b : U+2900[0xE2, 0xA4, 0x80]~U+297F[0xE2, 0xA5, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_SUPPLEMENTAL_ARROWS_B)
					{
						if(character->buffer[1] == 0xA4)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xA5)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//miscellaneous_symbols_and_arrows : U+2B00[0xE2, 0xAC, 0x80]~U+2BFF[0xE2, 0xAF, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_SYMBOLS_AND_ARROWS)
					{
						if(character->buffer[1] == 0xAC)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xAD)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0xAE)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0xAF)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					//cjk_symbol_and_punctuation : U+3000[0xE3, 0x80, 0x80]~U+303F[0xE3, 0x80, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_CJK_SYMBOL_AND_PUNCTUATION)
					{
						if(character->buffer[1] == 0x80)
							offset = character->buffer[2] - 0x80;
						else
							unknown = true;
					}
					else
						unknown = true;
				}
				else
				{
					//hiragana : U+3040[0xE3, 0x81, 0x80 ]~U+309F[0xE3, 0x82, 0x9F], passed
					if(block == DEF_EXFONT_BLOCK_HIRAGANA)
					{
						if(character->buffer[1] == 0x81)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x82 && character->buffer[2] <= 0x9F)
							offset = character->buffer[2] - 0x80 + 64;
						else
							unknown = true;
					}
					//katakana : U+30A0[0xE3, 0x82, 0xA0]~U+30FF[0xE3, 0x83, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_KATAKANA)
					{
						if(character->buffer[1] == 0x82 && character->buffer[2] >= 0xA0)
							offset = character->buffer[2] - 0xA0;
						else if(character->buffer[1] == 0x83)
							offset = character->buffer[2] - 0x80 + 32;
						else
							unknown = true;
					}
					//cjk_compatibility : U+3300[0xE3, 0x8C, 0x80]~U+33FF[0xE3, 0x8F, 0xBF], passed
					else if(block == DEF_EXFONT_BLOCK_CJK_COMPATIBILITY)
					{
						if(character->buffer[1] == 0x8C)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x8D)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0x8E)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0x8F)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					//cjk_unified_ideographs : U+4E00[0xE4, 0xB8, 0x80]~U+9FFF[0xE9, 0xBF, 0xBF]
					else if(block == DEF_EXFONT_BLOCK_CJK_UNIFIED_IDEOGRAPHS)
					{
						if(character->buffer[0] >= 0xE4 && character->buffer[0] <= 0xE9)
						{
							uint32_t start_pos = 0;
							uint32_t end_pos = 0;
							uint32_t offset_cache = 0;

							if(character->buffer[1] < 0x90)
								start_pos = 0x80;
							else if(character->buffer[1] < 0xA0)
								start_pos = 0x90;
							else if(character->buffer[1] < 0xB0)
								start_pos = 0xA0;
							else if(character->buffer[1] < 0xC0)
								start_pos = 0xB0;

							end_pos = start_pos + 0x10;

							if(character->buffer[0] == 0xE4)
								offset = 0;
							else if(character->buffer[0] == 0xE5)
								offset += 512;
							else if(character->buffer[0] == 0xE6)
								offset += 4608;
							else if(character->buffer[0] == 0xE7)
								offset += 8704;
							else if(character->buffer[0] == 0xE8)
								offset += 12800;
							else if(character->buffer[0] == 0xE9)
								offset += 16896;

							unknown = true;
							if(character->buffer[0] == 0xE4)
							{
								start_pos = 0xB8;
								end_pos = 0xC0;
								for(uint32_t i = start_pos; i < end_pos; i++)
								{
									if(character->buffer[1] == i)
									{
										offset_cache = character->buffer[2] - 0x80 + ((i - 0xB8) * 64);
										unknown = false;
										break;
									}
								}
							}
							else
							{
								for(uint32_t i = start_pos; i < end_pos; i++)
								{
									if(character->buffer[1] == i)
									{
										offset_cache = character->buffer[2] - 0x80 + ((i - 0x80) * 64);
										unknown = false;
										break;
									}
								}
							}

							offset += offset_cache;
						}
						else
							unknown = true;
					}
					//yi_syllables : U+A000[0xEA, 0x80, 0x80]~U+A48F[0xEA, 0x92, 0x8F], passed
					else if(block == DEF_EXFONT_BLOCK_YI_SYLLABLES)
					{
						if(character->buffer[1] == 0x80)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0x81)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0x82)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0x83)
							offset = character->buffer[2] - 0x80 + 192;
						else if(character->buffer[1] == 0x84)
							offset = character->buffer[2] - 0x80 + 256;
						else if(character->buffer[1] == 0x85)
							offset = character->buffer[2] - 0x80 + 320;
						else if(character->buffer[1] == 0x86)
							offset = character->buffer[2] - 0x80 + 384;
						else if(character->buffer[1] == 0x87)
							offset = character->buffer[2] - 0x80 + 448;
						else if(character->buffer[1] == 0x88)
							offset = character->buffer[2] - 0x80 + 512;
						else if(character->buffer[1] == 0x89)
							offset = character->buffer[2] - 0x80 + 576;
						else if(character->buffer[1] == 0x8A)
							offset = character->buffer[2] - 0x80 + 640;
						else if(character->buffer[1] == 0x8B)
							offset = character->buffer[2] - 0x80 + 704;
						else if(character->buffer[1] == 0x8C)
							offset = character->buffer[2] - 0x80 + 768;
						else if(character->buffer[1] == 0x8D)
							offset = character->buffer[2] - 0x80 + 832;
						else if(character->buffer[1] == 0x8E)
							offset = character->buffer[2] - 0x80 + 896;
						else if(character->buffer[1] == 0x8F)
							offset = character->buffer[2] - 0x80 + 960;
						else if(character->buffer[1] == 0x90)
							offset = character->buffer[2] - 0x80 + 1024;
						else if(character->buffer[1] == 0x91)
							offset = character->buffer[2] - 0x80 + 1088;
						else if(character->buffer[1] == 0x92 && character->buffer[2] <= 0x8F)
							offset = character->buffer[2] - 0x80 + 1152;
						else
							unknown = true;
					}
					//yi_radicals : U+A490[0xEA, 0x92, 0x90]~U+A4CF[0xEA, 0x93, 0x8F], passed
					else if(block == DEF_EXFONT_BLOCK_YI_RADICALS)
					{
						if(character->buffer[1] == 0x92 && character->buffer[2] >= 0x90)
							offset = character->buffer[2] - 0x90;
						else if(character->buffer[1] == 0x93)
							offset = character->buffer[2] - 0x80 + 48;
						else
							unknown = true;
					}
					//hangul_syllables : U+AC00[0xEA, 0xB0, 0x80]~U+D7AF[0xED, 0x9E, 0xAF]
					else if(block == DEF_EXFONT_BLOCK_HANGUL_SYLLABLES)
					{
						if(character->buffer[0] >= 0xEA && character->buffer[0] <= 0xED)
						{
							uint32_t start_pos = 0;
							uint32_t end_pos = 0;
							uint32_t offset_cache = 0;

							if(character->buffer[1] < 0x90)
								start_pos = 0x80;
							else if(character->buffer[1] < 0xA0)
								start_pos = 0x90;
							else if(character->buffer[1] < 0xB0)
								start_pos = 0xA0;
							else if(character->buffer[1] < 0xC0)
								start_pos = 0xB0;

							end_pos = start_pos + 0x10;

							if(character->buffer[0] == 0xEA)
								offset = 0;
							else if(character->buffer[0] == 0xEB)
								offset += 1024;
							else if(character->buffer[0] == 0xEC)
								offset += 5120;
							else if(character->buffer[0] == 0xED)
								offset += 9216;

							unknown = true;
							if(character->buffer[0] == 0xEA)
							{
								start_pos = 0xB0;
								end_pos = 0xC0;
								for(uint32_t i = start_pos; i < end_pos; i++)
								{
									if(character->buffer[1] == i)
									{
										offset_cache = character->buffer[2] - 0x80 + ((i - 0xB0) * 64);
										unknown = false;
										break;
									}
								}
							}
							else
							{
								if(!(character->buffer[0] == 0xED && character->buffer[1] >= 0x9E && character->buffer[0] > 0xAF))
								{
									for(uint32_t i = start_pos; i < end_pos; i++)
									{
										if(character->buffer[1] == i)
										{
											offset_cache = character->buffer[2] - 0x80 + ((i - 0x80) * 64);
											unknown = false;
											break;
										}
									}
								}
							}

							offset += offset_cache;
						}
						else
							unknown = true;
					}
					//cjk_compatibility_forms : U+FE30[0xEF, 0xB8, 0xB0]~U+FE4F[0xEF, 0xB9, 0x8F], passed
					else if(block == DEF_EXFONT_BLOCK_CJK_COMPATIBILITY_FORMS)
					{
						if(character->buffer[1] == 0xB8 && character->buffer[2] >= 0xB0)
							offset = character->buffer[2] - 0xB0;
						else if(character->buffer[1] == 0xB9)
							offset = character->buffer[2] - 0x80 + 16;
						else
							unknown = true;
					}
					//halfwidth_and_fullwidth_forms : U+FF00[0xEF, 0xBC, 0x80]~U+FFEF[0xEF, 0xBF, 0xAF], passed
					else if(block == DEF_EXFONT_BLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS)
					{
						if(character->buffer[1] == 0xBC)
							offset = character->buffer[2] - 0x80;
						else if(character->buffer[1] == 0xBD)
							offset = character->buffer[2] - 0x80 + 64;
						else if(character->buffer[1] == 0xBE)
							offset = character->buffer[2] - 0x80 + 128;
						else if(character->buffer[1] == 0xBF)
							offset = character->buffer[2] - 0x80 + 192;
						else
							unknown = true;
					}
					else
						unknown = true;
				}
			}
		}
		else if(length == 4)
		{
			if(character->buffer[0] != 0xF0 || character->buffer[1] != 0x9F
			|| character->buffer[3] < 0x80 || character->buffer[3] > 0xBf)
				unknown = true;
			else
			{
				//miscellaneous_symbols_and_pictographs : U+1F300[0xF0, 0x9F, 0x8C, 0x80]~U+1F5FF[0xF0, 0x9F, 0x97, 0xBF], passed
				if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS)
				{
					if(character->buffer[2] == 0x8C)
						offset = character->buffer[3] - 0x80;
					else if(character->buffer[2] == 0x8D)
						offset = character->buffer[3] - 0x80 + 64;
					else if(character->buffer[2] == 0x8E)
						offset = character->buffer[3] - 0x80 + 128;
					else if(character->buffer[2] == 0x8F)
						offset = character->buffer[3] - 0x80 + 192;
					else if(character->buffer[2] == 0x90)
						offset = character->buffer[3] - 0x80 + 256;
					else if(character->buffer[2] == 0x91)
						offset = character->buffer[3] - 0x80 + 320;
					else if(character->buffer[2] == 0x92)
						offset = character->buffer[3] - 0x80 + 384;
					else if(character->buffer[2] == 0x93)
						offset = character->buffer[3] - 0x80 + 448;
					else if(character->buffer[2] == 0x94)
						offset = character->buffer[3] - 0x80 + 512;
					else if(character->buffer[2] == 0x95)
						offset = character->buffer[3] - 0x80 + 576;
					else if(character->buffer[2] == 0x96)
						offset = character->buffer[3] - 0x80 + 640;
					else if(character->buffer[2] == 0x97)
						offset = character->buffer[3] - 0x80 + 704;
					else
						unknown = true;
				}
				//emoticons : U+1F600[0xF0, 0x9F, 0x98, 0x80]~U+1F64F[0xF0, 0x9F, 0x99, 0x8F], passed
				else if(block == DEF_EXFONT_BLOCK_EMOTICONS)
				{
					if(character->buffer[2] == 0x98)
						offset = character->buffer[3] - 0x80;
					else if(character->buffer[2] == 0x99 && character->buffer[3] <= 0x8F)
						offset = character->buffer[3] - 0x80 + 64;
					else
						unknown = true;
				}
			}
		}
		else
			unknown = true;
	}

	if(unknown || (base_index + offset) >= DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(util_exfont_font_images))
	{
		base_index = 0;
		offset = 0;
	}

	if(util_exfont_font_images[base_index + offset].subtex->height != 0)
		width_rate = ((double)util_exfont_font_images[base_index + offset].subtex->width / util_exfont_font_images[base_index + offset].subtex->height);

	x_size = ((base_size * width_rate) * x_scale);
	y_size = (base_size * y_scale);
	if(!size_only)
	{
		Draw_image_data font = { 0, };
		font.c2d = util_exfont_font_images[base_index + offset];
		Draw_texture(&font, abgr8888, texture_x, texture_y, x_size, y_size);
	}

	*out_width = x_size;
	*out_height = y_size;
}

static uint32_t Exfont_load_exfont(uint16_t exfont_id)
{
	uint32_t result = DEF_ERR_OTHER;
	Str_data path = { 0, };

	if (exfont_id >= DEF_EXFONT_NUM_OF_FONT_NAME)
		goto invalid_arg;

	if(Util_str_init(&path) != DEF_SUCCESS)
		goto out_of_memory;

	if(exfont_id == DEF_EXFONT_BLOCK_CJK_UNIFIED_IDEOGRAPHS)
	{
		uint32_t start_pos = util_exfont_font_start_num[exfont_id];

		for(uint8_t i = 0; i < 21; i++)
		{
			if(util_exfont_font_cjk_unified_ideographs_characters[i] == 0)
			{
				if(i != 0)
					util_exfont_loaded_external_font[exfont_id] = true;

				break;
			}

			Util_str_format(&path, "romfs:/gfx/font/%s_%" PRIu8 "_font.t3x", DEF_STR_NEVER_NULL(&util_exfont_font_name[exfont_id]), i);
			util_exfont_cjk_unified_ideographs_texture_num[i] = Draw_get_free_sheet_num();
			result = Draw_load_texture(path.buffer, util_exfont_cjk_unified_ideographs_texture_num[i], util_exfont_font_images, start_pos, util_exfont_font_cjk_unified_ideographs_characters[i]);

			if (result == DEF_SUCCESS)
			{
				for (uint16_t k = 0; k < util_exfont_font_cjk_unified_ideographs_characters[i]; k++)
				{
					C3D_TexSetFilter(util_exfont_font_images[start_pos + k].tex, GPU_LINEAR, GPU_LINEAR);
					C3D_TexSetWrap(util_exfont_font_images[start_pos + k].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
				}
				if(i == 20)
					util_exfont_loaded_external_font[exfont_id] = true;
			}
			else
			{
				Exfont_unload_exfont(exfont_id);
				util_exfont_loaded_external_font[exfont_id] = false;
				break;
			}

			start_pos += util_exfont_font_cjk_unified_ideographs_characters[i];
		}
	}
	else if(exfont_id == DEF_EXFONT_BLOCK_HANGUL_SYLLABLES)
	{
		uint32_t start_pos = util_exfont_font_start_num[exfont_id];

		for(uint8_t i = 0; i < 11; i++)
		{
			if(util_exfont_font_hangul_syllables_characters[i] == 0)
			{
				if(i != 0)
					util_exfont_loaded_external_font[exfont_id] = true;

				break;
			}

			Util_str_format(&path, "romfs:/gfx/font/%s_%" PRIu8 "_font.t3x", DEF_STR_NEVER_NULL(&util_exfont_font_name[exfont_id]), i);
			util_exfont_hangul_syllables_texture_num[i] = Draw_get_free_sheet_num();
			result = Draw_load_texture(path.buffer, util_exfont_hangul_syllables_texture_num[i], util_exfont_font_images, start_pos, util_exfont_font_hangul_syllables_characters[i]);

			if (result == DEF_SUCCESS)
			{
				for (uint16_t k = 0; k < util_exfont_font_hangul_syllables_characters[i]; k++)
				{
					C3D_TexSetFilter(util_exfont_font_images[start_pos + k].tex, GPU_LINEAR, GPU_LINEAR);
					C3D_TexSetWrap(util_exfont_font_images[start_pos + k].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
				}
				if(i == 10)
					util_exfont_loaded_external_font[exfont_id] = true;
			}
			else
			{
				Exfont_unload_exfont(exfont_id);
				util_exfont_loaded_external_font[exfont_id] = false;
				break;
			}

			start_pos += util_exfont_font_hangul_syllables_characters[i];
		}
	}
	else
	{
		Util_str_format(&path, "romfs:/gfx/font/%s_font.t3x", DEF_STR_NEVER_NULL(&util_exfont_font_name[exfont_id]));
		util_exfont_texture_num[exfont_id] = Draw_get_free_sheet_num();
		result = Draw_load_texture(path.buffer, util_exfont_texture_num[exfont_id], util_exfont_font_images, util_exfont_font_start_num[exfont_id], util_exfont_font_characters[exfont_id]);

		if (result == DEF_SUCCESS)
		{
			for (uint16_t i = 0; i < util_exfont_font_characters[exfont_id]; i++)
			{
				C3D_TexSetFilter(util_exfont_font_images[util_exfont_font_start_num[exfont_id] + i].tex, GPU_LINEAR, GPU_LINEAR);
				C3D_TexSetWrap(util_exfont_font_images[util_exfont_font_start_num[exfont_id] + i].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
			}
			util_exfont_loaded_external_font[exfont_id] = true;
		}
		else
		{
			Exfont_unload_exfont(exfont_id);
			util_exfont_loaded_external_font[exfont_id] = false;
		}
	}

	Util_str_free(&path);
	return result;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

static void Exfont_unload_exfont(uint16_t exfont_id)
{
	if (exfont_id < DEF_EXFONT_NUM_OF_FONT_NAME)
	{
		util_exfont_loaded_external_font[exfont_id] = false;

		for (uint32_t j = util_exfont_font_start_num[exfont_id]; j < util_exfont_font_characters[exfont_id]; j++)
			util_exfont_font_images[j].tex = NULL;

		if(exfont_id == DEF_EXFONT_BLOCK_CJK_UNIFIED_IDEOGRAPHS)
		{
			for(uint8_t i = 0; i < 21; i++)
				Draw_free_texture(util_exfont_cjk_unified_ideographs_texture_num[i]);
		}
		else if(exfont_id == DEF_EXFONT_BLOCK_HANGUL_SYLLABLES)
		{
			for(uint8_t i = 0; i < 11; i++)
				Draw_free_texture(util_exfont_hangul_syllables_texture_num[i]);
		}
		else
			Draw_free_texture(util_exfont_texture_num[exfont_id]);
	}
}

static void Exfont_load_font_callback(void)
{
	if(util_exfont_init)
	{
		if(util_exfont_load_external_font_request)
		{
			for(uint16_t i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
			{
				if(util_exfont_request_external_font_state[i] && !util_exfont_loaded_external_font[i])
				{
					uint32_t result = DEF_ERR_OTHER;

					DEF_LOG_RESULT_SMART(result, Exfont_load_exfont(i), (result == DEF_SUCCESS), result);
					Draw_set_refresh_needed(true);
				}
			}
			util_exfont_load_external_font_request = false;
		}
		else if(util_exfont_unload_external_font_request)
		{
			for(uint16_t i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
			{
				if(!util_exfont_request_external_font_state[i] && util_exfont_loaded_external_font[i])
				{
					Exfont_unload_exfont(i);
					Draw_set_refresh_needed(true);
				}
			}
			util_exfont_unload_external_font_request = false;
		}
	}
}
