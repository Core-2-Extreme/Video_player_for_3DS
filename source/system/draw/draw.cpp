#include "system/headers.hpp"

bool util_draw_init = false;
bool util_draw_sheet_texture_free[DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS];
double util_draw_frametime = 0;
int util_draw_rendered_frames = 0;
int util_draw_rendered_frames_cache = 0;
u64 util_draw_reset_fps_counter_time = 0;
std::string util_draw_part_text[2][1024];
C2D_Font util_draw_system_fonts[4];
C3D_RenderTarget* util_draw_screen[3];
C2D_SpriteSheet util_draw_sheet_texture[DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS];
C2D_Image util_draw_wifi_icon_image[9];
C2D_Image util_draw_battery_level_icon_image[21];
C2D_Image util_draw_battery_charge_icon_image[1];
C2D_Image util_draw_eco_image[2];
std::string util_draw_japanese_kanji[3000];
std::string util_draw_simple_chinese[6300];
TickCounter util_draw_frame_time_stopwatch;
Image_data util_draw_bot_ui;

extern "C" void memcpy_asm(u8*, u8*, int);
extern "C" void memcpy_asm_4b(u8*, u8*);

Result_with_string Draw_load_kanji_samples(void);
void Draw_debug_info(void);

Result_with_string Draw_init(bool wide, bool _3d)
{
	Result_with_string result;
	if(util_draw_init)
		goto already_inited;

	gfxInitDefault();
	gfxSet3D(false);
	gfxSetWide(false);

	if(wide)
		gfxSetWide(wide);
	else if(_3d)
		gfxSet3D(_3d);

	if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 1.5))
	{
		result.error_description = "[Error] C3D_Init() failed. ";
		goto other;
	}

	if(!C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 1.5))
	{
		result.error_description = "[Error] C2D_Init() failed. ";
		goto other;
	}

	C2D_Prepare();
	util_draw_screen[DEF_DRAW_SCREEN_TOP_LEFT] = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	util_draw_screen[DEF_DRAW_SCREEN_BOTTOM] = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	util_draw_screen[DEF_DRAW_SCREEN_TOP_RIGHT] = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	if(!util_draw_screen[DEF_DRAW_SCREEN_TOP_LEFT] || !util_draw_screen[DEF_DRAW_SCREEN_BOTTOM]
	|| !util_draw_screen[DEF_DRAW_SCREEN_TOP_RIGHT])
	{
		result.error_description = "[Error] C2D_CreateScreenTarget() failed. ";
		goto other;
	}

	C2D_TargetClear(util_draw_screen[DEF_DRAW_SCREEN_TOP_LEFT], C2D_Color32f(0, 0, 0, 0));
	C2D_TargetClear(util_draw_screen[DEF_DRAW_SCREEN_BOTTOM], C2D_Color32f(0, 0, 0, 0));
	C2D_TargetClear(util_draw_screen[DEF_DRAW_SCREEN_TOP_RIGHT], C2D_Color32f(0, 0, 0, 0));

	for (int i = 0; i < DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS; i++)
		util_draw_sheet_texture_free[i] = true;

	osTickCounterStart(&util_draw_frame_time_stopwatch);

	result = Draw_load_texture("romfs:/gfx/draw/wifi_signal.t3x", 0, util_draw_wifi_icon_image, 0, 9);
	if(result.code != 0)
		goto api_failed;

	result = Draw_load_texture("romfs:/gfx/draw/battery_level.t3x", 1, util_draw_battery_level_icon_image, 0, 21);
	if(result.code != 0)
		goto api_failed;

	result = Draw_load_texture("romfs:/gfx/draw/battery_charge.t3x", 2, util_draw_battery_charge_icon_image, 0, 1);
	if(result.code != 0)
		goto api_failed;

	result = Draw_load_texture("romfs:/gfx/draw/eco.t3x", 3, util_draw_eco_image, 0, 2);
	if(result.code != 0)
		goto api_failed;

	result = Draw_load_texture("romfs:/gfx/draw/square.t3x", 4, var_square_image, 0, 1);
	if(result.code != 0)
		goto api_failed;

	result = Draw_load_kanji_samples();
	if(result.code != 0)
		goto api_failed;

	util_draw_bot_ui.c2d = var_square_image[0];
	util_draw_reset_fps_counter_time = osGetTime() + 1000;
	util_draw_init = true;
	return result;

	already_inited:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	other:
	C2D_Fini();
	C3D_Fini();
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;

	api_failed:
	C2D_Fini();
	C3D_Fini();
	return result;
}

Result_with_string Draw_reinit(bool wide, bool _3d)
{
	Result_with_string result;

	if(!util_draw_init)
		goto not_inited;
	
	util_draw_init = false;
	C2D_Fini();
	C3D_Fini();

	gfxSet3D(false);
	gfxSetWide(false);

	if(wide)
		gfxSetWide(wide);
	else if(_3d)
		gfxSet3D(_3d);

	if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE * 1.5))
	{
		result.error_description = "[Error] C3D_Init() failed. ";
		goto other;
	}

	if(!C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 1.5))
	{
		result.error_description = "[Error] C2D_Init() failed. ";
		goto other;
	}

	C2D_Prepare();
	util_draw_screen[DEF_DRAW_SCREEN_TOP_LEFT] = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	util_draw_screen[DEF_DRAW_SCREEN_BOTTOM] = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	util_draw_screen[DEF_DRAW_SCREEN_TOP_RIGHT] = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
	if(!util_draw_screen[DEF_DRAW_SCREEN_TOP_LEFT] || !util_draw_screen[DEF_DRAW_SCREEN_BOTTOM]
	|| !util_draw_screen[DEF_DRAW_SCREEN_TOP_RIGHT])
	{
		result.error_description = "[Error] C2D_CreateScreenTarget() failed. ";
		goto other;
	}

	util_draw_init = true;
	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	other:
	C2D_Fini();
	C3D_Fini();
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

void Draw_exit(void)
{
	if(!util_draw_init)
		return;
	
	util_draw_init = false;
	for (int i = 0; i < 128; i++)
		Draw_free_texture(i);
	for (int i = 0; i < 4; i++)
		Draw_free_system_font(i);

	C2D_Fini();
	C3D_Fini();
	gfxExit();
}

double Draw_query_frametime(void)
{
	if(!util_draw_init)
		return 0;
	else
		return util_draw_frametime;
}

double Draw_query_fps(void)
{
	if(!util_draw_init)
		return 0;
	else
		return util_draw_rendered_frames;
}

int Draw_convert_to_pos(int height, int width, int img_height, int img_width, int pixel_size)
{
	int pos = img_width * height;
	if(pos == 0)
		pos = img_width;

	pos -= (img_width - width) - img_width;
	return pos * pixel_size;
}

