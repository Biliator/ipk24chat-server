#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "message_type_enum.h"

void message_id_increase(uint8_t *lsb, uint8_t *msb);
int udp_message_next(char input[], char **output, int start, size_t input_size);
enum message_type check_message_type_udp(char *buff, char **param1, char **param2, char **param3);
int confirm(char **content, size_t *length, uint8_t lsb, uint8_t msb);
int reply(char **content, size_t *length, uint8_t lsb, uint8_t msb, uint8_t result, uint8_t ref_lsb, uint8_t ref_msb, char *message_contents);
int err(char **content, size_t *length, uint8_t lsb, uint8_t msb, char *display_name, char *message_contents);
int msg(char **content, size_t *length, uint8_t lsb, uint8_t msb, char *display_name, char *message_contents);
int bye(char **content, size_t *length, uint8_t lsb, uint8_t msb);