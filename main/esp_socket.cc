#include <string.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netinet/in.h> //"in" for "sockaddr_in"
// #include <netdb.h> //"netdb" for "gethostbyname"
#include <errno.h>

#include "esp_log.h"
#include "../include/esp_socket.h"

static const char *TAG = "simple wifi";

void CloseSocket(int sock) {
  close(sock);
  return;
}

int CreateSocket(const char *dest, int port) {
  ESP_LOGI(TAG, "Entering CreateSocket() function");
  int sock, err;
  struct sockaddr_in temp;

  //sock = socket(AF_INET, SOCK_STREAM, 0);
  sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if ( sock == -1 ) {
    ESP_LOGI(TAG, "Error: sock() failed");
    return -1;
  }

  temp.sin_family = AF_INET;
  temp.sin_port = htons(port);
  //inet_pton(AF_INET, dest, &temp.sin_addr);
  err = inet_aton(dest, &temp.sin_addr);
  if ( err == 0 ) {
    ESP_LOGI(TAG, "Error: inet_aton() failed");
    return -1;
  }
  ESP_LOGI(TAG, "Try Connecting to '%s'...", inet_ntoa(temp.sin_addr));

  err = connect(sock, (struct sockaddr*) &temp, sizeof(temp));
  if ( err == -1 ) {
    ESP_LOGI(TAG, "Error: connect() failed");
    ESP_LOGI(TAG, "Error: show meaning: %s\n", strerror(errno));
    return -1;
  }

  ESP_LOGI(TAG, "Connected to %s", inet_ntoa(temp.sin_addr));
  return sock;
}

void SendData(int sock, std::string data) {
  int nsend = -1;
  //strncpy(data, "WIFI_MODE_STA_NOW\0", 18);
  //printf("STAMPA: %s\n", data);
  nsend = send(sock, data.c_str(), data.length(), 0);
  //return nsend > 0;
}
