// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module

#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile

RH_ASK driver(1000,3,5);

void setup()
{
    Serial.begin(9600);	// Debugging only
    if (!driver.init())
         Serial.println("init failed");
   
     pinMode(13,OUTPUT);
     pinMode(12,OUTPUT);
     pinMode(11,OUTPUT);
     digitalWrite(13,LOW);
     digitalWrite(12,LOW);
     digitalWrite(11,LOW);
}

void loop()
{
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t buflen = sizeof(buf);

    driver.waitAvailable();
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
	int i;

	// Message with a good checksum received, dump it.
	driver.printBuffer("Got:", buf, buflen);
        Serial.println(buf[0]);
    }
}
