#ifndef I2CTHREAD_H
#define I2CTHREAD_H

//#define MCP23016 0x20

void i2CThread(void *pvParameters);
void sendLCDOutI2C();

void sendThermoOutI2C();

uint8_t pollButtonsI2C();

#endif
