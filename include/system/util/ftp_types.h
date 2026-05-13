#if !defined(DEF_FTP_TYPES_H)
#define DEF_FTP_TYPES_H
#include <arpa/inet.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include "system/util/file_types.h"
#include "system/util/log_enum_types.h"
#include "system/util/str_types.h"

#define DEF_FTP_API_ENABLE				/*(bool)(*/true/*)*/	//Enable FTP API.
#define DEF_FTP_ARG_BUFFER_SIZE			(uint32_t)(500)			//Size of control connection command buffer.
#define DEF_FTP_MSG_BUFFER_SIZE			(uint32_t)(500)			//Size of control connection reply buffer.

typedef enum
{
	FTP_COMMAND_INVALID = -1,

	FTP_COMMAND_USER,	//User name.
	FTP_COMMAND_PASS,	//Password.
	FTP_COMMAND_ACCT,	//Account.
	FTP_COMMAND_CWD,	//Change working directory.
	FTP_COMMAND_CDUP,	//Change to parent directory.
	FTP_COMMAND_SMNT,	//Structure mount.
	FTP_COMMAND_REIN,	//Reinitialize. 
	FTP_COMMAND_QUIT,	//Logout.
	FTP_COMMAND_PORT,	//Data port.
	FTP_COMMAND_PASV,	//Passive.
	FTP_COMMAND_TYPE,	//Representation type.
	FTP_COMMAND_STRU,	//File structure.
	FTP_COMMAND_MODE,	//Transfer mode.
	FTP_COMMAND_RETR,	//Retrieve.
	FTP_COMMAND_STOR,	//Store.
	FTP_COMMAND_STOU,	//Store unique.
	FTP_COMMAND_APPE,	//Append.
	FTP_COMMAND_ALLO,	//Allocate.
	FTP_COMMAND_REST,	//Restart.
	FTP_COMMAND_RNFR,	//Rename from.
	FTP_COMMAND_RNTO,	//Rename to.
	FTP_COMMAND_ABOR,	//Abort.
	FTP_COMMAND_DELE,	//Delete.
	FTP_COMMAND_RMD,	//Remove directory.
	FTP_COMMAND_MKD,	//Make directory.
	FTP_COMMAND_PWD,	//Print working directory.
	FTP_COMMAND_LIST,	//List.
	FTP_COMMAND_NLST,	//Name list.
	FTP_COMMAND_SITE,	//Site parameters.
	FTP_COMMAND_SYST,	//System.
	FTP_COMMAND_STAT,	//Status.
	FTP_COMMAND_HELP,	//Help.
	FTP_COMMAND_NOOP,	//Noop.

	FTP_COMMAND_MAX,
} Ftp_command;

DEF_LOG_ENUM_DEBUG
(
	Ftp_command,
	FTP_COMMAND_INVALID,
	FTP_COMMAND_USER,
	FTP_COMMAND_PASS,
	FTP_COMMAND_ACCT,
	FTP_COMMAND_CWD,
	FTP_COMMAND_CDUP,
	FTP_COMMAND_SMNT,
	FTP_COMMAND_REIN,
	FTP_COMMAND_QUIT,
	FTP_COMMAND_PORT,
	FTP_COMMAND_PASV,
	FTP_COMMAND_TYPE,
	FTP_COMMAND_STRU,
	FTP_COMMAND_MODE,
	FTP_COMMAND_RETR,
	FTP_COMMAND_STOR,
	FTP_COMMAND_STOU,
	FTP_COMMAND_APPE,
	FTP_COMMAND_ALLO,
	FTP_COMMAND_REST,
	FTP_COMMAND_RNFR,
	FTP_COMMAND_RNTO,
	FTP_COMMAND_ABOR,
	FTP_COMMAND_DELE,
	FTP_COMMAND_RMD,
	FTP_COMMAND_MKD,
	FTP_COMMAND_PWD,
	FTP_COMMAND_LIST,
	FTP_COMMAND_NLST,
	FTP_COMMAND_SITE,
	FTP_COMMAND_SYST,
	FTP_COMMAND_STAT,
	FTP_COMMAND_HELP,
	FTP_COMMAND_NOOP,
	FTP_COMMAND_MAX
)

