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

void handle_udp(int port)
{
    struct sigaction sa;
    sa.sa_handler = handle_interrupt;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGQUIT, &sa, NULL) == -1)
    {
        fprintf(stderr, "ERR: Setting up signal handler!\n");
        exit(EXIT_FAILURE);
    }

    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket <= 0)
    {
        fprintf(stderr, "ERR: socket!\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    int address_size = sizeof(server_addr);
    if (bind(server_socket, (struct sockaddr *) &server_addr, address_size) < 0)
    {
        fprintf(stderr, "ERR: bind!\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_address;
    struct sockaddr *address = (struct sockaddr *) &client_address;

    while(!received_signal)
    {
        char *buff;

        int bytes_rx = recvfrom(server_socket, buff, BUFFER_SIZE, 0, address, &address_size);
        if (bytes_rx < 0)
        {
            fprintf(stderr, "ERR: recvfrom!\n");
            exit(EXIT_FAILURE);
        }
        
        printf("-%s-\n", buff);
        int bytes_tx = sendto(server_socket, buff, strlen(buff), 0, address, address_size);
        if (bytes_tx < 0)
        {
            fprintf(stderr, "ERR: sendto!\n");
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
}

void handle_tcp(int port)
{
    struct sigaction sa;
    sa.sa_handler = handle_interrupt;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGQUIT, &sa, NULL) == -1)
    {
        fprintf(stderr, "ERR: Setting up signal handler!\n");
        exit(EXIT_FAILURE);
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket <= 0)
    {
        fprintf(stderr, "ERR: socket!\n");
        exit(EXIT_FAILURE);
    }

	int optval = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
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
    if (bind(server_socket, (struct sockaddr *) &server_addr, address_size) < 0)
    {
        fprintf(stderr, "ERR: bind!\n");
        exit(EXIT_FAILURE);
    }

    int max_waiting_connections = 10;
    if (listen(server_socket, max_waiting_connections) < 0)
    {
        fprintf(stderr, "ERR: listen!\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in comm_addr;
    socklen_t comm_addr_size = sizeof(comm_addr);

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        struct epoll_event events[MAX_EVENTS];
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            if (errno == EINTR && received_signal)
            {
                fprintf(stdout, "Received signal, shutting down.\n");
                close(server_socket);
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
            if (events[i].data.fd == server_socket)
            {
                int comm_socket = accept(server_socket, (struct sockaddr *) &comm_addr, &comm_addr_size);
                if (comm_socket < 0)
                {
                    perror("accept");
                    continue;
                }
                
                event.events = EPOLLIN;
                event.data.fd = comm_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, comm_socket, &event) == -1)
                {
                    perror("epoll_ctl");
                    close(comm_socket);
                    continue;
                }
                printf("New connection established.\n");
            } 
            else
            {
                int comm_socket = events[i].data.fd;
                char buff[BUFFER_SIZE];
                ssize_t bytes_rx = recv(comm_socket, buff, BUFFER_SIZE, 0);
                if (bytes_rx < 0)
                {
                    perror("recv");
                    close(comm_socket);
                    continue;
                }
                else if (bytes_rx == 0)
                {
                    printf("Connection closed by client.\n");
                    close(comm_socket);
                    // Remove comm_socket from epoll instance
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, comm_socket, NULL) == -1)
                    {
                        perror("epoll_ctl");
                    }
                    continue;
                }
                printf("Received message: %.*s\n", (int)bytes_rx, buff);
                ssize_t bytes_tx = send(comm_socket, buff, bytes_rx, 0);
                if (bytes_tx < 0) 
                {
                    perror("send");
                    close(comm_socket);
                    continue;
                }
            }
        }
    }
}

void connect_socket(char *host, char *port)
{

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

    handle_tcp(4567);
    return EXIT_SUCCESS;
}