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
#include "freertos/semphr.h"
#include <stdio.h>

void sniffingFunc();
void storingFunc();
void createSemaphore();
void tasksCreation();

#endif /* CONTROLLER_H_ */