typedef enum
{
	FTP_RESPONSE_INVALID = -1,

	//Command related.
	FTP_RESPONSE_COMMAND_OK,								//200.
	FTP_RESPONSE_COMMAND_MALFORMED,							//500.
	FTP_RESPONSE_COMMAND_ARG_MALFORMED,						//501.
	FTP_RESPONSE_COMMAND_UNNECESSARY,						//202.
	FTP_RESPONSE_COMMAND_UNIMPLEMENTED,						//502.
	FTP_RESPONSE_COMMAND_SEQUENCE_ERROR,					//503.
	FTP_RESPONSE_COMMAND_UNIMPLEMENTED_FOR_GIVEN_ARG,		//504.
	//Marker related.
	FTP_RESPONSE_RESTART_MARKER,							//110.
	//Status related.
	FTP_RESPONSE_STATUS_SYSTEM_OR_HELP,						//211.
	FTP_RESPONSE_STATUS_DIRECTORY,							//212.
	FTP_RESPONSE_STATUS_FILE,								//213.
	FTP_RESPONSE_STATUS_HELP_FOR_USER,						//214.
	FTP_RESPONSE_STATUS_SYSTEM_TYPE,						//215.
	FTP_RESPONSE_STATUS_EXPECTED_DELAY,						//120.
	FTP_RESPONSE_STATUS_SERVICE_READY,						//220.
	FTP_RESPONSE_STATUS_CONTROL_CONNECTION_CLOSING,			//221.
	FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE,				//421.
	//Data connection related.
	FTP_RESPONSE_DATA_CONNECTION_EXIST_STARTING_TRANSFER,	//125.
	FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_WILL_OPEN,		//150.
	FTP_RESPONSE_DATA_CONNECTION_EXIST_NO_TRANSFER,			//225.
	FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_CANNOT_OPEN,		//425.
	FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK,			//226.
	FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_TRANSFER_ABORT,	//426.
	//Mode related.
	FTP_RESPONSE_PASSIVE_MODE,								//227.
	//Account related.
	FTP_RESPONSE_ACCOUNT_LOGIN,								//230.
	FTP_RESPONSE_ACCOUNT_LOGOUT,							//530.
	FTP_RESPONSE_ACCOUNT_NAME_OK_PASSWORD_NECESSARY,		//331.
	FTP_RESPONSE_ACCOUNT_NECESSARY,							//332.
	FTP_RESPONSE_ACCOUNT_NECESSARY_FOR_STORE,				//532.
	//File/directory related.
	FTP_RESPONSE_FILE_ACTION_OK,							//250.
	FTP_RESPONSE_DIRECTORY_CREATED,							//257.
	FTP_RESPONSE_FILE_ACTION_PENDING,						//350.
	FTP_RESPONSE_FILE_ACTION_ERROR_BUSY,					//450.
	FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS,	//550.
	FTP_RESPONSE_FILE_ACTION_ABORT_LOCAL_ERROR,				//451.
	FTP_RESPONSE_FILE_ACTION_ABORT_UNKNOWN_PAGE_TYPE,		//551.
	FTP_RESPONSE_FILE_ACTION_ERROR_NO_STORAGE,				//452.
	FTP_RESPONSE_FILE_ACTION_ABORT_NO_STORAGE,				//552.
	FTP_RESPONSE_FILE_ACTION_ERROR_INVALID_NAME,			//553.

	FTP_RESPONSE_MAX,
} Ftp_response;

