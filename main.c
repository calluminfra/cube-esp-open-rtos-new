/*
 * Example of using driver for text LCD
 * connected to I2C by PCF8574
 *
 * Part of esp-open-rtos
 * Copyright (C) 2016 Ruslan V. Uss <unclerus@gmail.com>
 * BSD Licensed as described in the file LICENSE
 */
#include <esp/uart.h>
#include <espressif/esp_common.h>
#include <i2c/i2c.h>
#include <stdio.h>
//TEst
#include <hd44780/hd44780.h>
#include <task.h>
#include <queue.h>
#include <FreeRTOS.h>
#include <esp8266.h>
#include <semphr.h>
#include <gui.h>
#include <burnerControl.h>
#include <inputs.h>

//
//Handle Declarations
//Queues
QueueHandle_t buttonQueue;

//Binary Semaphores
SemaphoreHandle_t printSemaphore;

//Mutexes
SemaphoreHandle_t pIDMutex;
SemaphoreHandle_t onOffMutex;
SemaphoreHandle_t vControlMutex;
SemaphoreHandle_t temperatureVarsMutex;
SemaphoreHandle_t processVarsMutex;
SemaphoreHandle_t menuVarsMutex;


void user_init(void){

  //Init Uart & I2C interfaces
  uart_set_baud(0, 115200);
  i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_100K);

  //!!REMOVE ME!!
  procVars.procType = 2;

  /*Create Queues*/
  buttonQueue = xQueueCreate(20, sizeof(uint32_t));

  //Create and take semaphore to initially block print thread
  vSemaphoreCreateBinary(printSemaphore);
  xSemaphoreTake(printSemaphore, 10);

  //Create Mutex's for handling access to datastructures
  pIDMutex = xSemaphoreCreateMutex();
  onOffMutex = xSemaphoreCreateMutex();
  vControlMutex = xSemaphoreCreateMutex();
  temperatureVarsMutex = xSemaphoreCreateMutex();
  processVarsMutex = xSemaphoreCreateMutex();
  menuVarsMutex = xSemaphoreCreateMutex();

  //Initialise threads
  xTaskCreate(buttonPollThread, "btnPoll", 256, &buttonQueue, 2, NULL);
  xTaskCreate(updateParametersThread, "updtParam", 1024, &buttonQueue, 2, NULL);
  xTaskCreate(drawDisplayThread, "drawDsp", 1024, NULL, 3, NULL);
}
