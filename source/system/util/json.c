//Includes.
#include "system/util/json.h"

#if DEF_JSON_API_ENABLE
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "3ds.h"
#include "jsmn.h"

#include "system/util/err.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/util.h"

//Defines.
//Empty.

//Typedefs.
typedef enum
{
	JSON_INTERNAL_STATE_INVALID = -1,

	JSON_INTERNAL_STATE_SEARCHING_KEY,
	JSON_INTERNAL_STATE_SEARCHING_ARRAY_INDEX,
	JSON_INTERNAL_STATE_FOUND,

	JSON_INTERNAL_STATE_MAX,
} Json_internal_search_state;

typedef enum
{
	JSON_INTERNAL_UNESCAPE_STATE_INVALID = -1,

	JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_BACKSLASH,
	JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_ESCAPED_CHAR,
	JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_4_HEX_DIGITS,

	JSON_INTERNAL_UNESCAPE_STATE_MAX,
} Json_internal_unescaping_search_state;

typedef struct 
{
	char escaped_chars;
	char unescaped_chars;
} Json_internal_escape_char_list;

typedef struct
{
	bool is_object;
	uint32_t size;
	uint32_t current_index;
} Json_internal_list_data;

typedef struct
{
	uint32_t capacity;
	uint32_t used;
	Json_internal_list_data* data;
} Json_internal_list;

typedef struct
{
	uint32_t capacity;
	uint32_t raw_json_size;
	char* raw_json;
	jsmn_parser parser;
	jsmntok_t* tokens;
} Json_internal_data;

typedef struct
{
	bool is_array;
	char* key;
	uint32_t array_index;
} Json_internal_requested_key_data;

typedef struct
{
	const char* key;
	const char* eof;
	uint32_t capacity;
	uint32_t used;
	uint32_t current_index;
	Json_internal_search_state state;
	Json_internal_requested_key_data* data;
} Json_internal_requested_key;

static Json_internal_escape_char_list json_escape_char_list[] =
{
	{ .escaped_chars = '\"', .unescaped_chars = '\"', },
	{ .escaped_chars = '\\', .unescaped_chars = '\\', },
	{ .escaped_chars = '/', .unescaped_chars = '/',  },
	{ .escaped_chars = 'b', .unescaped_chars = '\b', },
	{ .escaped_chars = 'f', .unescaped_chars = '\f', },
	{ .escaped_chars = 'n', .unescaped_chars = '\n', },
	{ .escaped_chars = 'r', .unescaped_chars = '\r', },
	{ .escaped_chars = 't', .unescaped_chars = '\t', },
	//"u" is special, so it's not included here.
};

//Prototypes.
static uint32_t Util_json_unescape(const char* escaped_string, Str_data* unescaped_string);

//Variables.
//Empty.

