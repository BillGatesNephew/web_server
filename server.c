#include "server.h"

/* Global Varaibles */
char *server_dir;                       // Root directory of the web server
char server_port[MAX_PORT_LENGTH];      // Port the server is running on
int client_fds[MAX_CLIENT_CONNECTIONS]; // File Descriptors for every possible client connection
int host_fd;                            // File Descriptor representing the Host

int main(int argc, char** argv)
{
    int flag, err;
    extern char *optarg;

    /* Set Default Values for Server Port and Directory */
    server_dir = getenv(DEFAULT_DIR);
    strcpy(server_port, DEFAULT_PORT);

    /* Parse Input Arguments */
    while ((flag = getopt(argc, argv, "d:p:")) != -1)
    {
        switch(flag) {
            case 'd':   // Server Directory Path
                server_dir = malloc(strlen(optarg) + 1);
                strcpy(server_dir, optarg);
                break;
            case 'p':   // Server Port Number
                strncpy(server_port, optarg, sizeof(server_port));
                break;
        }
    }

    /* Initialize the Server */
    err = initialize_server(server_port);
    check_error(err, INITIALIZE_SERVER_ERR_MSG, EXIT_ON_ERROR);

    /* Start the Host Listening */
    err = start_host_listening();
    check_error(err, START_HOST_ERR_MSG, EXIT_ON_ERROR);

    /* Begin Accepting Connections */
    err = handle_connections();
    check_error(err, HANDLE_CONNECTIONS_ERR_MSG, EXIT_ON_ERROR);

    free(server_dir);
    exit(EXIT_SUCCESS);
}

int check_error(int err_status, char* err_message, int exit_on_err)
{
    if(err_status && exit_on_err) {
        perror(err_message);
        free(server_dir);
        exit(EXIT_FAILURE);
    } else if(err_status) {
        perror(err_message);
    }
    return err_status;
}

int host_address_info(char* host, char* port, struct addrinfo **result_address)
{
    int err;
    struct addrinfo socket_hints;
    /* Set Hints for Determining Socket Address */
    memset(&socket_hints, 0, sizeof(struct addrinfo));
    socket_hints.ai_family   = AF_UNSPEC;   // Allow either IPv4 or IPv6 addressess
    socket_hints.ai_socktype = SOCK_STREAM; // Use two-way byte stream connection
    socket_hints.ai_flags    = AI_PASSIVE;  // Caller intends to bind() with returned addresses
    socket_hints.ai_protocol = 0;           // Use any protocol type

    /* Determine Socket Address */
    err = getaddrinfo(host, port, &socket_hints, result_address);
    return check_error(err, GET_ADDR_INFO_ERR_MSG, !EXIT_ON_ERROR);
}

int valid_req_method(char* request_method)
{
    return strncmp(request_method, GET_REQUEST, 4) == 0;
}

int valid_http_protocol(char* request_protocol)
{
    return strncmp(request_protocol, HTTP_1_0, 8) == 0 || strncmp(request_protocol, HTTP_1_1, 8) == 0;
}

int initialize_server(char* port)
{
    int i;
    /* Print Starting Message */
    printf("Server being spun up for directory '%s' on port '%s'\n", server_dir, port);

    /* Initialize Client File Descriptors */
    for(i = 0; i < MAX_CLIENT_CONNECTIONS; i++)
        client_fds[i] = INVALID_FILE_DESC;
    return SUCCESS;
}

int start_host_listening()
{
    int err, err_occurred;
    struct addrinfo *host_address, *p;
    /* Retrieve the Internet Address Info for the Host */
    err = host_address_info(NULL, server_port, &host_address);
    err_occurred = check_error(err, HOST_ADDR_INFO_ERR_MSG, !EXIT_ON_ERROR);

    /* Bind the Host to host_fd  */
    for(p = host_address; p != NULL; p = p->ai_next) // Iterates over potential host addresses
    {
        // Attempts to create a socket endpoint with the current address
        host_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        // Skip bind() attempt if address could not be used for socket
        if(host_fd != INVALID_FILE_DESC){
            // Attempts to bind() the address to the socket
            if(bind(host_fd, p->ai_addr, p->ai_addrlen) == HOST_BIND_SUCCESSFUL)
                break;
            close(host_fd);
        }
    }

    /* Error Checking and Memory Cleanup */
    err = (p == NULL);
    err_occurred = check_error(err, NO_VALID_BIND_ERR_MSG, !EXIT_ON_ERROR) || err_occurred;
    freeaddrinfo(host_address);

    /* Start Listening for Incoming Connections */
    // Listen for connections on the host's file descriptor, and allow
    // a maximum of MAX_PENDING_CLIENT_CONNECTIONS connections to be waiting
    err = listen(host_fd, MAX_PENDING_CLIENT_CONNECTIONS);
    err_occurred = check_error(err, LISTEN_ERR_MSG, !EXIT_ON_ERROR) || err_occurred;
    return err_occurred;
}

