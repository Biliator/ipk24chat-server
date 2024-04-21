#ifndef MESSAGE_TYPE_ENUM_H
#define MESSAGE_TYPE_ENUM_H

enum message_type
{
    AUTH,
    MSG,
    JOIN,
    REPLY,
    ERR,
    BYE,
    CONFIRM,
    UKNOWN,
    INT_ERR
};

static const char * const message_type_enum[] = {
	[AUTH] = "AUTH",
	[MSG] = "MSG",
	[JOIN] = "JOIN",
    [REPLY] = "REPLY",
	[ERR] = "ERR",
    [BYE] = "BYE",
    [CONFIRM] = "CONFIRM"
};

#endif