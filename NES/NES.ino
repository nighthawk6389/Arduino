/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */
 #define LATCH 13
 #define PULSE 11
 #define DATA 9

void setup() {                
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  Serial.begin(9600);
  pinMode(LATCH, OUTPUT); // NES_LATCH
  pinMode(PULSE, OUTPUT); // NES_PULSE
  pinMode(DATA, INPUT);  // NES_DATA
  digitalWrite(LATCH, LOW);
  digitalWrite(PULSE, LOW);
}

void loop() {
   read_controller();
   //int r = analogRead(3); 
   //Serial.println(r);
   //delay(100);
}

void read_controller() {

  digitalWrite(LATCH,HIGH);
  delayMicroseconds(12);
  
  digitalWrite(LATCH,LOW);
  delayMicroseconds(6);
  
  int a = digitalRead(DATA);
  
  digitalWrite(PULSE,HIGH);
  delayMicroseconds(6);
  digitalWrite(PULSE,LOW);
  delayMicroseconds(6);
  
  int b = digitalRead(DATA);
  
  digitalWrite(PULSE,HIGH);
  delayMicroseconds(6);
  digitalWrite(PULSE,LOW);
  delayMicroseconds(6);
  int sel = digitalRead(DATA);
  
  digitalWrite(PULSE,HIGH);
  delayMicroseconds(6);
  digitalWrite(PULSE,LOW);
  delayMicroseconds(6);
  int start = digitalRead(DATA);
  
  digitalWrite(PULSE,HIGH);
  delayMicroseconds(6);
  digitalWrite(PULSE,LOW);
  delayMicroseconds(6);
  int up = digitalRead(DATA);
  
  digitalWrite(PULSE,HIGH);
  delayMicroseconds(6);
  digitalWrite(PULSE,LOW);
  delayMicroseconds(6);
  int down = digitalRead(DATA);
  
  digitalWrite(PULSE,HIGH);
  delayMicroseconds(6);
  digitalWrite(PULSE,LOW);
  delayMicroseconds(6);
  int left = digitalRead(DATA);
  
  digitalWrite(PULSE,HIGH);
  delayMicroseconds(6);
  digitalWrite(PULSE,LOW);
  delayMicroseconds(6);
  int right = digitalRead(DATA);
  
  Serial.println(a);
  Serial.println(b);
  Serial.println(sel);
  Serial.println(start);
  Serial.println(up);
  Serial.println(down);
  Serial.println(left);
  Serial.println(right);
  Serial.println("------------------");
  
}
