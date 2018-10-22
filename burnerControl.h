#ifndef BURNERCONTROL_H
#define BURNERCONTROL_H

void pidControlThread(void *pvParameters);
void onOffControlThread(void *pvParameters);
void vControlThread(void *pvParameters);

#endif
