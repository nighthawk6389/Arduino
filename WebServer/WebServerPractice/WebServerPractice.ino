/*
  Web  Server
 
 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield. 
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)
 
 created 18 Dec 2009
 by David A. Mellis
 modified 4 Sep 2010
 by Tom Igoe
 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h> 
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
int pos = 0;    // variable to store the servo position 

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x6F, 0x1F }; //90 A2 DA 00 6F 1F
byte ip[] = { 192,168,1, 199 };
byte gateway[] = { 192, 168, 1, 1 };			// internet access via router
byte subnet[] = { 255, 255, 255, 0 };			 //subnet mask

int waitForServo1 = 1000;

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(9999);

void setup()
{
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(9600);
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
  //SERVO
   myservo.attach(9);  // attaches the servo on pin 9 to the servo object 
   myservo.write(40);    
}

void loop()
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.print("client connected");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          sendReply(client,"DONE");
          break;
        }
         
        if (c == '\n') {
          if( isAction(currentLine) ){
            sendReply(client, getActionFromString(currentLine) );
            delay(1);
            client.stop();
            String action = doAction(currentLine);
            return;
          }
          // you're starting a new line
          currentLineIsBlank = true;
          currentLine = "";
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
          currentLine += c;
        }
      }
      
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("connection closed");
  }
}

void sendReply(EthernetClient client, String action){
  // send a standard http response header
  Serial.println("sendReply");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  
  client.println("Action performed: " + action);
}

boolean isAction(String currentLine){
  String action = getActionFromString(currentLine);
  if(action.equals(""))
    return false;
   return true;
} 

String doAction(String currentLine){
  String action = getActionFromString(currentLine);
  if(action.equals("fire"))
    fire();
  if(action.equals("left"))
    ;//left();
  if(action.equals("right"))
    ;//right();
  if(action.equals("reset"))
    ;//reset();
    
   return action;
}

String getActionFromString(String currentLine){
  
  if(currentLine.startsWith("GET /Elkobi/?action=fire"))
    return "fire";
  if(currentLine.startsWith("GET /Elkobi/?action=left"))
    return "left";
  if(currentLine.startsWith("GET /Elkobi/?action=right"))
    return "right";
   if(currentLine.startsWith("GET /Elkobi/?action=reset"))
    return "reset";
    
    return "";
}

void fire(){
  Serial.print("Firing...");
  myservo.write(10);              // tell servo to go to position in variable 'pos' 
  delay(300);                       // waits 15ms for the servo to reach the position 
  myservo.write(120);
  
  return;
}
