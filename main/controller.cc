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

uint8_t channel = 1;
int i = 13;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

void sniffingFunc(void *pvParameters){
	while(true){
		//int i;
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//for(i = 0; i < 15; i++)
		//	printf("Packet %d captured!\n", i);
		wifi_sniffer_init();
		i = 13;
		channel = 1;
		changeChannel();
		esp_wifi_set_promiscuous(false);
		ESP_ERROR_CHECK(esp_wifi_stop());
		ESP_ERROR_CHECK(esp_wifi_deinit());
		//sleep(3);
		xTaskNotifyGive(xHandleStoring);
	}
}

void storingFunc(void *pvParameters){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		wifi_init_sta();
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
		printf("Sniffing done, I'm talking with server...\n");
		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		esp_wifi_disconnect();
		xTaskNotifyGive(xHandleSniffing);
	}
}

void tasksCreation(){
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	wifi_event_group = xEventGroupCreate();

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

void wifi_init_sta(){
	/*tcpip_adapter_init();
	if(firstFlag){
		ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	}*/

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


void wifi_sniffer_init(){
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg));
	ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );

    //ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));

	/* Filtro per catturare solo pacchetti di management */
	wifi_promiscuous_filter_t filter;
	filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filter));

}

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

	printf("--- Pacchetto catturato\n");

	printf("CHAN=%02d, RSSI=%02d,"
			" ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
			" ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
			" ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
			ppkt->rx_ctrl.channel,
			ppkt->rx_ctrl.rssi,
			/* ADDR1 */
			hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
			hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],
			/* ADDR2 */
			hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
			hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
			/* ADDR3 */
			hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
			hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
		);

	//const Wifi_packet packet(hdr->addr3, hdr->addr2, ppkt->rx_ctrl.rssi, ppkt->rx_ctrl.timestamp, ssid_, *ssid_len + 1);
	//contr->addPacket(packet);
}

void changeChannel(){
	while(i-- > 0){
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
		printf("--- Canale di cattura: %d\n", channel);
		channel = (channel % WIFI_CHANNEL_MAX) + 1;
	}
}





