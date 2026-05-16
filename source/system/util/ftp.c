//Includes.
#include "system/util/ftp.h"

#if DEF_FTP_API_ENABLE
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "3ds.h"

#include "system/util/err_types.h"
#include "system/util/file.h"
#include "system/util/log.h"
#include "system/util/str.h"
#include "system/util/util.h"

//Defines.
#define IO_BUFFER_SIZE		(uint32_t)(1000 * 250)
//Empty.

//Typedefs.
typedef struct
{
	const char* name;
	bool has_arg;
	Ftp_command id;
} Command;

//Prototypes.
static bool Ftp_case_insensitive_compare(const char* a, const char* b, uint32_t size);
static uint32_t Ftp_greet(Ftp_client* client);
static void Ftp_process_stor_stou_appe(Ftp_client* client, bool is_append, bool is_unique);
static void Ftp_process_dele_rmd(Ftp_client* client, bool is_file);
static void Ftp_process_list_nlst(Ftp_client* client, bool is_list);
static void Ftp_process_list_nlst_bof(Ftp_client* client);
static void Ftp_process_stor_stou_appe_data(Ftp_client* client);
static void Ftp_process_stor_stou_appe_eof(Ftp_client* client);
static bool Ftp_is_loopback_ip(uint32_t address);
static bool Ftp_is_local_ip(uint32_t address);
static int32_t Ftp_listen(uint16_t port);
static int32_t Ftp_connect(uint16_t port, uint32_t address);
static void Ftp_close_socket_internal(int32_t socket_handle, bool is_force);
static uint32_t Ftp_client_init(Ftp_client* client, uint16_t control_port, uint16_t data_port, uint32_t local_address,
uint32_t global_address, uint32_t store_buffer_size, uint32_t dir_content_max_files);
static uint32_t Ftp_current_dir_init(Ftp_current_dir* current_dir);
static uint32_t Ftp_list_init(Ftp_list* list);
static uint32_t Ftp_rename_init(Ftp_rename* rename);
static uint32_t Ftp_retrieve_init(Ftp_retrieve* retrieve);
static uint32_t Ftp_store_init(Ftp_store* store, uint32_t buffer_size);
static uint32_t Ftp_dir_content_init(Ftp_dir_content* dir_content, uint32_t max_files);
static void Ftp_client_exit(Ftp_client* client);
static void Ftp_current_dir_exit(Ftp_current_dir* current_dir);
static void Ftp_list_exit(Ftp_list* list);
static void Ftp_rename_exit(Ftp_rename* rename);
static void Ftp_retrieve_exit(Ftp_retrieve* retrieve);
static void Ftp_store_exit(Ftp_store* store);
static void Ftp_dir_content_exit(Ftp_dir_content* dir_content);

//Variables.
static const char util_ftp_responses[FTP_RESPONSE_MAX][5] =
{
	"200 ", "500 ", "501 ", "202 ", "502 ", "503 ", "504 ", "110 ",
	"211 ", "212 ", "213 ", "214 ", "215 ", "120 ", "220 ", "221 ",
	"421 ", "125 ", "150 ", "225 ", "425 ", "226 ", "426 ", "227 ",
	"230 ", "530 ", "331 ", "332 ", "532 ", "250 ", "257 ", "350 ",
	"450 ", "550 ", "451 ", "551 ", "452 ", "552 ", "553 ",
};
static const Command util_ftp_commands[] =
{
	{ .name = "USER",   .has_arg = true,  .id = FTP_COMMAND_USER, },	//"USER<SP><username><CRLF>".
	{ .name = "PASS",   .has_arg = true,  .id = FTP_COMMAND_PASS, },	//"PASS<SP><password><CRLF>".
	{ .name = "ACCT",   .has_arg = true,  .id = FTP_COMMAND_ACCT, },	//"ACCT<SP><account-information><CRLF>".
	{ .name = "CWD",    .has_arg = true,  .id = FTP_COMMAND_CWD,  },	//"CWD<SP><pathname><CRLF>".
	{ .name = "CDUP",   .has_arg = false, .id = FTP_COMMAND_CDUP, },	//"CDUP<CRLF>".
	{ .name = "SMNT",   .has_arg = true,  .id = FTP_COMMAND_SMNT, },	//"SMNT<SP><pathname><CRLF>".
	{ .name = "REIN",   .has_arg = false, .id = FTP_COMMAND_REIN, },	//"REIN<CRLF>".
	{ .name = "QUIT",   .has_arg = false, .id = FTP_COMMAND_QUIT, },	//"QUIT<CRLF>".
	{ .name = "PORT",   .has_arg = true,  .id = FTP_COMMAND_PORT, },	//"PORT<SP><host-port><CRLF>".
	{ .name = "PASV",   .has_arg = false, .id = FTP_COMMAND_PASV, },	//"PASV<CRLF>".
	{ .name = "TYPE",   .has_arg = true,  .id = FTP_COMMAND_TYPE, },	//"TYPE<SP><type-code><CRLF>".
	{ .name = "STRU",   .has_arg = true,  .id = FTP_COMMAND_STRU, },	//"STRU<SP><structure-code><CRLF>".
	{ .name = "MODE",   .has_arg = true,  .id = FTP_COMMAND_MODE, },	//"MODE<SP><mode-code><CRLF>".
	{ .name = "RETR",   .has_arg = true,  .id = FTP_COMMAND_RETR, },	//"RETR<SP><pathname><CRLF>".
	{ .name = "STOR",   .has_arg = true,  .id = FTP_COMMAND_STOR, },	//"STOR<SP><pathname><CRLF>".
	{ .name = "STOU",   .has_arg = false, .id = FTP_COMMAND_STOU, },	//"STOU<CRLF>".
	{ .name = "APPE",   .has_arg = true,  .id = FTP_COMMAND_APPE, },	//"APPE<SP><pathname><CRLF>".
	{ .name = "ALLO R", .has_arg = true,  .id = FTP_COMMAND_ALLO, },	//"ALLO<SP>R<SP><decimal-integer><CRLF>".
	{ .name = "ALLO",   .has_arg = true,  .id = FTP_COMMAND_ALLO, },	//"ALLO<SP><decimal-integer><CRLF>".
	{ .name = "REST",   .has_arg = true,  .id = FTP_COMMAND_REST, },	//"REST<SP><marker><CRLF>".
	{ .name = "RNFR",   .has_arg = true,  .id = FTP_COMMAND_RNFR, },	//"RNFR<SP><pathname><CRLF>".
	{ .name = "RNTO",   .has_arg = true,  .id = FTP_COMMAND_RNTO, },	//"RNTO<SP><pathname><CRLF>".
	{ .name = "ABOR",   .has_arg = false, .id = FTP_COMMAND_ABOR, },	//"ABOR<CRLF>".
	{ .name = "DELE",   .has_arg = true,  .id = FTP_COMMAND_DELE, },	//"DELE<SP><pathname><CRLF>".
	{ .name = "RMD",    .has_arg = true,  .id = FTP_COMMAND_RMD,  },	//"RMD<SP><pathname><CRLF>".
	{ .name = "MKD",    .has_arg = true,  .id = FTP_COMMAND_MKD,  },	//"MKD<SP><pathname><CRLF>".
	{ .name = "PWD",    .has_arg = false, .id = FTP_COMMAND_PWD,  },	//"PWD<CRLF>".
	{ .name = "LIST",   .has_arg = true,  .id = FTP_COMMAND_LIST, },	//"LIST<SP><pathname><CRLF>".
	{ .name = "LIST",   .has_arg = false, .id = FTP_COMMAND_LIST, },	//"LIST<CRLF>".
	{ .name = "NLST",   .has_arg = true,  .id = FTP_COMMAND_NLST, },	//"NLST<SP><pathname><CRLF>".
	{ .name = "NLST",   .has_arg = false, .id = FTP_COMMAND_NLST, },	//"NLST<CRLF>".
	{ .name = "SITE",   .has_arg = true,  .id = FTP_COMMAND_SITE, },	//"SITE<SP><string><CRLF>".
	{ .name = "SYST",   .has_arg = false, .id = FTP_COMMAND_SYST, },	//"SYST<CRLF>".
	{ .name = "STAT",   .has_arg = true,  .id = FTP_COMMAND_STAT, },	//"STAT<SP><pathname><CRLF>".
	{ .name = "STAT",   .has_arg = false, .id = FTP_COMMAND_STAT, },	//"STAT<CRLF>".
	{ .name = "HELP",   .has_arg = true,  .id = FTP_COMMAND_HELP, },	//"HELP<SP><string><CRLF>".
	{ .name = "HELP",   .has_arg = false, .id = FTP_COMMAND_HELP, },	//"HELP<CRLF>".
	{ .name = "NOOP",   .has_arg = false, .id = FTP_COMMAND_NOOP, },	//"NOOP<CRLF>".
};
static const char util_ftp_crlf[3] = "\r\n";

