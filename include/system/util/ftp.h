#if !defined(DEF_FTP_H)
#define DEF_FTP_H
#include <stdbool.h>
#include <stdint.h>
#include "system/util/ftp_types.h"
#include "system/util/str_types.h"

#if DEF_FTP_API_ENABLE

/**
 * @brief Receive a FTP command via control connection.
 * @param socket_handle (in) Socket handle for control connection.
 * @param command (out) Received command, this will be FTP_COMMAND_INVALID if unknown FTP command was received.
 * @param arg (out) Arg for command (e.g. if raw command was "USER anonymous<CRLF>", this will be "anonymous<NULL>").
 * @param arg_size (in) Size of arg.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_receive_command(int32_t socket_handle, Ftp_command* command, char* arg, uint32_t arg_size);

/**
 * @brief Send a FTP response via control connection.
 * @param socket_handle (in) Socket handle for control connection.
 * @param response (in) Response code.
 * @param message (in) Message after status code (e.g. if this was "OK<NULL>", raw response will be "[response code] OK<CRLF>").
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_send_response(int32_t socket_handle, Ftp_response response, const char* message);

/**
 * @brief Receive data via data connection.
 * @param socket_handle (in) Socket handle for data connection.
 * @param buffer (out) Received data.
 * @param buffer_size (in) Buffer size.
 * @param received_size (out) Received size.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_receive_data(int32_t socket_handle, uint8_t* buffer, uint32_t buffer_size, uint32_t* received_size);

/**
 * @brief Send data via data connection.
 * @param socket_handle (in) Socket handle for data connection.
 * @param buffer (out) Data to be sent.
 * @param buffer_size (in) Buffer size.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_send_data(int32_t socket_handle, const uint8_t* buffer, uint32_t size);

/**
 * @brief Get the path.
 * @param current_dir (in) Current working directory.
 * @param cmd_path (in) New path requested via command parameter.
 * @param dir (out) Directory name.
 * @param file (out) Filename, can be NULL and cmd_path is assumed to contain directory name in that case.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_get_path(const char* current_dir, const char* cmd_path, Str_data* dir, Str_data* file);

/**
 * @brief Get unique filename.
 * @param dir (in) Directory.
 * @param filename_base (in) Base filename to start with.
 * @param filename (out) Unique filename.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_get_unique_filename(const char* dir, const char* filename_base, Str_data* filename);

/**
 * @brief Accept incoming control connection.
 * @param clients (in) Target clients.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_accept_control_connection(Ftp_clients* clients);

/**
 * @brief Accept incoming data connection.
 * @param client (in) Target client.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_accept_data_connection(Ftp_client* client);

/**
 * @brief Listen to incoming data connection.
 * @param client (in) Target client.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_listen_data_connection(Ftp_client* client);

/**
 * @brief Open data connection.
 * @param client (in) Target client.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_connect_data_connection(Ftp_client* client);

/**
 * @brief Process USER command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_user(Ftp_client* client);

/**
 * @brief Process PASS command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_pass(Ftp_client* client);

/**
 * @brief Process ACCT command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_acct(Ftp_client* client);

/**
 * @brief Process CWD command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_cwd(Ftp_client* client);

/**
 * @brief Process CDUP command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_cdup(Ftp_client* client);

/**
 * @brief Process SMNT command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_smnt(Ftp_client* client);

/**
 * @brief Process REIN command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_rein(Ftp_client* client);

/**
 * @brief Process QUIT command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_quit(Ftp_client* client);

/**
 * @brief Process PORT command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_port(Ftp_client* client);

/**
 * @brief Process PASV command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_pasv(Ftp_client* client);

/**
 * @brief Process TYPE command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_type(Ftp_client* client);

/**
 * @brief Process STRU command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stru(Ftp_client* client);

/**
 * @brief Process MODE command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_mode(Ftp_client* client);

/**
 * @brief Process RETR command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_retr(Ftp_client* client);

/**
 * @brief Process STOR command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stor(Ftp_client* client);

/**
 * @brief Process STOU command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stou(Ftp_client* client);

/**
 * @brief Process APPE command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_appe(Ftp_client* client);

/**
 * @brief Process ALLO command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_allo(Ftp_client* client);

/**
 * @brief Process REST command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_rest(Ftp_client* client);

/**
 * @brief Process RNFR command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_rnfr(Ftp_client* client);

/**
 * @brief Process RNTO command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_rnto(Ftp_client* client);

/**
 * @brief Process ABOR command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_abor(Ftp_client* client);

/**
 * @brief Process DELE command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_dele(Ftp_client* client);

/**
 * @brief Process RMD command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_rmd(Ftp_client* client);

/**
 * @brief Process MKD command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_mkd(Ftp_client* client);

/**
 * @brief Process PWD command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_pwd(Ftp_client* client);

/**
 * @brief Process LIST command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_list(Ftp_client* client);

/**
 * @brief Process NLST command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_nlst(Ftp_client* client);

/**
 * @brief Process SITE command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_site(Ftp_client* client);

/**
 * @brief Process SYST command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_syst(Ftp_client* client);

/**
 * @brief Process STAT command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stat(Ftp_client* client);

/**
 * @brief Process HELP command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_help(Ftp_client* client);

/**
 * @brief Process NOOP command.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_noop(Ftp_client* client);

/**
 * @brief Process RETR command for BOF.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_retr_bof(Ftp_client* client);

/**
 * @brief Process LIST command for BOF.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_list_bof(Ftp_client* client);

/**
 * @brief Process NLST command for BOF.
 * This won't send the response internally, therefore you need to call 
 * Ftp_send_response(client.control_handle.fd, client.response, client.message_buffer); to send the response.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_nlst_bof(Ftp_client* client);

/**
 * @brief Process STOR command for data.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stor_data(Ftp_client* client);

/**
 * @brief Process STOU command for data.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stou_data(Ftp_client* client);

/**
 * @brief Process APPE command for data.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_appe_data(Ftp_client* client);

/**
 * @brief Process RETR command for EOF.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_retr_eof(Ftp_client* client);

/**
 * @brief Process STOR command for EOF.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stor_eof(Ftp_client* client);

/**
 * @brief Process STOU command for EOF.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_stou_eof(Ftp_client* client);

/**
 * @brief Process APPE command for EOF.
 * @param client (in) Target client.
 * @note Thread safe.
*/
void Ftp_process_appe_eof(Ftp_client* client);

