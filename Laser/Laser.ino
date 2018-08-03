
const int laserPin = A5;
const int maxValue = 280;
const int sensorPin = A0;

void setup() {
 Serial.begin(9600); 
 pinMode(laserPin, OUTPUT);
 analogWrite(laserPin,maxValue);
}

void loop() {
  // read the value from the sensor:
  int sensorValue = analogRead(sensorPin);
  Serial.print(sensorValue);  
  Serial.print(" ");
  Serial.print(sensorValue * 5.0 / 1024);
  Serial.println();  
  delay(100);                          
}
