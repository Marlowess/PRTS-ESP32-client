#ifndef ESP_SOCKET_H
#define ESP_SOCKET_H

#include <string>

void CloseSocket(int sock);
int CreateSocket(const char *dest, int port);
//void SendData(int sock, char *data);
void SendData(int sock, std::string data);
long ReceiveData(int sock);

#endif // ESP_SOCKET_H
