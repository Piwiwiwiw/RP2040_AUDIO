#ifndef AUDIO_I2S_H
#define AUDIO_I2S_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include <stdint.h>

/**
 * @brief 初始化 I2S PIO 状态机 (硬编码 44100 Hz, 132 MHz, PIO0, SM0)。
 *
 * @param data_pin I2S 数据引脚 (SD/DOUT)。
 * @param clock_pin_base I2S 时钟引脚的基准引脚 (BCLK)。
 * LRCLK/WS 将是 clock_pin_base + 1。
 */
 void audio_i2s_init(uint data_pin, uint clock_pin_base, uint simple_rate);

void audio_i2s_set_simplerate(uint sample_rate);

/**
 * @brief 将 32 位立体声样本推送到 PIO FIFO。
 *
 * 32 位数据格式: | 31:16 (左声道, WS=0) | 15:0 (右声道, WS=1) |
 *
 * @param data 包含左右声道 16 位样本的 32 位字。
 */
static inline void audio_i2s_put_sample(uint32_t data) {
    pio_sm_put_blocking(pio0, 0, data);
}

#endif // AUDIO_I2S_H