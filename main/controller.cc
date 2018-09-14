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
#include "../include/my_mdns.h"
#include <unistd.h>
#include <functional> //for std::hash
#include <string>

using namespace std;
#define ebST_BIT_TASK_PRIORITY	(tskIDLE_PRIORITY)
#define ebSN_BIT_TASK_PRIORITY	(tskIDLE_PRIORITY + 1)

#define EXAMPLE_MDNS_HOSTNAME CONFIG_MDNS_HOSTNAME
#define EXAMPLE_MDNS_INSTANCE CONFIG_MDNS_INSTANCE
#define SERVER_HOSTNAME CONFIG_SERVER_HOSTNAME

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int IP4_CONNECTED_BIT = BIT0;
const int IP6_CONNECTED_BIT = BIT1;

TaskHandle_t xHandleSniffing;
TaskHandle_t xHandleStoring;
TaskHandle_t xHandleTimes; // task to exchange time informations

/* This list contains the wifi packets */
list<Wifi_packet> *myList;


uint8_t channel = 1;
int i = 13;
char *address = "10.42.0.1";
int port = 1026;
int s = -1;
static bool auto_reconnect = true;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

static string hashFunction(string str){
	hash<std::string> hasher;
	auto hashed = hasher(str); //returns std::size_t
	//cout << hashed << '\n'; //outputs 2146989006636459346 on my machine
	stringstream ss;
	ss << hashed << "\r\n";
	string st = ss.str();
	return st;
}

/**
 * It initialises the mdns protocol service
 */
static void initialise_mdns(void){
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(EXAMPLE_MDNS_HOSTNAME) );
    //set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set(EXAMPLE_MDNS_INSTANCE) );

    //structure with TXT records
    mdns_txt_item_t serviceTxtData[3] = {
        {"board","esp32"},
        {"u","user"},
        {"p","password"}
    };

    //initialize service
    ESP_ERROR_CHECK( mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData, 3) );
    //add another TXT item
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "path", "/foobar") );
    //change TXT item value
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "u", "admin") );
}

/**
 * This function allows to obtain the IP of a host by knowing its hostname.
 */
static void query_mdns_host(const char * host_name){
    ESP_LOGI(TAG, "Query A: %s.local", host_name);

    struct ip4_addr addr;
    addr.addr = 0;

    esp_err_t err = mdns_query_a(host_name, 2000,  &addr);
    if(err){
        if(err == ESP_ERR_NOT_FOUND){
            ESP_LOGW(TAG, "%s: Host was not found!", esp_err_to_name(err));
            return;
        }
        ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
        return;
    }

    address = inet_ntoa(addr.addr);
    ESP_LOGI(TAG, IPSTR, IP2STR(&addr));
}

static void do_mdsnQuery(const char *host){
	cout << "--- Trying to find sb's IP address\n";
	/* Wait for the callback to set the CONNECTED_BIT in the event group. */
	xEventGroupWaitBits(wifi_event_group, IP4_CONNECTED_BIT | IP6_CONNECTED_BIT,
			false, true, portMAX_DELAY);
	sleep(3);
	query_mdns_host(SERVER_HOSTNAME);
}

/**
 * This function is able to exchange time informations with server.
 * It will be used everytime to syncronize the internal clock of board,
 * in order to perform a more accurate analysis.
 */
void timestampExchFunc(void *pvParameters){
	while(true){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		wifi_init_sta();
		xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

		do_mdsnQuery("sb");

		cout << "--- Trying to connect" << '\n';
		s = CreateSocket(address, 1026);
		if(s != -1){
			SendData(s, std::move("|"));
			cout << "--- Timestamp request sent" << '\n';
			long time = ReceiveData(s);
			setTime(time);
			printf("--- Current timestamp: %ld\n", getTime());
		}

		printf("--- Timestamp task ended, control given to sniffing task\n");

		xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		esp_wifi_disconnect();
		xTaskNotifyGive(xHandleSniffing);
	}
}

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

		s = CreateSocket(address, 1026);
		if(s != -1){
			for (auto it = myList->begin(); it != myList->end(); ++it){
				string data = it->retrieveData();
				string hashCode = hashFunction(data);

				stringstream ss;
				ss << data << "," << hashCode;
				string st = ss.str();

				cout << st << endl;
				SendData(s, st);
			}
			SendData(s, std::move("|"));
			cout << "End of packets transmission. Waiting for timestamp..." << '\n';
			long time = ReceiveData(s);
			setTime(time);
			printf("--- Now: %ld\n", getTime());
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

	initialise_mdns();

	myList = new list<Wifi_packet>();

	if(xTaskCreate(sniffingFunc, "Sniffing task", 2048, NULL, ebSN_BIT_TASK_PRIORITY, &xHandleSniffing) == pdPASS)
		printf("Sniffing task created!\n");
	else
		printf("Error on creating the sniffing task!\n");

	if(xTaskCreate(timestampExchFunc, "Timestamp task", 2048, NULL, ebSN_BIT_TASK_PRIORITY, &xHandleTimes) == pdPASS)
		printf("Timestamp task created!\n");

	if(xTaskCreate(storingFunc, "Storing task", 4096, NULL, ebST_BIT_TASK_PRIORITY, &xHandleStoring) == pdPASS)
		printf("Storing task created!\n");
	else
		printf("Error on creating the storing task!\n");

	/* The first task to launch is the sniffing task */
	//xTaskNotifyGive(xHandleSniffing);

	xTaskNotifyGive(xHandleTimes);
	//hashFunction(string("Stringa di prova!"));
}

/**
 * Any time a wifi event occures, this handler performs such operations
 *  **/
//static esp_err_t event_handler(void *ctx, system_event_t *event){
//	switch(event->event_id) {
//	case SYSTEM_EVENT_STA_START:
//		esp_wifi_connect();
//		ESP_LOGI(TAG, "STA START!\n");
//		break;
//	case SYSTEM_EVENT_STA_GOT_IP:
//		ESP_LOGI(TAG, "got ip:%s",
//				ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
//		xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
//		break;
//	case SYSTEM_EVENT_STA_DISCONNECTED:
//		ESP_LOGI(TAG, "Disconnected from sta_mode\n");
//		break;
//	default:
//		break;
//	}
//	mdns_handle_system_event(ctx, event);
//	return ESP_OK;
//}
static esp_err_t event_handler(void *ctx, system_event_t *event){
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        /* enable ipv6 */
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, IP4_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        xEventGroupSetBits(wifi_event_group, IP6_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        if (auto_reconnect) {
            esp_wifi_connect();
        }
        xEventGroupClearBits(wifi_event_group, IP4_CONNECTED_BIT | IP6_CONNECTED_BIT);
        break;
    default:
        break;
    }
    mdns_handle_system_event(ctx, event);
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

	//long systemTime = getTime();

	//printf("System: %ld --- packet: %d --- total: %ld\n\n", systemTime, (int)(ppkt->rx_ctrl.timestamp/1000000), (long)(ppkt->rx_ctrl.timestamp/1000000+systemTime));

	const Wifi_packet packet(hdr->addr3, hdr->addr2, ppkt->rx_ctrl.rssi, getTime(), ssid_, *ssid_len + 1);
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
