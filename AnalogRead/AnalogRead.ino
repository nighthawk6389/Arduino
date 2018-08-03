
int sensorPin = 0;
void setup() {
 Serial.begin(9600); 
}

void loop() {
  // read the value from the sensor:
  int sensorValue = analogRead(sensorPin);  
  Serial.print(sensorValue);  
  Serial.print(" ");
  Serial.print(sensorValue * 5.0 / 1024);
  Serial.println();  
  delay(50);                          
}
