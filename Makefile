CC=g++
CFLAGS=$(shell pkg-config --cflags opencv) -lm -fpermissive --debug
LDFLAGS=$(shell pkg-config --libs opencv) -lpthread -lwebsockets -lwiringPi

all: roombot

roombot: main.o base64.o guidance.o
	$(CC) $(CFLAGS) $(LDFLAGS) guidance.o base64.o main.o -o roombot

main.o: main.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c main.cpp

guidance.o: guidance.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c guidance.cpp
	
base64.o: base64.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -c base64.cpp

sensortest: sensortest.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) sensortest.cpp guidance.cpp base64.cpp -o sensortest

clean:
	rm *.o roombot
