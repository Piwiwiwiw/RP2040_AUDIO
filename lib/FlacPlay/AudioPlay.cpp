#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_CRC
#define DR_MP3_IMPLEMENTATION
#include "AudioPlay.h"
AudioPlayer* AudioPlayer::instance = nullptr;


// 构造函数
AudioPlayer::AudioPlayer(PIO pio, uint dmaChannel, uint i2sDataPin, uint i2sClockPin)
    : dmaChannel(dmaChannel),
      pFlac(nullptr), bufferReady(false), 
      framesRead(0), 
      isPlaying(false)
      {
      uint sm= pio_claim_unused_sm(pio, true);
      audioI2S = AudioI2S(pio, sm, i2sDataPin, i2sClockPin);
      audioI2S.initialize();
      instance = this;

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

drmp3_bool32 AudioPlayer::lfsSeekProc(void* pUserData, int offset, drmp3_seek_origin origin) {
    File* f = static_cast<File*>(pUserData);
    SeekMode mode = (origin == DRMP3_SEEK_CUR) ? SeekCur : (origin == DRMP3_SEEK_END ? SeekEnd : SeekSet);
    return f->seek(offset, mode);
}


// DMA中断处理程序
void AudioPlayer::dmaPlayIrqHandler() {
    dma_channel_acknowledge_irq1(instance->dmaChannel);
    if (!instance->framesRead) {
        dma_channel_set_irq1_enabled(instance->dmaChannel, false);
        irq_remove_handler(DMA_IRQ_0, dmaPlayIrqHandler);
        if (instance->pFlac) {
            drflac_close(instance->pFlac);
            instance->pFlac = NULL;
        }
        if (instance->pMP3) {
            drmp3_uninit(instance->pMP3);
        }

        if (instance->audioFile) {
            instance->audioFile = nullptr;
        }
        instance->isPlaying = false;
        return;
    }
    
    switch(instance->form){
        case wav:
             break;
        case flac:
            std::swap(instance->pPlayBuffer.flacBuffer, instance->pNextBuffer.flacBuffer);
            dma_channel_set_read_addr(instance->dmaChannel, instance->pPlayBuffer.flacBuffer, false);
            dma_channel_set_trans_count(instance->dmaChannel, instance->framesRead * instance->pFlac->channels, true);
            instance->framesRead = drflac_read_pcm_frames_s32(instance->pFlac, PCM_FRAME_COUNT, instance->pNextBuffer.flacBuffer);
            break;
        case mp3:
            std::swap(instance->pPlayBuffer.mp3Buff, instance->pNextBuffer.mp3Buff);
            dma_channel_set_read_addr(instance->dmaChannel, instance->pPlayBuffer.mp3Buff, false);
            dma_channel_set_trans_count(instance->dmaChannel, instance->framesRead * instance->pMP3->channels, true);
            instance->framesRead = drmp3_read_pcm_frames_s16(instance->pMP3, PCM_FRAME_COUNT, instance->pNextBuffer.mp3Buff);
            break;
    }
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
int AudioPlayer::play(File* audio, format mode) {
    form = mode;
    if(isPlaying) return 0;
    audioFile = audio;
    if(!audioFile){
        Serial.println("打开文件失败");
        return -1;
    }
    switch(form){
        case wav: 
            break;
        case flac: 
            pFlac = drflac_open(lfsReadProc, lfsSeekProc, NULL, audioFile, NULL);
            if(pFlac){
                audioI2S.reset(pFlac->sampleRate, pFlac->channels);
                setupDMAChain(pFlac->bitsPerSample);
                pNextBuffer.flacBuffer = buffer_A.bufferFlac;
                pPlayBuffer.flacBuffer = buffer_B.bufferFlac;
                framesRead = drflac_read_pcm_frames_s32(pFlac, PCM_FRAME_COUNT, pNextBuffer.flacBuffer);
                std::swap(pPlayBuffer, pNextBuffer);
                dma_channel_set_read_addr(dmaChannel, pPlayBuffer.flacBuffer, false);
                dma_channel_set_trans_count(dmaChannel, framesRead * pFlac->channels, true);
                irq_set_enabled(DMA_IRQ_0, true);
                isPlaying = true;
            }
            break;
        case mp3:

            if(drmp3_init(pMP3, lfsReadProc, lfsSeekProc, NULL, NULL, audioFile, NULL)){
                audioI2S.reset(pMP3->sampleRate, pMP3->channels);
                setupDMAChain(16);
                pNextBuffer.mp3Buff = buffer_A.bufferMP3;
                pPlayBuffer.mp3Buff = buffer_B.bufferMP3;
                framesRead = drmp3_read_pcm_frames_s16(pMP3, PCM_FRAME_COUNT, pNextBuffer.mp3Buff);
                std::swap(pPlayBuffer, pNextBuffer);
                dma_channel_set_read_addr(dmaChannel, pPlayBuffer.mp3Buff, false);
                dma_channel_set_trans_count(dmaChannel, framesRead * pMP3->channels, true);
                irq_set_enabled(DMA_IRQ_0, true);
                isPlaying = true;
            }
            break;
        default:
            break;
    }
    while(isPlaying)
        delay(50);
    return 0;
}
