/*
  DigitalReadSerial
 Reads a digital input on pin 2, prints the result to the serial monitor 
 
 This example code is in the public domain.
 */

// digital pin 2 has a pushbutton attached to it. Give it a name:
int pinOut = 7;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  pinMode(pinOut, OUTPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input pin:
  digitalWrite(pinOut, HIGH);
  delay(10);        // delay in between reads for stability
}



