#include "globals.h"

#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <M5-HTTPUpdate.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPIFFS.h>
#include "esp_ota_ops.h"

#include <iostream>
#include <functional>
#include <vector>
#include <string>


// Public Globals
int prog_handler;    // 0 - Flash, 1 - SPIFFS
int currentIndex;
int rotation;
bool sdcardMounted;
bool onlyBins;
bool returnToMenu;
bool update;
bool stopOta;
//bool command;
size_t file_size;
String ssid;
String pwd;
JsonDocument doc;
std::vector<std::pair<std::string, std::function<void()>>> options;
const int bufSize = 4096;
uint8_t buff[4096] = {0};


#include "display.h"
#include "mykeyboard.h"
#include "onlineLauncher.h"
#include "sd_functions.h"
#include "webInterface.h"


//Functions in this file;

void loopSD();
void setBrightnessMenu();
void setBrightness(int bright, bool save = true);
void getBrightness(); 
bool gsetOnlyBins(bool set = false, bool value = true);
int gsetRotation(bool set = false);
void backToMenu();


/*********************************************************************
**  Function: setBrightness                             
**  save brightness value into EEPROM
**********************************************************************/
void setBrightness(int bright, bool save) {
  if(bright>100) bright=100;

  #if !defined(STICK_C_PLUS)
  int bl = MINBRIGHT + round(((255 - MINBRIGHT) * bright/100 )); 
  analogWrite(BACKLIGHT, bl);
  #else
  axp192.ScreenBreath(bright);
  #endif
  
  EEPROM.begin(EEPROMSIZE); // open eeprom
  EEPROM.write(2, bright); //set the byte
  EEPROM.commit(); // Store data to EEPROM
  EEPROM.end(); // Free EEPROM memory
}

/*********************************************************************
**  Function: getBrightness                             
**  save brightness value into EEPROM
**********************************************************************/
void getBrightness() {
  EEPROM.begin(EEPROMSIZE);
  int bright = EEPROM.read(2);
  EEPROM.end(); // Free EEPROM memory
  if(bright>100) { 
    bright = 100;
    #if !defined(STICK_C_PLUS)
    int bl = MINBRIGHT + round(((255 - MINBRIGHT) * bright/100 )); 
    analogWrite(BACKLIGHT, bl);
    #else
    axp192.ScreenBreath(bright);
    #endif

    setBrightness(100);

  }
  
  #if !defined(STICK_C_PLUS)
  int bl = MINBRIGHT + round(((255 - MINBRIGHT) * bright/100 )); 
  analogWrite(BACKLIGHT, bl);
  #else
  axp192.ScreenBreath(bright);
  #endif
}

/*********************************************************************
**  Function: gsetOnlyBins                             
**  get onlyBins from EEPROM
**********************************************************************/
bool gsetOnlyBins(bool set, bool value) {
  EEPROM.begin(EEPROMSIZE);
  int onlyBin = EEPROM.read(9);
  bool result = false;

  if(onlyBin>1) { 
    set=true;
  } 
   
  if(onlyBin==0) result = false;
  else result = true;

  if(set) {
    result=value;
    onlyBins=value;         //update the global variable
    EEPROM.write(9, result);
    EEPROM.commit();
  }
  EEPROM.end(); // Free EEPROM memory
  return result;
}
/*********************************************************************
**  Function: gsetRotation                             
**  get onlyBins from EEPROM
**********************************************************************/
int gsetRotation(bool set){
  EEPROM.begin(EEPROMSIZE);
  int getRot = EEPROM.read(0);
  int result = ROTATION;
  
  if(getRot==1 && set) result = 3;
  else if(getRot==3 && set) result = 1;
  else if(getRot<=3) result = getRot;
  else {
    set=true;
    result = ROTATION;
  } 

  if(set) {
    rotation = result;
    tft.setRotation(result);
    EEPROM.write(0, result);    // Left rotation
    EEPROM.commit();
  }
  EEPROM.end(); // Free EEPROM memory
  return result;
}
/*********************************************************************
**  Function: setBrightnessMenu                             
**  Handles Menu to set brightness
**********************************************************************/
void setBrightnessMenu() {
  options = {
    {"100%", [=]() { setBrightness(100); }},
    {"75 %", [=]() { setBrightness(75); }},
    {"50 %", [=]() { setBrightness(50); }},
    {"25 %", [=]() { setBrightness(25); }},
    {" 0 %", [=]() { setBrightness(1); }},
  };
  delay(200);
  loopOptions(options, true);
  delay(200);
}


