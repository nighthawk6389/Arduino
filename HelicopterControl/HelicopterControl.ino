
#include <Servo.h>
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

/* Servo Stuff */
Servo servo1;  //90 - 170
int servoPin1 = 2;
int servoOff1 = 15;
int servoLow1 = 30;
int servoHigh1 = 140;
int servoStart1 = servoOff1;

Servo servo2;
int servoPin2 = 3;
int servoLow2 = 80;
int servoHigh2 = 160;
int servoStart2 = (servoHigh2+servoLow2)/2;

int servoDelay = 50;

/* Ultrasonic stuff */
int triggerPinReal = 13;
int triggerPinGeneric = 4;
int echoPinGeneric = 5;
int sonicDelay = 50;
int sonicLow1 = 10; //cm
int sonicHigh1 = 35; //cm
int sonicLow2 = 10; //cm
int sonicHigh2 = 35; //cm

/* Button */
int buttonPin = 6;
int prevButtonState = 0;
int buttonAction = 0;

int START = true;

boolean LOOP = false;
int LOOP_SPREAD = 5;

void setup()
{
  Serial.begin(9600);

  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);

  servo1.attach(servoPin1);
  servo1.write(servoStart1); 

  servo2.attach(servoPin2);
  servo2.write(servoStart2);  
  
  pinMode(buttonPin, INPUT);

  int throttle = getGenericPingDistance(triggerPinGeneric, echoPinGeneric);
  int rudder = getRealPingDistance(triggerPinReal);
}


void loop()
{


  if( LOOP ) {
    
        // Rudder 
    int rudder = getRealPingDistance(triggerPinReal);
    int constrainedRudder = constrain(rudder, sonicLow2, sonicHigh2);
    int mappedRudder = map(constrainedRudder, sonicLow2, sonicHigh2, servoHigh2, servoLow2); 
    servo2.write(mappedRudder);
    Serial.println(mappedRudder);
    
    // Throttle
    int throttle = getGenericPingDistance(triggerPinGeneric, echoPinGeneric);
    int constrainedThrottle = constrain(throttle, sonicLow1, sonicHigh1);
    int mappedThrottle = map(constrainedThrottle, sonicLow1, sonicHigh1, servoHigh1, servoLow1); 
    
    //Additional throttle
    int mappedAddThrottle = map(mappedRudder, servoLow2, servoHigh2, 0, 00); 
    
    if( mappedThrottle <= servoLow1 )
      servo1.write(servoOff1-mappedAddThrottle);
    else
      servo1.write(mappedThrottle - mappedAddThrottle);
    //Serial.println(mappedThrottle);

    // LCD 
    int mappedThrottleLCD = map(mappedThrottle, servoLow1,servoHigh1,0,100);
    int mappedRudderLCD = map(mappedRudder,servoLow2,servoHigh2,0,100);
    lcd.setCursor(0,0);
    lcd.print("Throttle: ");
    lcd.print(mappedThrottleLCD);
    lcd.print("%    ");
    lcd.setCursor(0,1);
    lcd.print("Rudder: ");
    lcd.print(mappedRudderLCD);
    lcd.print("%   ");
    delay(10);
  }
  
  if( LOOP ){
    lcd.setCursor(13,1);
    lcd.print("ON ");
  } else if( START ){
    lcd.setCursor(0,0);
    lcd.print("No Hand Heli Fly");
    lcd.setCursor(0,1);
    lcd.print("  Push Button  ");
  } else {
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print("             ");
    lcd.setCursor(13,1);
    lcd.print("OFF");
  }
  
  buttonAction = 0;
  int buttonState = digitalRead(buttonPin);
  delay(1);
  if( buttonState == 1 && prevButtonState == 0){
    Serial.print("ACTION ");
    Serial.print(buttonState);
    buttonAction = 1;
  }
  prevButtonState = buttonState;
  //Serial.print(" ");
  //Serial.println(prevButtonState);

  String action = "";
  while(Serial.available() > 0){
    int inByte = Serial.read();
    delay(100);
    action += (char) inByte;
  }

  if(action.equals("fire") || (buttonAction && !LOOP) ){
    Serial.println("Fire");
    LOOP = true;
    START= false;
    //servo1.write(servoStart); 
    //servo2.write(servoStart); 
  }
  else if(action.equals("stop") || ( buttonAction && LOOP) ){
    Serial.println("Stop");
    LOOP = false;
    servo1.write(servoOff1);
    servo2.write(servoStart2);  
  }

}

int getGenericPingDistance(int triggerPin, int echoPin)
{

  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  long duration, inches, cm;

  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(triggerPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);

  //Serial.print("Generic ");
  //Serial.print(inches);
  //Serial.print("in, ");
  //Serial.print(cm);
  //Serial.print("cm");
  //Serial.print("     ");
  //Serial.print(duration);
  //Serial.println();

  return cm;
}


int getRealPingDistance(int triggerPin)
{

  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  long duration, inches, cm;

  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // The same pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(triggerPin, INPUT);
  duration = pulseIn(triggerPin, HIGH);

  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);

  Serial.print("Real ");
  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.print("     ");
  Serial.print(duration);
  Serial.println();

  return cm;
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


