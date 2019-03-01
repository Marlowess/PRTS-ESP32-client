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
	if(sock == -1) return;
	int nsend = -1;
	//uint32_t dim = htonl(data.length());
	//nsend = send(sock, &dim, sizeof(uint32_t), 0);
	int dim = htonl(data.length());
	//printf("--- Dim: %d\n", dim);
	nsend = send(sock, &dim, sizeof(int), 0);
	if(nsend < sizeof(int))
		printf("--- Error on sending size\n");
	else
		printf("--- Socket: size sent\n");
	nsend = send(sock, data.c_str(), data.length(), 0);
	if(nsend < data.length())
		printf("--- Error on sending data\n");
	else
		printf("--- Socket: data sent\n");
}

unsigned long ReceiveData(int sock){
	char strtime[11];
	unsigned long time = 0;
	int result = recv(sock, (void *)strtime, 10, 0);
	printf("--- Bytes received: %d\n", result);
	strtime[10] = '\0';
	time = strtoul (strtime, NULL, 0);
	time = ntohl(time);
	printf("--- Prova: %lu\n", time);
	return time;
}

void waitResponse(int sock){
	// The board will be locked until it not receives the signal to start capturing
	char start[3];
	int result = recv(sock, (void *)start, 2, 0);
	if(result < 2)
		printf("--- Error on receiving signal to start\n");
	if(strcmp(start, "GO") == 0)
		printf("--- Starting signal received\n");
	else
		printf("--- Wrong starting signal received\n");
}
