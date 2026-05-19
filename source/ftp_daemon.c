//Includes.
#include "ftp_daemon.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "system/menu.h"
#include "system/sem.h"
#include "system/draw/draw.h"
#include "system/util/cpu_usage.h"
#include "system/util/curl.h"
#include "system/util/err.h"
#include "system/util/expl.h"
#include "system/util/file.h"
#include "system/util/ftp.h"
#include "system/util/gpu_usage.h"
#include "system/util/hid.h"
#include "system/util/json.h"
#include "system/util/log.h"
#include "system/util/net_usage.h"
#include "system/util/nvs_usage.h"
#include "system/util/queue.h"
#include "system/util/ram_usage.h"
#include "system/util/str.h"
#include "system/util/sync.h"
#include "system/util/thread_types.h"
#include "system/util/util.h"
#include "system/util/watch.h"

//Defines.
#define GLOBAL_IP_ADDRESS_API_URL		(const char*)("https://api.ipify.org")	//API for getting global IP address.

#define MAX_FILES						(uint32_t)(1024)		//Maximum number of files.
#define MAX_CLIENTS						(uint8_t)(4)			//Maximum number of clients.
#define CONTROL_PORT					(uint16_t)(2100)		//Port for control connection.
#define DATA_PORT_BASE					(uint16_t)(2101)		//Base port for data connection.
#define CONTROL_LISTEN_INDEX			(uint8_t)(0)			//Index for listening to incoming control connection socket.
#define IO_BUFFER_SIZE					(uint32_t)(1000 * 250)	//250KB of IO buffer.
#define MAX_SERVER_QUEUE				(uint8_t)(4)			//Queue capacity for server thread.
#define MAX_IO_QUEUE					(uint8_t)(4)			//Queue capacity for IO thread.
#define MAX_READ_STATE					(uint8_t)(MAX_CLIENTS)	//Number of read state.

//System UI.
#define HID_SYSTEM_UI_SEL(k)			(bool)((DEF_HID_PHY_PR((k).touch) && DEF_HID_INIT_IN((*Draw_get_bot_ui_button()), (k))) || DEF_HID_PHY_PR((k).start))
#define HID_SYSTEM_UI_CFM(k)			(bool)(((DEF_HID_PR_EM((k).touch, 1) || DEF_HID_HD((k).touch)) && DEF_HID_INIT_LAST_IN((*Draw_get_bot_ui_button()), (k))) || (DEF_HID_PR_EM((k).start, 1) || DEF_HID_HD((k).start)))
#define HID_SYSTEM_UI_DESEL(k)			(bool)(DEF_HID_PHY_NP((k).touch) && DEF_HID_PHY_NP((k).start))
//Toggle LAN address visibility.
#define HID_LAN_VISIBILITY_CFM(k)		(bool)(DEF_HID_PR_EM((k).x, 1) || DEF_HID_HD((k).x))
//Toggle WAN address visibility.
#define HID_WAN_VISIBILITY_CFM(k)		(bool)((DEF_HID_PR_EM((k).y, 1) || DEF_HID_HD((k).y)))
//Sleep policy.
#define HID_SLEEP_POLICY_SEL(k)			(bool)(DEF_HID_PHY_PR((k).touch) && DEF_HID_INIT_IN(ftpd.sleep_policy_button, (k)))
#define HID_SLEEP_POLICY_CFM(k)			(bool)((DEF_HID_PR_EM((k).touch, 1) || DEF_HID_HD((k).touch)) && DEF_HID_INIT_LAST_IN(ftpd.sleep_policy_button, (k)))
#define HID_SLEEP_POLICY_DESEL(k)		(bool)(DEF_HID_PHY_NP((k).touch))
//Read policy.
#define HID_READ_POLICY_SEL(k)			(bool)(DEF_HID_PHY_PR((k).touch) && DEF_HID_INIT_IN(ftpd.read_policy_button, (k)))
#define HID_READ_POLICY_CFM(k)			(bool)((DEF_HID_PR_EM((k).touch, 1) || DEF_HID_HD((k).touch)) && DEF_HID_INIT_LAST_IN(ftpd.read_policy_button, (k)))
#define HID_READ_POLICY_DESEL(k)		(bool)(DEF_HID_PHY_NP((k).touch))
//Write policy.
#define HID_WRITE_POLICY_SEL(k)			(bool)(DEF_HID_PHY_PR((k).touch) && DEF_HID_INIT_IN(ftpd.write_policy_button, (k)))
#define HID_WRITE_POLICY_CFM(k)			(bool)((DEF_HID_PR_EM((k).touch, 1) || DEF_HID_HD((k).touch)) && DEF_HID_INIT_LAST_IN(ftpd.write_policy_button, (k)))
#define HID_WRITE_POLICY_DESEL(k)		(bool)(DEF_HID_PHY_NP((k).touch))
//Rename policy.
#define HID_RENAME_POLICY_SEL(k)		(bool)(DEF_HID_PHY_PR((k).touch) && DEF_HID_INIT_IN(ftpd.rename_policy_button, (k)))
#define HID_RENAME_POLICY_CFM(k)		(bool)((DEF_HID_PR_EM((k).touch, 1) || DEF_HID_HD((k).touch)) && DEF_HID_INIT_LAST_IN(ftpd.rename_policy_button, (k)))
#define HID_RENAME_POLICY_DESEL(k)		(bool)(DEF_HID_PHY_NP((k).touch))
//Delete policy.
#define HID_DELETE_POLICY_SEL(k)		(bool)(DEF_HID_PHY_PR((k).touch) && DEF_HID_INIT_IN(ftpd.delete_policy_button, (k)))
#define HID_DELETE_POLICY_CFM(k)		(bool)((DEF_HID_PR_EM((k).touch, 1) || DEF_HID_HD((k).touch)) && DEF_HID_INIT_LAST_IN(ftpd.delete_policy_button, (k)))
#define HID_DELETE_POLICY_DESEL(k)		(bool)(DEF_HID_PHY_NP((k).touch))

#define CLIENT_INFO_X					(double)(5)				//X offset for client info in px.
#define CLIENT_INFO_Y					(double)(20)			//Y offset for client info in px.
#define CLIENT_INFO_SPACE_Y				(double)(0)				//Element spacing for client info (for Y direction) in px.
#define CLIENT_INFO_TITLE_HEIGHT		(double)(15)			//Element height for client title in px.
#define CLIENT_INFO_HEIGHT				(double)(50)			//Element height for client info in px.

#define SERVER_INFO_X					(double)(5)				//X offset for server info in px.
#define SERVER_INFO_Y					(double)(15)			//Y offset for server info in px.
#define SERVER_INFO_SPACE_Y				(double)(0)				//Element spacing for server info (for Y direction) in px.
#define SERVER_INFO_HEIGHT				(double)(25)			//Element height for server info in px.

#define SERVER_CONFIG_INITIAL_SPACE_Y	(double)(35)			//First element spacing for server config (for Y direction) in px.
#define SERVER_CONFIG_SPACE_Y			(double)(2.5)			//Element spacing for server config (for Y direction) in px.
#define SERVER_CONFIG_WIDTH				(double)(310)			//Element width for server config in px.
#define SERVER_CONFIG_HEIGHT			(double)(15)			//Element height for server config in px.

#define FONT_SIZE_STATUS				(float)(19.50)			//Font size for status in px.
#define FONT_SIZE_VER					(float)(12.75)			//Font size for version in px.
#define FONT_SIZE_CLIENT_INFO_TITLE		(float)(14.50)			//Font size for title of client info in px.
#define FONT_SIZE_CLIENT_INFO			(float)(13.50)			//Font size for client info in px.
#define FONT_SIZE_SERVER_INFO			(float)(14.00)			//Font size for server info in px.
#define FONT_SIZE_SERVER_CONFIG			(float)(14.00)			//Font size for server config in px.

//Typedefs.
typedef enum
{
	MSG_CLIENT_INFO_TITLE,
	MSG_CLIENT_INFO_CONTROL_CONN,
	MSG_CLIENT_INFO_DATA_CONN,
	MSG_CLIENT_INFO_RECEIVED,
	MSG_CLIENT_INFO_SENT,
	MSG_CLIENT_INFO_LAST_CMD,
	MSG_CLIENT_INFO_LAST_RES,
	MSG_CLIENT_INFO_CURRENT_DIR,
	MSG_ADDRESS_LAN_INITIALIZING,
	MSG_ADDRESS_LAN,
	MSG_ADDRESS_LAN_UNKNOWN,
	MSG_ADDRESS_WAN_INITIALIZING,
	MSG_ADDRESS_WAN,
	MSG_ADDRESS_WAN_UNKNOWN,
	MSG_CONFIG_ALLOW,
	MSG_CONFIG_ALLOW_WHEN_NO_DATA_CONN,
	MSG_CONFIG_ALLOW_WHEN_NO_CONTROL_CONN,
	MSG_CONFIG_DISALLOW,
	MSG_CONFIG_SLEEP,
	MSG_CONFIG_READ,
	MSG_CONFIG_WRITE,
	MSG_CONFIG_RENAME,
	MSG_CONFIG_DELETE,
	MSG_CONFIG_SHOW,
	MSG_CONFIG_HIDE,

	MSG_MAX,
} Ftpd_msg;

typedef enum
{
	IO_CMD_READ,		//For RETR command.
	IO_CMD_WRITE,		//For STOR/STOU/APPE commands.

	IO_CMD_FORCE_32BIT = INT32_MAX,
} Ftpd_io_cmd;

typedef enum
{
	IO_RES_READ,		//For RETR command.
	IO_RES_WRITE,		//For STOR/STOU/APPE commands.

	IO_RES_FORCE_32BIT = INT32_MAX,
} Ftpd_io_res;

typedef enum
{
	//If sleep mode is disallowed in app wide settings, these are same as SLEEP_POLICY_DISALLOW.
	SLEEP_POLICY_ALLOW,							//Always allow entering sleep mode.
	SLEEP_POLICY_ALLOW_WHEN_NO_DATA_CONN,		//Only allow entering sleep mode when there's no data connection.
	SLEEP_POLICY_ALLOW_WHEN_NO_CONTROL_CONN,	//Only allow entering sleep mode when there's no control connection.

	SLEEP_POLICY_DISALLOW,						//Never allow entering sleep mode.

	SLEEP_POLICY_MAX,
} Ftpd_sleep_policy;

DEF_LOG_ENUM_DEBUG
(
	Ftpd_sleep_policy,
	SLEEP_POLICY_ALLOW,
	SLEEP_POLICY_ALLOW_WHEN_NO_DATA_CONN,
	SLEEP_POLICY_ALLOW_WHEN_NO_CONTROL_CONN,
	SLEEP_POLICY_DISALLOW,
	SLEEP_POLICY_MAX
)

typedef enum
{
	ACCESS_POLICY_ALLOW,		//Allow.
	ACCESS_POLICY_DISALLOW,		//Disallow.

	ACCESS_POLICY_MAX,
} Ftpd_access_policy;

DEF_LOG_ENUM_DEBUG
(
	Ftpd_access_policy,
	ACCESS_POLICY_ALLOW,
	ACCESS_POLICY_DISALLOW,
	ACCESS_POLICY_MAX
)

typedef struct
{
	bool is_retrieve_started;		//Whether reading request was sent to IO thread.
	bool is_store_eof_sent;			//Whether EOF was sent to IO thread.
	uint32_t store_buffer_offset;	//Buffer offset for store buffer.
} Ftpd_client_user_data;

typedef struct
{
	bool is_used;			//Whether this entry is being used.
	uint8_t client_index;	//Index of client.
	uint32_t buffer_size;	//Buffer size.
	uint64_t offset;		//Current read offset.
	Str_data dir;			//Target directory name.
	Str_data file_name;		//Target file name.
} Ftpd_read_state;

typedef struct
{
	Ftpd_read_state read[MAX_READ_STATE];	//Read state.
} Ftpd_fs;

typedef struct
{
	uint8_t client_index;	//Index of client.
	Str_data dir;			//Target directory name.
	Str_data file_name;		//Target file name.
} Ftpd_io_cmd_read;

typedef struct
{
	bool is_append;			//Whether this data should be appended.
	bool is_eof;			//Whether this is the last packet.
	uint8_t client_index;	//Index of client.
	uint8_t* buffer;		//Buffer.
	uint32_t buffer_size;	//Buffer size.
	Str_data dir;			//Target directory name.
	Str_data file_name;		//Target file name.
} Ftpd_io_cmd_write;

typedef struct
{
	bool is_error;			//Whether there was an error.
	bool is_eof;			//Whether this is the last packet.
	uint8_t client_index;	//Index of client.
	uint8_t* buffer;		//Buffer.
	uint32_t buffer_size;	//Buffer size.
	Str_data dir;			//Target directory name.
	Str_data file_name;		//Target file name.
} Ftpd_io_res_read;

typedef struct
{
	bool is_error;			//Whether there was an error.
	uint8_t client_index;	//Index of client.
	Str_data dir;			//Target directory name.
	Str_data file_name;		//Target file name.
} Ftpd_io_res_write;

typedef struct
{
	bool is_active_mode;					//Whether data connection uses active connection mode.
	uint64_t sent;							//Sent size (for current data connection) in bytes.
	uint64_t received;						//Received size (for current data connection) in bytes.
	Ftp_command last_cmd;					//Most recently received command.
	Str_data last_arg;						//Most recently received arg.
	Str_data last_res;						//Most recently sent response message.
	Str_data dir;							//Current working directory.
	struct sockaddr_in control_address;		//Control connection address.
	struct sockaddr_in data_address;		//Data connection address.
} Ftpd_ui_client_data;

typedef struct
{
	bool is_lan_unknown;						//Whether local IP address is unknown.
	bool is_wan_unknown;						//Whether global IP address is unknown.
	uint8_t max_clients;						//Maximum FTP clients.
	uint8_t connected_clients;					//Active FTP clients.
	uint16_t control_port;						//Listening port for incoming control connections.
	uint32_t lan_ip;							//Server's local IP address.
	uint32_t wan_ip;							//Server's global IP address.
	Ftpd_ui_client_data clients[MAX_CLIENTS];	//Clients data.
} Ftpd_ui_data;