//Code.
uint32_t Util_json_parse(const char* raw_json, Json_data* parsed_json_data)
{
	int32_t elements = 0;
	uint32_t raw_json_size = 0;
	Json_internal_data* data = NULL;

	if(!raw_json || !parsed_json_data)
		goto invalid_arg;

	raw_json_size = strlen(raw_json);

	if(raw_json_size <= 0)
		goto invalid_arg;

	Util_json_free(parsed_json_data);
	parsed_json_data->internal_data = (Json_internal_data*)malloc(sizeof(Json_internal_data));
	if(!parsed_json_data->internal_data)
	{
		DEF_LOG_RESULT(malloc, false, 0);
		goto out_of_memory;
	}

	data = (Json_internal_data*)(parsed_json_data->internal_data);
	data->capacity = 0;
	data->parser.pos = 0;
	data->parser.toknext = 0;
	data->parser.toksuper = 0;
	data->raw_json = NULL;
	data->raw_json_size = 0;
	data->tokens = NULL;

	data->raw_json = (char*)malloc(raw_json_size + 1);
	if(!data->raw_json)
	{
		DEF_LOG_RESULT(malloc, false, 0);
		goto out_of_memory;
	}

	data->raw_json_size = raw_json_size;
	memcpy(data->raw_json, raw_json, data->raw_json_size);
	data->raw_json[data->raw_json_size] = 0x00;//Add a NULL terminator.

	//Check how many tokens we need to allocate.
	jsmn_init(&data->parser);
	elements = jsmn_parse(&data->parser, data->raw_json, data->raw_json_size, NULL, 0);
	if(elements < 0)
	{
		DEF_LOG_RESULT(jsmn_parse, false, elements);
		goto jsmn_api_failed;
	}

	data->tokens = (jsmntok_t*)malloc(sizeof(jsmntok_t) * elements);
	if(!data->tokens)
	{
		DEF_LOG_RESULT(malloc, false, 0);
		goto out_of_memory;
	}

	data->capacity = elements;

	//Parse json data.
	jsmn_init(&data->parser);
	elements = jsmn_parse(&data->parser, data->raw_json, data->raw_json_size, data->tokens, elements);
	if(elements < 0)
	{
		DEF_LOG_RESULT(jsmn_parse, false, elements);
		goto jsmn_api_failed;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	Util_json_free(parsed_json_data);
	return DEF_ERR_OUT_OF_MEMORY;

	jsmn_api_failed:
	Util_json_free(parsed_json_data);
	return DEF_ERR_JSMN_RETURNED_NOT_SUCCESS;
}

uint32_t Util_json_get_data(const char* key, const Json_data* json_data, Json_extracted_data* extracted_data)
{
	bool is_searching_key = true;
	uint32_t result = DEF_ERR_OTHER;
	uint32_t key_length = 0;
	Json_internal_data* data = NULL;
	Json_internal_requested_key* key_info = NULL;
	Json_internal_list* list = NULL;

	if(!key || !json_data || !extracted_data || !json_data->internal_data)
		goto invalid_arg_0;

	key_length = strlen(key);
	if(key_length == 0)
		goto invalid_arg_0;

	data = (Json_internal_data*)json_data->internal_data;
	if(!data->raw_json || data->raw_json_size == 0 || !data->tokens || data->capacity == 0)
		goto invalid_arg_0;

	free(extracted_data->value);
	extracted_data->size = 0;
	extracted_data->value = NULL;
	extracted_data->type = JSON_TYPE_INVALID;

	key_info = (Json_internal_requested_key*)malloc(sizeof(Json_internal_requested_key));
	if(!key_info)
	{
		DEF_LOG_RESULT(malloc, false, 0);
		goto out_of_memory;
	}

	key_info->key = key;
	key_info->eof = (key + key_length);
	key_info->capacity = 0;
	key_info->used = 0;
	key_info->current_index = 0;
	key_info->state = JSON_INTERNAL_STATE_SEARCHING_KEY;

	key_info->data = (Json_internal_requested_key_data*)calloc(64, sizeof(Json_internal_requested_key_data));
	if(!key_info->data)
	{
		DEF_LOG_RESULT(calloc, false, 0);
		goto out_of_memory;
	}

	key_info->capacity = 64;

	do
	{
		const char* dot_pos = strchr(key_info->key, '.');
		const char* start_of_array_pos = strchr(key_info->key, '[');
		const char* end_of_array_pos = strchr(key_info->key, ']');

		//If there are no ".", "[" nor "]", we reached EOF.
		if(!dot_pos && !start_of_array_pos && !end_of_array_pos)
		{
			uint32_t size = 0;

			if(key_info->eof <= key_info->key)
			{
				DEF_LOG_STRING("EOF content is empty!!!!!");
				goto invalid_arg_1;
			}
			size = (key_info->eof - key_info->key);

			key_info->data[key_info->used].key = (char*)malloc(size + 1);
			if(!key_info->data[key_info->used].key)
			{
				DEF_LOG_RESULT(malloc, false, 0);
				goto out_of_memory;
			}

			memcpy(key_info->data[key_info->used].key, key_info->key, size);
			key_info->data[key_info->used].key[size] = 0x00;//Add null terminator.
			key_info->used++;

			break;
		}
		else
		{
			bool is_array_detected = false;
			bool is_dot_detected = false;

			//If only "[" or "]" exists, it's illegal.
			//e.g object.array[0.data
			//                ^
			//object.array0].data
			//             ^
			if((start_of_array_pos && !end_of_array_pos) || (!start_of_array_pos && end_of_array_pos))
			{
				if(start_of_array_pos)
					DEF_LOG_STRING("\"[\" exists, but \"]\" does NOT!!!!!");
				if(end_of_array_pos)
					DEF_LOG_STRING("\"]\" exists, but \"[\" does NOT!!!!!");

				goto invalid_arg_1;
			}

			if(start_of_array_pos && end_of_array_pos)
				is_array_detected = true;
			if(dot_pos)
				is_dot_detected = true;

			if(is_array_detected)
			{
				//It's also illegal if (start_pos + 1) is greater than or equal to end_pos.
				//e.g object.array[].data
				//                ^^
				if((start_of_array_pos + 1) >= end_of_array_pos)
				{
					DEF_LOG_STRING("\"[\" and \"]\" exist, but its content is empty!!!!!");
					goto invalid_arg_1;
				}

				if(is_dot_detected)
				{
					//Finally, "." can't exist between "[" and "]".
					//e.g object.array[.3].data
					//                 ^
					if(dot_pos < start_of_array_pos && dot_pos > end_of_array_pos)
					{
						DEF_LOG_STRING("\".\" exists between \"[\" and \"]\"!!!!!");
						goto invalid_arg_1;
					}
				}
			}

			//If only dot exists or brackets comes later, process dot first.
			//e.g. object.data
			//           ^
			//object.data[2]
			//      ^    ^ ^
			if((is_dot_detected && !is_array_detected) || (is_dot_detected && is_array_detected && dot_pos < start_of_array_pos))
			{
				uint32_t size = 0;

				if(dot_pos <= key_info->key)
				{
					DEF_LOG_STRING("\".\" exists, but its content is empty!!!!!");
					goto invalid_arg_1;
				}
				size = (dot_pos - key_info->key);

				key_info->data[key_info->used].key = (char*)malloc(size + 1);
				if(!key_info->data[key_info->used].key)
				{
					DEF_LOG_RESULT(malloc, false, 0);
					goto out_of_memory;
				}

				memcpy(key_info->data[key_info->used].key, key_info->key, size);
				key_info->data[key_info->used].key[size] = 0x00;//Add null terminator.
				key_info->used++;

				//Increment index by key_size + dot_size.
				key_info->key += (size + 1);
			}
			//If only brackets exist or dot comes later, process brackets first.
			//e.g. data[2]
			//         ^ ^
			//object[1].data
			//      ^ ^^
			else if((!is_dot_detected && is_array_detected) || (is_dot_detected && is_array_detected && start_of_array_pos < dot_pos))
			{
				uint32_t size = 0;
				char* array_index_temp = NULL;

				//If dot exists, it must be just after "]", if not it's illegal.
				//e.g. object[1]z.data
				//              ^
				if(is_dot_detected && (end_of_array_pos + 1) != dot_pos)
				{
					DEF_LOG_STRING("\".\" exists, but it's NOT just after \"]\"!!!!!");
					goto invalid_arg_1;
				}

				if(start_of_array_pos <= key_info->key)
				{
					DEF_LOG_STRING("\"[\" and \"]\" exist, but its content is empty!!!!!");
					goto invalid_arg_1;
				}
				size = (start_of_array_pos - key_info->key);

				key_info->data[key_info->used].key = (char*)malloc(size + 1);
				if(!key_info->data[key_info->used].key)
				{
					DEF_LOG_RESULT(malloc, false, 0);
					goto out_of_memory;
				}

				memcpy(key_info->data[key_info->used].key, key_info->key, size);
				key_info->data[key_info->used].key[size] = 0x00;//Add null terminator.

				//Increment index by key_size + bracket_size.
				key_info->key += (size + 1);

				size = (end_of_array_pos - key_info->key);

				array_index_temp = (char*)malloc(size + 1);
				if(!array_index_temp)
				{
					DEF_LOG_RESULT(malloc, false, 0);
					goto out_of_memory;
				}

				memcpy(array_index_temp, key_info->key, size);
				array_index_temp[size] = 0x00;//Add null terminator.

				//Check if it's a valid integer.
				for(uint32_t i = 0; i < size; i++)
				{
					if(array_index_temp[i] < '0' || array_index_temp[i] > '9')
					{
						free(array_index_temp);
						array_index_temp = NULL;
						DEF_LOG_STRING("Value between \"[\" and \"]\" is NOT valid!!!!!");
						goto invalid_arg_1;
					}
				}

				key_info->data[key_info->used].array_index = atoi(array_index_temp);
				key_info->data[key_info->used].is_array = true;
				key_info->used++;

				free(array_index_temp);
				array_index_temp = NULL;

				//Increment index by index_size + bracket_size.
				key_info->key += (size + 1);
				if(is_dot_detected)//If dot exists, + dot_size.
					key_info->key++;
			}
		}
	}
	while(key_info->key < key_info->eof);

	list = (Json_internal_list*)malloc(sizeof(Json_internal_list));
	if(!list)
	{
		DEF_LOG_RESULT(malloc, false, 0);
		goto out_of_memory;
	}

	list->data = (Json_internal_list_data*)calloc(64, sizeof(Json_internal_list_data));
	if(!list->data)
	{
		DEF_LOG_RESULT(calloc, false, 0);
		goto out_of_memory;
	}

	list->capacity = 64;
	list->used = 0;

	//Search for the value.
	for(uint32_t i = 1; i < data->capacity; i++)
	{
		if(data->tokens[i].type == JSMN_ARRAY || data->tokens[i].type == JSMN_OBJECT)
		{
			if(list->used + 1 >= 64)
			{
				//todo realloc
				DEF_LOG_STRING("Capacity overflow!!!!!");
				goto out_of_memory;
			}

			list->used++;
			list->data[list->used].is_object = (data->tokens[i].type == JSMN_OBJECT);
			list->data[list->used].size = data->tokens[i].size;
			list->data[list->used].current_index = 0;

			if(data->tokens[i].type == JSMN_OBJECT)
				is_searching_key = true;
		}
		else if(is_searching_key)
		{
			if(data->tokens[i].type == JSMN_STRING)
			{
				uint32_t key_size = data->tokens[i].end - data->tokens[i].start;

				//Check if key matches.
				if(key_size == strlen(key_info->data[key_info->current_index].key)
				&& memcmp((data->raw_json + data->tokens[i].start), key_info->data[key_info->current_index].key, key_size) == 0)
				{
					if((key_info->current_index + 1) == key_info->used)
					{
						if(!key_info->data[key_info->current_index].is_array)
							key_info->state = JSON_INTERNAL_STATE_FOUND;//We found a key.
						else
							key_info->state = JSON_INTERNAL_STATE_SEARCHING_ARRAY_INDEX;//We found a key and need to search for array index.
					}

					if(!key_info->data[key_info->current_index].is_array)
						key_info->current_index++;
				}

				is_searching_key = false;
			}
		}
		else
		{
			if(key_info->data[key_info->current_index].array_index == list->data[list->used].current_index
			&& key_info->state == JSON_INTERNAL_STATE_SEARCHING_ARRAY_INDEX && !list->data[list->used].is_object
			&& list->used > 0)
			{
				//We found a key.
				key_info->state = JSON_INTERNAL_STATE_FOUND;
				key_info->current_index++;
			}

			if(key_info->state == JSON_INTERNAL_STATE_FOUND && key_info->current_index >= key_info->used)
				goto found;

			if(list->used == 0)
				is_searching_key = true;
			else
			{
				list->data[list->used].current_index++;

				if(list->data[list->used].current_index >= list->data[list->used].size)
				{
					list->used--;

					if(key_info->current_index > list->used)
					{
						//We've finished the nest and the key that is the same name can't exist,
						//so there will be no hope finding a requested key.
						goto not_found;
					}

					is_searching_key = true;
				}

				if(list->data[list->used].is_object)
					is_searching_key = true;
			}
		}

		continue;

		found:
		//We found the all keys, get the value.
		if(data->tokens[i].type == JSMN_STRING)//Data type: string.
		{
			uint32_t escaped_size = (data->tokens[i].end - data->tokens[i].start);
			char* escaped_text = (char*)malloc(escaped_size + 1);
			Str_data unescaped_text = { 0, };

			if(!escaped_text)
			{
				DEF_LOG_RESULT(malloc, false, 0);
				goto out_of_memory;
			}

			//Get the actual value.
			memcpy(escaped_text, (data->raw_json + data->tokens[i].start), escaped_size);
			escaped_text[escaped_size] = 0x00;//Add a null terminator.

			//Unescape it.
			result = Util_json_unescape(escaped_text, &unescaped_text);
			if(result != DEF_SUCCESS)
			{
				Util_str_free(&unescaped_text);
				DEF_LOG_RESULT(Util_json_unescape, false, result);
				goto error;
			}
			free(escaped_text);
			escaped_text = NULL;

			extracted_data->value = (void*)malloc(unescaped_text.length + 1);
			if(!extracted_data->value)
			{
				Util_str_free(&unescaped_text);
				DEF_LOG_RESULT(malloc, false, 0);
				goto out_of_memory;
			}

			extracted_data->type = JSON_TYPE_STRING;
			extracted_data->size = unescaped_text.length;
			memcpy(extracted_data->value, unescaped_text.buffer, extracted_data->size);
			((char*)extracted_data->value)[extracted_data->size] = 0x00;//Add a null terminator.
			Util_str_free(&unescaped_text);
		}
		else if(data->tokens[i].type == JSMN_PRIMITIVE)
		{
			char type = *((char*)data->raw_json + data->tokens[i].start);

			if(type == 't' || type == 'f')//Data type: bool.
			{
				extracted_data->type = JSON_TYPE_BOOL;
				extracted_data->size = sizeof(bool);
			}
			else if(type == 'n')//Data type: null.
			{
				extracted_data->type = JSON_TYPE_NULL;
				extracted_data->size = 0;
			}
			else//Data type: number.
			{
				extracted_data->type = JSON_TYPE_NUMBER;
				extracted_data->size = sizeof(double);
			}

			if(extracted_data->type == JSON_TYPE_BOOL || extracted_data->type == JSON_TYPE_NUMBER)
			{
				extracted_data->value = (void*)malloc(extracted_data->size);
				if(!extracted_data->value)
				{
					DEF_LOG_RESULT(malloc, false, 0);
					goto out_of_memory;
				}

				//Get the actual value.
				if(extracted_data->type == JSON_TYPE_BOOL)
				{
					if(type == 't')
						*((bool*)extracted_data->value) = true;
					else
						*((bool*)extracted_data->value) = false;
				}
				else if(extracted_data->type == JSON_TYPE_NUMBER)
				{
					char* value_cache = NULL;
					uint32_t value_size = (data->tokens[i].end - data->tokens[i].start);

					value_cache = (char*)malloc(value_size + 1);
					if(!value_cache)
					{
						DEF_LOG_RESULT(malloc, false, 0);
						goto out_of_memory;
					}

					memcpy(value_cache, (data->raw_json + data->tokens[i].start), value_size);
					value_cache[value_size] = 0x00;//Null terminator.

					*((double*)extracted_data->value) = atof(value_cache);

					free(value_cache);
					value_cache = NULL;
				}
			}
		}

		break;
	}

	//Fallthrough.
	not_found:

	if(key_info)
	{
		if(key_info->data)
		{
			for(uint32_t i = 0; i < key_info->capacity; i++)
			{
				free(key_info->data[i].key);
				key_info->data[i].key = NULL;
			}
			free(key_info->data);
			key_info->data = NULL;
		}
		free(key_info);
		key_info = NULL;
	}

	if(list)
	{
		free(list->data);
		list->data = NULL;
		free(list);
		list = NULL;
	}

	return DEF_SUCCESS;

	invalid_arg_0:
	return DEF_ERR_INVALID_ARG;

	invalid_arg_1:
	result = DEF_ERR_INVALID_ARG;
	goto free_ram;

	out_of_memory:
	result = DEF_ERR_OUT_OF_MEMORY;
	goto free_ram;

	error:
	goto free_ram;

	free_ram:
	if(extracted_data)
	{
		free(extracted_data->value);
		extracted_data->size = 0;
		extracted_data->value = NULL;
		extracted_data->type = JSON_TYPE_INVALID;
	}

	if(key_info)
	{
		if(key_info->data)
		{
			for(uint32_t i = 0; i < key_info->capacity; i++)
			{
				free(key_info->data[i].key);
				key_info->data[i].key = NULL;
			}
			free(key_info->data);
			key_info->data = NULL;
		}
		free(key_info);
		key_info = NULL;
	}

	if(list)
	{
		free(list->data);
		list->data = NULL;
		free(list);
		list = NULL;
	}

	return result;
}

void Util_json_free(Json_data* parsed_json_data)
{
	Json_internal_data* data = NULL;

	if(!parsed_json_data)
		return;

	data = (Json_internal_data*)(parsed_json_data->internal_data);
	if(data)
	{
		free(data->raw_json);
		free(data->tokens);
		data->raw_json = NULL;
		data->tokens = NULL;
	}
	free(parsed_json_data->internal_data);
	parsed_json_data->internal_data = NULL;
}

static uint32_t Util_json_unescape(const char* escaped_string, Str_data* unescaped_string)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t string_index = 0;
	uint32_t escaped_string_length = strlen(escaped_string);
	Json_internal_unescaping_search_state state = JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_BACKSLASH;

	result = Util_str_init(unescaped_string);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	while ((escaped_string_length - string_index) > 0)
	{
		int32_t char_length = mblen(&(escaped_string[string_index]), (escaped_string_length - string_index));
		if(char_length == 1)
		{
			switch (state)
			{
				case JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_BACKSLASH:
				{
					if(escaped_string[string_index] == '\\')
					{
						//We found a start of escaping character.
						//e.g. "data 0 \n data 1"
						//             ^
						//"data 0 \t data 1"
						//        ^
						//"We love \u2622 so much."
						//         ^
						state = JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_ESCAPED_CHAR;
					}
					else
					{
						char temp_char[2] = { escaped_string[string_index], };
						Util_str_add(unescaped_string, temp_char);//Normal character.
					}

					break;
				}

				case JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_ESCAPED_CHAR:
				{
					if(escaped_string[string_index] == 'u')
					{
						//We found a unicode escaping character.
						//"We love \u2622 so much."
						//          ^
						state = JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_4_HEX_DIGITS;
						break;
					}

					for(uint32_t i = 0; i < DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(json_escape_char_list); i++)
					{
						if(escaped_string[string_index] == json_escape_char_list[i].escaped_chars)
						{
							char temp_char[2] = { json_escape_char_list[i].unescaped_chars, };
							//We found a escaping character.
							//e.g. "data 0 \n data 1"
							//              ^
							//"data 0 \t data 1"
							//         ^
							Util_str_add(unescaped_string, temp_char);
							break;
						}
					}

					state = JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_BACKSLASH;

					break;
				}

				case JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_4_HEX_DIGITS:
				{
					if((escaped_string_length - string_index) >= 4)
					{
						char escaped_unicode_temp[5] = { 0, };
						char out[5] = { 0, };
						uint32_t in = 0;

						memcpy(escaped_unicode_temp, &(escaped_string[string_index]), 4);
						sscanf(escaped_unicode_temp, "%lX", &in);

						//Convert escaped unicode char to actual unicode char.
						if(encode_utf8((uint8_t*)out, in) > 0)
							Util_str_add(unescaped_string, out);
					}

					string_index += 3;
					state = JSON_INTERNAL_UNESCAPE_STATE_SEARCHING_BACKSLASH;

					break;
				}

				default:
					break;
			}

			string_index += char_length;
		}
		else if(char_length > 0)
		{
			char temp_char[5] = { 0, };

			if(char_length > 4)
				char_length = 4;

			memcpy(temp_char, &(escaped_string[string_index]), char_length);
			Util_str_add(unescaped_string, temp_char);
			string_index += char_length;
		}
		else
			string_index++;
	}

	return DEF_SUCCESS;

	error:
	return result;
}
#endif //DEF_CURL_API_ENABLE
