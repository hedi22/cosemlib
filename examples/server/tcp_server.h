#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stdlib.h>
#include "transports.h"

int tcp_server_init(data_handler data_func, conn_handler conn_func, char *buffer, int buf_size, int tcp_port);

#endif // TCP_SERVER_H
