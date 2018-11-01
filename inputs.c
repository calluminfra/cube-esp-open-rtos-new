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
#include <queue.h>
#include <semphr.h>
#include <task.h>

extern SemaphoreHandle_t i2CCountingSemaphore;
extern QueueHandle_t xGPIOForProcQueue;

/*Thread reads temperature and places into current temp variable*/
void thermocoupleReadThread(void *pvParameters) {
  /*Use Mutex to write the new value*/
}

/*Thread reads buttons and scroll and feeds forward the action to
 * updateParameters thread*/
void buttonPollThread(void *pvParameters) {

  // Initialise masks for button debounce
  static uint8_t selectStateHistory;
  static uint8_t backStateMaskHistory;
  /*Will need to add in the rotary encoder stuff here*/

  QueueHandle_t *queue = (QueueHandle_t *)pvParameters;

  struct buttonPressQueue sentFromButtonPollThread;
  struct buttonPressQueue *pSentFromButtonPollThread;
  sentFromButtonPollThread.opType = buttonThreadMessage;

  uint8_t *rxButton;

  static uint8_t selButtonState, backButtonState, rotaryLPin, rotaryRPin;

  while (1) {
    // Wait for update to
    if (xQueueReceive(xGPIOForProcQueue, &(rxButton), 2)) {
      uint8_t buttonMask = rxButton;
      buttonMask = buttonMask >> 4;
      rotaryLPin = (buttonMask & 0b1000) >> 3;
      rotaryRPin = (buttonMask & 0b0100) >> 2;
      backButtonState = (buttonMask & 0b0010) >> 1;
      selButtonState = buttonMask & 0b0001;

      if (selButtonState == 1 && selectStateHistory == 0) {
        if (xSemaphoreTake(i2CCountingSemaphore, 50) == pdTRUE) {
          sentFromButtonPollThread.data.buttonOperation = 1;
          pSentFromButtonPollThread = &sentFromButtonPollThread;
          xQueueSend(*queue, &pSentFromButtonPollThread, 0);
          selectStateHistory = 1;
        }
      } else if (selButtonState == 0 && selectStateHistory == 1) {
        selectStateHistory = 0;
      }

      if (backButtonState == 1 && backStateMaskHistory == 0) {
        if (xSemaphoreTake(i2CCountingSemaphore, 50) == pdTRUE) {
          sentFromButtonPollThread.data.buttonOperation = 2;
          pSentFromButtonPollThread = &sentFromButtonPollThread;
          xQueueSend(*queue, &pSentFromButtonPollThread, 0);
          backStateMaskHistory = 1;
        }
      } else if (backButtonState == 0 && backStateMaskHistory == 1) {
        backStateMaskHistory = 0;
      }

      static uint8_t rotState = 0;
      static int8_t rotDir = 0;
      if (rotState == 0) {
        if (rotaryLPin == 0) {
          // CCW
          rotDir = -1;
          rotState = 1;
        } else if (rotaryRPin == 0) {
          // CW
          rotDir = 1;
          rotState = 1;
        }
      } else if (rotState == 1) {
        if (rotaryRPin == 1 && rotaryLPin == 1) {
          // reset
          rotDir = 0;
          rotState = 0;
        } else {
          rotState = 2;
        }
      } else {
        if (rotDir == 1) {
          if (xSemaphoreTake(i2CCountingSemaphore, 50) == pdTRUE) {
            sentFromButtonPollThread.data.buttonOperation = 3;
            pSentFromButtonPollThread = &sentFromButtonPollThread;
            xQueueSend(*queue, &pSentFromButtonPollThread, 0);
            // Place message in queue for proc thread
          } else {
            printf("Couldn't access i2c counting semaphore\r\n");
          }
        } else {
          if (xSemaphoreTake(i2CCountingSemaphore, 50) == pdTRUE) {
            sentFromButtonPollThread.data.buttonOperation = 4;
            pSentFromButtonPollThread = &sentFromButtonPollThread;
            xQueueSend(*queue, &pSentFromButtonPollThread, 0);
          } else {
            printf("Couldn't access i2c counting semaphore\r\n");
          }
        }
        rotDir = 0;
        rotState = 0;
      }
    }
  }
}
