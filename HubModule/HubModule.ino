//CPE 123 Group 2 -- Home-Pass Hub Code //COM 8
#include <AsyncDelay.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

WiFiServer server(80);
IPAddress IP(192,168,4,15);
IPAddress mask = (255, 255, 255, 0);

const int ledPin = 16;
const int sync_ledPin = 15;

void setup() {
  Serial.begin(9600);
  while(!Serial){}
  WiFi.mode(WIFI_AP);
  WiFi.softAP("HomePassHub", "cpe123g2Gang");
  WiFi.softAPConfig(IP, IP, mask);
  server.begin();
  pinMode(ledPin, OUTPUT);
  pinMode(sync_ledPin, OUTPUT);
  Serial.println();
  Serial.println("Server started.");
  Serial.print("IP: ");     Serial.println(WiFi.softAPIP());
  Serial.print("MAC:");     Serial.println(WiFi.softAPmacAddress());
}

void loop() {
  clientListener();
}

void clientListener(){
  WiFiClient client = server.available();
  if(!client){return;}

  Serial.println("Request received from station.");
  digitalWrite(ledPin, LOW);
  
  String request = client.readStringUntil('\r');
  client.flush();

  if(request == "x1"){
    Serial.println("Requesting module identity...");
    client.print("i1"); //Requests identity of station...
    waitingForIdentity();
  } else {
    Serial.println("Communication error with satellite module, sending error code.");
    client.print("401\r");
    digitalWrite(ledPin, HIGH);
  }
}

void waitingForIdentity(){
  WiFiClient client = server.available();
  AsyncDelay timeoutTimer;
  timeoutTimer.start(1000, AsyncDelay::MILLIS);

  while(!client){ if(timeoutTimer.isExpired()){return;} } //Request for identity has timeout at 1 second...

  String sentIdentity = client.readStringUntil('\r');

  Serial.println("Station identified as: " + sentIdentity);
  Serial.println("Requesting communication reason.");
  
  client.print("d0"); //Request for communicaton reason...
}

void listenForStation(){
  WiFiClient client = server.available();
  if (!client) {return;}
  digitalWrite(ledPin, LOW);
  String request = client.readStringUntil('\r');
  Serial.println("********************************");
  Serial.println("From the station: " + request);
  client.flush();
  Serial.print("Byte sent to the station: ");

  int confirmation = 1;
  char snum[5];
  itoa(confirmation, snum, 10);

  Serial.println(client.print(String(snum) + "\r"));
  digitalWrite(ledPin, HIGH);

  int message = 0;
  message = atoi(request.c_str());

  if(message==1){
    digitalWrite(sync_ledPin, HIGH);
  } else {
    digitalWrite(sync_ledPin, LOW);
  }
  
}
