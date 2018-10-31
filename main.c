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
// TEst
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <burnerControl.h>
#include <esp8266.h>
#include <gui.h>
#include <hd44780/hd44780.h>
#include <i2cThread.h>
#include <inputs.h>
#include <pcf8574/pcf8574.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

//
// Handle Declarations
// Queues
QueueHandle_t buttonQueue;
QueueHandle_t xI2CQueue;
QueueHandle_t xGPIOForProcQueue;

// Counting Semaphores
SemaphoreHandle_t i2CCountingSemaphore;

// Binary Semaphores
SemaphoreHandle_t printSemaphore;

// Mutexes
SemaphoreHandle_t pIDMutex;
SemaphoreHandle_t onOffMutex;
SemaphoreHandle_t vControlMutex;
SemaphoreHandle_t temperatureVarsMutex;
SemaphoreHandle_t processVarsMutex;
SemaphoreHandle_t menuVarsMutex;
SemaphoreHandle_t i2CMutex;

void user_init(void) {

  // Init Uart & I2C interfaces
  // Test
  uart_set_baud(0, 115200);
  i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_100K);
  i2c_set_clock_stretch(I2C_BUS, 300);

  /*Create Queues*/
  buttonQueue = xQueueCreate(20, sizeof(uint32_t));
  xI2CQueue = xQueueCreate(100, sizeof(struct I2CVarsStruct *));
  xGPIOForProcQueue = xQueueCreate(20, sizeof(uint8_t));

  // Create and take semaphore to initially block print thread
  vSemaphoreCreateBinary(printSemaphore);
  xSemaphoreTake(printSemaphore, 10);

  // Create counting semaphore for use by I2C threads
  i2CCountingSemaphore = xSemaphoreCreateCounting(1, 1);

  // Create Mutex's for handling access to datastructures
  pIDMutex = xSemaphoreCreateMutex();
  onOffMutex = xSemaphoreCreateMutex();
  vControlMutex = xSemaphoreCreateMutex();
  temperatureVarsMutex = xSemaphoreCreateMutex();
  processVarsMutex = xSemaphoreCreateMutex();
  menuVarsMutex = xSemaphoreCreateMutex();
  i2CMutex = xSemaphoreCreateMutex();

  // Initialise threads
  printf("Starting Setup Thread\r\n");
  xTaskCreate(updateParametersThread, "updtParam", 1024, &buttonQueue, 2, NULL);
  xTaskCreate(drawDisplayThread, "drawDsp", 2048, NULL, 2, NULL);
  xTaskCreate(i2CThread, "i2c", 2048, NULL, 1, NULL);
  xTaskCreate(buttonPollThread, "btnPoll", 1024, &buttonQueue, 2, NULL);
  xTaskCreate(pwmOutputThread, "pwm", 1024, NULL, 5, NULL);

  enterNewMenu(0);
  xSemaphoreGive(printSemaphore);
}
