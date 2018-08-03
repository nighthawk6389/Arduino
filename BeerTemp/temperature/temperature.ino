#include <DallasTemperature.h>
#include <OneWire.h>
#include <LiquidCrystal.h>


// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 6
#define HIGH_ALARM 68
#define LOW_ALARM 58

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS); 

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress thermometer;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

byte degree_symbol[8] = {
  0x6,
  0x9,
  0x9,
  0x6,
  0x0,
  0x0,
  0x0,
  0x0
};

boolean highAlarmCalled = false;
boolean lowAlarmCalled = false;

// function that will be called when an alarm condition exists during DallasTemperatures::processAlarms();
void alarmHandler(uint8_t* deviceAddress)
{
  Serial.println("Alarm Handler Start"); 
  printAlarmInfo(deviceAddress);
  printTemp(deviceAddress);
  highAlarmCalled = true;
  lowAlarmCalled = true;
  Serial.println();
  Serial.println("Alarm Handler Finish");
}

void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("Address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
  Serial.print(" ");
}

void printTemp(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC != DEVICE_DISCONNECTED)
  {
    Serial.print("Current Temp F: ");
    Serial.print(DallasTemperature::toFahrenheit(tempC));
  }
  else Serial.print("DEVICE DISCONNECTED");
  Serial.print(" ");
  
}

float getTempInF(DeviceAddress deviceAddress){
  float tempC = getTempInC(deviceAddress);
  return DallasTemperature::toFahrenheit(tempC);
}
float getTempInC(DeviceAddress deviceAddress){
  float tempC = sensors.getTempC(deviceAddress);
  return tempC;
}

void printAlarmInfo(DeviceAddress deviceAddress)
{
  char temp;
  printAddress(deviceAddress);
  temp = sensors.getHighAlarmTemp(deviceAddress);
  Serial.print("High Alarm: ");
  Serial.print(DallasTemperature::toFahrenheit(temp), DEC);
  Serial.print("F");
  Serial.print(" Low Alarm: ");
  temp = sensors.getLowAlarmTemp(deviceAddress);
  Serial.print(DallasTemperature::toFahrenheit(temp), DEC);
  Serial.print("F");
  Serial.print(" ");
}

boolean isAlarmCallable(){

  if( highAlarmCalled || lowAlarmCalled ){
    return false;
  }
  return true;
}

boolean resetAlarmsCalled(){
  highAlarmCalled = false;
  lowAlarmCalled = false;
}

void lcd_temp_setup() {
  
    // create a new character
  lcd.createChar(0, degree_symbol);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  /*
  // Print a message to the LCD.
  lcd.print("Temp: 10");
  lcd.write((uint8_t)0);
  lcd.write(" F");
  */
}

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");
  
    // Start up the library
  sensors.begin();
  
  // locate devices on the bus
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // search for devices on the bus and assign based on an index
  if (!sensors.getAddress(thermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  
  Serial.print("Device thermometer ");
  printAlarmInfo(thermometer);
  Serial.println();
  
  sensors.setHighAlarmTemp(thermometer, DallasTemperature::toCelsius(HIGH_ALARM));
  sensors.setLowAlarmTemp(thermometer, DallasTemperature::toCelsius(LOW_ALARM));

  Serial.print("New thermometer ");
  printAlarmInfo(thermometer);
  Serial.println();
  
    // attach alarm handler
  sensors.setAlarmHandler(&alarmHandler);
  
  lcd_temp_setup();
}

void loop(void)
{ 
  // call sensors.requestTemperatures() to issue a global temperature 
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
   // just print out the current temperature
   printTemp(thermometer);

  // call alarm handlerfunction defined by sensors.setAlarmHandler
  // for each device reporting an alarm
  if( isAlarmCallable() ){
    sensors.processAlarms();
  }

  if (!sensors.hasAlarm())
  {
    resetAlarmsCalled();
  }
  
  
  // print to lcd screen
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(getTempInF(thermometer));
  lcd.write((uint8_t)0);
  lcd.write(" F");
  
  delay(1000);
}


  

