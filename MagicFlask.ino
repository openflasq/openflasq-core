// #include <WiFi.h>
// #include <ESPmDNS.h>
// #include <WiFiUdp.h>
// #include <ArduinoOTA.h>
//#include "wifiSettings.h"


/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;


#include <NeoPatterns.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "lcdContent.h"

volatile int interruptCounter;
int totalInterruptCounter;
hw_timer_t * timer = NULL;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64


const uint16_t PAUSE_TIME = 1000; // in milliseconds
const uint8_t SPEED_DEADBAND = 5; // in analog units

int touchSeconds;
int lastTouch;
int flaskTouch;
int cupTouch;
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#include <avr/pgmspace.h>
#endif

void allPatterns(NeoPatterns * aLedsPtr);

NeoPatterns flaskRing = NeoPatterns(16, 13, NEO_GRB + NEO_KHZ800, &allPatterns);
// onComplete callback functions

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// Alternative to delay function
const unsigned long eventInterval = 1000;
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;


int sPomodoroTimer=0;
int sPomodoroTimerEnd=1500;//25 mins testing;
bool activePomodoroTimer = false;
    
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  delay(1000); // give me time to bring up serial monitor
  Wire.begin(5, 4);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);
    }
    delay(2000); // Pause for 2 seconds
   
    // Clear the buffer.
    display.clearDisplay();
    // Draw bitmap on the screen
    display.drawBitmap(0, 0, image_data_flasqLogo, 128, 64, 1);
    display.display();
 
  
  flaskRing.begin(); // INITIALIZE NeoPixel pixels object (REQUIRED)
  flaskRing.PixelFlags |= PIXEL_FLAG_GEOMETRY_CIRCLE;
  // flaskRing.ColorWipe(COLOR32(0, 0, 02), 50, 0, REVERSE);

  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // while (WiFi.waitForConnectResult() != WL_CONNECTED) {
  //   Serial.println("Connection Failed! Rebooting...");
  //   delay(5000);
  //   ESP.restart();
  // }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("magicflask");


  // ArduinoOTA
  //   .onStart([]() {
  //     String type;
  //     if (ArduinoOTA.getCommand() == U_FLASH)
  //       type = "sketch";
  //     else // U_SPIFFS
  //       type = "filesystem";

  //     // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  //     Serial.println("Start updating " + type);
  //   })
  //   .onEnd([]() {
  //     Serial.println("\nEnd");
  //   })
  //   .onProgress([](unsigned int progress, unsigned int total) {
  //     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  //   })
  //   .onError([](ota_error_t error) {
  //     Serial.printf("Error[%u]: ", error);
  //     if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
  //     else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
  //     else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
  //     else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  //     else if (error == OTA_END_ERROR) Serial.println("End Failed");
  //   });

  // ArduinoOTA.begin();

  // Serial.println("Ready");
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());
}

void updateSensors() {
  flaskTouch = touchRead(T3);
  //cupTouch = touchRead(T2);
}

void updateMode(){
  if(activePomodoroTimer == false){
    flaskRing.ColorWipe(COLOR32_GREEN, 48, 0, DIRECTION_DOWN);
    flaskRing.setPixelColor(0, 0,0,0);
    Serial.println("Pomodoro start"); 
    lastTouch=0;
    activePomodoroTimer=true;

  }else if (activePomodoroTimer  == true ) {
    
    flaskRing.ColorWipe(COLOR32_RED, 48, 0, DIRECTION_UP);
    flaskRing.setPixelColor(0, 0,0,0);
    Serial.println("Pomodoro stop"); 
    lastTouch=0;
    sPomodoroTimer=0;
    activePomodoroTimer=false;

  }else
  {
    /* code */
  }
  
  
}



void checkTouch() {
  updateSensors();

  if (flaskTouch<10 && flaskTouch>0 && lastTouch>2 )    {
    updateMode();
  // REDUNDANT AF
  }//else if (flaskTouch<10 && flaskTouch>0 && lastTouch>2 && activePomodoroTimer==true) {
  //   updateMode();
  // }

  else{
    if((unsigned long)(currentMillis-previousMillis) >= eventInterval){
      if (flaskTouch<10 ){
        touchSeconds++;
        Serial.println("Touch 1s");
        if(touchSeconds >= 3){
          Serial.println("success long press!");
        }
      } else if (touchSeconds >=10){
        //touchSeconds =0;
      }
      }
    
  }

  
    

  }


void loop() {  
  checkTouch();
  currentMillis = millis();

  if ((unsigned long)(currentMillis-previousMillis) >= eventInterval){
    if(activePomodoroTimer == true){
      if(sPomodoroTimer < sPomodoroTimerEnd){
        sPomodoroTimer++;
        Serial.println("Pomodoro running for " + String(sPomodoroTimer) "seconds / " + String(sPomodoroTimerEnd) + "seconds"  );
      }else if(sPomodoroTimer >= sPomodoroTimerEnd) {
        updateMode();
      }
    }
    lastTouch++;
    Serial.println("Last touch " + String(lastTouch) + "ago");
    previousMillis = currentMillis;
  }

  flaskRing.update();    
  // ArduinoOTA.handle();
}

/*
 * Handler for all pattern
 */
void allPatterns(NeoPatterns * aLedsPtr) {

    uint8_t tDuration = 100;
    uint8_t tColor = random(255);

    if (activePomodoroTimer == true  ){
      if (sPomodoroTimer <  (0.8 * sPomodoroTimerEnd) ){
        aLedsPtr->RainbowCycle(tDuration / 4, (tDuration & DIRECTION_DOWN));
      }else if(sPomodoroTimer >  (0.8 * sPomodoroTimerEnd)) {
        aLedsPtr->Heartbeat(NeoPatterns::Wheel(tColor), tDuration / 2, 2);
      }
    }else if (activePomodoroTimer == false){
      initMultipleFallingStars(aLedsPtr, COLOR32_WHITE_HALF, 10, tDuration / 2, 3, &allPatterns);
    }
    
}
