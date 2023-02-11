#include "system/headers.hpp"

struct One_character
{
    char buffer[5];
};

bool util_exfont_loaded_external_font[DEF_EXFONT_NUM_OF_FONT_NAME];
bool util_exfont_request_external_font_state[DEF_EXFONT_NUM_OF_FONT_NAME];
bool util_exfont_load_external_font_request = false;
bool util_exfont_unload_external_font_request = false;
bool util_exfont_init = false;
int util_exfont_texture_num[DEF_EXFONT_NUM_OF_FONT_NAME];
int util_exfont_cjk_unified_ideographs_texture_num[21];
int util_exfont_hangul_syllables_texture_num[11];

std::string util_exfont_font_right_to_left_samples[257];
std::string util_exfont_font_name[DEF_EXFONT_NUM_OF_FONT_NAME];
std::string util_exfont_ignore_chars = "\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u200B\u200C\u200D\u200E\u200F";
std::string util_exfont_sample_one_byte[DEF_EXFONT_NUM_OF_ONE_BYTE_FONT] = { "\u0080", };
std::string util_exfont_samples_two_bytes[DEF_EXFONT_NUM_OF_TWO_BYTES_FONT] = { "\u0100", "\u0180", "\u0250", "\u02B0", "\u0300", "\u0370", "\u0400", "\u0500", "\u0530", "\u0590", "\u0600", "\u0700", };
std::string util_exfont_samples_three_bytes[DEF_EXFONT_NUM_OF_THREE_BYTES_FONT] = { "\u0980", "\u0A80", "\u0C00", "\u0C80", "\u0D00", "\u0E00", "\u0E80", "\u0F00",
"\u1000", "\u1100", "\u1680", "\u1D80", "\u1E00", "\u2000", "\u2070", "\u20A0", "\u2100", "\u2200", "\u2300", "\u2400", "\u2460", "\u2500", "\u2580", "\u25A0",
"\u2600", "\u2700", "\u27C0", "\u2980", "\u2C00", "\u3040", "\u30A0", "\u3100", "\u3400", "\uA000", "\uA490", "\uA4D0", "\uD7B0", "\uFE50", "\uFFF0", };
std::string util_exfont_samples_four_bytes[DEF_EXFONT_NUM_OF_FOUR_BYTES_FONT] = { "\U0001F600", "\U0001F650", };
C2D_Image util_exfont_font_images[41376];

int util_exfont_font_characters[DEF_EXFONT_NUM_OF_FONT_NAME] = 
{
    128,  96, 128, 208,  96,  80, 112, 144, 256,  48,  96, 112, 256, 128, 128, 128, 
    128, 128, 128, 128, 128, 256,  96, 640, 128,  64, 256, 112,  48,  48, 112, 256,
    256,  32, 160, 128,  32,  96, 256, 192, 128, 256,  64,  96,  96, 256, 20992, 1168,
    64, 11184, 32, 240, 768,  80,
};
int util_exfont_font_cjk_unified_ideographs_characters[21] = 
{
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
    1024, 1024, 1024, 1024,  512,
};
int util_exfont_font_hangul_syllables_characters[11] = 
{
    1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,  944,
};
int util_exfont_num_of_right_left_charcters = 0;
int util_exfont_font_start_num[DEF_EXFONT_NUM_OF_FONT_NAME];

Result_with_string Exfont_load_exfont(int exfont_num);
void Exfont_unload_exfont(int exfont_num);

