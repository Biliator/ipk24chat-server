#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum message_type
{
    AUTH,
    MSG,
    JOIN,
    REPLY,
    ERR,
    BYE,
    UKNOWN
};

static const char * const message_type_enum[] = {
	[AUTH] = "AUTH",
	[MSG] = "MSG",
	[JOIN] = "JOIN",
    [REPLY] = "REPLY",
	[ERR] = "ERR",
    [BYE] = "BYE"
};

int find_crlf(const char *buff, ssize_t length);
void free_params(char **buff_copy, char **param1, char **param2, char **param3);
int tcp_check_auth(char *buff, char **username, char **display_name, char **secret);
int tcp_check_join(char *buff, char **channel_id, char **display_name);
int tcp_check_msg(char *buff, char **display_name, char **message);
int tcp_check_err(char *reply, char **display_name, char **message);
int tcp_check_bye(char *reply);
enum message_type check_message_type(char *buff, char **param1, char **param2, char **param3);
int content_response(char **buff, char *reply_status, char *message);
int content_err(char **buff, char *display_name, char *message);
int content_joined_msg(char **buff, char *display_name, char *message);
int content_left_msg(char **buff, char *display_name, char *channel);
int content_bye(char **buff);