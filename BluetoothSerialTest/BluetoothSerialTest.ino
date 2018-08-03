/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

char INBYTE;
int  LED = 13; // LED on pin 13

void setup() {
  Serial.begin(9600); 
  pinMode(LED, OUTPUT);
}

void loop() {
  if(Serial.available()){
    while (Serial.available()){
      INBYTE = Serial.read();        // read next available byte
      Serial.print(INBYTE);
    }
    Serial.println(); 
  }
  delay(50);
}
