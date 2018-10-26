#ifndef INPUTS_H
#define INPUTS_H

void thermocoupleReadThread(void *pvParameters);
void buttonPollThread(void *pvParameters);

enum OperationType {
  defaultMessage,
  buttonThreadMessage,
  pidThreadMessage,
  onOffThreadMessage,
  voltageControlThreadMessage
};

union DataForQueue {
  uint16_t temperature;
  uint8_t buttonOperation;
};

struct buttonPressQueue {
  enum OperationType opType;
  union DataForQueue data;
};

#endif
