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
#include <i2cThread.h>
#include <inputs.h>
#include <pcf8574/pcf8574.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

void i2CThread(void *pvParameters) {
  // Setup various I2C stuff HERE

  struct I2CVarsStruct *rxI2CVars;
  extern QueueHandle_t xI2CQueue;
  hd44780_t lcd = {
      .i2c_dev.bus = I2C_BUS,
      .i2c_dev.addr = HD44780,
      .font = HD44780_FONT_5X8,
      .lines = 2,
      .pins = {.rs = 0, .e = 2, .d4 = 4, .d5 = 5, .d6 = 6, .d7 = 7, .bl = 3},
      .backlight = true};

  while (1) {
    if (xQueueReceive(xI2CQueue, &(rxI2CVars), 50)) {
      printf("%s\r\n", rxI2CVars->dataForI2CQueue);
      hd44780_gotoxy(&lcd, 0, rxI2CVars->printRow);
      hd44780_puts(&lcd, rxI2CVars->dataForI2CQueue);
    }
  }

  // Wait for message queue here to print to LCD

  // Wait on semaphore for button polls
  // Send message to print thread & processing thread

  // Wait on semaphore for Temp reading
  // Send to the correct proc thread
}

void sendLCDOutI2C() {}

void sendThermoOutI2C() {}

void pollButtonsOutI2C() {}
