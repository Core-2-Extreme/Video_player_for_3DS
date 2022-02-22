#include "system/headers.hpp"

bool util_exfont_loaded_external_font[DEF_EXFONT_NUM_OF_FONT_NAME];
bool util_exfont_request_external_font_state[DEF_EXFONT_NUM_OF_FONT_NAME];
bool util_exfont_loaded_system_font[4] = { false, false, false, false, };
bool util_exfont_request_system_font_state[4] = { false, false, false, false, };
bool util_exfont_thread_run = false;
bool util_exfont_load_external_font_request = false;
bool util_exfont_unload_external_font_request = false;
bool util_exfont_load_system_font_request = false;
bool util_exfont_unload_system_font_request = false;
bool util_exfont_init = false;
int util_exfont_texture_num[DEF_EXFONT_NUM_OF_FONT_NAME];

std::string util_exfont_part_string[1024];
std::string util_exfont_font_samples[9216];
std::string util_exfont_font_right_to_left_samples[257];
std::string util_exfont_font_name[DEF_EXFONT_NUM_OF_FONT_NAME];
std::string util_exfont_ignore_chars = "\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u200B\u200C\u200D\u200E\u200F";
C2D_Image util_exfont_font_images[9216];

int util_exfont_font_characters[DEF_EXFONT_NUM_OF_FONT_NAME] = {
 128,  96, 128, 208,  96,  80, 112, 135, 256,  48,  89,  88,  255, 128, 79,  72, 
  96,  88,  90,  87,  67, 211,  88, 640, 128,  63, 233,  71,  42,  33, 112, 256,
 256,  11, 160, 128,  32,  96, 256, 192, 128, 252,  64,  93,  96, 256, 1165, 55, 
  32, 225, 768,  80,
};
int util_exfont_num_of_right_left_charcters = 0;
int util_exfont_font_start_num[DEF_EXFONT_NUM_OF_FONT_NAME];
Thread util_exfont_load_font_thread;

Result_with_string Exfont_load_exfont(int exfont_num);
void Exfont_unload_exfont(int exfont_num);

