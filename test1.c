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
        fprintf(stderr, "ERR: setsocket!\n");
        exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) != 1)
    {
        perror("ERR: inet_pton");
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
    Client_udp *clients_udp = NULL;

    while (1)
    {
        struct epoll_event events[MAX_EVENTS];
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {   
            end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);

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
                
                add_client(&clients, comm_socket);
            } 
            // udp connection proccess
            else if (events[i].data.fd == server_socket_udp)
            {
                char buff[BUFFER_SIZE] = {0};

                int bytes_rx = recvfrom(server_socket_udp, buff, BUFFER_SIZE, 0, address, &address_size);
                if (bytes_rx < 0)
                {
                    perror("recvfrom");
                    continue;
                }

                char *response = NULL;
                size_t response_length = 0;
                
                if (buff[0] != 0x00)
                {
                    Client_udp *client = NULL;
                    if (add_client_udp(&clients_udp, &client, server_socket_udp))
                    {
                        end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
                        exit(EXIT_FAILURE);
                    }
                    
                    if (confirm(&response, &response_length, (uint8_t *) &buff[1], (uint8_t *) &buff[2]))
                    {
                        end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
                        exit(EXIT_FAILURE);
                    }

                    int bytes_tx = sendto(server_socket_udp, response, response_length, 0, address, address_size);
                    if (bytes_tx < 0)
                    {
                        perror("sendto");
                        remove_client_udp(&clients_udp, server_socket_udp);
                        continue;
                    }

                    int new_socket = socket(AF_INET, SOCK_DGRAM, 0);
                    if (new_socket <= 0)
                    {
                        perror("socket");
                        remove_client_udp(&clients_udp, server_socket_udp);
                        continue;
                    }

                    // Update event for new socket
                    struct epoll_event event_new;
                    memset(&event_new, 0, sizeof(struct epoll_event));
                    event_new.events = EPOLLIN;
                    event_new.data.fd = new_socket;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event_new) == -1)
                    {
                        perror("epoll_ctl");
                        remove_client_udp(&clients_udp, server_socket_udp);
                        close(new_socket);
                        continue;
                    }
                    
                    client->data.socket = new_socket;
                    free(response);
                    enum message_type msg_type = UKNOWN;
                    int result = next_state_udp(clients_udp, client, buff, &response, &response_length, &msg_type);
                    if (result)
                    {
                        end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
                        exit(EXIT_FAILURE);
                    }
                    (void) msg_type;

                    bytes_tx = sendto(new_socket, response, response_length, 0, address, address_size);
                    if (bytes_tx < 0)
                    {
                        perror("sendto");
                        remove_client_udp(&clients_udp, new_socket);
                        close(new_socket);
                        continue;
                    }
                    
                    free(response);
                    memset(&buff, 0, BUFFER_SIZE);
                    int bytes_rx = recvfrom(new_socket, buff, BUFFER_SIZE, 0, address, &address_size);
                    if (bytes_rx < 0)
                    {
                        perror("recvfrom");
                        remove_client_udp(&clients_udp, new_socket);
                        close(new_socket);
                        continue;
                    }
                }
            }
            // tcp connection proccess
            else
            {
                int comm_socket = events[i].data.fd;
                char buff[BUFFER_SIZE] = {0};

                Client *client = search_client(&clients, comm_socket);
                Client_udp *client_udp = search_client_udp(&clients_udp, comm_socket);

                if (client != NULL)
                {
                    printf("TCP\n");
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
                        end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
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
                                end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
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
                                    end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
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
                                end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
                                exit(EXIT_FAILURE);
                            }
                            send_msg_to_clients(clients, comm_socket, client->data.channel, response, MSG);

                            if (strcmp(channel, client->data.channel))
                            {
                                if (response != NULL) free(response);
                                response = NULL;
                                if (content_left_msg(&response, client->data.display_name, channel))
                                {
                                    end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
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
                else if (client_udp != NULL)
                {
                    int bytes_rx = recvfrom(comm_socket, buff, BUFFER_SIZE, 0, address, &address_size);
                    if (bytes_rx < 0)
                    {
                        perror("recvfrom");
                        continue;
                    }

                    if (buff[0] == 0x00) continue;

                    char *response = NULL;
                    size_t response_length = 0;

                    Client_udp *client = search_client_udp(&clients_udp, comm_socket);
                    if (client == NULL)
                    {

                    }

                    if (confirm(&response, &response_length, (uint8_t *) &buff[2], (uint8_t *) &buff[1]))
                    {
                        end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
                        exit(EXIT_FAILURE);
                    }
                        
                    int bytes_tx = sendto(comm_socket, response, response_length, 0, address, address_size);
                    if (bytes_tx < 0)
                    {
                        perror("sendto");
                        continue;
                    }

                    free(response);
                    enum message_type msg_type = UKNOWN;
                    int result = next_state_udp(clients_udp, client, buff, &response, &response_length, &msg_type);
                    if (result == 1)
                    {
                        end_server(&clients, &clients_udp, server_socket_tcp, server_socket_udp, epoll_fd);
                        exit(EXIT_FAILURE);
                    }
                    else if (result == -1)
                    {
                        send_msg_to_clients_udp(clients_udp, client->data.socket, client->data.channel, response, msg_type);
                        /*
                        int bytes_tx = sendto(comm_socket, response, response_length, 0, address, address_size);
                        if (bytes_tx < 0)
                        {
                            perror("sendto");
                            continue;
                        }
                        free(response);
                        */
                    }
                }
            }
        }
    }
}