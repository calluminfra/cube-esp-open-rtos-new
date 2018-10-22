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
#include <queue.h>
#include <semphr.h>
#include <task.h>

/*STRINGS AND SUCH HERE*/
/*---------------------*/
const char menu0Strings[3][30] = {"InsertMenu0Strings"};

// Static string definitions
const char menu1Strings[3][30] = {
    "START / STOP",
    "RUN TYPE",
    "SETTINGS",
};

const char menu2Strings[3][30] = {
    "PID",
    "ON/OFF",
    "MANUAL 0-10V",
};

static const uint8_t taChar[8] = {0x1c, 0x08, 0x08, 0x08,
                                  0x02, 0x05, 0x07, 0x05};

static const uint8_t rgChar[8] = {0x1c, 0x14, 0x18, 0x14,
                                  0x03, 0x04, 0x05, 0x03};

static const uint8_t etChar[8] = {0x1c, 0x10, 0x18, 0x10,
                                  0x1F, 0x02, 0x02, 0x02};

static const uint8_t onOff[8] = {0x0E, 0x0A, 0x0A, 0x00,
                                 0x1B, 0x12, 0x1B, 0x12};

/*---------------------*/

// Preset bottom as doesn't want to init at 0

// Binary Semaphores
extern SemaphoreHandle_t printSemaphore;

// Mutex's
extern SemaphoreHandle_t pIDMutex;
extern SemaphoreHandle_t onOffMutex;
extern SemaphoreHandle_t vControlMutex;
extern SemaphoreHandle_t temperatureVarsMutex;
extern SemaphoreHandle_t processVarsMutex;
extern SemaphoreHandle_t menuVarsMutex;

