#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "logic_analyzer.pio.h"

#define CAPTURE_DEPTH 100000 
uint16_t capture_buf[CAPTURE_DEPTH]; 

void logic_analyzer_init(PIO pio, uint sm, uint offset, uint pin_base, uint pin_count, float div) {
    pio_sm_config c = logic_analyzer_program_get_default_config(offset);

    sm_config_set_in_pins(&c, pin_base);
    for(uint i=0; i<pin_count; i++) {
        pio_gpio_init(pio, pin_base + i);
    }

    sm_config_set_in_shift(&c, false, true, 16);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    sm_config_set_clkdiv(&c, div); // Dùng biến div truyền vào

    pio_sm_init(pio, sm, offset, &c);
}

int main() {
    stdio_init_all();

    // --- BƯỚC NÂNG CẤP TỐC ĐỘ ---
    // Ép xung hệ thống lên 150 MHz thay vì 120 MHz
    set_sys_clock_khz(150000, true);

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &logic_analyzer_program);

    // Tính tốc độ: Hệ thống 150MHz / Bộ chia 5.0 = 30 MHz!
    // Đọc từ chân GP0, số lượng 16 chân.
    logic_analyzer_init(pio, sm, offset, 0, 16, 5.0f);

    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dma_conf = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&dma_conf, false); 
    channel_config_set_write_increment(&dma_conf, true); 
    channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_16);
    channel_config_set_dreq(&dma_conf, pio_get_dreq(pio, sm, false)); 

    while (true) {
        printf("\n=== LOGIC ANALYZER 16 KENH (TOC DO 30MHz) ===\n");
        printf("He thong dang chay o xung nhip: %d Hz\n", clock_get_hz(clk_sys));
        printf("-> Nhan phim 's' + Enter de bat dau chup tin hieu...\n");

        char cmd = getchar();
        if (cmd == 's') {
            printf(">>> DANG GHI HINH (Khoang 3.3 mili-giay)...\n");

            dma_channel_configure(
                dma_chan, &dma_conf,
                capture_buf,        
                &pio->rxf[sm],      
                CAPTURE_DEPTH,      
                true                
            );

            pio_sm_set_enabled(pio, sm, true);
            dma_channel_wait_for_finish_blocking(dma_chan);
            pio_sm_set_enabled(pio, sm, false);
            pio_sm_clear_fifos(pio, sm);

            printf(">>> CHUP THANH CONG! Dang gui du lieu len...\n");

            for (int i = 0; i < CAPTURE_DEPTH; i++) {
                printf("%d\n", capture_buf[i]);
            }
            printf("--- KET THUC ---\n");
        }
    }
}