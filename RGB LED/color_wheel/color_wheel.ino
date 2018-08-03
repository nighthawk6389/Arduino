int REDPin = 9;    // RED pin of the LED to PWM pin 4
int GREENPin = 10;  // GREEN pin of the LED to PWM pin 5
int BLUEPin = 11;   // BLUE pin of the LED to PWM pin 6
int rBrightness = 0; // LED brightness
int gBrightness = 0;
int bBrightness = 0;
int INC = 5;
int OFF = 255;
int ON = 0;
int rInc = INC;  // brightness increment
int gInc = INC;  // brightness increment
int bInc = INC;  // brightness increment
bool rOn = false;
bool gOn = false;
bool bOn = false;

int delayms = 25; 
int limitInc = OFF*2/INC;
int RLIMIT = 0, GLIMIT = RLIMIT + limitInc, BLIMIT = GLIMIT + limitInc;
int counter = 0;

int choice = 3;
int lastChoice = choice;

void setup() {
  // put your setup code here, to run once:
  pinMode(REDPin, OUTPUT);
  pinMode(GREENPin, OUTPUT);
  pinMode(BLUEPin, OUTPUT);
  Serial.begin(9600);

  randomSeed(analogRead(0));

  rBrightness = OFF; //random(0,255);
  gBrightness = OFF; //random(0,255);
  bBrightness = OFF; //random(0,255);

  analogWrite(REDPin, rBrightness);
  analogWrite(GREENPin, gBrightness);
  analogWrite(BLUEPin, bBrightness);
}

void loop() {
  // put your main code here, to run repeatedly:

  if(lastChoice != choice){
      rOn = false;
      gOn = false;
      bOn = false;
  }

  if(choice == 1){
    delayms = 25;
    singleColorFade();
  } else if(choice == 2){
    delayms = 25;
    singleRandom();
  } else if(choice == 3){
    delayms = 100;
    singleHold();
  }

  lastChoice = choice;
  

  Serial.print(rBrightness);
  Serial.print(" ");
  Serial.print(gBrightness);
  Serial.print(" ");
  Serial.print(bBrightness);
  Serial.println();

  analogWrite(REDPin, rBrightness);
  analogWrite(GREENPin, gBrightness);
  analogWrite(BLUEPin, bBrightness);

  delay(delayms);  // wait for 20 milliseconds to see the dimming effect
}

void singleHold(){

    int low = 200;

    int randNum = random(0,3);

    if( randNum < 1){
      gBrightness = random(low,255);
      rBrightness = random(low,255);
      bBrightness = ON;  
    } else if( randNum < 2){
      bBrightness = random(low,255);
      rBrightness = random(low,255);  
      gBrightness = ON;
    } else {
      bBrightness = random(low,255);
      gBrightness = random(low,255);
      rBrightness = ON;
    }
    
}

void singleRandom(){

    int low = 200;
    
    if(counter > BLIMIT + GLIMIT + RLIMIT){
      counter = 0;
    }

    if( counter >= BLIMIT ){
      if( counter == BLIMIT ){
        gBrightness = random(low,255);
        rBrightness = random(low,255);
      }
      bOn = true;
      gOn = false;
      rOn = false;
    } else if( counter >= GLIMIT){
      if( counter == GLIMIT ){
        bBrightness = random(low,255);
        rBrightness = random(low,255);
      }
      gOn = true;
      bOn = false;
      rOn = false;
    } else {
      if( counter == RLIMIT ){
        bBrightness = random(low,255);
        gBrightness = random(low,255);
      }
      rOn = true;
      bOn = false;
      gOn = false;
    }

    counter++;
    
    if(rOn){
      rBrightness = incBright(rBrightness, &rInc);
    }

    if(gOn){
      gBrightness = incBright(gBrightness, &gInc);
    }

    if(bOn){
      bBrightness = incBright(bBrightness, &bInc);
    }
}

void singleColorFade(){
      
    rOn = false;
    gOn = false;
    bOn = false;

    if(counter > BLIMIT + GLIMIT + RLIMIT){
      counter = 0;
    }

    counter++;
    if( counter > BLIMIT ){
      bOn = true;
    } else if( counter > GLIMIT){
      gOn = true;
    } else {
      rOn = true;
    }
    
    if(rOn){
      rBrightness = incBright(rBrightness, &rInc);
    } else {
      rBrightness = OFF;
    }

    if(gOn){
      gBrightness = incBright(gBrightness, &gInc);
    } else {
      gBrightness = OFF;
    }

    if(bOn){
      bBrightness = incBright(bBrightness, &bInc);
    } else {
      bBrightness = OFF;
    }
  
}

int incBright(int brightness, int *increment){
    brightness = brightness + *increment;  // increment brightness for next loop iteration

    if (brightness <= 0 || brightness >= 255)    // reverse the direction of the fading
    {
      *increment = -(*increment);
    }
    brightness = constrain(brightness, 0, 255);
    return brightness;
}

