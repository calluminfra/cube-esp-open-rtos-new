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
#include <pwm.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

extern SemaphoreHandle_t pIDMutex;
extern SemaphoreHandle_t onOffMutex;
extern SemaphoreHandle_t vControlMutex;
extern SemaphoreHandle_t temperatureVarsMutex;
extern SemaphoreHandle_t processVarsMutex;
extern SemaphoreHandle_t menuVarsMutex;
extern SemaphoreHandle_t i2CMutex;

/*Thread controls the PID output based on PID parameters and current/target
 * temps*/
void pwmOutputThread(void *pvParameters) {

  TickType_t lastWakeTime;
  const TickType_t xFreq = 100 / portTICK_PERIOD_MS;
  lastWakeTime = xTaskGetTickCount();

  printf("Starting pwm\r\n");
  uint8_t pwmPin[1];
  pwmPin[0] = 12;
  // Init PWM at 1 kHz
  pwm_init(1, pwmPin, false);
  pwm_set_freq(1000);
  pwm_set_duty(0);
  pwm_start();
  printf("Started pwm\r\n");

  while (1) {
    // Is the state of the burner set to run currently?
    if (xSemaphoreTake(processVarsMutex, 50) == pdTRUE) {
      bool runState = procVars.runState;
      uint8_t procType = procVars.procType;
      xSemaphoreGive(processVarsMutex);

      if (runState == 1) {
        switch (procType) {
        // PID set.
        case 0:
          break;
        // onOff set
        case 1:
          break;
        // Manual set
        case 2:
          if (xSemaphoreTake(vControlMutex, 20) == pdTRUE) {
            uint16_t pwmOutput =
                (UINT16_MAX / 100) * (vControlVars.outputVoltage * 10);
            pwm_set_duty(pwmOutput);

            xSemaphoreGive(vControlMutex);
          } else {
            printf("Couldn't acquire vControl in pwmthread\r\n");
          }
          break;
        }
      } else {
        // pwm_set_duty(0);
      }
    } else {
      printf("Couldn't acquire ProcMutex in the PWM output thread");
    }
    vTaskDelayUntil(&lastWakeTime, xFreq);
  }
}

/*Thread controls the onOff output based on on/off voltages and current/target
 * temps*/
void onOffControlThread(void *pvParameters) {}

/*Thread controls the output of the PWM based on the current Voltage setting*/
void vControlThread(void *pvParameters) {}