typedef struct
{
	Ftpd_sleep_policy sleep_policy;			//Sleep policy when server is open.
	Ftpd_access_policy read_policy;			//Access policy for RETR.
	Ftpd_access_policy write_policy;		//Access policy for STOR/STOU/APPE.
	Ftpd_access_policy rename_policy;		//Access policy for RNFR/RNTO.
	Ftpd_access_policy delete_policy;		//Access policy for RMD/DELE.
} Ftpd_config;

typedef struct
{
	bool is_lan_address_displayed;			//Whether LAN address is displayed.
	bool is_wan_address_displayed;			//Whether WAN address is displayed.
} Ftpd_ram_config;

typedef struct
{
	//Global.
	bool inited;							//Whether this app is initialized.
	bool main_run;							//Whether this app should run.
	bool thread_run;						//Whether worker threads should run.
	bool thread_suspend;					//Whether worker threads should be suspended.
	Str_data ftpd_status;					//Status message.

	//Settings (saved to NVS).
	Ftpd_config config_hid;					//Actual config updated by hid thread.
	Ftpd_config config_copy_draw;			//Copy of config used by rendering thread.
	Ftpd_config config_copy_server;			//Copy of config used by server thread.

	//Settings (saved to RAM).
	Ftpd_ram_config ram_config_hid;			//Actual config updated by hid thread.
	Ftpd_ram_config ram_config_copy_draw;	//Copy of config used by rendering thread.

	//UI data.
	Ftpd_ui_data ui_data_server;			//Actual UI data updated by server thread.
	Ftpd_ui_data ui_data_copy_draw;			//Copy of UI data used by rendering thread.

	//Server data.
	Ftp_clients clients;					//Connected clients data.

	//Buttons.
	Draw_image_data sleep_policy_button, read_policy_button,
	write_policy_button, rename_policy_button, delete_policy_button;

	//Threads and queues.
	Thread server_thread;					//Server thread.
	Queue_data server_thread_queue;			//Server thread response queue.

	Thread io_thread;						//IO thread.
	Queue_data io_thread_queue;				//IO thread command queue.

	Thread init_thread;						//Init thread handle.
	Thread exit_thread;						//Exit thread handle.

	//Mutexs.
	Sync_data mutex;						//Mutex for UI data and config.
} Ftpd;

//Prototypes.
static void Ftpd_draw_init_exit_message(void);
static void Ftpd_init_thread(void* arg);
static void Ftpd_exit_thread(void* arg);
static uint32_t Ftpd_load_settings(void);
static uint32_t Ftpd_save_settings(void);
static void Ftpd_log_settings(void);
static void Ftpd_get_local_ip(uint32_t* ip);
static void Ftpd_get_global_ip(uint32_t* ip);
static uint32_t Ftpd_client_user_data_init(Ftp_client* client);
static uint32_t Ftpd_io_init(Ftpd_fs* fs, uint32_t read_buffer_size);
static uint32_t Ftpd_read_state_init(Ftpd_read_state* read_state, uint32_t buffer_size);
static uint32_t Ftpd_io_cmd_read_init(Ftpd_io_cmd_read* read, uint8_t client_index, const char* file_name, const char* dir);
static uint32_t Ftpd_io_cmd_write_init(Ftpd_io_cmd_write* write, bool is_append, bool is_eof, uint8_t client_index, const uint8_t* buffer, uint32_t buffer_size, const char* file_name, const char* dir);
static uint32_t Ftpd_io_res_read_init(Ftpd_io_res_read* read, bool is_error, bool is_eof, uint8_t client_index, const uint8_t* buffer, uint32_t buffer_size, const char* file_name, const char* dir);
static uint32_t Ftpd_io_res_write_init(Ftpd_io_res_write* write, bool is_error, uint8_t client_index, const char* file_name, const char* dir);
static uint32_t Ftpd_ui_data_init(Ftpd_ui_data* ui_data);
static uint32_t Ftpd_config_init(Ftpd_config* config);
static uint32_t Ftpd_ram_config_init(Ftpd_ram_config* ram_config);
static uint32_t Ftpd_ui_client_data_init(Ftpd_ui_client_data* ui_data);
static void Ftpd_client_user_data_clear(Ftp_client* client);
static void Ftpd_io_clear(Ftpd_fs* fs);
static void Ftpd_read_state_clear(Ftpd_read_state* read_state);
static void Ftpd_ui_data_copy(Ftpd_ui_data* ui_data_dst, const Ftpd_ui_data* ui_data_src);
static void Ftpd_config_copy(Ftpd_config* config_dst, const Ftpd_config* config_src);
static void Ftpd_ram_config_copy(Ftpd_ram_config* ram_config_dst, const Ftpd_ram_config* ram_config_src);
static void Ftpd_ui_client_data_copy(Ftpd_ui_client_data* ui_data_dst, const Ftpd_ui_client_data* ui_data_src);
static void Ftpd_client_user_data_exit(Ftp_client* client);
static void Ftpd_io_exit(Ftpd_fs* fs);
static void Ftpd_read_state_exit(Ftpd_read_state* read_state);
static void Ftpd_io_cmd_read_exit(Ftpd_io_cmd_read* read);
static void Ftpd_io_cmd_write_exit(Ftpd_io_cmd_write* write);
static void Ftpd_io_res_read_exit(Ftpd_io_res_read* read);
static void Ftpd_io_res_write_exit(Ftpd_io_res_write* write);
static void Ftpd_ui_data_exit(Ftpd_ui_data* ui_data);
static void Ftpd_config_exit(Ftpd_config* config);
static void Ftpd_ram_config_exit(Ftpd_ram_config* ram_config);
static void Ftpd_ui_client_data_exit(Ftpd_ui_client_data* ui_data);
static void Ftpd_reject(Ftp_client* client);
static void Ftpd_process_retr_bof(Ftp_client* client, uint8_t client_index);
static void Ftpd_process_stor_stou_appe_data(Ftp_client* client, uint8_t client_index);
static uint32_t Ftpd_process_control_connection(Ftp_clients* clients, uint8_t client_index);
static uint32_t Ftpd_process_bof(Ftp_clients* clients, uint8_t client_index);
static uint32_t Ftpd_process_eof(Ftp_clients* clients, uint8_t client_index);
static uint32_t Ftpd_process_data_connection(Ftp_clients* clients, uint8_t client_index);
static void Ftpd_process_res_read(Ftp_clients* clients, Ftpd_io_res_read* res_read);
static void Ftpd_process_res_write(Ftp_clients* clients, Ftpd_io_res_write* res_write);
static void Ftpd_server_thread(void* arg);
static void Ftpd_io_process_read(Ftpd_fs* fs);
static void Ftpd_io_process_cmd_read(Ftpd_fs* fs, Ftpd_io_cmd_read* cmd_read);
static void Ftpd_io_process_cmd_write(Ftpd_fs* fs, Ftpd_io_cmd_write* cmd_write);
static void Ftpd_io_thread(void* arg);

//Variables.
static const char* ftpd_msg_keys[MSG_MAX] =
{
	"client_info.title",
	"client_info.control_conn",
	"client_info.data_conn",
	"client_info.received",
	"client_info.sent",
	"client_info.last_cmd",
	"client_info.last_res",
	"client_info.current_dir",
	"address.lan_initializing",
	"address.lan",
	"address.lan_unknown",
	"address.wan_initializing",
	"address.wan",
	"address.wan_unknown",
	"config.allow",
	"config.allow_when_no_data_conn",
	"config.allow_when_no_control_conn",
	"config.disallow",
	"config.sleep",
	"config.read",
	"config.write",
	"config.rename",
	"config.delete",
	"config.show",
	"config.hide",
};
static const char* ftpd_settings_key[] =
{
	"sleep_policy",
	"read_policy",
	"write_policy",
	"rename_policy",
	"delete_policy",
};
static Str_data ftpd_msg[MSG_MAX] = { 0, };
static Ftpd ftpd = { 0, };

//Code.
bool Ftpd_query_init_flag(void)
{
	return ftpd.inited;
}

bool Ftpd_query_running_flag(void)
{
	return ftpd.main_run;
}

void Ftpd_hid(Hid_info* key)
{
	Sem_config config = { 0, };

	//Do nothing if app is suspended.
	if(aptShouldJumpToHome())
		return;

	Sem_get_config(&config);

	if(Util_err_query_show_flag())
		Util_err_main(key);
	else if(Util_expl_query_show_flag())
		Util_expl_main(key, config.scroll_speed);
	else
	{
		//Notify user that button is being pressed.
		if(HID_SYSTEM_UI_SEL(*key))
			Draw_get_bot_ui_button()->selected = true;
		if(HID_SLEEP_POLICY_SEL(*key))
			ftpd.sleep_policy_button.selected = true;
		if(HID_READ_POLICY_SEL(*key))
			ftpd.read_policy_button.selected = true;
		if(HID_WRITE_POLICY_SEL(*key))
			ftpd.write_policy_button.selected = true;
		if(HID_RENAME_POLICY_SEL(*key))
			ftpd.rename_policy_button.selected = true;
		if(HID_DELETE_POLICY_SEL(*key))
			ftpd.delete_policy_button.selected = true;

		Util_sync_lock(&ftpd.mutex, UINT64_MAX);
		//Execute functions if conditions are satisfied.
		if(HID_SYSTEM_UI_CFM(*key))
			Ftpd_suspend();
		else if(HID_LAN_VISIBILITY_CFM(*key))
			ftpd.ram_config_hid.is_lan_address_displayed = !ftpd.ram_config_hid.is_lan_address_displayed;
		else if(HID_WAN_VISIBILITY_CFM(*key))
			ftpd.ram_config_hid.is_wan_address_displayed = !ftpd.ram_config_hid.is_wan_address_displayed;
		else if(HID_SLEEP_POLICY_CFM(*key))
		{
			if((ftpd.config_hid.sleep_policy + 1) < SLEEP_POLICY_MAX)
				ftpd.config_hid.sleep_policy++;
			else
				ftpd.config_hid.sleep_policy = SLEEP_POLICY_ALLOW;
		}
		else if(HID_READ_POLICY_CFM(*key))
			ftpd.config_hid.read_policy = (ftpd.config_hid.read_policy == ACCESS_POLICY_ALLOW ? ACCESS_POLICY_DISALLOW : ACCESS_POLICY_ALLOW);
		else if(HID_WRITE_POLICY_CFM(*key))
			ftpd.config_hid.write_policy = (ftpd.config_hid.write_policy == ACCESS_POLICY_ALLOW ? ACCESS_POLICY_DISALLOW : ACCESS_POLICY_ALLOW);
		else if(HID_RENAME_POLICY_CFM(*key))
			ftpd.config_hid.rename_policy = (ftpd.config_hid.rename_policy == ACCESS_POLICY_ALLOW ? ACCESS_POLICY_DISALLOW : ACCESS_POLICY_ALLOW);
		else if(HID_DELETE_POLICY_CFM(*key))
			ftpd.config_hid.delete_policy = (ftpd.config_hid.delete_policy == ACCESS_POLICY_ALLOW ? ACCESS_POLICY_DISALLOW : ACCESS_POLICY_ALLOW);

		Util_sync_unlock(&ftpd.mutex);
	}

	//Notify user that button is NOT being pressed anymore.
	if(HID_SYSTEM_UI_DESEL(*key))
		Draw_get_bot_ui_button()->selected = false;
	if(HID_SLEEP_POLICY_DESEL(*key))
		ftpd.sleep_policy_button.selected = false;
	if(HID_READ_POLICY_DESEL(*key))
		ftpd.read_policy_button.selected = false;
	if(HID_WRITE_POLICY_DESEL(*key))
		ftpd.write_policy_button.selected = false;
	if(HID_RENAME_POLICY_DESEL(*key))
		ftpd.rename_policy_button.selected = false;
	if(HID_DELETE_POLICY_DESEL(*key))
		ftpd.delete_policy_button.selected = false;

	if(Util_log_query_show_flag())
		Util_log_main(key);
}

void Ftpd_resume(void)
{
	ftpd.thread_suspend = false;
	ftpd.main_run = true;
	//Reset key state on scene change.
	Util_hid_reset_key_state(HID_KEY_BIT_ALL);
	Draw_set_refresh_needed(true);
	Menu_suspend();
}

void Ftpd_suspend(void)
{
	ftpd.thread_suspend = true;
	ftpd.main_run = false;
	Draw_set_refresh_needed(true);
	Menu_resume();
}

uint32_t Ftpd_load_msg(const char* lang)
{
	char file_name[32] = { 0, };
	uint8_t* buffer = NULL;
	uint32_t read_size = 0;
	uint32_t result = DEF_ERR_OTHER;

	snprintf(file_name, sizeof(file_name), "ftpd_%s.json", (lang ? lang : ""));
	result = Util_file_load_from_rom(file_name, "romfs:/gfx/msg/", &buffer, 0x2000, &read_size);
	if(result == DEF_SUCCESS)
	{
		result = Util_load_json_msg((char*)buffer, ftpd_msg_keys, ftpd_msg, MSG_MAX);
		if(result != DEF_SUCCESS)
			DEF_LOG_RESULT(Util_load_json_msg, false, result);
	}
	else
		DEF_LOG_RESULT(Util_file_load_from_rom, false, result);

	free(buffer);
	buffer = NULL;
	return result;
}

