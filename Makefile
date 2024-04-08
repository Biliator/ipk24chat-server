CFLAGS=-std=gnu17 -Wall -D_GNU_SOURCE -Wextra -Werror
CFLAGS_EZ=-std=gnu17 -Werror -D_GNU_SOURCE
FILES=ipk24chat-server.c
NAME=ipk24chat-server

compile:
	gcc $(CFLAGS_EZ) $(FILES) -o $(NAME)