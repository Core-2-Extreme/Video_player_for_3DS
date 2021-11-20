#include "system/headers.hpp"

bool util_swkbd_init = false;
int util_swkbd_max_length = 0;
std::string util_swkbd_hint_text = "";
std::string util_swkbd_init_text = "";
SwkbdStatusData util_swkbd_state;
SwkbdLearningData util_swkbd_learn_data;
SwkbdDictWord util_swkbd_user_words[DEF_SWKBD_MAX_DIC_WORDS];
SwkbdState util_swkbd;

Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text)
{
	u32 feature = 0;
	SwkbdPasswordMode password_mode = SWKBD_PASSWORD_NONE;
	return Util_swkbd_init(type, valid_type, num_of_button, max_length, hint_text, init_text, password_mode, feature);
}

Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text,
u32 feature)
{
	SwkbdPasswordMode password_mode = SWKBD_PASSWORD_NONE;
	return Util_swkbd_init(type, valid_type, num_of_button, max_length, hint_text, init_text, password_mode, feature);
}

Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text,
SwkbdPasswordMode password_mode)
{
	u32 feature = 0;
	return Util_swkbd_init(type, valid_type, num_of_button, max_length, hint_text, init_text, password_mode, feature);
}

Result_with_string Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text,
SwkbdPasswordMode password_mode, u32 feature)
{
	Result_with_string result;
	if(util_swkbd_init)
		goto already_init;

	if(type < 0 || type > 3 || valid_type < 0 || valid_type > 4 || num_of_button <= 0 || num_of_button > 3 || max_length <= 0
	|| password_mode < 0 || password_mode > 2)
		goto invalid_arg;
	
	util_swkbd_hint_text = hint_text;
	util_swkbd_init_text = init_text;
	util_swkbd_max_length = max_length;
	swkbdInit(&util_swkbd, type, num_of_button, max_length);
	swkbdSetHintText(&util_swkbd, util_swkbd_hint_text.c_str());
	swkbdSetValidation(&util_swkbd, valid_type, 0, 0);
	swkbdSetInitialText(&util_swkbd, util_swkbd_init_text.c_str());
	swkbdSetStatusData(&util_swkbd, &util_swkbd_state, true, true);
	swkbdSetLearningData(&util_swkbd, &util_swkbd_learn_data, true, true);
	swkbdSetPasswordMode(&util_swkbd, password_mode);
	swkbdSetFeatures(&util_swkbd, feature);
	util_swkbd_init = true;

	return result;

	already_init:
	result.code = DEF_ERR_ALREADY_INITIALIZED;
	result.string = DEF_ERR_ALREADY_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;
}

Result_with_string Util_swkbd_set_dic_word(std::string first_spell[], std::string full_spell[], int num_of_word)
{
	Result_with_string result;
	if(!util_swkbd_init)
		goto not_inited;
	
	if(!first_spell || !full_spell || num_of_word <= 0 || num_of_word > DEF_SWKBD_MAX_DIC_WORDS)
		goto invalid_arg;

	for(int i = 0; i < num_of_word; i++)
		swkbdSetDictWord(&util_swkbd_user_words[i], first_spell[i].c_str(), full_spell[i].c_str());

	swkbdSetDictionary(&util_swkbd, util_swkbd_user_words, num_of_word);

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

Result_with_string Util_swkbd_launch(std::string* out_data)
{
	int pressed_button = 0;
	return Util_swkbd_launch(out_data, &pressed_button);
}

Result_with_string Util_swkbd_launch(std::string* out_data, int* pressed_button)
{
	char* input_text = NULL;
	SwkbdButton button;
	Result_with_string result;

	if(!util_swkbd_init)
		goto not_inited;

	if(!out_data || !pressed_button)
		goto invalid_arg;
	
	input_text = (char*)malloc(util_swkbd_max_length + 1);
	if(!input_text)
		goto out_of_memory;

	memset(input_text, 0x0, util_swkbd_max_length + 1);
	button = swkbdInputText(&util_swkbd, input_text, util_swkbd_max_length + 1);
	*out_data = input_text;
	free(input_text);
	input_text = NULL;

	if(button == SWKBD_BUTTON_LEFT)
		*pressed_button = DEF_SWKBD_BUTTON_LEFT;
	else if(button == SWKBD_BUTTON_MIDDLE)
		*pressed_button = DEF_SWKBD_BUTTON_MIDDLE;
	else if(button == SWKBD_BUTTON_RIGHT)
		*pressed_button = DEF_SWKBD_BUTTON_RIGHT;
	else if(button == SWKBD_BUTTON_NONE)
		*pressed_button = DEF_SWKBD_BUTTON_NONE;

	return result;

	not_inited:
	result.code = DEF_ERR_NOT_INITIALIZED;
	result.string = DEF_ERR_NOT_INITIALIZED_STR;
	return result;

	invalid_arg:
	result.code = DEF_ERR_INVALID_ARG;
	result.string = DEF_ERR_INVALID_ARG_STR;
	return result;

	out_of_memory:
	result.code = DEF_ERR_OUT_OF_MEMORY;
	result.string = DEF_ERR_OUT_OF_MEMORY_STR;
	return result;
}

void Util_swkbd_exit(void)
{
	std::string text = "";
	if(!util_swkbd_init)
		return;

	util_swkbd_init = false;
	util_swkbd_max_length = 1;
	swkbdInit(&util_swkbd, SWKBD_TYPE_QWERTY, 1, 1);
	swkbdSetHintText(&util_swkbd, text.c_str());
	swkbdSetValidation(&util_swkbd, SWKBD_ANYTHING, 0, 0);
	swkbdSetInitialText(&util_swkbd, text.c_str());
	swkbdSetStatusData(&util_swkbd, &util_swkbd_state, true, true);
	swkbdSetLearningData(&util_swkbd, &util_swkbd_learn_data, true, true);
	swkbdSetPasswordMode(&util_swkbd, SWKBD_PASSWORD_NONE);
	swkbdSetFeatures(&util_swkbd, 0);
	swkbdSetDictionary(&util_swkbd, util_swkbd_user_words, 0);
}
