#include <Arduino.h>
#include <AudioPlay.h>
#include <SD.h>

#define SD_CLK_PIN 8
#define SD_CMD_PIN 9
#define SD_DATA_PIN 10

AudioPlayer* pAudio;
uint8_t rxBuffer[3];
uint8_t txBuffer[3] = {0xFF, 114, 0XFE};
void setup() {
    Serial.begin(115200);
    Serial2.setRX(21);
    Serial2.setTX(20);
    Serial2.begin(115200);
    if (!SD.begin(SD_CLK_PIN, SD_CMD_PIN, SD_DATA_PIN)) {
        delay(100);
        Serial.println("文件系统启动失败");
    }
    Serial.println("文件系统就绪!");
    static AudioPlayer AudioOutput(pio1, 5, 3, 4);
    pAudio = &AudioOutput;
}



void loop() {
    if(Serial2.available() > 2){
        Serial2.readBytes(rxBuffer, 3);
        if(rxBuffer[0] == 0xFF || rxBuffer[2] == 0xFE){
            File audioFile;
            switch (rxBuffer[1])
            {
            case 114:
                audioFile = SD.open("114514.mp3", "r");
                if(!audioFile)
                    Serial.printf("无指定文件\n");
                else{
                    pAudio->play(&audioFile, mp3);
                }
                break;
            case 228:
                audioFile = SD.open("1919810.mp3", "r");
                if(!audioFile)
                    Serial.printf("无指定文件\n");
                else{
                    pAudio->play(&audioFile, mp3);
                }
                break;
            default:
                break;
            }
            Serial2.write(txBuffer, 3);
        }
    }
}

