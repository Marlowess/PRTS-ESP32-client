#include "../include/wifi_packet.h"
#include <stdio.h>
#include <iostream>
#include <string>

/*
		uint8_t getSenderAddr();
		uint8_t getReceiverAddr();
		signed getRssi();
		unsigned getTimestamp();

*/
/* It returns sender address */
std::string Wifi_packet::getSenderAddr(){
	//std::string s = int_array_to_string(this->addr2, 6);
	//std::ostringstream oss;
	char str[18] = "";
	for(int i = 0; i < 5; i++)
		sprintf(str, "%s%02x:", str, this->addr2[i]);
	sprintf(str, "%s%02x", str, this->addr2[5]);
	return std::string(str);
}

/* It returns receiver address */
std::string Wifi_packet::getReceiverAddr(){
	std::string s = int_array_to_string(this->addr1, 6);
	return s;
}

/* It returns signal intensity of packet */
signed Wifi_packet::getRssi(){

	return this->rssi;
}

std::string Wifi_packet::getSSID(){
	return this->ssid;
}

unsigned Wifi_packet::getTimestamp(){
	return this->timestamp;
}	

void Wifi_packet::printData(){
	printf("RSSI=%02d,"
		" ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
		" ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
		" TIMESTAMP= %d ",
		this->rssi,
		 /* ADDR1 */
		this->addr1[0],this->addr1[1],this->addr1[2],
		this->addr1[3],this->addr1[4],this->addr1[5],
		 /* ADDR2 */
		this->addr2[0],this->addr2[1],this->addr2[2],
		this->addr2[3],this->addr2[4],this->addr2[5],
		this->timestamp
	);
	
	std::cout << "SSID= " << ssid << "\n" << std::endl;
	
}

std::string Wifi_packet::retrieveData(){
	std::ostringstream oss;
	std::string senderMAC = getSenderAddr();
	std::string ssid = getSSID();
	std::string timestamp = std::to_string(getTimestamp());
	std::string rssi = std::to_string(getRssi());
	oss << senderMAC << "," << ssid << "," << timestamp << "," << rssi << "\r\n";
	std::string var = oss.str();
	return var;
}





