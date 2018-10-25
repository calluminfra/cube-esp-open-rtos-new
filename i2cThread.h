#ifndef I2CTHREAD_H
#define I2CTHREAD_H

void i2CThread(void *pvParameters);
void sendLCDOutI2C();

void sendThermoOutI2C();

void pollButtonsOutI2C();

#endif
