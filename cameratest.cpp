#include "opencv2/opencv.hpp"
#include "base64.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

using namespace cv;
using namespace std;

Mat cameraFrame;
bool closeThread = false;

void* capture(void* arg)
{
    VideoCapture camera;
    if(!camera.open(0))
    {
        pthread_exit(NULL);
    }
    while(!closeThread)
    {
        camera >> cameraFrame;
    }
    pthread_exit(NULL);
    return NULL;
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

int main(int argc, char** argv)
{
    startCapture();
    sleep(3);
    for(;;)
    {
        imshow("camera", cameraFrame);
        if(waitKey(0) == 27)
        {
            closeThread = true;
            break;
        }
        string encoded = base64_encode(cameraFrame.data, cameraFrame.rows * cameraFrame.cols);
        char* imgstring = encoded.c_str();
        printf("%s\n", imgstring);
    }
    // the camera will be closed automatically upon exit
    // cap.close();
    return 0;
}