void Ftpd_init(bool draw)
{
	DEF_LOG_STRING("Initializing...");
	uint32_t result = DEF_ERR_OTHER;
	Sem_state state = { 0, };

	//Reset everything first.
	memset(&ftpd, 0x00, sizeof(Ftpd));

	Sem_get_state(&state);
	DEF_LOG_RESULT_SMART(result, Util_str_init(&ftpd.ftpd_status), (result == DEF_SUCCESS), result);

	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ftpd_status.sequential_id, sizeof(ftpd.ftpd_status.sequential_id));

	if(DEF_SEM_MODEL_IS_NEW(state.console_model) && Util_is_core_available(2))
		ftpd.init_thread = threadCreate(Ftpd_init_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 2, false);
	else
	{
		APT_SetAppCpuTimeLimit(80);
		ftpd.init_thread = threadCreate(Ftpd_init_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);
	}

	while(!ftpd.inited)
	{
		if(draw)
			Ftpd_draw_init_exit_message();
		else
			Util_sleep(20000);
	}

	if(!DEF_SEM_MODEL_IS_NEW(state.console_model) || !Util_is_core_available(2))
		APT_SetAppCpuTimeLimit(10);

	DEF_LOG_RESULT_SMART(result, threadJoin(ftpd.init_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	threadFree(ftpd.init_thread);

	Util_str_clear(&ftpd.ftpd_status);
	Ftpd_resume();

	DEF_LOG_STRING("Initialized.");
}

void Ftpd_exit(bool draw)
{
	DEF_LOG_STRING("Exiting...");
	uint32_t result = DEF_ERR_OTHER;

	ftpd.exit_thread = threadCreate(Ftpd_exit_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_NORMAL, 1, false);

	while(ftpd.inited)
	{
		if(draw)
			Ftpd_draw_init_exit_message();
		else
			Util_sleep(20000);
	}

	DEF_LOG_RESULT_SMART(result, threadJoin(ftpd.exit_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	threadFree(ftpd.exit_thread);

	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ftpd_status.sequential_id);
	Util_str_free(&ftpd.ftpd_status);
	Draw_set_refresh_needed(true);

	DEF_LOG_STRING("Exited.");
}

void Ftpd_main(void)
{
	uint32_t color = DEF_DRAW_BLACK;
	uint32_t back_color = DEF_DRAW_WHITE;
	double x_offset = 0;
	double y_offset = 0;
	Watch_handle_bit watch_handle_bit = (DEF_WATCH_HANDLE_BIT_GLOBAL | DEF_WATCH_HANDLE_BIT_FTP_DAEMON);
	Sem_config config = { 0, };
	Sem_state state = { 0, };

	if(Util_err_query_show_flag())
		watch_handle_bit |= DEF_WATCH_HANDLE_BIT_ERR;
	if(Util_expl_query_show_flag())
		watch_handle_bit |= DEF_WATCH_HANDLE_BIT_EXPL;
	if(Util_log_query_show_flag())
		watch_handle_bit |= DEF_WATCH_HANDLE_BIT_LOG;

	Sem_get_config(&config);
	Sem_get_state(&state);

	if (config.is_night)
	{
		color = DEF_DRAW_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	//Check if we should update the screen.
	if(Util_watch_is_changed(watch_handle_bit) || Draw_is_refresh_needed() || !config.is_eco)
	{
		Str_data temp_text = { 0, };

		Util_str_init(&temp_text);

		//Copy UI data and config.
		Util_sync_lock(&ftpd.mutex, UINT64_MAX);
		Ftpd_ui_data_copy(&ftpd.ui_data_copy_draw, &ftpd.ui_data_server);
		Ftpd_config_copy(&ftpd.config_copy_draw, &ftpd.config_hid);
		Ftpd_ram_config_copy(&ftpd.ram_config_copy_draw, &ftpd.ram_config_hid);
		Util_sync_unlock(&ftpd.mutex);

		Draw_set_refresh_needed(false);
		Draw_frame_ready();

		if(config.is_top_lcd_on)
		{
			Draw_screen_ready(DRAW_SCREEN_TOP_LEFT, back_color);

			//Title.
			{
				x_offset = CLIENT_INFO_X;
				y_offset = CLIENT_INFO_Y;
				Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_TITLE]), ftpd.ui_data_copy_draw.connected_clients, ftpd.ui_data_copy_draw.max_clients);
				Draw(&temp_text, x_offset, y_offset, FONT_SIZE_CLIENT_INFO_TITLE, color);
				y_offset += (CLIENT_INFO_TITLE_HEIGHT + CLIENT_INFO_SPACE_Y);
			}

			//Connected clients.
			{
				for(uint8_t i = 0; i < MAX_CLIENTS; i++)
				{
					const Ftpd_ui_client_data* ui_data = &ftpd.ui_data_copy_draw.clients[i];

					if(ui_data->control_address.sin_addr.s_addr != 0 && ui_data->control_address.sin_port != 0)
					{
						Str_data size_str = { 0, };

						//Control connection info.
						Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_CONTROL_CONN]),
						inet_ntoa(ui_data->control_address.sin_addr), ui_data->control_address.sin_port);

						//Data connection info.
						if(ui_data->data_address.sin_addr.s_addr != 0 && ui_data->data_address.sin_port != 0)
						{
							const char* mode = (ui_data->is_active_mode ? "Active" : "Passive");

							Util_str_format_append(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_DATA_CONN]),
							inet_ntoa(ui_data->data_address.sin_addr), ui_data->data_address.sin_port, mode);
						}
						else
							Util_str_add(&temp_text, "\n");

						//Most recent command, response.
						Util_str_format_append(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_LAST_CMD]), Ftp_command_get_name(ui_data->last_cmd), DEF_STR_NEVER_NULL(&ui_data->last_arg));
						Util_str_format_append(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_LAST_RES]), DEF_STR_NEVER_NULL(&ui_data->last_res));

						//Sent or received size depending on state.
						if(ui_data->last_cmd == FTP_COMMAND_STOR || ui_data->last_cmd == FTP_COMMAND_STRU || ui_data->last_cmd == FTP_COMMAND_APPE)
						{
							//Received size.
							Util_format_size(ui_data->received, &size_str);
							Util_str_format_append(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_RECEIVED]), DEF_STR_NEVER_NULL(&size_str));
						}
						else if(ui_data->last_cmd == FTP_COMMAND_RETR)
						{
							//Sent size.
							Util_format_size(ui_data->sent, &size_str);
							Util_str_format_append(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_SENT]), DEF_STR_NEVER_NULL(&size_str));
						}
						else
							Util_str_add(&temp_text, "\n");

						//Current directory.
						Util_str_format_append(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CLIENT_INFO_CURRENT_DIR]), DEF_STR_NEVER_NULL(&ui_data->dir));

						Draw_with_scale(&temp_text, x_offset, y_offset, FONT_SIZE_CLIENT_INFO, DEF_DRAW_NORMAL_SCALE_AND_COMPACT, color,
						DRAW_X_ALIGN_LEFT, DRAW_Y_ALIGN_TOP, 0, 0, DRAW_BACKGROUND_NONE, NULL, DEF_DRAW_NO_COLOR);
						y_offset += (CLIENT_INFO_HEIGHT + CLIENT_INFO_SPACE_Y);

						Util_str_free(&size_str);
					}
				}
			}

			if(Util_log_query_show_flag())
				Util_log_draw();

			Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

			if(config.is_debug)
				Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

			if(Util_cpu_usage_query_show_flag())
				Util_cpu_usage_draw();

			if(Util_gpu_usage_query_show_flag())
				Util_gpu_usage_draw();

			if(Util_net_usage_query_show_flag())
				Util_net_usage_draw();

			if(Util_nvs_usage_query_show_flag())
				Util_nvs_usage_draw();

			if(Util_ram_usage_query_show_flag())
				Util_ram_usage_draw();
		}

		if(config.is_bottom_lcd_on)
		{
			Draw_screen_ready(DRAW_SCREEN_BOTTOM, back_color);

			Draw_c(DEF_FTPD_VER, 0, 0, FONT_SIZE_VER, DEF_DRAW_GREEN);

			//Server info.
			{
				const char* addr_text = NULL;
				struct in_addr address = { 0, };

				//Local address.
				x_offset = SERVER_INFO_X;
				y_offset = SERVER_INFO_Y;
				if(ftpd.ui_data_copy_draw.is_lan_unknown)
					Util_str_set(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_ADDRESS_LAN_UNKNOWN]));
				else if(ftpd.ui_data_copy_draw.lan_ip != 0)
				{
					Ftpd_msg message_id = (ftpd.ram_config_copy_draw.is_lan_address_displayed ? MSG_CONFIG_HIDE : MSG_CONFIG_SHOW);

					address.s_addr = ftpd.ui_data_copy_draw.lan_ip;
					addr_text = (ftpd.ram_config_copy_draw.is_lan_address_displayed ? inet_ntoa(address) : "xxx.xxx.xxx.xxx");

					Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_ADDRESS_LAN]), addr_text,
					ftpd.ui_data_copy_draw.control_port, DEF_STR_NEVER_NULL(&ftpd_msg[message_id]), "[X]");
				}
				else
					Util_str_set(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_ADDRESS_LAN_INITIALIZING]));

				//Global address.
				if(ftpd.ui_data_copy_draw.is_wan_unknown)
					Util_str_add(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_ADDRESS_WAN_UNKNOWN]));
				else if(ftpd.ui_data_copy_draw.wan_ip != 0)
				{
					Ftpd_msg message_id = (ftpd.ram_config_copy_draw.is_wan_address_displayed ? MSG_CONFIG_HIDE : MSG_CONFIG_SHOW);

					address.s_addr = ftpd.ui_data_copy_draw.wan_ip;
					addr_text = (ftpd.ram_config_copy_draw.is_wan_address_displayed ? inet_ntoa(address) : "xxx.xxx.xxx.xxx");

					Util_str_format_append(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_ADDRESS_WAN]), addr_text,
					ftpd.ui_data_copy_draw.control_port, DEF_STR_NEVER_NULL(&ftpd_msg[message_id]), "[Y]");
				}
				else
					Util_str_add(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_ADDRESS_WAN_INITIALIZING]));

				Draw(&temp_text, x_offset, y_offset, FONT_SIZE_SERVER_INFO, DEF_DRAW_RED);
				y_offset += (SERVER_INFO_HEIGHT + SERVER_INFO_SPACE_Y);
			}

			//Sleep policy.
			{
				uint32_t temp_back_color = (ftpd.sleep_policy_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN);
				Ftpd_msg message_id = MSG_CONFIG_DISALLOW;

				if(ftpd.config_copy_draw.sleep_policy == SLEEP_POLICY_ALLOW)
					message_id = MSG_CONFIG_ALLOW;
				else if(ftpd.config_copy_draw.sleep_policy == SLEEP_POLICY_ALLOW_WHEN_NO_DATA_CONN)
					message_id = MSG_CONFIG_ALLOW_WHEN_NO_DATA_CONN;
				else if(ftpd.config_copy_draw.sleep_policy == SLEEP_POLICY_ALLOW_WHEN_NO_CONTROL_CONN)
					message_id = MSG_CONFIG_ALLOW_WHEN_NO_CONTROL_CONN;

				y_offset += SERVER_CONFIG_INITIAL_SPACE_Y;
				Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CONFIG_SLEEP]), DEF_STR_NEVER_NULL(&ftpd_msg[message_id]));
				Draw_with_background(&temp_text, x_offset, y_offset, FONT_SIZE_SERVER_CONFIG, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER,
				SERVER_CONFIG_WIDTH, SERVER_CONFIG_HEIGHT, DRAW_BACKGROUND_ENTIRE_BOX_CROP, &ftpd.sleep_policy_button, temp_back_color);
				y_offset += (SERVER_CONFIG_HEIGHT + SERVER_CONFIG_SPACE_Y);
			}

			//Read policy.
			{
				uint32_t temp_back_color = (ftpd.read_policy_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN);
				Ftpd_msg message_id = MSG_CONFIG_DISALLOW;

				if(ftpd.config_copy_draw.read_policy == ACCESS_POLICY_ALLOW)
					message_id = MSG_CONFIG_ALLOW;

				Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CONFIG_READ]), DEF_STR_NEVER_NULL(&ftpd_msg[message_id]));
				Draw_with_background(&temp_text, x_offset, y_offset, FONT_SIZE_SERVER_CONFIG, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER,
				SERVER_CONFIG_WIDTH, SERVER_CONFIG_HEIGHT, DRAW_BACKGROUND_ENTIRE_BOX_CROP, &ftpd.read_policy_button, temp_back_color);
				y_offset += (SERVER_CONFIG_HEIGHT + SERVER_CONFIG_SPACE_Y);
			}

			//Write policy.
			{
				uint32_t temp_back_color = (ftpd.write_policy_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN);
				Ftpd_msg message_id = MSG_CONFIG_DISALLOW;

				if(ftpd.config_copy_draw.write_policy == ACCESS_POLICY_ALLOW)
					message_id = MSG_CONFIG_ALLOW;

				Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CONFIG_WRITE]), DEF_STR_NEVER_NULL(&ftpd_msg[message_id]));
				Draw_with_background(&temp_text, x_offset, y_offset, FONT_SIZE_SERVER_CONFIG, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER,
				SERVER_CONFIG_WIDTH, SERVER_CONFIG_HEIGHT, DRAW_BACKGROUND_ENTIRE_BOX_CROP, &ftpd.write_policy_button, temp_back_color);
				y_offset += (SERVER_CONFIG_HEIGHT + SERVER_CONFIG_SPACE_Y);
			}

			//Rename policy.
			{
				uint32_t temp_back_color = (ftpd.rename_policy_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN);
				Ftpd_msg message_id = MSG_CONFIG_DISALLOW;

				if(ftpd.config_copy_draw.rename_policy == ACCESS_POLICY_ALLOW)
					message_id = MSG_CONFIG_ALLOW;

				Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CONFIG_RENAME]), DEF_STR_NEVER_NULL(&ftpd_msg[message_id]));
				Draw_with_background(&temp_text, x_offset, y_offset, FONT_SIZE_SERVER_CONFIG, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER,
				SERVER_CONFIG_WIDTH, SERVER_CONFIG_HEIGHT, DRAW_BACKGROUND_ENTIRE_BOX_CROP, &ftpd.rename_policy_button, temp_back_color);
				y_offset += (SERVER_CONFIG_HEIGHT + SERVER_CONFIG_SPACE_Y);
			}

			//Delete policy.
			{
				uint32_t temp_back_color = (ftpd.delete_policy_button.selected ? DEF_DRAW_GREEN : DEF_DRAW_WEAK_GREEN);
				Ftpd_msg message_id = MSG_CONFIG_DISALLOW;

				if(ftpd.config_copy_draw.delete_policy == ACCESS_POLICY_ALLOW)
					message_id = MSG_CONFIG_ALLOW;

				Util_str_format(&temp_text, DEF_STR_NEVER_NULL(&ftpd_msg[MSG_CONFIG_DELETE]), DEF_STR_NEVER_NULL(&ftpd_msg[message_id]));
				Draw_with_background(&temp_text, x_offset, y_offset, FONT_SIZE_SERVER_CONFIG, color, DRAW_X_ALIGN_CENTER, DRAW_Y_ALIGN_CENTER,
				SERVER_CONFIG_WIDTH, SERVER_CONFIG_HEIGHT, DRAW_BACKGROUND_ENTIRE_BOX_CROP, &ftpd.delete_policy_button, temp_back_color);
				y_offset += (SERVER_CONFIG_HEIGHT + SERVER_CONFIG_SPACE_Y);
			}

			if(Util_expl_query_show_flag())
				Util_expl_draw();

			if(Util_err_query_show_flag())
				Util_err_draw();

			Draw_bot_ui();
		}

		Draw_apply_draw();

		Util_str_free(&temp_text);
	}
	else
		gspWaitForVBlank();
}

