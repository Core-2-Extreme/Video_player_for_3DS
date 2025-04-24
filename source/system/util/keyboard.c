//Includes.
#include "system/util/keyboard.h"

#if DEF_KEYBOARD_API_ENABLE
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/log.h"
#include "system/util/str.h"

//Defines.
//N/A.

//Typedefs.
//N/A.

//Prototypes.
static uint32_t Util_keyboard_init_internal(Keyboard_type type, Keyboard_acceptable_input valid_type, Keyboard_display_button button_type,
uint32_t max_length, const char* hint_text, const char* init_text, Keyboard_password_mode password_mode, Keyboard_features_bit features);

//Variables.
static bool util_keyboard_init = false;
static uint32_t util_keyboard_max_length = 0;
static uint32_t util_keyboard_features = 0;
static int32_t util_keyboard_num_of_buttons = 0;
static Str_data util_keyboard_hint_text = { 0, };
static Str_data util_keyboard_init_text = { 0, };
static SwkbdValidInput util_keyboard_valid_type = SWKBD_ANYTHING;
static SwkbdPasswordMode util_keyboard_password_mode = SWKBD_PASSWORD_NONE;
static SwkbdType util_keyboard_type = SWKBD_TYPE_NORMAL;
static SwkbdStatusData util_keyboard_state = { 0, };
static SwkbdLearningData util_keyboard_learn_data = { 0, };
static SwkbdDictWord util_keyboard_user_words[DEF_KEYBOARD_MAX_DIC_WORDS] = { 0, };
static SwkbdState util_keyboard = { 0, };

//Code.
uint32_t Util_keyboard_init(Keyboard_type type, Keyboard_acceptable_input valid_type, Keyboard_display_button button_type,
uint32_t max_length, const Str_data* hint_text, const Str_data* init_text, Keyboard_password_mode password_mode, Keyboard_features_bit features)
{
	if(!Util_str_is_valid(hint_text) || !Util_str_is_valid(init_text))
		goto invalid_arg;

	return Util_keyboard_init_internal(type, valid_type, button_type, max_length, hint_text->buffer, init_text->buffer, password_mode, features);

	invalid_arg:
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_keyboard_set_dic_word(Str_data* first_spell, Str_data* full_spell, uint16_t num_of_word)
{
	if(!util_keyboard_init)
		goto not_inited;

	if(!first_spell || !full_spell || num_of_word == 0 || num_of_word > DEF_KEYBOARD_MAX_DIC_WORDS)
		goto invalid_arg;

	for(uint8_t i = 0; i < num_of_word; i++)
	{
		if(!Util_str_has_data(&first_spell[i]) || !Util_str_has_data(&full_spell[i]))
			goto invalid_arg;
	}

	for(uint8_t i = 0; i < num_of_word; i++)
		swkbdSetDictWord(&util_keyboard_user_words[i], first_spell[i].buffer, full_spell[i].buffer);

	swkbdSetDictionary(&util_keyboard, util_keyboard_user_words, num_of_word);

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;
}

uint32_t Util_keyboard_launch(Str_data* out_data, Keyboard_button* pressed_button)
{
	char* input_text = NULL;
	uint32_t result = DEF_ERR_OTHER;
	uint32_t input_buffer_size = 0;
	SwkbdButton button = SWKBD_BUTTON_NONE;

	if(!util_keyboard_init)
		goto not_inited;

	if(!out_data)//pressed_button can be NULL.
		goto invalid_arg;

	input_buffer_size = (util_keyboard_max_length * 4);//1 character may have up to 4 bytes.
	input_buffer_size++;//+1 for NULL terminator.
	input_text = (char*)malloc(input_buffer_size);
	if(!input_text)
		goto out_of_memory;

	result = Util_str_init(out_data);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_error;
	}

	memset(input_text, 0x0, input_buffer_size);
	swkbdInit(&util_keyboard, util_keyboard_type, util_keyboard_num_of_buttons, util_keyboard_max_length);//Number of characters.
	swkbdSetHintText(&util_keyboard, util_keyboard_hint_text.buffer);
	swkbdSetValidation(&util_keyboard, util_keyboard_valid_type, 0, 0);
	swkbdSetInitialText(&util_keyboard, util_keyboard_init_text.buffer);
	swkbdSetStatusData(&util_keyboard, &util_keyboard_state, true, true);
	swkbdSetLearningData(&util_keyboard, &util_keyboard_learn_data, true, true);
	swkbdSetPasswordMode(&util_keyboard, util_keyboard_password_mode);
	swkbdSetFeatures(&util_keyboard, util_keyboard_features);

	button = swkbdInputText(&util_keyboard, input_text, input_buffer_size);//Number of bytes.

	result = Util_str_set(out_data, input_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_error;
	}

	free(input_text);
	input_text = NULL;

	if(pressed_button)
	{
		if(button == SWKBD_BUTTON_LEFT)
			*pressed_button = KEYBOARD_BUTTON_LEFT;
		else if(button == SWKBD_BUTTON_MIDDLE)
			*pressed_button = KEYBOARD_BUTTON_MIDDLE;
		else if(button == SWKBD_BUTTON_RIGHT)
			*pressed_button = KEYBOARD_BUTTON_RIGHT;
		else if(button == SWKBD_BUTTON_NONE)
			*pressed_button = KEYBOARD_BUTTON_NONE;
	}

	return DEF_SUCCESS;

	not_inited:
	return DEF_ERR_NOT_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;

	api_error:
	free(input_text);
	input_text = NULL;
	Util_str_free(out_data);
	return result;
}

