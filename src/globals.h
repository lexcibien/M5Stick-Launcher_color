// Globals.h
#ifndef GLOBALS_H
#define GLOBALS_H
#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>

extern int prog_handler;    // 0 - Flash, 1 - SPIFFS, 2 - Download

extern bool sdcardMounted;

extern std::vector<std::pair<std::string, std::function<void()>>> options;

extern  String ssid;

extern  String pwd;

extern int currentIndex;

extern JsonDocument doc;

extern String fileToCopy;

extern bool onlyBins;

extern int rotation;

extern bool returnToMenu;

extern uint8_t buff[4096];

extern const int bufSize;

//Used to handle the update in webUI
extern bool update;

//Don't let open OTA after use WebUI due t oRAM handling
extern bool stopOta;

//size o the file in the webInterface
extern size_t file_size;

#endif