/**
 * ==========================================================
 * @author Valentyn Vorobec
 * login: xvorob02
 * email: xvorob02@stud.fit.vutbr.cz
 * date: 2024
 * ==========================================================
 */

#include "ipk24chat-server.h"

/**
 * @brief Ctrl + C/Ctrl + D
 * 
 * @param signum 
 */
void handle_interrupt(int signum) 
{
    (void)signum;
    received_signal = 1;
}

/**
 * @brief prints help
 * 
 */
void print_help()
{
    fprintf(stdout, 
            "Usage: ./ipk24-chat-server -l <IPv4 address> -p <port> -d <number> -r <number> -h\n"
            "Argument    | Default alue  | Possible values	        | Description\n"
            "---------------------------------------------------------------------------------------------------------\n"
            "-l          | 0.0.0.        | IPv4 address              | Server listening IP address for welcome sockets\n"
            "-p          | 4567          | uint16	                | Server listening port for welcome sockets\n"
            "-d          | 250           | uint16	                | UDP confirmation timeout\n"
            "-r          | 3	            | uint8                     | Maximum number of UDP retransmissions\n"
            "-h          | 	            |                           | Prints program help output and exits\n\n");
    fflush(stdout);
}

/**
 * @brief Sends messages to all clients in certain channel like when someone leaves or joins or sends MSG.
 * 
 * @param clients list of clients
 * @param sender_socket client's socket, client that is sending message or leaving/joining channel
 * @param channel channel the client is currently in
 * @param message 
 * @param msg_type type of sending message
 * @return int return 1 if error occured, otherwise 0
 */
int send_msg_to_clients(Client *clients, int sender_socket, char *channel, const char *message, enum message_type msg_type)
{
    Client *current = clients;
    while (current != NULL)
    {
        if (current->data.socket != sender_socket && !strcmp(channel, current->data.channel))
        {
            print_addr_port(current->data.socket, "SENT", message_type_enum[msg_type]);
            ssize_t bytes_tx = send(current->data.socket, message, strlen(message), 0);
            if (bytes_tx < 0)
            {
                perror("send");
                return 1;
            }
        }
        current = current->next;
    }
    
    return 0;
}

/**
 * @brief end connection with specific client
 * 
 * @param clients list of clients
 * @param comm_socket client's socket
 * @param epoll_fd 
 * @return int 
 */
void client_end(Client **clients, int comm_socket, int epoll_fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
    {
        perror("epoll_ctl");
    }
    remove_client(clients, comm_socket);
    close(comm_socket);
}

/**
 * @brief End all sessions, free memmory and close all sockets
 * 
 * @param clients list of all currently connected clients
 * @param server_socket_tcp tcp socket
 * @param server_socket_udp udp socket
 * @param epoll_fd 
 */
void end_server(Client **clients, int server_socket_tcp, int server_socket_udp, int epoll_fd)
{
    close(server_socket_tcp);
    close(server_socket_udp);
    Client *current = *clients;
    while (current != NULL)
    {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current->data.socket, NULL) == -1)
        {
            perror("epoll_ctl");
        }
        close(current->data.socket);
        current = current->next;
    }
    free_clients(clients);
}

/**
 * @brief prints IPv4 address and port of receiver or sender.
 * 
 * @param comm_socket socket of client
 * @param recv_send RECV/SENT
 * @param message_type type of message (MSG, BYE, ERR...)
 * @param message_content additional information
 * @return int return 1 if error occured, otherwise 0
 */
int print_addr_port(int comm_socket, char *recv_send, const char *message_type)
{
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    if (getpeername(comm_socket, (struct sockaddr*) &client_addr, &addr_len) == 0)
    {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);
        fprintf(stdout, "%s %s:%d | %s\n", recv_send, client_ip, client_port, message_type);
        fflush(stdout);
        return 0;
    }
    else
    {
        perror("getpeername");
        return 1;
    }
}

/**
 * @brief Proccess udp and tcp. Proccess clients with help of epoll.
 * 
 * @param port
 */