DEF_LOG_ENUM_DEBUG
(
	Ftp_response,
	FTP_RESPONSE_INVALID,
	FTP_RESPONSE_COMMAND_OK,
	FTP_RESPONSE_COMMAND_MALFORMED,
	FTP_RESPONSE_COMMAND_ARG_MALFORMED,
	FTP_RESPONSE_COMMAND_UNNECESSARY,
	FTP_RESPONSE_COMMAND_UNIMPLEMENTED,
	FTP_RESPONSE_COMMAND_SEQUENCE_ERROR,
	FTP_RESPONSE_COMMAND_UNIMPLEMENTED_FOR_GIVEN_ARG,
	FTP_RESPONSE_RESTART_MARKER,
	FTP_RESPONSE_STATUS_SYSTEM_OR_HELP,
	FTP_RESPONSE_STATUS_DIRECTORY,
	FTP_RESPONSE_STATUS_FILE,
	FTP_RESPONSE_STATUS_HELP_FOR_USER,
	FTP_RESPONSE_STATUS_SYSTEM_TYPE,
	FTP_RESPONSE_STATUS_EXPECTED_DELAY,
	FTP_RESPONSE_STATUS_SERVICE_READY,
	FTP_RESPONSE_STATUS_CONTROL_CONNECTION_CLOSING,
	FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE,
	FTP_RESPONSE_DATA_CONNECTION_EXIST_STARTING_TRANSFER,
	FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_WILL_OPEN,
	FTP_RESPONSE_DATA_CONNECTION_EXIST_NO_TRANSFER,
	FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_CANNOT_OPEN,
	FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK,
	FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_TRANSFER_ABORT,
	FTP_RESPONSE_PASSIVE_MODE,
	FTP_RESPONSE_ACCOUNT_LOGIN,
	FTP_RESPONSE_ACCOUNT_LOGOUT,
	FTP_RESPONSE_ACCOUNT_NAME_OK_PASSWORD_NECESSARY,
	FTP_RESPONSE_ACCOUNT_NECESSARY,
	FTP_RESPONSE_ACCOUNT_NECESSARY_FOR_STORE,
	FTP_RESPONSE_FILE_ACTION_OK,
	FTP_RESPONSE_DIRECTORY_CREATED,
	FTP_RESPONSE_FILE_ACTION_PENDING,
	FTP_RESPONSE_FILE_ACTION_ERROR_BUSY,
	FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS,
	FTP_RESPONSE_FILE_ACTION_ABORT_LOCAL_ERROR,
	FTP_RESPONSE_FILE_ACTION_ABORT_UNKNOWN_PAGE_TYPE,
	FTP_RESPONSE_FILE_ACTION_ERROR_NO_STORAGE,
	FTP_RESPONSE_FILE_ACTION_ABORT_NO_STORAGE,
	FTP_RESPONSE_FILE_ACTION_ERROR_INVALID_NAME,
	FTP_RESPONSE_MAX
)

typedef struct
{
	Str_data dir;	//Current directory name.
} Ftp_current_dir;

typedef struct
{
	Str_data dir;	//Target directory name.
} Ftp_list;

typedef struct
{
	Str_data dir_from;			//Original directory name.
	Str_data dir_to;			//Target directory name.
	Str_data file_name_from;	//Original file name.
	Str_data file_name_to;		//Target file name.
} Ftp_rename;

typedef struct
{
	Str_data dir;				//Target directory name.
	Str_data file_name;			//Target file name.
} Ftp_retrieve;

typedef struct
{
	bool is_append;				//Whether this data should be appended.
	uint8_t* buffer;			//Buffer.
	uint32_t buffer_size;		//Buffer size.
	Str_data dir;				//Target directory name.
	Str_data file_name;			//Target file name.
} Ftp_store;

typedef struct
{
	uint32_t max_files;			//Maximum number of files.
	uint32_t detected_files;	//Number of files.
	Str_data* file_names;		//File name list.
	File_type* file_types;		//File type list.
} Ftp_dir_content;

typedef struct
{
	uint16_t control_port;							//Port for listen to incoming control connection.
	uint16_t data_port_passive;						//Port for listen to incoming data connection (for passive mode).
	uint16_t data_port_active;						//Port for data connection (for active mode).
	uint32_t data_address_active;					//IP address for data connection (for active mode).
	uint32_t my_local_address_passive;				//My local IP address (for passive mode).
	uint32_t my_global_address_passive;				//My global IP address (for passive mode).
	Ftp_command pending_command;					//Pending command.
	Ftp_response response;							//Response code.
	Ftp_list list;									//Data for LIST/NLIST commands.
	Ftp_rename rename;								//Data for RNFR/RNTO commands.
	Ftp_retrieve retrieve;							//Data for RETR command.
	Ftp_store store;								//Data for STOR/STOU/APPE commands.
	Ftp_current_dir current_dir;					//Current directory and data for PWD command.
	Ftp_dir_content dir_content;					//Directory content.
	char arg_buffer[DEF_FTP_ARG_BUFFER_SIZE];		//Arg buffer for control connection.
	char message_buffer[DEF_FTP_MSG_BUFFER_SIZE];	//Message buffer for reply.
	struct pollfd data_listen_handle;				//Socket for listening to incoming data connection.
	struct pollfd control_handle;					//Socket for control connection.
	struct pollfd data_handle;						//Socket for data connection.
	struct sockaddr_in control_address;				//IP address for control connection.
	struct sockaddr_in data_address;				//IP address data connection.
	void* user_data;								//User data.
} Ftp_client;

typedef struct
{
	uint8_t max_clients;					//Maximum number of clients.
	struct pollfd control_listen_handle;	//Socket for listening to incoming control connection.
	Ftp_client* clients;					//Client list.
	void* user_data;						//User data.
} Ftp_clients;

#endif //!defined(DEF_FTP_TYPES_H)
