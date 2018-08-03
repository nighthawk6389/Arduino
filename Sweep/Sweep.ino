// Sweep
// by BARRAGAN <http://barraganstudio.com> 
// This example code is in the public domain.


#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
Servo myservo2; 
 
int pos = 0;    // variable to store the servo position 
 
void setup() 
{ 
  myservo.attach(7);  // attaches the servo on pin 9 to the servo object 
  myservo2.attach(3);
} 
 
 
void loop() 
{ 
  
   myservo2.write(100);
  
  for(pos = 2; pos < 175; pos += 2)  // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myservo2.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(50);       // waits 15ms for the servo to reach the position 
  }
  for(pos = 175; pos > 2; pos -= 2)  // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myservo2.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(50);       // waits 15ms for the servo to reach the position 
  }
//  for(pos = 0; pos < 180; pos += 1)  // goes from 0 degrees to 180 degrees 
//  {                                  // in steps of 1 degree 
//    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
//    delay(5);       // waits 15ms for the servo to reach the position 
//    myservo2.write(pos
//  }
//  for(pos = 180; pos>=1; pos-=1)     // goes from 180 degrees to 0 degrees 
//  {                                
//    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
//    delay(5);                       // waits 15ms for the servo to reach the position 
//  } 
} 
