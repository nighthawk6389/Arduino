
#include <Servo.h>

#define LOOP_SPREAD 5

 
/* Servo Stuff */
Servo myservo;  // create servo object to control a servo
                // a maximum of eight servo objects can be created 
int pos = 0;    // variable to store the servo position
int servoPin = 9;
int servoStart = 90;
int servoSpread = 45;
int servoDelay = 15;

/* Ultrasonic stuff */ 
int triggerPin = 7;
int echoPin = 8;
int sonicDelay = 50;


int LOOP = 0;
 
void setup()
{
  Serial.begin(9600);
  myservo.attach(servoPin);
  myservo.write(servoStart);  // attaches the servo on pin 9 to the servo object
}
 
 
void loop()
{
  String action = "";
  while(Serial.available() > 0){
    int inByte = Serial.read();
    delay(100);
    action += (char) inByte;
   }
   
   if(action.equals("fire")){
     LOOP = true;
     myservo.write(servoStart); 
   }
   
   if(action.equals("stop")){
     LOOP = false;
     myservo.write(servoStart); 
   }
   
   
   if( LOOP ) {
   
     for( int i = servoStart - servoSpread; i < servoStart + servoSpread; i=i+LOOP_SPREAD) {
         myservo.write(i);  
         delay(servoDelay);     // tell servo to go to position in variable 'pos'    // waits 15ms for the servo to reach the position
         int left = getDistance();
         delay(sonicDelay);
     }
     
     for( int i = servoStart + servoSpread; i > servoStart - servoSpread; i=i-LOOP_SPREAD) {
         myservo.write(i);  
         delay(servoDelay);     // tell servo to go to position in variable 'pos'    // waits 15ms for the servo to reach the position
         int right = getDistance();
         delay(sonicDelay);
     }
    
   }
  
  
}

int getDistance()
{

  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  long duration, inches, cm;
  
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);
 
  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.print("     ");
  Serial.print(duration);
  Serial.println();
  
  return inches;
}

long microsecondsToInches(long microseconds)
{
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

