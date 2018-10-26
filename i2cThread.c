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

extern SemaphoreHandle_t i2CMutex;
extern SemaphoreHandle_t i2CCountingSemaphore2;
extern QueueHandle_t xGPIOForProcQueue;

void i2CThread(void *pvParameters) {
  // Setup various I2C stuff HERE
  TickType_t lastWakeTime;
  const TickType_t xFreq = 20 / portTICK_PERIOD_MS;
  lastWakeTime = xTaskGetTickCount();
  // Setup MCP23016 as inputs for all GP0
  uint8_t devAddr = 0x20;
  uint8_t dataBuffer[4] = {0x06, 0xFF, 0, 0};

  int err = i2c_slave_write(I2C_BUS, devAddr, NULL, &dataBuffer, 2);
  if (err != 0) {
    printf("Could not set the inputs for MCP\r\n");
  }

  static const uint8_t taChar[8] = {0x1c, 0x08, 0x08, 0x08,
                                    0x02, 0x05, 0x07, 0x05};

  static const uint8_t rgChar[8] = {0x1c, 0x14, 0x18, 0x14,
                                    0x03, 0x04, 0x05, 0x03};

  static const uint8_t etChar[8] = {0x1c, 0x10, 0x18, 0x10,
                                    0x1F, 0x02, 0x02, 0x02};

  static const uint8_t onOff[8] = {0x0E, 0x0A, 0x0A, 0x00,
                                   0x1B, 0x12, 0x1B, 0x12};

  hd44780_t lcd = {
      .i2c_dev.bus = I2C_BUS,
      .i2c_dev.addr = HD44780,
      .font = HD44780_FONT_5X8,
      .lines = 2,
      .pins = {.rs = 0, .e = 2, .d4 = 4, .d5 = 5, .d6 = 6, .d7 = 7, .bl = 3},
      .backlight = true};

  hd44780_init(&lcd);
  hd44780_upload_character(&lcd, 0, taChar);
  hd44780_upload_character(&lcd, 1, rgChar);
  hd44780_upload_character(&lcd, 2, etChar);
  hd44780_upload_character(&lcd, 3, onOff);

  struct I2CVarsStruct *rxI2CVars;
  extern QueueHandle_t xI2CQueue;

  while (1) {
    if (xQueueReceive(xI2CQueue, &(rxI2CVars), 0)) {
      // uint8_t testVar1 = pollButtonsI2C();
      taskENTER_CRITICAL();
      printf("%s\r\n", rxI2CVars->dataForI2CQueue);
      hd44780_gotoxy(&lcd, 0, rxI2CVars->printRow);
      hd44780_puts(&lcd, rxI2CVars->dataForI2CQueue);
      taskEXIT_CRITICAL();
    }
    vTaskDelayUntil(&lastWakeTime, xFreq);
    static uint8_t lastGPIOBuffer = 0;
    taskENTER_CRITICAL();
    dataBuffer[0] = 0;
    err = i2c_slave_write(I2C_BUS, devAddr, NULL, &dataBuffer, 1);
    if (err != 0) {
      printf("Could address the slave to prep read\r\n");
    }
    err = i2c_slave_read(I2C_BUS, devAddr, NULL, &dataBuffer, 1);
    if (err != 0) {
      printf("couldn't read from slave\r\n");
    } else {
      // printf("%2x, %2x\r\n", dataBuffer[0], dataBuffer[1]);
    }
    taskEXIT_CRITICAL();
    // static uint8_t *pDataBuffer;
    if (dataBuffer[0] != lastGPIOBuffer && dataBuffer[0] != 0) {
      // Send value to thread for processing

      uint8_t txDataBuffer = dataBuffer[0];
      // pDataBuffer = &txDataBuffer;
      xQueueSend(xGPIOForProcQueue, &txDataBuffer, 10);
    }
    lastGPIOBuffer = dataBuffer[0];
  }

  // Wait for message queue here to print to LCD

  // Wait on semaphore for button polls
  // Send message to print thread & processing thread

  // Wait on semaphore for Temp reading
  // Send to the correct proc thread
}

void sendLCDOutI2C() {}

void sendThermoOutI2C() {}

uint8_t pollButtonsI2C() {
  /*
  uint8_t devAddr = 0x20;
  uint8_t dataBuffer[4] = {0x06, 0xFF, 0, 0};

  if (xSemaphoreTake(i2CMutex, 1) == pdTRUE) {
    dataBuffer[0] = 0;
    int err = i2c_slave_write(I2C_BUS, devAddr, NULL, &dataBuffer, 1);
    if (err != 0) {
      printf("Could address the slave to prep read\r\n");
    }
    err = i2c_slave_read(I2C_BUS, devAddr, NULL, &dataBuffer, 4);
    if (err != 0) {
      printf("couldn't read from slave\r\n");
    } else {
      printf("%2x, %2x\r\n", dataBuffer[0], dataBuffer[1]);
    }
    xSemaphoreGive(i2CMutex);
  } else {
    printf("Couldn't acquire mutex in button poll\r\n");
  }
  */
  return 0;
}