//Code.
uint32_t Ftp_receive_command(int32_t socket_handle, Ftp_command* command, char* arg, uint32_t arg_size)
{
	uint32_t received_offset = 0;
	char* eof = NULL;

	if(socket_handle < 0 || !command || !arg || arg_size <= 1)
		goto invalid_arg;

	*command = FTP_COMMAND_INVALID;

	while(!eof && (arg_size - received_offset - 1) > 0)
	{
		ssize_t received = 0;

		errno = 0;
		//Reserve 1 byte for NULL terminator.
		received = recv(socket_handle, (arg + received_offset), (arg_size - received_offset - 1), 0);
		if(received <= 0)
		{
			DEF_LOG_RESULT(recv, false, received);
			DEF_LOG_UINT(errno);
			goto socket_failed;
		}

		received_offset += received;
		arg[received_offset] = 0x00;//NULL terminator.
		eof = strstr(arg, util_ftp_crlf);
	}

	if(!eof)
	{
		DEF_LOG_STRING("Buffer is too small!!!!!");
		goto out_of_memory;
	}

	for(uint32_t i = 0; i < DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(util_ftp_commands); i++)
	{
		uint32_t buffer_offset = 0;
		uint32_t name_length = Util_min((received_offset - buffer_offset), strlen(util_ftp_commands[i].name));

		//Check for name.
		//RFC959: Upper and lower case alphabetic characters are to be treated identically.
		if(Ftp_case_insensitive_compare((arg + buffer_offset), util_ftp_commands[i].name, name_length))
		{
			buffer_offset += name_length;

			//Check for arg.
			if(util_ftp_commands[i].has_arg)
			{
				uint32_t arg_length = 0;

				//Minimal valid arg: " \r\n".
				if((received_offset - buffer_offset) < 3 || arg[buffer_offset] != ' ')
					continue;//Buffer too short or malformed command arg, check for different commands.

				//Keep only arg.
				buffer_offset++;
				arg_length = (eof - &arg[buffer_offset]);
				memmove(arg, &arg[buffer_offset], arg_length);
				arg[arg_length] = 0x00;//NULL terminator.
			}
			else
			{
				if((eof - &arg[buffer_offset]) == 0)
					arg[0] = 0x00;//NULL terminator.
				else
					continue;//Malformed command, check for different commands.
			}

			*command = util_ftp_commands[i].id;
			break;
		}
	}

	//If command was unknown, just remove \r\n.
	if(*command == FTP_COMMAND_INVALID)
	{
		uint32_t eof_index = (eof - arg);
		arg[eof_index] = 0x00;//NULL terminator.
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Ftp_send_response(int32_t socket_handle, Ftp_response response, const char* message)
{
	ssize_t length = 0;
	ssize_t sent = 0;

	if(socket_handle < 0 || response <= FTP_RESPONSE_INVALID || response >= FTP_RESPONSE_MAX || !message)
		goto invalid_arg;

	errno = 0;
	//Send the response code + <SP>.
	length = strlen(util_ftp_responses[response]);
	sent = send(socket_handle, util_ftp_responses[response], length, 0);
	if(sent != length)
	{
		DEF_LOG_FORMAT("Response \"%s\" couldn't be sent!!!!!", util_ftp_responses[response]);
		DEF_LOG_RESULT(send, false, sent);
		DEF_LOG_UINT(errno);
		goto socket_failed;
	}

	//Send the message.
	length = strlen(message);
	if(length > 0)
	{
		errno = 0;
		sent = send(socket_handle, message, length, 0);
		if(sent != length)
		{
			DEF_LOG_FORMAT("Message \"%s\" couldn't be sent!!!!!", message);
			DEF_LOG_RESULT(send, false, sent);
			DEF_LOG_UINT(errno);
			goto socket_failed;
		}
	}

	errno = 0;
	//Send the <CRLF>.
	length = strlen(util_ftp_crlf);
	sent = send(socket_handle, util_ftp_crlf, length, 0);
	if(sent != length)
	{
		DEF_LOG_STRING("<CRLF> couldn't be sent!!!!!");
		DEF_LOG_RESULT(send, false, sent);
		DEF_LOG_UINT(errno);
		goto socket_failed;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;
}

uint32_t Ftp_receive_data(int32_t socket_handle, uint8_t* buffer, uint32_t buffer_size, uint32_t* received_size)
{
	ssize_t received = 0;

	if(socket_handle < 0 || !buffer || buffer_size == 0 || !received_size)
		goto invalid_arg;

	errno = 0;
	//Receive the data.
	received = recv(socket_handle, buffer, buffer_size, 0);
	if(received < 0)
	{
		DEF_LOG_RESULT(recv, false, received);
		DEF_LOG_UINT(errno);
		goto socket_failed;
	}

	*received_size = received;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;
}

uint32_t Ftp_send_data(int32_t socket_handle, const uint8_t* buffer, uint32_t size)
{
	ssize_t length = size;
	ssize_t sent = 0;

	if(socket_handle < 0 || !buffer || size == 0)
		goto invalid_arg;

	errno = 0;
	//Send the data.
	sent = send(socket_handle, buffer, length, 0);
	if(sent != length)
	{
		DEF_LOG_RESULT(send, false, sent);
		DEF_LOG_UINT(errno);
		goto socket_failed;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;
}

uint32_t Ftp_get_path(const char* current_dir, const char* cmd_path, Str_data* dir, Str_data* file)
{
	const char parent_dir[2] = "..";
	char* last_slash_pos = NULL;
	uint32_t length = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(!current_dir || !cmd_path || !dir)
		goto invalid_arg;

	length = strlen(cmd_path);
	if(length == 0)
		goto invalid_arg;

	result = Util_str_init(dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	if(file)
	{
		result = Util_str_init(file);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto error;
		}
	}

	result = Util_str_set(dir, current_dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	if(length == sizeof(parent_dir) && memcmp(cmd_path, parent_dir, sizeof(parent_dir)) == 0)
	{
		//Back to parent directory.
		bool is_root = (dir->length == 1 && dir->buffer[0] == '/');

		if(!is_root)
		{
			last_slash_pos = strrchr(dir->buffer, '/');
			if(last_slash_pos)
			{
				//Remove last slash first.
				uint32_t new_length = (last_slash_pos - dir->buffer);

				result = Util_str_resize(dir, new_length);
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_str_resize, false, result);
					goto error;
				}

				last_slash_pos = strrchr(dir->buffer, '/');
				if(last_slash_pos)
				{
					//Then remove until next slash.
					new_length = ((last_slash_pos - dir->buffer) + 1);
					result = Util_str_resize(dir, new_length);
					if(result != DEF_SUCCESS)
					{
						DEF_LOG_RESULT(Util_str_resize, false, result);
						goto error;
					}
				}
				else
					is_root = true;//Fallback to root directory.
			}
			else
				is_root = true;//Fallback to root directory.
		}

		if(is_root)
		{
			//We are on root.
			result = Util_str_set(dir, "/");
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_set, false, result);
				goto error;
			}
		}
	}
	else if(cmd_path[0] == '/')
	{
		//Absolute path.
		result = Util_str_set(dir, cmd_path);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_set, false, result);
			goto error;
		}
	}
	else
	{
		//Relative path.
		result = Util_str_add(dir, cmd_path);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_add, false, result);
			goto error;
		}
	}

	if(file)
	{
		//Split filename (if exists).
		last_slash_pos = strrchr(dir->buffer, '/');
		if(last_slash_pos && last_slash_pos > dir->buffer)
		{
			uint32_t new_length = ((last_slash_pos - dir->buffer) + 1);

			//Split filename.
			result = Util_str_set(file, (last_slash_pos + 1));
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_set, false, result);
				goto error;
			}

			//Remove filename from dir.
			result = Util_str_resize(dir, new_length);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_resize, false, result);
				goto error;
			}
		}
	}

	//Make sure to include slash at end of directory path.
	if(dir->length > 0 && dir->buffer[dir->length - 1] != '/')
	{
		result = Util_str_add(dir, "/");
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_add, false, result);
			goto error;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error:
	return result;
}

uint32_t Ftp_get_unique_filename(const char* dir, const char* filename_base, Str_data* filename)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!dir || !filename_base || !filename)
		goto invalid_arg;

	result = Util_str_init(filename);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_set(filename, filename_base);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	while(Util_file_check_file_exist(filename->buffer, dir) == DEF_SUCCESS)
	{
		char* last_dot_pos = strrchr(filename->buffer, '.');

		if(last_dot_pos)
		{
			uint32_t new_length = (last_dot_pos - filename->buffer);
			char ext[16] = { 0, };

			//Save extension, remove extension then add "_" with saved extension.
			snprintf(ext, sizeof(ext), "%s", last_dot_pos);

			result = Util_str_resize(filename, new_length);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_resize, false, result);
				goto error;
			}

			result = Util_str_format_append(filename, "_%s", ext);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_format_append, false, result);
				goto error;
			}
		}
		else//No extension.
		{
			result = Util_str_add(filename, "_");
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_add, false, result);
				goto error;
			}
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	error:
	return result;
}