Result_with_string Draw_texture_init(Image_data* image, int tex_size_x, int tex_size_y, int color_format)
{
	int color = 0;
	Result_with_string result;
	if(!util_draw_init)
		goto not_inited;

	if(!image || tex_size_x <= 0 || tex_size_y <= 0
	|| (color_format != DEF_DRAW_FORMAT_RGBA8888 && color_format != DEF_DRAW_FORMAT_RGB888 && color_format != DEF_DRAW_FORMAT_RGB565))
		goto invalid_arg;

	if(color_format == DEF_DRAW_FORMAT_RGBA8888)
		color = GPU_RGBA8;
	else if(color_format == DEF_DRAW_FORMAT_RGB888)
		color = GPU_RGB8;
	else if(color_format == DEF_DRAW_FORMAT_RGB565)
		color = GPU_RGB565;

	image->subtex = (Tex3DS_SubTexture*)Util_safe_linear_alloc(sizeof(Tex3DS_SubTexture*));
	image->c2d.tex = (C3D_Tex*)Util_safe_linear_alloc(sizeof(C3D_Tex*));
	if(!image->subtex || !image->c2d.tex)
		goto out_of_linear_memory;
	
	if (!C3D_TexInit(image->c2d.tex, (u16)tex_size_x, (u16)tex_size_y, (GPU_TEXCOLOR)color))
		goto out_of_linear_memory;

	image->c2d.subtex = image->subtex;
	C3D_TexSetFilter(image->c2d.tex, GPU_LINEAR, GPU_LINEAR);
	image->c2d.tex->border = 0xFFFFFF;
	C3D_TexSetWrap(image->c2d.tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_linear_memory:
	Util_safe_linear_free(image->subtex);
	Util_safe_linear_free(image->c2d.tex);
	image->subtex = NULL;
	image->c2d.tex = NULL;
	result.code = DEF_ERR_OUT_OF_LINEAR_MEMORY;
	result.string = DEF_ERR_OUT_OF_LINEAR_MEMORY_STR;
	return result;
}

void Draw_texture_free(Image_data* image)
{
	if(!util_draw_init)
		return;

	if(!image)
		return;
	
	if(image->c2d.tex)
	{
		Util_safe_linear_free(image->c2d.tex->data);
		image->c2d.tex->data = NULL;
	}

	Util_safe_linear_free(image->c2d.tex);
	Util_safe_linear_free(image->subtex);
	image->c2d.tex = NULL;
	image->c2d.subtex = NULL;
	image->subtex = NULL;
}

Result_with_string Draw_set_texture_data_direct(Image_data* image, u8* buf, int pic_width, int pic_height,
int tex_size_x, int tex_size_y, int color_format)
{
	int pixel_size = 0;
	int tex_offset = 0;
	int buffer_offset = 0;
	Result_with_string result;
#if DEF_DRAW_USE_DMA
	bool dma_result[4] = { false, false, false, false, };
	int dma_count = 0;
	Handle dma_handle[4] = { 0, 0, 0, 0, };
	DmaConfig dma_config;
#endif

	if(!util_draw_init)
		goto not_inited;

	if(!image || !image->subtex || !image->c2d.tex || !buf || pic_width > 1024 || pic_width <= 0 
	|| pic_height > 1024 || pic_height <= 0 || pic_width > tex_size_x || pic_height > tex_size_y
	|| (color_format != DEF_DRAW_FORMAT_RGBA8888 && color_format != DEF_DRAW_FORMAT_RGB888 && color_format != DEF_DRAW_FORMAT_RGB565))
		goto invalid_arg;
	
	if(color_format == DEF_DRAW_FORMAT_RGBA8888)
		pixel_size = 4;
	else if(color_format == DEF_DRAW_FORMAT_RGB888)
		pixel_size = 3;
	else if(color_format == DEF_DRAW_FORMAT_RGB565)
		pixel_size = 2;
	
	image->subtex->width = (u16)pic_width;
	image->subtex->height = (u16)pic_height;
	image->subtex->left = 0.0;
	image->subtex->top = 1.0;
	image->subtex->right = pic_width / (float)tex_size_x;
	image->subtex->bottom = 1.0 - pic_height / (float)tex_size_y;
	image->c2d.subtex = image->subtex;

#if DEF_DRAW_USE_DMA
	dmaConfigInitDefault(&dma_config);
#endif
	for(int i = 0; i < pic_height / 8; i ++)
	{
#if DEF_DRAW_USE_DMA
		dma_result[dma_count] = svcStartInterProcessDma(&dma_handle[dma_count], CUR_PROCESS_HANDLE, (u32)((u8*)image->c2d.tex->data + tex_offset),
		CUR_PROCESS_HANDLE, (u32)buf + buffer_offset, pic_width * 8 * pixel_size, &dma_config);
		dma_count++;
#else
		memcpy_asm(((u8*)image->c2d.tex->data + tex_offset), buf + buffer_offset, pic_width * 8 * pixel_size);
#endif
		tex_offset += tex_size_x * pixel_size * 8;
		buffer_offset += pic_width * pixel_size * 8;

#if DEF_DRAW_USE_DMA
		if(dma_count > 3)
		{
			for(int k = 0; k < 4; k++)
			{
				if(dma_result[k] == 0)
					svcWaitSynchronization(dma_handle[k], INT64_MAX);

				svcCloseHandle(dma_handle[k]);
				dma_result[k] = -1;
			}
			dma_count = 0;
		}
#endif
	}

#if DEF_DRAW_USE_DMA
	for(int k = 0; k < 4; k++)
	{
		if(dma_result[k] == 0)
			svcWaitSynchronization(dma_handle[k], INT64_MAX);

		svcCloseHandle(dma_handle[k]);
	}
#endif

	C3D_TexFlush(image->c2d.tex);

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
}

Result_with_string Draw_set_texture_data(Image_data* image, u8* buf, int pic_width, int pic_height, int tex_size_x,
int tex_size_y, int color_format)
{
	return Draw_set_texture_data(image, buf, pic_width, pic_height, 0, 0, tex_size_x, tex_size_y, color_format);
}

Result_with_string Draw_set_texture_data(Image_data* image, u8* buf, int pic_width, int pic_height,
int width_offset, int height_offset, int tex_size_x, int tex_size_y, int color_format)
{
	int x_max = 0;
	int y_max = 0;
	int increase_list_x[1024 + 8]; //= { 4, 12, 4, 44, }
	int increase_list_y[1024 + 8]; //= { 2, 6, 2, 22, 2, 6, 2, tex_size_x * 8 - 42, };
	int count[2] = { 0, 0, };
	int c3d_pos = 0;
	int c3d_offset = 0;
	int pixel_size = 0;
	Result_with_string result;

	if(!util_draw_init)
		goto not_inited;

	if(!image || !image->subtex || !image->c2d.tex || !buf || pic_width <= 0 || pic_height <= 0
	|| width_offset > pic_width || height_offset > pic_height || tex_size_x <= 0 || tex_size_y <= 0
	|| (color_format != DEF_DRAW_FORMAT_RGBA8888 && color_format != DEF_DRAW_FORMAT_RGB888 && color_format != DEF_DRAW_FORMAT_RGB565))
		goto invalid_arg;

	if(color_format == DEF_DRAW_FORMAT_RGBA8888)
		pixel_size = 4;
	else if(color_format == DEF_DRAW_FORMAT_RGB888)
		pixel_size = 3;
	else if(color_format == DEF_DRAW_FORMAT_RGB565)
		pixel_size = 2;

	for(int i = 0; i <= tex_size_x; i+=4)
	{
		increase_list_x[i] = 4 * pixel_size;
		increase_list_x[i + 1] = 12 * pixel_size;
		increase_list_x[i + 2] = 4 * pixel_size;
		increase_list_x[i + 3] = 44 * pixel_size;
	}
	for(int i = 0; i <= tex_size_y; i+=8)
	{
		increase_list_y[i] = 2 * pixel_size;
		increase_list_y[i + 1] = 6 * pixel_size;
		increase_list_y[i + 2] = 2 * pixel_size;
		increase_list_y[i + 3] = 22 * pixel_size;
		increase_list_y[i + 4] = 2 * pixel_size;
		increase_list_y[i + 5] = 6 * pixel_size;
		increase_list_y[i + 6] = 2 * pixel_size;
		increase_list_y[i + 7] = (tex_size_x * 8 - 42) * pixel_size;
	}
	
	y_max = pic_height - (u32)height_offset;
	x_max = pic_width - (u32)width_offset;
	if (tex_size_y < y_max)
		y_max = tex_size_y;
	if (tex_size_x < x_max)
		x_max = tex_size_x;

	image->subtex->width = (u16)x_max;
	image->subtex->height = (u16)y_max;
	image->subtex->left = 0.0;
	image->subtex->top = 1.0;
	image->subtex->right = x_max / (float)tex_size_x;
	image->subtex->bottom = 1.0 - y_max / (float)tex_size_y;
	image->c2d.subtex = image->subtex;

	if(pixel_size == 2)
	{
		for(int k = 0; k < y_max; k++)
		{
			for(int i = 0; i < x_max; i += 2)
			{
				memcpy_asm_4b(&(((u8*)image->c2d.tex->data)[c3d_pos + c3d_offset]), &(((u8*)buf)[Draw_convert_to_pos(k + height_offset, i + width_offset, pic_height, pic_width, pixel_size)]));
				c3d_pos += increase_list_x[count[0]];
				count[0]++;
			}
			count[0] = 0;
			c3d_pos = 0;
			c3d_offset += increase_list_y[count[1]];
			count[1]++;
		}
	}
	else if(pixel_size == 3)
	{
		for(int k = 0; k < y_max; k++)
		{
			for(int i = 0; i < x_max; i += 2)
			{
				memcpy_asm_4b(&(((u8*)image->c2d.tex->data)[c3d_pos + c3d_offset]), &(((u8*)buf)[Draw_convert_to_pos(k + height_offset, i + width_offset, pic_height, pic_width, pixel_size)]));
				memcpy(&(((u8*)image->c2d.tex->data)[c3d_pos + c3d_offset + 4]), &(((u8*)buf)[Draw_convert_to_pos(k + height_offset, i + width_offset, pic_height, pic_width, pixel_size) + 4]), 2);
				c3d_pos += increase_list_x[count[0]];
				count[0]++;
			}
			count[0] = 0;
			c3d_pos = 0;
			c3d_offset += increase_list_y[count[1]];
			count[1]++;
		}
	}
	else if(pixel_size == 4)
	{
		for(int k = 0; k < y_max; k++)
		{
			for(int i = 0; i < x_max; i += 2)
			{
				memcpy_asm_4b(&(((u8*)image->c2d.tex->data)[c3d_pos + c3d_offset]), &(((u8*)buf)[Draw_convert_to_pos(k + height_offset, i + width_offset, pic_height, pic_width, pixel_size)]));
				memcpy_asm_4b(&(((u8*)image->c2d.tex->data)[c3d_pos + c3d_offset + 4]), &(((u8*)buf)[Draw_convert_to_pos(k + height_offset, i + width_offset, pic_height, pic_width, pixel_size) + 4]));
				c3d_pos += increase_list_x[count[0]];
				count[0]++;
			}
			count[0] = 0;
			c3d_pos = 0;
			c3d_offset += increase_list_y[count[1]];
			count[1]++;
		}
	}

	C3D_TexFlush(image->c2d.tex);

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
}

void Draw_set_texture_filter(Image_data* image, bool filter)
{
	if(!util_draw_init)
		return;
	
	if(!image || !image->c2d.tex)
		return;
	
	if(filter)
		C3D_TexSetFilter(image->c2d.tex, GPU_LINEAR, GPU_LINEAR);
	else
		C3D_TexSetFilter(image->c2d.tex, GPU_NEAREST, GPU_NEAREST);
}

void Draw_parse_text(std::string text, int* count, short font_list[2][1024])
{
	bool reverse = false;
	bool found = false;
	bool font_loaded[2] = { Exfont_is_loaded_system_font(0), Exfont_is_loaded_system_font(1), };//JPN, CHN
	int previous_num = -3;
	int memcmp_result = -1;
	int array_count = 0;
	int characters = 0;
	std::string sample[8] = { "\u0000", "\u000A", "\u4DFF", "\uA000", "\u312F", "\u3190", "\uABFF", "\uD7B0", };

	Exfont_text_parse(text, util_draw_part_text[0], 1023, &characters);
	Exfont_text_parse(Exfont_text_sort(util_draw_part_text[0], 1023), util_draw_part_text[0], 1023, &characters);

	for (int i = 0; i < characters; i++)
	{
		reverse = false;
		if (memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[0].c_str(), 0x1) == 0)
		{
			font_list[0][i] = -2;
			break;
		}
		else if (memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[1].c_str(), 0x1) == 0)
		{
			font_list[0][i] = -1;
			continue;
		}

		if((memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[2].c_str(), 0x3) > 0 && memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[3].c_str(), 0x3) < 0)
		|| (memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[4].c_str(), 0x3) > 0 && memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[5].c_str(), 0x3) < 0)
		|| (memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[6].c_str(), 0x3) > 0 && memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[7].c_str(), 0x3) < 0))
		{
			if(memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[2].c_str(), 0x3) > 0 && memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)sample[3].c_str(), 0x3) < 0)
			{
				found = false;
				reverse = false;
				memcmp_result = 1;

				if(font_loaded[0])
				{
					for(int s = 0;;)
					{
						if(!reverse)
							s += 100;
						else
							s--;

						if((s < 0 || s > 3000) && reverse)
							break;
						else if(s > 3000)
						{
							reverse = true;
							s = 3000;
						}
						else
							memcmp_result = memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)util_draw_japanese_kanji[s].c_str(), 3);

						if(memcmp_result == 0)
						{
							font_list[0][i] = 0; //JPN
							found = true;
							break;
						}
						else if(memcmp_result < 0)
							reverse = true;
					}
				}

				if(!found)
				{
					reverse = false;
					memcmp_result = 1;

					if(font_loaded[1])
					{
						for(int s = 0;;)
						{
							if(!reverse)
								s += 100;
							else
								s--;

							if((s < 0 || s > 6300) && reverse)
								break;
							else if(s > 6300)
							{
								reverse = true;
								s = 6300;
							}
							else
								memcmp_result = memcmp((void*)util_draw_part_text[0][i].c_str(), (void*)util_draw_simple_chinese[s].c_str(), 3);

							if(memcmp_result == 0)
							{
								font_list[0][i] = 1; //CHN
								found = true;
								break;
							}
							else if(memcmp_result < 0)
								reverse = true;
						}
					}
				}

				if(!found)
				  font_list[0][i] = 3; //TWN
			}
			else
				font_list[0][i] = 2; //KOR
		}
		else
			font_list[0][i] = 4;
	}

	util_draw_part_text[1][0] = "";
	previous_num = font_list[0][0];
	for (int i = 0; i < characters; i++)
	{
		if(font_list[0][i] == -2)
		{
			font_list[1][array_count + 1] = font_list[0][i];
			break;
		}
		else if(previous_num != font_list[0][i] || font_list[0][i] == -1)
		{
			array_count++;
			util_draw_part_text[1][array_count] = "";
		}

		util_draw_part_text[1][array_count] += util_draw_part_text[0][i];
		font_list[1][array_count] = font_list[0][i];
		previous_num = font_list[0][i];
	}
	*count = array_count;
}

