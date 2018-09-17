#include "../include/esp_socket.h"

static const char *TAG = "simple wifi";

void CloseSocket(int sock) {
  close(sock);
  return;
}

int CreateSocket(char *dest, int port) {
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
  uint32_t dim = htonl(data.length());
  nsend = send(sock, &dim, sizeof(uint32_t), 0);
  nsend = send(sock, data.c_str(), data.length(), 0);
}

long ReceiveData(int sock){
	long time;
	int result = recv(sock, (void *)&time, sizeof(long), 0);
	time = ntohl(time);
	return time;
}
