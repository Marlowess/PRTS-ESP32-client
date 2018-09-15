/*
 *	This class holds informations about
 *	WiFi packets, captured by board in promiscous mode
 *
 *	Added by Stefano Brilli, monday may 7th 2018
 */
#include <stdint.h>// or inttypes.h
#include <string>
#include <iterator>
#include <sstream>

class Wifi_packet{
public:
	Wifi_packet(const uint8_t addr1[6], const uint8_t addr2[6], signed rssi, unsigned timestamp, char* ssid_, int dim){

		for(int i = 0; i < 6; i++){
			this->addr1[i] = addr1[i];
			this->addr2[i] = addr2[i];
		}

		this->rssi = rssi;
		this->timestamp = timestamp;
		std::string str;
		std::string ssid2(ssid_, dim);
		ssid = ssid2;

	};

	~Wifi_packet(){};
	void printData();
	std::string getSenderAddr();
	std::string getReceiverAddr();
	signed getRssi();
	unsigned getTimestamp();
	std::string getSSID();
	std::string int_array_to_string(uint8_t int_array[], int size_of_array) {
		std::ostringstream oss("");
		for (uint8_t temp = 0; temp < size_of_array; temp++)
			oss << int_array[temp];
		return oss.str();
	}
	std::string retrieveData();

private:
	uint8_t addr1[6]; /* receiver address */
	uint8_t addr2[6]; /* sender address */
	signed rssi:8; /**< signal intensity of packet */
	unsigned timestamp:32; /**< timestamp */
	std::string ssid;
};
