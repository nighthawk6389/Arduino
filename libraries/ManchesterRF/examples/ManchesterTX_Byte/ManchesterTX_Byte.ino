/*

  Manchester Receiver example
  
  In this example transmitter will transmit two 8 bit numbers per transmittion
    
*/


#include "ManchesterRF.h" //https://github.com/cano64/ManchesterRF



#define TX_PIN 3 //any pin can transmit
#define LED_PIN 13

ManchesterRF rf(MAN_4800); //link speed, try also MAN_300, MAN_600, MAN_1200, MAN_2400, MAN_4800, MAN_9600, MAN_19200, MAN_38400

uint8_t data = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, HIGH);
  rf.TXInit(TX_PIN);
}

void loop() {

  rf.transmitByte(++data, data*data);
  digitalWrite(LED_PIN, digitalRead(LED_PIN)); //blink the LED on receive
  delay(100);
}