void Draw_get_text_size(std::string text, float text_size_x, float text_size_y, double* out_text_size_x, double* out_text_size_y)
{
	short font_list[2][1024];
	int count = 0;
	float width = 0, height = 0, y_offset = 0, used_x = 0, used_x_max = 0, used_y_max = 0;
	double font_height_factor = 1, font_width_factor = 1;
	C2D_Text c2d_text;
	C2D_TextBuf c2d_buf;

	if(!util_draw_init)
		return;
	
	c2d_buf = C2D_TextBufNew(4096);
	if(!c2d_buf)
		return;


	Draw_parse_text(text, &count, font_list);

	for (int i = 0; i <= count; i++)
	{
		if (font_list[1][i] == -2)
			break;
		else if (font_list[1][i] == -1)
		{			
			height = 30 * text_size_y;
			if(height > used_y_max)
				used_y_max = height;

			y_offset += used_y_max;
			used_y_max = 0;
			used_x = 0;
			continue;
		}

		if(!Exfont_is_loaded_external_font(0) || (font_list[1][i] >= 0 && font_list[1][i] <= 3))
		{
			if(!Exfont_is_loaded_external_font(0))
				font_list[1][i] = 0;

			if(font_list[1][i] == 1)
			{
				font_width_factor = 1;
				font_height_factor = 0.83333;
			}
			else if(font_list[1][i] == 2)
			{
				font_width_factor = 1;
				font_height_factor = 0.83333;
			}
			else if(font_list[1][i] == 3)
			{
				font_width_factor = 1.2;
				font_height_factor = 0.83333;
			}
			else
			{
				font_width_factor = 1;
				font_height_factor = 1;
			}

			C2D_TextBufClear(c2d_buf);
			C2D_TextFontParse(&c2d_text, util_draw_system_fonts[font_list[1][i]], c2d_buf, util_draw_part_text[1][i].c_str());
			C2D_TextOptimize(&c2d_text);
			C2D_TextGetDimensions(&c2d_text, text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
		}
		else if(font_list[1][i] == 4)
		{
			font_width_factor = 1.2;
			font_height_factor = 1.2;
			Exfont_draw_get_text_size(util_draw_part_text[1][i], text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
		}
		
		used_x += width;
		if(used_x > used_x_max)
			used_x_max = used_x;
		if(height > used_y_max)
			used_y_max = height;
	}

	*out_text_size_x = used_x_max;
	*out_text_size_y = y_offset + used_y_max;
	C2D_TextBufDelete(c2d_buf);
}

void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888)
{
	Draw(text, x, y, text_size_x, text_size_y, abgr8888, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_TOP, 0, 0, DEF_DRAW_BACKGROUND_NONE, var_null_image, DEF_DRAW_NO_COLOR);
}

void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888, int x_align, int y_align,
 float box_size_x, float box_size_y)
{
	Draw(text, x, y, text_size_x, text_size_y, abgr8888, x_align, y_align, box_size_x, box_size_y, DEF_DRAW_BACKGROUND_NONE, var_null_image, DEF_DRAW_NO_COLOR);
}

