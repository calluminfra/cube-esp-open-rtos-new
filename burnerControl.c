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

/*Thread controls the PID output based on PID parameters and current/target
 * temps*/
void pidControlThread(void *pvParameters) {}

/*Thread controls the onOff output based on on/off voltages and current/target
 * temps*/
void onOffControlThread(void *pvParameters) {}

/*Thread controls the output of the PWM based on the current Voltage setting*/
void vControlThread(void *pvParameters) {}
