#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <limits.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

void tcp(char *host, int port) 
{
    int client_socket;
    const char *server_hostname = host;
    struct hostent *server;
    struct sockaddr_in server_address;

    if ((server = gethostbyname(server_hostname)) == NULL) 
    {
        fprintf(stderr, "ERROR: No such host as %s!\n", server_hostname);
        exit(1);
    }

    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *) &server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(port);

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    {
        fprintf(stderr, "ERROR: Socket creation!");
        exit(1);
    }
        
    if (connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0)
    {
        fprintf(stderr, "ERROR: Socket connection!");
        exit(1);        
    }

    int state = 0;
    char response[1460];

    char *buff = "AUTH xx AS";

    if (send(client_socket, buff, strlen(buff), 0) < 0) 
    { 
        fprintf(stderr, "ERROR: Can't send message!\n");
        exit(1);
    }        

usleep(100000); 

    char *buff2 = " zz USING ";

    if (send(client_socket, buff2, strlen(buff2), 0) < 0) 
    { 
        fprintf(stderr, "ERROR: Can't send message!\n");
        exit(1);
    }        
    usleep(100000); 
    
    char *buff3 = "yy\r\n";

    if (send(client_socket, buff3, strlen(buff3), 0) < 0) 
    { 
        fprintf(stderr, "ERROR: Can't send message!\n");
        exit(1);
    } 

    if (recv(client_socket, response, 1460, 0) < 0) 
    {
        fprintf(stderr, "ERROR: Can't recive message!\n");
        exit(1);
    }   
    printf("%s\n", response);


        usleep(100000); 
    
    char *buff4 = "MSG FROM ";

    if (send(client_socket, buff4, strlen(buff4), 0) < 0) 
    { 
        fprintf(stderr, "ERROR: Can't send message!\n");
        exit(1);
    } 
            usleep(100000); 
    
    char *buff5 = "xx IS ahoj\r";

    if (send(client_socket, buff5, strlen(buff5), 0) < 0) 
    { 
        fprintf(stderr, "ERROR: Can't send message!\n");
        exit(1);
    } 

        char *buff6 = "\n";

    if (send(client_socket, buff6, strlen(buff6), 0) < 0) 
    { 
        fprintf(stderr, "ERROR: Can't send message!\n");
        exit(1);
    } 
    while(1){}
}

int main(int argc, char *argv[])
{
    tcp("localhost", 4567);
    return 0;
}