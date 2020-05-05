/*
 * controller.h
 *
 *  Created on: 03 ago 2018
 *      Author: stefano
 */

#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include <tcpip_adapter.h>
#include <lwip/sockets.h>
#include "esp_wifi_types.h"
#include <lwip/sockets.h>
#include <inttypes.h>

#include <sys/time.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
 */
#define EXAMPLE_ESP_WIFI_MODE_AP   CONFIG_ESP_WIFI_MODE_AP //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "simple wifi";

#define	WIFI_CHANNEL_MAX		(13)
#define	WIFI_CHANNEL_SWITCH_INTERVAL	(500)

static wifi_country_t wifi_country = {.cc="CN", .schan=1, .nchan=13, .policy=WIFI_COUNTRY_POLICY_AUTO};

int setSelect(int s);
int doActionSelect(int n, int s);
void clearSetSelect(int s);

/********************  STRUCTURES FOR WIFI PACKET *************************/

/**
 * Header wifi-packet
 *
 */
typedef struct {
	unsigned frame_ctrl:16;
	unsigned duration_id:16;
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	uint8_t addr3[6]; /* filtering address */
	unsigned sequence_ctrl:16;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

/**
 * Wifi-packet: header + data
 *
 */
typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

void sniffingFunc();
void storingFunc();
void tasksCreation();
static esp_err_t event_handler(void *ctx, system_event_t *event);
void wifi_init_sta();
void wifi_sniffer_init();
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
void changeChannel();
void printTime();
void setTime(time_t);
unsigned long getTime();

#endif /* CONTROLLER_H_ */
