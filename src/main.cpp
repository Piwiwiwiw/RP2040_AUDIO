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
    Serial2.setTimeout(200);
    Serial2.setRX(21);
    Serial2.setTX(20);
    Serial2.begin(9600);
    while (!SD.begin(SD_CLK_PIN, SD_CMD_PIN, SD_DATA_PIN)) {
        delay(100);
        Serial.println("文件系统启动失败");
    }
    Serial.println("文件系统就绪!");
    static AudioPlayer AudioOutput(pio1, 5, 3, 4);
    pAudio = &AudioOutput;
    Serial.println("开机");
    File audioFile;
    audioFile = SD.open("POWERON.mp3", "r");
    pAudio->play(&audioFile, mp3);
    audioFile.close();
    Serial2.write(txBuffer, 3);
    while(Serial2.available())
        Serial2.read();
}



void loop() {
    if(Serial2.available()){
        Serial2.readBytes(rxBuffer, 3);
        if(rxBuffer[0] == 0xFF || rxBuffer[2] == 0xFE){
            SD.begin(SD_CLK_PIN, SD_CMD_PIN, SD_DATA_PIN);
            Serial2.write(txBuffer, 3);
            File audioFile;
            switch (rxBuffer[1])
            {
                case 114:
                    audioFile = SD.open("114514.mp3", "r");
                    if(!audioFile)
                        Serial.printf("无指定文件\n");
                    else{
                        pAudio->play(&audioFile, mp3);
                        audioFile.close();
                    }
                    break;
                case 228:
                    audioFile = SD.open("1919810.mp3", "r");
                    if(!audioFile)
                        Serial.printf("无指定文件\n");
                    else{
                        pAudio->play(&audioFile, mp3);
                        audioFile.close();
                    }
                    break;
                default:
                    break;
            }
        }
        else if(rxBuffer[0] = 0XF1){
            pAudio->setGain((rxBuffer[1] << 8) & rxBuffer[2]);
        }
        Serial2.write(txBuffer, 3);
        while(Serial2.available())
            Serial2.read();
        memset(rxBuffer, 0, 3);
    }
}

