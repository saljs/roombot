#!/bin/bash

SERVO=20
UP=0.2
DOWN=0.12

/usr/bin/pi-blaster -g $SERVO > /dev/null

if [[ "$1" == "up" ]]
then
    echo "$SERVO=$UP" > /dev/pi-blaster
elif [[ "$1" == "down" ]]
    echo "$SERVO=$DOWN" > /dev/pi-blaster
else
    echo "$SERVO=$1" > /dev/pi-blaster
fi

sleep 0.5
echo "release $SERVO" > /dev/pi-blaster
