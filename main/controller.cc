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

/* I've to define a binary semaphore here */
static SemaphoreHandle_t xSemaphore;
TaskHandle_t xHandleSniffing;
TaskHandle_t xHandleStoring;
#define ebST_BIT_TASK_PRIORITY	( tskIDLE_PRIORITY )
#define ebSN_BIT_TASK_PRIORITY	( tskIDLE_PRIORITY + 1 )

void sniffingFunc(void *pvParameters){
	while(true){
		int i;
		//while(xSemaphoreTake(xSemaphore, 0) != pdTRUE);
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
		printf("Sniffing done, I'm talking with server...\n");
		sleep(3);
		xTaskNotifyGive(xHandleSniffing);
	}
}

void tasksCreation(){
	if(xTaskCreate(sniffingFunc, "Sniffing task", 2048, NULL, ebSN_BIT_TASK_PRIORITY, &xHandleSniffing) == pdPASS){
		printf("Sniffing task created!\n");
	}
	else{
		printf("Error on creating task 1!\n");
	}

	//xSemaphoreGive(xSemaphore);

	if(xTaskCreate(storingFunc, "Storing task", 2048, NULL, ebST_BIT_TASK_PRIORITY, &xHandleStoring) == pdPASS){
		printf("Storing task created!\n");
	}
	else{
		printf("Error on creating task 2!\n");
	}

	xTaskNotifyGive(xHandleSniffing);
}