uint32_t Ftp_accept_control_connection(Ftp_clients* clients)
{
	bool is_full = true;
	int32_t socket_handle = 0;
	uint32_t result = DEF_ERR_OTHER;
	struct sockaddr_in address = { 0, };
	socklen_t address_length = sizeof(struct sockaddr_in);

	if(!clients)
		goto invalid_arg;

	errno = 0;
	socket_handle = accept(clients->control_listen_handle.fd, (struct sockaddr*)&address, &address_length);
	if(socket_handle == -1)
	{
		DEF_LOG_RESULT(accept, false, socket_handle);
		DEF_LOG_UINT(errno);
		goto socket_failed;
	}

	for(uint8_t i = 0; i < clients->max_clients; i++)
	{
		if(clients->clients[i].control_handle.fd < 0)
		{
			//Register it.
			clients->clients[i].control_handle.fd = socket_handle;
			clients->clients[i].control_address = address;
			clients->clients[i].data_address_active = ntohl(address.sin_addr.s_addr);
			clients->clients[i].data_port_active = ntohs(address.sin_port);

			//Then greet.
			result = Ftp_greet(&clients->clients[i]);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Ftp_greet, false, result);
				goto error;
			}

			is_full = false;
			break;
		}
	}

	if(is_full)
	{
		DEF_LOG_STRING("Control connection is full!!!!!");
		Ftp_close_socket_internal(socket_handle, true);
		goto out_of_memory;
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;

	error:
	return result;

	out_of_memory:
	return DEF_ERR_OUT_OF_MEMORY;
}

uint32_t Ftp_accept_data_connection(Ftp_client* client)
{
	int32_t socket_handle = 0;
	struct sockaddr_in address = { 0, };
	socklen_t address_length = sizeof(struct sockaddr_in);

	if(!client)
		goto invalid_arg;

	errno = 0;
	socket_handle = accept(client->data_listen_handle.fd, (struct sockaddr*)&address, &address_length);
	if(socket_handle == -1)
	{
		DEF_LOG_RESULT(accept, false, socket_handle);
		DEF_LOG_UINT(errno);
		goto socket_failed;
	}

	//Stop listening.
	Ftp_close_socket_internal(client->data_listen_handle.fd, false);
	client->data_listen_handle.fd = -1;

	if(client->data_handle.fd >= 0)
	{
		DEF_LOG_STRING("Data connection already exists!!!!!");
		Ftp_close_socket_internal(socket_handle, true);
		goto socket_failed;
	}

	//Register it.
	client->data_handle.fd = socket_handle;
	client->data_address = address;
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;
}

uint32_t Ftp_listen_data_connection(Ftp_client* client)
{
	if(!client)
		goto invalid_arg;

	if(client->data_listen_handle.fd < 0 && client->data_handle.fd < 0)
	{
		client->data_listen_handle.fd = Ftp_listen(client->data_port_passive);
		if(client->data_listen_handle.fd < 0)
		{
			DEF_LOG_RESULT(Ftp_listen, false, client->data_listen_handle.fd);
			goto socket_failed;
		}
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;
}

uint32_t Ftp_connect_data_connection(Ftp_client* client)
{
	if(!client)
		goto invalid_arg;

	if(client->data_listen_handle.fd < 0 && client->data_handle.fd < 0)
	{
		client->data_handle.fd = Ftp_connect(client->data_port_active, client->data_address_active);
		if(client->data_handle.fd < 0)
		{
			DEF_LOG_RESULT(Ftp_connect, false, client->data_handle.fd);
			goto socket_failed;
		}

		client->data_address.sin_family = AF_INET;
		client->data_address.sin_port = htons(client->data_port_active);
		client->data_address.sin_addr.s_addr = htonl(client->data_address_active);
	}

	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	socket_failed:
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;
}

void Ftp_process_user(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "Logged in.");
	client->response = FTP_RESPONSE_ACCOUNT_LOGIN;
}

void Ftp_process_pass(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "We don't require a password.");
	client->response = FTP_RESPONSE_COMMAND_UNNECESSARY;
}

void Ftp_process_acct(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "We don't require an account.");
	client->response = FTP_RESPONSE_COMMAND_UNNECESSARY;
}

void Ftp_process_cwd(Ftp_client* client)
{
	uint32_t length = 0;
	uint32_t result = DEF_ERR_OTHER;
	Str_data temp_dir = { 0, };

	if(!client)
		return;

	length = strlen(client->arg_buffer);
	if(length == 0)
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must NOT be empty!!!!!");
		client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
	}
	else
	{
		result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &temp_dir, NULL);
		if(result == DEF_SUCCESS)
		{
			//Check if it actually exists.
			result = Util_file_check_directory_exist(temp_dir.buffer);
			if(result == DEF_SUCCESS)
			{
				//Update current directory.
				result = Util_str_set(&client->current_dir.dir, temp_dir.buffer);
				if(result == DEF_SUCCESS)
				{
					snprintf(client->message_buffer, sizeof(client->message_buffer), "Good news, specified directory actually exists.");
					client->response = FTP_RESPONSE_FILE_ACTION_OK;
				}
				else
				{
					DEF_LOG_RESULT(Ftp_get_path, false, result);
					snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
					client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
				}
			}
			else
			{
				DEF_LOG_FORMAT("Specified directory \"%s\" does NOT exist!!!!!", temp_dir.buffer);
				DEF_LOG_RESULT(Util_file_check_directory_exist, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Specified directory does NOT exist!!!!!");
				client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;
			}
		}
		else
		{
			DEF_LOG_RESULT(Ftp_get_path, false, result);
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
		}
	}

	Util_str_free(&temp_dir);
}

void Ftp_process_cdup(Ftp_client* client)
{
	if(!client)
		return;

	//RFC959: The reply codes shall be identical to the reply codes of CWD.
	snprintf(client->arg_buffer, sizeof(client->arg_buffer), "..");//Simulate CWD command with the arg "..".
	Ftp_process_cwd(client);
}

void Ftp_process_smnt(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "Unsupported!!!!!");
	client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED;
}

void Ftp_process_rein(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "Unsupported!!!!!");
	client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED;
}

void Ftp_process_quit(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "See you.");
	client->response = FTP_RESPONSE_STATUS_CONTROL_CONNECTION_CLOSING;
}

void Ftp_process_port(Ftp_client* client)
{
	const char* arg = NULL;
	uint8_t ip_u8[4] = { 0, };
	uint8_t port_u8[2] = { 0, };

	if(!client)
		return;

	arg = client->arg_buffer;
	for(uint8_t i = 0; i < DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(ip_u8); i++)
	{
		uint32_t size = 0;
		int32_t temp_value = 0;
		char temp_text[4] = { 0, };
		char* comma_pos = strchr(arg, ',');

		if(!comma_pos)
			goto parse_error;

		size = (comma_pos - arg);
		if(size == 0 || size > 3)
			goto parse_error;//Invalid size.

		memcpy(temp_text, arg, size);
		temp_value = strtol(temp_text, NULL, 10);
		if(temp_value > 255)
			goto parse_error;//Invalid value.

		ip_u8[i] = temp_value;
		//Update offset.
		arg += (size + 1);//Size of number + size of comma.
	}

	for(uint8_t i = 0; i < DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(port_u8); i++)
	{
		uint32_t size = 0;
		int32_t temp_value = 0;
		char temp_text[4] = { 0, };
		char* comma_pos = NULL;

		if((i + 1) == DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(port_u8))
			comma_pos = strchr(arg, 0x00);
		else
			comma_pos = strchr(arg, ',');

		if(!comma_pos)
			goto parse_error;

		size = (comma_pos - arg);
		if(size == 0 || size > 3)
			goto parse_error;//Invalid size.

		memcpy(temp_text, arg, size);
		temp_value = strtol(temp_text, NULL, 10);
		if(temp_value > 255)
			goto parse_error;//Invalid value.

		port_u8[i] = temp_value;
		//Update offset.
		arg += (size + 1);//Size of number + size of comma.
	}

	client->data_address_active = ((ip_u8[0] << 24) | (ip_u8[1] << 16) | (ip_u8[2] << 8) | ip_u8[3]);
	client->data_port_active = ((port_u8[0] << 8) | port_u8[1]);

	snprintf(client->message_buffer, sizeof(client->message_buffer), "Sure.");
	client->response = FTP_RESPONSE_COMMAND_OK;
	return;

	parse_error:
	snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must contain IPv4 address and port!!!!!");
	client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
}

void Ftp_process_pasv(Ftp_client* client)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!client)
		return;

	if(client->data_listen_handle.fd < 0 && client->data_handle.fd < 0)
	{
		uint32_t ip = 0;

		if(Ftp_is_loopback_ip(ntohl(client->control_address.sin_addr.s_addr)))
			ip = (127 | (0 << 16) | (0 << 16) | (1 << 24));
		else if(Ftp_is_local_ip(ntohl(client->control_address.sin_addr.s_addr)))
			ip = client->my_local_address_passive;
		else
			ip = client->my_global_address_passive;

		result = Ftp_listen_data_connection(client);
		if(result == DEF_SUCCESS)
		{
			uint8_t port_u8[2] = { ((client->data_port_passive) >> 8), client->data_port_passive, };
			uint8_t ip_u8[4] = { ip, (ip >> 8), (ip >> 16), (ip >> 24), };

			snprintf(client->message_buffer, sizeof(client->message_buffer), "Passive mode has been enabled. (%" PRIu8 ",%" PRIu8 ",%" PRIu8
			",%" PRIu8 ",%" PRIu8 ",%" PRIu8 ")", ip_u8[0], ip_u8[1], ip_u8[2], ip_u8[3], port_u8[0], port_u8[1]);
			client->response = FTP_RESPONSE_PASSIVE_MODE;
		}
		else
		{
			DEF_LOG_RESULT(Ftp_listen_data_connection, false, result);
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
		}
	}
	else
	{
		DEF_LOG_STRING("Passive mode is already active or data connection exists!!!!!");
		DEF_LOG_UINT(client->data_listen_handle.fd);
		DEF_LOG_UINT(client->data_handle.fd);
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Passive mode is already active or data connection exists!!!!!");
		client->response = FTP_RESPONSE_COMMAND_MALFORMED;
	}
}