static void Ftpd_draw_init_exit_message(void)
{
	uint32_t color = DEF_DRAW_BLACK;
	uint32_t back_color = DEF_DRAW_WHITE;
	Watch_handle_bit watch_handle_bit = (DEF_WATCH_HANDLE_BIT_GLOBAL | DEF_WATCH_HANDLE_BIT_FTP_DAEMON);
	Sem_config config = { 0, };
	Sem_state state = { 0, };

	Sem_get_config(&config);
	Sem_get_state(&state);

	if (config.is_night)
	{
		color = DEF_DRAW_WHITE;
		back_color = DEF_DRAW_BLACK;
	}

	//Check if we should update the screen.
	if(Util_watch_is_changed(watch_handle_bit) || Draw_is_refresh_needed() || !config.is_eco)
	{
		Draw_set_refresh_needed(false);
		Draw_frame_ready();

		Draw_screen_ready(DRAW_SCREEN_TOP_LEFT, back_color);

		if(Util_log_query_show_flag())
			Util_log_draw();

		Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

		if(config.is_debug)
			Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

		if(Util_cpu_usage_query_show_flag())
			Util_cpu_usage_draw();

		if(Util_gpu_usage_query_show_flag())
			Util_gpu_usage_draw();

		if(Util_net_usage_query_show_flag())
			Util_net_usage_draw();

		if(Util_nvs_usage_query_show_flag())
			Util_nvs_usage_draw();

		if(Util_ram_usage_query_show_flag())
			Util_ram_usage_draw();

		Draw(&ftpd.ftpd_status, 0, 20, FONT_SIZE_STATUS, color);

		//Draw the same things on right screen if 3D mode is enabled.
		//So that user can easily see them.
		if(Draw_is_3d_mode())
		{
			Draw_screen_ready(DRAW_SCREEN_TOP_RIGHT, back_color);

			if(Util_log_query_show_flag())
				Util_log_draw();

			Draw_top_ui(config.is_eco, state.is_charging, state.wifi_signal, state.battery_level, state.msg);

			if(config.is_debug)
				Draw_debug_info(config.is_night, state.free_ram, state.free_linear_ram);

			if(Util_cpu_usage_query_show_flag())
				Util_cpu_usage_draw();

			if(Util_gpu_usage_query_show_flag())
				Util_gpu_usage_draw();

			if(Util_net_usage_query_show_flag())
				Util_net_usage_draw();

			if(Util_nvs_usage_query_show_flag())
				Util_nvs_usage_draw();

			if(Util_ram_usage_query_show_flag())
				Util_ram_usage_draw();

			Draw(&ftpd.ftpd_status, 0, 20, FONT_SIZE_STATUS, color);
		}

		Draw_apply_draw();
	}
	else
		gspWaitForVBlank();
}

static void Ftpd_init_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;
	Sem_state state = { 0, };

	Sem_get_state(&state);

	Util_str_set(&ftpd.ftpd_status, "Initializing variables...");
	DEF_LOG_RESULT_SMART(result, Util_sync_create(&ftpd.mutex, SYNC_TYPE_NON_RECURSIVE_MUTEX), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Ftpd_ui_data_init(&ftpd.ui_data_server), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Ftpd_ui_data_init(&ftpd.ui_data_copy_draw), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Ftpd_config_init(&ftpd.config_hid), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Ftpd_config_init(&ftpd.config_copy_draw), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Ftpd_config_init(&ftpd.config_copy_server), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Ftpd_ram_config_init(&ftpd.ram_config_hid), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Ftpd_ram_config_init(&ftpd.ram_config_copy_draw), (result == DEF_SUCCESS), result);

	ftpd.sleep_policy_button = Draw_get_empty_image();
	ftpd.read_policy_button = Draw_get_empty_image();
	ftpd.write_policy_button = Draw_get_empty_image();
	ftpd.rename_policy_button = Draw_get_empty_image();
	ftpd.delete_policy_button = Draw_get_empty_image();

	for(uint8_t i = 0; i < MAX_CLIENTS; i++)
	{
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].is_active_mode, sizeof(ftpd.ui_data_server.clients[i].is_active_mode));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].sent, sizeof(ftpd.ui_data_server.clients[i].sent));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].received, sizeof(ftpd.ui_data_server.clients[i].received));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].last_cmd, sizeof(ftpd.ui_data_server.clients[i].last_cmd));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].last_arg.sequential_id, sizeof(ftpd.ui_data_server.clients[i].last_arg.sequential_id));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].last_res.sequential_id, sizeof(ftpd.ui_data_server.clients[i].last_res.sequential_id));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].dir.sequential_id, sizeof(ftpd.ui_data_server.clients[i].dir.sequential_id));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].control_address, sizeof(ftpd.ui_data_server.clients[i].control_address));
		Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].data_address, sizeof(ftpd.ui_data_server.clients[i].data_address));
	}
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.is_lan_unknown, sizeof(ftpd.ui_data_server.is_lan_unknown));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.is_wan_unknown, sizeof(ftpd.ui_data_server.is_wan_unknown));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.max_clients, sizeof(ftpd.ui_data_server.max_clients));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.connected_clients, sizeof(ftpd.ui_data_server.connected_clients));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.control_port, sizeof(ftpd.ui_data_server.control_port));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.lan_ip, sizeof(ftpd.ui_data_server.lan_ip));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.wan_ip, sizeof(ftpd.ui_data_server.wan_ip));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.sleep_policy, sizeof(ftpd.config_hid.sleep_policy));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.read_policy, sizeof(ftpd.config_hid.read_policy));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.write_policy, sizeof(ftpd.config_hid.write_policy));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.rename_policy, sizeof(ftpd.config_hid.rename_policy));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.delete_policy, sizeof(ftpd.config_hid.delete_policy));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ram_config_hid.is_lan_address_displayed, sizeof(ftpd.ram_config_hid.is_lan_address_displayed));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.ram_config_hid.is_wan_address_displayed, sizeof(ftpd.ram_config_hid.is_wan_address_displayed));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.sleep_policy_button.selected, sizeof(ftpd.sleep_policy_button.selected));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.read_policy_button.selected, sizeof(ftpd.read_policy_button.selected));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.write_policy_button.selected, sizeof(ftpd.write_policy_button.selected));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.rename_policy_button.selected, sizeof(ftpd.rename_policy_button.selected));
	Util_watch_add(WATCH_HANDLE_FTP_DAEMON, &ftpd.delete_policy_button.selected, sizeof(ftpd.delete_policy_button.selected));

	Util_str_add(&ftpd.ftpd_status, "\nInitializing queue...");
	DEF_LOG_RESULT_SMART(result, Util_queue_create(&ftpd.server_thread_queue, MAX_SERVER_QUEUE), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, Util_queue_create(&ftpd.io_thread_queue, MAX_IO_QUEUE), (result == DEF_SUCCESS), result);

	Util_str_add(&ftpd.ftpd_status, "\nLoading settings...");
	DEF_LOG_RESULT_SMART(result, Ftpd_load_settings(), (result == DEF_SUCCESS), result);

	Util_str_add(&ftpd.ftpd_status, "\nStarting threads...");
	ftpd.thread_run = true;

	if(DEF_SEM_MODEL_IS_NEW(state.console_model) && Util_is_core_available(2))
	{
		ftpd.server_thread = threadCreate(Ftpd_server_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_ABOVE_NORMAL, 2, false);
		ftpd.io_thread = threadCreate(Ftpd_io_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_ABOVE_NORMAL, 2, false);
	}
	else
	{
		ftpd.server_thread = threadCreate(Ftpd_server_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_ABOVE_NORMAL, 0, false);
		ftpd.io_thread = threadCreate(Ftpd_io_thread, NULL, DEF_THREAD_STACKSIZE, DEF_THREAD_PRIORITY_ABOVE_NORMAL, 1, false);
	}

	ftpd.inited = true;

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

static void Ftpd_exit_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;

	ftpd.thread_suspend = false;
	ftpd.thread_run = false;

	Util_str_set(&ftpd.ftpd_status, "Exiting threads...");
	DEF_LOG_RESULT_SMART(result, threadJoin(ftpd.server_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);
	DEF_LOG_RESULT_SMART(result, threadJoin(ftpd.io_thread, DEF_THREAD_WAIT_TIME), (result == DEF_SUCCESS), result);

	Util_str_add(&ftpd.ftpd_status, "\nSaving settings...");
	DEF_LOG_RESULT_SMART(result, Ftpd_save_settings(), (result == DEF_SUCCESS), result);

	Util_str_add(&ftpd.ftpd_status, "\nCleaning up...");
	threadFree(ftpd.server_thread);
	threadFree(ftpd.io_thread);

	//Flush the queue.
	{
		uint32_t id = 0;
		void* data = NULL;

		while(true)
		{
			result = Util_queue_get(&ftpd.server_thread_queue, &id, &data, 0);
			if(result != DEF_SUCCESS)
				break;

			if(data)
			{
				switch (id)
				{
					case IO_RES_READ:	{ Ftpd_io_res_read_exit((Ftpd_io_res_read*)data);	break; }
					case IO_RES_WRITE:	{ Ftpd_io_res_write_exit((Ftpd_io_res_write*)data);	break; }
					default:			{ break; }
				}
			}

			free(data);
			data = NULL;
		}
		while(true)
		{
			result = Util_queue_get(&ftpd.io_thread_queue, &id, &data, 0);
			if(result != DEF_SUCCESS)
				break;

			if(data)
			{
				switch (id)
				{
					case IO_CMD_READ:	{ Ftpd_io_cmd_read_exit((Ftpd_io_cmd_read*)data);		break; }
					case IO_CMD_WRITE:	{ Ftpd_io_cmd_write_exit((Ftpd_io_cmd_write*)data);		break; }
					default:			{ break; }
				}
			}

			free(data);
			data = NULL;
		}
	}

	Util_queue_delete(&ftpd.server_thread_queue);
	Util_queue_delete(&ftpd.io_thread_queue);
	Util_sync_destroy(&ftpd.mutex);

	Ftpd_ui_data_exit(&ftpd.ui_data_server);
	Ftpd_ui_data_exit(&ftpd.ui_data_copy_draw);
	Ftpd_config_exit(&ftpd.config_hid);
	Ftpd_config_exit(&ftpd.config_copy_draw);
	Ftpd_config_exit(&ftpd.config_copy_server);
	Ftpd_ram_config_exit(&ftpd.ram_config_hid);
	Ftpd_ram_config_exit(&ftpd.ram_config_copy_draw);

	for(uint8_t i = 0; i < MAX_CLIENTS; i++)
	{
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].is_active_mode);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].sent);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].received);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].last_cmd);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].last_arg.sequential_id);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].last_res.sequential_id);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].dir.sequential_id);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].control_address);
		Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.clients[i].data_address);
	}
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.is_lan_unknown);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.is_wan_unknown);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.max_clients);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.connected_clients);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.control_port);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.lan_ip);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ui_data_server.wan_ip);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.sleep_policy);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.read_policy);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.write_policy);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.rename_policy);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.config_hid.delete_policy);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ram_config_hid.is_lan_address_displayed);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.ram_config_hid.is_wan_address_displayed);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.sleep_policy_button.selected);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.read_policy_button.selected);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.write_policy_button.selected);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.rename_policy_button.selected);
	Util_watch_remove(WATCH_HANDLE_FTP_DAEMON, &ftpd.delete_policy_button.selected);

	ftpd.inited = false;

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

static uint32_t Ftpd_load_settings(void)
{
	uint32_t is_out_data_valid[DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(ftpd_settings_key)] = { 0, };
	uint8_t* settings = NULL;
	uint32_t result = DEF_ERR_OTHER;
	uint32_t read_size = 0;
	uint32_t out_data[DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(ftpd_settings_key)] = { 0, };

	DEF_LOG_RESULT_SMART(result, Util_file_load_from_file("ftpd_settings.json", DEF_MENU_MAIN_DIR, &settings, 0x1000, 0, &read_size), (result == DEF_SUCCESS), result);

	if(result == DEF_SUCCESS)
	{
		Json_data json = { 0, };

		result = Util_json_parse((char*)settings, &json);
		if(result == DEF_SUCCESS)
		{
			//Try to extract settings.
			for(uint8_t i = 0; i < DEF_UTIL_ARRAY_NUM_OF_ELEMENTS(ftpd_settings_key); i++)
			{
				Json_extracted_data extracted_data = { 0, };

				result = Util_json_get_data(ftpd_settings_key[i], &json, &extracted_data);
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_json_get_data, false, result);
					DEF_LOG_STRING(ftpd_settings_key[i]);
					continue;
				}

				if(extracted_data.type == JSON_TYPE_NUMBER)
				{
					out_data[i] = *((double*)extracted_data.value);
					is_out_data_valid[i] = true;
				}
				else
				{
					if(extracted_data.type == JSON_TYPE_INVALID)
						DEF_LOG_STRING("Key not found!!!!!");
					else
						DEF_LOG_STRING("Invalid type!!!!!");

					DEF_LOG_STRING(Json_value_type_get_name(extracted_data.type));
					DEF_LOG_STRING(ftpd_settings_key[i]);
				}

				free(extracted_data.value);
				extracted_data.value = NULL;
			}
		}
		else
			DEF_LOG_STRING("Couldn't parse settings file, using default settings!!!!!");

		Util_json_free(&json);
	}
	else
		DEF_LOG_STRING("Couldn't read settings file, using default settings!!!!!");

	Util_sync_lock(&ftpd.mutex, UINT64_MAX);
	//Load value if it's valid (exists in config).
	if(is_out_data_valid[0]) ftpd.config_hid.sleep_policy = (Ftpd_sleep_policy)out_data[0];
	if(is_out_data_valid[1]) ftpd.config_hid.read_policy = (Ftpd_access_policy)out_data[1];
	if(is_out_data_valid[2]) ftpd.config_hid.write_policy = (Ftpd_access_policy)out_data[2];
	if(is_out_data_valid[3]) ftpd.config_hid.rename_policy = (Ftpd_access_policy)out_data[3];
	if(is_out_data_valid[4]) ftpd.config_hid.delete_policy = (Ftpd_access_policy)out_data[4];

	//Check if value is in valid range.
	if(ftpd.config_hid.sleep_policy >= SLEEP_POLICY_MAX)
		ftpd.config_hid.sleep_policy = SLEEP_POLICY_ALLOW_WHEN_NO_CONTROL_CONN;
	if(ftpd.config_hid.read_policy >= ACCESS_POLICY_MAX)
		ftpd.config_hid.read_policy = ACCESS_POLICY_ALLOW;
	if(ftpd.config_hid.write_policy >= ACCESS_POLICY_MAX)
		ftpd.config_hid.write_policy = ACCESS_POLICY_ALLOW;
	if(ftpd.config_hid.rename_policy >= ACCESS_POLICY_MAX)
		ftpd.config_hid.rename_policy = ACCESS_POLICY_ALLOW;
	if(ftpd.config_hid.delete_policy >= ACCESS_POLICY_MAX)
		ftpd.config_hid.delete_policy = ACCESS_POLICY_DISALLOW;

	Ftpd_log_settings();
	Util_sync_unlock(&ftpd.mutex);

	return DEF_SUCCESS;//Settings (or default one) has been loaded.
}