/**
 * @brief Initialize clients.
 * @param clients (out) Clients to initialize.
 * @param num_of_clients (in) Maximum number of clients.
 * @param control_port (in) Port for control connection.
 * @param data_port_base (in) Base port for data connection (data_port_base + 0, data_port_base + 1, data_port_base + n...).
 * @param store_buffer_size (in) Buffer size for STOR/STOU/APPE commands.
 * @param dir_content_max_files (in) Maximum number of files for LIST/NLST commands.
 * @return On success DEF_SUCCESS, on failure DEF_ERR_*.
 * @note Thread safe.
*/
uint32_t Ftp_clients_init(Ftp_clients* clients, uint8_t num_of_clients, uint16_t control_port, uint16_t data_port_base, uint32_t store_buffer_size, uint32_t dir_content_max_files);

/**
 * @brief Disconnect and clear clients data.
 * @param clients (out) Clients to clear.
 * @note Thread safe.
*/
void Ftp_clients_clear(Ftp_clients* clients);

/**
 * @brief Disconnect and clear client data.
 * @param client (out) Client to clear.
 * @note Thread safe.
*/
void Ftp_client_clear(Ftp_client* client);

/**
 * @brief Clear data for current directory.
 * @param current_dir (out) Data to clear.
 * @note Thread safe.
*/
void Ftp_current_dir_clear(Ftp_current_dir* current_dir);

/**
 * @brief Clear data for LIST/NLST commands.
 * @param list (out) Data to clear.
 * @note Thread safe.
*/
void Ftp_list_clear(Ftp_list* list);

/**
 * @brief Clear data for RNFR/RNTO commands.
 * @param rename (out) Data to clear.
 * @note Thread safe.
*/
void Ftp_rename_clear(Ftp_rename* rename);

