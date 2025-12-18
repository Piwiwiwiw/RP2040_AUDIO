#ifndef AUDIO_I2S_H
#define AUDIO_I2S_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include <stdint.h>

class AudioI2S {
public:
    // 构造函数和析构函数
    AudioI2S(PIO pio, uint sm, uint data_pin, uint clock_pin_base);
    AudioI2S();
    ~AudioI2S();
    // 初始化和重置音频设置
    void initialize();
    void reset(uint sample_rate, uint channels);
    PIO getPio() {return pio;}
    uint getSM() {return sm;}
private:
    // 成员变量
    PIO pio;
    uint sm;
    uint offset;
    uint data_pin;
    uint clock_pin_base;
};

#endif // AUDIO_I2S_H
