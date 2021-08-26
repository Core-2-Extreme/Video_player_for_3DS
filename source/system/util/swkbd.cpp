#include "headers.hpp"

std::string swkbd_hint_text;
std::string swkbd_init_text;
SwkbdStatusData swkbd_state;
SwkbdLearningData swkbd_learn_data;
SwkbdDictWord swkbd_words[DEF_SWKBD_MAX_DIC_WORDS];
SwkbdButton swkbd_press_button;
SwkbdState swkbd;

Result_with_string Util_swkbd_set_dic_word(std::string first_spell[], std::string full_spell[], int num_of_word)
{
	Result_with_string result;
	if(num_of_word <= DEF_SWKBD_MAX_DIC_WORDS)
	{
		for(int i = 0; i < num_of_word; i++)
			swkbdSetDictWord(&swkbd_words[i], first_spell[i].c_str(), full_spell[i].c_str());

		swkbdSetDictionary(&swkbd, swkbd_words, num_of_word);
	}
	else
	{
		result.code = DEF_ERR_OTHER;
		result.string = "Too many dic words.";
	}
	return result;
}

void Util_swkbd_init(SwkbdType type, SwkbdValidInput valid_type, int num_of_button, int max_length, std::string hint_text, std::string init_text)
{
	swkbd_hint_text = hint_text;
	swkbd_init_text = init_text;
	swkbdInit(&swkbd, type, num_of_button, max_length);
	swkbdSetHintText(&swkbd, swkbd_hint_text.c_str());
	swkbdSetValidation(&swkbd, valid_type, 0, 0);
	swkbdSetInitialText(&swkbd, swkbd_init_text.c_str());
	swkbdSetStatusData(&swkbd, &swkbd_state, true, true);
	swkbdSetLearningData(&swkbd, &swkbd_learn_data, true, true);
}

void Util_swkbd_set_password_mode(SwkbdPasswordMode password_mode)
{
	swkbdSetPasswordMode(&swkbd, password_mode);
}

void Util_swkbd_set_feature(u32 feature)
{
	swkbdSetFeatures(&swkbd, feature);
}

std::string Util_swkbd_launch(int max_length, std::string* out_data)
{
	max_length++;//for null terminator
	char swkb_input_text[max_length];
	std::string button = "";
	SwkbdButton press_button;
	memset(swkb_input_text, 0x0, max_length);
	press_button = swkbdInputText(&swkbd, swkb_input_text, max_length);
	*out_data = swkb_input_text;

	if(press_button == SWKBD_BUTTON_LEFT)
		button = "left";
	else if(press_button == SWKBD_BUTTON_MIDDLE)
		button = "middle";
	else if(press_button == SWKBD_BUTTON_RIGHT)
		button = "right";
	else if(press_button == SWKBD_BUTTON_NONE)
		button = "none";

	return button;
}
