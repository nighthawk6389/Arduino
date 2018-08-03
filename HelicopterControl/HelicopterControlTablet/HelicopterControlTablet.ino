
#include <Servo.h>
#include <LiquidCrystal.h>
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

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
int servoLow2 = 75;
int servoHigh2 = 160;
int servoStart2 = (servoHigh2+servoLow2)/2;

int servoDelay = 50;

int throttleLow = 5;
int throttleHigh = 100;
int rudderLow = -40;
int rudderHigh = 40;

int rudderStatic = 0;
int throttleStatic = 0;

boolean LOOP = true;
int LOOP_SPREAD = 5;


byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x6F, 0x1F }; //90 A2 DA 00 6F 1F
byte ip[] = { 192,168,1, 199 };
int localPort = 9999;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
EthernetUDP Udp;


void setup()
{
  Serial.begin(9600);
  
  Ethernet.begin(mac,ip);
  Udp.begin(localPort);
  
    // set up the LCD's number of columns and rows: 
  //lcd.begin(16, 2);

  servo1.attach(servoPin1);
  servo1.write(servoStart1); 

  servo2.attach(servoPin2);
  servo2.write(servoStart2);  
  
}


void loop()
{
  
    // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if(packetSize)
  {
    //Serial.print("Received packet of size ");
    //Serial.println(packetSize);
    //Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i =0; i < 4; i++)
    {
     // Serial.print(remote[i], DEC);
      if (i < 3)
      {
       // Serial.print(".");
      }
    }
    //Serial.print(", port ");
    //Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);
    
    int p_i = 0;
    int i = 0;
    char left[5] = {'\0','\0','\0','\0','\0'};
    char right[5] = {'\0','\0','\0','\0','\0'};
    char *ref = left;
    while(packetBuffer[p_i] != '\0'){
      if( packetBuffer[p_i] == ' '){
        ref = right;
        i = 0;
        p_i++;
        continue;
      }
      ref[i] = packetBuffer[p_i];
      i++;
      p_i++;
    }
    //Serial.println("Contents:");
    throttleStatic = atoi(left);
    rudderStatic = atoi(right);
    //Serial.println(packetBuffer);
    ///Serial.print("L:");
    //Serial.println(leftNum);
    //Serial.print("R:");
    //Serial.println(rightNum);
    //Serial.println(UDP_TX_PACKET_MAX_SIZE);
  }
  delay(10);


  if( LOOP ) {
    
    // Rudder 
    int constrainedRudder = constrain(rudderStatic, rudderLow, rudderHigh);
    int mappedRudder = map(constrainedRudder, rudderLow, rudderHigh, servoHigh2, servoLow2); 
    servo2.write(mappedRudder);
    //Serial.println(mappedRudder);
    
    // Throttle
    int constrainedThrottle = constrain(throttleStatic, throttleLow, throttleHigh);
    int mappedThrottle = map(constrainedThrottle, throttleLow, throttleHigh, servoLow1, servoHigh1); 
   
    int subtractThrottle = map(mappedRudder, rudderHigh, rudderLow, 0, 10); 
    
    if( mappedThrottle <= servoLow1 )
      servo1.write(servoOff1);
    else
      servo1.write(mappedThrottle - subtractThrottle);
    //Serial.println(mappedThrottle);
    
      Serial.print(mappedRudder);
      Serial.print(" ");
      Serial.println(mappedThrottle);

/*
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
    */
  }
  
  if( LOOP ){
    lcd.setCursor(13,1);
    lcd.print("ON ");
  } else {
    lcd.setCursor(0,0);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print("             ");
    lcd.setCursor(13,1);
    lcd.print("OFF");
  }

  String action = "";
  while(Serial.available() > 0){
    int inByte = Serial.read();
    delay(100);
    action += (char) inByte;
  }

  if(action.equals("fire")){
    Serial.println("Fire");
    LOOP = true;
    //servo1.write(servoStart); 
    //servo2.write(servoStart); 
  }
  else if(action.equals("stop")){
    Serial.println("Stop");
    LOOP = false;
    servo1.write(servoOff1);
    servo2.write(servoStart2);  
  }
  
  delay(50);

}



