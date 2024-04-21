CFLAGS=-std=gnu17 -Wall -D_GNU_SOURCE -Wextra -Werror
CFLAGS_EZ=-std=gnu17 -Werror -D_GNU_SOURCE
FILES=ipk24chat-server.c client.c tcp.c udp.c 
NAME=ipk24chat-server

compile:
	gcc $(CFLAGS) $(FILES) -o $(NAME)