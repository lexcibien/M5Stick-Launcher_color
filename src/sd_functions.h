#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <FFat.h>
#include <Update.h>

extern SPIClass sdcardSPI;

bool eraseFAT();

bool setupSdCard();

void closeSdCard();

bool ToggleSDCard();

bool deleteFromSd(String path);

bool renameFile(String path, String filename);

bool copyFile(String path);

bool pasteFile(String path);

bool createFolder(String path);

void readFs(String folder, String result[][3]);

void sortList(String fileList[][3], int fileListCount);

void loopSD();

void performUpdate(Stream &updateSource, size_t updateSize, int command);

void updateFromSD(String path);