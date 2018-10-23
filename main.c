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
#include <burnerControl.h>
#include <esp8266.h>
#include <gui.h>
#include <hd44780/hd44780.h>
#include <inputs.h>
#include <pcf8574/pcf8574.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

//
// Handle Declarations
// Queues
QueueHandle_t buttonQueue;

// Binary Semaphores
SemaphoreHandle_t printSemaphore;

// Mutexes
SemaphoreHandle_t pIDMutex;
SemaphoreHandle_t onOffMutex;
SemaphoreHandle_t vControlMutex;
SemaphoreHandle_t temperatureVarsMutex;
SemaphoreHandle_t processVarsMutex;
SemaphoreHandle_t menuVarsMutex;

void user_init(void) {

  // Init Uart & I2C interfaces
  uart_set_baud(0, 115200);
  i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_100K);
  uint8_t devAddr = 0x20;
  uint8_t dataBuffer[] = {0x06, 0xFF};
  int err = i2c_slave_write(I2C_BUS, devAddr, NULL, &dataBuffer, 2);

  if (err != 0) {
    printf("oops\r\n");
  }

  dataBuffer[0] = 0;
  err = i2c_slave_write(I2C_BUS, devAddr, NULL, &dataBuffer, 1);
  if (err != 0) {
    printf("oops2\r\n");
  }

  err = i2c_slave_read(I2C_BUS, devAddr, NULL, &dataBuffer, 1);
  if (err != 0) {
    printf("oops3\r\n");
  } else {
    printf("%2x\r\n", dataBuffer[0]);
  }
  /*
  uint32_t deviceAddr = 0x20;
  uint8_t i2cBuffer[] = {0x06, 0xFF};
  pcf8574_port_write_buf(&deviceAddr, &i2cBuffer, 2);

  pcf

  /*Create Queues*/
  buttonQueue = xQueueCreate(20, sizeof(uint32_t));

  // Create and take semaphore to initially block print thread
  vSemaphoreCreateBinary(printSemaphore);
  xSemaphoreTake(printSemaphore, 10);

  // Create Mutex's for handling access to datastructures
  pIDMutex = xSemaphoreCreateMutex();
  onOffMutex = xSemaphoreCreateMutex();
  vControlMutex = xSemaphoreCreateMutex();
  temperatureVarsMutex = xSemaphoreCreateMutex();
  processVarsMutex = xSemaphoreCreateMutex();
  menuVarsMutex = xSemaphoreCreateMutex();

  // Initialise threads
  xTaskCreate(buttonPollThread, "btnPoll", 256, &buttonQueue, 2, NULL);
  xTaskCreate(updateParametersThread, "updtParam", 1024, &buttonQueue, 2, NULL);
  xTaskCreate(drawDisplayThread, "drawDsp", 1024, NULL, 3, NULL);
}