void connect_sockets(char *ip_address, int port)
{
    // CTR + C and Ctr+ D signal
    struct sigaction sa;
    sa.sa_handler = handle_interrupt;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGQUIT, &sa, NULL) == -1)
    {
        fprintf(stderr, "ERR: Setting up signal handler!\n");
        exit(EXIT_FAILURE);
    }

    // UDP socket
    int server_socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket_udp <= 0)
    {
        fprintf(stderr, "ERR: socket!\n");
        exit(EXIT_FAILURE);
    }

    // TCP socket
    int server_socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_tcp <= 0)
    {
        fprintf(stderr, "ERR: socket!\n");
        exit(EXIT_FAILURE);
    }

	int optval = 1;
	if (setsockopt(server_socket_tcp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
	{
        perror("setsocket!\n");
        exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) != 1)
    {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    socklen_t address_size = sizeof(server_addr);

    // UDP binding
    if (bind(server_socket_udp, (struct sockaddr *) &server_addr, address_size) < 0)
    {
        fprintf(stderr, "ERR: bind!\n");
        exit(EXIT_FAILURE);
    }

    // TCP binding
    if (bind(server_socket_tcp, (struct sockaddr *) &server_addr, address_size) < 0)
    {
        fprintf(stderr, "ERR: bind!\n");
        exit(EXIT_FAILURE);
    }

    int max_waiting_connections = 10;
    if (listen(server_socket_tcp, max_waiting_connections) < 0)
    {
        fprintf(stderr, "ERR: listen!\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in comm_addr;
    socklen_t comm_addr_size = sizeof(comm_addr);
    struct sockaddr *address = (struct sockaddr *) &comm_addr;

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // epoll event for tcp
    struct epoll_event event_tcp;
    memset(&event_tcp, 0, sizeof(struct epoll_event));
    event_tcp.events = EPOLLIN;
    event_tcp.data.fd = server_socket_tcp;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_tcp, &event_tcp) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    // epoll event for udp
    struct epoll_event event_udp;
    memset(&event_udp, 0, sizeof(struct epoll_event)); 
    event_udp.events = EPOLLIN;
    event_udp.data.fd = server_socket_udp;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_udp, &event_udp) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    // list of connected clients
    Client *clients = NULL; 

    while (1)
    {
        struct epoll_event events[MAX_EVENTS];
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {   
            end_server(&clients, server_socket_tcp, server_socket_udp, epoll_fd);

            // Ctrl + C | Ctrl + D
            if (received_signal)
            {
                fprintf(stdout, "Received signal, shutting down.\n");
                exit(EXIT_SUCCESS);
            }
            // error
            else
            {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < num_events; i++)
        {
            // client connected to the server with tcp
            if (events[i].data.fd == server_socket_tcp)
            {
                int comm_socket = accept(server_socket_tcp, address, &comm_addr_size);
                if (comm_socket < 0)
                {
                    perror("accept");
                    continue;
                }

                struct epoll_event event;
                memset(&event, 0, sizeof(struct epoll_event));
                event.events = EPOLLIN;
                event.data.fd = comm_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, comm_socket, &event) == -1)
                {
                    perror("epoll_ctl");
                    close(comm_socket);
                    continue;
                }
                
                add_client(&clients, comm_socket, 0);
            } 
            // client connected to the server with udp
            else if (events[i].data.fd == server_socket_udp)
            {
                char buff[BUFFER_SIZE];

                int bytes_rx = recvfrom(server_socket_udp, buff, BUFFER_SIZE, 0, address, &address_size);
                if (bytes_rx < 0)
                {
                    perror("recvfrom");
                    continue;
                }
                
                //fprintf(stdout, "RECV %s:%s | %s %s\n", "{FROM_IP}", "{FROM_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
                //fflush(stdout);

                int bytes_tx = sendto(server_socket_udp, buff, strlen(buff), 0, address, address_size);
                if (bytes_tx < 0)
                {
                    perror("sendto");
                    continue;
                }
                
                //fprintf(stdout, "SENT %s:%s | %s %s\n", "{TO_IP}", "{TO_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
                //fflush(stdout);
            }
            // tcp connection proccess
            else
            {
                int comm_socket = events[i].data.fd;
                char buff[BUFFER_SIZE] = {0};
                ssize_t bytes_rx = recv(comm_socket, buff, BUFFER_SIZE, 0);
                if (bytes_rx <= 0)
                {
                    perror("recv");
                    client_end(&clients, comm_socket, epoll_fd);
                    continue;
                }

                Client *client = search_client(&clients, comm_socket);
                char *channel = strdup(client->data.channel);
                char *response = NULL; 
                enum message_type msg_type = UKNOWN ;
                int result = next_state(clients, client, buff, &response, &msg_type);
                if (result == 1)
                {
                    end_server(&client, server_socket_tcp, server_socket_udp, epoll_fd);
                    exit(EXIT_FAILURE);
                }

                // received UKNOWN message from client
                if (msg_type != UKNOWN)
                {
                    print_addr_port(comm_socket, "RECV", message_type_enum[msg_type]);
                }
                // received BYE from client
                if (result == -2)
                {
                    // if client already use AUTH remove from list
                    if (client->data.username != NULL)
                    {
                        if (response != NULL) free(response);
                        response = NULL;
                        content_left_msg(&response, client->data.display_name, client->data.channel);
                        send_msg_to_clients(clients, comm_socket, client->data.channel, response, MSG);
                    }
                    client_end(&clients, comm_socket, epoll_fd);
                }
                // other messages but not MSG or BYE
                else if (result != -1)
                {
                    ssize_t bytes_tx = send(comm_socket, response, strlen(response), 0);
                    if (bytes_tx < 0)
                    {
                        perror("send");
                        client_end(&clients, comm_socket, epoll_fd);
                        free(channel);
                        continue;
                    }

                    // ERR message was send, now send BYE
                    if (!strncmp(response, "ERR", strlen("ERR")))
                    {
                        print_addr_port(comm_socket, "SENT", message_type_enum[ERR]);
                        if (response != NULL) free(response);
                        response = NULL;
                        if (content_bye(&response))
                        {
                            end_server(&clients, server_socket_tcp, server_socket_udp, epoll_fd);
                            exit(EXIT_FAILURE);
                        }
                        bytes_tx = send(comm_socket, response, strlen(response), 0);
                        if (bytes_tx < 0)
                        {
                            perror("send");
                            client_end(&clients, comm_socket, epoll_fd);
                            free(channel);
                            continue;
                        }

                        print_addr_port(comm_socket, "SENT", message_type_enum[BYE]);

                        if (client->data.display_name != NULL)
                        {
                            if (response != NULL) free(response);
                            response = NULL;
                            if (content_left_msg(&response, client->data.display_name, client->data.channel))
                            {
                                end_server(&clients, server_socket_tcp, server_socket_udp, epoll_fd);
                                exit(EXIT_FAILURE);
                            }
                            send_msg_to_clients(clients, comm_socket, client->data.channel, response, MSG);
                        }
                        client_end(&clients, comm_socket, epoll_fd);
                    }
                    // REPLY message was send
                    else if (!strncmp(response, "REPLY OK", strlen("REPLY OK")))
                    {
                        print_addr_port(comm_socket, "SENT", message_type_enum[REPLY]);
                        if (response != NULL) free(response);
                        response = NULL;
                        if (content_joined_msg(&response, client->data.display_name, client->data.channel))
                        {
                            end_server(&clients, server_socket_tcp, server_socket_udp, epoll_fd);
                            exit(EXIT_FAILURE);
                        }
                        send_msg_to_clients(clients, comm_socket, client->data.channel, response, MSG);

                        if (strcmp(channel, client->data.channel))
                        {
                            if (response != NULL) free(response);
                            response = NULL;
                            if (content_left_msg(&response, client->data.display_name, channel))
                            {
                                end_server(&clients, server_socket_tcp, server_socket_udp, epoll_fd);
                                exit(EXIT_FAILURE);
                            }
                            send_msg_to_clients(clients, comm_socket, channel, response, MSG);
                        }
                    }
                    else if (!strncmp(response, "BYE", strlen("BYE")))
                    {
                        print_addr_port(comm_socket, "SENT", message_type_enum[BYE]);
                    }
                }
                else
                {
                    response = strdup(buff);
                    send_msg_to_clients(clients, comm_socket, client->data.channel, response, MSG);
                }
                free(channel);
                if (response != NULL) free(response);
                response = NULL;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int opt;
    int conf_timeout = DEFAULT_CONF_TIMEOUT;
    int max_num_retransmissions = DEFAULT_MAX_RETRANSMISSIONS;
    (void) max_num_retransmissions;
    int port = DEFAULT_SERVER_PORT;
    char *ip_addr = DEFAULT_SERVER_IP;

    while ((opt = getopt(argc, argv, "l:p:d:r:h")) != -1) 
    {
        switch (opt)
        {
            case 'l':
                if (strcmp(optarg, "localhost"))
                    ip_addr = optarg;
                break;
            case 'p':
                if (atoi(optarg) <= 0 || atoi(optarg) > 65535) 
                {
                    fprintf(stderr, "ERR: Invalid port number! Port must be between 1 and 65535!\n");
                    exit(EXIT_FAILURE);
                }
                port = atoi(optarg);
                break;
            case 'd':
                conf_timeout = atoi(optarg);
                if (conf_timeout <= 0) 
                {
                    fprintf(stderr, "ERR: Invalid UDP confirmation timeout! Must be a positive integer!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'r':
                max_num_retransmissions = atoi(optarg);
                break;
            case 'h':
                print_help();
                exit(0);
            default:
                fprintf(stderr, "ERR: Use './ipk24chat-server -h' for help!\n");
                exit(EXIT_FAILURE);
        }
    }

    connect_sockets(ip_addr, port);
    return EXIT_SUCCESS;
}