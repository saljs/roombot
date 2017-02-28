#ifndef GUIDANCE_H_
#define GUIDANCE_H_


void* capture(void* arg);
void* vaccuum(void* arg);
void toggleVac();
const char* c_isVacOn();
bool isVacOn();
void startCapture();
void stopCapture();
int initHardware();
int readSensor();
int mkUpMind();
void turn(int degrees);
char* base64img();

#define SENSOR_COORD 355 //coordinates of distance sensor on the camera image
#define CLOSE_OBJECT 50
#define ERR_BAR 10
#define FEED_QUAL 10 //jpeg quality for video stream (0-100)
#endif
