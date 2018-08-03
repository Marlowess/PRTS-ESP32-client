/*
 * controller.c
 *
 *  Created on: 03 ago 2018
 *      Author: stefano
 */

#include "controller.h"
#include <exception>
#include <iostream>

using namespace std;

TaskHandle_t xHandleSniffing;
TaskHandle_t xHandleStoring;
#define ebST_BIT_TASK_PRIORITY	(tskIDLE_PRIORITY)
#define ebSN_BIT_TASK_PRIORITY	(tskIDLE_PRIORITY + 1)
static int firstFlag = true;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

void sniffingFunc(void *pvParameters){
	while(true){
		int i;
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		for(i = 0; i < 15; i++)
			printf("Packet %d captured!\n", i);
		sleep(3);
		xTaskNotifyGive(xHandleStoring);
	}
}

void storingFunc(void *pvParameters){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		wifi_event_group = xEventGroupCreate();
		wifi_init_sta();
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
		printf("Sniffing done, I'm talking with server...\n");
		sleep(3);
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		vEventGroupDelete(wifi_event_group);
		esp_wifi_disconnect();
		//esp_wifi_deinit();
		xTaskNotifyGive(xHandleSniffing);
	}
}

void tasksCreation(){
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

static esp_err_t event_handler(void *ctx, system_event_t *event){
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "got ip:%s",
				ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",
				MAC2STR(event->event_info.sta_connected.mac),
				event->event_info.sta_connected.aid);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
				MAC2STR(event->event_info.sta_disconnected.mac),
				event->event_info.sta_disconnected.aid);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		//esp_wifi_connect();
		//xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		ESP_LOGI(TAG, "Disconnected from sta_mode\n");
		break;
	default:
		break;
	}
	return ESP_OK;
}

void wifi_init_sta(){

	tcpip_adapter_init();
	if(firstFlag){
		ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	}

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));



	wifi_config_t wifi_config = {};
	strcpy((char*)wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID);
	strcpy((char*)wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS);

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

	if(firstFlag){
		ESP_ERROR_CHECK(esp_wifi_start());
		firstFlag = false;
	}
	else
		ESP_ERROR_CHECK(esp_wifi_connect());


	ESP_LOGI(TAG, "wifi_init_sta finished.");
	ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
			EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}