void Ftp_process_type(Ftp_client* client)
{
	uint32_t arg_size = 0;

	if(!client)
		return;

	arg_size = strlen(client->arg_buffer);
	//RFC959: Upper and lower case alphabetic characters are to be treated identically.
	//This also applies to any symbols representing parameter values, such as A or a for ASCII TYPE.
	if(arg_size == 1 && (client->arg_buffer[0] == 'A' || client->arg_buffer[0] == 'a'
	|| client->arg_buffer[0] == 'I' || client->arg_buffer[0] == 'i'))
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Sure.");
		client->response = FTP_RESPONSE_COMMAND_OK;
	}
	else
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "We don't support that type!!!!!");
		client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED_FOR_GIVEN_ARG;
	}
}

void Ftp_process_stru(Ftp_client* client)
{
	uint32_t arg_size = 0;

	if(!client)
		return;

	arg_size = strlen(client->arg_buffer);
	//RFC959: Upper and lower case alphabetic characters are to be treated identically.
	//This also applies to any symbols representing parameter values, such as A or a for ASCII TYPE.
	if(arg_size == 1 && (client->arg_buffer[0] == 'F' || client->arg_buffer[0] == 'f'
	|| client->arg_buffer[0] == 'R' || client->arg_buffer[0] == 'r'))
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Sure.");
		client->response = FTP_RESPONSE_COMMAND_OK;
	}
	else
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "We don't support that type!!!!!");
		client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED_FOR_GIVEN_ARG;
	}
}

void Ftp_process_mode(Ftp_client* client)
{
	uint32_t arg_size = 0;

	if(!client)
		return;

	arg_size = strlen(client->arg_buffer);
	//RFC959: Upper and lower case alphabetic characters are to be treated identically.
	//This also applies to any symbols representing parameter values, such as A or a for ASCII TYPE.
	if(arg_size == 1 && (client->arg_buffer[0] == 'S' || client->arg_buffer[0] == 's'))
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Sure.");
		client->response = FTP_RESPONSE_COMMAND_OK;
	}
	else
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "We don't support that mode!!!!!");
		client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED_FOR_GIVEN_ARG;
	}
}

void Ftp_process_retr(Ftp_client* client)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!client)
		return;

	if(client->pending_command == FTP_COMMAND_INVALID)
	{
		uint32_t length = strlen(client->arg_buffer);

		if(length == 0)
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must NOT be empty!!!!!");
			client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
		}
		else
		{
			result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &client->retrieve.dir, &client->retrieve.file_name);
			if(result == DEF_SUCCESS)
			{
				//Check if it actually exists.
				result = Util_file_check_file_exist(client->retrieve.file_name.buffer, client->retrieve.dir.buffer);
				if(result == DEF_SUCCESS)
				{
					client->pending_command = FTP_COMMAND_RETR;

					if(client->data_handle.fd < 0)
					{
						snprintf(client->message_buffer, sizeof(client->message_buffer), "Opening data connection...");
						client->response = FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_WILL_OPEN;
					}
					else
					{
						snprintf(client->message_buffer, sizeof(client->message_buffer), "Data connection already exists, starting transfer...");
						client->response = FTP_RESPONSE_DATA_CONNECTION_EXIST_STARTING_TRANSFER;
					}
				}
				else
				{
					DEF_LOG_FORMAT("Specified file \"%s%s\" does NOT exist!!!!!", client->retrieve.dir.buffer, client->retrieve.file_name.buffer);
					DEF_LOG_RESULT(Util_file_check_file_exist, false, result);
					snprintf(client->message_buffer, sizeof(client->message_buffer), "Specified file does NOT exist!!!!!");
					client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;

					//Clear state.
					Ftp_retrieve_clear(&client->retrieve);
				}
			}
			else
			{
				DEF_LOG_RESULT(Ftp_get_path, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
			}
		}
	}
	else
	{
		DEF_LOG_STRING("There's pending command!!!!!");
		DEF_LOG_STRING(Ftp_command_get_name(client->pending_command));
		snprintf(client->message_buffer, sizeof(client->message_buffer), "You must wait for current command to finish!!!!!");
		client->response = FTP_RESPONSE_FILE_ACTION_ABORT_LOCAL_ERROR;
	}
}

void Ftp_process_stor(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe(client, false, false);
}

void Ftp_process_stou(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe(client, false, true);
}

void Ftp_process_appe(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe(client, true, false);
}

void Ftp_process_allo(Ftp_client* client)
{
	if(!client)
		return;

	//RFC959: The ALLO command should be treated as a NOOP (no operation) by those servers
	//which do not require that the maximum size of the file be declared beforehand.
	snprintf(client->message_buffer, sizeof(client->message_buffer), "OK.");
	client->response = FTP_RESPONSE_COMMAND_OK;
}

void Ftp_process_rest(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "Unsupported!!!!!");
	client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED;
}

void Ftp_process_rnfr(Ftp_client* client)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!client)
		return;

	if(client->pending_command == FTP_COMMAND_INVALID)
	{
		uint32_t length = strlen(client->arg_buffer);

		if(length == 0)
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must NOT be empty!!!!!");
			client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
		}
		else
		{
			result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &client->rename.dir_from, &client->rename.file_name_from);
			if(result == DEF_SUCCESS)
			{
				//Check if it actually exists.
				result = Util_file_check_file_exist(client->rename.file_name_from.buffer, client->rename.dir_from.buffer);
				if(result == DEF_SUCCESS)
				{
					//RFC959: This command must be immediately followed by a "rename to" command specifying the new file pathname.
					client->pending_command = FTP_COMMAND_RNFR;

					snprintf(client->message_buffer, sizeof(client->message_buffer), "OK, waiting for next command...");
					client->response = FTP_RESPONSE_FILE_ACTION_PENDING;
				}
				else
				{
					//Retry as directory.
					DEF_LOG_FORMAT("Specified file \"%s%s\" does NOT exist, retrying as a directory...", client->rename.dir_from.buffer, client->rename.file_name_from.buffer);
					DEF_LOG_RESULT(Util_file_check_file_exist, false, result);
					Ftp_rename_clear(&client->rename);

					result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &client->rename.dir_from, NULL);
					if(result == DEF_SUCCESS)
					{
						//Check if it actually exists.
						result = Util_file_check_directory_exist(client->rename.dir_from.buffer);
						if(result == DEF_SUCCESS)
						{
							//RFC959: This command must be immediately followed by a "rename to" command specifying the new file pathname.
							client->pending_command = FTP_COMMAND_RNFR;

							snprintf(client->message_buffer, sizeof(client->message_buffer), "OK, waiting for next command...");
							client->response = FTP_RESPONSE_FILE_ACTION_PENDING;
						}
						else
						{
							DEF_LOG_FORMAT("Specified directory \"%s\" does NOT exist!!!!!", client->rename.dir_from.buffer);
							DEF_LOG_RESULT(Util_file_check_directory_exist, false, result);
							snprintf(client->message_buffer, sizeof(client->message_buffer), "Specified path does NOT exist!!!!!");
							client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;

							//Clear state.
							Ftp_rename_clear(&client->rename);
						}
					}
					else
					{
						DEF_LOG_RESULT(Ftp_get_path, false, result);
						snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
						client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
					}
				}
			}
			else
			{
				DEF_LOG_RESULT(Ftp_get_path, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
			}
		}
	}
	else
	{
		DEF_LOG_STRING("There's pending command!!!!!");
		DEF_LOG_STRING(Ftp_command_get_name(client->pending_command));
		snprintf(client->message_buffer, sizeof(client->message_buffer), "You must wait for current command to finish!!!!!");
		client->response = FTP_RESPONSE_FILE_ACTION_ABORT_LOCAL_ERROR;
	}
}