int handle_connections()
{
    int err, client_fds_ind;
    struct sockaddr_in client_sock_addr;
    socklen_t request_addr_len;
    /* Check for Connection Requests */
    client_fds_ind = 0;
    while(1)
    {
        // Attempt to accept an incoming client request
        request_addr_len = sizeof(client_sock_addr);
        client_fds[client_fds_ind] = accept(host_fd, (struct sockaddr *)&client_sock_addr, &request_addr_len);
        if(client_fds[client_fds_ind] == INVALID_FILE_DESC) {
            return check_error(INTENDED_ERROR, ACCEPT_ERR_MSG, !EXIT_ON_ERROR);
        }
        // Create a child process to respond to the client's request
        if(fork() == CHILD_PROCESS) {
            err = respond_to_client(client_fds[client_fds_ind]);
            shutdown(client_fds[client_fds_ind], SHUT_RDWR);
            close(client_fds[client_fds_ind]);
            client_fds[client_fds_ind] = INVALID_FILE_DESC;
            return check_error(err, RESPOND_TO_CLIENT_ERR_MSG, !EXIT_ON_ERROR);
        }
        // Cycle to next available client connection file descriptor
        while(client_fds[client_fds_ind] != INVALID_FILE_DESC)
            client_fds_ind = (client_fds_ind + 1) % MAX_CLIENT_CONNECTIONS;
    }
    return SUCCESS;
}

int respond_to_client(int client_fd)
{
    int client_msg_length;
    char client_msg[MAX_CLIENT_MSG_LENGTH];
    char* request_method;
    char* url_path;
    char* http_protocol;
    /* Read the Client's Message and Handle Appropriately */
    memset((void*) client_msg, (int) '\0', MAX_CLIENT_MSG_LENGTH);
    client_msg_length = recv(client_fd, client_msg, MAX_CLIENT_MSG_LENGTH, 0);
    if(client_msg_length < 0) {
        fprintf(stderr, RECV_ERR_MSG);
    } else if (client_msg_length == 0) {
        fprintf(stderr, CLIENT_DISCONNECT_ERR_MSG);
    } else {
        printf(RECEIVED_CLIENT_MSG);
        request_method = strtok(client_msg, " \t\n"); // Request method
        url_path       = strtok(NULL, " \t");         // URL path requested
        http_protocol  = strtok(NULL, " \t\n");       // HTTP protocol used
        if(valid_http_protocol(http_protocol)) {
            respond_with_file(client_fd, request_method, url_path);
        } else {
            write(client_fd, HTTP_400_RES, 25);
        }
    }
    return SUCCESS;
}

int respond_with_file(int client_fd, char* request_method, char* url_path)
{
    int requested_file_fd, bytes_read;
    char path[MAX_PATH_LENGTH], data_to_send[1024], php_cmd[strlen(url_path) + 7];;
    memset((void*) php_cmd, (int) '\0', strlen(url_path) + 7);

    /* Validate Client's Request is GET Request */
    if(!valid_req_method(request_method))
        return check_error(INTENDED_ERROR, INVALID_REQ_METHOD_ERR_MSG, !EXIT_ON_ERROR);

    /* Create Requested Filepath */
    // '/' Root URL path needs special handling
    if(strlen(url_path) == 1) url_path = "/index.html";
    strcpy(path, server_dir);
    strcpy(&path[strlen(server_dir)], url_path);
    printf("The file: '%s' was requested\n", path);

    /* Check for PHP Extension */
    FILE* php_file;
    if(strlen(url_path) > 5 && strncmp(".php", url_path + (strlen(url_path) - 4), 4) == 0) {
        strcat(php_cmd, "php -f ");
        strcat(php_cmd, path);
        php_file = popen(php_cmd, "r");
        send(client_fd, HTTP_200_RES, 17, 0);
        while (fgets(data_to_send, 1024, php_file) != NULL)
            write(client_fd, data_to_send, strlen(data_to_send));
        fclose(php_file);
        return SUCCESS;
    }

    /* Open Requested File */
    if((requested_file_fd = open(path, O_RDONLY)) == INVALID_FILE_DESC)
        return check_error(INTENDED_ERROR, OPEN_FILE_ERR_MSG, !EXIT_ON_ERROR);
    send(client_fd, HTTP_200_RES, 17, 0);
    while((bytes_read = read(requested_file_fd, data_to_send, 1024)) > 0)
        write(client_fd, data_to_send, bytes_read);
    return SUCCESS;
}