#include <Arduino.h>
#include <AudioPlay.h>
#include "dr_flac.h"
AudioPlayer AudioOutput(pio0, 0, 0, 14, 15);
void setup() {
    Serial.begin(9600);
    AudioOutput.init();
    //启动文件系统
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Error");
        return;
    }  
}




void loop() {
    Serial.println(AudioOutput.play("JI.mp3", mp3));

    delay(1000);
    Serial.printf("播放中");
}

