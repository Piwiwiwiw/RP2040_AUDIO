#include "audio_i2s_PIO.h"
#include "audio_i2s.pio.h" // 假设 PIO 汇编程序已包含
#include "hardware/clocks.h"
#include "hardware/dma.h"


#define TARGET_PIO pio0
#define TARGET_SM 0



void audio_i2s_init(uint data_pin, uint clock_pin_base, uint simple_rate) {
    // 1. 加载 PIO 程序
    uint offset = pio_add_program(TARGET_PIO, &audio_i2s_program);
    // 2. 获取默认配置
    pio_sm_config sm_config = audio_i2s_program_get_default_config(offset);
    // 3. 配置引脚
    sm_config_set_out_pins(&sm_config, data_pin, 1);
    sm_config_set_sideset_pins(&sm_config, clock_pin_base);
    // 4. 配置移位和 Autopull
    // 启用 Autopull, 移位方向向左 (MSB-first), 阈值 32 位
    sm_config_set_out_shift(&sm_config, false, true, 32); 
    float PIO_CLK_DIV = (float)clock_get_hz(clk_sys)/(float)(simple_rate *256);
    // 5. 硬编码时钟分频器
    sm_config_set_clkdiv(&sm_config, PIO_CLK_DIV);
    // 6. 初始化状态机
    pio_sm_init(TARGET_PIO, TARGET_SM, offset, &sm_config);
    // 7. 配置引脚方向和初始值
    uint pin_mask = (1u << data_pin) | (3u << clock_pin_base); // data_pin + BCLK + LRCLK
    pio_sm_set_pindirs_with_mask(TARGET_PIO, TARGET_SM, pin_mask, pin_mask); // 全部设置为 OUT
    pio_sm_set_pins(TARGET_PIO, TARGET_SM, 0); // 清除引脚初始输出
    gpio_set_function(data_pin, GPIO_FUNC_PIO0);
    gpio_set_function(clock_pin_base, GPIO_FUNC_PIO0);
    gpio_set_function(clock_pin_base + 1, GPIO_FUNC_PIO0);
    // 8. 启动状态机并跳转到入口点
    pio_sm_exec(TARGET_PIO, TARGET_SM, pio_encode_jmp(offset + audio_i2s_offset_entry_point));
    pio_sm_set_enabled(TARGET_PIO, TARGET_SM, true);
}



void audio_i2s_set_simplerate(uint sample_rate) {
    pio_sm_set_enabled(TARGET_PIO, TARGET_SM, false);
    pio_sm_clear_fifos(TARGET_PIO, TARGET_SM);
    float div = (float)clock_get_hz(clk_sys) / (float)(sample_rate * 256);
    pio_sm_set_clkdiv(TARGET_PIO, TARGET_SM, div);
    pio_sm_set_enabled(TARGET_PIO, TARGET_SM, true);

}

