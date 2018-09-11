/*
 * controller.c
 *
 *  Created on: 03 ago 2018
 *      Author: stefano
 */
#include "../include/controller.h"
#include <exception>
#include <iostream>
#include <list>
#include "../include/wifi_packet.h"
#include "../include/esp_socket.h"

using namespace std;
#define ebST_BIT_TASK_PRIORITY	(tskIDLE_PRIORITY)
#define ebSN_BIT_TASK_PRIORITY	(tskIDLE_PRIORITY + 1)

TaskHandle_t xHandleSniffing;
TaskHandle_t xHandleStoring;

/* This list contains the wifi packets */
list<Wifi_packet> *myList;


uint8_t channel = 1;
int i = 13;
char const *address = "192.168.1.101";
int port = 9876;
int s = -1;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/**
 * This function is used to set the handler to save packets
 *  **/
void sniffingFunc(void *pvParameters){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		wifi_sniffer_init();
		changeChannel();
		esp_wifi_set_promiscuous(false);
		ESP_ERROR_CHECK(esp_wifi_stop());
		ESP_ERROR_CHECK(esp_wifi_deinit());
		xTaskNotifyGive(xHandleStoring);
	}
}

/** This function is used to talk with the server and to send it the data about wifi packets
 *  captured in the promiscuous mode before
 *  **/
void storingFunc(void *pvParameters){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		wifi_init_sta();
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

		/*
		 * Inserire qui la funzione per comunicare i dati letti al server
		 */

		s = CreateSocket(address, 9876);
		if(s != -1)
			for (auto it = myList->begin(); it != myList->end(); ++it){
				//it->printData();
				string data = it->retrieveData();
				cout << data << '\n';
				SendData(s, data);
			}

		//printf("Catured %d packets in the last sniffing\n", myList->size());
		//myList->clear();
		/*for (auto it = myList->begin(); it != myList->end(); ++it)
		    it->printData();*/
		CloseSocket(s);
		myList->clear();

		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		esp_wifi_disconnect();
		xTaskNotifyGive(xHandleSniffing);
	}
}

/**
 * It creates two tasks: one to perform the sniffing operations, and another one
 * to send data to the PC
 * **/
void tasksCreation(){
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_event_group = xEventGroupCreate();
	myList = new list<Wifi_packet>();

	if(xTaskCreate(sniffingFunc, "Sniffing task", 2048, NULL, ebSN_BIT_TASK_PRIORITY, &xHandleSniffing) == pdPASS)
		printf("Sniffing task created!\n");
	else
		printf("Error on creating the sniffing task!\n");

	if(xTaskCreate(storingFunc, "Storing task", 4096, NULL, ebST_BIT_TASK_PRIORITY, &xHandleStoring) == pdPASS)
		printf("Storing task created!\n");
	else
		printf("Error on creating the storing task!\n");

	/* The first task to launch is the sniffing task */
	xTaskNotifyGive(xHandleSniffing);
}

/**
 * Any time a wifi event occures, this handler performs such operations
 *  **/
static esp_err_t event_handler(void *ctx, system_event_t *event){
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		ESP_LOGI(TAG, "STA START!\n");
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "got ip:%s",
				ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		ESP_LOGI(TAG, "Disconnected from sta_mode\n");
		break;
	default:
		break;
	}
	return ESP_OK;
}

/**
 * It sets up the station mode, by connecting to server (that is in hotspot mode)
 * and by obtaining an IP address to communicate with it
 * **/
void wifi_init_sta(){
	wifi_config_t wifi_config = {};
	strcpy((char*)wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID);
	strcpy((char*)wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
			EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

/**
 * It sets up the sniffer mode, by going in promiscous mode and setting
 * the handler to threat the packets capture
 * **/
void wifi_sniffer_init(){
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg));
	ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );

	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));

	/* Filtro per catturare solo pacchetti di management */
	wifi_promiscuous_filter_t filter;
	filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));

}

/**
 * Handler to handle the packets captured by the esp32
 *  **/
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type){

	if (type != WIFI_PKT_MGMT)
		return;

	const wifi_promiscuous_pkt_t *pkkt = (wifi_promiscuous_pkt_t *)buff;
	const uint8_t *var3 = (const uint8_t*)pkkt->payload;

	/* Non consideriamo pacchetti che non siano probe request */
	if((*var3) != 0x40) return;

	//const uint8_t *mgmt_type = (const uint8_t*)pkkt->payload+24;
	const uint8_t *ssid_len = (const uint8_t*)pkkt->payload+25;

	char* ssid_ = new char[*ssid_len + 1];
	int i = 0;
	for(i = 0; i < *ssid_len; i++){
		ssid_[i] = *(char*)(pkkt->payload + 26 + i);
	}
	ssid_[i] = '\0';


	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
	const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

	printf("--- Packet captured\n");

	long systemTime = getTime();

	//printf("System: %ld --- packet: %d --- total: %ld\n\n", systemTime, (int)(ppkt->rx_ctrl.timestamp/1000000), (long)(ppkt->rx_ctrl.timestamp/1000000+systemTime));

	const Wifi_packet packet(hdr->addr3, hdr->addr2, ppkt->rx_ctrl.rssi, ppkt->rx_ctrl.timestamp + systemTime, ssid_, *ssid_len + 1);
	myList->push_back(packet);
}

/**
 * When we're in promiscuous mode, this function changes the channel, to scan any wifi "space"
 *  **/
void changeChannel(){
	i = 13;
	channel = 1;
	while(i-- > 0){
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
		printf("--- Canale di cattura: %d\n", channel);
		channel = (channel % WIFI_CHANNEL_MAX) + 1;
	}
}

void printTime(){
	struct timeval time;
	int res = gettimeofday(&time, NULL);
	if(res == -1)
		printf("Error on retrieving time\n");
	else{
		printf("Time OK\n");
		printf("Seconds: %lld\n", (long long)time.tv_sec);
	}
}

//1533370708

void setTime(long seconds){
	struct timeval time;
	time.tv_sec = (time_t)seconds;
	time.tv_usec = (suseconds_t)0;

	int res = settimeofday(&time, NULL);
	if(res == -1)
		printf("Error on setting time\n");
	else
		printf("Time setting OK\n");

}

long getTime(){
	struct timeval time;
	int res = gettimeofday(&time, NULL);
	if(res == -1)
		return (long)-1;
	else
		return (long long)time.tv_sec;
}





