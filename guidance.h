#ifndef GUIDANCE_H_
#define GUIDANCE_H_


void* capture(void* arg);
void* vaccuum(void* arg);
void toggleVac();
const char* isVacOn();
void startCapture();
void stopCapture();
int readSensor();
int mkUpMind();
void turn(int degrees);
char* base64img();

#define SENSOR_COORD 320
#define CLOSE_OBJECT 50
#define ERR_BAR 25
#endif
