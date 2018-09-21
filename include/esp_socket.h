#ifndef ESP_SOCKET_H
#define ESP_SOCKET_H

#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h> //"in" for "sockaddr_in"
#include <errno.h>
#include <stdio.h>
#include "esp_log.h"

void CloseSocket(int sock);
int CreateSocket(char *dest, int port);
void SendData(int sock, std::string data);
unsigned long ReceiveData(int sock);
void waitResponse(int sock);

#endif // ESP_SOCKET_H
