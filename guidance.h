#ifndef GUIDANCE_H_
#define GUIDANCE_H_


void* capture(void* arg);
void* vaccuum(void* arg);
void* startCharge(void* arg);
void charge();
void toggleVac();
const char* c_isVacOn();
bool isVacOn();
void startCapture();
void stopCapture();
int initHardware();
int readSensor(int trig, int echo);
int mkUpMind();
void turn(int degrees);
void drunkWalk();
char* base64img();
int findCharger();
bool directedWalk();

#define SENSOR_COORD 355 //coordinates of distance sensor on the camera image
#define CLOSE_OBJECT 50
#define ERR_BAR 10
#define FEED_QUAL 10 //jpeg quality for video stream (0-100)
#define VAC_TIME 7200
#define QR_CODE "/root/roombot/qr_code.jpg"
#define MATCH_THRESH 0.9 //threshold for image match
#endif