void Ftp_process_rnto(Ftp_client* client)
{
	uint32_t result = DEF_ERR_OTHER;

	if(!client)
		return;

	//This command specifies the new pathname of the file specified in the immediately preceding "rename from" command.
	if(client->pending_command == FTP_COMMAND_RNFR)
	{
		uint32_t length = strlen(client->arg_buffer);

		if(length == 0)
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must NOT be empty!!!!!");
			client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
		}
		else
		{
			bool is_file = Util_str_has_data(&client->rename.file_name_from);

			if(is_file)
				result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &client->rename.dir_to, &client->rename.file_name_to);
			else
				result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &client->rename.dir_to, NULL);

			if(result == DEF_SUCCESS)
			{
				bool exists = true;

				//Check if it actually exists.
				if(is_file)
					exists = (Util_file_check_file_exist(client->rename.file_name_to.buffer, client->rename.dir_to.buffer) == DEF_SUCCESS);
				else
					exists = (Util_file_check_directory_exist(client->rename.dir_to.buffer) == DEF_SUCCESS);

				if(exists)
				{
					if(is_file)
						DEF_LOG_FORMAT("Specified file \"%s%s\" already exists!!!!!", client->rename.dir_to.buffer, client->rename.file_name_to.buffer);
					else
						DEF_LOG_FORMAT("Specified directory \"%s\" already exists!!!!!", client->rename.dir_to.buffer);

					snprintf(client->message_buffer, sizeof(client->message_buffer), "Specified path already exists!!!!!");
					client->response = FTP_RESPONSE_FILE_ACTION_ERROR_INVALID_NAME;
				}
				else
				{
					if(is_file)
						result = Util_file_rename_file(client->rename.file_name_from.buffer, client->rename.dir_to.buffer, client->rename.file_name_to.buffer);
					else
						result = Util_file_rename_directory(client->rename.dir_from.buffer, client->rename.dir_to.buffer);

					if(result == DEF_SUCCESS)
					{
						snprintf(client->message_buffer, sizeof(client->message_buffer), "Specified path has been renamed.");
						client->response = FTP_RESPONSE_FILE_ACTION_OK;
					}
					else
					{
						if(is_file)
						{
							DEF_LOG_FORMAT("Couldn't rename file \"%s%s\" to \"%s%s\"!!!!!", client->rename.dir_from.buffer,
							client->rename.file_name_from.buffer, client->rename.dir_to.buffer, client->rename.file_name_to.buffer);
							DEF_LOG_RESULT(Util_file_rename_file, false, result);
						}
						else
						{
							DEF_LOG_FORMAT("Couldn't rename directory \"%s\" to \"%s\"!!!!!", client->rename.dir_from.buffer, client->rename.dir_to.buffer);
							DEF_LOG_RESULT(Util_file_rename_directory, false, result);
						}

						snprintf(client->message_buffer, sizeof(client->message_buffer), "Couldn't rename the specified path, maybe it is locked!!!!!");
						client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;
					}
				}

				client->pending_command = FTP_COMMAND_INVALID;

				//Clear state.
				Ftp_rename_clear(&client->rename);
			}
			else
			{
				DEF_LOG_RESULT(Ftp_get_path, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
			}
		}
	}
	else
	{
		DEF_LOG_STRING("Wrong command order!!!!!");
		DEF_LOG_STRING(Ftp_command_get_name(client->pending_command));
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Invalid command sequence!!!!!");
		client->response = FTP_RESPONSE_COMMAND_SEQUENCE_ERROR;
	}
}

void Ftp_process_abor(Ftp_client* client)
{
	if(!client)
		return;

	//RFC959:
	//(1) the FTP service command was already completed.
	//  The server closes the data connection and responds with a 226 reply.
	//(2) the FTP service command is still in progress.
	//  The server aborts the FTP service in progress and closes the data connection, returning a 426 reply,
	//  The server then sends a 226 reply.
	if(client->pending_command == FTP_COMMAND_INVALID)//(1).
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Aborted.");
		client->response = FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK;
	}
	else//(2).
	{
		uint32_t result = DEF_ERR_OTHER;

		//Close data connection.
		Ftp_close_socket(client, false, false);
		client->pending_command = FTP_COMMAND_INVALID;

		snprintf(client->message_buffer, sizeof(client->message_buffer), "Aborted by ABOR command!!!!!");
		client->response = FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_TRANSFER_ABORT;

		result = Ftp_send_response(client->control_handle.fd, client->response, client->message_buffer);
		if(result != DEF_SUCCESS)
			DEF_LOG_RESULT(Ftp_send_response, false, result);

		snprintf(client->message_buffer, sizeof(client->message_buffer), "Aborted.");
		client->response = FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK;
	}

	//Clear state.
	Ftp_list_clear(&client->list);
	Ftp_store_clear(&client->store);
	Ftp_dir_content_clear(&client->dir_content);
}

void Ftp_process_dele(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_dele_rmd(client, true);
}

void Ftp_process_rmd(Ftp_client* client)
{
	if(!client)
		return;

	//RFC959: The reply codes for RMD be identical to the reply codes for DELE.
	Ftp_process_dele_rmd(client, false);
}

void Ftp_process_mkd(Ftp_client* client)
{
	uint32_t length = 0;
	uint32_t result = DEF_ERR_OTHER;
	Str_data temp_dir = { 0, };

	if(!client)
		return;

	length = strlen(client->arg_buffer);
	if(length == 0)
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must NOT be empty!!!!!");
		client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
	}
	else
	{
		result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &temp_dir, NULL);
		if(result == DEF_SUCCESS)
		{
			result = Util_file_check_directory_exist(temp_dir.buffer);
			if(result == DEF_SUCCESS)
			{
				//RFC959: The prior existence of a subdirectory with the same name is an error,
				//and the server must return an "access denied" error reply.
				DEF_LOG_FORMAT("Specified directory \"%s\" already exists!!!!!", temp_dir.buffer);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s\" already exist!!!!!", temp_dir.buffer);
				client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;
			}
			else
			{
				result = Util_file_create_directory(temp_dir.buffer);
				if(result == DEF_SUCCESS)
				{
					snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s\" has been created.", temp_dir.buffer);
					client->response = FTP_RESPONSE_DIRECTORY_CREATED;
				}
				else
				{
					DEF_LOG_FORMAT("Specified directory \"%s\" couldn't be created!!!!!", temp_dir.buffer);
					DEF_LOG_RESULT(Util_file_create_directory, false, result);
					snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s\" couldn't be created!!!!!", temp_dir.buffer);
					client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;
				}
			}
		}
		else
		{
			DEF_LOG_RESULT(Ftp_get_path, false, result);
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
		}
	}

	Util_str_free(&temp_dir);
}

void Ftp_process_pwd(Ftp_client* client)
{
	if(!client)
		return;

	//RFC959: Essentially because the PWD command returns the same type of information as the
	//successful MKD command, the successful PWD command uses the 257 reply code as well.
	snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s\" is the current directory.", DEF_STR_NEVER_NULL(&client->current_dir.dir));
	client->response = FTP_RESPONSE_DIRECTORY_CREATED;
}

void Ftp_process_list(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_list_nlst(client, true);
}

void Ftp_process_nlst(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_list_nlst(client, false);
}

void Ftp_process_site(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "We don't have site specific commands.");
	client->response = FTP_RESPONSE_COMMAND_UNNECESSARY;
}

void Ftp_process_syst(Ftp_client* client)
{
	if(!client)
		return;

	//RFC959: The reply shall have as its first word one of the system names
	//listed in the current version of the https://www.rfc-editor.org/rfc/rfc943.
	snprintf(client->message_buffer, sizeof(client->message_buffer), "UNIX ...actually, we are running on 3DS/2DS.");
	client->response = FTP_RESPONSE_STATUS_SYSTEM_TYPE;
}

void Ftp_process_stat(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "Unsupported!!!!!");
	client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED;
}

void Ftp_process_help(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "FTP daemon for 3DS/2DS.");
	client->response = FTP_RESPONSE_STATUS_HELP_FOR_USER;
}

void Ftp_process_noop(Ftp_client* client)
{
	if(!client)
		return;

	snprintf(client->message_buffer, sizeof(client->message_buffer), "OK.");
	client->response = FTP_RESPONSE_COMMAND_OK;
}

void Ftp_process_retr_bof(Ftp_client* client)
{
	uint8_t* buffer = NULL;
	uint32_t read_size = 0;
	uint64_t offset = 0;
	uint32_t result = DEF_ERR_OTHER;

	if(!client)
		return;

	while(true)
	{
		result = Util_file_load_from_file(DEF_STR_NEVER_NULL(&client->retrieve.file_name),
		DEF_STR_NEVER_NULL(&client->retrieve.dir), &buffer, IO_BUFFER_SIZE, offset, &read_size);
		if(result == DEF_SUCCESS)
		{
			bool is_eof = (read_size < IO_BUFFER_SIZE);

			offset += read_size;
			//Send the data via data connection.
			result = Ftp_send_data(client->data_handle.fd, buffer, read_size);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Ftp_send_data, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
				break;
			}

			if(is_eof)
			{
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Done.");
				client->response = FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK;
				break;
			}
		}
		else
		{
			DEF_LOG_FORMAT("Specified file \"%s%s\" does NOT exist!!!!!", client->retrieve.dir.buffer, client->retrieve.file_name.buffer);
			DEF_LOG_RESULT(Util_file_load_from_file, false, result);
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Specified file does NOT exist!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
			break;
		}

		free(buffer);
		buffer = NULL;
	}

	//In case of break.
	free(buffer);
	buffer = NULL;
}

