
#include <LiquidCrystal.h>
#include <Manchester.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(5, 6, 7, 2, 3, 4);


/* Joystick */
const int joystickHorizPin = A2;
const int joystickVertPin = A3;
const int joystickButtonPin = A1;
int prevHoriz = 0;
int prevVert = 0;
int prevJoystickButtonState = 0;

/* Missile Switch */
const int missileSwitchPin = 8;
int prevMissileButtonState = 0;

/* Transmitter */
const int transmit_pin = 9;

const int UNUSED_PIN_3 = 3;
const int UNUSED_PIN_5 = 5;
const int ONCE = 1;
const int TWICE = 2;

const int THRESHHOLD = 10;

void setup()
{
  Serial.begin(9600);

  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);

  /* Setup joystick */
  pinMode(joystickHorizPin, INPUT);
  pinMode(joystickVertPin, INPUT); 
  digitalWrite(joystickButtonPin, LOW);
  pinMode(joystickButtonPin, INPUT);
  
  /* Setup Missile Switch */
  pinMode(missileSwitchPin, INPUT);
  
  /* Setup transmitter */
  man.workAround1MhzTinyCore(); //add this in order for transmitter to work with 1Mhz Attiny85/84
  man.setupTransmit(transmit_pin, MAN_1200);
  
  lcd.setCursor(0,0);
  lcd.print("Hello World");
  
  delay(2000);
  lcd.clear();

}


void loop()
{
    int joystickButtonState = digitalRead(joystickButtonPin);
    int horiz = analogRead(joystickHorizPin);
    int vert = analogRead(joystickVertPin);
    int missileState = digitalRead(missileSwitchPin);
    
    char joystickButtonMessage [5] = {'0','\0'};
    char horizMessage[7] = {'\0','\0','\0','\0','\0'};
    char vertMessage[7] = {'\0','\0','\0','\0','\0'};
    char missileMessage[5] = {'0','\0'};
    
    sprintf(joystickButtonMessage,"JB:%d",joystickButtonState );
    sprintf(horizMessage,"H:%d",horiz);
    sprintf(vertMessage,"V:%d", vert);
    sprintf(missileMessage,"TR:%d", missileState);
    
    lcd.setCursor(0,0);
    lcd.print(joystickButtonMessage);
    lcd.print(" ");
    lcd.print(missileMessage);

    lcd.setCursor(0,1);
    lcd.print(horizMessage);
    lcd.print(" ");
    lcd.print(vertMessage);
    lcd.print("   ");
    
    
    if( prevJoystickButtonState != joystickButtonState){ 
      sendMessage((uint8_t *)joystickButtonMessage, 5, TWICE);
    }
    if( abs(horiz - 520) > THRESHHOLD){
      sendMessage((uint8_t *)horizMessage, 7, ONCE);
    }
    if( abs(vert - 500) > THRESHHOLD ){
     sendMessage((uint8_t *)vertMessage, 7, ONCE);
    }
    if( prevMissileButtonState != missileState ){
     sendMessage((uint8_t *)missileMessage, 5, TWICE);
    }
    
    prevJoystickButtonState = joystickButtonState;
    prevHoriz = horiz;
    prevVert = vert;
    prevMissileButtonState = missileState;  

}


void sendMessage( uint8_t *message, uint8_t len, int send_count ){

    int count = 0;
  
    while( count++ < send_count ){
      man.transmitArray(len, message);
    }
  
}
