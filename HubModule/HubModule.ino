//CPE 123 Group 2 -- Home-Pass Hub Code //COM 8
#include <AsyncDelay.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rdm6300.h>
#include <FastLED.h>

WiFiServer server(80);
IPAddress IP(192,168,4,15);
IPAddress mask = (255, 255, 255, 0);

#define RDM6300_RX_PIN 13 // can be only 13 - on esp8266 force hardware uart!
#define NUM_LEDS 40
#define DATA_PIN 1
#define CLOCK_PIN 2
#define LED_LIGHT_PIN 16

CRGB ring[NUM_LEDS];
Rdm6300 rdm6300;

enum AnimationType{bootAnimation, defaultAnimation, disconnectAnimation, 
                   card1AcquisitionAnimation, card2AcquisitionAnimation,
                   card1DockedAnimation, card2DockedAnimation, LOOP_CALL};
bool animationManager(AnimationType selectedAnimation); 

void setup() {
  Serial1.begin(115200);
  while(!Serial1){}

  hardwareSetup();
  wifiSetup();

  while(!animationManager(bootAnimation)){}
}

void hardwareSetup(){
    rdm6300.begin(RDM6300_RX_PIN);
    pinMode(LED_LIGHT_PIN, OUTPUT);
    
    LEDS.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR>(ring, NUM_LEDS);
    LEDS.setBrightness(15);
    for (int i = 0; i < NUM_LEDS; i++){
      ring[i] = CHSV(0, 0, 0);
    }
}

void wifiSetup(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP("HomePassHub", "cpe123g2Gang");
  WiFi.softAPConfig(IP, IP, mask);
  server.begin();
  Serial1.println();
  Serial1.println("Server started.");
  Serial1.print("IP: ");     Serial1.println(WiFi.softAPIP());
  Serial1.print("MAC:");     Serial1.println(WiFi.softAPmacAddress());
}

void loop() {
  maintainCurrentTag();
  clientListener();

  animationManager(LOOP_CALL);
}

//FUNCTIONS THAT VERIFY CONNECTION AND NOTIFY DISCONNECT
bool getConnectionStatus(){
  if(connectionTracker(false)){
    return true;
  } else {
    return false;
  }
}

bool connectionTracker(bool updatingTag){
  static AsyncDelay timeout;
  if(updatingTag){
    timeout.start(10000, AsyncDelay::MILLIS);
  }
  if(timeout.isExpired()){
    return false;
  } else {
    return true;
  }
}

//COMMUNICATION FUNCTIONS
void clientListener(){
  WiFiClient client = server.available();
  if(!client){return;}
  
  String request = client.readStringUntil('\r');

  String answer = responseChooser(request);
  Serial1.println("Responding with: " + answer);
  client.print(answer + "\r");
  client.flush();
}

String responseChooser(String message){
  String messageCode = getMessageCode(message);
  static int currentIndex = 0;
  Serial1.println("Request Code: " + messageCode);

  if(messageCode == "90"){
    connectionTracker(true);
    return intToString(accessStoredRFID(false, 0, 0))+intToString(accessStoredRFID(false, 0, 1));
//    if(currentIndex == 0){
//      currentIndex = 1;
//      return intToString(accessStoredRFID(false, 0, 0));
//
//    } else {
//      currentIndex = 0;
//      return intToString(accessStoredRFID(false, 0, 1));
//    }
  } else {
    connectionTracker(true);
    return String(!rdm6300.is_tag_near());
  }
}

//RFID READING FUNCTIONS
bool maintainCurrentTag(){
  static int tagInQuestion = 0;
  
  if(updateRFID()){
    int currentReadTag = rdm6300.get_tag_id();
    switch(tagInQuestion){
      case 0:
        if(accessStoredRFID(false, 0, tagInQuestion) != currentReadTag && accessStoredRFID(false, 0, 1) != currentReadTag){
          while(!animationManager(card1AcquisitionAnimation));
          accessStoredRFID(true, currentReadTag, tagInQuestion); 
          tagInQuestion = 1;
        }
      break;
      
      case 1:
        if(accessStoredRFID(false, 0, tagInQuestion) != currentReadTag && accessStoredRFID(false, 0, 0) != currentReadTag){
          while(!animationManager(card2AcquisitionAnimation));
          accessStoredRFID(true, currentReadTag, tagInQuestion); 
          tagInQuestion = 0;
        }
      break;
    }
    if(accessStoredRFID(false, 0, 0)==currentReadTag){
      animationManager(card1DockedAnimation);
      Serial1.println("hiyo");
    }
    if(accessStoredRFID(false, 0, 1)==currentReadTag){
      animationManager(card2DockedAnimation);
    }
  }

  if(!rdm6300.is_tag_near()){
    digitalWrite(LED_LIGHT_PIN, HIGH);
    animationManager(defaultAnimation);
  } else {
    digitalWrite(LED_LIGHT_PIN, LOW);
  }
  
  return updateRFID();
}

