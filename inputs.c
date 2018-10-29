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

  /*
    GPIO 12 = Input button 1
    GPIO 13 = Input button 2
    GPIO 14 = Rotary Encoder A
    GPIO 15 = Rotary Encoder B
  */
  // Set MCP registers as inputs
  // Set IO Dir

  const uint8_t enterButton = 12;
  const uint8_t backButton = 13;
  const uint8_t rotaryA = 14;
  const uint8_t rotaryB = 15;

  gpio_enable(enterButton, GPIO_INPUT);
  gpio_enable(backButton, GPIO_INPUT);
  gpio_enable(rotaryA, GPIO_INPUT);
  gpio_enable(rotaryB, GPIO_INPUT);

  // Initialise masks for button debounce
  static uint8_t selectStateHistory;
  static uint8_t backStateMaskHistory;
  /*Will need to add in the rotary encoder stuff here*/
  static uint8_t previousAState = 0;
  static uint8_t currentAState = 0;

  // Timing definitions for 10ms polls
  // TickType_t lastWakeTime;
  // const TickType_t tickFreq = 20 / portTICK_PERIOD_MS;
  // lastWakeTime = xTaskGetTickCount();

  QueueHandle_t *queue = (QueueHandle_t *)pvParameters;

  struct buttonPressQueue sentFromButtonPollThread;
  struct buttonPressQueue *pSentFromButtonPollThread;
  sentFromButtonPollThread.opType = buttonThreadMessage;

  uint8_t *rxButton;

  static uint8_t selButtonState, backButtonState, rotaryLPin, rotaryRPin;

  while (1) {
    // Wait for update to
    if (xQueueReceive(xGPIOForProcQueue, &(rxButton), 2)) {
      printf("!!!%2x\r\n", rxButton);
      uint8_t buttonMask = rxButton;
      buttonMask = buttonMask >> 4;
      rotaryLPin = (buttonMask & 0b1000) >> 3;
      rotaryRPin = (buttonMask & 0b0100) >> 2;
      backButtonState = (buttonMask & 0b0010) >> 1;
      selButtonState = buttonMask & 0b0001;
      printf("Buttons = %d, %d, %d, %d\r\n", rotaryLPin, rotaryRPin,
             backButtonState, selButtonState);

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

      // NEED TO FIX ROTARY ENCODER!!!
      static uint8_t inChange = 0;

      if (((rotaryLPin != 1) || (rotaryRPin != 1)) && (inChange == 0)) {
        if (rotaryLPin == 0) {
          // CCW
          if (xSemaphoreTake(i2CCountingSemaphore, 50) == pdTRUE) {
            sentFromButtonPollThread.data.buttonOperation = 4;
            pSentFromButtonPollThread = &sentFromButtonPollThread;
            xQueueSend(*queue, &pSentFromButtonPollThread, 0);
          } else {
            printf("Couldn't access i2c counting semaphore\r\n");
          }
        } else {
          // CW
          if (xSemaphoreTake(i2CCountingSemaphore, 50) == pdTRUE) {
            sentFromButtonPollThread.data.buttonOperation = 3;
            pSentFromButtonPollThread = &sentFromButtonPollThread;
            xQueueSend(*queue, &pSentFromButtonPollThread, 0);
            // Place message in queue for proc thread
          } else {
            printf("Couldn't access i2c counting semaphore\r\n");
          }
        }
        inChange = 1;
      } else {
        inChange = 0;
      }
      previousAState = currentAState;
    }
  }
}
