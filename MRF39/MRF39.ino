/*

*/


// inslude the SPI library:
#include <SPI.h>

const int REGOPMODE_REG=0x01;
const int REGDATAMODULE_REG=0x02;
const int REGDBITRATEMSB_REG=0x03;
const int REGDBITRATELSB_REG=0x04;

// set pin 10 as the slave select for the digital pot:
const int slaveSelectPin = 10;
//
SPISettings settings(4000000, MSBFIRST, SPI_MODE0); 

void setup() {
  Serial.begin(9600);
  // set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // initialize SPI:
  SPI.begin();
}

void loop() {
  byte regop = spiRead(REGOPMODE_REG);
  Serial.println(regop);
  byte regdata = spiRead(REGDATAMODULE_REG);
  Serial.println(regdata);
  byte msb = spiRead(REGDBITRATEMSB_REG);
  Serial.println(msb);
  byte lsb = spiRead(REGDBITRATELSB_REG);
  Serial.println(lsb);
  Serial.println("---------------------");
  delay(1000);
}

byte spiRead(byte address) {
  SPI.beginTransaction(settings);
  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin, LOW);
  //  send in the address and value via SPI:
  SPI.transfer(address);
  byte readAddr = SPI.transfer(address);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPin, HIGH);
  SPI.endTransaction();
  return readAddr;
}
