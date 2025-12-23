
#include "audio_i2s_PIO.h"   // 假设 PIO 汇编程序已包含
#include "audio_i2s.pio.h"
#include "hardware/clocks.h"



// 构造函数
AudioI2S::AudioI2S(PIO _pio, uint _sm, uint _data_pin, uint _clock_pin_base)
    : pio(_pio), sm(_sm), data_pin(_data_pin), clock_pin_base(_clock_pin_base) {
}


AudioI2S::AudioI2S(){
        pio = pio1, sm = 4, data_pin = 14, clock_pin_base = 15;
}
     

// 析构函数
AudioI2S::~AudioI2S() {
    pio_sm_set_enabled(pio, sm, false);
    pio_remove_program_and_unclaim_sm(&audio_i2s_program, pio, sm, 0);
}



// 初始化 I2S
void AudioI2S::initialize() {
    // 加载 PIO 程序
    offset = pio_add_program(pio, &audio_i2s_program);
    pio_sm_config sm_config = audio_i2s_program_get_default_config(offset);

    // 配置引脚
    sm_config_set_out_pins(&sm_config, data_pin, 1);
    sm_config_set_sideset_pins(&sm_config, clock_pin_base);
    sm_config_set_out_shift(&sm_config, false, false, 32); 

    // 设置时钟分频
    float PIO_CLK_DIV = (float)clock_get_hz(clk_sys) / (float)(16000 * 512);
    sm_config_set_clkdiv(&sm_config, PIO_CLK_DIV);

    // 初始化状态机
    pio_sm_init(pio, sm, offset, &sm_config);

    // 配置引脚方向和初始值
    uint pin_mask = (1u << data_pin) | (3u << clock_pin_base);
    pio_sm_set_pindirs_with_mask(pio, sm, pin_mask, pin_mask);
    pio_sm_set_pins(pio, sm, 0);
    gpio_set_function(data_pin, pio==pio0?GPIO_FUNC_PIO0:GPIO_FUNC_PIO1);
    gpio_set_function(clock_pin_base, pio==pio0?GPIO_FUNC_PIO0:GPIO_FUNC_PIO1);
    gpio_set_function(clock_pin_base + 1, pio==pio0?GPIO_FUNC_PIO0:GPIO_FUNC_PIO1);
    // 启动状态机并跳转到入口点
    pio_sm_exec(pio, sm, pio_encode_jmp(offset + audio_i2s_offset_entry_point));
    pio_sm_set_enabled(pio, sm, true);
}



// 重置 I2S 配置
void AudioI2S::reset(uint sample_rate, uint channels) {
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);
    float div = (float)clock_get_hz(clk_sys) / (float)(sample_rate * 512);
    pio_sm_set_clkdiv(pio, sm, div);
    if (channels != 1) {
        pio_sm_exec(pio, sm, pio_encode_jmp(offset + audio_i2s_offset_stereo));
    } else {
        pio_sm_exec(pio, sm, pio_encode_jmp(offset + audio_i2s_offset_single));
    }
    pio_sm_set_enabled(pio, sm, true);
}
