/**
 * ==========================================================
 * @author Valentyn Vorobec
 * login: xvorob02
 * email: xvorob02@stud.fit.vutbr.cz
 * date: 2024
 * ==========================================================
 */

#include "ipk24chat-server.h"

void handle_interrupt(int signum) 
{
    (void)signum;
    received_signal = 1;
}

void print_help()
{

}

int send_msg_to_clients(Client *clients, int sender_socket, char *channel, const char *message)
{
    Client *current = clients;
    while (current != NULL)
    {
        if (current->data.socket != sender_socket && !strcmp(channel, current->data.channel))
        {
            
            fprintf(stdout, "SENT %s:%s | %s %s\n", "{TO_IP}", "{TO_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
            fflush(stdout);
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

void connect_sockets(int port)
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
        fprintf(stderr, "ERR: setsocket!\n");
        exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr =  htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

    int address_size = sizeof(server_addr);

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

    struct epoll_event event_tcp;
    memset(&event_tcp, 0, sizeof(struct epoll_event));
    event_tcp.events = EPOLLIN;
    event_tcp.data.fd = server_socket_tcp;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket_tcp, &event_tcp) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

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
            Client *current = clients;
            while (current != NULL)
            {
                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current->data.socket, NULL) == -1)
                {
                    perror("epoll_ctl");
                }
                close(current->data.socket);
                current = current->next;
            }
            close(server_socket_tcp);
            close(server_socket_udp);
            free_clients(&clients);

            if (received_signal)
            {
                fprintf(stdout, "Received signal, shutting down.\n");
                exit(EXIT_SUCCESS);
            }
            else
            {
                perror("epoll_wait");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < num_events; i++)
        {
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
            else if (events[i].data.fd == server_socket_udp)
            {
                char buff[BUFFER_SIZE];

                int bytes_rx = recvfrom(server_socket_udp, buff, BUFFER_SIZE, 0, address, &address_size);
                if (bytes_rx < 0)
                {
                    perror("recvfrom");
                    continue;
                }
                
                fprintf(stdout, "RECV %s:%s | %s %s\n", "{FROM_IP}", "{FROM_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
                fflush(stdout);

                int bytes_tx = sendto(server_socket_udp, buff, strlen(buff), 0, address, address_size);
                if (bytes_tx < 0)
                {
                    perror("sendto");
                    continue;
                }
                
                fprintf(stdout, "SENT %s:%s | %s %s\n", "{TO_IP}", "{TO_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
                fflush(stdout);
            }
            else
            {
                int comm_socket = events[i].data.fd;
                char buff[BUFFER_SIZE] = {0};
                ssize_t bytes_rx = recv(comm_socket, buff, BUFFER_SIZE, 0);
                if (bytes_rx < 0)
                {
                    perror("recv");
                    // remove client from client list and close socket
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
                    {
                        perror("epoll_ctl");
                    }
                    remove_client(&clients, comm_socket);
                    close(comm_socket);
                    continue;
                } 
                else if (bytes_rx == 0)
                {
                    // remove client from client list and close socket
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
                    {
                        perror("epoll_ctl");
                    }
                    remove_client(&clients, comm_socket);
                    close(comm_socket);
                    continue;
                }
             
                char *response = NULL; 
                int result = next_state(&clients, comm_socket, buff, &response);
                if (result == 1)
                {
                    close(server_socket_tcp);
                    close(server_socket_udp);
                    Client *current = clients;
                    while (current != NULL)
                    {
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current->data.socket, NULL) == -1)
                        {
                            perror("epoll_ctl");
                        }
                        close(current->data.socket);
                        current = current->next;
                    }
                    free_clients(&clients);
                    exit(EXIT_FAILURE);
                }

                fprintf(stdout, "RECV %s:%s | %s %s\n", "{FROM_IP}", "{FROM_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
                fflush(stdout);

                if (result == -2)
                {
                    // remove client from client list and close socket
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
                    {
                        perror("epoll_ctl");
                    }
                    remove_client(&clients, comm_socket);
                    close(comm_socket);
                }
                else if (result != -1)
                {
                    ssize_t bytes_tx = send(comm_socket, response, strlen(response), 0);
                    if (bytes_tx < 0)
                    {
                        perror("send");
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
                        {
                            perror("epoll_ctl");
                        }
                        close(comm_socket);
                        remove_client(&clients, comm_socket);
                        continue;
                    }

                    fprintf(stdout, "SENT %s:%s | %s %s\n", "{TO_IP}", "{TO_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
                    fflush(stdout);

                    // ERR message was send, now send BYE
                    if (!strncmp(response, "ERR", strlen("ERR")))
                    {
                        if (response != NULL) free(response);
                        response = NULL;
                        content_bye(&response);
                        bytes_tx = send(comm_socket, response, strlen(response), 0);
                        if (bytes_tx < 0)
                        {
                            perror("send");
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
                            {
                                perror("epoll_ctl");
                            }
                            close(comm_socket);
                            remove_client(&clients, comm_socket);
                            continue;
                        }

                        fprintf(stdout, "SENT %s:%s | %s %s\n", "{TO_IP}", "{TO_PORT}", "{MESSAGE_TYPE}", "[MESSAGE_CONTENTS]");
                        fflush(stdout);

                        if (response != NULL) free(response);
                        response = NULL;
                        Client *client = search_client(&clients, comm_socket);
                        content_left_msg(&response, client->data.display_name, client->data.channel);
                        send_msg_to_clients(clients, comm_socket, client->data.channel, response);
                        // remove client from client list and close socket
                        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
                        {
                            perror("epoll_ctl");
                        }
                        remove_client(&clients, comm_socket);
                        close(comm_socket);
                    }
                    else if (!strncmp(response, "REPLY", strlen("REPLY")))
                    {
                        if (response != NULL) free(response);
                        response = NULL;
                        Client *client = search_client(&clients, comm_socket);
                        content_joined_msg(&response, client->data.display_name, client->data.channel);
                        send_msg_to_clients(clients, comm_socket, client->data.channel, response);
                    }
                }
                else
                {
                    Client *client = search_client(&clients, comm_socket);
                    response = strdup(buff);
                    send_msg_to_clients(clients, comm_socket, client->data.channel, response);
                }
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

    char *port = DEFAULT_SERVER_PORT;
    char *ip_addr = NULL;

    while ((opt = getopt(argc, argv, "l:p:d:r:h")) != -1) 
    {
        switch (opt)
        {
            case 'l':
                ip_addr = optarg;
                break;
            case 'p':
                if (atoi(optarg) <= 0 || atoi(optarg) > 65535) 
                {
                    fprintf(stderr, "ERR: Invalid port number! Port must be between 1 and 65535!\n");
                    exit(EXIT_FAILURE);
                }
                port = optarg;
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

    if (ip_addr == NULL)
    {
        fprintf(stderr, "ERR: Use './ipk24chat-server -h' for help!\n");
        exit(EXIT_FAILURE);
    }

    connect_sockets(4567);
    return EXIT_SUCCESS;
}