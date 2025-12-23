#include <Arduino.h>
#include <AudioPlay.h>
#include <SimpleFSTerminal.h>
#include "AudioPlay.h"
#include "dr_flac.h"
#include "SD.h"



void setup() {
    Serial.begin(115200);
    delay(3000);
    SimpleFSTerminal::begin();
    // AudioOutput.init();
}



void loop() {
    SimpleFSTerminal::processSerial();
}

