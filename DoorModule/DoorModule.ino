//CPE 123 Group 2 -- Home-Pass Door Station Code //COM 11
#include <ESP8266WiFi.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AsyncDelay.h>
#include <iostream> 
#include <string>
#include <rdm6300.h>
#include <Servo.h>

#define RDM6300_RX_PIN 13 // can be only 13 - on esp8266 force hardware uart!
#define SERVO_PIN 5

Rdm6300 rdm6300;
Servo doorServo;

char ssid[] = "HomePassHub";         
char pass[] = "cpe123g2Gang";        

IPAddress server(192,168,4,15);    
WiFiClient client;

void setup() {
  Serial1.begin(115200);
  while(!Serial1){}
  Serial1.println("DoorModule successfully initiated.");
  hardwareSetup();
  wifiSetup();
}

void hardwareSetup(){
  rdm6300.begin(RDM6300_RX_PIN);
  doorServo.attach(SERVO_PIN);
  doorSwitch(1); //Defaults the door to closed
}

void wifiSetup(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);           // connects to the WiFi AP
  Serial1.println();
  Serial1.println("Attempting to connect to HubModule");
  while (WiFi.status() != WL_CONNECTED) {
    Serial1.print(".");
    delay(500);
  }
  Serial1.println();
  Serial1.println("Connection Successful...");
  Serial1.print("LocalIP:"); Serial1.println(WiFi.localIP());
  Serial1.println("MAC:" + WiFi.macAddress());
  Serial1.print("Gateway:"); Serial1.println(WiFi.gatewayIP());
  Serial1.print("AP MAC:"); Serial1.println(WiFi.BSSIDstr());
}

void loop() {
  verifyRFID();
  verifyState();
}

void verifyRFID(){
  static AsyncDelay readDelay;
  String rfidTag;
  
  char tag1[8]; 
  char tag2[8];
  bool updateTag = rdm6300.update();

  if(updateTag){
    rfidTag = intToString(rdm6300.get_tag_id());
    char receivedTag[14];

    strcpy(receivedTag, requestCurrentRFID().c_str());

    strncpy(tag1, receivedTag, 7);
    strcpy(tag2, &receivedTag[7]);

    tag1[7] = '\0';
    
    Serial1.println(String(tag1) + " " + String(tag2));
  }
  
  if(readDelay.isExpired()){
    if(updateTag){
      if(String(tag1)==rfidTag|| String(tag2)==rfidTag){
        toggleDoor();
        readDelay.start(5000, AsyncDelay::MILLIS);
      }
    }
  }
}

void verifyState(){
  static AsyncDelay intervalTimer;
  String statesReceived;
  static bool hasHubReset = false;

  if(intervalTimer.isExpired()){
    statesReceived = requestStateData();
    intervalTimer.start(9000, AsyncDelay::MILLIS);
  }
  if(statesReceived == "1"){
    Serial1.println("Actual state received: " + statesReceived);
    hasHubReset = true;
  }
  if(statesReceived == "0"){
    if(hasHubReset){
      Serial1.println("Actual state received: " + statesReceived);
      doorSwitch(0);
      hasHubReset = false;
    }
  }
}

//DOOR SERVO FUNCTIONS
void toggleDoor(){
  Serial1.println(getDoorPosition());
  if(getDoorPosition()==180){
    doorSwitch(0);
  } else {
    doorSwitch(1);
  }
}

int getDoorPosition(){
  return doorServo.read();
}

void openDoor(){
  doorServo.write(90);
}

void closeDoor(){
  doorServo.write(180);
}

void doorSwitch(int openOrClose){
  enum{OPEN_DOOR, CLOSE_DOOR};
  switch(openOrClose){
    case OPEN_DOOR:
        openDoor(); //writes 90 degrees
    break;

    case CLOSE_DOOR:
        closeDoor(); //writes 180 degrees
    break;
  }
}

//COMMUNICATION TASKS
String contactHubWithInt(int intToSend){
  client.connect(server, 80);
  Serial1.println("Sending Data: " + intToString(intToSend));
  client.print(intToString(intToSend) + "\r");
  String answer = client.readStringUntil('\r');
  client.flush();
  client.stop();
  return answer.c_str();
}

String requestCurrentRFID(){
  String currentTag = contactHubWithInt(90);
  Serial1.println("Tag Code Received: " + currentTag);
  return currentTag;
}

String requestStateData(){
  String stateData = contactHubWithInt(50);
  Serial1.println("State Data Received: " + stateData);
  return stateData; 
}

//DATA MANIPULATION FUNCTIONS
String intToString(int input){
  char int2char[20];
  itoa(input, int2char, 10);
  return String(int2char);
}

int stringToInt(String input){
  int x = atoi(input.c_str());
  return x;
}