/*Thread draws to LCD based on index*/
void drawDisplayThread(void *pvParameters) {

  // Setup LCD
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

  /*?!!?WAIT ON SEMAPHORE AND WRITE BASED ON THE MENU STATE?!!?*/
  /*data locked with mutex,*/
  if (printSemaphore != NULL) {
    while (1) {
      // Wait for semaphore
      if (xSemaphoreTake(printSemaphore, 10) == pdTRUE) {
        // OK to print!
        uint8_t menuLevel = 0;
        // Get current Menu Level
        if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
          menuLevel = menuVars.menuLevel;
          xSemaphoreGive(menuVarsMutex);
        } else {
          printf("could not grab the menu mutex so not printing\r\n");
        }
        hd44780_clear(&lcd);

        // Declare structs
        union TargetTempOrVolt {
          uint16_t targetTemp;
          float operatingVoltage;
        };

        struct Printingtruct {
          uint16_t currentTemp;
          union TargetTempOrVolt tempOrVoltVar;
          uint8_t procType;
          bool runState;
        } print;

        switch (menuLevel) {
        // Draw default view
        case 0:;

          // Grab process mutex
          if (xSemaphoreTake(processVarsMutex, 50) == pdTRUE) {
            print.procType = procVars.procType;
            print.runState = procVars.runState;
            xSemaphoreGive(processVarsMutex);
          } else {
            printf("could not grab the process mutex so not printing\r\n");
          }

          // Grab temp mutex
          if (xSemaphoreTake(temperatureVarsMutex, 50) == pdTRUE) {
            print.currentTemp = temperatureVars.currentTemperature;
            // Check if vcontrol enabled
            if (print.procType == 2) {
              if (xSemaphoreTake(vControlMutex, 50) == pdTRUE) {
                print.tempOrVoltVar.operatingVoltage =
                    vControlVars.outputVoltage;
                xSemaphoreGive(vControlMutex);
              } else {
                printf("Could not grab vcontrol mutex");
              }
            } else {
              print.tempOrVoltVar.targetTemp =
                  temperatureVars.targetTemperature;
            }
            xSemaphoreGive(temperatureVarsMutex);
          } else {
            printf("could not grab the process mutex so not printing\r\n");
          }

          // Print current temp
          hd44780_gotoxy(&lcd, 0, 0);
          hd44780_puts(&lcd, "Tmp  = ");
          char lcdBuffer[16];
          snprintf(lcdBuffer, 4, "%d   ", print.currentTemp);
          lcdBuffer[sizeof(lcdBuffer) - 1] = 0;
          hd44780_gotoxy(&lcd, 7, 0);
          hd44780_puts(&lcd, lcdBuffer);
          // printf("Current temp = %d\t\t", print.currentTemp);

          // print proc type
          switch (print.procType) {
          // PID
          case 0:
            hd44780_gotoxy(&lcd, 13, 0);
            hd44780_puts(&lcd, "PID");
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, "\x08\x09\x0A  = ");

            snprintf(lcdBuffer, 4, "%d   ", print.tempOrVoltVar.targetTemp);
            lcdBuffer[sizeof(lcdBuffer) - 1] = 0;
            hd44780_gotoxy(&lcd, 7, 1);
            hd44780_puts(&lcd, lcdBuffer);
            // printf("PID\r\n");
            break;
          // On Off
          case 1:
            hd44780_gotoxy(&lcd, 13, 0);
            hd44780_puts(&lcd, "O\x0B");
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, "\x08\x09\x0A  = ");

            snprintf(lcdBuffer, 4, "%d   ", print.tempOrVoltVar.targetTemp);
            lcdBuffer[sizeof(lcdBuffer) - 1] = 0;
            hd44780_gotoxy(&lcd, 7, 1);
            hd44780_puts(&lcd, lcdBuffer);
            // printf("On/Off\r\n");
            break;
          // print Manual votlage
          case 2:
            hd44780_gotoxy(&lcd, 13, 0);
            hd44780_puts(&lcd, "Man");
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, "Vout = ");

            snprintf(lcdBuffer, 5, "%2.2f   ",
                     print.tempOrVoltVar.operatingVoltage);
            lcdBuffer[sizeof(lcdBuffer) - 1] = 0;
            hd44780_gotoxy(&lcd, 7, 1);
            hd44780_puts(&lcd, lcdBuffer);
            // printf("V Control\r\n");
            break;
          }

          hd44780_gotoxy(&lcd, 13, 1);
          // print run state
          if (print.runState == 0) {
            hd44780_puts(&lcd, "   ");
          } else {

            hd44780_puts(&lcd, "RUN");
          }
          break;

        // Draw main menu
        case 1:

          if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
            hd44780_clear(&lcd);
            if (menuVars.selectorPos == 0) {
              hd44780_gotoxy(&lcd, 0, 0);
              hd44780_puts(&lcd, ">");
              hd44780_gotoxy(&lcd, 1, 0);
              hd44780_puts(&lcd, menu1Strings[menuVars.topElement]);
              hd44780_gotoxy(&lcd, 1, 1);
              hd44780_puts(&lcd, menu1Strings[menuVars.bottomElement]);
            } else {

              hd44780_gotoxy(&lcd, 1, 0);
              hd44780_puts(&lcd, menu1Strings[menuVars.topElement]);
              hd44780_gotoxy(&lcd, 0, 1);
              hd44780_puts(&lcd, ">");
              hd44780_gotoxy(&lcd, 1, 1);
              hd44780_puts(&lcd, menu1Strings[menuVars.bottomElement]);
            }
            xSemaphoreGive(menuVarsMutex);
          } else {
            printf("Could not grab menuVars Mutex");
          }
          break;

        // Draw proc selection
        case 2:
          if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
            hd44780_clear(&lcd);
            if (menuVars.selectorPos == 0) {
              hd44780_gotoxy(&lcd, 0, 0);
              hd44780_puts(&lcd, ">");
              hd44780_gotoxy(&lcd, 1, 0);
              hd44780_puts(&lcd, menu2Strings[menuVars.topElement]);
              hd44780_gotoxy(&lcd, 1, 1);
              hd44780_puts(&lcd, menu2Strings[menuVars.bottomElement]);

            } else {
              hd44780_gotoxy(&lcd, 1, 0);
              hd44780_puts(&lcd, menu2Strings[menuVars.topElement]);
              hd44780_gotoxy(&lcd, 0, 1);
              hd44780_puts(&lcd, ">");
              hd44780_gotoxy(&lcd, 1, 1);
              hd44780_puts(&lcd, menu2Strings[menuVars.bottomElement]);
            }
            xSemaphoreGive(menuVarsMutex);
          } else {
            printf("Could not grab menuVars Mutex\r\n");
          }
          break;

        // Draw PID menu
        case 3:

          break;

        // Draw onOff menu
        case 4:

          break;
        default:
          printf("you shouldn't be here!\r\n");
          break;
        }
      }
    }
  }
}

/*Master thread responsible for updating different variables such as menu and
 * PID etc*/
/*This thread holds an image of the current menu level and will update variables
 * based on user decisions*/