void Exfont_load_font_thread(void* arg)
{
    Util_log_save(DEF_EXFONT_LOAD_FONT_THREAD_STR, "Thread started.");
    Result_with_string result;

    while(util_exfont_thread_run)
    {
        if(util_exfont_load_external_font_request)
        {
            for(int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
            {
                if(util_exfont_request_external_font_state[i] && !util_exfont_loaded_external_font[i])
                {
                    result = Exfont_load_exfont(i);
					Util_log_save(DEF_EXFONT_LOAD_FONT_THREAD_STR, "Exfont_load_exfont()..." + result.string + result.error_description, result.code);
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
        else if(util_exfont_load_system_font_request)
        {
            for(int i = 0; i < 4; i++)
            {
                if(util_exfont_request_system_font_state[i] && !util_exfont_loaded_system_font[i])
                {
					result = Draw_load_system_font(i);
					Util_log_save(DEF_EXFONT_LOAD_FONT_THREAD_STR, "Draw_load_system_font()..." + result.string + result.error_description, result.code);
                    var_need_reflesh = true;
                    if(result.code == 0 || i == var_system_region)
                        util_exfont_loaded_system_font[i] = true;
                }
            }
            util_exfont_load_system_font_request = false;
        }
        else if(util_exfont_unload_system_font_request)
        {
            for(int i = 0; i < 4; i++)
            {
                if(!util_exfont_request_system_font_state[i] && util_exfont_loaded_system_font[i])
                {
					Draw_free_system_font(i);
                    var_need_reflesh = true;
                    util_exfont_loaded_system_font[i] = false;
                }
            }
            util_exfont_unload_system_font_request = false;
        }
        else
            usleep(DEF_ACTIVE_THREAD_SLEEP_TIME);
    }
	Util_log_save(DEF_EXFONT_LOAD_FONT_THREAD_STR, "Thread exit.");
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

    result = Util_file_load_from_rom("font_samples.txt", "romfs:/gfx/font/sample/", &fs_buffer, 0x8000, &read_size);
    if(result.code != 0)
        goto api_failed;

    Exfont_text_parse((char*)fs_buffer, util_exfont_font_samples, 10240, &characters);
    Util_safe_linear_free(fs_buffer);
    fs_buffer = NULL;

    for (int i = characters; i > -1; i--)
        util_exfont_font_samples[i + 1] = util_exfont_font_samples[i];

    result = Util_file_load_from_rom("font_right_to_left_samples.txt", "romfs:/gfx/font/sample/", &fs_buffer, 0x8000, &read_size);
    if(result.code != 0)
        goto api_failed;

    Exfont_text_parse((char*)fs_buffer, util_exfont_font_right_to_left_samples, 256, &characters);
    Util_safe_linear_free(fs_buffer);
    fs_buffer = NULL;

    util_exfont_num_of_right_left_charcters = characters;

    util_exfont_font_samples[0] = "\u0000";
    util_exfont_font_start_num[0] = 0;
    for (int i = 1; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
        util_exfont_font_start_num[i] = util_exfont_font_start_num[i - 1] + util_exfont_font_characters[i - 1];
    
    for (int i = 0; i < DEF_EXFONT_NUM_OF_FONT_NAME; i++)
    {
        util_exfont_loaded_external_font[i] = false;
        util_exfont_request_external_font_state[i] = false;
        util_exfont_texture_num[i] = 0;
    }

	util_exfont_thread_run = true;
	util_exfont_load_font_thread = threadCreate(Exfont_load_font_thread, (void*)(""), DEF_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 0, false);
    if(!util_exfont_load_font_thread)
    {
        result.error_description = "[Error] threadCreate() failed. ";
        goto nintendo_api_failed;
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

	nintendo_api_failed:
	result.string = DEF_ERR_NINTENDO_RETURNED_NOT_SUCCESS_STR;
	return result;
}

void Exfont_exit(void)
{
    if(!util_exfont_init)
        return;
    
    util_exfont_init = false;
    util_exfont_thread_run = false;
	threadJoin(util_exfont_load_font_thread, 10000000000);
	threadFree(util_exfont_load_font_thread);
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

bool Exfont_is_loaded_system_font(int system_font_num)
{
    if(!util_exfont_init)
        return false;
    else if (system_font_num >= 0 && system_font_num < 4)
        return util_exfont_loaded_system_font[system_font_num];
    else
        return false;
}

bool Exfont_is_loading_system_font(void)
{
    if(!util_exfont_init)
        return false;

    return util_exfont_load_system_font_request;
}

bool Exfont_is_unloading_system_font(void)
{
    if(!util_exfont_init)
        return false;

    return util_exfont_unload_system_font_request;
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

void Exfont_set_system_font_request_state(int system_font_num, bool flag)
{
    if(!util_exfont_init)
        return;

    if (system_font_num >= 0 && system_font_num < 4)
        util_exfont_request_system_font_state[system_font_num] = flag;
}

void Exfont_request_load_system_font(void)
{
    if(!util_exfont_init)
        return;

    util_exfont_load_system_font_request = true;
}

void Exfont_request_unload_system_font(void)
{
    if(!util_exfont_init)
        return;

    util_exfont_unload_system_font_request = true;
}

std::string Exfont_text_sort(std::string sorce_part_string[], int max_loop)
{
    int right_to_left_pos = -1;
    bool found = false;
    std::string result_string = "";
    std::string right_to_left_sample[2] = { "\u05BD", "\u0700", };

    if(!util_exfont_init)
        return "";
    
    if(!sorce_part_string || max_loop <= 0)
        return "";
    
    for (int i = 0; i < max_loop; i++)
    {
        found = false;
        if (memcmp((void*)sorce_part_string[i].c_str(), (void*)util_exfont_font_samples[0].c_str(), 0x1) == 0)
            break;

        if (util_exfont_font_right_to_left_samples[201] == sorce_part_string[i])
            found = true;
        else if (sorce_part_string[i].length() == 2 && memcmp((void*)sorce_part_string[i].c_str(), (void*)right_to_left_sample[0].c_str(), 0x2) > 0
            && memcmp((void*)sorce_part_string[i].c_str(), (void*)right_to_left_sample[1].c_str(), 0x2) < 0)
        {
            for (int j = 0; j < util_exfont_num_of_right_left_charcters - 1; j++)
            {
                if (memcmp((void*)sorce_part_string[i].c_str(), (void*)util_exfont_font_right_to_left_samples[j].c_str(), 0x2) == 0)
                {
                    found = true;
                    break;
                }
            }
        }

        if (found)
        {
            if (right_to_left_pos <= -1)
                right_to_left_pos = result_string.length();

            result_string.insert(right_to_left_pos, sorce_part_string[i]);
        }
        else
        {
            result_string += sorce_part_string[i];
            right_to_left_pos = -1;
        }
    }
    return result_string;
}

void Exfont_text_parse(std::string sorce_string, std::string part_string[], int max_loop, int* out_element)
{
    int sorce_string_length = 0;
    int std_num = 0;
    int parse_string_length = 0;
    int i = 0;
    char* sorce_string_char = NULL;
    
    if(!part_string || max_loop <= 0 || !out_element)
        return;
    
    *out_element = 0;
    sorce_string_length = sorce_string.length();
    sorce_string_char = (char*)malloc(sorce_string.length() + 10);
    if(!sorce_string_char)
        return;

    memset(sorce_string_char, 0x0, sorce_string.length() + 10);
    strcpy(sorce_string_char, (char*)sorce_string.c_str());

    for (int k = 0; k < max_loop; k++)
    {
        parse_string_length = mblen(&sorce_string_char[i], 4);

        if (i >= sorce_string_length)
            break;
        else if (parse_string_length >= 1)
        {
            part_string[std_num] = sorce_string.substr(i, parse_string_length);
            i += parse_string_length;
            std_num++;
        }
        else
            i++;
    }
    part_string[std_num] = "\u0000";
    free(sorce_string_char);
    sorce_string_char = NULL;
    *out_element = std_num;
}

void Exfont_draw_external_fonts(std::string in_string, float texture_x, float texture_y, float texture_size_x, float texture_size_y, int abgr8888, float* out_width, float* out_height, bool size_only)
{
    double interval_offset = 1;
    double x_offset = 0.0;
    double x_size = 0.0;
    int block = -1;
    int char_size = 0;
    int characters = 0;
    int memcmp_result = 0;
    bool reverse = false;
    bool unknown = true;
    std::string sample_one_byte[DEF_EXFONT_NUM_OF_ONE_BYTE_FONT] = { "\u0080", };
    std::string samples_two_bytes[DEF_EXFONT_NUM_OF_TWO_BYTES_FONT] = { "\u0100", "\u0180", "\u0250", "\u02B0", "\u0300", "\u0370", "\u0400", "\u0500", "\u0530", "\u0590", "\u0600", "\u0700", };
    std::string samples_three_bytes[DEF_EXFONT_NUM_OF_THREE_BYTES_FONT] = { "\u0980", "\u0A80", "\u0C00", "\u0C80", "\u0D00", "\u0E00", "\u0E80", "\u0F00",
                                            "\u1000", "\u1100", "\u1680", "\u1D80", "\u1E00", "\u2000", "\u2070", "\u20A0",
                                            "\u2100", "\u2200", "\u2300", "\u2400", "\u2460", "\u2500", "\u2580", "\u25A0",
                                            "\u2600", "\u2700", "\u27C0", "\u2980", "\u2C00", "\u3040", "\u30A0", "\u3100",
                                            "\u3400", "\uA490", "\uA4D0", "\uFE50", "\uFFF0", };
    std::string samples_four_bytes[DEF_EXFONT_NUM_OF_FOUR_BYTES_FONT] = { "\U0001F600", "\U0001F650", };

    if(!util_exfont_init)
        return;

    if(!out_width || !out_height)
        return;

    *out_width = 0;
    *out_height = 0;
    Exfont_text_parse(in_string, util_exfont_part_string, 1023, &characters);

    for (int s = 0; s < characters; s++)
    {
        block = -1;
        unknown = true;
        if (memcmp((void*)util_exfont_part_string[s].c_str(), (void*)util_exfont_font_samples[0].c_str(), 0x1) == 0)
            break;

        if (util_exfont_part_string[s].length() == 1)
        {
            char_size = 1;
            if (memcmp((void*)util_exfont_part_string[s].c_str(), (void*)sample_one_byte[0].c_str(), 0x1) < 0)
                block = 0;
        }
        else if (util_exfont_part_string[s].length() == 2)
        {
            char_size = 2;
            for (int i = 0; i < DEF_EXFONT_NUM_OF_TWO_BYTES_FONT; i++)
            {
                if (memcmp((void*)util_exfont_part_string[s].c_str(), (void*)samples_two_bytes[i].c_str(), 0x2) < 0)
                {
                    block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT;
                    break;
                }
            }
        }
        else if (util_exfont_part_string[s].length() == 3)
        {
            char_size = 3;
            for (int i = 0; i < DEF_EXFONT_NUM_OF_THREE_BYTES_FONT; i++)
            {
                if (memcmp((void*)util_exfont_part_string[s].c_str(), (void*)samples_three_bytes[i].c_str(), 0x3) < 0)
                {
                    block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT + DEF_EXFONT_NUM_OF_TWO_BYTES_FONT;
                    break;
                }
            }
        }
        else if (util_exfont_part_string[s].length() == 4)
        {
            char_size = 4;
            for (int i = 0; i < DEF_EXFONT_NUM_OF_FOUR_BYTES_FONT; i++)
            {
                if (memcmp((void*)util_exfont_part_string[s].c_str(), (void*)samples_four_bytes[i].c_str(), 0x4) < 0)
                {
                    block = i + DEF_EXFONT_NUM_OF_ONE_BYTE_FONT + DEF_EXFONT_NUM_OF_TWO_BYTES_FONT + DEF_EXFONT_NUM_OF_THREE_BYTES_FONT;
                    break;
                }
            }
        }

        if (block == 27)//General punctuation
        {
            if (!(util_exfont_ignore_chars.find(util_exfont_part_string[s]) == std::string::npos))
            {
                unknown = false;
                block = -1;
            }
        }

        if (block != -1 && util_exfont_loaded_external_font[block] && char_size >= 1 && char_size <= 4)
        {
            reverse = false;
            for (int k = 0;;)
            {
                if (!reverse)
                    k += 20;
                else
                    k--;

                if ((k < 0 || k > util_exfont_font_characters[block]) && reverse)
                    break;
                else
                    memcmp_result = memcmp((void*)util_exfont_part_string[s].c_str(), (void*)util_exfont_font_samples[util_exfont_font_start_num[block] + k].c_str(), char_size);

                if (memcmp_result == 0)
                {
                    unknown = false;
                    x_size = util_exfont_font_images[util_exfont_font_start_num[block] + k].subtex->width * texture_size_x;
                    if(!size_only)
                        Draw_texture(util_exfont_font_images[util_exfont_font_start_num[block] + k], abgr8888, (texture_x + x_offset), texture_y, x_size, util_exfont_font_images[0].subtex->height * texture_size_y);

                    x_offset += x_size + (interval_offset * texture_size_x);
                    break;
                }
                else if (memcmp_result < 0 || k >= util_exfont_font_characters[block])
                {
                    reverse = true;
                    if (k >= util_exfont_font_characters[block])
                        k = util_exfont_font_characters[block];
                }
            }
        }

        if (unknown)
        {
            x_size = util_exfont_font_images[0].subtex->width * texture_size_x;
            if(!size_only)
                Draw_texture(util_exfont_font_images[0], abgr8888, (texture_x + x_offset), texture_y, x_size, util_exfont_font_images[0].subtex->height * texture_size_y);

            x_offset += x_size + (interval_offset * texture_size_x);
        }
    }
    *out_width = x_offset;
    *out_height = util_exfont_font_images[0].subtex->height * texture_size_y;
}

void Exfont_draw_external_fonts(std::string in_string, float texture_x, float texture_y, float texture_size_x,
 float texture_size_y, int abgr8888, float* out_width, float* out_height)
{
    Exfont_draw_external_fonts(in_string, texture_x, texture_y, texture_size_x, texture_size_y, abgr8888, out_width, out_height, false);    
}

void Exfont_draw_get_text_size(std::string in_string, float texture_size_x, float texture_size_y, float* out_width, float* out_height)
{
    Exfont_draw_external_fonts(in_string, 0, 0, texture_size_x, texture_size_y, DEF_DRAW_NO_COLOR, out_width, out_height, true);
}

Result_with_string Exfont_load_exfont(int exfont_num)
{
    Result_with_string result;
    if (exfont_num >= 0 && exfont_num < DEF_EXFONT_NUM_OF_FONT_NAME)
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
        Draw_free_texture(util_exfont_texture_num[exfont_num]);
        for (int j = util_exfont_font_start_num[exfont_num]; j < util_exfont_font_characters[exfont_num]; j++)
            util_exfont_font_images[j].tex = NULL;

        util_exfont_loaded_external_font[exfont_num] = false;
    }
}
