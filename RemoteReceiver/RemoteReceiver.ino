
// receiver.pde
//
// Simple example of how to use VirtualWire to receive messages
// Implements a simplex (one-way) receiver with an Rx-B1 module
//
// See VirtualWire.h for detailed API docs
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// $Id: receiver.pde,v 1.3 2009/03/30 00:07:24 mikem Exp $

#include <Manchester.h>
//#include <VirtualWire.h>
//#include <ServoTimer2.h> 
#include <MissileLauncher.h>
#include <Usb.h>

const int led_pin = 13;
const int transmit_pin = 6;
const int receive_pin = 5;
const int transmit_en_pin = 4;

const int laserPin = A0;
const int LASER_ON = 280;
const int LASER_OFF = 0;

const int MIN_JOYSTICK = -0;
const int MAX_JOYSTICK = 1024;
const int AVG_JOYSTICK = 512;

//const int MIN_JMAP = -75;
//const int MAX_JMAP = 75;

const int C_MOVE_TIME = 300;
const int C_STOP_TIME = 10;

//ServoTimer2 horizServo;
//const int horizServoPin = 2;

//ServoTimer2 vertServo;
//const int vertServoPin = 3;
 
int pos = 0;    // variable to store the servo position 

MissileLauncher ml;

#define BUFFER_SIZE 7
uint8_t buf[BUFFER_SIZE];

void setup()
{
    Serial.begin(9600);	// Debugging only
    Serial.println("setup");
    
    if (ml.Init() == -1) {
        Serial.println("OSCOKIRQ failed to assert");
    }

    // Initialise the IO and ISR
    man.setupReceive(receive_pin, MAN_1200);
    man.beginReceiveArray(BUFFER_SIZE, buf);
    
    // Setup laser`
    pinMode(laserPin, OUTPUT);

    Serial.println("Done Setup");
    
    //Setup Servo
    //horizServo.attach(horizServoPin);
    //horizServo.write(0);
    
    //vertServo.attach(vertServoPin);
    //vertServo.write(0);
 
}

void loop()
{
    if (man.receiveComplete()) // Non-blocking
    {
        Serial.println("REC");
	      int i;
        
        ml.Task();
	
        int num = doActionFromBuf(buf);
        
        //reset receiver
        man.beginReceiveArray(BUFFER_SIZE, buf);
    }
    else {
      ml.stop();
      delay(C_STOP_TIME);
    }


//    if( ml.getUsbTaskState() == USB_STATE_RUNNING ) {
//        Serial.println("Device Running");
//
//        ml.moveUp();
//        delay(1000);
//        ml.stop();
//        delay(200);
//
//        ml.moveDown();
//        delay(1000);
//        ml.stop();
//        delay(200);
//
//        ml.moveLeft();
//        delay(3000);
//        ml.stop();
//        delay(200);
//
//        ml.moveRight();
//        delay(3000);
//        ml.stop();
//        delay(200);
//
//        ml.fire();
//        delay(3000);
//        ml.stop();
//        delay(200);
//    }//if( Usb.getUsbTaskState() == USB_STATE_RUNNING..
}

int doActionFromBuf(uint8_t *buf){

  if( buf[0] == 'J' && buf[1] == 'B' ){
    Serial.println("JB");
    int action = atoi((char *)buf + 3);
    Serial.println(action);
    toggleLaser(action);
    return action;
  }
  
  else if( buf[0] == 'T' && buf[1] == 'R' ){
    Serial.println("MB");
    int action = atoi((char *)buf + 3);
    Serial.println(action);
    fireMissile(action);
    return action;
  }
  
  else if( buf[0] == 'H' ){
      Serial.println("H");
      int moveVal = atoi((char *)buf + 2);
      Serial.println(moveVal);
      sweepHorizontal( moveVal );
  }
  
  else if( buf[0] == 'V' ){
      Serial.println("V");
      int moveVal = atoi((char *)buf + 2);
      Serial.println(moveVal);
      sweepVertical( moveVal );
  } 
  
  return -1;
  
}

void toggleLaser(int action){
Serial.println(action);
   if( action == 1 )
     analogWrite(laserPin,LASER_ON);
   else
     analogWrite(laserPin,LASER_OFF);
}

void fireMissile(int action){

   if( action == 1 ){
        ml.fire();
        delay(6000);
        ml.stop();
        delay(200);
   }
}

void sweepHorizontal( int moveVal ){

  /*
  int mapped = map(moveVal, MIN_JOYSTICK, MAX_JOYSTICK, MIN_JMAP, MAX_JMAP);
  int val = horizServo.read() + mapped;
  val = (val <= MIN_PULSE_WIDTH ? MIN_PULSE_WIDTH : val);
  val = (val >= MAX_PULSE_WIDTH ? MAX_PULSE_WIDTH : val);
  horizServo.write( val );
  */
  
  if( ml.getUsbTaskState() == USB_STATE_RUNNING ) {
    
    if( moveVal > AVG_JOYSTICK && abs(moveVal - AVG_JOYSTICK) > 15 ){
      ml.moveLeft();
      delay(C_MOVE_TIME);
      //ml.stop();
      //delay(C_STOP_TIME);
    } else if( moveVal < AVG_JOYSTICK && abs(moveVal - AVG_JOYSTICK) > 15 ) {
      ml.moveRight();
      delay(C_MOVE_TIME);
      //ml.stop();
      //delay(C_STOP_TIME);
    } else {
      ml.stop();
      delay(C_STOP_TIME);
    }
  }//if( Usb.getUsbTaskState() == USB_STATE_RUNNING..
}

void sweepVertical( int moveVal ){

  /*
  int mapped = map(moveVal, MIN_JOYSTICK, MAX_JOYSTICK, MIN_JMAP, MAX_JMAP);
  int val = vertServo.read() + mapped;
  val = (val <= MIN_PULSE_WIDTH ? MIN_PULSE_WIDTH : val);
  val = (val >= MAX_PULSE_WIDTH ? MAX_PULSE_WIDTH : val);
  vertServo.write( val );
  */
  

  if( ml.getUsbTaskState() == USB_STATE_RUNNING ) {
    
    if( moveVal > AVG_JOYSTICK && abs(moveVal - AVG_JOYSTICK) > 15 ){
      ml.moveUp();
      delay(C_MOVE_TIME);
      //ml.stop();
      //delay(C_STOP_TIME);
    } else if( moveVal < AVG_JOYSTICK && abs(moveVal - AVG_JOYSTICK) > 15 ){
      ml.moveDown();
      delay(C_MOVE_TIME);
      //ml.stop();
      //delay(C_STOP_TIME);
    } else {
      ml.stop();
      delay(C_STOP_TIME);
    }
  }//if( Usb.getUsbTaskState() == USB_STATE_RUNNING..
}


