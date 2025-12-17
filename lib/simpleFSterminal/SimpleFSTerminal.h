#ifndef SIMPLE_FS_TERMINAL_H
#define SIMPLE_FS_TERMINAL_H

#include <Arduino.h>
#include <LittleFS.h>
#include <BackgroundAudio.h>
#include <I2S.h>

class SimpleFSTerminal {
public:
    static void begin();
    static void processSerial();
    
private:
    static String currentDir;
    static void executeCommand(const String& cmd, const String& args);
    static void ls(const String& path);
    static void cd(const String& path);
    static void mkdir(const String& path);
    static void rm(const String& path);
    static void play(const String& path);
    static void cat(const String& path);
    static void writeFile(const String& path, const String& content);
    static void showHelp();
    static void showInfo();
    static String getFullPath(const String& path);
};

#endif