int accessStoredRFID(bool writing, int writeValue, int index){ //Use this function to overwrite the stored RFID tag or to access stored tag
  static int storedTag[] = {0,0};
  if(writing){
    storedTag[index] = writeValue;
    Serial1.println("New RFID tag detected, replacing stored tag " + String(index) + " to: " + String(writeValue));
  }

  return storedTag[index];
}

bool updateRFID(){ //This must be called repeatedly for the RFID detection to operate nominally
  return rdm6300.update();
}

//LED RING ANIMATION FUNCTIONS                  
bool animationManager(AnimationType selectedAnimation){
  static AnimationType previouslyCalledAnimation;
  static int animationReset = 1;
  static AnimationType preferredDockingType;

  if(selectedAnimation!=LOOP_CALL && animationReset == 1){
      previouslyCalledAnimation = selectedAnimation;
  }
  
  if(!getConnectionStatus() && selectedAnimation!=bootAnimation){
    previouslyCalledAnimation = disconnectAnimation;
  }

  if(selectedAnimation == card1DockedAnimation || selectedAnimation == card2DockedAnimation){
    preferredDockingType == selectedAnimation;
  }

  if(rdm6300.is_tag_near()){
    previouslyCalledAnimation == preferredDockingType;
  }

  
  switch(previouslyCalledAnimation){
  //CALLABLE ANIMATIONS
    case bootAnimation:
    return purpleBootingAnimation();

    case defaultAnimation:
      defaultPurpleAnimation();
    return true;

    case disconnectAnimation:
      redSubtlePulsing();
    return true;

    case card1AcquisitionAnimation:
    return orangeAcquisitionAnimation();
 
    case card2AcquisitionAnimation:
    return greenAcquisitionAnimation();
     
    case card1DockedAnimation:
      defaultOrangeAnimation();
    return true;

    case card2DockedAnimation:
      defaultGreenAnimation();
    return true;
  }
  
}

//LIGHTING FUNCTIONS
void animationLoop(){
  static AsyncDelay changeDelay;
  static uint8_t hue = 0;
  static int i = 0;
  if(rdm6300.is_tag_near()){
   for (int y = 0; y < NUM_LEDS; y++){
    ring[y] = CHSV(86, 240, 255);
    FastLED.show();
   }
  } else {
    for (int i = 0; i < NUM_LEDS; i++){
    ring[i] = CHSV(195, 181, 255);
    }
    if(changeDelay.isExpired()){
      if(i==0){
        ring[0] = CHSV(195, 0, 255);
        ring[39] = CHSV(195, 50, 255);
        ring[38] = CHSV(195, 100, 255);
        ring[37] = CHSV(195, 181, 255);
      }
      if(i==1){
        ring[1] = CHSV(195, 0, 255);
        ring[0] = CHSV(195, 50, 255);
        ring[39] = CHSV(195, 100, 255);
        ring[38] = CHSV(195, 181, 255);
     }
      if(i==2){
        ring[2] = CHSV(195, 0, 255);
        ring[1] = CHSV(195, 50, 255);
        ring[0] = CHSV(195, 100, 255);
        ring[39] = CHSV(195, 181, 255);
      }
      if(i>2){
        ring[i] = CHSV(195, 0, 255);
       ring[i-1] = CHSV(195, 50, 255);
       ring[i-2] = CHSV(195, 100, 255);
       ring[i-3] = CHSV(195, 181, 255);
     }
     FastLED.show();
     i++;
     changeDelay.start(80, AsyncDelay::MILLIS);
    }
    if(i==40){
    i = 0;
    }
  }
}

