// ask_transmitter.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to transmit messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) transmitter with an TX-C1 module

#include <RH_ASK.h>
//#include <SPI.h> // Not actually used but needed to compile

RH_ASK driver(1000,4,3);

void setup()
{
    //pinMode(2,OUTPUT);
//    Serial.begin(9600);	  // Debugging only
    if (!driver.init()){
        pinMode(1,OUTPUT);
        digitalWrite(1,HIGH);
//         Serial.println("init failed");
    }
}

void loop()
{
    const char *msg = "h";

    boolean ret = driver.send((uint8_t *)msg, strlen(msg));
    //if( !ret ){
      //digitalWrite(2,HIGH);
    //}
    driver.waitPacketSent();
}