void Ftp_process_list_bof(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_list_nlst_bof(client);
}

void Ftp_process_nlst_bof(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_list_nlst_bof(client);
}

void Ftp_process_stor_data(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe_data(client);
}

void Ftp_process_stou_data(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe_data(client);
}

void Ftp_process_appe_data(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe_data(client);
}

void Ftp_process_retr_eof(Ftp_client* client)
{
	if(!client)
		return;

	//Close data connection first.
	Ftp_close_socket(client, false, false);
	client->pending_command = FTP_COMMAND_INVALID;

	//Clear state.
	Ftp_retrieve_clear(&client->retrieve);
}

void Ftp_process_stor_eof(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe_eof(client);
}

void Ftp_process_stou_eof(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe_eof(client);
}

void Ftp_process_appe_eof(Ftp_client* client)
{
	if(!client)
		return;

	Ftp_process_stor_stou_appe_eof(client);
}

uint32_t Ftp_clients_init(Ftp_clients* clients, uint8_t num_of_clients, uint16_t control_port, uint32_t local_address,
uint32_t global_address, uint16_t data_port_base, uint32_t store_buffer_size, uint32_t dir_content_max_files)
{
	uint32_t result = DEF_ERR_OTHER;
	uint16_t data_port_end = 0;

	if(!clients || num_of_clients == 0 || store_buffer_size == 0 || dir_content_max_files == 0)
		goto invalid_arg;

	//Check for port overlap.
	data_port_end = (data_port_base + (num_of_clients - 1));
	if(control_port >= data_port_base && control_port <= data_port_end)
		goto invalid_arg;

	clients->control_listen_handle.events = POLLIN;
	clients->control_listen_handle.fd = -1;
	clients->clients = (Ftp_client*)calloc(num_of_clients, sizeof(Ftp_client));
	if(!clients->clients)
	{
		DEF_LOG_RESULT(calloc, false, 0);
		goto out_of_memory;
	}

	//Mark them as invalid.
	for(uint8_t i = 0; i < num_of_clients; i++)
	{
		clients->clients[i].data_listen_handle.fd = -1;
		clients->clients[i].control_handle.fd = -1;
		clients->clients[i].data_handle.fd = -1;
	}

	clients->max_clients = num_of_clients;

	clients->control_listen_handle.fd = Ftp_listen(control_port);
	if(clients->control_listen_handle.fd < 0)
	{
		DEF_LOG_RESULT(Ftp_listen, false, clients->control_listen_handle.fd);
		goto socket_failed;
	}

	for(uint8_t i = 0; i < num_of_clients; i++)
	{
		result = Ftp_client_init(&clients->clients[i], control_port, (data_port_base + i), local_address, global_address, store_buffer_size, dir_content_max_files);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Ftp_client_init, false, result);
			goto error;
		}
	}

	Ftp_clients_clear(clients);
	return DEF_SUCCESS;

	invalid_arg:
	return DEF_ERR_INVALID_ARG;

	out_of_memory:
	Ftp_clients_exit(clients);
	return DEF_ERR_OUT_OF_MEMORY;

	socket_failed:
	Ftp_clients_exit(clients);
	return DEF_ERR_SOCKET_RETURNED_NOT_SUCCESS;

	error:
	Ftp_clients_exit(clients);
	return result;
}

void Ftp_clients_clear(Ftp_clients* clients)
{
	if(!clients)
		return;

	if(clients->clients)
	{
		for(uint8_t i = 0; i < clients->max_clients; i++)
			Ftp_client_clear(&clients->clients[i]);
	}
}

void Ftp_client_clear(Ftp_client* client)
{
	if(!client)
		return;

	client->data_port_active = 0;
	client->data_address_active = 0;
	client->pending_command = FTP_COMMAND_INVALID;
	client->response = FTP_RESPONSE_INVALID;
	memset(client->arg_buffer, 0x00, sizeof(client->arg_buffer));
	memset(client->message_buffer, 0x00, sizeof(client->message_buffer));
	Ftp_close_socket_internal(client->data_listen_handle.fd, true);
	client->data_listen_handle.fd = -1;
	Ftp_close_socket(client, false, true);
	Ftp_close_socket(client, true, true);
	Ftp_current_dir_clear(&client->current_dir);
	Ftp_list_clear(&client->list);
	Ftp_rename_clear(&client->rename);
	Ftp_retrieve_clear(&client->retrieve);
	Ftp_store_clear(&client->store);
	Ftp_dir_content_clear(&client->dir_content);
}

void Ftp_current_dir_clear(Ftp_current_dir* current_dir)
{
	if(!current_dir)
		return;

	Util_str_set(&current_dir->dir, "/");
}

void Ftp_list_clear(Ftp_list* list)
{
	if(!list)
		return;

	Util_str_clear(&list->dir);
}

void Ftp_rename_clear(Ftp_rename* rename)
{
	if(!rename)
		return;

	Util_str_clear(&rename->dir_from);
	Util_str_clear(&rename->dir_to);
	Util_str_clear(&rename->file_name_from);
	Util_str_clear(&rename->file_name_to);
}

void Ftp_retrieve_clear(Ftp_retrieve* retrieve)
{
	if(!retrieve)
		return;

	Util_str_clear(&retrieve->dir);
	Util_str_clear(&retrieve->file_name);
}

void Ftp_store_clear(Ftp_store* store)
{
	if(!store)
		return;

	store->is_append = false;
	Util_str_clear(&store->dir);
	Util_str_clear(&store->file_name);
	if(store->buffer)
		memset(store->buffer, 0x00, store->buffer_size);
}

void Ftp_dir_content_clear(Ftp_dir_content* dir_content)
{
	if(!dir_content)
		return;

	dir_content->detected_files = 0;
	if(dir_content->file_names)
	{
		for(uint32_t i = 0; i < dir_content->max_files; i++)
			Util_str_clear(&dir_content->file_names[i]);
	}
	if(dir_content->file_types)
	{
		for(uint32_t i = 0; i < dir_content->max_files; i++)
			dir_content->file_types[i] = FILE_TYPE_NONE;
	}
}

void Ftp_clients_exit(Ftp_clients* clients)
{
	if(!clients)
		return;

	Ftp_close_socket_internal(clients->control_listen_handle.fd, true);
	clients->control_listen_handle.fd = -1;

	if(clients->clients)
	{
		for(uint8_t i = 0; i < clients->max_clients; i++)
			Ftp_client_exit(&clients->clients[i]);
	}

	free(clients->clients);
	clients->clients = NULL;
}

void Ftp_close_socket(Ftp_client* client, bool is_control, bool is_force)
{
	if(!client)
		return;

	Ftp_close_socket_internal((is_control ? client->control_handle.fd : client->data_handle.fd), is_force);

	if(is_control)
	{
		client->control_handle.fd = -1;
		memset(&client->control_address, 0x00, sizeof(client->control_address));
	}
	else
	{
		client->data_handle.fd = -1;
		memset(&client->data_address, 0x00, sizeof(client->data_address));
	}
}

static bool Ftp_case_insensitive_compare(const char* a, const char* b, uint32_t size)
{
	for(uint32_t i = 0; i < size; i++)
	{
		if(toupper(a[i]) != toupper(b[i]))
			return false;
	}

	return true;
}

static uint32_t Ftp_greet(Ftp_client* client)
{
	uint32_t result = DEF_ERR_OTHER;

	result = Ftp_send_response(client->control_handle.fd, FTP_RESPONSE_STATUS_SERVICE_READY, "Hi, how can I help you today?");
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_send_response, false, result);
		goto error;
	}

	return DEF_SUCCESS;

	error:
	Ftp_client_clear(client);
	return result;
}

static void Ftp_process_stor_stou_appe(Ftp_client* client, bool is_append, bool is_unique)
{
	uint32_t length = strlen(client->arg_buffer);
	uint32_t result = DEF_ERR_OTHER;

	if(client->pending_command == FTP_COMMAND_INVALID)
	{
		if(length == 0)
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must NOT be empty!!!!!");
			client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
		}
		else
		{
			result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &client->store.dir, &client->store.file_name);
			if(result == DEF_SUCCESS)
			{
				//If unique one is requested, generate unique filename.
				if(is_unique)
				{
					Str_data unique_filename = { 0, };

					result = Ftp_get_unique_filename(client->store.dir.buffer, client->store.file_name.buffer, &unique_filename);
					if(result == DEF_SUCCESS)
					{
						result = Util_str_set(&client->store.file_name, unique_filename.buffer);
						if(result != DEF_SUCCESS)
							DEF_LOG_RESULT(Util_str_set, false, result);
					}
					else
						DEF_LOG_RESULT(Ftp_get_unique_filename, false, result);

					Util_str_free(&unique_filename);
				}
				client->store.is_append = is_append;

				if(result == DEF_SUCCESS)
				{
					if(client->data_handle.fd < 0)
					{
						snprintf(client->message_buffer, sizeof(client->message_buffer), "Opening data connection...");
						client->response = FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_WILL_OPEN;
					}
					else
					{
						snprintf(client->message_buffer, sizeof(client->message_buffer), "Data connection already exists, starting transfer...");
						client->response = FTP_RESPONSE_DATA_CONNECTION_EXIST_STARTING_TRANSFER;
					}

					if(is_append)
						client->pending_command = FTP_COMMAND_APPE;
					else if(is_unique)
						client->pending_command = FTP_COMMAND_STOU;
					else
						client->pending_command = FTP_COMMAND_STOR;
				}
				else
				{
					snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
					client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
				}
			}
			else
			{
				DEF_LOG_RESULT(Ftp_get_path, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
			}
		}
	}
	else
	{
		DEF_LOG_STRING("There's pending command!!!!!");
		DEF_LOG_STRING(Ftp_command_get_name(client->pending_command));
		snprintf(client->message_buffer, sizeof(client->message_buffer), "You must wait for current command to finish!!!!!");
		client->response = FTP_RESPONSE_FILE_ACTION_ABORT_LOCAL_ERROR;
	}
}