bool purpleBootingAnimation(){
  static AsyncDelay sequenceTimer;
  static uint8_t hue = 50;
  enum{STARTING, STARTING2, STARTING3, STARTING4, STARTING5};
  static int state = STARTING;
  static int ledTracker = 0; //Which LED is currently being changed.
  
  switch(state){
    case STARTING:
      if(sequenceTimer.isExpired() && ledTracker<40){
        ring[ledTracker] = CRGB(50, 10, 200);
        FastLED.show();

        sequenceTimer.start(20, AsyncDelay::MILLIS);
        ledTracker++;
      }
       
       if(ledTracker==39){
        ledTracker = 0;
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING2;
        return false;
       }             
    return false;
 
    case STARTING2:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(200, 100, 200);        
      }
      FastLED.show();
            
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(150, AsyncDelay::MILLIS);
        state = STARTING3;        
        return false;    
      }   
    return false;
    
    case STARTING3:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(50, 10, 200);
      }
      FastLED.show();
      
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING4;        
        return false;    
      }            
    return false;
    
    case STARTING4:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(200, 100, 200);       
      }
      FastLED.show();       
      
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING5;        
        return false;    
      } 
    return false;
    
    case STARTING5:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(50, 10, 200);       
      }
      FastLED.show();          
    
      if(sequenceTimer.isExpired()){
        state = STARTING;        
        return true;    
      }      
    return false;
  }
}

bool orangeAcquisitionAnimation(){
  static AsyncDelay sequenceTimer;
  static uint8_t hue = 50;
  enum{STARTING, STARTING2, STARTING3, STARTING4, STARTING5};
  static int state = STARTING;
  static int ledTracker = 0; //Which LED is currently being changed.
  
  switch(state){
    case STARTING:
      if(sequenceTimer.isExpired() && ledTracker<40){
        ring[ledTracker] = CRGB(250, 180, 30);
        FastLED.show();

        sequenceTimer.start(20, AsyncDelay::MILLIS);
        ledTracker++;
      }
       
       if(ledTracker==39){
        ledTracker = 0;
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING2;
        return false;
       }             
    return false;
 
    case STARTING2:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(200, 100, 200);        
      }
      FastLED.show();
            
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(150, AsyncDelay::MILLIS);
        state = STARTING3;        
        return false;    
      }   
    return false;
    
    case STARTING3:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(250, 180, 30);
      }
      FastLED.show();
      
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING4;        
        return false;    
      }            
    return false;
    
    case STARTING4:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(200, 100, 200);       
      }
      FastLED.show();       
      
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING5;        
        return false;    
      } 
    return false;
    
    case STARTING5:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(250, 180, 30);       
      }
      FastLED.show();          
    
      if(sequenceTimer.isExpired()){
        state = STARTING;        
        return true;    
      }      
    return false;
  }
}

bool greenAcquisitionAnimation(){
  static AsyncDelay sequenceTimer;
  static uint8_t hue = 50;
  enum{STARTING, STARTING2, STARTING3, STARTING4, STARTING5};
  static int state = STARTING;
  static int ledTracker = 0; //Which LED is currently being changed.
  
  switch(state){
    case STARTING:
      if(sequenceTimer.isExpired()){
        ring[ledTracker] = CRGB(0, 153, 51);
        FastLED.show();

        sequenceTimer.start(20, AsyncDelay::MILLIS);
        ledTracker++;
      }
       
       if(ledTracker==40){
        ledTracker = 0;
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING2;
        return false;
       }             
    return false;
 
    case STARTING2:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(200, 100, 200);        
      }
      FastLED.show();
            
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(150, AsyncDelay::MILLIS);
        state = STARTING3;        
        return false;    
      }   
    return false;
    
    case STARTING3:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(0, 153, 51);
      }
      FastLED.show();
      
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING4;        
        return false;    
      }            
    return false;
    
    case STARTING4:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(200, 100, 200);       
      }
      FastLED.show();       
      
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(200, AsyncDelay::MILLIS);
        state = STARTING5;        
        return false;    
      } 
    return false;
    
    case STARTING5:
      for(int i=0; i<40; i++){
        ring[i] = CRGB(0, 153, 51);       
      }
      FastLED.show();          
    
      if(sequenceTimer.isExpired()){
        state = STARTING;        
        return true;    
      }      
    return false;
  }
}

