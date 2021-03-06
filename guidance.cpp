#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <wiringPi.h>

#include "guidance.h"
#include "hardware.h"

using namespace std;
using namespace cv;

Mat cameraFrame;
pthread_mutex_t lockImg = PTHREAD_MUTEX_INITIALIZER;
bool closeThread = false, vacOnbool = false;

void* capture(void* arg)
{
    VideoCapture camera;

    for(int i = 0; i < 5; i++)
    {
        if(camera.open(i))
            break;
    }
    camera.set(CV_CAP_PROP_FRAME_WIDTH,640);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT,480);

    while(!closeThread)
    {
        if(pthread_mutex_trylock(&lockImg) == 0)
        {
            if(!camera.read(cameraFrame) || !camera.isOpened())
            {
                for(int i = 0; i < 5; i++)
                {
                    if(camera.open(i))
                        break;
                }
            }
            pthread_mutex_unlock(&lockImg);
        }
        else
        {
            Mat junk;
            if(!camera.read(junk) || !camera.isOpened())
            {
                for(int i = 0; i < 5; i++)
                {
                    if(camera.open(i))
                        break;
                }
            }
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void* vaccuum(void* arg)
{
    digitalWrite(VAC, 1);
    delay(1000);
    while(vacOnbool)
    {
        drunkWalk();
    }
    digitalWrite(VAC, 0);
    pthread_exit(NULL);
    return NULL;
}

void toggleVac()
{
    if(!vacOnbool)
    {
        vacOnbool = true;
        pthread_t vacThread;
        pthread_create(&vacThread, NULL, vaccuum, NULL);
    }
    else
    {
        vacOnbool = false;
    }
}
const char* c_isVacOn()
{
    if(vacOnbool)
    {
        return "true";
    }
    return "false";
}
bool isVacOn()
{
    if(vacOnbool)
    {
        return true;
    }
    return false;
}
void startCapture()
{
    pthread_t captureThread;
    pthread_create(&captureThread, NULL, capture, NULL);
}

void stopCapture()
{
    closeThread = true;
}

int initHardware()
{	
    if(wiringPiSetup () == -1)
    {
        return 1;
    }
	pinMode(MOTORS, OUTPUT);
	pinMode(MOTOR_L, OUTPUT);	
    pinMode(MOTOR_R, OUTPUT);
	
    pinMode(VAC, OUTPUT);	
    pinMode(STATUS_LED, OUTPUT);
	
    pinMode(TRIG_F, OUTPUT);	
    pinMode(ECHO_F, INPUT);

    pinMode(TRIG_L, OUTPUT);	
    pinMode(ECHO_L, INPUT);
    
    pinMode(TRIG_R, OUTPUT);	
    pinMode(ECHO_R, INPUT);
	
    return 0;
}

char* base64img()
{
    vector<uchar> toSend;
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(FEED_QUAL);
    pthread_mutex_lock(&lockImg);
    imencode(".jpeg", cameraFrame, toSend, compression_params); //low quality jpeg
    pthread_mutex_unlock(&lockImg);
    string encoded = base64_encode(&*toSend.begin(), toSend.size());
    char* imgstring = (char *)malloc(strlen(encoded.c_str()) + 25);
    if(imgstring == NULL)
    {
        return NULL;
    }
    memset(imgstring, 0, strlen(encoded.c_str() + 25));
    strcat(imgstring, "data:image/jpeg;base64,");
    strcat(imgstring, encoded.c_str());
    return imgstring;
}

int readSensor(int trig, int echo)
{
    //we don't want to get stuck in an infinite loop, so set a small timout
    time_t timeout = time(NULL);
    //Send TRIG pulse
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    //Wait for ECHO start
    while(digitalRead(echo) == LOW && time(NULL) - timeout < 2);
    //Wait for ECHO end
    timeout = time(NULL);
    long startTime = micros();
    while(digitalRead(echo) == HIGH && time(NULL) - timeout < 2);
    long travelTime = micros() - startTime;
    //Get distance in cm
    return (int)((double)travelTime * 0.01715);
}
                                                
int mkUpMind()
{
    //start off with some manual checks to keep from hitting stuff too much
    int distance_front = readSensor(TRIG_F, ECHO_F);
    int distance_left = readSensor(TRIG_L, ECHO_L);
    int distance_right = readSensor(TRIG_R, ECHO_R);

    if(distance_front < 20)
    {
        //we are close to a thing here. Back up a bit and turn around.
        digitalWrite(MOTORS, 0);
        digitalWrite(MOTOR_L, 1);
        digitalWrite(MOTOR_R, 1);
        digitalWrite(MOTORS, 1);
        while(readSensor(TRIG_F, ECHO_F) < 20) 
            delay(DRIVE_DUR);
        digitalWrite(MOTORS, 0);
        
        //turn left or right
        if(readSensor(TRIG_L, ECHO_L) < readSensor(TRIG_R, ECHO_R))
        {
            //turn right
            return 90;
        }
        else
        {
            //turn left
            return -90;
        }
    }
    else if(distance_left < 20) 
    {
        //turn right
        return 90;
    }
    else if(distance_right < 20) 
    {
        //turn left
        return -90;
    }

    int freq[2048];
    Mat img;
    pthread_mutex_lock(&lockImg);
    cv::cvtColor(cameraFrame, img, CV_BGR2GRAY);
    pthread_mutex_unlock(&lockImg);
    int desPath, maxVal = 0, minVal = 255;
    for(int col = 0; col < img.cols; col++)
    {
        int sum = 0;
        for(int row = 0; row < img.rows; row++)
        {
            sum += (int)img.at<uchar>(row, col);
        }
        freq[col] = sum / img.rows;
        if(freq[col] < minVal)
        {
            minVal = freq[col];
        }
        if(freq[col] > maxVal)
        {
            maxVal = freq[col];
        }
    }
    int midpt = ((maxVal - minVal) / 2) + minVal;
    
    //calculate avg around sensor position
    int sensorAvgColor, sum = 0;
    for (int i = SENSOR_COORD - 10; i < SENSOR_COORD + 10; i++)
    {
        sum += freq[i];
    }
    sensorAvgColor = sum / 20;

    //determine obstical values to use
    if(distance_front > CLOSE_OBJECT)
    {
        //far away distance reading
        if(sensorAvgColor < midpt) //light obsticals
        {
            desPath = minVal; //seek out darkness
        }
        else //dark obsticals
        {
            desPath = maxVal; //go into the light
        }
    }
    else
    {
        //close up distance reading
        if(sensorAvgColor < midpt) //dark obsticals
        {
            desPath = maxVal; //go into the light
        }
        else //light obsticals
        {
            desPath = minVal; //seek out darkness
        }
    }
    
    //now comes the difficult bit. Use the value distribuion and info obtained from disance to calculate optimum route. It's a small search space so we'll just use a linear search.
    //look for the longest flattest bit that is as close to the desired path as possible
    int index, length = 0, longest = 0, Lindex;
    for(int i = 0; i < img.cols; i++)
    {
        if(desPath == minVal && freq[i] <= desPath + ERR_BAR)
        {
            index = i;
            length++;
            if(length > longest)
            {
                longest = length;
                Lindex = index;
            }
        }
        else if(desPath == maxVal && freq[i] >= desPath - ERR_BAR)
        {
            index = i;
            length++;
            if(length > longest)
            {
                longest = length;
                Lindex = index;
            }
        }
        else
        {
            length = 0;
        }
    }
    
    //draw nav line on image
    pthread_mutex_lock(&lockImg);
    if(longest >= 0 && longest <= 255)
        line(cameraFrame, Point(Lindex, 0), Point(Lindex, cameraFrame.rows), Scalar(0, 255, 0), longest);
    else
        line(cameraFrame, Point(Lindex, 0), Point(Lindex, cameraFrame.rows), Scalar(0, 255, 0), 2);
    pthread_mutex_unlock(&lockImg);
    //calculate degrees of direction
    int degrees;
    if( Lindex - (longest / 2) < img.cols / 2)
    {
        //turn left 
        degrees = -(FOV / 2) + (((double)Lindex / (img.cols / 2)) * (FOV / 2));
    }
    else
    {
        //turn right
        degrees = (((double)Lindex - (img.cols / 2)) / (img.cols / 2)) * (FOV / 2);
    }

    return degrees;
}

void turn(int degrees)
{
    //determine direction
    if(degrees < 0)
    {
        //left
        digitalWrite(MOTORS, 0); //turn off motors
        digitalWrite(MOTOR_L, 1);
        digitalWrite(MOTOR_R, 0);
        //engage turning
        digitalWrite(MOTORS, 1);
        delay(DEGREE*abs(degrees));
        digitalWrite(MOTORS, 0);
    }
    else if(degrees > 0)
    {
        //right
        digitalWrite(MOTORS, 0); //turn off motors
        digitalWrite(MOTOR_L, 0);
        digitalWrite(MOTOR_R, 1);
        //engage turning
        digitalWrite(MOTORS, 1);
        delay(DEGREE*degrees);
        digitalWrite(MOTORS, 0);
    }
}

void drunkWalk()
{
    turn(mkUpMind());
    //go forward
    digitalWrite(MOTORS, 0);
    digitalWrite(MOTOR_L, 0);
    digitalWrite(MOTOR_R, 0);
    digitalWrite(MOTORS, 1);
    delay(DRIVE_DUR);
    digitalWrite(MOTORS, 0);
    delay(200); //wait a fifth of a second to steady the camera
}
