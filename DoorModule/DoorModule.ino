//CPE 123 Group 2 -- Home-Pass Door Station Code //COM 11
#include <ESP8266WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AsyncDelay.h>

byte connection_ledPin = 16;
byte sync_ledPin = 15;
byte button_pin = 5;

char ssid[] = "HomePassHub";         
char pass[] = "cpe123g2Gang";        

IPAddress server(192,168,4,15);    
WiFiClient client;

void setup() {
  Serial.begin(9600);
  while(!Serial){}
  pinMode(connection_ledPin, OUTPUT);
  pinMode(sync_ledPin, OUTPUT);
  pinMode(button_pin, INPUT_PULLUP);
  Serial.println("DoorModule successfully initiated.");
  wifiSetup();
}

void wifiSetup(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);           // connects to the WiFi AP
  Serial.println();
  Serial.println("Attempting to connect to HubModule");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Connection Successful...");
  Serial.print("LocalIP:"); Serial.println(WiFi.localIP());
  Serial.println("MAC:" + WiFi.macAddress());
  Serial.print("Gateway:"); Serial.println(WiFi.gatewayIP());
  Serial.print("AP MAC:"); Serial.println(WiFi.BSSIDstr());
}

void loop() {
  requestCommsPerm();
  delay(9000);
}

bool requestCommsPerm(){
  client.connect(server, 80);
  digitalWrite(connection_ledPin, LOW);
  Serial.println("Sending permission request to hub module.");

  client.print("x1");
  String answer = client.readStringUntil('\r');
  client.flush();
  client.stop();

  Serial.println("Received: " + answer);

  if(answer == "i1"){
    Serial.println("Communication accepted -- Identity request received from hub module");
    return true;
  } else {
    Serial.println("Communication rejected from hub module");
    return false;
  }
  return false;
}

String contactHubWithInt(int intToSend){
  client.connect(server, 80);
  digitalWrite(connection_ledPin, LOW);
  Serial.println("********************************");
  Serial.print("Data sent to the AP: ");

  Serial.println(intToString(intToSend));
  client.print(intToString(intToSend) + "\r");
  
  String answer = client.readStringUntil('\r');
  Serial.println("From the AP: " + answer);
  client.flush();
  digitalWrite(connection_ledPin, HIGH);
  client.stop();

  return answer.c_str();
}

String intToString(int input){
  char int2char[20];
  itoa(input, int2char, 10);

  return String(int2char);
}

void blinkLedFromButton(){
  enum{LED_ON, LED_OFF};
  static int state = LED_OFF;

  switch(state){
    case LED_OFF:
      if(digitalRead(button_pin)==HIGH){
        state = LED_ON;
        contactHubWithInt(1);
        digitalWrite(sync_ledPin, HIGH);
      }
    break;

    case LED_ON:
      if(digitalRead(button_pin)==LOW){
        state = LED_OFF;
        contactHubWithInt(0);
        digitalWrite(sync_ledPin, LOW);
      }
    break;
  }
}