static uint32_t Ftpd_save_settings(void)
{
	uint32_t result = DEF_ERR_OTHER;
	Str_data data = { 0, };

	Util_str_init(&data);

	Util_sync_lock(&ftpd.mutex, UINT64_MAX);
	Ftpd_log_settings();

	//Make a json data, then save it.
	Util_str_format(&data, DEF_JSON_START_OBJECT);
	Util_str_format_append(&data, DEF_JSON_NON_STR_DATA_WITH_KEY(ftpd_settings_key[0], "%" PRIu32, (uint32_t)ftpd.config_hid.sleep_policy));
	Util_str_format_append(&data, DEF_JSON_NON_STR_DATA_WITH_KEY(ftpd_settings_key[1], "%" PRIu32, (uint32_t)ftpd.config_hid.read_policy));
	Util_str_format_append(&data, DEF_JSON_NON_STR_DATA_WITH_KEY(ftpd_settings_key[2], "%" PRIu32, (uint32_t)ftpd.config_hid.write_policy));
	Util_str_format_append(&data, DEF_JSON_NON_STR_DATA_WITH_KEY(ftpd_settings_key[3], "%" PRIu32, (uint32_t)ftpd.config_hid.rename_policy));
	Util_str_format_append(&data, DEF_JSON_NON_STR_DATA_WITH_KEY_WITHOUT_COMMA(ftpd_settings_key[4], "%" PRIu32, (uint32_t)ftpd.config_hid.delete_policy));
	Util_str_format_append(&data, DEF_JSON_END_OBJECT);
	Util_sync_unlock(&ftpd.mutex);

	DEF_LOG_RESULT_SMART(result, Util_file_save_to_file("ftpd_settings.json", DEF_MENU_MAIN_DIR, (uint8_t*)data.buffer, data.capacity, true), (result == DEF_SUCCESS), result);

	Util_str_free(&data);
	return result;
}

static void Ftpd_log_settings(void)
{
	DEF_LOG_STRING(Ftpd_sleep_policy_get_name(ftpd.config_hid.sleep_policy));
	DEF_LOG_STRING(Ftpd_access_policy_get_name(ftpd.config_hid.read_policy));
	DEF_LOG_STRING(Ftpd_access_policy_get_name(ftpd.config_hid.write_policy));
	DEF_LOG_STRING(Ftpd_access_policy_get_name(ftpd.config_hid.rename_policy));
	DEF_LOG_STRING(Ftpd_access_policy_get_name(ftpd.config_hid.delete_policy));
}

static void Ftpd_get_local_ip(uint32_t* ip)
{
	uint32_t result = DEF_ERR_OTHER;
	socklen_t address_len = sizeof(SOCU_IPInfo);
	SOCU_IPInfo address = { 0, };

	result = SOCU_GetNetworkOpt(SOL_CONFIG, NETOPT_IP_INFO, &address, &address_len);
	if(result == DEF_SUCCESS)
		*ip = address.ip.s_addr;
	else
		DEF_LOG_RESULT(SOCU_GetNetworkOpt, false, result);
}

static void Ftpd_get_global_ip(uint32_t* ip)
{
	uint32_t result = DEF_ERR_OTHER;
	struct in_addr address = { 0, };
	Net_dl_parameters parameters = { 0, };

	parameters.url = GLOBAL_IP_ADDRESS_API_URL;
	parameters.data = NULL;
	parameters.max_redirect = 0;
	parameters.max_size = 100;
	result = Util_curl_dl_data(&parameters);
	if(result == DEF_SUCCESS)
	{
		result = inet_aton((char*)parameters.data, &address);
		if(result != 0)
			*ip = address.s_addr;
		else
			DEF_LOG_RESULT(inet_aton, false, result);
	}
	else
		DEF_LOG_RESULT(Util_curl_dl_data, false, result);

	free(parameters.data);
	parameters.data = NULL;
}

static uint32_t Ftpd_client_user_data_init(Ftp_client* client)
{
	uint32_t result = DEF_ERR_OTHER;

	client->user_data = (Ftpd_client_user_data*)malloc(sizeof(Ftpd_client_user_data));
	if(!client->user_data)
	{
		DEF_LOG_RESULT(malloc, false, 0);
		goto out_of_memory;
	}

	Ftpd_client_user_data_clear(client);
	return DEF_SUCCESS;

	out_of_memory:
	Ftpd_client_user_data_exit(client);
	return result;
}

static uint32_t Ftpd_io_init(Ftpd_fs* fs, uint32_t read_buffer_size)
{
	uint32_t result = DEF_ERR_OTHER;

	for(uint8_t i = 0; i < MAX_READ_STATE; i++)
	{
		result = Ftpd_read_state_init(&fs->read[i], read_buffer_size);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Ftpd_read_state_init, false, result);
			goto error;
		}
	}

	Ftpd_io_clear(fs);
	return DEF_SUCCESS;

	error:
	Ftpd_io_exit(fs);
	return result;
}