void updateParametersThread(void *pvParameters) {
  QueueHandle_t *queue = (QueueHandle_t *)pvParameters;
  menuVars.bottomElement = 1;

  struct buttonPressQueue *rxSentFromButtonPollThread;
  while (1) {
    /*Wait for message to come in containing button press*/
    /*Var read with rxSentFromButtonPollThread->varHere */
    if (xQueueReceive(*queue, &(rxSentFromButtonPollThread), 50)) {

      switch (rxSentFromButtonPollThread->opType) {
      // defaultMessage
      case 0:

        break;

      // buttonThreadMessage
      case 1:;
        uint8_t buttonPasser = rxSentFromButtonPollThread->data.buttonOperation;
        updateMenu(buttonPasser);
        // Signal printThread to update screen
        xSemaphoreGive(printSemaphore);
        break;

      // pidThreadMessage
      case 2:

        break;

      // onOffThreadMessage
      case 3:

        break;

      // voltageControlThreadMessage
      case 4:

        break;
      }
    }
  }
}

void updateMenu(uint8_t buttonPressed) {
  // Do action based on menu level
  static uint8_t currentMenu;
  uint8_t currentProc;

  // Add any new static menu string arrays here
  static uint8_t sizeOfMenus[] = {
      (sizeof(menu0Strings) / sizeof(menu0Strings[0])),
      (sizeof(menu1Strings) / sizeof(menu1Strings[0])),
      (sizeof(menu2Strings) / sizeof(menu2Strings[0]))};

  if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
    currentMenu = menuVars.menuLevel;
  } else {
    printf("Couldn't access the menu semaphore \r\n");
  }
  xSemaphoreGive(menuVarsMutex);
  if (xSemaphoreTake(processVarsMutex, 50) == pdTRUE) {
    currentProc = procVars.procType;
  } else {
    printf("Couldn't access the process mutex");
  }
  xSemaphoreGive(processVarsMutex);

  // Check the menu level and switch based on this
  switch (currentMenu) {
  // !!At base Menu (Level 0)!!
  case 0:
    switch (buttonPressed) {
    // Button press moves into menu 1;
    case 1:
      enterNewMenu(1);
      break;
    // Button press does nothing
    case 2:
      break;
    // Button increases the 0 - 10V value if Vcontrol set
    case 3:
      if (currentProc == 2) {
        if (xSemaphoreTake(vControlMutex, 50) == pdTRUE) {
          if (vControlVars.outputVoltage <= 10.0) {
            vControlVars.outputVoltage += 0.1;
          }
        } else {
          printf("Could not accces the vcontrol mutex");
        }
        xSemaphoreGive(vControlMutex);
      } else {
      }
      break;
    // Button decreases the 0-10V value if Vcontrol set
    case 4:
      if (currentProc == 2) {
        if (xSemaphoreTake(vControlMutex, 50) == pdTRUE) {
          if (vControlVars.outputVoltage >= 0.1) {
            vControlVars.outputVoltage -= 0.1;
          }
        } else {
          printf("Could not accces the vcontrol mutex");
        }
        xSemaphoreGive(vControlMutex);
      } else {
      }
      break;
    }
    break;

  // !!At General selection menu (Level 1)!!
  case 1:
    switch (buttonPressed) {
    // move menu appropriate to selector & top element
    case 1:
        // Get the selectoritem
        ;
      uint8_t selectorItem = getSelectedItem();

      switch (selectorItem) {
      // Toggle Run and send back to base menu (Level 0)
      case 0:
        if (xSemaphoreTake(processVarsMutex, 50) == pdTRUE) {
          if (procVars.runState == 0) {
            procVars.runState = 1;
            // Signal thread to resume?
          } else {
            procVars.runState = 0;
            // Signal thread to pause?
          }
          xSemaphoreGive(processVarsMutex);
        } else {
          printf("Couldn't access the menu semaphore \r\n");
        }
        enterNewMenu(0);
        break;
      // Enter Process selector Menu (Level 2)
      case 1:
        enterNewMenu(2);
        break;
      // Enter Further options menu stuff here (E.g. Wifi Settings Etc)
      case 2:
        break;
      }
      // If the selector is toggle run

      //

      break;

    // go back to Base Menu (Level 0)
    case 2:
      enterNewMenu(0);
      break;

    // Scroll up in menu 1
    case 3:
      moveSelectorDown(sizeOfMenus[currentMenu]);
      break;

    // Scroll down in menu 1
    case 4:
      moveSelectorUp(sizeOfMenus[currentMenu]);
      break;
    }
    break;

  // !!At process selection menu (Level 2)!!
  case 2:
    switch (buttonPressed) {
    // Select the appropriate item from the menu
    case 1:;
      // get selected item
      uint8_t selectorItem = getSelectedItem();
      switch (selectorItem) {
      // Enter PID Menu
      case 0:
        // Enter Menu3
        break;
      // Enter OnOff menuVars
      case 1:
        // Enter menu4
        break;
      // Set control to manual and go back to menu 0
      case 2:
        if (xSemaphoreTake(processVarsMutex, 50) == pdTRUE) {
          procVars.procType = 2;
        } else {
          printf("couldn't access the processVars mutex");
        }
        xSemaphoreGive(processVarsMutex);
        enterNewMenu(0);
        break;
      }
      break;

    case 2:
      // Go back to general selection menu (Level 1)
      enterNewMenu(1);
      break;

    case 3:
      moveSelectorDown(sizeOfMenus[currentMenu]);
      break;

    case 4:
      moveSelectorUp(sizeOfMenus[currentMenu]);
      break;
    }

    break;

  // At PID menu
  case 3:
    break;

  // At OnOff menu
  case 4:
    break;
  }
}