static void Ftp_process_dele_rmd(Ftp_client* client, bool is_file)
{
	uint32_t length = strlen(client->arg_buffer);
	uint32_t result = DEF_ERR_OTHER;
	Str_data temp_file = { 0, };
	Str_data temp_dir = { 0, };

	if(length == 0)
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Arg must NOT be empty!!!!!");
		client->response = FTP_RESPONSE_COMMAND_ARG_MALFORMED;
	}
	else
	{
		result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &temp_dir, (is_file ? &temp_file : NULL));
		if(result == DEF_SUCCESS)
		{
			if(is_file)
				result = Util_file_delete_file(temp_file.buffer, temp_dir.buffer);
			else
				result = Util_file_delete_directory(temp_dir.buffer);

			if(result == DEF_SUCCESS)
			{
				//"temp_file.buffer" is NULL when "!is_file".
				snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s%s\" has been permanently deleted.", temp_dir.buffer, DEF_STR_NEVER_NULL(&temp_file));
				client->response = FTP_RESPONSE_FILE_ACTION_OK;
			}
			else
			{
				if(is_file)
				{
					DEF_LOG_FORMAT("File \"%s%s\" does NOT exist or locked!!!!!", temp_dir.buffer, temp_file.buffer);
					DEF_LOG_RESULT(Util_file_delete_file, false, result);
				}
				else
				{
					DEF_LOG_FORMAT("Directory \"%s\" does NOT exist or locked!!!!!", temp_dir.buffer);
					DEF_LOG_RESULT(Util_file_delete_directory, false, result);
				}

				//"temp_file.buffer" is NULL when "!is_file".
				snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s%s\" does NOT exist or locked!!!!!", temp_dir.buffer, DEF_STR_NEVER_NULL(&temp_file));
				client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;
			}
		}
		else
		{
			DEF_LOG_RESULT(Ftp_get_path, false, result);
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
		}
	}

	Util_str_free(&temp_file);
	Util_str_free(&temp_dir);
}

static void Ftp_process_list_nlst(Ftp_client* client, bool is_list)
{
	uint32_t result = DEF_ERR_OTHER;

	if(client->pending_command == FTP_COMMAND_INVALID)
	{
		uint32_t length = strlen(client->arg_buffer);

		//Treat "LIST/NLST -a" as a plain "LIST/NLST".
		if(length == 0 || (length == 2 && client->arg_buffer[0] == '-' && client->arg_buffer[1] == 'a'))
		{
			result = Util_str_set(&client->list.dir, DEF_STR_NEVER_NULL(&client->current_dir.dir));
			if(result != DEF_SUCCESS)
				DEF_LOG_RESULT(Util_str_set, false, result);
		}
		else
		{
			result = Ftp_get_path(DEF_STR_NEVER_NULL(&client->current_dir.dir), client->arg_buffer, &client->list.dir, NULL);
			if(result != DEF_SUCCESS)
				DEF_LOG_RESULT(Ftp_get_path, false, result);
		}

		if(result == DEF_SUCCESS)
		{
			if(client->data_handle.fd < 0)
			{
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Opening data connection...");
				client->response = FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_WILL_OPEN;
			}
			else
			{
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Data connection already exists, starting transfer...");
				client->response = FTP_RESPONSE_DATA_CONNECTION_EXIST_STARTING_TRANSFER;
			}

			if(is_list)
				client->pending_command = FTP_COMMAND_LIST;
			else
				client->pending_command = FTP_COMMAND_NLST;
		}
		else
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
		}
	}
	else
	{
		DEF_LOG_STRING("There's pending command!!!!!");
		DEF_LOG_STRING(Ftp_command_get_name(client->pending_command));
		snprintf(client->message_buffer, sizeof(client->message_buffer), "You must wait for current command to finish!!!!!");
		client->response = FTP_RESPONSE_FILE_ACTION_ABORT_LOCAL_ERROR;
	}
}

static void Ftp_process_list_nlst_bof(Ftp_client* client)
{
	bool is_success = true;
	uint32_t result = DEF_ERR_OTHER;

	result = Util_file_read_dir(DEF_STR_NEVER_NULL(&client->list.dir), &client->dir_content.detected_files, client->dir_content.file_names, client->dir_content.file_types, client->dir_content.max_files);
	if(result == DEF_SUCCESS)
	{
		for(uint32_t i = 0; i < client->dir_content.detected_files; i++)
		{
			uint32_t size = 0;

			if(client->pending_command == FTP_COMMAND_LIST)
			{
				//https://cr.yp.to/ftp/list/binls.html
				if(client->dir_content.file_types[i] & FILE_TYPE_DIR)
					size = snprintf(client->message_buffer, sizeof(client->message_buffer), "drwxr-xr-x 1 owner group             0 Jan 01  1990 %s\r\n", DEF_STR_NEVER_NULL(&client->dir_content.file_names[i]));
				else if(client->dir_content.file_types[i] & FILE_TYPE_FILE)
				{
					uint64_t file_size = 0;

					if(Util_file_check_file_size(DEF_STR_NEVER_NULL(&client->dir_content.file_names[i]), DEF_STR_NEVER_NULL(&client->list.dir), &file_size) != DEF_SUCCESS)
						file_size = 0;

					size = snprintf(client->message_buffer, sizeof(client->message_buffer), "-rwxr-xr-x 1 owner group %13" PRIu64 " Jan 01  1990 %s\r\n", file_size, DEF_STR_NEVER_NULL(&client->dir_content.file_names[i]));
				}
				else
					continue;
			}
			else
				size = snprintf(client->message_buffer, sizeof(client->message_buffer), "%s\r\n", DEF_STR_NEVER_NULL(&client->dir_content.file_names[i]));

			//Send the data via data connection.
			result = Ftp_send_data(client->data_handle.fd, (const uint8_t*)client->message_buffer, Util_min(sizeof(client->message_buffer), size));
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Ftp_send_data, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
				is_success = false;
				break;
			}
		}
	}
	else
	{
		DEF_LOG_RESULT(Util_file_read_dir, false, result);
		//Nothing to send.
	}

	//Close data connection first.
	Ftp_close_socket(client, false, false);
	client->pending_command = FTP_COMMAND_INVALID;

	//Clear state.
	Ftp_list_clear(&client->list);
	Ftp_dir_content_clear(&client->dir_content);

	if(is_success)
	{
		snprintf(client->message_buffer, sizeof(client->message_buffer), "Done.");
		client->response = FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK;
	}
}

static void Ftp_process_stor_stou_appe_data(Ftp_client* client)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t received_size = 0;

	result = Ftp_receive_data(client->data_handle.fd, client->store.buffer, client->store.buffer_size, &received_size);
	if(result == DEF_SUCCESS)
	{
		if(received_size > 0)
		{
			result = Util_file_save_to_file(DEF_STR_NEVER_NULL(&client->store.file_name), DEF_STR_NEVER_NULL(&client->store.dir),
			client->store.buffer, received_size, !client->store.is_append);
			if(result == DEF_SUCCESS)
			{
				client->store.is_append = true;
				client->response = FTP_RESPONSE_INVALID;//No response at this time, wait for next packet.
			}
			else
			{
				DEF_LOG_FORMAT("Couldn't write to file \"%s%s\"!!!!!", client->retrieve.dir.buffer, client->retrieve.file_name.buffer);
				DEF_LOG_RESULT(Util_file_save_to_file, false, result);
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
			}
		}
		else
		{
			//EOF.
			if(client->pending_command == FTP_COMMAND_STOU)//RFC959: The response must include the name generated.
				snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s\" is the one actually used, done.", DEF_STR_NEVER_NULL(&client->store.file_name));
			else
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Done.");

			client->response = FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK;
		}
	}
	else
	{
		DEF_LOG_RESULT(Ftp_receive_data, false, result);
		client->response = FTP_RESPONSE_INVALID;//No response at this time, wait for next packet.
	}
}

static void Ftp_process_stor_stou_appe_eof(Ftp_client* client)
{
	//Close data connection first.
	Ftp_close_socket(client, false, false);
	client->pending_command = FTP_COMMAND_INVALID;

	//Clear state.
	Ftp_store_clear(&client->store);
}

