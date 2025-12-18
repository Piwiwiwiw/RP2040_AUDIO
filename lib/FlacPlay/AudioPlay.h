#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H


#include "dr_flac.h"
#include <Arduino.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include "LittleFS.h"
#include "Audio_i2s_PIO.h"  // 包含 AudioI2S 类的头文件





#define PCM_FRAME_COUNT 4096   // 每个缓存区的帧数
#define CHANNELS 2             // 假设双声道
#define BUFFER_SIZE (PCM_FRAME_COUNT * CHANNELS)




class AudioPlayer {
public:
    // 构造函数：接受 PIO、状态机编号、DMA 通道、I2S 数据引脚和时钟引脚
    AudioPlayer(PIO pio, uint sm, uint dmaChannel, uint i2sDataPin, uint i2sClockPin);
    ~AudioPlayer();
    // 播放音频文件
    void init() {audioI2S.initialize();}
    int play(const String& path);

    
private:
    // DMA中断处理程序
    static void dmaPlayIrqHandler();
    void setupDMAChain(uint bits);
    // 文件读写回调
    static size_t lfsReadProc(void* pUserData, void* pBufferOut, size_t bytesToRead);
    static drflac_bool32 lfsSeekProc(void* pUserData, int offset, drflac_seek_origin origin);
    // 成员变量
    uint dmaChannel; 
    drflac_int32 buffer_A[BUFFER_SIZE];
    drflac_int32 buffer_B[BUFFER_SIZE];
    drflac_int32* pNextBuffer;
    drflac_int32* pPlayBuffer;
    drflac* pFlac;
    File audioFile;
    bool bufferReady;
    bool isPlaying;
    uint32_t framesRead;
    static AudioPlayer* instance;  // 用于中断处理程序访问
    AudioI2S audioI2S;  // 内含 AudioI2S 类的实例
};
#endif // AUDIOPLAY_H
