#include <stdio.h>
#include <wiringPi.h>
#include "guidance.h"
#include "hardware.h"

int main(void) 
{
    if(wiringPiSetup () == -1)
    {
        fprintf(stderr, "WiringPi init failed\n");
        return -1;
    }
    printf("Distance front: %d CM\n", readSensor(TRIG_F, ECHO_F));
    printf("Distance left : %d CM\n", readSensor(TRIG_L, ECHO_L));
    printf("Distance right: %d CM\n", readSensor(TRIG_R, ECHO_R));

    return 0;
}