static bool Ftp_is_loopback_ip(uint32_t address)
{
	uint8_t address_a = ((address) >> 24);

    if(address_a == 127)
		return true;
	else
		return false;
}

static bool Ftp_is_local_ip(uint32_t address)
{
	uint8_t address_a = ((address) >> 24);
	uint8_t address_b = ((address) >> 16);

    if(address_a == 10)
		return true;//Class A.
    else if(address_a == 172 && address_b >= 16 && address_b <= 31)
		return true;//Class B.
    else if(address_a == 192 && address_b == 168)
		return true;//Class C.
	else
		return false;
}

static int32_t Ftp_listen(uint16_t port)
{
	int32_t socket_handle = 0;
	uint32_t result = DEF_ERR_OTHER;
	struct sockaddr_in address = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = htonl(INADDR_ANY), };

	errno = 0;
	socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_handle == -1)
	{
		DEF_LOG_UINT(errno);
		DEF_LOG_RESULT(socket, false, socket_handle);
		goto error;
	}

	errno = 0;
	result = bind(socket_handle, (struct sockaddr*)&address, sizeof(struct sockaddr_in));
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_UINT(errno);
		DEF_LOG_RESULT(bind, false, result);
		goto error;
	}

	errno = 0;
	result = listen(socket_handle, 1);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_UINT(errno);
		DEF_LOG_RESULT(listen, false, result);
		goto error;
	}

	return socket_handle;

	error:
	Ftp_close_socket_internal(socket_handle, true);
	return -1;
}

static int32_t Ftp_connect(uint16_t port, uint32_t address)
{
	int32_t socket_handle = 0;
	uint32_t result = DEF_ERR_OTHER;
	struct sockaddr_in target_address = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = htonl(address), };

	errno = 0;
	socket_handle = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_handle == -1)
	{
		DEF_LOG_UINT(errno);
		DEF_LOG_RESULT(socket, false, socket_handle);
		goto error;
	}

	errno = 0;
	result = connect(socket_handle, (struct sockaddr*)&target_address, sizeof(struct sockaddr_in));
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_UINT(errno);
		DEF_LOG_RESULT(connect, false, result);
		goto error;
	}

	return socket_handle;

	error:
	Ftp_close_socket_internal(socket_handle, true);
	return -1;
}

static void Ftp_close_socket_internal(int32_t socket_handle, bool is_force)
{
	if(socket_handle >= 0)
	{
		if(is_force)
		{
			//This will make close() send RST.
			struct linger linger = { .l_onoff = true, .l_linger = 0, };

			setsockopt(socket_handle, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
		}

		shutdown(socket_handle, SHUT_RDWR);
		close(socket_handle);
	}
}

static uint32_t Ftp_client_init(Ftp_client* client, uint16_t control_port, uint16_t data_port, uint32_t local_address,
uint32_t global_address, uint32_t store_buffer_size, uint32_t dir_content_max_files)
{
	uint32_t result = DEF_ERR_OTHER;

	client->control_port = control_port;
	client->data_port_passive = data_port;
	client->my_local_address_passive = local_address;
	client->my_global_address_passive = global_address;
	client->data_listen_handle.events = POLLIN;
	client->data_listen_handle.fd = -1;
	client->control_handle.events = POLLIN;
	client->control_handle.fd = -1;
	client->data_handle.events = POLLIN;
	client->data_handle.fd = -1;

	result = Ftp_current_dir_init(&client->current_dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_current_dir_init, false, result);
		goto error;
	}

	result = Ftp_list_init(&client->list);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_list_init, false, result);
		goto error;
	}

	result = Ftp_rename_init(&client->rename);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_rename_init, false, result);
		goto error;
	}

	result = Ftp_retrieve_init(&client->retrieve);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_retrieve_init, false, result);
		goto error;
	}

	result = Ftp_store_init(&client->store, store_buffer_size);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_store_init, false, result);
		goto error;
	}

	result = Ftp_dir_content_init(&client->dir_content, dir_content_max_files);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_dir_content_init, false, result);
		goto error;
	}

	Ftp_client_clear(client);
	return DEF_SUCCESS;

	error:
	Ftp_client_exit(client);
	return result;
}

static uint32_t Ftp_current_dir_init(Ftp_current_dir* current_dir)
{
	uint32_t result = DEF_ERR_OTHER;

	result = Util_str_init(&current_dir->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	Ftp_current_dir_clear(current_dir);
	return DEF_SUCCESS;

	error:
	Ftp_current_dir_exit(current_dir);
	return result;
}

static uint32_t Ftp_list_init(Ftp_list* list)
{
	uint32_t result = DEF_ERR_OTHER;

	result = Util_str_init(&list->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	Ftp_list_clear(list);
	return DEF_SUCCESS;

	error:
	Ftp_list_exit(list);
	return result;
}

static uint32_t Ftp_rename_init(Ftp_rename* rename)
{
	uint32_t result = DEF_ERR_OTHER;

	result = Util_str_init(&rename->dir_from);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&rename->dir_to);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&rename->file_name_from);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&rename->file_name_to);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	Ftp_rename_clear(rename);
	return DEF_SUCCESS;

	error:
	Ftp_rename_exit(rename);
	return result;
}

static uint32_t Ftp_retrieve_init(Ftp_retrieve* retrieve)
{
	uint32_t result = DEF_ERR_OTHER;

	result = Util_str_init(&retrieve->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&retrieve->file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	Ftp_retrieve_clear(retrieve);
	return DEF_SUCCESS;

	error:
	Ftp_retrieve_exit(retrieve);
	return result;
}

static uint32_t Ftp_store_init(Ftp_store* store, uint32_t buffer_size)
{
	uint32_t result = DEF_ERR_OTHER;

	store->buffer_size = buffer_size;
	store->buffer = (uint8_t*)malloc(buffer_size);
	if(!store->buffer)
	{
		DEF_LOG_RESULT(malloc, false, 0);
		goto out_of_memory;
	}

	result = Util_str_init(&store->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&store->file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	Ftp_store_clear(store);
	return DEF_SUCCESS;

	out_of_memory:
	Ftp_store_exit(store);
	return DEF_ERR_OUT_OF_MEMORY;

	error:
	Ftp_store_exit(store);
	return result;
}

static uint32_t Ftp_dir_content_init(Ftp_dir_content* dir_content, uint32_t max_files)
{
	uint32_t result = DEF_ERR_OTHER;

	dir_content->file_names = (Str_data*)calloc(max_files, sizeof(Str_data));
	dir_content->file_types = (File_type*)malloc(sizeof(File_type) * max_files);
	if(!dir_content->file_names || !dir_content->file_types)
	{
		if(!dir_content->file_names)
			DEF_LOG_RESULT(calloc, false, 0);
		if(!dir_content->file_types)
			DEF_LOG_RESULT(malloc, false, 0);

		goto out_of_memory;
	}

	dir_content->max_files = max_files;
	for(uint32_t i = 0; i < max_files; i++)
	{
		result = Util_str_init(&dir_content->file_names[i]);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto error;
		}
	}

	Ftp_dir_content_clear(dir_content);
	return DEF_SUCCESS;

	out_of_memory:
	Ftp_dir_content_exit(dir_content);
	return DEF_ERR_OUT_OF_MEMORY;

	error:
	Ftp_dir_content_exit(dir_content);
	return result;
}

static void Ftp_client_exit(Ftp_client* client)
{
	Ftp_close_socket_internal(client->data_listen_handle.fd, true);
	client->data_listen_handle.fd = -1;
	Ftp_close_socket(client, false, true);
	Ftp_close_socket(client, true, true);
	Ftp_current_dir_exit(&client->current_dir);
	Ftp_list_exit(&client->list);
	Ftp_rename_exit(&client->rename);
	Ftp_retrieve_exit(&client->retrieve);
	Ftp_store_exit(&client->store);
	Ftp_dir_content_exit(&client->dir_content);
}

static void Ftp_current_dir_exit(Ftp_current_dir* current_dir)
{
	Util_str_free(&current_dir->dir);
}

static void Ftp_list_exit(Ftp_list* list)
{
	Util_str_free(&list->dir);
}

static void Ftp_rename_exit(Ftp_rename* rename)
{
	Util_str_free(&rename->dir_from);
	Util_str_free(&rename->dir_to);
	Util_str_free(&rename->file_name_from);
	Util_str_free(&rename->file_name_to);
}

static void Ftp_retrieve_exit(Ftp_retrieve* retrieve)
{
	Util_str_free(&retrieve->dir);
	Util_str_free(&retrieve->file_name);
}

static void Ftp_store_exit(Ftp_store* store)
{
	free(store->buffer);
	store->buffer = NULL;
	Util_str_free(&store->dir);
	Util_str_free(&store->file_name);
}

static void Ftp_dir_content_exit(Ftp_dir_content* dir_content)
{
	if(dir_content->file_names)
	{
		for(uint32_t i = 0; i < dir_content->max_files; i++)
			Util_str_free(&dir_content->file_names[i]);
	}

	free(dir_content->file_names);
	free(dir_content->file_types);
	dir_content->file_names = NULL;
	dir_content->file_types = NULL;
}

#endif //DEF_FTP_API_ENABLE
