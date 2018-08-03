
// include the library code:
#ifndef _DISPLAY_TEMP
#define _DISPLAY_TEMP
#include <LiquidCrystal.h>
#endif

int pin = 2;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);


int count = 0;

void setup() {
  
  Serial.begin(9600);

  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  
  pinMode(pin, INPUT);
}

void loop() {
  
  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  
  int buttonState = digitalRead(pin);
  Serial.println(buttonState);
    
  lcd.clear();
  
  if( buttonState == 1 )
    lcd.print("Alarm is on");
  else {
    lcd.print("ROBBBER");
    delay(700);
    lcd.clear();
  }
  
  delay(500);
  

}

