#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

/***********************
 ****   CONSTANTS   ****
 ***********************/
/* General Flags */
#define CHILD_PROCESS 0        // Fork() Child Process Identifier
#define EXIT_ON_ERROR 1        // Call exit() if an error occurred
#define HOST_BIND_SUCCESSFUL 0 // bind() was successful
#define INTENDED_ERROR 1       // Intentionally created error status
#define INVALID_FILE_DESC -1   // Unusable File Descriptor
#define SUCCESS 0              // Function executed without errors

/* Status Messages */
#define RECEIVED_CLIENT_MSG "Client message was received\n"

/* Program Default Settings */
#define DEFAULT_PORT "10000"  // Default Server Port
#define DEFAULT_DIR  "PWD"    // Default Server Directory
#define MAX_PORT_LENGTH 6     // Max length of port argument
#define MAX_PATH_LENGTH 99999 // Max length of path argument
#define MAX_CLIENT_MSG_LENGTH 99999 // Largest allowed client message
#define MAX_CLIENT_CONNECTIONS 1000 // Largest amount of concurrent clients being served
#define MAX_PENDING_CLIENT_CONNECTIONS 10000 // Largest number of clients waiting

/* HTTP Responses and Request Types */
#define HTTP_400_RES "HTTP/1.0 400 Bad Request\n"
#define HTTP_200_RES "HTTP/1.0 200 OK\n\n"
#define GET_REQUEST "GET\0"

/* Valid HTTP Protocols. */
#define HTTP_1_0 "HTTP/1.0"
#define HTTP_1_1 "HTTP/1.1"

/* Error Messages */
#define INITIALIZE_SERVER_ERR_MSG  "initialize_server() error"
#define START_HOST_ERR_MSG         "start_host_listening() error"
#define HANDLE_CONNECTIONS_ERR_MSG "handle_connections() error"
#define GET_ADDR_INFO_ERR_MSG      "getaddrinfo() error"
#define HOST_ADDR_INFO_ERR_MSG     "host_address_info() error"
#define NO_VALID_BIND_ERR_MSG      "socket() or bind() error"
#define LISTEN_ERR_MSG             "listen() error"
#define ACCEPT_ERR_MSG             "accept() error"
#define RESPOND_TO_CLIENT_ERR_MSG  "respond_to_client() error"
#define RECV_ERR_MSG               "recv() error in child\n"
#define CLIENT_DISCONNECT_ERR_MSG  "Client disconnected\n"
#define OPEN_FILE_ERR_MSG          "open() error"
#define INVALID_REQ_METHOD_ERR_MSG "Only GET requests allowed"

/***********************
 ****   FUNCTIONS   ****
 ***********************/
/**
 * Checks if passed error status reflects an error, and prints the
 * error (using perror) with the message err_message. The function
 * then exits the program with status failure.
 *
 * int   err_status  - Error status code to check
 * char* err_message - Error message to use with perror
 * int   exit_on_err - If set, then exit the program on an error
 *
 * Returns (err_status && exit_on_err)
 */
int check_error(int err_status, char* err_message, int exit_on_err);

/**
 * Sets the addrinfo struct pointed to by result_address to be the
 * result of calling getaddrinfo() with the specified settings that
 * are defined in the constant above.
 *
 * char* host - Host used with getaddrinfo
 * char* port - Port used with getaddrinfo
 * struct addrinfo **result_address - Pointer for storing result of
 * getaddrinfo
 */
int host_address_info(char* host, char* port, struct addrinfo **result_address);

int valid_req_method(char* request_method);
/**
 * Determines if the request message received by the client was made
 * using a valid HTTP protocol or not. The valid protocols are
 * "HTTP/1.0" and "HTTP/1.1"
 *
 * char* request_protocol - Protocol specified as being used by the
 * client
 */
int valid_http_protocol(char* request_protocol);

/**
 * Performs setup for the server before any sockets are actually
 * bound. Currently prints out a server starting message, and
 * initializes client file descriptors to be -1
 *
 * char* port - Port used for server
 */
int initialize_server(char* port);

/**
 * First finds and bind()'s an address to use for the host on
 * a socket. Then setups listening for connections by the host
 * on the socket indicated by the file descriptor.
 */
int start_host_listening();

/**
 * Houses the main process' request loop that continuously checks
 * for incoming client connections and then spawns child processes
 * to respond to the requests.
 */
int handle_connections();

/**
 * Parses and replies to the request recieved from the client that
 * is identified by the passed file descriptor.
 *
 * int client_fd - File descriptor of client making the request
 */
int respond_to_client(int client_fd);

/**
 * Parses the Client's file request and handles it appropriately
 */
int respond_with_file(int client_fd, char* request_method, char* url_path);