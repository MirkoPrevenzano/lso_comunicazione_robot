#ifndef HEADER_H
#define HEADER_H

#include <cjson/cJSON.h>
#include <stdbool.h>

void handle_prestito(cJSON *body);
void handle_restituzione(cJSON *body);
bool handle_login(cJSON *body);
cJSON *handle_get_film(cJSON *body);
void handle_send_msg(cJSON *body);
bool handle_check_prestito(cJSON *body);
cJSON *handle_get_msg(cJSON *body);



#endif