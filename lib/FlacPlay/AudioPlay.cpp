#define DR_FLAC_IMPLEMENTATION
#include "AudioPlay.h"


AudioPlayer* AudioPlayer::instance = nullptr;

// 构造函数
AudioPlayer::AudioPlayer(PIO pio, uint sm, uint dmaChannel, uint i2sDataPin, uint i2sClockPin)
    : dmaChannel(dmaChannel),
      pFlac(nullptr), bufferReady(false), 
      framesRead(0), 
      isPlaying(false),
      audioI2S(pio, sm, i2sDataPin, i2sClockPin){

      instance = this;
      pNextBuffer = buffer_A;
      pPlayBuffer = buffer_B;
}


// 析构函数
AudioPlayer::~AudioPlayer() {
    instance = nullptr;
}


// 文件读回调
size_t AudioPlayer::lfsReadProc(void* pUserData, void* pBufferOut, size_t bytesToRead) {
    return static_cast<File*>(pUserData)->read(static_cast<uint8_t*>(pBufferOut), bytesToRead);
}


// 文件定位回调
drflac_bool32 AudioPlayer::lfsSeekProc(void* pUserData, int offset, drflac_seek_origin origin) {
    File* f = static_cast<File*>(pUserData);
    SeekMode mode = (origin == DRFLAC_SEEK_CUR) ? SeekCur : (origin == DRFLAC_SEEK_END ? SeekEnd : SeekSet);
    return f->seek(offset, mode);
}


// DMA中断处理程序
void AudioPlayer::dmaPlayIrqHandler() {
    dma_channel_acknowledge_irq0(instance->dmaChannel);
    if (!instance->framesRead) {
        irq_set_enabled(DMA_IRQ_0, false);
        irq_remove_handler(DMA_IRQ_0, dmaPlayIrqHandler);
        if (instance->pFlac) {
            drflac_close(instance->pFlac);
        }
        if (instance->audioFile) {
            instance->audioFile.close();
        }
        Serial.print("播放完成\n");
        instance->isPlaying = false;
        return;
    }
    std::swap(instance->pPlayBuffer, instance->pNextBuffer);
    dma_channel_set_read_addr(instance->dmaChannel, instance->pPlayBuffer, false);
    dma_channel_set_trans_count(instance->dmaChannel, instance->framesRead * instance->pFlac->channels, true);
    instance->framesRead = drflac_read_pcm_frames_s32(instance->pFlac, PCM_FRAME_COUNT, instance->pNextBuffer);
}


// 设置DMA链
void AudioPlayer::setupDMAChain(uint bits) {
    dma_channel_config cfgDMA_play = dma_channel_get_default_config(dmaChannel);
    channel_config_set_transfer_data_size(&cfgDMA_play, bits == 16 ? DMA_SIZE_16 : DMA_SIZE_32);
    channel_config_set_dreq(&cfgDMA_play, pio_get_dreq(audioI2S.getPio(), audioI2S.getSM(), true));
    channel_config_set_read_increment(&cfgDMA_play, true);
    channel_config_set_write_increment(&cfgDMA_play, false);
    channel_config_set_irq_quiet(&cfgDMA_play, false);
    dma_channel_configure(dmaChannel, &cfgDMA_play, &audioI2S.getPio()->txf[audioI2S.getSM()], NULL, 16, false);
    // 设置中断处理程序
    dma_channel_set_irq0_enabled(dmaChannel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dmaPlayIrqHandler);
}


// 播放音频文件
int AudioPlayer::play(const String& path) {
    if(isPlaying) return 0;
    isPlaying = true;
    audioFile = LittleFS.open(path, "r");
    pFlac = drflac_open(lfsReadProc, lfsSeekProc, NULL, &audioFile, NULL);
    if (pFlac) {
        Serial.printf("正在播放: %uHz  %uBits  %u声道\n", pFlac->sampleRate, pFlac->bitsPerSample, pFlac->channels);
        audioI2S.reset(pFlac->sampleRate, pFlac->channels);
        setupDMAChain(pFlac->bitsPerSample);
        framesRead = drflac_read_pcm_frames_s32(pFlac, PCM_FRAME_COUNT, pNextBuffer);
        std::swap(pPlayBuffer, pNextBuffer);
        dma_channel_set_read_addr(dmaChannel, pPlayBuffer, false);
        dma_channel_set_trans_count(dmaChannel, framesRead * pFlac->channels, true);
        irq_set_enabled(DMA_IRQ_0, true);
        return 1;
    }
    return -1;
}
