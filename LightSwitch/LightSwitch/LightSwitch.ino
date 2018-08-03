#include <Manchester.h>

//
int rx_pin    = 8;
int servoPin  = 10;   // pin attached to servo

//
#define LIGHT_ON 900 //On Servo position
#define LIGHT_OFF 1500 //Off Servo Position

//
#define OFF_STATE 0
#define ON_STATE 1
int state = OFF_STATE;

void setup() {                
  
  //Servo
  pinMode(servoPin, OUTPUT); 

  //RF
  man.setupReceive(rx_pin, MAN_1200);
  man.beginReceive();  
}

void loop() {

  if (man.receiveComplete()) {

    //Message
    uint16_t m = man.getMessage();

    //Servo action
    moveServo(state == ON_STATE ? LIGHT_OFF : LIGHT_ON);
    state = (state == ON_STATE ? OFF_STATE : ON_STATE);

    //Sleep to avoid accidental presses
    delay(1000);
    man.beginReceive(); //start listening for next message right after you retrieve the message
  }
  
}

void moveServo(int pos){

  for( int x =0; x < 15; ++x ){
    digitalWrite(servoPin, HIGH);   // start PPM pulse
    delayMicroseconds(pos);         // wait pulse diration
    digitalWrite(servoPin, LOW);    // complete the pulse
      // Note: delayMicroseconds() is limited to 16383µs
      // see http://arduino.cc/en/Reference/DelayMicroseconds
      // Hence delayMicroseconds(20000L-pos); has been replaced by:
    delayMicroseconds(5000-pos);   // adjust to 5ms period
    delay(15);                      // pad out 5ms to 20ms PPM period
  }

}