//todo : merge code
void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888, int x_align, int y_align,
 float box_size_x, float box_size_y, int texture_position, C2D_Image background_image, int texture_abgr8888)
{
	short font_list[2][1024];
	float x_start [1024];
	int lines = 0;
	int line_count = 0;
	int array_count = 0;
	float width = 0, height = 0, original_x = 0, original_y = 0, x_min = 0, y_offset = 0, used_x = 0, used_x_max = 0, used_y_max = 0;
	double font_height_factor = 1, font_width_factor = 1, font_draw_height_factor = 1, font_draw_width_factor = 1;
	C2D_Text c2d_text;
	C2D_TextBuf c2d_buf;
	original_x = x;
	original_y = y;

	if(!util_draw_init)
		return;
	
	if((x_align != DEF_DRAW_X_ALIGN_LEFT && x_align != DEF_DRAW_X_ALIGN_CENTER && x_align != DEF_DRAW_X_ALIGN_RIGHT)
	|| (y_align != DEF_DRAW_Y_ALIGN_TOP && y_align != DEF_DRAW_Y_ALIGN_CENTER && y_align != DEF_DRAW_Y_ALIGN_BOTTOM)
	|| (texture_position != DEF_DRAW_BACKGROUND_NONE && texture_position != DEF_DRAW_BACKGROUND_ENTIRE_BOX && texture_position != DEF_DRAW_BACKGROUND_UNDER_TEXT))
		return;

	c2d_buf = C2D_TextBufNew(4096);
	if(!c2d_buf)
		return;


	Draw_parse_text(text, &array_count, font_list);

	if(x_align == DEF_DRAW_X_ALIGN_LEFT && y_align == DEF_DRAW_Y_ALIGN_TOP && texture_position == DEF_DRAW_BACKGROUND_NONE)
			x = original_x;
	else
	{
		for (int i = 0; i <= array_count; i++)
		{
			if (font_list[1][i] == -2)
				break;
			else if (font_list[1][i] == -1)
			{
				height = 30 * text_size_y;
				if(height > used_y_max)
					used_y_max = height;

				y_offset += used_y_max;
				if(x_align == DEF_DRAW_X_ALIGN_CENTER)
					x_start[lines] = ((box_size_x - used_x) / 2) + x;
				else if(x_align == DEF_DRAW_X_ALIGN_RIGHT)
					x_start[lines] = box_size_x - used_x + x;

				used_y_max = 0;
				used_x = 0;
				lines++;
				continue;
			}

			if(!Exfont_is_loaded_external_font(0) || (font_list[1][i] >= 0 && font_list[1][i] <= 3))
			{
				if(!Exfont_is_loaded_external_font(0))
					font_list[1][i] = 0;

				if(font_list[1][i] == 1)
				{
					font_width_factor = 1;
					font_height_factor = 0.83333;
				}
				else if(font_list[1][i] == 2)
				{
					font_width_factor = 1;
					font_height_factor = 0.83333;
				}
				else if(font_list[1][i] == 3)
				{
					font_width_factor = 1.2;
					font_height_factor = 0.83333;
				}
				else
				{
					font_width_factor = 1;
					font_height_factor = 1;
				}

				C2D_TextBufClear(c2d_buf);
				C2D_TextFontParse(&c2d_text, util_draw_system_fonts[font_list[1][i]], c2d_buf, util_draw_part_text[1][i].c_str());
				C2D_TextOptimize(&c2d_text);
				C2D_TextGetDimensions(&c2d_text, text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
			}
			else if(font_list[1][i] == 4)
			{
				font_width_factor = 1.2;
				font_height_factor = 1.2;
				Exfont_draw_get_text_size(util_draw_part_text[1][i], text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
			}
			
			used_x += width;
			if(used_x > used_x_max)
				used_x_max = used_x;
			if(height > used_y_max)
				used_y_max = height;
		}
		used_y_max = y_offset + used_y_max;

		if(x_align == DEF_DRAW_X_ALIGN_CENTER)
			x_start[lines] = ((box_size_x - used_x) / 2.0) + x;
		else if(x_align == DEF_DRAW_X_ALIGN_RIGHT)
			x_start[lines] = box_size_x - used_x + x;
		if(y_align == DEF_DRAW_Y_ALIGN_CENTER)
			y = ((box_size_y - used_y_max) / 2.0) + y;
		else if(y_align == DEF_DRAW_Y_ALIGN_BOTTOM)
			y = box_size_y - used_y_max + y;

		lines++;

		if(x_align == DEF_DRAW_X_ALIGN_LEFT)
		{
			x = original_x;
			x_min = original_x;
		}
		else if(x_align == DEF_DRAW_X_ALIGN_CENTER || x_align == DEF_DRAW_X_ALIGN_RIGHT)
		{
			x = x_start[line_count];
			x_min = x_start[0];
			for (int i = 1; i < lines; i++)
			{
				if(x_min > x_start[i])
					x_min = x_start[i];
			}
		}

		if(texture_position == DEF_DRAW_BACKGROUND_ENTIRE_BOX)
			Draw_texture(background_image, texture_abgr8888, original_x, original_y, box_size_x, box_size_y);
		else if(texture_position == DEF_DRAW_BACKGROUND_UNDER_TEXT)
			Draw_texture(background_image, texture_abgr8888, x_min, y, used_x_max, used_y_max);
	}

	for (int i = 0; i <= array_count; i++)
	{
		if (font_list[1][i] == -2)
			break;
		else if (font_list[1][i] == -1)
		{
			y += 30 * text_size_y;
			if(x_align == DEF_DRAW_X_ALIGN_LEFT)
				x = original_x;
			else if(x_align == DEF_DRAW_X_ALIGN_CENTER || x_align == DEF_DRAW_X_ALIGN_RIGHT)
			{
				line_count++;
				x = x_start[line_count];
			}
			else
				x = original_x;

			continue;
		}

		if(!Exfont_is_loaded_external_font(0) || (font_list[1][i] >= 0 && font_list[1][i] <= 3))
		{
			if(!Exfont_is_loaded_external_font(0))
				font_list[1][i] = 0;

			C2D_TextBufClear(c2d_buf);
			if(font_list[1][i] == 1)
			{
				y_offset = 4 * text_size_y;
				font_width_factor = 1;
				font_height_factor = 0.83333;
				font_draw_width_factor = 0.83333;
				font_draw_height_factor = 0.83333;
			}
			else if(font_list[1][i] == 2)
			{
				y_offset = 4 * text_size_y;
				font_width_factor = 1;
				font_height_factor = 0.83333;
				font_draw_width_factor = 0.83333;
				font_draw_height_factor = 0.83333;
			}
			else if(font_list[1][i] == 3)
			{
				y_offset = 4.5 * text_size_y;
				font_width_factor = 1.2;
				font_height_factor = 0.83333;
				font_draw_width_factor = 0.83333;
				font_draw_height_factor = 0.83333;
			}
			else
			{
				y_offset = 0;
				font_width_factor = 1;
				font_height_factor = 1;
				font_draw_width_factor = 1;
				font_draw_height_factor = 1;
			}

			C2D_TextFontParse(&c2d_text, util_draw_system_fonts[font_list[1][i]], c2d_buf, util_draw_part_text[1][i].c_str());
			C2D_TextOptimize(&c2d_text);
			C2D_TextGetDimensions(&c2d_text, text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
			C2D_DrawText(&c2d_text, C2D_WithColor, x, y + y_offset, 0.0, text_size_x * font_draw_width_factor, text_size_y * font_draw_height_factor, abgr8888);
		}
		else if(font_list[1][i] == 4)
		{
			font_draw_width_factor = 1.2;
			font_draw_height_factor = 1.2;
			Exfont_draw_external_fonts(util_draw_part_text[1][i], x, y, text_size_x * font_draw_width_factor, text_size_y * font_draw_height_factor, abgr8888, &width, &height);
		}

		x += width;
	}
	C2D_TextBufDelete(c2d_buf);
}

//todo : merge code
void Draw(std::string text, float x, float y, float text_size_x, float text_size_y, int abgr8888, int x_align, int y_align,
 float box_size_x, float box_size_y, int texture_position, Image_data* background_image, int texture_abgr8888)
{
	short font_list[2][1024];
	float x_start [1024];
	int lines = 0;
	int line_count = 0;
	int array_count = 0;
	float width = 0, height = 0, original_x = 0, original_y = 0, x_min = 0, y_offset = 0, used_x = 0, used_x_max = 0, used_y_max = 0;
	double font_height_factor = 1, font_width_factor = 1, font_draw_height_factor = 1, font_draw_width_factor = 1;
	C2D_Text c2d_text;
	C2D_TextBuf c2d_buf;
	original_x = x;
	original_y = y;

	if(!util_draw_init)
		return;
	
	if((x_align != DEF_DRAW_X_ALIGN_LEFT && x_align != DEF_DRAW_X_ALIGN_CENTER && x_align != DEF_DRAW_X_ALIGN_RIGHT)
	|| (y_align != DEF_DRAW_Y_ALIGN_TOP && y_align != DEF_DRAW_Y_ALIGN_CENTER && y_align != DEF_DRAW_Y_ALIGN_BOTTOM)
	|| (texture_position != DEF_DRAW_BACKGROUND_NONE && texture_position != DEF_DRAW_BACKGROUND_ENTIRE_BOX && texture_position != DEF_DRAW_BACKGROUND_UNDER_TEXT))
		return;

	c2d_buf = C2D_TextBufNew(4096);
	if(!c2d_buf)
		return;


	Draw_parse_text(text, &array_count, font_list);

	if(x_align == DEF_DRAW_X_ALIGN_LEFT && y_align == DEF_DRAW_Y_ALIGN_TOP && texture_position == DEF_DRAW_BACKGROUND_NONE)
			x = original_x;
	else
	{
		for (int i = 0; i <= array_count; i++)
		{
			if (font_list[1][i] == -2)
				break;
			else if (font_list[1][i] == -1)
			{
				height = 30 * text_size_y;
				if(height > used_y_max)
					used_y_max = height;

				y_offset += used_y_max;
				if(x_align == DEF_DRAW_X_ALIGN_CENTER)
					x_start[lines] = ((box_size_x - used_x) / 2) + x;
				else if(x_align == DEF_DRAW_X_ALIGN_RIGHT)
					x_start[lines] = box_size_x - used_x + x;

				used_y_max = 0;
				used_x = 0;
				lines++;
				continue;
			}

			if(!Exfont_is_loaded_external_font(0) || (font_list[1][i] >= 0 && font_list[1][i] <= 3))
			{
				if(!Exfont_is_loaded_external_font(0))
					font_list[1][i] = 0;

				if(font_list[1][i] == 1)
				{
					font_width_factor = 1;
					font_height_factor = 0.83333;
				}
				else if(font_list[1][i] == 2)
				{
					font_width_factor = 1;
					font_height_factor = 0.83333;
				}
				else if(font_list[1][i] == 3)
				{
					font_width_factor = 1.2;
					font_height_factor = 0.83333;
				}
				else
				{
					font_width_factor = 1;
					font_height_factor = 1;
				}

				C2D_TextBufClear(c2d_buf);
				C2D_TextFontParse(&c2d_text, util_draw_system_fonts[font_list[1][i]], c2d_buf, util_draw_part_text[1][i].c_str());
				C2D_TextOptimize(&c2d_text);
				C2D_TextGetDimensions(&c2d_text, text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
			}
			else if(font_list[1][i] == 4)
			{
				font_width_factor = 1.2;
				font_height_factor = 1.2;
				Exfont_draw_get_text_size(util_draw_part_text[1][i], text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
			}
			
			used_x += width;
			if(used_x > used_x_max)
				used_x_max = used_x;
			if(height > used_y_max)
				used_y_max = height;
		}
		used_y_max = y_offset + used_y_max;

		if(x_align == DEF_DRAW_X_ALIGN_CENTER)
			x_start[lines] = ((box_size_x - used_x) / 2.0) + x;
		else if(x_align == DEF_DRAW_X_ALIGN_RIGHT)
			x_start[lines] = box_size_x - used_x + x;
		if(y_align == DEF_DRAW_Y_ALIGN_CENTER)
			y = ((box_size_y - used_y_max) / 2.0) + y;
		else if(y_align == DEF_DRAW_Y_ALIGN_BOTTOM)
			y = box_size_y - used_y_max + y;

		lines++;

		if(x_align == DEF_DRAW_X_ALIGN_LEFT)
		{
			x = original_x;
			x_min = original_x;
		}
		else if(x_align == DEF_DRAW_X_ALIGN_CENTER || x_align == DEF_DRAW_X_ALIGN_RIGHT)
		{
			x = x_start[line_count];
			x_min = x_start[0];
			for (int i = 1; i < lines; i++)
			{
				if(x_min > x_start[i])
					x_min = x_start[i];
			}
		}

		if(texture_position == DEF_DRAW_BACKGROUND_ENTIRE_BOX)
			Draw_texture(background_image, texture_abgr8888, original_x, original_y, box_size_x, box_size_y);
		else if(texture_position == DEF_DRAW_BACKGROUND_UNDER_TEXT)
			Draw_texture(background_image, texture_abgr8888, x_min, y, used_x_max, used_y_max);
	}

	for (int i = 0; i <= array_count; i++)
	{
		if (font_list[1][i] == -2)
			break;
		else if (font_list[1][i] == -1)
		{
			y += 30 * text_size_y;
			if(x_align == DEF_DRAW_X_ALIGN_LEFT)
				x = original_x;
			else if(x_align == DEF_DRAW_X_ALIGN_CENTER || x_align == DEF_DRAW_X_ALIGN_RIGHT)
			{
				line_count++;
				x = x_start[line_count];
			}
			else
				x = original_x;

			continue;
		}

		if(!Exfont_is_loaded_external_font(0) || (font_list[1][i] >= 0 && font_list[1][i] <= 3))
		{
			if(!Exfont_is_loaded_external_font(0))
				font_list[1][i] = 0;

			C2D_TextBufClear(c2d_buf);
			if(font_list[1][i] == 1)
			{
				y_offset = 4 * text_size_y;
				font_width_factor = 1;
				font_height_factor = 0.83333;
				font_draw_width_factor = 0.83333;
				font_draw_height_factor = 0.83333;
			}
			else if(font_list[1][i] == 2)
			{
				y_offset = 4 * text_size_y;
				font_width_factor = 1;
				font_height_factor = 0.83333;
				font_draw_width_factor = 0.83333;
				font_draw_height_factor = 0.83333;
			}
			else if(font_list[1][i] == 3)
			{
				y_offset = 4.5 * text_size_y;
				font_width_factor = 1.2;
				font_height_factor = 0.83333;
				font_draw_width_factor = 0.83333;
				font_draw_height_factor = 0.83333;
			}
			else
			{
				y_offset = 0;
				font_width_factor = 1;
				font_height_factor = 1;
				font_draw_width_factor = 1;
				font_draw_height_factor = 1;
			}

			C2D_TextFontParse(&c2d_text, util_draw_system_fonts[font_list[1][i]], c2d_buf, util_draw_part_text[1][i].c_str());
			C2D_TextOptimize(&c2d_text);
			C2D_TextGetDimensions(&c2d_text, text_size_x * font_width_factor, text_size_y * font_height_factor, &width, &height);
			C2D_DrawText(&c2d_text, C2D_WithColor, x, y + y_offset, 0.0, text_size_x * font_draw_width_factor, text_size_y * font_draw_height_factor, abgr8888);
		}
		else if(font_list[1][i] == 4)
		{
			font_draw_width_factor = 1.2;
			font_draw_height_factor = 1.2;
			Exfont_draw_external_fonts(util_draw_part_text[1][i], x, y, text_size_x * font_draw_width_factor, text_size_y * font_draw_height_factor, abgr8888, &width, &height);
		}

		x += width;
	}
	C2D_TextBufDelete(c2d_buf);
}

int Draw_get_free_sheet_num(void)
{
	if(!util_draw_init)
		return -1;
	
	for(int i = 0; i < DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS; i++)
	{
		if(util_draw_sheet_texture_free[i])
			return i;
	}
	return -1;
}

Result_with_string Draw_load_texture(std::string file_path, int sheet_map_num, C2D_Image return_image[], int start_num, int num_of_array)
{
	size_t num_of_images  = 0;
	Result_with_string result;
	
	if(file_path == "" || sheet_map_num < 0 || sheet_map_num > DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS || !return_image
	|| start_num < 0 || num_of_array < 0 || !util_draw_sheet_texture_free[sheet_map_num])
		goto invalid_arg;

	util_draw_sheet_texture[sheet_map_num] = C2D_SpriteSheetLoad(file_path.c_str());
	if (!util_draw_sheet_texture[sheet_map_num])
	{
		result.error_description = "[Error] Couldn't load texture file : " + file_path + " ";
		goto other;
	}

	num_of_images = C2D_SpriteSheetCount(util_draw_sheet_texture[sheet_map_num]);
	if (num_of_array < (int)num_of_images)
		goto out_of_memory;

	for (int i = 0; i <= (num_of_array - 1); i++)
		return_image[start_num + i] = C2D_SpriteSheetGetImage(util_draw_sheet_texture[sheet_map_num], i);

	util_draw_sheet_texture_free[sheet_map_num] = false;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	other:
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

void Draw_free_texture(int sheet_map_num)
{
	if(!util_draw_init)
		return;

	if(sheet_map_num >= 0 && sheet_map_num < DEF_DRAW_MAX_NUM_OF_SPRITE_SHEETS)
	{
		if (util_draw_sheet_texture[sheet_map_num])
		{
			C2D_SpriteSheetFree(util_draw_sheet_texture[sheet_map_num]);
			util_draw_sheet_texture[sheet_map_num] = NULL;
		}
		util_draw_sheet_texture_free[sheet_map_num] = true;
	}
}

void Draw_top_ui(void)
{
	if(!util_draw_init)
		return;

	Draw_texture(var_square_image[0], DEF_DRAW_BLACK, 0.0, 0.0, 400.0, 15.0);
	Draw_texture(util_draw_wifi_icon_image[var_wifi_signal], DEF_DRAW_NO_COLOR, 360.0, 0.0, 15.0, 15.0);
	Draw_texture(util_draw_battery_level_icon_image[var_battery_level_raw / 5], DEF_DRAW_NO_COLOR, 315.0, 0.0, 30.0, 15.0);
	Draw_texture(util_draw_eco_image[var_eco_mode], DEF_DRAW_NO_COLOR, 345.0, 0.0, 15.0, 15.0);
	if (var_battery_charge)
		Draw_texture(util_draw_battery_charge_icon_image[0], DEF_DRAW_NO_COLOR, 295.0, 0.0, 20.0, 15.0);
	Draw(var_status, 0.0, 0.0, 0.45, 0.45, DEF_DRAW_GREEN);
	Draw(std::to_string(var_battery_level_raw), 322.5, 1.25, 0.425, 0.425, DEF_DRAW_BLACK);

	if (var_debug_mode)
		Draw_debug_info();
}

void Draw_bot_ui(void)
{
	if(!util_draw_init)
		return;

	Draw_texture(&util_draw_bot_ui, DEF_DRAW_BLACK, 0.0, 225.0, 320.0, 15.0);
	Draw("▽", 155.0, 220.0, 0.75, 0.75, DEF_DRAW_WHITE);
}

Image_data* Draw_get_bot_ui_button(void)
{
	if(!util_draw_init)
		return NULL;

	return &util_draw_bot_ui;
}

void Draw_texture(C2D_Image image, float x, float y, float x_size, float y_size)
{
	Draw_texture(image, DEF_DRAW_NO_COLOR, x, y, x_size, y_size);
}

void Draw_texture(C2D_Image image, int abgr8888, float x, float y, float x_size, float y_size)
{
	Draw_texture(image, abgr8888, x, y, x_size, y_size, 0, 0, 0);
}

void Draw_texture(C2D_Image image, int abgr8888, float x, float y, float x_size, float y_size, float angle, float center_x, float center_y)
{
	if(!image.tex || !image.subtex || x_size <= 0 || y_size <= 0)
		return;

	C2D_ImageTint tint;
	C2D_DrawParams c2d_parameter =
	{
		{
			x,
			y,
			x_size,
			y_size
		},
		{
			center_x,
			center_y
		},
		0.0f,
		angle
	};
	if(!util_draw_init)
		return;

	if(!image.tex)
		return;

	if(abgr8888 == DEF_DRAW_NO_COLOR)
		C2D_DrawImage(image, &c2d_parameter, NULL);
	else
	{
		C2D_PlainImageTint(&tint, abgr8888, true);
		C2D_DrawImage(image, &c2d_parameter, &tint);
	}
}

void Draw_texture(Image_data* image, float x, float y, float x_size, float y_size)
{
	Draw_texture(image, DEF_DRAW_NO_COLOR, x, y, x_size, y_size);
}

void Draw_texture(Image_data* image, int abgr8888, float x, float y, float x_size, float y_size)
{
	Draw_texture(image, abgr8888, x, y, x_size, y_size, 0, 0, 0);
}

void Draw_texture(Image_data* image, int abgr8888, float x, float y, float x_size, float y_size, float angle, float center_x, float center_y)
{
	if(!util_draw_init)
		return;

	if(!image || !image->c2d.tex)
		return;

	image->x = x;
	image->y = y;
	image->x_size = x_size;
	image->y_size = y_size;
	Draw_texture(image->c2d, abgr8888, x, y, x_size, y_size, angle, center_x, center_y);
}

void Draw_line(float x_0, float y_0, int abgr8888_0, float x_1, float y_1, int abgr8888_1, float width)
{
	if(!util_draw_init)
		return;

	C2D_DrawRectangle(0, 0, 0, 0, 0, 0x0, 0x0, 0x0, 0x0);
	//magic C2D_DrawLine() won't work without calling C2D_DrawRectangle()
	C2D_DrawLine(x_0, y_0, abgr8888_0, x_1, y_1, abgr8888_1, width, 0);
}

#if DEF_ENABLE_CPU_MONITOR_API
void Draw_cpu_usage_info(void)
{
	int char_length = 0;
	char msg_cache[128];
	if(!util_draw_init)
		return;

	char_length = snprintf(msg_cache, 128, "CPU : %.1f%%", Util_cpu_usage_monitor_get_cpu_usage(-1));
	for(int i = 0; i < 4; i++)
		char_length += snprintf((msg_cache + char_length), 128 - char_length, "\nCore #%d : %.1f%%", i, Util_cpu_usage_monitor_get_cpu_usage(i));

	snprintf((msg_cache + char_length), 128 - char_length, "\n(#1 max : %ld%%)", Util_get_core_1_max());

	Draw(msg_cache, 300, 25, 0.4, 0.4, DEF_DRAW_BLACK, DEF_DRAW_X_ALIGN_RIGHT, DEF_DRAW_Y_ALIGN_CENTER, 100, 60,
	DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], 0x80FFFFFF);
}
#endif

void Draw_debug_info(void)
{
	int color = DEF_DRAW_BLACK;
	Hid_info key;
	Util_hid_query_key_state(&key);
	
	if (var_night_mode)
		color = DEF_DRAW_WHITE;

	Draw("A p: " + std::to_string(key.p_a) + " h: " + std::to_string(key.h_a) + " B p: " + std::to_string(key.p_b) + " h: " + std::to_string(key.h_b), 0, 20, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("X p: " + std::to_string(key.p_x) + " h: " + std::to_string(key.h_x) + " Y p: " + std::to_string(key.p_y) + " h: " + std::to_string(key.h_y), 0, 30, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("L p: " + std::to_string(key.p_l) + " h: " + std::to_string(key.h_l) + " R p: " + std::to_string(key.p_r) + " h: " + std::to_string(key.h_r), 0, 40, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("ZL p: " + std::to_string(key.p_zl) + " h: " + std::to_string(key.h_zl) + " ZR p: " + std::to_string(key.p_zr) + " h: " + std::to_string(key.h_zr), 0, 50, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("C↓ p: " + std::to_string(key.p_c_down) + " h: " + std::to_string(key.h_c_down) + " C→ p: " + std::to_string(key.p_c_right) + " h: " + std::to_string(key.h_c_right), 0, 60, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("C↑ p: " + std::to_string(key.p_c_up) + " h: " + std::to_string(key.h_c_up) + " C← p: " + std::to_string(key.p_c_left) + " h: " + std::to_string(key.h_c_left), 0, 70, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("D↓ p: " + std::to_string(key.p_d_down) + " h: " + std::to_string(key.h_d_down) + " D→ p: " + std::to_string(key.p_d_right) + " h: " + std::to_string(key.h_d_right), 0, 80, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("D↑ p: " + std::to_string(key.p_d_up) + " h: " + std::to_string(key.h_d_up) + " D← p: " + std::to_string(key.p_d_left) + " h: " + std::to_string(key.h_d_left), 0, 90, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("CS↓ p: " + std::to_string(key.p_cs_down) + " h: " + std::to_string(key.h_cs_down) + " CS→ p: " + std::to_string(key.p_cs_right) + " h: " + std::to_string(key.h_cs_right), 0, 100, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("CS↑ p: " + std::to_string(key.p_cs_up) + " h: " + std::to_string(key.h_cs_up) + " CS← p: " + std::to_string(key.p_cs_left) + " h: " + std::to_string(key.h_cs_left), 0, 110, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("START p: " + std::to_string(key.p_start) + " h: " + std::to_string(key.h_start), 0, 120, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("SELET p: " + std::to_string(key.p_select) + " h: " + std::to_string(key.h_select), 0, 130, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("touch x: " + std::to_string(key.touch_x) + ", y: " + std::to_string(key.touch_y), 0, 140, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("CPU: " + std::to_string(C3D_GetProcessingTime()).substr(0, 5) + "ms", 0, 150, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("GPU: " + std::to_string(C3D_GetDrawingTime()).substr(0, 5) + "ms", 0, 160, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("Frametime: " + std::to_string(util_draw_frametime).substr(0, 6) + "ms", 0, 170, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("RAM: " + std::to_string(var_free_ram / 1024.0 / 1024.0).substr(0, 5) + " MB", 0, 180, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("linear RAM: " + std::to_string(var_free_linear_ram / 1024.0 / 1024.0).substr(0, 5) +" MB", 0, 190, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("Watch(bool): " + std::to_string(Util_get_watch_bool_usage()) + "/" + std::to_string(DEF_DRAW_MAX_WATCH_BOOL_VARIABLES) + "(" + std::to_string((double)Util_get_watch_bool_usage() / DEF_DRAW_MAX_WATCH_BOOL_VARIABLES * 100).substr(0, 4)+ "%)", 0, 200, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("Watch(int): " + std::to_string(Util_get_watch_int_usage()) + "/" + std::to_string(DEF_DRAW_MAX_WATCH_INT_VARIABLES) + "(" + std::to_string((double)Util_get_watch_int_usage() / DEF_DRAW_MAX_WATCH_INT_VARIABLES * 100).substr(0, 4) + "%)", 0, 210, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("Watch(double): " + std::to_string(Util_get_watch_double_usage()) + "/" + std::to_string(DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES) + "(" + std::to_string((double)Util_get_watch_double_usage() / DEF_DRAW_MAX_WATCH_DOUBLE_VARIABLES * 100).substr(0, 4) + "%)", 0, 220, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
	Draw("Watch(string): " + std::to_string(Util_get_watch_string_usage()) + "/" + std::to_string(DEF_DRAW_MAX_WATCH_STRING_VARIABLES) + "(" + std::to_string((double)Util_get_watch_string_usage() / DEF_DRAW_MAX_WATCH_STRING_VARIABLES * 100).substr(0, 4) + "%)", 0, 230, 0.35, 0.35, color, DEF_DRAW_X_ALIGN_LEFT, DEF_DRAW_Y_ALIGN_CENTER, 300, 10, DEF_DRAW_BACKGROUND_UNDER_TEXT, var_square_image[0], DEF_DRAW_WEAK_BLUE);
}

Result_with_string Draw_load_kanji_samples(void)
{
	int characters = 0;
	u8* fs_buffer = NULL;
	u32 read_size = 0;
	Result_with_string result;

	result = Util_file_load_from_rom("kanji.txt", "romfs:/gfx/font/sample/", &fs_buffer, 0x8000, &read_size);
	if(result.code == 0)
	{
		Exfont_text_parse((char*)fs_buffer, util_draw_japanese_kanji, 3000, &characters);
		Util_safe_linear_free(fs_buffer);
		fs_buffer = NULL;
	}

	result = Util_file_load_from_rom("hanyu_s.txt", "romfs:/gfx/font/sample/", &fs_buffer, 0x8000, &read_size);
	if(result.code == 0)
	{
		Exfont_text_parse((char*)fs_buffer, util_draw_simple_chinese, 6300, &characters);
		Util_safe_linear_free(fs_buffer);
		fs_buffer = NULL;
	}

	return result;
}

Result_with_string Draw_load_system_font(int system_font_num)
{
	Result_with_string result;
	if(!util_draw_init)
		goto not_inited;
	
	if(system_font_num < 0 || system_font_num > 3)
		goto invalid_arg;

	if (system_font_num == 0)
		util_draw_system_fonts[0] = C2D_FontLoadSystem(CFG_REGION_JPN);
	else if (system_font_num == 1)
		util_draw_system_fonts[1] = C2D_FontLoadSystem(CFG_REGION_CHN);
	else if (system_font_num == 2)
		util_draw_system_fonts[2] = C2D_FontLoadSystem(CFG_REGION_KOR);
	else if (system_font_num == 3)
		util_draw_system_fonts[3] = C2D_FontLoadSystem(CFG_REGION_TWN);

	if(!util_draw_system_fonts[system_font_num])
	{
		goto other;
		result.error_description = "[Error] Couldn't load font : " + std::to_string(system_font_num) + " ";
	}

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	other:
	result.code = DEF_ERR_OTHER;
	result.string = DEF_ERR_OTHER_STR;
	return result;
}

void Draw_free_system_font(int system_font_num)
{
	if(!util_draw_init)
		return;

	if (system_font_num >= 0 && system_font_num <= 3)
	{
		if (util_draw_system_fonts[system_font_num])
		{
			C2D_FontFree(util_draw_system_fonts[system_font_num]);
			util_draw_system_fonts[system_font_num] = NULL;
		}
	}
}

void Draw_frame_ready(void)
{
	if(!util_draw_init)
		return;

	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
}

void Draw_screen_ready(int screen_num, int abgr8888)
{
	if(!util_draw_init)
		return;

	if (screen_num != DEF_DRAW_SCREEN_TOP_LEFT && screen_num != DEF_DRAW_SCREEN_BOTTOM
	&& screen_num != DEF_DRAW_SCREEN_TOP_RIGHT)
		return;

	C2D_TargetClear(util_draw_screen[screen_num], abgr8888);
	C2D_SceneBegin(util_draw_screen[screen_num]);
}

void Draw_apply_draw(void)
{
	if(!util_draw_init)
		return;

	C3D_FrameEnd(0);
	osTickCounterUpdate(&util_draw_frame_time_stopwatch);
	util_draw_frametime = osTickCounterRead(&util_draw_frame_time_stopwatch);

	util_draw_rendered_frames_cache++;
	if(osGetTime() >= util_draw_reset_fps_counter_time)
	{
		util_draw_rendered_frames = util_draw_rendered_frames_cache;
		util_draw_rendered_frames_cache = 0;
		util_draw_reset_fps_counter_time = osGetTime() + 1000;
	}
}