static uint32_t Ftpd_read_state_init(Ftpd_read_state* read_state, uint32_t buffer_size)
{
	uint32_t result = DEF_ERR_OTHER;

	read_state->buffer_size = buffer_size;

	result = Util_str_init(&read_state->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&read_state->file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	Ftpd_read_state_clear(read_state);
	return DEF_SUCCESS;

	error:
	Ftpd_read_state_exit(read_state);
	return result;
}

static uint32_t Ftpd_io_cmd_read_init(Ftpd_io_cmd_read* read, uint8_t client_index, const char* file_name, const char* dir)
{
	uint32_t result = DEF_ERR_OTHER;

	read->client_index = client_index;
	result = Util_str_init(&read->file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&read->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_set(&read->file_name, file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	result = Util_str_set(&read->dir, dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	return DEF_SUCCESS;

	error:
	Ftpd_io_cmd_read_exit(read);
	return result;
}

static uint32_t Ftpd_io_cmd_write_init(Ftpd_io_cmd_write* write, bool is_append, bool is_eof, uint8_t client_index, const uint8_t* buffer, uint32_t buffer_size, const char* file_name, const char* dir)
{
	uint32_t result = DEF_ERR_OTHER;

	write->is_append = is_append;
	write->is_eof = is_eof;
	write->client_index = client_index;
	write->buffer_size = buffer_size;

	if(buffer_size > 0)
	{
		write->buffer = (uint8_t*)malloc(buffer_size);
		if(!write->buffer)
		{
			DEF_LOG_RESULT(malloc, false, 0);
			goto out_of_memory;
		}

		memcpy(write->buffer, buffer, buffer_size);
	}
	else
		write->buffer = NULL;

	result = Util_str_init(&write->file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&write->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_set(&write->file_name, file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	result = Util_str_set(&write->dir, dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	return DEF_SUCCESS;

	out_of_memory:
	Ftpd_io_cmd_write_exit(write);
	return DEF_ERR_OUT_OF_MEMORY;

	error:
	Ftpd_io_cmd_write_exit(write);
	return result;
}

static uint32_t Ftpd_io_res_read_init(Ftpd_io_res_read* read, bool is_error, bool is_eof, uint8_t client_index, const uint8_t* buffer, uint32_t buffer_size, const char* file_name, const char* dir)
{
	uint32_t result = DEF_ERR_OTHER;

	read->is_error = is_error;
	read->is_eof = is_eof;
	read->client_index = client_index;
	read->buffer_size = buffer_size;

	if(buffer_size > 0)
	{
		read->buffer = (uint8_t*)malloc(buffer_size);
		if(!read->buffer)
		{
			DEF_LOG_RESULT(malloc, false, 0);
			goto out_of_memory;
		}

		memcpy(read->buffer, buffer, buffer_size);
	}
	else
		read->buffer = NULL;

	result = Util_str_init(&read->file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&read->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_set(&read->file_name, file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	result = Util_str_set(&read->dir, dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	return DEF_SUCCESS;

	out_of_memory:
	Ftpd_io_res_read_exit(read);
	return DEF_ERR_OUT_OF_MEMORY;

	error:
	Ftpd_io_res_read_exit(read);
	return result;
}

static uint32_t Ftpd_io_res_write_init(Ftpd_io_res_write* write, bool is_error, uint8_t client_index, const char* file_name, const char* dir)
{
	uint32_t result = DEF_ERR_OTHER;

	write->is_error = is_error;
	write->client_index = client_index;

	result = Util_str_init(&write->file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_init(&write->dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_init, false, result);
		goto error;
	}

	result = Util_str_set(&write->file_name, file_name);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	result = Util_str_set(&write->dir, dir);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Util_str_set, false, result);
		goto error;
	}

	return DEF_SUCCESS;

	error:
	Ftpd_io_res_write_exit(write);
	return result;
}

static uint32_t Ftpd_ui_data_init(Ftpd_ui_data* ui_data)
{
	uint32_t result = DEF_ERR_OTHER;

	memset(ui_data, 0x00, sizeof(Ftpd_ui_data));
	result = Ftpd_ui_client_data_init(ui_data->clients);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftpd_ui_client_data_init, false, result);
		goto error;
	}

	return DEF_SUCCESS;

	error:
	Ftpd_ui_data_exit(ui_data);
	return result;
}

static uint32_t Ftpd_config_init(Ftpd_config* config)
{
	config->sleep_policy = SLEEP_POLICY_ALLOW_WHEN_NO_CONTROL_CONN;
	config->read_policy = ACCESS_POLICY_ALLOW;
	config->write_policy = ACCESS_POLICY_ALLOW;
	config->rename_policy = ACCESS_POLICY_ALLOW;
	config->delete_policy = ACCESS_POLICY_DISALLOW;
	return DEF_SUCCESS;
}

static uint32_t Ftpd_ram_config_init(Ftpd_ram_config* ram_config)
{
	ram_config->is_lan_address_displayed = true;
	ram_config->is_wan_address_displayed = false;
	return DEF_SUCCESS;
}

static uint32_t Ftpd_ui_client_data_init(Ftpd_ui_client_data* ui_data)
{
	uint32_t result = DEF_ERR_OTHER;

	memset(ui_data, 0x00, sizeof(Ftpd_ui_client_data));
	for(uint8_t i = 0; i < MAX_CLIENTS; i++)
	{
		ui_data->last_cmd = FTP_COMMAND_INVALID;

		result = Util_str_init(&ui_data[i].last_arg);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto error;
		}

		result = Util_str_init(&ui_data[i].last_res);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto error;
		}

		result = Util_str_init(&ui_data[i].dir);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_str_init, false, result);
			goto error;
		}
	}

	return DEF_SUCCESS;

	error:
	Ftpd_ui_client_data_exit(ui_data);
	return result;
}

static void Ftpd_client_user_data_clear(Ftp_client* client)
{
	if(client->user_data)
	{
		Ftpd_client_user_data* user_data = (Ftpd_client_user_data*)client->user_data;

		user_data->is_retrieve_started = false;
		user_data->is_store_eof_sent = false;
		user_data->store_buffer_offset = 0;
	}
}

static void Ftpd_io_clear(Ftpd_fs* fs)
{
	for(uint8_t i = 0; i < MAX_READ_STATE; i++)
		Ftpd_read_state_clear(&fs->read[i]);
}

static void Ftpd_read_state_clear(Ftpd_read_state* read_state)
{
	read_state->is_used = false;
	read_state->client_index = UINT8_MAX;
	read_state->offset = 0;
	Util_str_clear(&read_state->dir);
	Util_str_clear(&read_state->file_name);
}

static void Ftpd_ui_data_copy(Ftpd_ui_data* ui_data_dst, const Ftpd_ui_data* ui_data_src)
{
	ui_data_dst->is_lan_unknown = ui_data_src->is_lan_unknown;
	ui_data_dst->is_wan_unknown = ui_data_src->is_wan_unknown;
	ui_data_dst->max_clients = ui_data_src->max_clients;
	ui_data_dst->connected_clients = ui_data_src->connected_clients;
	ui_data_dst->control_port = ui_data_src->control_port;
	ui_data_dst->lan_ip = ui_data_src->lan_ip;
	ui_data_dst->wan_ip = ui_data_src->wan_ip;
	Ftpd_ui_client_data_copy(ui_data_dst->clients, ui_data_src->clients);
}

static void Ftpd_config_copy(Ftpd_config* config_dst, const Ftpd_config* config_src)
{
	config_dst->sleep_policy = config_src->sleep_policy;
	config_dst->read_policy = config_src->read_policy;
	config_dst->write_policy = config_src->write_policy;
	config_dst->rename_policy = config_src->rename_policy;
	config_dst->delete_policy = config_src->delete_policy;
}

static void Ftpd_ram_config_copy(Ftpd_ram_config* ram_config_dst, const Ftpd_ram_config* ram_config_src)
{
	ram_config_dst->is_lan_address_displayed = ram_config_src->is_lan_address_displayed;
	ram_config_dst->is_wan_address_displayed = ram_config_src->is_wan_address_displayed;
}

static void Ftpd_ui_client_data_copy(Ftpd_ui_client_data* ui_data_dst, const Ftpd_ui_client_data* ui_data_src)
{
	for(uint8_t i = 0; i < MAX_CLIENTS; i++)
	{
		ui_data_dst[i].is_active_mode = ui_data_src[i].is_active_mode;
		ui_data_dst[i].sent = ui_data_src[i].sent;
		ui_data_dst[i].received = ui_data_src[i].received;
		ui_data_dst[i].last_cmd = ui_data_src[i].last_cmd;
		Util_str_set(&ui_data_dst[i].last_arg, DEF_STR_NEVER_NULL(&ui_data_src[i].last_arg));
		Util_str_set(&ui_data_dst[i].last_res, DEF_STR_NEVER_NULL(&ui_data_src[i].last_res));
		Util_str_set(&ui_data_dst[i].dir, DEF_STR_NEVER_NULL(&ui_data_src[i].dir));
		ui_data_dst[i].control_address = ui_data_src[i].control_address;
		ui_data_dst[i].data_address = ui_data_src[i].data_address;
	}
}

static void Ftpd_client_user_data_exit(Ftp_client* client)
{
	free(client->user_data);
	client->user_data = NULL;
}

static void Ftpd_io_exit(Ftpd_fs* fs)
{
	for(uint8_t i = 0; i < MAX_READ_STATE; i++)
		Ftpd_read_state_exit(&fs->read[i]);
}

static void Ftpd_read_state_exit(Ftpd_read_state* read_state)
{
	Util_str_free(&read_state->dir);
	Util_str_free(&read_state->file_name);
}

static void Ftpd_io_cmd_read_exit(Ftpd_io_cmd_read* read)
{
	Util_str_free(&read->file_name);
	Util_str_free(&read->dir);
}

static void Ftpd_io_cmd_write_exit(Ftpd_io_cmd_write* write)
{
	free(write->buffer);
	write->buffer = NULL;
	Util_str_free(&write->file_name);
	Util_str_free(&write->dir);
}

static void Ftpd_io_res_read_exit(Ftpd_io_res_read* read)
{
	free(read->buffer);
	read->buffer = NULL;
	Util_str_free(&read->file_name);
	Util_str_free(&read->dir);
}

static void Ftpd_io_res_write_exit(Ftpd_io_res_write* write)
{
	Util_str_free(&write->file_name);
	Util_str_free(&write->dir);
}

static void Ftpd_ui_data_exit(Ftpd_ui_data* ui_data)
{
	Ftpd_ui_client_data_exit(ui_data->clients);
}

static void Ftpd_config_exit(Ftpd_config* config)
{
	(void)config;
	//Do nothing.
}

static void Ftpd_ram_config_exit(Ftpd_ram_config* ram_config)
{
	(void)ram_config;
	//Do nothing.
}

static void Ftpd_ui_client_data_exit(Ftpd_ui_client_data* ui_data)
{
	for(uint8_t i = 0; i < MAX_CLIENTS; i++)
	{
		Util_str_free(&ui_data[i].last_arg);
		Util_str_free(&ui_data[i].last_res);
		Util_str_free(&ui_data[i].dir);
	}
}

static void Ftpd_reject(Ftp_client* client)
{
	snprintf(client->message_buffer, sizeof(client->message_buffer), "Rejected by user policy!!!!!");
	client->response = FTP_RESPONSE_FILE_ACTION_ERROR_NOT_FOUND_OR_NO_ACCESS;
}

static void Ftpd_process_retr_bof(Ftp_client* client, uint8_t client_index)
{
	uint32_t result = DEF_ERR_OTHER;
	Ftpd_client_user_data* user_data = (Ftpd_client_user_data*)client->user_data;

	if(!user_data->is_retrieve_started)
	{
		bool is_error = true;
		Ftpd_io_cmd_read* cmd = (Ftpd_io_cmd_read*)calloc(1, sizeof(Ftpd_io_cmd_read));

		if(cmd)
		{
			result = Ftpd_io_cmd_read_init(cmd, client_index, DEF_STR_NEVER_NULL(&client->retrieve.file_name), DEF_STR_NEVER_NULL(&client->retrieve.dir));
			if(result == DEF_SUCCESS)
			{
				//Send cmd to IO thread for reading.
				result = Util_queue_add(&ftpd.io_thread_queue, IO_CMD_READ, cmd, DEF_UTIL_S_TO_US(60), QUEUE_OPTION_NONE);
				if(result == DEF_SUCCESS)
				{
					//cmd will be freed in IO thread.
					cmd = NULL;
					is_error = false;
					user_data->is_retrieve_started = true;
				}
				else
					DEF_LOG_RESULT(Util_queue_add, false, result);
			}
			else
				DEF_LOG_RESULT(Ftpd_io_cmd_read_init, false, result);
		}
		else
			DEF_LOG_RESULT(calloc, false, 0);

		if(cmd)
		{
			Ftpd_io_cmd_read_exit(cmd);
			free(cmd);
			cmd = NULL;
		}

		if(is_error)
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
		}
		else
			client->response = FTP_RESPONSE_INVALID;//No response at this time, result will be sent on IO_RES_READ.
	}
	else
		client->response = FTP_RESPONSE_INVALID;//No response at this time, result will be sent on IO_RES_READ.
}

static void Ftpd_process_stor_stou_appe_data(Ftp_client* client, uint8_t client_index)
{
	uint32_t result = DEF_ERR_OTHER;
	uint32_t buffer_size = 0;
	uint32_t received_size = 0;
	Ftpd_client_user_data* user_data = (Ftpd_client_user_data*)client->user_data;

	buffer_size = (client->store.buffer_size - user_data->store_buffer_offset);
	if(!user_data->is_store_eof_sent)
	{
		result = Ftp_receive_data(client->data_handle.fd, (client->store.buffer + user_data->store_buffer_offset), buffer_size, &received_size);
		if(result == DEF_SUCCESS)
		{
			bool must_send = false;
			bool is_eof = (received_size == 0);

			//Update UI data for received bytes.
			Util_sync_lock(&ftpd.mutex, UINT64_MAX);
			ftpd.ui_data_server.clients[client_index].received += received_size;
			Util_sync_unlock(&ftpd.mutex);

			user_data->store_buffer_offset += received_size;
			if(is_eof)
				must_send = true;//Always send on EOF.
			else
			{
				if(client->store.buffer_size <= user_data->store_buffer_offset)
					must_send = true;//Buffer is full, send it.
			}

			if(must_send)
			{
				bool is_error = true;
				Ftpd_io_cmd_write* cmd = (Ftpd_io_cmd_write*)calloc(1, sizeof(Ftpd_io_cmd_write));

				if(cmd)
				{
					result = Ftpd_io_cmd_write_init(cmd, client->store.is_append, is_eof, client_index, client->store.buffer, user_data->store_buffer_offset,
					DEF_STR_NEVER_NULL(&client->store.file_name), DEF_STR_NEVER_NULL(&client->store.dir));
					if(result == DEF_SUCCESS)
					{
						//Send cmd to IO thread for writing.
						result = Util_queue_add(&ftpd.io_thread_queue, IO_CMD_WRITE, cmd, DEF_UTIL_S_TO_US(60), QUEUE_OPTION_NONE);
						if(result == DEF_SUCCESS)
						{
							//cmd will be freed in IO thread.
							cmd = NULL;
							is_error = false;
							client->store.is_append = true;
							user_data->store_buffer_offset = 0;
							if(is_eof)
								user_data->is_store_eof_sent = true;
						}
						else
							DEF_LOG_RESULT(Util_queue_add, false, result);
					}
					else
						DEF_LOG_RESULT(Ftpd_io_cmd_write_init, false, result);
				}
				else
					DEF_LOG_RESULT(calloc, false, 0);

				if(cmd)
				{
					Ftpd_io_cmd_write_exit(cmd);
					free(cmd);
					cmd = NULL;
				}

				if(is_error)
				{
					snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
					client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
				}
				else
					client->response = FTP_RESPONSE_INVALID;//No response at this time, result will be sent on IO_RES_WRITE.
			}
			else
				client->response = FTP_RESPONSE_INVALID;//No response at this time, wait for next packet.
		}
		else
		{
			DEF_LOG_RESULT(Ftp_receive_data, false, result);
			client->response = FTP_RESPONSE_INVALID;//No response at this time, wait for next packet.
		}
	}
	else
		client->response = FTP_RESPONSE_INVALID;//No response at this time, result will be sent on IO_RES_WRITE.
}

static uint32_t Ftpd_process_control_connection(Ftp_clients* clients, uint8_t client_index)
{
	bool is_read_allowed = (ftpd.config_copy_server.read_policy == ACCESS_POLICY_ALLOW);
	bool is_write_allowed = (ftpd.config_copy_server.write_policy == ACCESS_POLICY_ALLOW);
	bool is_rename_allowed = (ftpd.config_copy_server.rename_policy == ACCESS_POLICY_ALLOW);
	bool is_delete_allowed = (ftpd.config_copy_server.delete_policy == ACCESS_POLICY_ALLOW);
	uint32_t result = DEF_ERR_OTHER;
	Ftp_command cmd = FTP_COMMAND_INVALID;
	Ftp_client* client = NULL;

	if(client_index >= clients->max_clients)
		return DEF_ERR_INVALID_ARG;

	client = &clients->clients[client_index];

	result = Ftp_receive_command(client->control_handle.fd, &cmd, client->arg_buffer, sizeof(client->arg_buffer));
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_receive_command, false, result);
		goto error;
	}

	//Process commands.
	switch (cmd)
	{
		//Access control commands.
		case FTP_COMMAND_USER:	{ Ftp_process_user(client);	break; }
		case FTP_COMMAND_PASS:	{ Ftp_process_pass(client);	break; }
		case FTP_COMMAND_ACCT:	{ Ftp_process_acct(client);	break; }
		case FTP_COMMAND_CWD:	{ Ftp_process_cwd(client);	break; }
		case FTP_COMMAND_CDUP:	{ Ftp_process_cdup(client);	break; }
		case FTP_COMMAND_SMNT:	{ Ftp_process_smnt(client);	break; }
		case FTP_COMMAND_REIN:	{ Ftp_process_rein(client);	break; }
		case FTP_COMMAND_QUIT:	{ Ftp_process_quit(client);	break; }

		//Transfer parameter commands.
		case FTP_COMMAND_PORT:	{ Ftp_process_port(client);	break; }
		case FTP_COMMAND_PASV:	{ Ftp_process_pasv(client);	break; }
		case FTP_COMMAND_TYPE:	{ Ftp_process_type(client);	break; }
		case FTP_COMMAND_STRU:	{ Ftp_process_stru(client);	break; }
		case FTP_COMMAND_MODE:	{ Ftp_process_mode(client);	break; }

		//Service commands.
		case FTP_COMMAND_RETR:	{ (is_read_allowed ? Ftp_process_retr(client) : Ftpd_reject(client));	break; }
		case FTP_COMMAND_STOR:	{ (is_write_allowed ? Ftp_process_stor(client) : Ftpd_reject(client));	break; }
		case FTP_COMMAND_STOU:	{ (is_write_allowed ? Ftp_process_stou(client) : Ftpd_reject(client));	break; }
		case FTP_COMMAND_APPE:	{ (is_write_allowed ? Ftp_process_appe(client) : Ftpd_reject(client));	break; }
		case FTP_COMMAND_ALLO:	{ Ftp_process_allo(client);	break; }
		case FTP_COMMAND_REST:	{ Ftp_process_rest(client);	break; }
		case FTP_COMMAND_RNFR:	{ (is_rename_allowed ? Ftp_process_rnfr(client) : Ftpd_reject(client));	break; }
		case FTP_COMMAND_RNTO:	{ Ftp_process_rnto(client); break; }
		case FTP_COMMAND_ABOR:	{ Ftp_process_abor(client);	break; }
		case FTP_COMMAND_DELE:	{ (is_delete_allowed ? Ftp_process_dele(client) : Ftpd_reject(client));	break; }
		case FTP_COMMAND_RMD:	{ (is_delete_allowed ? Ftp_process_rmd(client) : Ftpd_reject(client));	break; }
		case FTP_COMMAND_MKD:	{ Ftp_process_mkd(client);	break; }
		case FTP_COMMAND_PWD:	{ Ftp_process_pwd(client);	break; }
		case FTP_COMMAND_LIST:	{ Ftp_process_list(client);	break; }
		case FTP_COMMAND_NLST:	{ Ftp_process_nlst(client);	break; }
		case FTP_COMMAND_SITE:	{ Ftp_process_site(client);	break; }
		case FTP_COMMAND_SYST:	{ Ftp_process_syst(client);	break; }
		case FTP_COMMAND_STAT:	{ Ftp_process_stat(client);	break; }
		case FTP_COMMAND_HELP:	{ Ftp_process_help(client);	break; }
		case FTP_COMMAND_NOOP:	{ Ftp_process_noop(client);	break; }

		default:
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Unknown command!!!!!");
			client->response = FTP_RESPONSE_COMMAND_UNIMPLEMENTED;
			break;
		}
	}

	//Update UI data for last command/response and current directory.
	Util_sync_lock(&ftpd.mutex, UINT64_MAX);
	ftpd.ui_data_server.clients[client_index].last_cmd = cmd;
	Util_str_set(&ftpd.ui_data_server.clients[client_index].last_arg, client->arg_buffer);
	Util_str_set(&ftpd.ui_data_server.clients[client_index].last_res, client->message_buffer);
	if(cmd == FTP_COMMAND_CWD || cmd == FTP_COMMAND_CDUP)
		Util_str_set(&ftpd.ui_data_server.clients[client_index].dir, DEF_STR_NEVER_NULL(&client->current_dir.dir));

	Util_sync_unlock(&ftpd.mutex);

	//Send response if exists.
	if(client->response != FTP_RESPONSE_INVALID)
	{
		Ftp_response response = client->response;

		result = Ftp_send_response(client->control_handle.fd, client->response, client->message_buffer);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Ftp_send_response, false, result);
			goto error;
		}
		client->response = FTP_RESPONSE_INVALID;
		client->message_buffer[0] = 0x00;

		//Close control connection if response code says so.
		if(response == FTP_RESPONSE_STATUS_CONTROL_CONNECTION_CLOSING || response == FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE)
		{
			Ftp_client_clear(client);//Close control connection after sending response.
			Ftpd_client_user_data_clear(client);//Clear user data.
		}
		//Open connection if we are active mode and does NOT have data connection.
		else if(response == FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_WILL_OPEN)
		{
			if(client->data_listen_handle.fd < 0)
			{
				result = Ftp_connect_data_connection(client);
				if(result == DEF_SUCCESS)
				{
					//Update UI data for data address and active mode.
					Util_sync_lock(&ftpd.mutex, UINT64_MAX);
					ftpd.ui_data_server.clients[client_index].is_active_mode = true;
					ftpd.ui_data_server.clients[client_index].data_address = client->data_address;
					Util_sync_unlock(&ftpd.mutex);

					//Process BOF after connection establishment.
					result = Ftpd_process_bof(clients, client_index);
					if(result != DEF_SUCCESS)
					{
						DEF_LOG_RESULT(Ftpd_process_bof, false, result);
						goto error;
					}
				}
				else
				{
					DEF_LOG_STRING("Data connection couldn't be opened!!!!!");
					DEF_LOG_RESULT(Ftp_connect_data_connection, false, result);
					snprintf(client->message_buffer, sizeof(client->message_buffer), "Insecure channel couldn't be opened!!!!!");
					client->response = FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_CANNOT_OPEN;

					//Update UI data for last response.
					Util_sync_lock(&ftpd.mutex, UINT64_MAX);
					Util_str_set(&ftpd.ui_data_server.clients[client_index].last_res, client->message_buffer);
					Util_sync_unlock(&ftpd.mutex);

					result = Ftp_send_response(client->control_handle.fd, client->response, client->message_buffer);
					if(result != DEF_SUCCESS)
					{
						DEF_LOG_RESULT(Ftp_send_response, false, result);
						goto error;
					}
					client->response = FTP_RESPONSE_INVALID;
					client->message_buffer[0] = 0x00;
				}
			}
			else
			{
				//Wait for client to establish data connection.
			}
		}
		//Immediately process BOF if data connection already exists.
		else if(response == FTP_RESPONSE_DATA_CONNECTION_EXIST_STARTING_TRANSFER)
		{
			result = Ftpd_process_bof(clients, client_index);
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Ftpd_process_bof, false, result);
				goto error;
			}
		}
	}

	return DEF_SUCCESS;

	error:
	Ftp_client_clear(client);
	Ftpd_client_user_data_clear(client);
	return result;
}

