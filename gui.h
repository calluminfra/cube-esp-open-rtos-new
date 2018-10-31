#ifndef GUI_H
#define GUI_H

// I2C addressing
#define I2C_BUS 0
#define SCL_PIN 5
#define SDA_PIN 4
// HD44780 Addr
#define HD44780 0x27

void drawDisplayThread(void *pvParameters);
void updateParametersThread(void *pvParameters);
void updateMenu(uint8_t buttonPressed);
void moveSelectorDown(uint8_t sizeOfMenuStrings);
void moveSelectorUp(uint8_t sizeOfMenuStrings);
uint8_t getSelectedItem();
void enterNewMenu(uint8_t menuToEnter);

struct I2CVarsStruct {
  char dataForI2CQueue[24];
  uint8_t printRow;
};

struct PidVarsStruct {
  uint8_t pVar;
  uint8_t iVar;
  uint8_t dVar;
  uint8_t pIDSteppingRate;
} pidVars;

struct OnOffVarsStruct {
  float onVoltage;
  float offVoltage;
  uint8_t onOffSteppingRate;
  uint16_t steppingTemperature;
} onOffVars;

struct VControlVarsStruct {
  float outputVoltage;
} vControlVars;

struct TemperatureVarsStruct {
  uint16_t currentTemperature;
  uint16_t targetTemperature;
} temperatureVars;

struct ProcVarsStruct {
  bool runState;
  uint8_t procType; // 0 = PID, 1 = onoff, 2 = vcontrol
} procVars;

struct MenuVarsStruct {
  uint8_t selectorPos;
  uint8_t topElement;
  uint8_t bottomElement;
  uint8_t menuLevel;
  uint8_t temporaryPVar;
  uint8_t temporaryIVar;
  uint8_t temporaryDVar;
  uint8_t temporaryPIDSteppingRate;
  float temporaryOnVoltage;
  float temporaryOffVoltage;
  uint8_t temporaryOnOffSteppingRate;
} menuVars;

// extern const char menu0ptions[][];
extern const char menu1Options[3][30];

#endif