/**
 * @brief Clear data for RETR command.
 * @param retrieve (out) Data to clear.
 * @note Thread safe.
*/
void Ftp_retrieve_clear(Ftp_retrieve* retrieve);

/**
 * @brief Clear data for STOR/STOU/APPE commands.
 * @param store (out) Data to clear.
 * @note Thread safe.
*/
void Ftp_store_clear(Ftp_store* store);

/**
 * @brief Clear data for directory contents.
 * @param dir_content (out) Data to clear.
 * @note Thread safe.
*/
void Ftp_dir_content_clear(Ftp_dir_content* dir_content);

/**
 * @brief Stop listening to incoming connections, disconnect clients and free clients data.
 * @param clients (out) Clients to free.
 * @note Thread safe.
*/
void Ftp_clients_exit(Ftp_clients* clients);

/**
 * @brief Disconnect client connection.
 * @param client (in) Target client.
 * @param is_control (in) Whether close control connection.
 * @param is_force (in) Whether forcibly close connection by sending RST.
 * @note Thread safe.
*/
void Ftp_close_socket(Ftp_client* client, bool is_control, bool is_force);

#else

#define Ftp_receive_command(...) DEF_ERR_DISABLED
#define Ftp_send_response(...) DEF_ERR_DISABLED
#define Ftp_receive_data(...) DEF_ERR_DISABLED
#define Ftp_send_data(...) DEF_ERR_DISABLED
#define Ftp_get_path(...) DEF_ERR_DISABLED
#define Ftp_get_unique_filename(...) DEF_ERR_DISABLED
#define Ftp_accept_control_connection(...) DEF_ERR_DISABLED
#define Ftp_accept_data_connection(...) DEF_ERR_DISABLED
#define Ftp_listen_data_connection(...) DEF_ERR_DISABLED
#define Ftp_connect_data_connection(...) DEF_ERR_DISABLED
#define Ftp_process_user(...)
#define Ftp_process_pass(...)
#define Ftp_process_acct(...)
#define Ftp_process_cwd(...)
#define Ftp_process_cdup(...)
#define Ftp_process_smnt(...)
#define Ftp_process_rein(...)
#define Ftp_process_quit(...)
#define Ftp_process_port(...)
#define Ftp_process_pasv(...)
#define Ftp_process_type(...)
#define Ftp_process_stru(...)
#define Ftp_process_mode(...)
#define Ftp_process_retr(...)
#define Ftp_process_stor(...)
#define Ftp_process_stou(...)
#define Ftp_process_appe(...)
#define Ftp_process_allo(...)
#define Ftp_process_rest(...)
#define Ftp_process_rnfr(...)
#define Ftp_process_rnto(...)
#define Ftp_process_abor(...)
#define Ftp_process_dele(...)
#define Ftp_process_rmd(...)
#define Ftp_process_mkd(...)
#define Ftp_process_pwd(...)
#define Ftp_process_list(...)
#define Ftp_process_nlst(...)
#define Ftp_process_site(...)
#define Ftp_process_syst(...)
#define Ftp_process_stat(...)
#define Ftp_process_help(...)
#define Ftp_process_noop(...)
#define Ftp_process_retr_bof(...)
#define Ftp_process_list_bof(...)
#define Ftp_process_nlst_bof(...)
#define Ftp_process_stor_data(...)
#define Ftp_process_stou_data(...)
#define Ftp_process_appe_data(...)
#define Ftp_process_retr_eof(...)
#define Ftp_process_stor_eof(...)
#define Ftp_process_stou_eof(...)
#define Ftp_process_appe_eof(...)
#define Ftp_clients_init(...) DEF_ERR_DISABLED
#define Ftp_clients_clear(...)
#define Ftp_client_clear(...)
#define Ftp_current_dir_clear(...)
#define Ftp_list_clear(...)
#define Ftp_rename_clear(...)
#define Ftp_retrieve_clear(...)
#define Ftp_store_clear(...)
#define Ftp_dir_content_clear(...)
#define Ftp_clients_exit(...)
#define Ftp_close_socket(...)

#endif //DEF_FTP_API_ENABLE

#endif //!defined(DEF_FTP_H)