static uint32_t Ftpd_process_bof(Ftp_clients* clients, uint8_t client_index)
{
	uint32_t result = DEF_ERR_OTHER;
	Ftp_client* client = NULL;

	if(client_index >= clients->max_clients)
		return DEF_ERR_INVALID_ARG;

	client = &clients->clients[client_index];

	//Process BOF.
	switch (client->pending_command)
	{
		//Library version isn't optimized and is very slow.
		// case FTP_COMMAND_RETR:	{ Ftp_process_retr_bof(client);					break; }
		case FTP_COMMAND_RETR:	{ Ftpd_process_retr_bof(client, client_index);	break; }
		case FTP_COMMAND_LIST:	{ Ftp_process_list_bof(client);					break; }
		case FTP_COMMAND_NLST:	{ Ftp_process_nlst_bof(client);					break; }
		default: 				{ break; }
	}

	//Process EOF if response exists.
	if(client->response != FTP_RESPONSE_INVALID)
	{
		result = Ftpd_process_eof(clients, client_index);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Ftpd_process_eof, false, result);
			goto error;
		}
	}

	return DEF_SUCCESS;

	error:
	Ftp_client_clear(client);
	Ftpd_client_user_data_clear(client);
	return result;
}

static uint32_t Ftpd_process_eof(Ftp_clients* clients, uint8_t client_index)
{
	uint32_t result = DEF_ERR_OTHER;
	Ftp_client* client = NULL;
	Ftpd_client_user_data* user_data = NULL;

	if(client_index >= clients->max_clients)
		return DEF_ERR_INVALID_ARG;

	client = &clients->clients[client_index];
	user_data = (Ftpd_client_user_data*)client->user_data;

	//Clear application state first.
	if(client->pending_command == FTP_COMMAND_STOR || client->pending_command == FTP_COMMAND_STOU || client->pending_command == FTP_COMMAND_APPE)
	{
		user_data->is_store_eof_sent = false;
		user_data->store_buffer_offset = 0;
	}
	else if(client->pending_command == FTP_COMMAND_RETR)
		user_data->is_retrieve_started = false;

	//Process EOF.
	switch (client->pending_command)
	{
		case FTP_COMMAND_RETR:	{ Ftp_process_retr_eof(client);				break; }
		case FTP_COMMAND_STOR:	{ Ftp_process_stor_eof(client);				break; }
		case FTP_COMMAND_STOU:	{ Ftp_process_stou_eof(client);				break; }
		case FTP_COMMAND_APPE:	{ Ftp_process_appe_eof(client);				break; }
		default: 				{ break; }
	}

	//Send response if exists.
	if(client->response != FTP_RESPONSE_INVALID)
	{
		Ftp_response response = client->response;

		//Update UI data for last response.
		Util_sync_lock(&ftpd.mutex, UINT64_MAX);
		Util_str_set(&ftpd.ui_data_server.clients[client_index].last_res, client->message_buffer);
		Util_sync_unlock(&ftpd.mutex);

		result = Ftp_send_response(client->control_handle.fd, client->response, client->message_buffer);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Ftp_send_response, false, result);
			goto error;
		}
		client->response = FTP_RESPONSE_INVALID;
		client->message_buffer[0] = 0x00;

		//Close control connection if response code says so.
		if(response == FTP_RESPONSE_STATUS_CONTROL_CONNECTION_CLOSING || response == FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE)
		{
			Ftp_client_clear(client);//Close control connection after sending response.
			Ftpd_client_user_data_clear(client);//Clear user data.
		}
	}

	return DEF_SUCCESS;

	error:
	Ftp_client_clear(client);
	Ftpd_client_user_data_clear(client);
	return result;
}

static uint32_t Ftpd_process_data_connection(Ftp_clients* clients, uint8_t client_index)
{
	uint32_t result = DEF_ERR_OTHER;
	Ftp_client* client = NULL;

	if(client_index >= clients->max_clients)
		return DEF_ERR_INVALID_ARG;

	client = &clients->clients[client_index];
	if(client->data_handle.fd < 0)
		return DEF_SUCCESS;//Do nothing.

	//Process data.
	switch (client->pending_command)
	{
		//Library version isn't optimized and is very slow.
		// case FTP_COMMAND_STOR:	{ Ftp_process_stor_data(client);							break; }
		// case FTP_COMMAND_STOU:	{ Ftp_process_stou_data(client);							break; }
		// case FTP_COMMAND_APPE:	{ Ftp_process_appe_data(client);							break; }
		case FTP_COMMAND_STOR:	{ Ftpd_process_stor_stou_appe_data(client, client_index);	break; }
		case FTP_COMMAND_STOU:	{ Ftpd_process_stor_stou_appe_data(client, client_index);	break; }
		case FTP_COMMAND_APPE:	{ Ftpd_process_stor_stou_appe_data(client, client_index);	break; }
		default: 				{ break; }
	}

	//Process EOF if response exists.
	if(client->response != FTP_RESPONSE_INVALID)
	{
		result = Ftpd_process_eof(clients, client_index);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Ftpd_process_eof, false, result);
			goto error;
		}
	}

	return DEF_SUCCESS;

	error:
	Ftp_client_clear(client);
	Ftpd_client_user_data_clear(client);
	return result;
}

static void Ftpd_process_res_read(Ftp_clients* clients, Ftpd_io_res_read* res_read)
{
	bool is_error = true;
	uint32_t result = DEF_ERR_OTHER;

	if(res_read->client_index < clients->max_clients)
	{
		Ftp_client* client = &clients->clients[res_read->client_index];

		//Send file data first.
		if(res_read->buffer_size > 0)
		{
			result = Ftp_send_data(client->data_handle.fd, res_read->buffer, res_read->buffer_size);
			if(result == DEF_SUCCESS)
			{
				//Update UI data for sent bytes.
				Util_sync_lock(&ftpd.mutex, UINT64_MAX);
				ftpd.ui_data_server.clients[res_read->client_index].sent += res_read->buffer_size;
				Util_sync_unlock(&ftpd.mutex);
			}
			else
				DEF_LOG_RESULT(Ftp_send_data, false, result);
		}
		else
			result = DEF_SUCCESS;//Nothing to send.

		is_error = (res_read->is_error || result != DEF_SUCCESS);
		if(res_read->is_eof || is_error)
		{
			//Set response and process EOF.
			if(!is_error)
			{
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Done.");
				client->response = FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK;
			}
			else
			{
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
				client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
			}

			result = Ftpd_process_eof(clients, res_read->client_index);
			if(result != DEF_SUCCESS)
				DEF_LOG_RESULT(Ftpd_process_eof, false, result);
		}
	}

	Ftpd_io_res_read_exit(res_read);
}

static void Ftpd_process_res_write(Ftp_clients* clients, Ftpd_io_res_write* res_write)
{
	uint32_t result = DEF_ERR_OTHER;

	if(res_write->client_index < clients->max_clients)
	{
		Ftp_client* client = &clients->clients[res_write->client_index];

		//Set response and process EOF.
		if(!res_write->is_error)
		{
			if(client->pending_command == FTP_COMMAND_STOU)//RFC959: The response must include the name generated.
				snprintf(client->message_buffer, sizeof(client->message_buffer), "\"%s\" is the one actually used, done.", DEF_STR_NEVER_NULL(&client->store.file_name));
			else
				snprintf(client->message_buffer, sizeof(client->message_buffer), "Done.");

			client->response = FTP_RESPONSE_DATA_CONNECTION_CLOSING_ACTION_OK;
		}
		else
		{
			snprintf(client->message_buffer, sizeof(client->message_buffer), "Catastrophic internal failure!!!!!");
			client->response = FTP_RESPONSE_STATUS_SERVICE_UNAVAILABLE;
		}

		result = Ftpd_process_eof(clients, res_write->client_index);
		if(result != DEF_SUCCESS)
			DEF_LOG_RESULT(Ftpd_process_eof, false, result);
	}

	Ftpd_io_res_write_exit(res_write);
}