void Exfont_load_font_callback(void)
{
    Result_with_string result;

    if(util_exfont_init)
    {
        if(util_exfont_load_external_font_request)
        {
            for(int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
            {
                if(util_exfont_request_external_font_state[i] && !util_exfont_loaded_external_font[i])
                {
                    result = Exfont_load_exfont(i);
					Util_log_save(DEF_EXFONT_LOAD_FONT_CALLBACK_STR, "Exfont_load_exfont()..." + result.string + result.error_description, result.code);
                    var_need_reflesh = true;
                }
            }
            util_exfont_load_external_font_request = false;
        }
        else if(util_exfont_unload_external_font_request)
        {
            for(int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
            {
                if(!util_exfont_request_external_font_state[i] && util_exfont_loaded_external_font[i])
                {
					Exfont_unload_exfont(i);
                    var_need_reflesh = true;
                }
            }
            util_exfont_unload_external_font_request = false;
        }
    }
}

Result_with_string Exfont_init(void)
{
    int characters = 0;
    u8* fs_buffer = NULL;
    u32 read_size = 0;
    Result_with_string result;

    if(util_exfont_init)
        goto already_inited;

    result = Util_file_load_from_rom("font_name.txt", "romfs:/gfx/msg/", &fs_buffer, 0x2000, &read_size);
    if(result.code != 0)
        goto api_failed;

    result = Util_parse_file((char*)fs_buffer, DEF_EXFONT_NUM_OF_FONT_NAME, util_exfont_font_name);
    if(result.code != 0)
        goto api_failed;

    Util_safe_linear_free(fs_buffer);
    fs_buffer = NULL;

    result = Util_file_load_from_rom("font_right_to_left_samples.txt", "romfs:/gfx/font/sample/", &fs_buffer, 0x8000, &read_size);
    if(result.code != 0)
        goto api_failed;

    Exfont_text_parse_ignore_rtl((char*)fs_buffer, util_exfont_font_right_to_left_samples, 256, &characters);
    Util_safe_linear_free(fs_buffer);
    fs_buffer = NULL;

    util_exfont_num_of_right_left_charcters = characters - 1;

    util_exfont_font_start_num[0] = 0;
    for (int i = 1; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
        util_exfont_font_start_num[i] = util_exfont_font_start_num[i - 1] + util_exfont_font_characters[i - 1];

    for(int i = 0; i < 21; i++)
        util_exfont_cjk_unified_ideographs_texture_num[i] = -1;
    for(int i = 0; i < 11; i++)
        util_exfont_hangul_syllables_texture_num[i] = -1;

    for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
    {
        util_exfont_loaded_external_font[i] = false;
        util_exfont_request_external_font_state[i] = false;
        util_exfont_texture_num[i] = -1;
    }

	if(!Menu_add_worker_thread_callback(Exfont_load_font_callback))
	{
		result.error_description = "[Error] Menu_add_worker_thread_callback() failed. ";
		goto other;
	}

    util_exfont_init = true;
    return result;
    
    already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

    api_failed:
    Util_safe_linear_free(fs_buffer);
    fs_buffer = NULL;
	return result;

	other:
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

void Exfont_exit(void)
{
    if(!util_exfont_init)
        return;

    util_exfont_init = false;
    Menu_remove_worker_thread_callback(Exfont_load_font_callback);
}

std::string Exfont_query_external_font_name(int exfont_num)
{
    if(!util_exfont_init)
        return "";
    else if (exfont_num >= 0 && exfont_num < DEF_EXFONT_NUM_OF_FONT_NAME)
        return util_exfont_font_name[exfont_num];
    else
        return "";
}

bool Exfont_is_loaded_external_font(int exfont_num)
{
    if(!util_exfont_init)
        return false;
    else if (exfont_num >= 0 && exfont_num < DEF_EXFONT_NUM_OF_FONT_NAME)
        return util_exfont_loaded_external_font[exfont_num];
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

void Exfont_set_external_font_request_state(int exfont_num, bool flag)
{
    if(!util_exfont_init)
        return;

    if (exfont_num >= 0 && exfont_num < DEF_EXFONT_NUM_OF_FONT_NAME)
        util_exfont_request_external_font_state[exfont_num] = flag;
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

void Exfont_text_parse_ignore_rtl(std::string source_string, std::string part_string[], int max_loop, int* out_element)
{
    int source_string_length = 0;
    int std_index = 0;
    int offset = 0;
    char* source_string_char = NULL;

    if(!part_string || max_loop <= 0 || !out_element)
        return;
    
    *out_element = 0;
    source_string_length = source_string.length();
    source_string_char = (char*)source_string.c_str();

    for (int k = 0; k < max_loop; k++)
    {
        int parse_string_length = mblen((source_string_char + offset), 4);

        if (offset >= source_string_length)
            break;
        else if (parse_string_length >= 1)
        {
            part_string[std_index] = source_string.substr(offset, parse_string_length);
            offset += parse_string_length;
            std_index++;
        }
        else
            offset++;
    }
    part_string[std_index] = "\u0000";
    *out_element = std_index;
}

void Exfont_text_parse(std::string source_string, std::string part_string[], int max_loop, int* out_element)
{
    short* rtl_index_list = NULL;
    int source_string_length = 0;
    int std_index = 0;
    int rtl_index = 0;
    int offset = 0;
    char* source_string_char = NULL;
    std::string right_to_left_sample[3] = { "\u05BD", "\u0700", "\u200F", };
    One_character* cache_part_string = NULL;

    if(source_string.length() <= 0 || !part_string || max_loop <= 0 || !out_element)
        return;
    
    cache_part_string = (One_character*)malloc(sizeof(One_character) * source_string.length());
    rtl_index_list = (short*)malloc(sizeof(short) * source_string.length());
    if(!cache_part_string || !rtl_index_list)
    {
        free(cache_part_string);
        free(rtl_index_list);
        cache_part_string = NULL;
        rtl_index_list = NULL;
    }

    *out_element = 0;
    source_string_length = source_string.length();
    source_string_char = (char*)source_string.c_str();

    for (int k = 0; k < (max_loop - 1); k++)
    {
        int parse_string_length = 0;
        if(source_string_length - offset <= 0)
            break;

        parse_string_length = mblen((source_string_char + offset), (source_string_length - offset >= 4 ? 4 : (source_string_length - offset)));
        if (parse_string_length >= 1)
        {
            bool rtl_found = false;
            int give_up_pos = 0;
            One_character one_char;

            memcpy(one_char.buffer, (source_string.c_str() + offset), parse_string_length);
            one_char.buffer[parse_string_length] = 0x0;

            //Sort for RTL characters, currently only Arabic and Armenian are supported.
            if((parse_string_length == 2 && memcmp((void*)one_char.buffer, (void*)right_to_left_sample[0].c_str(), 0x2) > 0
            && memcmp((void*)one_char.buffer, (void*)right_to_left_sample[1].c_str(), 0x2) < 0))
            {
                int increment = util_exfont_num_of_right_left_charcters / 10;

                for(int i = 0; i < util_exfont_num_of_right_left_charcters; i += increment)
                {
                    int memcmp_result = 0;
                    if(increment == -1 && (i < 0 || i == give_up_pos))
                        break;

                    memcmp_result = memcmp((void*)one_char.buffer, (void*)util_exfont_font_right_to_left_samples[i].c_str(), 0x2);
                    if (memcmp_result == 0)
                    {
                        rtl_found = true;
                        break;
                    }
                    else if(increment != -1 && memcmp_result < 0)
                    {
                        give_up_pos = i - increment;
                        increment = -1;
                    }
                }
            }
            else if(right_to_left_sample[2] == (char*)one_char.buffer)
                rtl_found = true;

            if (rtl_found)
            {
                memcpy(cache_part_string[rtl_index].buffer, one_char.buffer, parse_string_length + 1);
                rtl_index_list[rtl_index] = std_index;
                rtl_index++;
            }
            else
            {
                if(rtl_index > 0)
                {
                    for(int i = 0, m = (rtl_index - 1); i < rtl_index; i++, m--)
                        part_string[rtl_index_list[m]] = (char*)cache_part_string[i].buffer;

                    rtl_index = 0;
                }
                part_string[std_index] = (char*)one_char.buffer;
            }

            offset += parse_string_length;
            std_index++;
        }
        else
            offset++;
    }

    if(rtl_index > 0)
    {
        for(int i = 0, m = (rtl_index - 1); i < rtl_index; i++, m--)
            part_string[rtl_index_list[m]] = (char*)cache_part_string[i].buffer;

        rtl_index = 0;
    }

    free(rtl_index_list);
    free(cache_part_string);
    rtl_index_list = NULL;
    cache_part_string = NULL;
    part_string[std_index] = "\u0000";
    *out_element = std_index;
}

void Exfont_draw_external_fonts(std::string* in_part_string, int num_of_characters, float texture_x, float texture_y, float texture_size_x, float texture_size_y, int abgr8888, float* out_width, float* out_height, bool size_only)
{
    double interval_offset = 1;
    double x_offset = 0.0;
    int block = -1;

    if(!util_exfont_init)
        return;

    if(!util_exfont_loaded_external_font[DEF_EXFONT_BLOCK_BASIC_LATIN])
        return;

    if(!in_part_string || num_of_characters <= 0 || !out_width || !out_height)
        return;

    *out_width = 0;
    *out_height = 0;

    for (int s = 0; s < num_of_characters; s++)
    {
        bool unknown = true;
        int base_index = 0;
        int offset = 0;
        double x_size = 0;
        double y_size = 0;

        if (in_part_string[s].length() <= 0)
            break;

        if(in_part_string[s].c_str()[0] == 0x0)
            break;

        block = -1;

        if (in_part_string[s].length() == 1)
        {
            if (memcmp((void*)in_part_string[s].c_str(), (void*)util_exfont_sample_one_byte[0].c_str(), 0x1) < 0)
                block = 0;
        }
        else if (in_part_string[s].length() == 2)
        {
            for (int i = 0; i < DEF_EXFONT_NUM_OF_TWO_BYTES_FONT; i++)
            {
                if (memcmp((void*)in_part_string[s].c_str(), (void*)util_exfont_samples_two_bytes[i].c_str(), 0x2) < 0)
                {
                    block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT;
                    break;
                }
            }
        }
        else if (in_part_string[s].length() == 3)
        {
            int start_pos = -1;
            int end_pos = 0;
            int loop = DEF_EXFONT_NUM_OF_THREE_BYTES_FONT / 10;

            for (int i = 1; i <= loop; i++)
            {
                int index = (i * 10) - 1;
                if (memcmp((void*)in_part_string[s].c_str(), (void*)util_exfont_samples_three_bytes[index].c_str(), 0x3) < 0)
                {
                    start_pos = index - 9;
                    end_pos = start_pos + 10;
                    if(end_pos > DEF_EXFONT_NUM_OF_THREE_BYTES_FONT)
                        end_pos = DEF_EXFONT_NUM_OF_THREE_BYTES_FONT;

                    break;
                }
            }

            if(start_pos == -1)
            {
                start_pos = loop * 10;
                end_pos = DEF_EXFONT_NUM_OF_THREE_BYTES_FONT;
            }

            for (int i = start_pos; i < end_pos; i++)
            {
                if (memcmp((void*)in_part_string[s].c_str(), (void*)util_exfont_samples_three_bytes[i].c_str(), 0x3) < 0)
                {
                    block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT + DEF_EXFONT_NUM_OF_TWO_BYTES_FONT;
                    break;
                }
            }
        }
        else if (in_part_string[s].length() == 4)
        {
            for (int i = 0; i < DEF_EXFONT_NUM_OF_FOUR_BYTES_FONT; i++)
            {
                if (memcmp((void*)in_part_string[s].c_str(), (void*)util_exfont_samples_four_bytes[i].c_str(), 0x4) < 0)
                {
                    block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT + DEF_EXFONT_NUM_OF_TWO_BYTES_FONT + DEF_EXFONT_NUM_OF_THREE_BYTES_FONT;
                    break;
                }
            }
        }

        if (block == DEF_EXFONT_BLOCK_GENERAL_PUNCTUATION)
        {
            if (!(util_exfont_ignore_chars.find(in_part_string[s]) == std::string::npos))
            {
                unknown = false;
                block = -1;
            }
        }

        if (block != -1 && util_exfont_loaded_external_font[block])
        {
            unknown = false;
            base_index = util_exfont_font_start_num[block];

            if(in_part_string[s].length() == 1)
            {
                if(in_part_string[s].c_str()[0] > 0x7F)
                    unknown = true;
                else
                {
                    //basic latin : U+0000[0x00]~U+007F[0x7F], passed
                    if(block == DEF_EXFONT_BLOCK_BASIC_LATIN)
                        offset = in_part_string[s].c_str()[0];
                    else
                        unknown = true;
                }
            }
            else if(in_part_string[s].length() == 2)
            {
                if(in_part_string[s].c_str()[1] < 0x80 || in_part_string[s].c_str()[1] > 0xBF)
                    unknown = true;
                else
                {
                    //latin 1 supplement : U+0080[0xC2, 0x80]~U+00FF[0xC3, 0xBF], passed
                    //U+0080[0xC2, 0x80]~U+009F[0xC2, 0x9F] are invalid character.
                    if(block == DEF_EXFONT_BLOCK_LATIN_1_SUPPLEMENT)
                    {
                        if(in_part_string[s].c_str()[0] == 0xC2)
                            offset = in_part_string[s].c_str()[1] - 0xA0;
                        else if(in_part_string[s].c_str()[0] == 0xC3)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 32;
                        else
                            unknown = true;
                    }
                    //latin extended a : U+0100[0xC4, 0x80]~U+017F[0xC5, 0xBF], passed
                    else if(block == DEF_EXFONT_BLOCK_LATIN_EXTENDED_A)
                    {
                        if(in_part_string[s].c_str()[0] == 0xC4)
                            offset = in_part_string[s].c_str()[1] - 0x80;
                        else if(in_part_string[s].c_str()[0] == 0xC5)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 64;
                        else
                            unknown = true;
                    }
                    //latin extended b : U+0180[0xC6, 0x80]~U+024F[0xC9, 0x8F], passed
                    else if(block == DEF_EXFONT_BLOCK_LATIN_EXTENDED_B)
                    {
                        if(in_part_string[s].c_str()[0] == 0xC6)
                            offset = in_part_string[s].c_str()[1] - 0x80;
                        else if(in_part_string[s].c_str()[0] == 0xC7)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 64;
                        else if(in_part_string[s].c_str()[0] == 0xC8)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 128;
                        else if(in_part_string[s].c_str()[0] == 0xC9 && in_part_string[s].c_str()[1] <= 0x8F)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 192;
                        else
                            unknown = true;
                    }
                    //ipa extensions : U+0250[0xC9, 0x90]~U+02AF[0xCA, 0xAF], passed
                    else if(block == DEF_EXFONT_BLOCK_IPA_EXTENSIONS)
                    {
                        if(in_part_string[s].c_str()[0] == 0xC9 && in_part_string[s].c_str()[1] >= 0x90)
                            offset = in_part_string[s].c_str()[1] - 0x90;
                        else if(in_part_string[s].c_str()[0] == 0xCA && in_part_string[s].c_str()[1] <= 0xAF)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 48;
                        else
                            unknown = true;
                    }
                    //spacing_modifier_letters : U+02B0[0xCA, 0xB0]~U+02FF[0xCB, 0xBF], passed
                    else if(block == DEF_EXFONT_BLOCK_SPACING_MODIFIER_LETTERS)
                    {
                        if(in_part_string[s].c_str()[0] == 0xCA && in_part_string[s].c_str()[1] >= 0xB0)
                            offset = in_part_string[s].c_str()[1] - 0xB0;
                        else if(in_part_string[s].c_str()[0] == 0xCB)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 16;
                        else
                            unknown = true;
                    }
                    //combining_diacritical_marks : U+0300[0xCC, 0x80]~U+036F[0xCD, 0xAF], passed
                    else if(block == DEF_EXFONT_BLOCK_COMBINING_DIACRITICAL_MARKS)
                    {
                        if(in_part_string[s].c_str()[0] == 0xCC)
                            offset = in_part_string[s].c_str()[1] - 0x80;
                        else if(in_part_string[s].c_str()[0] == 0xCD && in_part_string[s].c_str()[1] <= 0xAF)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 64;
                        else
                            unknown = true;
                    }
                    //greek and coptic : U+0370[0xCD, 0xB0]~U+03FF[0xCF, 0xBF], passed
                    else if(block == DEF_EXFONT_BLOCK_GREEK_AND_COPTIC)
                    {
                        if(in_part_string[s].c_str()[0] == 0xCD && in_part_string[s].c_str()[1] >= 0xB0)
                            offset = in_part_string[s].c_str()[1] - 0xB0;
                        else if(in_part_string[s].c_str()[0] == 0xCE)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 16;
                        else if(in_part_string[s].c_str()[0] == 0xCF)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 80;
                        else
                            unknown = true;
                    }
                    //cyrillic : U+0400[0xD0, 0x80]~U+04FF[0xD3, 0xBF], passed
                    else if(block == DEF_EXFONT_BLOCK_CYRILLIC)
                    {
                        if(in_part_string[s].c_str()[0] == 0xD0)
                            offset = in_part_string[s].c_str()[1] - 0x80;
                        else if(in_part_string[s].c_str()[0] == 0xD1)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 64;
                        else if(in_part_string[s].c_str()[0] == 0xD2)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 128;
                        else if(in_part_string[s].c_str()[0] == 0xD3)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 192;
                        else
                            unknown = true;
                    }
                    //cyrillic supplement : U+0500[0xD4, 0x80]~U+052F[0xD4, 0xAF], passed
                    else if(block == DEF_EXFONT_BLOCK_CYRILLIC_SUPPLEMENT)
                    {
                        if(in_part_string[s].c_str()[0] == 0xD4 && in_part_string[s].c_str()[1] <= 0xAF)
                            offset = in_part_string[s].c_str()[1] - 0x80;
                        else
                            unknown = true;
                    }
                    //armenian : U+0530[0xD4, 0xB0]~U+058F[0xD6, 0x8F], passed
                    else if(block == DEF_EXFONT_BLOCK_ARMENIAN)
                    {
                        if(in_part_string[s].c_str()[0] == 0xD4 && in_part_string[s].c_str()[1] >= 0xB0)
                            offset = in_part_string[s].c_str()[1] - 0xB0;
                        else if(in_part_string[s].c_str()[0] == 0xD5)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 16;
                        else if(in_part_string[s].c_str()[0] == 0xD6 && in_part_string[s].c_str()[1] <= 0x8F)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 80;
                        else
                            unknown = true;
                    }
                    //hebrew : U+0590[0xD6, 0x90]~U+05FF[0xD7, 0xBF], passed
                    else if(block == DEF_EXFONT_BLOCK_HEBREW)
                    {
                        if(in_part_string[s].c_str()[0] == 0xD6 && in_part_string[s].c_str()[1] >= 0x90)
                            offset = in_part_string[s].c_str()[1] - 0x90;
                        else if(in_part_string[s].c_str()[0] == 0xD7)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 48;
                        else
                            unknown = true;
                    }
                    //arabic : U+0600[0xD8, 0x80]~U+06FF[0xDB, 0xBF], passed
                    else if(block == DEF_EXFONT_BLOCK_ARABIC)
                    {
                        if(in_part_string[s].c_str()[0] == 0xD8)
                            offset = in_part_string[s].c_str()[1] - 0x80;
                        else if(in_part_string[s].c_str()[0] == 0xD9)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 64;
                        else if(in_part_string[s].c_str()[0] == 0xDA)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 128;
                        else if(in_part_string[s].c_str()[0] == 0xDB)
                            offset = in_part_string[s].c_str()[1] - 0x80 + 192;
                        else
                            unknown = true;
                    }
                    else
                        unknown = true;
                }
            }
            else if(in_part_string[s].length() == 3)
            {
                if(in_part_string[s].c_str()[0] < 0xE0 || in_part_string[s].c_str()[0] > 0xEF
                || in_part_string[s].c_str()[2] < 0x80 || in_part_string[s].c_str()[2] > 0xBF)
                    unknown = true;
                else
                {
                    if(block <= DEF_EXFONT_BLOCK_GEORGIAN)
                    {
                        //devanagari : U+0900[0xE0, 0xA4, 0x80]~U+097F[0xE0, 0xA5, 0xBF], passed
                        if(block == DEF_EXFONT_BLOCK_DEVANAGARI)
                        {
                            if(in_part_string[s].c_str()[1] == 0xA4)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xA5)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //gurmukhi : U+0A00[0xE0, 0xA8, 0x80]~U+0A7F[0xE0, 0xA9, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_GURMUKHI)
                        {
                            if(in_part_string[s].c_str()[1] == 0xA8)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xA9)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //tamil : U+0B80[0xE0, 0xAE, 0x80]~U+0BFF[0xE0, 0xAF, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_TAMIL)
                        {
                            if(in_part_string[s].c_str()[1] == 0xAE)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xAF)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //telugu : U+0C00[0xE0, 0xB0, 0x80]~U+0C7F[0xE0, 0xB1, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_TELUGU)
                        {
                            if(in_part_string[s].c_str()[1] == 0xB0)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xB1)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //kannada : U+0C80[0xE0, 0xB2, 0x80]~U+0CFF[0xE0, 0xB3, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_KANNADA)
                        {
                            if(in_part_string[s].c_str()[1] == 0xB2)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xB3)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //sinhala : U+0D80[0xE0, 0xB6, 0x80]~U+0DFF[0xE0, 0xB7, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_SINHALA)
                        {
                            if(in_part_string[s].c_str()[1] == 0xB6)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xB7)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //thai : U+0E00[0xE0, 0xB8, 0x80]~U+0E7F[0xE0, 0xB9, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_THAI)
                        {
                            if(in_part_string[s].c_str()[1] == 0xB8)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xB9)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //lao : U+0E80[0xE0, 0xBA, 0x80]~U+0EFF[0xE0, 0xBB, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_LAO)
                        {
                            if(in_part_string[s].c_str()[1] == 0xBA)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xBB)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //tibetan : U+0F00[0xE0, 0xBC, 0x80]~U+0FFF[0xE0, 0xBF, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_TIBETAN)
                        {
                            if(in_part_string[s].c_str()[1] == 0xBC)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xBD)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0xBE)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0xBF)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else
                                unknown = true;
                        }
                        //georgian : U+10A0[0xE1, 0x82, 0xA0]~U+10FF[0xE1, 0x83, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_GEORGIAN)
                        {
                            if(in_part_string[s].c_str()[1] == 0x82 && in_part_string[s].c_str()[2] >= 0xA0)
                                offset = in_part_string[s].c_str()[2] - 0xA0;
                            else if(in_part_string[s].c_str()[1] == 0x83)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 32;
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
                            if(in_part_string[s].c_str()[1] == 0x90)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x91)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0x92)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0x93)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else if(in_part_string[s].c_str()[1] == 0x94)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 256;
                            else if(in_part_string[s].c_str()[1] == 0x95)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 320;
                            else if(in_part_string[s].c_str()[1] == 0x96)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 384;
                            else if(in_part_string[s].c_str()[1] == 0x97)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 448;
                            else if(in_part_string[s].c_str()[1] == 0x98)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 512;
                            else if(in_part_string[s].c_str()[1] == 0x99)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 576;
                            else
                                unknown = true;
                        }
                        //phonetic_extensions : U+1D00[0xE1, 0xB4, 0x80]~U+1D7F[0xE1, 0xB5, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_PHONETIC_EXTENSIONS)
                        {
                            if(in_part_string[s].c_str()[1] == 0xB4)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xB5)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //combining_diacritical_marks_supplement : U+1DC0[0xE1, 0xB7, 0x80]~U+1DFF[0xE1, 0xB7, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_COMBINING_DIACRITICAL_MARKS_SUPPLEMENT)
                        {
                            if(in_part_string[s].c_str()[1] == 0xB7)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else
                                unknown = true;
                        }
                        //greek_extended : U+1F00[0xE1, 0xBC, 0x80]~U+1FFF[0xE1, 0xBF, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_GREEK_EXTENDED)
                        {
                            if(in_part_string[s].c_str()[1] == 0xBC)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xBD)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0xBE)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0xBF)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else
                                unknown = true;
                        }
                        //general_punctuation : U+2000[0xE2, 0x80, 0x80]~U+206F[0xE2, 0x81, 0xAF], passed
                        else if(block == DEF_EXFONT_BLOCK_GENERAL_PUNCTUATION)
                        {
                            if(in_part_string[s].c_str()[1] == 0x80)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x81 && in_part_string[s].c_str()[2] <= 0xAF)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //superscripts_and_subscripts : U+2070[0xE2, 0x81, 0xB0]~U+209F[0xE2, 0x82, 0x9F], passed
                        else if(block == DEF_EXFONT_BLOCK_SUPERSCRIPTS_AND_SUBSCRIPTS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x81 && in_part_string[s].c_str()[2] >= 0xB0)
                                offset = in_part_string[s].c_str()[2] - 0xB0;
                            else if(in_part_string[s].c_str()[1] == 0x82 && in_part_string[s].c_str()[2] <= 0x9F)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 16;
                            else
                                unknown = true;
                        }
                        //combining_diacritical_marks_for_symbols : U+20D0[0xE2, 0x83, 0x90]~U+20FF[0xE2, 0x83, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x83 && in_part_string[s].c_str()[2] >= 0x90)
                                offset = in_part_string[s].c_str()[2] - 0x90;
                            else
                                unknown = true;
                        }
                        //arrows : U+2190[0xE2, 0x86, 0x90]~U+21FF[0xE2, 0x87, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_ARROWS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x86 && in_part_string[s].c_str()[2] >= 0x90)
                                offset = in_part_string[s].c_str()[2] - 0x90;
                            else if(in_part_string[s].c_str()[1] == 0x87)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 48;
                            else
                                unknown = true;
                        }
                        //mathematical_operators : U+2200[0xE2, 0x88, 0x80]~U+22FF[0xE2, 0x8B, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_MATHEMATICAL_OPERATORS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x88)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x89)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0x8A)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0x8B)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else
                                unknown = true;
                        }
                        //miscellaneous_technical : U+2300[0xE2, 0x8C, 0x80]~U+23FF[0xE2, 0x8F, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_TECHNICAL)
                        {
                            if(in_part_string[s].c_str()[1] == 0x8C)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x8D)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0x8E)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0x8F)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
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
                            if(in_part_string[s].c_str()[1] == 0x91 && in_part_string[s].c_str()[2] <= 0x9F)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else
                                unknown = true;
                        }
                        //enclosed_alphanumerics : U+2460[0xE2, 0x91, 0xA0]~U+24FF[0xE2, 0x93, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_ENCLOSED_ALPHANUMERICS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x91 && in_part_string[s].c_str()[2] >= 0xA0)
                                offset = in_part_string[s].c_str()[2] - 0xA0;
                            else if(in_part_string[s].c_str()[1] == 0x92)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 32;
                            else if(in_part_string[s].c_str()[1] == 0x93)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 96;
                            else
                                unknown = true;
                        }
                        //box_drawing : U+2500[0xE2, 0x94, 0x80]~U+257F[0xE2, 0x95, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_BOX_DRAWING)
                        {
                            if(in_part_string[s].c_str()[1] == 0x94)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x95)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //block_elements : U+2580[0xE2, 0x96, 0x80]~U+259F[0xE2, 0x96, 0x9F], passed
                        else if(block == DEF_EXFONT_BLOCK_BLOCK_ELEMENTS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x96 && in_part_string[s].c_str()[2] <= 0x9F)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else
                                unknown = true;
                        }
                        //geometric_shapes : U+25A0[0xE2, 0x96, 0xA0]~U+25FF[0xE2, 0x97, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_GEOMETRIC_SHAPES)
                        {
                            if(in_part_string[s].c_str()[1] == 0x96 && in_part_string[s].c_str()[2] >= 0xA0)
                                offset = in_part_string[s].c_str()[2] - 0xA0;
                            else if(in_part_string[s].c_str()[1] == 0x97)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 32;
                            else
                                unknown = true;
                        }
                        //miscellaneous_symbols : U+2600[0xE2, 0x98, 0x80]~U+26FF[0xE2, 0x9B, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_SYMBOLS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x98)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x99)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0x9A)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0x9B)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else
                                unknown = true;
                        }
                        //dingbats : U+2700[0xE2, 0x9C, 0x80]~U+27BF[0xE2, 0x9E, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_DINGBATS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x9C)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x9D)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0x9E)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else
                                unknown = true;
                        }
                        //supplemental_arrows_b : U+2900[0xE2, 0xA4, 0x80]~U+297F[0xE2, 0xA5, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_SUPPLEMENTAL_ARROWS_B)
                        {
                            if(in_part_string[s].c_str()[1] == 0xA4)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xA5)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //miscellaneous_symbols_and_arrows : U+2B00[0xE2, 0xAC, 0x80]~U+2BFF[0xE2, 0xAF, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_SYMBOLS_AND_ARROWS)
                        {
                            if(in_part_string[s].c_str()[1] == 0xAC)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xAD)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0xAE)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0xAF)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else
                                unknown = true;
                        }
                        //cjk_symbol_and_punctuation : U+3000[0xE3, 0x80, 0x80]~U+303F[0xE3, 0x80, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_CJK_SYMBOL_AND_PUNCTUATION)
                        {
                            if(in_part_string[s].c_str()[1] == 0x80)
                                offset = in_part_string[s].c_str()[2] - 0x80;
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
                            if(in_part_string[s].c_str()[1] == 0x81)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x82 && in_part_string[s].c_str()[2] <= 0x9F)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else
                                unknown = true;
                        }
                        //katakana : U+30A0[0xE3, 0x82, 0xA0]~U+30FF[0xE3, 0x83, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_KATAKANA)
                        {
                            if(in_part_string[s].c_str()[1] == 0x82 && in_part_string[s].c_str()[2] >= 0xA0)
                                offset = in_part_string[s].c_str()[2] - 0xA0;
                            else if(in_part_string[s].c_str()[1] == 0x83)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 32;
                            else
                                unknown = true;
                        }
                        //cjk_compatibility : U+3300[0xE3, 0x8C, 0x80]~U+33FF[0xE3, 0x8F, 0xBF], passed
                        else if(block == DEF_EXFONT_BLOCK_CJK_COMPATIBILITY)
                        {
                            if(in_part_string[s].c_str()[1] == 0x8C)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x8D)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0x8E)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0x8F)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else
                                unknown = true;
                        }
                        //cjk_unified_ideographs : U+4E00[0xE4, 0xB8, 0x80]~U+9FFF[0xE9, 0xBF, 0xBF]
                        else if(block == DEF_EXFONT_BLOCK_CJK_UNIFIED_IDEOGRAPHS)
                        {
                            if(in_part_string[s].c_str()[0] >= 0xE4 && in_part_string[s].c_str()[0] <= 0xE9)
                            {
                                int start_pos = 0;
                                int end_pos = 0;
                                int offset_cache = 0;

                                if(in_part_string[s].c_str()[1] < 0x90)
                                    start_pos = 0x80;
                                else if(in_part_string[s].c_str()[1] < 0xA0)
                                    start_pos = 0x90;
                                else if(in_part_string[s].c_str()[1] < 0xB0)
                                    start_pos = 0xA0;
                                else if(in_part_string[s].c_str()[1] < 0xC0)
                                    start_pos = 0xB0;

                                end_pos = start_pos + 0x10;

                                if(in_part_string[s].c_str()[0] == 0xE4)
                                    offset = 0;
                                else if(in_part_string[s].c_str()[0] == 0xE5)
                                    offset += 512;
                                else if(in_part_string[s].c_str()[0] == 0xE6)
                                    offset += 4608;
                                else if(in_part_string[s].c_str()[0] == 0xE7)
                                    offset += 8704;
                                else if(in_part_string[s].c_str()[0] == 0xE8)
                                    offset += 12800;
                                else if(in_part_string[s].c_str()[0] == 0xE9)
                                    offset += 16896;

                                unknown = true;
                                if(in_part_string[s].c_str()[0] == 0xE4)
                                {
                                    start_pos = 0xB8;
                                    end_pos = 0xC0;
                                    for(int i = start_pos; i < end_pos; i++)
                                    {
                                        if(in_part_string[s].c_str()[1] == i)
                                        {
                                            offset_cache = in_part_string[s].c_str()[2] - 0x80 + ((i - 0xB8) * 64);
                                            unknown = false;
                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    for(int i = start_pos; i < end_pos; i++)
                                    {
                                        if(in_part_string[s].c_str()[1] == i)
                                        {
                                            offset_cache = in_part_string[s].c_str()[2] - 0x80 + ((i - 0x80) * 64);
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
                            if(in_part_string[s].c_str()[1] == 0x80)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0x81)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0x82)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0x83)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else if(in_part_string[s].c_str()[1] == 0x84)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 256;
                            else if(in_part_string[s].c_str()[1] == 0x85)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 320;
                            else if(in_part_string[s].c_str()[1] == 0x86)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 384;
                            else if(in_part_string[s].c_str()[1] == 0x87)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 448;
                            else if(in_part_string[s].c_str()[1] == 0x88)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 512;
                            else if(in_part_string[s].c_str()[1] == 0x89)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 576;
                            else if(in_part_string[s].c_str()[1] == 0x8A)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 640;
                            else if(in_part_string[s].c_str()[1] == 0x8B)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 704;
                            else if(in_part_string[s].c_str()[1] == 0x8C)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 768;
                            else if(in_part_string[s].c_str()[1] == 0x8D)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 832;
                            else if(in_part_string[s].c_str()[1] == 0x8E)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 896;
                            else if(in_part_string[s].c_str()[1] == 0x8F)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 960;
                            else if(in_part_string[s].c_str()[1] == 0x90)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 1024;
                            else if(in_part_string[s].c_str()[1] == 0x91)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 1088;
                            else if(in_part_string[s].c_str()[1] == 0x92 && in_part_string[s].c_str()[2] <= 0x8F)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 1152;
                            else
                                unknown = true;
                        }
                        //yi_radicals : U+A490[0xEA, 0x92, 0x90]~U+A4CF[0xEA, 0x93, 0x8F], passed
                        else if(block == DEF_EXFONT_BLOCK_YI_RADICALS)
                        {
                            if(in_part_string[s].c_str()[1] == 0x92 && in_part_string[s].c_str()[2] >= 0x90)
                                offset = in_part_string[s].c_str()[2] - 0x90;
                            else if(in_part_string[s].c_str()[1] == 0x93)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 48;
                            else
                                unknown = true;
                        }
                        //hangul_syllables : U+AC00[0xEA, 0xB0, 0x80]~U+D7AF[0xED, 0x9E, 0xAF]
                        else if(block == DEF_EXFONT_BLOCK_HANGUL_SYLLABLES)
                        {
                            if(in_part_string[s].c_str()[0] >= 0xEA && in_part_string[s].c_str()[0] <= 0xED)
                            {
                                int start_pos = 0;
                                int end_pos = 0;
                                int offset_cache = 0;

                                if(in_part_string[s].c_str()[1] < 0x90)
                                    start_pos = 0x80;
                                else if(in_part_string[s].c_str()[1] < 0xA0)
                                    start_pos = 0x90;
                                else if(in_part_string[s].c_str()[1] < 0xB0)
                                    start_pos = 0xA0;
                                else if(in_part_string[s].c_str()[1] < 0xC0)
                                    start_pos = 0xB0;

                                end_pos = start_pos + 0x10;

                                if(in_part_string[s].c_str()[0] == 0xEA)
                                    offset = 0;
                                else if(in_part_string[s].c_str()[0] == 0xEB)
                                    offset += 1024;
                                else if(in_part_string[s].c_str()[0] == 0xEC)
                                    offset += 5120;
                                else if(in_part_string[s].c_str()[0] == 0xED)
                                    offset += 9216;

                                unknown = true;
                                if(in_part_string[s].c_str()[0] == 0xEA)
                                {
                                    start_pos = 0xB0;
                                    end_pos = 0xC0;
                                    for(int i = start_pos; i < end_pos; i++)
                                    {
                                        if(in_part_string[s].c_str()[1] == i)
                                        {
                                            offset_cache = in_part_string[s].c_str()[2] - 0x80 + ((i - 0xB0) * 64);
                                            unknown = false;
                                            break;
                                        }
                                    }
                                }
                                else
                                {
                                    if(!(in_part_string[s].c_str()[0] == 0xED && in_part_string[s].c_str()[1] >= 0x9E && in_part_string[s].c_str()[0] > 0xAF))
                                    {
                                        for(int i = start_pos; i < end_pos; i++)
                                        {
                                            if(in_part_string[s].c_str()[1] == i)
                                            {
                                                offset_cache = in_part_string[s].c_str()[2] - 0x80 + ((i - 0x80) * 64);
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
                            if(in_part_string[s].c_str()[1] == 0xB8 && in_part_string[s].c_str()[2] >= 0xB0)
                                offset = in_part_string[s].c_str()[2] - 0xB0;
                            else if(in_part_string[s].c_str()[1] == 0xB9)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 16;
                            else
                                unknown = true;
                        }
                        //halfwidth_and_fullwidth_forms : U+FF00[0xEF, 0xBC, 0x80]~U+FFEF[0xEF, 0xBF, 0xAF], passed
                        else if(block == DEF_EXFONT_BLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS)
                        {
                            if(in_part_string[s].c_str()[1] == 0xBC)
                                offset = in_part_string[s].c_str()[2] - 0x80;
                            else if(in_part_string[s].c_str()[1] == 0xBD)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 64;
                            else if(in_part_string[s].c_str()[1] == 0xBE)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 128;
                            else if(in_part_string[s].c_str()[1] == 0xBF)
                                offset = in_part_string[s].c_str()[2] - 0x80 + 192;
                            else
                                unknown = true;
                        }
                        else
                            unknown = true;
                    }
                }
            }
            else if(in_part_string[s].length() == 4)
            {
                if(in_part_string[s].c_str()[0] != 0xF0 || in_part_string[s].c_str()[1] != 0x9F
                || in_part_string[s].c_str()[3] < 0x80 || in_part_string[s].c_str()[3] > 0xBf)
                    unknown = true;
                else
                {
                    //miscellaneous_symbols_and_pictographs : U+1F300[0xF0, 0x9F, 0x8C, 0x80]~U+1F5FF[0xF0, 0x9F, 0x97, 0xBF], passed
                    if(block == DEF_EXFONT_BLOCK_MISCELLANEOUS_SYMBOLS_AND_PICTOGRAPHS)
                    {
                        if(in_part_string[s].c_str()[2] == 0x8C)
                            offset = in_part_string[s].c_str()[3] - 0x80;
                        else if(in_part_string[s].c_str()[2] == 0x8D)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 64;
                        else if(in_part_string[s].c_str()[2] == 0x8E)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 128;
                        else if(in_part_string[s].c_str()[2] == 0x8F)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 192;
                        else if(in_part_string[s].c_str()[2] == 0x90)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 256;
                        else if(in_part_string[s].c_str()[2] == 0x91)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 320;
                        else if(in_part_string[s].c_str()[2] == 0x92)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 384;
                        else if(in_part_string[s].c_str()[2] == 0x93)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 448;
                        else if(in_part_string[s].c_str()[2] == 0x94)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 512;
                        else if(in_part_string[s].c_str()[2] == 0x95)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 576;
                        else if(in_part_string[s].c_str()[2] == 0x96)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 640;
                        else if(in_part_string[s].c_str()[2] == 0x97)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 704;
                        else
                            unknown = true;
                    }
                    //emoticons : U+1F600[0xF0, 0x9F, 0x98, 0x80]~U+1F64F[0xF0, 0x9F, 0x99, 0x8F], passed
                    else if(block == DEF_EXFONT_BLOCK_EMOTICONS)
                    {
                        if(in_part_string[s].c_str()[2] == 0x98)
                            offset = in_part_string[s].c_str()[3] - 0x80;
                        else if(in_part_string[s].c_str()[2] == 0x99 && in_part_string[s].c_str()[3] <= 0x8F)
                            offset = in_part_string[s].c_str()[3] - 0x80 + 64;
                        else
                            unknown = true;
                    }
                }
            }
            else
                unknown = true;
        }

        if(unknown)
        {
            base_index = 0;
            offset = 0;
        }

        x_size = util_exfont_font_images[base_index + offset].subtex->width * texture_size_x;
        y_size = util_exfont_font_images[base_index + offset].subtex->height * texture_size_y;
        if(!size_only)
            Draw_texture(util_exfont_font_images[base_index + offset], abgr8888, (texture_x + x_offset), texture_y, x_size, y_size);

        x_offset += x_size + (interval_offset * texture_size_x);
    }

    *out_width = x_offset;
    *out_height = util_exfont_font_images[0].subtex->height * texture_size_y;
}

void Exfont_draw_external_fonts(std::string* in_part_string, int num_of_characters, float texture_x, float texture_y, float texture_size_x,
float texture_size_y, int abgr8888, float* out_width, float* out_height)
{
    Exfont_draw_external_fonts(in_part_string, num_of_characters, texture_x, texture_y, texture_size_x, texture_size_y, abgr8888, out_width, out_height, false);    
}

void Exfont_draw_get_text_size(std::string* in_part_string, int num_of_characters, float texture_size_x, float texture_size_y, float* out_width, float* out_height)
{
    Exfont_draw_external_fonts(in_part_string, num_of_characters, 0, 0, texture_size_x, texture_size_y, DEF_DRAW_NO_COLOR, out_width, out_height, true);
}

Result_with_string Exfont_load_exfont(int exfont_num)
{
    Result_with_string result;
    if (exfont_num >= 0 && exfont_num < DEF_EXFONT_NUM_OF_FONT_NAME)
    {
        if(exfont_num == DEF_EXFONT_BLOCK_CJK_UNIFIED_IDEOGRAPHS)
        {
            int start_pos = util_exfont_font_start_num[exfont_num];
            for(int i = 0; i < 21; i++)
            {
                if(util_exfont_font_cjk_unified_ideographs_characters[i] <= 0)
                {
                    if(i != 0)
                        util_exfont_loaded_external_font[exfont_num] = true;

                    break;
                }

                util_exfont_cjk_unified_ideographs_texture_num[i] = Draw_get_free_sheet_num();
                result = Draw_load_texture("romfs:/gfx/font/" + util_exfont_font_name[exfont_num] + "_" + std::to_string(i) + "_font.t3x", util_exfont_cjk_unified_ideographs_texture_num[i], util_exfont_font_images, start_pos, util_exfont_font_cjk_unified_ideographs_characters[i]);

                if (result.code == 0)
                {
                    for (int i = 0; i < util_exfont_font_cjk_unified_ideographs_characters[i]; i++)
                    {
                        C3D_TexSetFilter(util_exfont_font_images[start_pos + i].tex, GPU_LINEAR, GPU_LINEAR);
                        C3D_TexSetWrap(util_exfont_font_images[start_pos + i].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
                    }
                    if(i == 20)
                        util_exfont_loaded_external_font[exfont_num] = true;
                }

                if (result.code != 0)
                {
                    Exfont_unload_exfont(exfont_num);
                    util_exfont_loaded_external_font[exfont_num] = false;
                    break;
                }

                start_pos += util_exfont_font_cjk_unified_ideographs_characters[i];
            }
        }
        else if(exfont_num == DEF_EXFONT_BLOCK_HANGUL_SYLLABLES)
        {
            int start_pos = util_exfont_font_start_num[exfont_num];
            for(int i = 0; i < 11; i++)
            {
                if(util_exfont_font_hangul_syllables_characters[i] <= 0)
                {
                    if(i != 0)
                        util_exfont_loaded_external_font[exfont_num] = true;

                    break;
                }

                util_exfont_hangul_syllables_texture_num[i] = Draw_get_free_sheet_num();
                result = Draw_load_texture("romfs:/gfx/font/" + util_exfont_font_name[exfont_num] + "_" + std::to_string(i) + "_font.t3x", util_exfont_hangul_syllables_texture_num[i], util_exfont_font_images, start_pos, util_exfont_font_hangul_syllables_characters[i]);

                if (result.code == 0)
                {
                    for (int i = 0; i < util_exfont_font_hangul_syllables_characters[i]; i++)
                    {
                        C3D_TexSetFilter(util_exfont_font_images[start_pos + i].tex, GPU_LINEAR, GPU_LINEAR);
                        C3D_TexSetWrap(util_exfont_font_images[start_pos + i].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
                    }
                    if(i == 10)
                        util_exfont_loaded_external_font[exfont_num] = true;
                }

                if (result.code != 0)
                {
                    Exfont_unload_exfont(exfont_num);
                    util_exfont_loaded_external_font[exfont_num] = false;
                    break;
                }

                start_pos += util_exfont_font_hangul_syllables_characters[i];
            }
        }
        else
        {
            util_exfont_texture_num[exfont_num] = Draw_get_free_sheet_num();
            result = Draw_load_texture("romfs:/gfx/font/" + util_exfont_font_name[exfont_num] + "_font.t3x", util_exfont_texture_num[exfont_num], util_exfont_font_images, util_exfont_font_start_num[exfont_num], util_exfont_font_characters[exfont_num]);

            if (result.code == 0)
            {
                for (int i = 0; i < util_exfont_font_characters[exfont_num]; i++)
                {
                    C3D_TexSetFilter(util_exfont_font_images[util_exfont_font_start_num[exfont_num] + i].tex, GPU_LINEAR, GPU_LINEAR);
                    C3D_TexSetWrap(util_exfont_font_images[util_exfont_font_start_num[exfont_num] + i].tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
                }
                util_exfont_loaded_external_font[exfont_num] = true;
            }

            if (result.code != 0)
            {
                Exfont_unload_exfont(exfont_num);
                util_exfont_loaded_external_font[exfont_num] = false;
            }
        }
    }
    else
        goto invalid_arg;

    return result;

    invalid_arg:
    result.code = DEF_ERR_INVALID_ARG;
    result.string = DEF_ERR_INVALID_ARG_STR;
    return result;
}

void Exfont_unload_exfont(int exfont_num)
{
    if (exfont_num >= 0 && exfont_num < DEF_EXFONT_NUM_OF_FONT_NAME)
    {
        util_exfont_loaded_external_font[exfont_num] = false;

        for (int j = util_exfont_font_start_num[exfont_num]; j < util_exfont_font_characters[exfont_num]; j++)
            util_exfont_font_images[j].tex = NULL;

        if(exfont_num == DEF_EXFONT_BLOCK_CJK_UNIFIED_IDEOGRAPHS)
        {
            for(int i = 0; i < 21; i++)
                Draw_free_texture(util_exfont_cjk_unified_ideographs_texture_num[i]);
        }
        else if(exfont_num == DEF_EXFONT_BLOCK_HANGUL_SYLLABLES)
        {
            for(int i = 0; i < 11; i++)
                Draw_free_texture(util_exfont_hangul_syllables_texture_num[i]);
        }
        else
            Draw_free_texture(util_exfont_texture_num[exfont_num]);
    }
}
