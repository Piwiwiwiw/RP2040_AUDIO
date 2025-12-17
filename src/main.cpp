#include <Arduino.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include "LittleFS.h"
#include "audio_i2s_PIO.h"


#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

// --- 配置参数 ---
#define PCM_FRAME_COUNT 4096   // 每个缓存区的帧数
#define CHANNELS 2            // 假设双声道
#define BUFFER_SIZE (PCM_FRAME_COUNT * CHANNELS)
#define chanDMA_play 0

// --- 缓存区定义 ---
int16_t buffer_A[BUFFER_SIZE];
int16_t buffer_B[BUFFER_SIZE];
int16_t* pNextBuffer = buffer_A;  // 指向正在等待填充的缓存
int16_t* pPlayBuffer = buffer_B;  // 指向正在“播放”的缓存
// --- 状态控制 ---
drflac* pFlac = NULL;
File audioFile;
bool bufferReady = false; // 标记当前等待填充的缓存是否已填满
uint32_t framesRead;



// --- LittleFS 回调函数 (Buffer 1 的搬运逻辑) ---
size_t lfs_read_proc(void* pUserData, void* pBufferOut, size_t bytesToRead) {
    return ((File*)pUserData)->read((uint8_t*)pBufferOut, bytesToRead);
}



drflac_bool32 lfs_seek_proc(void* pUserData, int offset, drflac_seek_origin origin) {
    File* f = (File*)pUserData;
    SeekMode mode = (origin == DRFLAC_SEEK_CUR) ? SeekCur : (origin == DRFLAC_SEEK_END ? SeekEnd : SeekSet);
    return f->seek(offset, mode);
}




void dmaPlay_irq_handler(){
      dma_channel_acknowledge_irq0(chanDMA_play);
      if(!framesRead){
          irq_set_enabled(DMA_IRQ_0, false);
          Serial.println("--- 播放结束 ---"); 
          return;
      } 
      int16_t* temp = pPlayBuffer;
      pPlayBuffer = pNextBuffer;
      pNextBuffer = temp;

      dma_channel_set_read_addr(chanDMA_play, pPlayBuffer, false);
      dma_channel_set_trans_count(chanDMA_play, framesRead, true);

      framesRead = drflac_read_pcm_frames_s16(pFlac, PCM_FRAME_COUNT, pNextBuffer);
};



void setupDMAChain(uint channel) {

    dma_channel_config cfgDMA_play = dma_channel_get_default_config(chanDMA_play);
    channel_config_set_transfer_data_size(&cfgDMA_play, channel == 1 ? DMA_SIZE_16 : DMA_SIZE_32);
    channel_config_set_dreq(&cfgDMA_play, pio_get_dreq(pio0, 0, true));
    channel_config_set_read_increment(&cfgDMA_play, true);
    channel_config_set_write_increment(&cfgDMA_play, false);
    channel_config_set_irq_quiet(&cfgDMA_play, false); 
    dma_channel_configure(chanDMA_play, &cfgDMA_play, &pio0_hw->txf[0], NULL, 256, false);
    
    // 设置中断处理器
    dma_channel_set_irq0_enabled(chanDMA_play, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dmaPlay_irq_handler);
    
}



void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    //启动文件系统
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Error");
        return;
    }


    audio_i2s_init(18, 19, 44100);
    //打开音频文件
    audioFile = LittleFS.open("/sb4.flac", "r");
    pFlac = drflac_open(lfs_read_proc, lfs_seek_proc, NULL, &audioFile, NULL);
    Serial.printf("正在播放: %uHz  %uBits  %u声道\n", pFlac->sampleRate, pFlac->bitsPerSample, pFlac->channels);
    if (pFlac) {
        //设置
        audio_i2s_set_simplerate(pFlac->sampleRate);
        setupDMAChain(pFlac->channels);
        //预解码
        framesRead = drflac_read_pcm_frames_s16(pFlac, PCM_FRAME_COUNT, pNextBuffer);
        //交换缓存指针
        int16_t* temp = pPlayBuffer;
        pPlayBuffer = pNextBuffer;
        pNextBuffer = temp;

        dma_channel_set_read_addr(chanDMA_play, pPlayBuffer, false);
        dma_channel_set_trans_count(chanDMA_play, framesRead, true);
        irq_set_enabled(DMA_IRQ_0, true);


    }
}




void loop() {
}

