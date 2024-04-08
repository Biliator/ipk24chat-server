/*
 * IPK.2024
 *
 * Demonstration of trivial UDP server.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <signal.h>

typedef struct
{
    int port_number;
} Config;

typedef enum
{
    ERROR,
    SUCCESS,
} Status;

typedef struct
{
    int capacity;
    int size;
    char *buffer;
} DynamicBuffer;

#define CHECK_ERROR(val)     \
    if (val == ERROR)        \
    {                        \
        return EXIT_FAILURE; \
    }

#define CHECK_PERROR(condition, message) \
    if (condition)                       \
    {                                    \
        perror(message);                 \
        return EXIT_FAILURE;             \
    }

Status GetConfigFromCLArguments(int argc, const char *argv[], Config *config);
Status AddToBuffer(DynamicBuffer *dynBuffer, char *contentToAdd, int length);
void InitBuffer(DynamicBuffer *dynBuffer);
void DestroyBuffer(DynamicBuffer *dynBuffer);

DynamicBuffer dynBuffer;
int comm_socket = -1;

void sigintHandler(int sig_num)
{
    fprintf(stderr, "\nOk, I'm out.\n");
    if (dynBuffer.buffer != NULL)
    {
        DestroyBuffer(&dynBuffer);
    }
    if (comm_socket != -1)
    {
        close(comm_socket);
    }
    exit(EXIT_SUCCESS);
}

// Message size
// MAX_MESSAGE_SIZE > 1472 generally undergoes IP fragmentation (the value depends on MTU of the path)
// MAX_MESSAGE_SIZE > 65,507 is unsendable on IPv4, sets errno to "Message too long" (limit of IPv4 header)
// MAX_MESSAGE_SIZE > 65,535 is unsendable on IPv6, sets errno to "Message too long" (limit of UDP header)
#define MAX_MESSAGE_SIZE 1024

int main(int argc, const char *argv[])
{
    // Ctrl+C handler
    signal(SIGINT, sigintHandler);

    Config config;

    // Check and parse arguments
    CHECK_ERROR(GetConfigFromCLArguments(argc, argv, &config));

    // Create socket
    comm_socket = socket(AF_INET, SOCK_DGRAM, 0); // IPv4, UDP, Protocol 0
    CHECK_PERROR(comm_socket == -1, "Socket");

    // Optionally set socket options
    // SO_REUSEADDR is great for debugging as the same port can be reused in multiple program executions in quick succession
    int optval = 1;
    if (setsockopt(comm_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        perror("Setsockopt");
    }

    // Prepare server address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;                 // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY;         // Listen on all interfaces, i.e. 0.0.0.0
    server_address.sin_port = htons(config.port_number); // Port

    // Assing server address to the socket
    CHECK_PERROR(bind(comm_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1, "Bind");

    printf("INFO: Server listening on port %d\n", config.port_number);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while (true)
    {
        // Prepare buffer for the whole incoming message
        InitBuffer(&dynBuffer);

        // Add header to the message for later transmit back to the client
        char *message_header = "--Your message--\n";
        int message_header_length = strlen(message_header);
        CHECK_ERROR(AddToBuffer(&dynBuffer, message_header, message_header_length))

        // Start receiving message
        char receive_buffer[MAX_MESSAGE_SIZE] = {0};
        bool terminate_char_found = false;

        while (!terminate_char_found)
        {
            // Receive bytes into receive buffer - only works with one single client!
            int received_bytes = recvfrom(comm_socket, receive_buffer, MAX_MESSAGE_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
            if (received_bytes <= 0)
            {
                fprintf(stderr, "Client timeout!\n");
                break;
            }

            // Check if '\0' is present
            for (int i = 0; i < received_bytes; i++)
            {
                if (receive_buffer[i] == '\0')
                {
                    received_bytes = i + 1; // Everything after '\0' can be ignored
                    terminate_char_found = true;
                    break;
                }
            }

            printf("Received %d bytes from %s:%d\n", received_bytes, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            CHECK_ERROR(AddToBuffer(&dynBuffer, receive_buffer, received_bytes)); // Add received chunk to the rest

            if (!terminate_char_found)
            {
                // Set receive timeout to 5 seconds
                struct timeval timeval = {
                    .tv_sec = 5};
                CHECK_PERROR(setsockopt(comm_socket, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval)) < 0, "Setsockopt");
            }
        }

        // Complete message can be printed out (without header)
        printf("--Message is complete--\n%s", dynBuffer.buffer + message_header_length);

        // Unset receive timeout
        struct timeval timeval = {
            .tv_sec = 0};
        CHECK_PERROR(setsockopt(comm_socket, SOL_SOCKET, SO_RCVTIMEO, &timeval, sizeof(timeval)) < 0, "Setsockopt");

        // Send message back
        int total_bytes_sent = 0;
        int total_bytes_to_send = dynBuffer.size;
        while (total_bytes_sent != total_bytes_to_send)
        {
            int bytes_sent = sendto(comm_socket, dynBuffer.buffer + total_bytes_sent, dynBuffer.size - total_bytes_sent, 0, (struct sockaddr *)&client_addr, client_len);
            if (bytes_sent < 0)
            {
                perror("sendto");
                break;
            }
            total_bytes_sent += bytes_sent;
        }

        // Cleanup resources associated with the client
        DestroyBuffer(&dynBuffer); // Free memory
        printf("INFO: Client served\n");
    }

    return EXIT_SUCCESS;
}

// Parse CL Arguments
Status GetConfigFromCLArguments(int argc, const char *argv[], Config *config)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return ERROR;
    }

    // Port
    int port_for_validation = atoi(argv[1]);
    if (port_for_validation < 0 || port_for_validation > USHRT_MAX)
    {
        fprintf(stderr, "Port number out of range\n");
        return ERROR;
    }
    config->port_number = port_for_validation;
}

void InitBuffer(DynamicBuffer *dynBuffer)
{
    dynBuffer->buffer = NULL;
    dynBuffer->capacity = 0;
    dynBuffer->size = 0;
}

void DestroyBuffer(DynamicBuffer *dynBuffer)
{
    free(dynBuffer->buffer);
    dynBuffer->buffer = NULL;
    dynBuffer->capacity = 0;
    dynBuffer->size = 0;
}

Status AddToBuffer(DynamicBuffer *dynBuffer, char *contentToAdd, int length)
{
    if ((dynBuffer->size + length) > dynBuffer->capacity)
    {
        dynBuffer->buffer = realloc(dynBuffer->buffer, dynBuffer->capacity + (length * 3));
        if (dynBuffer->buffer == NULL)
        {
            fprintf(stderr, "Realloc failed!\n");
            return ERROR;
        }
        dynBuffer->capacity = dynBuffer->capacity + (length * 3);
    }

    memcpy(dynBuffer->buffer + dynBuffer->size, contentToAdd, length);
    dynBuffer->size = dynBuffer->size + length;

    return SUCCESS;
}