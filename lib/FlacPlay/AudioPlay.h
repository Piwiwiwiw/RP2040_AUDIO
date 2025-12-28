#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H


#include "dr_flac.h"
#include "dr_mp3.h"


#include <Arduino.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include "LittleFS.h"
#include "Audio_i2s_PIO.h"  // 包含 AudioI2S 类的头文件



#define SD_CLK_PIN 8
#define SD_CMD_PIN 9
#define SD_DATA_PIN 10

#define PCM_FRAME_COUNT 8196   // 每个缓存区的帧数
#define CHANNELS 2             // 假设双声道
#define BUFFER_SIZE (PCM_FRAME_COUNT * CHANNELS)


enum format{
    wav = 0,
    flac = 1,
    mp3 = 2,
};



union AudioBuffer {
    drflac_int32    bufferFlac[BUFFER_SIZE]; // 32-bit buffer
    drmp3_int16     bufferMP3[BUFFER_SIZE * 2]; // 16-bit buffer (twice the number of 16-bit elements)
};



union BufferPtr {
    drflac_int32* flacBuffer;
    drmp3_int16* mp3Buff;
};


class AudioPlayer {
public:
    // 构造函数：接受 PIO、状态机编号、DMA 通道、I2S 数据引脚和时钟引脚
    AudioPlayer(PIO pio, uint dmaChannel, uint i2sDataPin, uint i2sClockPin);
    ~AudioPlayer();
    // 播放音频文件
    int play(File* audio, format mode);
    bool getStatus() {return isPlaying;}
    void setGain(int _gain) {gain = _gain;}
private:
    // DMA中断处理程序
    static void dmaPlayIrqHandler();
    void setupDMAChain(uint bits);
    // 文件读写回调
    static size_t lfsReadProc(void* pUserData, void* pBufferOut, size_t bytesToRead);
    
    static drflac_bool32 lfsSeekProc(void* pUserData, int offset, drflac_seek_origin origin);
    static drmp3_bool32 lfsSeekProc(void* pUserData, int offset, drmp3_seek_origin origin);
    // 成员变量
    uint dmaChannel;
    format form; 
    AudioBuffer buffer_A;
    AudioBuffer buffer_B;
    BufferPtr pNextBuffer;
    BufferPtr pPlayBuffer;
    drflac* pFlac;
    drmp3   MP3;
    drmp3*  pMP3 = &MP3;
    File* audioFile;
    uint16_t gain = 16384;
    bool bufferReady;
    bool isPlaying;
    uint32_t framesRead;
    static AudioPlayer* instance;  // 用于中断处理程序访问
    AudioI2S audioI2S;  // 内含 AudioI2S 类的实例
};
#endif // AUDIOPLAY_H