static void Ftpd_server_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;
	uint32_t local_ip = 0;
	uint32_t global_ip = 0;

	//Update UI data.
	Util_sync_lock(&ftpd.mutex, UINT64_MAX);
	ftpd.ui_data_server.max_clients = MAX_CLIENTS;
	ftpd.ui_data_server.control_port = CONTROL_PORT;
	Util_sync_unlock(&ftpd.mutex);

	Ftpd_get_local_ip(&local_ip);
	//Update UI data.
	Util_sync_lock(&ftpd.mutex, UINT64_MAX);
	ftpd.ui_data_server.is_lan_unknown = (local_ip == 0);
	ftpd.ui_data_server.lan_ip = local_ip;
	Util_sync_unlock(&ftpd.mutex);

	Ftpd_get_global_ip(&global_ip);
	//Update UI data.
	Util_sync_lock(&ftpd.mutex, UINT64_MAX);
	ftpd.ui_data_server.is_wan_unknown = (global_ip == 0);
	ftpd.ui_data_server.wan_ip = global_ip;
	Util_sync_unlock(&ftpd.mutex);

	result = Ftp_clients_init(&ftpd.clients, MAX_CLIENTS, CONTROL_PORT, local_ip, global_ip, DATA_PORT_BASE, IO_BUFFER_SIZE, MAX_FILES);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftp_clients_init, false, result);
		goto error;
	}

	for(uint8_t i = 0; i < ftpd.clients.max_clients; i++)
	{
		result = Ftpd_client_user_data_init(&ftpd.clients.clients[i]);
		if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Ftpd_client_user_data_init, false, result);
			goto error;
		}
	}

	while (ftpd.thread_run)
	{
		bool has_control_connection = false;
		bool has_data_connection = false;
		int32_t poll_result = 0;
		uint32_t id = 0;
		uint32_t max_handles = ((MAX_CLIENTS * 3) + 1);
		void* data = NULL;
		struct pollfd handles[max_handles];
		nfds_t count = 0;

		//Listening sockets.
		handles[CONTROL_LISTEN_INDEX] = ftpd.clients.control_listen_handle;
		count = 1;

		Util_sync_lock(&ftpd.mutex, UINT64_MAX);
		ftpd.ui_data_server.connected_clients = 0;
		for(uint16_t i = 0; i < MAX_CLIENTS; i++)
		{
			//Clients sockets.
			if(ftpd.clients.clients[i].control_handle.fd >= 0)
			{
				handles[count++] = ftpd.clients.clients[i].control_handle;
				ftpd.ui_data_server.clients[i].control_address = ftpd.clients.clients[i].control_address;
				ftpd.ui_data_server.connected_clients++;
				has_control_connection = true;
			}
			else
			{
				ftpd.ui_data_server.clients[i].last_cmd = FTP_COMMAND_INVALID;
				if(Util_str_has_data(&ftpd.ui_data_server.clients[i].last_arg))
					Util_str_clear(&ftpd.ui_data_server.clients[i].last_arg);
				if(Util_str_has_data(&ftpd.ui_data_server.clients[i].last_res))
					Util_str_clear(&ftpd.ui_data_server.clients[i].last_res);
				if(!Util_str_is_same_c(&ftpd.ui_data_server.clients[i].dir, "/"))
					Util_str_set(&ftpd.ui_data_server.clients[i].dir, "/");

				memset(&ftpd.ui_data_server.clients[i].control_address, 0x00, sizeof(ftpd.ui_data_server.clients[i].control_address));
			}

			if(ftpd.clients.clients[i].data_handle.fd >= 0)
			{
				handles[count++] = ftpd.clients.clients[i].data_handle;
				ftpd.ui_data_server.clients[i].data_address = ftpd.clients.clients[i].data_address;
				has_data_connection = true;
			}
			else
			{
				ftpd.ui_data_server.clients[i].is_active_mode = false;
				ftpd.ui_data_server.clients[i].sent = 0;
				ftpd.ui_data_server.clients[i].received = 0;
				memset(&ftpd.ui_data_server.clients[i].data_address, 0x00, sizeof(ftpd.ui_data_server.clients[i].data_address));
			}

			//Listening sockets.
			if(ftpd.clients.clients[i].data_listen_handle.fd >= 0)
				handles[count++] = ftpd.clients.clients[i].data_listen_handle;
		}
		Ftpd_config_copy(&ftpd.config_copy_server, &ftpd.config_hid);
		Util_sync_unlock(&ftpd.mutex);

		//Reset afk count so that system won't go sleep if configured so.
		if(ftpd.config_copy_server.sleep_policy == SLEEP_POLICY_ALLOW_WHEN_NO_DATA_CONN && has_data_connection)
			Util_hid_reset_afk_time();//Disallow due to existence of data connection.
		else if(ftpd.config_copy_server.sleep_policy == SLEEP_POLICY_ALLOW_WHEN_NO_CONTROL_CONN && has_control_connection)
			Util_hid_reset_afk_time();//Disallow due to existence of control connection.
		else if(ftpd.config_copy_server.sleep_policy == SLEEP_POLICY_DISALLOW)
			Util_hid_reset_afk_time();//Unconditionally disallow.

		//Check for incoming commands/connections.
		poll_result = poll(handles, count, DEF_UTIL_US_TO_MS(DEF_THREAD_ACTIVE_SLEEP_TIME));
		if(poll_result > 0)
		{
			for(uint16_t i = 0; i < count; i++)
			{
				if(handles[i].revents & POLLIN)
				{
					if(i == CONTROL_LISTEN_INDEX)
					{
						result = Ftp_accept_control_connection(&ftpd.clients);
						if(result == DEF_SUCCESS)
						{
							if(ftpd.config_copy_server.sleep_policy == SLEEP_POLICY_ALLOW_WHEN_NO_CONTROL_CONN)
								Util_hid_reset_afk_time();//Disallow due to existence of control connection.

							//Update UI data for control address.
							Util_sync_lock(&ftpd.mutex, UINT64_MAX);
							for(uint16_t k = 0; k < MAX_CLIENTS; k++)
							{
								if(ftpd.clients.clients[k].control_handle.fd >= 0)
									ftpd.ui_data_server.clients[k].control_address = ftpd.clients.clients[k].control_address;
							}
							Util_sync_unlock(&ftpd.mutex);
						}
						else
						{
							DEF_LOG_STRING("Control connection couldn't be established!!!!!");
							DEF_LOG_RESULT(Ftp_accept_control_connection, false, result);
						}
					}
					else
					{
						for(uint8_t k = 0; k < MAX_CLIENTS; k++)
						{
							Ftp_client* client = &ftpd.clients.clients[k];

							if(client->control_handle.fd == handles[i].fd)
							{
								//Process control commands.
								result = Ftpd_process_control_connection(&ftpd.clients, k);
								if(result != DEF_SUCCESS)
									DEF_LOG_RESULT(Ftpd_process_control_connection, false, result);

								break;
							}
							else if(client->data_handle.fd == handles[i].fd)
							{
								//Process data.
								result = Ftpd_process_data_connection(&ftpd.clients, k);
								if(result != DEF_SUCCESS)
									DEF_LOG_RESULT(Ftpd_process_data_connection, false, result);

								break;
							}
							else if(client->data_listen_handle.fd == handles[i].fd)
							{
								result = Ftp_accept_data_connection(&ftpd.clients.clients[k]);
								if(result == DEF_SUCCESS)
								{
									if(ftpd.config_copy_server.sleep_policy == SLEEP_POLICY_ALLOW_WHEN_NO_DATA_CONN)
										Util_hid_reset_afk_time();//Disallow due to existence of data connection.

									//Update UI data for data address and active mode.
									Util_sync_lock(&ftpd.mutex, UINT64_MAX);
									ftpd.ui_data_server.clients[k].is_active_mode = false;
									ftpd.ui_data_server.clients[k].data_address = client->data_address;
									Util_sync_unlock(&ftpd.mutex);

									//Now, we have a connection.
									result = Ftpd_process_bof(&ftpd.clients, k);
									if(result != DEF_SUCCESS)
										DEF_LOG_RESULT(Ftpd_process_bof, false, result);
								}
								else
								{
									DEF_LOG_STRING("Data connection couldn't be established!!!!!");
									DEF_LOG_RESULT(Ftp_accept_data_connection, false, result);
									snprintf(client->message_buffer, sizeof(client->message_buffer), "Insecure channel couldn't be opened!!!!!");
									client->response = FTP_RESPONSE_DATA_CONNECTION_NOT_EXIST_CANNOT_OPEN;

									//Update UI data for last response.
									Util_sync_lock(&ftpd.mutex, UINT64_MAX);
									Util_str_set(&ftpd.ui_data_server.clients[k].last_res, client->message_buffer);
									Util_sync_unlock(&ftpd.mutex);

									result = Ftp_send_response(client->control_handle.fd, client->response, client->message_buffer);
									if(result != DEF_SUCCESS)
									{
										DEF_LOG_RESULT(Ftp_send_response, false, result);
										Ftp_client_clear(client);
										Ftpd_client_user_data_clear(client);
									}
									else
									{
										client->response = FTP_RESPONSE_INVALID;
										client->message_buffer[0] = 0x00;
									}
								}

								break;
							}
						}
					}
				}
			}
		}
		else if(poll_result < 0)
			Util_sleep(DEF_THREAD_ACTIVE_SLEEP_TIME);

		//Check for response from IO thread.
		result = Util_queue_get(&ftpd.server_thread_queue, &id, &data, 0);
		if(result == DEF_ERR_TRY_AGAIN)
			continue;
		else if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_queue_get, false, result);
			break;
		}

		if(data)
		{
			switch (id)
			{
				case IO_RES_READ:	{ Ftpd_process_res_read(&ftpd.clients, (Ftpd_io_res_read*)data);		break; }
				case IO_RES_WRITE:	{ Ftpd_process_res_write(&ftpd.clients, (Ftpd_io_res_write*)data);		break; }
				default:			{ break; }
			}
		}

		free(data);
		data = NULL;
	}

	error:
	if(ftpd.clients.clients)
	{
		for(uint8_t i = 0; i < ftpd.clients.max_clients; i++)
			Ftpd_client_user_data_exit(&ftpd.clients.clients[i]);
	}

	Ftp_clients_exit(&ftpd.clients);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}

static void Ftpd_io_process_read(Ftpd_fs* fs)
{
	uint32_t result = DEF_ERR_OTHER;

	for(uint8_t i = 0; i < MAX_READ_STATE; i++)
	{
		if(fs->read[i].is_used)
		{
			bool is_eof = false;
			bool is_error = true;
			Ftpd_io_res_read* res = (Ftpd_io_res_read*)calloc(1, sizeof(Ftpd_io_res_read));

			if(res)
			{
				uint8_t* buffer = NULL;
				uint32_t buffer_size = IO_BUFFER_SIZE;
				uint32_t read_size = 0;

				result = Util_file_load_from_file(DEF_STR_NEVER_NULL(&fs->read[i].file_name), DEF_STR_NEVER_NULL(&fs->read[i].dir),
				&buffer, buffer_size, fs->read[i].offset, &read_size);
				if(result != DEF_SUCCESS)
				{
					DEF_LOG_RESULT(Util_file_load_from_file, false, result);
					read_size = 0;
				}

				//If read size was less than buffer size, that's EOF.
				is_eof = (read_size < buffer_size);

				result = Ftpd_io_res_read_init(res, (result != DEF_SUCCESS), is_eof, fs->read[i].client_index, buffer,
				read_size, DEF_STR_NEVER_NULL(&fs->read[i].file_name), DEF_STR_NEVER_NULL(&fs->read[i].dir));
				if(result == DEF_SUCCESS)
				{
					//Send result to server thread.
					result = Util_queue_add(&ftpd.server_thread_queue, IO_RES_READ, res, DEF_UTIL_S_TO_US(60), QUEUE_OPTION_NONE);
					if(result == DEF_SUCCESS)
					{
						//res will be freed in server thread.
						res = NULL;
						is_error = false;
						fs->read[i].offset += read_size;
					}
					else
						DEF_LOG_RESULT(Util_queue_add, false, result);
				}
				else
					DEF_LOG_RESULT(Ftpd_io_res_read_init, false, result);

				free(buffer);
				buffer = NULL;
			}
			else
				DEF_LOG_RESULT(calloc, false, 0);

			if(res)
			{
				Ftpd_io_res_read_exit(res);
				free(res);
				res = NULL;
			}

			//Delete queue if we've reached EOF or there was an error.
			if(is_eof || is_error)
				Ftpd_read_state_clear(&fs->read[i]);
		}
	}
}

static void Ftpd_io_process_cmd_read(Ftpd_fs* fs, Ftpd_io_cmd_read* cmd_read)
{
	bool is_error = true;
	uint8_t index = UINT8_MAX;
	uint32_t result = DEF_ERR_OTHER;

	//Register it.
	for(uint8_t i = 0; i < MAX_READ_STATE; i++)
	{
		if(!fs->read[i].is_used)
		{
			index = i;
			fs->read[i].is_used = true;
			fs->read[i].client_index = cmd_read->client_index;
			fs->read[i].offset = 0;

			result = Util_str_set(&fs->read[i].dir, DEF_STR_NEVER_NULL(&cmd_read->dir));
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_set, false, result);
				break;
			}

			result = Util_str_set(&fs->read[i].file_name, DEF_STR_NEVER_NULL(&cmd_read->file_name));
			if(result != DEF_SUCCESS)
			{
				DEF_LOG_RESULT(Util_str_set, false, result);
				break;
			}

			is_error = false;
			break;
		}
	}

	//Send the result if registration was unsuccessful.
	if(is_error)
	{
		Ftpd_io_res_read* res = (Ftpd_io_res_read*)calloc(1, sizeof(Ftpd_io_res_read));

		if(index < MAX_READ_STATE)
			Ftpd_read_state_clear(&fs->read[index]);

		if(res)
		{
			result = Ftpd_io_res_read_init(res, true, false, cmd_read->client_index, NULL, 0, DEF_STR_NEVER_NULL(&cmd_read->file_name), DEF_STR_NEVER_NULL(&cmd_read->dir));
			if(result == DEF_SUCCESS)
			{
				//Send result to server thread.
				result = Util_queue_add(&ftpd.server_thread_queue, IO_RES_READ, res, DEF_UTIL_S_TO_US(60), QUEUE_OPTION_NONE);
				if(result == DEF_SUCCESS)
				{
					//res will be freed in server thread.
					res = NULL;
				}
				else
					DEF_LOG_RESULT(Util_queue_add, false, result);
			}
			else
				DEF_LOG_RESULT(Ftpd_io_res_read_init, false, result);
		}
		else
			DEF_LOG_RESULT(calloc, false, 0);

		if(res)
		{
			Ftpd_io_res_read_exit(res);
			free(res);
			res = NULL;
		}
	}

	Ftpd_io_cmd_read_exit(cmd_read);
}

static void Ftpd_io_process_cmd_write(Ftpd_fs* fs, Ftpd_io_cmd_write* cmd_write)
{
	(void)fs;
	uint32_t result = DEF_ERR_OTHER;

	//Write it.
	if(cmd_write->buffer && cmd_write->buffer_size > 0)
	{
		result = Util_file_save_to_file(DEF_STR_NEVER_NULL(&cmd_write->file_name), DEF_STR_NEVER_NULL(&cmd_write->dir),
		cmd_write->buffer, cmd_write->buffer_size, !cmd_write->is_append);
		if(result != DEF_SUCCESS)
			DEF_LOG_RESULT(Util_file_save_to_file, false, result);
	}
	else
		result = DEF_SUCCESS;//Nothing to write.

	//Send the result if writing was unsuccessful or we've reached EOF.
	if(cmd_write->is_eof || result != DEF_SUCCESS)
	{
		Ftpd_io_res_write* res = (Ftpd_io_res_write*)calloc(1, sizeof(Ftpd_io_res_write));

		if(res)
		{
			result = Ftpd_io_res_write_init(res, (result != DEF_SUCCESS), cmd_write->client_index, DEF_STR_NEVER_NULL(&cmd_write->file_name), DEF_STR_NEVER_NULL(&cmd_write->dir));
			if(result == DEF_SUCCESS)
			{
				//Send result to server thread.
				result = Util_queue_add(&ftpd.server_thread_queue, IO_RES_WRITE, res, DEF_UTIL_S_TO_US(60), QUEUE_OPTION_NONE);
				if(result == DEF_SUCCESS)
				{
					//res will be freed in server thread.
					res = NULL;
				}
				else
					DEF_LOG_RESULT(Util_queue_add, false, result);
			}
			else
				DEF_LOG_RESULT(Ftpd_io_res_write_init, false, result);
		}
		else
			DEF_LOG_RESULT(calloc, false, 0);

		if(res)
		{
			Ftpd_io_res_write_exit(res);
			free(res);
			res = NULL;
		}
	}

	Ftpd_io_cmd_write_exit(cmd_write);
}

static void Ftpd_io_thread(void* arg)
{
	(void)arg;
	DEF_LOG_STRING("Thread started.");
	uint32_t result = DEF_ERR_OTHER;
	Ftpd_fs fs = { 0, };

	result = Ftpd_io_init(&fs, IO_BUFFER_SIZE);
	if(result != DEF_SUCCESS)
	{
		DEF_LOG_RESULT(Ftpd_io_init, false, result);
		goto error;
	}

	while (ftpd.thread_run)
	{
		uint32_t id = 0;
		uint32_t timeout_us = DEF_THREAD_ACTIVE_SLEEP_TIME;
		void* data = NULL;

		//Process read queue first.
		Ftpd_io_process_read(&fs);

		//Check read queue state again.
		for(uint8_t i = 0; i < MAX_READ_STATE; i++)
		{
			if(fs.read[i].is_used)
			{
				//We want to read asap again.
				timeout_us = 0;
				break;
			}
		}

		result = Util_queue_get(&ftpd.io_thread_queue, &id, &data, timeout_us);
		if(result == DEF_ERR_TRY_AGAIN)
			continue;
		else if(result != DEF_SUCCESS)
		{
			DEF_LOG_RESULT(Util_queue_get, false, result);
			break;
		}

		if(data)
		{
			switch (id)
			{
				case IO_CMD_READ:	{ Ftpd_io_process_cmd_read(&fs, (Ftpd_io_cmd_read*)data);		break; }
				case IO_CMD_WRITE:	{ Ftpd_io_process_cmd_write(&fs, (Ftpd_io_cmd_write*)data);		break; }
				default:			{ break; }
			}
		}

		free(data);
		data = NULL;
	}

	error:
	Ftpd_io_exit(&fs);

	DEF_LOG_STRING("Thread exit.");
	threadExit(0);
}