void defaultPurpleAnimation(){
  static uint8_t hue = 50;
  enum{ON, ON2};
  static int state = ON;
  static AsyncDelay sequenceTimer;
  static int ledTracker = 0;
  switch(state){
    case ON:
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(80, AsyncDelay::MILLIS);
        ring[ledTracker] = CRGB(hue++, 10, 200);
        ledTracker++;
        FastLED.show();
        if(ledTracker == 40){
          ledTracker = 0;
        }
        if(hue>90){
          state = ON2;
          break;
        }       
      }
    break;
    
    case ON2:
      if(sequenceTimer.isExpired()){
        ring[ledTracker] = CRGB(hue--, 5, 180);
        ledTracker++;
        FastLED.show();
        if(ledTracker == 40){
          ledTracker = 0;
        }        
        if(hue<50){
          state = ON;
          break;
        }
      }
    break;
  }
}

void defaultOrangeAnimation(){
  static uint8_t hue = 200;
  enum{ON, ON2};
  static int state = ON;
  static AsyncDelay sequenceTimer;
  static int ledTracker = 0;
  switch(state){
    case ON:
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(80, AsyncDelay::MILLIS);
        ring[ledTracker] = CRGB(hue++, 180, 30);
        ledTracker++;
        FastLED.show();
        if(ledTracker == 40){
          ledTracker = 0;
        }
        if(hue>250){
          state = ON2;
          break;
        }       
      }
    break;
    
    case ON2:
      if(sequenceTimer.isExpired()){
        ring[ledTracker] = CRGB(hue--, 180, 30);
        ledTracker++;
        FastLED.show();
        if(ledTracker == 40){
          ledTracker = 0;
        }        
        if(hue<200){
          state = ON;
          break;
        }
      }
    break;
  }
}
void defaultGreenAnimation(){
  static uint8_t hue = 0;
  enum{ON, ON2};
  static int state = ON;
  static AsyncDelay sequenceTimer;
  static int ledTracker = 0;
  switch(state){
    case ON:
      if(sequenceTimer.isExpired()){
        sequenceTimer.start(80, AsyncDelay::MILLIS);
        ring[ledTracker] = CRGB(hue++, 153, 51);
        ledTracker++;
        FastLED.show();
        if(ledTracker == 40){
          ledTracker = 0;
        }
        if(hue>50){
          state = ON2;
          break;
        }       
      }
    break;
    
    case ON2:
      if(sequenceTimer.isExpired()){
        ring[ledTracker] = CRGB(hue--, 153, 351);
        ledTracker++;
        FastLED.show();
        if(ledTracker == 40){
          ledTracker = 0;
        }        
        if(hue<10){
          state = ON;
          break;
        }
      }
    break;
  }
}

void redSubtlePulsing(){
  for(int i = 0; i < NUM_LEDS; i++){
    ring[i] = CHSV(0, 255, timedFader());
    FastLED.show();
  }    
}

//LIGHTING MATH FUNCTIONS
int timedFader() {
  static AsyncDelay fadeDelay;
  static int currentFade = 0;
  enum{UP_FADING, DOWN_FADING};
  static int state = UP_FADING;

  if(currentFade <= 75){
    state = UP_FADING;
  }
  if(currentFade > 165){
    state = DOWN_FADING;
  }
  if(fadeDelay.isExpired()){
    switch(state){
      case UP_FADING:
        currentFade = currentFade + 2;
     break;

     case DOWN_FADING:
        currentFade = currentFade - 2; 
      break;
    }
    fadeDelay.start(10, AsyncDelay::MILLIS);
  }
  return currentFade;
}

//DATA MANIPULATION FUNCTIONS
String getMessageCode(String receivedMessage){
  char messageCode[2];
  int i;
  for(i=0; i<2; i++){
    messageCode[i] = receivedMessage[i];
  }
  
  messageCode[2] = '\0';

  return String(messageCode);
}

String intToString(int input){
  char int2char[20];
  itoa(input, int2char, 10);
  return String(int2char);
}

int stringToInt(String input){
    String s = input; 
    int output = atoi(input.c_str());
    return output;
}