void Util_keyboard_exit(void)
{
	if(!util_keyboard_init)
		return;

	util_keyboard_init = false;
	util_keyboard_max_length = 0;
	util_keyboard_features = 0;
	util_keyboard_num_of_buttons = 0;
	util_keyboard_valid_type = SWKBD_ANYTHING;
	util_keyboard_password_mode = SWKBD_PASSWORD_NONE;
	util_keyboard_type = SWKBD_TYPE_NORMAL;
	swkbdSetDictionary(&util_keyboard, util_keyboard_user_words, 0);
}

static uint32_t Util_keyboard_init_internal(Keyboard_type type, Keyboard_acceptable_input valid_type, Keyboard_display_button button_type,
uint32_t max_length, const char* hint_text, const char* init_text, Keyboard_password_mode password_mode, Keyboard_features_bit features)
{
	uint32_t result = DEF_ERR_OTHER;

	if(util_keyboard_init)
		goto already_init;

	if(type <= KEYBOARD_TYPE_INVALID || type >= KEYBOARD_TYPE_MAX || valid_type <= KEYBOARD_ACCEPTABLE_INPUT_INVALID
	|| valid_type >= KEYBOARD_ACCEPTABLE_INPUT_MAX || button_type <= KEYBOARD_DISPLAY_BUTTON_INVALID || button_type >= KEYBOARD_DISPLAY_BUTTON_MAX
	|| max_length == 0 || !hint_text || !init_text || password_mode <= KEYBOARD_PASSWORD_MODE_INVALID || password_mode >= KEYBOARD_PASSWORD_MODE_MAX
	|| ((features & KEYBOARD_FEATURES_BIT_ALL) != features))
		goto invalid_arg;

	result = Util_str_init(&util_keyboard_hint_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_error;
	}

	result = Util_str_init(&util_keyboard_init_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto api_error;
	}

	result = Util_str_set(&util_keyboard_hint_text, hint_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_error;
	}

	result = Util_str_set(&util_keyboard_init_text, init_text);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto api_error;
	}

	if(type == KEYBOARD_TYPE_NORMAL)
		util_keyboard_type = SWKBD_TYPE_NORMAL;
	else if(type == KEYBOARD_TYPE_QWERTY)
		util_keyboard_type = SWKBD_TYPE_QWERTY;
	else if(type == KEYBOARD_TYPE_NUMPAD)
		util_keyboard_type = SWKBD_TYPE_NUMPAD;
	else if(type == KEYBOARD_TYPE_WESTERN)
		util_keyboard_type = SWKBD_TYPE_WESTERN;

	if(valid_type == KEYBOARD_ACCEPTABLE_INPUT_ANY)
		util_keyboard_valid_type = SWKBD_ANYTHING;
	else if(valid_type == KEYBOARD_ACCEPTABLE_INPUT_NO_EMPTY)
		util_keyboard_valid_type = SWKBD_NOTEMPTY_NOTBLANK;

	if(button_type == KEYBOARD_DISPLAY_BUTTON_MIDDLE)
		util_keyboard_num_of_buttons = 1;
	else if(button_type == KEYBOARD_DISPLAY_BUTTON_LEFT_MIDDLE)
		util_keyboard_num_of_buttons = 2;
	else if(button_type == KEYBOARD_DISPLAY_BUTTON_LEFT_MIDDLE_RIGHT)
		util_keyboard_num_of_buttons = 3;

	if(password_mode == KEYBOARD_PASSWORD_MODE_OFF)
		util_keyboard_password_mode = SWKBD_PASSWORD_NONE;
	else if(password_mode == KEYBOARD_PASSWORD_MODE_ON_DELAY)
		util_keyboard_password_mode = SWKBD_PASSWORD_HIDE_DELAY;
	else if(password_mode == KEYBOARD_PASSWORD_MODE_ON)
		util_keyboard_password_mode = SWKBD_PASSWORD_HIDE;

	if(features & KEYBOARD_FEATURES_BIT_DARKEN_SCREEN)
		util_keyboard_features |= SWKBD_DARKEN_TOP_SCREEN;
	if(features & KEYBOARD_FEATURES_BIT_PREDICTIVE_INPUT)
		util_keyboard_features |= SWKBD_PREDICTIVE_INPUT;
	if(features & KEYBOARD_FEATURES_BIT_MULTILINE)
		util_keyboard_features |= SWKBD_MULTILINE;
	if(features & KEYBOARD_FEATURES_BIT_ALLOW_HOME)
		util_keyboard_features |= SWKBD_ALLOW_HOME;

	util_keyboard_max_length = max_length;
	util_keyboard_init = true;
	return DEF_SUCCESS;

	already_init:
	return DEF_ERR_ALREADY_INITIALIZED;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	api_error:
	Util_str_free(&util_keyboard_init_text);
	Util_str_free(&util_keyboard_hint_text);
	return result;
}
#endif //DEF_KEYBOARD_API_ENABLE
