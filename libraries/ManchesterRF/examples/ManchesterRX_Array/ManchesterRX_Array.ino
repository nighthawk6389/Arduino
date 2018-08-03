/*

  Manchester Receiver example
  
  In this example receiver will receive an array of 8 bit numbers per transmittion
    
*/


#include "ManchesterRF.h" //https://github.com/cano64/ManchesterRF



#define RX_PIN 4 //any pin can receive
#define LED_PIN 13

ManchesterRF rf(MAN_4800); //link speed, try also MAN_300, MAN_600, MAN_1200, MAN_2400, MAN_4800, MAN_9600, MAN_19200, MAN_38400

uint8_t size;
uint8_t *data;

void setup() {
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(LED_PIN, HIGH);
  rf.RXInit(RX_PIN);
}

void loop() {

  if (rf.available()) { //something is in RX buffer
    if (rf.receiveArray(size, &data)) {
      //process the data
      for (int i = 0; i < size; i++) {
         data[i]; //do something with the data 
      }
      digitalWrite(LED_PIN, digitalRead(LED_PIN)); //blink the LED on receive
    }
  }  

}


