else if (events[i].data.fd == server_socket_udp)
            {
                char buff[BUFFER_SIZE];
                ssize_t bytes_rx = recvfrom(server_socket_udp, buff, BUFFER_SIZE, 0, address, &comm_addr_size);
                if (bytes_rx < 0)
                {
                    perror("recvfrom");
                    continue;
                }

                printf("-%.*s-\n", (int)bytes_rx, buff);

                // If the received message is AUTH, reply with CONFIRM
                if (strncmp(buff, "AUTH", 4) == 0)
                {
                    ssize_t bytes_tx = sendto(server_socket_udp, "CONFIRM", 7, 0, (struct sockaddr *)&comm_addr, comm_addr_size);
                    if (bytes_tx < 0)
                    {
                        perror("sendto");
                        continue;
                    }

                    // Allocate a new socket for further communication
                    int new_socket = socket(AF_INET, SOCK_DGRAM, 0);
                    if (new_socket <= 0)
                    {
                        perror("socket");
                        continue;
                    }

                    // Bind the new socket
                    if (bind(new_socket, (struct sockaddr *)&server_addr, address_size) < 0)
                    {
                        perror("bind");
                        close(new_socket);
                        continue;
                    }

                    printf("New socket allocated for further communication.\n");

                    // Update event for new socket
                    struct epoll_event event_new;
                    event_new.events = EPOLLIN;
                    event_new.data.fd = new_socket;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event_new) == -1)
                    {
                        perror("epoll_ctl");
                        close(new_socket);
                        continue;
                    }
                }
            }