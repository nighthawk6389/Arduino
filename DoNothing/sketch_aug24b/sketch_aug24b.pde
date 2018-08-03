/*
Practice
*/
void setup() {     
  Serial.begin(9600);
  Serial.println("Begin Serial");  
}

void loop() {
  String neo = "";
  while(Serial.available() > 0){
    int inByte = Serial.read();
    delay(100);
    neo += (char) inByte;
   }
   if(neo.equals("My name is neo")){
     Serial.print("Follow the white bunny");
     delay(1000);
     Serial.print("MIWWWWWWK");
   }
   else if( neo.equals("Hello") )
     Serial.print("Hello Tushy!!!!");
   else
     Serial.println("NOOOO");
   Serial.println();
   while(Serial.available() <=0)
     continue;
}