// move selector / menu item down and redraw
void moveSelectorDown(uint8_t sizeOfMenuStrings) {
  // Grab mutex to make changes to the menu
  if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
    if (menuVars.selectorPos == 0) {
      menuVars.selectorPos = 1;
    } else {
      // uint8_t sizeOfMenu1Strings =
      // sizeof(menu1Strings)/sizeof(menu1Strings[0]);

      if ((menuVars.topElement != sizeOfMenuStrings - 1) &&
          (menuVars.bottomElement != sizeOfMenuStrings - 1)) {
        menuVars.topElement++;
        menuVars.bottomElement++;
      } else {
        if (menuVars.bottomElement >= sizeOfMenuStrings - 1) {
          menuVars.bottomElement = 0;
          menuVars.topElement++;
        } else {
          menuVars.topElement = 0;
          menuVars.bottomElement++;
        }
      }
    }
    printf("Sel = %d, top = %d, bot = %d\r\n", menuVars.selectorPos,
           menuVars.topElement, menuVars.bottomElement);
    xSemaphoreGive(menuVarsMutex);
  } else {
    printf("Menu mutex could not be accessed and selector has not been moved "
           "as a result\r\n");
  }
}

// move selector / menu item up and redraw
void moveSelectorUp(uint8_t sizeOfMenuStrings) {
  // Grab mutex to make changes to the menu
  if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
    if (menuVars.selectorPos == 1) {
      menuVars.selectorPos = 0;
    } else {
      // uint8_t sizeOfMenu1Strings =
      // sizeof(menu1Strings)/sizeof(menu1Strings[0]);
      if ((menuVars.topElement != 0) && (menuVars.bottomElement != 0)) {
        menuVars.topElement--;
        menuVars.bottomElement--;
      } else {
        if (menuVars.bottomElement == 0) {
          menuVars.bottomElement = sizeOfMenuStrings - 1;
          menuVars.topElement--;
        } else {
          menuVars.topElement = sizeOfMenuStrings - 1;
          menuVars.bottomElement--;
        }
      }
    }
    printf("Sel = %d, top = %d, bot = %d\r\n", menuVars.selectorPos,
           menuVars.topElement, menuVars.bottomElement);
    xSemaphoreGive(menuVarsMutex);
  } else {
    printf("Menu mutex could not be accessed and selector has not been moved "
           "as a result\r\n");
  }
}

uint8_t getSelectedItem() {
  // Pass back the item that has been selected
  if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
    uint8_t menuSelection;
    // Take top element
    if (menuVars.selectorPos == 0) {
      menuSelection = menuVars.topElement;
    }
    // take bottom element
    else {
      menuSelection = menuVars.bottomElement;
    }
    xSemaphoreGive(menuVarsMutex);
    return menuSelection;
  } else {
    printf("Menu mutex could not be accessed and selector has not been moved "
           "as a result\r\n");
    return 0;
  }
}

void enterNewMenu(uint8_t menuToEnter) {
  if (xSemaphoreTake(menuVarsMutex, 50) == pdTRUE) {
    menuVars.menuLevel = menuToEnter;
    menuVars.selectorPos = 0;
    menuVars.topElement = 0;
    menuVars.bottomElement = 1;
    xSemaphoreGive(menuVarsMutex);
  } else {
    printf("Couldn't access the menu semaphore from the entermenu func\r\n");
  }
}

void calcMenuMoveAccel() {}

/*
increaseOnScreenVar(){
    //Increase a temp var and redraw
}

decreaseOnScreenVar(){
    //decrease a temp var and redraw
}

updateVar(){
    //update a specific variable to = the temp var
}
*/