/*********************************************************************
**  Function: setup                                    
**  Where the devices are started and variables set    
*********************************************************************/
void setup() {
  Serial.begin(115200);

  // declare variables
  size_t currentIndex=0;  
  prog_handler=0;
  sdcardMounted=false;
  String fileToCopy;

  //Define variables to identify if there is an app installed after Launcher 
  esp_app_desc_t ota_desc;
  esp_err_t err = esp_ota_get_partition_description(esp_ota_get_next_update_partition(NULL), &ota_desc);  

  // Setup GPIOs and stuff
  #if  defined(STICK_C_PLUS2)
    pinMode(UP_BTN, INPUT);
  #elif defined(STICK_C_PLUS)
    axp192.begin();

  #endif
  
 
  #ifndef CARDPUTER
  pinMode(SEL_BTN, INPUT);
  pinMode(DW_BTN, INPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4,HIGH);
  #else
  Keyboard.begin();
  pinMode(10, INPUT);
  #endif

  tft.init();
  rotation = gsetRotation();
  tft.setRotation(rotation);
  resetTftDisplay();

  #if defined(BACKLIGHT)
  pinMode(BACKLIGHT, OUTPUT);
  #endif

  EEPROM.begin(EEPROMSIZE); // open eeprom
if(EEPROM.read(0) > 3 || EEPROM.read(1) > 240 || EEPROM.read(2) > 100 || EEPROM.read(3) > 1 || EEPROM.read(4) > 19 || EEPROM.read(5) > 19) {
#if defined(CARDPUTER)
  EEPROM.write(0, 1);    // Right rotation for cardputer
#else
  EEPROM.write(0, 3);    // Left rotation
#endif
  EEPROM.write(1, 15);   // 15 second auto dim time
  EEPROM.write(2, 100);  // 100% brightness
  EEPROM.write(3, 0);    // TVBG NA Region
  EEPROM.write(4, 11);   // FGColor Green
  EEPROM.write(5, 1);    // BGcolor Black
  EEPROM.commit();       // Store data to EEPROM
}
if(EEPROM.read(0) != 1 && EEPROM.read(0) != 3)  { 
  EEPROM.write(0, 3);    // Left rotation
  EEPROM.commit();       // Store data to EEPROM
}
  EEPROM.end(); // Free EEPROM memory

  

  getBrightness();  
  onlyBins=gsetOnlyBins();
  sprite.createSprite(WIDTH-15,HEIGHT-15);
  //Start Bootscreen timer
  int i = millis();
  while(millis()<i+5000) { // increased from 2500 to 5000
    initDisplay();        //Inicia o display
  
  #if defined (CARDPUTER)
    Keyboard.update();
    if(Keyboard.isKeyPressed(KEY_ENTER))
  #else
    if(digitalRead(SEL_BTN)==LOW) 
  #endif
     {
        tft.fillScreen(TFT_BLACK);
        delay(50);
        goto Launcher;
      }

  #if defined (CARDPUTER)
    Keyboard.update();
    if (Keyboard.isPressed() && !(Keyboard.isKeyPressed(KEY_ENTER)))
  #elif defined(STICK_C_PLUS2)
    if(digitalRead(UP_BTN)==LOW || digitalRead(DW_BTN)==LOW) 
  #elif defined(STICK_C_PLUS)
    if(axp192.GetBtnPress() || digitalRead(DW_BTN)==LOW)
  #endif 
      {
        tft.fillScreen(TFT_BLACK);
        ESP.restart();
      } 
  }
  
  // If nothing is done, check if there are any app installed in the ota partition, if it does, restart device to start installed App.
  if(err == ESP_OK) { 
	  tft.fillScreen(TFT_BLACK);
	  ESP.restart(); 
  }

  // If M5 or Enter button is pressed, continue from here
  Launcher:
  delay(200);

}

/**********************************************************************
**  Function: loop                                     
**  Main loop                                          
**********************************************************************/
void loop() {
  bool redraw = true;
  int index = 0;
  int opt = 3; // there are 3 options> 1 list SD files, 2 OTA and 3 Config
  stopOta = false; // variable used in WebUI, and to prevent open OTA after webUI without restart
  tft.fillRect(0,0,WIDTH,HEIGHT,BGCOLOR);
  if(!setupSdCard()) index=1; //if SD card is not present, paint SD square grey and auto select OTA
  while(1){
    if (redraw) { 
      drawMainMenu(index); 
      redraw = false; 
      delay(200); 
    }

    if(checkPrevPress()) {
      if(index==0) index = opt - 1;
      else if(index>0) index--;
      redraw = true;
    }
    /* DW Btn to next item */
    if(checkNextPress()) { 
      index++;
      if((index+1)>opt) index = 0;
      redraw = true;
    }

    /* Select and run function */
    if(checkSelPress()) { 
      if(index == 0) {  
        if(setupSdCard()) { 
          loopSD(); 
          redraw=true;
        }
        else {
          displayRedStripe("Insert SD Card");
          delay(3000);
        }
        
      }
      if(index == 1) {  
        if (!stopOta) {
          if (WiFi.status() != WL_CONNECTED) {
            int nets;
            WiFi.mode(WIFI_MODE_STA);
            displayScanning();
            nets=WiFi.scanNetworks();
            //delay(3000);
            options = { };
            for(int i=0; i<nets; i++){
              options.push_back({WiFi.SSID(i).c_str(), [=]() { wifiConnect(WiFi.SSID(i).c_str(),int(WiFi.encryptionType(i))); }});
            }
            delay(200);
            loopOptions(options);
            if (WiFi.status() == WL_CONNECTED) {
              if(GetJsonFromM5()) loopFirmware();
            }

          } else {
            //If it is already connected, download the JSON again... it loses the information once you step out of loopFirmware(), dkw
            closeSdCard();
            if(GetJsonFromM5()) loopFirmware();
          }
          redraw=true;
        } 
        else {
          displayRedStripe("Restart to open OTA");
          delay(3000);
        } 

      }
      if(index == 2) {  
        options = {
          {"Brightness", [=]() { setBrightnessMenu(); }},
        };
        if(sdcardMounted) {
          if(onlyBins) options.push_back({"All Files",  [=]() { gsetOnlyBins(true, false); }});
          else         options.push_back({"Only Bins",  [=]() { gsetOnlyBins(true, true); }});
        }
        #ifndef CARDPUTER
        options.push_back({"Rotate 180",  [=]() { gsetRotation(true); }});
        #endif

        options.push_back({"Start WebUI",  [=]() { loopOptionsWebUi(); }});
        options.push_back({"Restart",  [=]() { ESP.restart(); }});
        
        delay(200);
        loopOptions(options);
        redraw=true;
      }
      returnToMenu = false;
      redraw = true;
    }
  }
}

