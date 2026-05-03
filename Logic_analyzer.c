#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "logic_analyzer.pio.h" // Header sinh ra từ file .pio

// Cấu hình dung lượng bộ nhớ đệm: 100,000 mẫu = 100 KiloBytes RAM
#define CAPTURE_DEPTH 100000 
uint8_t capture_buf[CAPTURE_DEPTH]; // Mảng lưu trữ 8-bit (1 Byte cho 8 kênh)

void logic_analyzer_init(PIO pio, uint sm, uint offset, uint pin_base, uint pin_count, float div) {
    pio_sm_config c = logic_analyzer_program_get_default_config(offset);

    // 1. Cấu hình chân GPIO để PIO đọc
    sm_config_set_in_pins(&c, pin_base);
    for(uint i=0; i<pin_count; i++) {
        pio_gpio_init(pio, pin_base + i);
    }

    // 2. Cấu hình thanh ghi dịch (Shift Register) siêu tốc
    // Dịch phải = false, Autopush = true, Ngưỡng đẩy = 8 bits
    sm_config_set_in_shift(&c, false, true, 8);

    // Nối 2 mảng FIFO lại với nhau để chống nghẽn cổ chai
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    // 3. Cấu hình bộ chia xung nhịp (Divider)
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
}

int main() {
    stdio_init_all();

    // 🚀 BƯỚC ĐỘT PHÁ: ÉP XUNG HỆ THỐNG LÊN 150 MHz
    set_sys_clock_khz(150000, true);

    // --- SETUP PIO ---
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &logic_analyzer_program);

    // Tính tốc độ: 150MHz / 5.0 = ĐẠT CHUẨN 30 MHz!
    // Đọc từ chân GP0, số lượng 8 chân.
    logic_analyzer_init(pio, sm, offset, 0, 8, 5.0f);

    // --- SETUP DMA (Vận chuyển dữ liệu không cần CPU) ---
    uint dma_chan = dma_claim_unused_channel(true);
    dma_channel_config dma_conf = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&dma_conf, false); // Nguồn tĩnh (luôn đọc từ cổng PIO)
    channel_config_set_write_increment(&dma_conf, true); // Đích động (đẩy dần vào mảng RAM)
    channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_8); // Vận chuyển từng Byte (8-bit)
    channel_config_set_dreq(&dma_conf, pio_get_dreq(pio, sm, false)); // Đồng bộ nhịp với PIO

    while (true) {
        printf("\n=== LOGIC ANALYZER 8 KENH (TOC DO 30MHz) ===\n");
        printf("He thong dang chay o xung nhip: %d Hz\n", clock_get_hz(clk_sys));
        printf("-> Nhan phim 's' + Enter de bat dau chup tin hieu...\n");

        char cmd = getchar();
        if (cmd == 's') {
            printf(">>> DANG GHI HINH (3.3 mili-giay)...\n");

            // 1. Lên nòng DMA
            dma_channel_configure(
                dma_chan, &dma_conf,
                capture_buf,        // Nơi đổ vào: Mảng RAM
                &pio->rxf[sm],      // Nơi hút ra: Cổng dữ liệu của PIO
                CAPTURE_DEPTH,      // Số lượng: 100,000 mẫu
                true                // Kích hoạt DMA chờ sẵn
            );

            // 2. Bấm máy chụp (Kích hoạt PIO)
            pio_sm_set_enabled(pio, sm, true);

            // 3. Chờ DMA vận chuyển đủ 100,000 điểm dữ liệu
            dma_channel_wait_for_finish_blocking(dma_chan);

            // 4. Ngưng chụp
            pio_sm_set_enabled(pio, sm, false);
            pio_sm_clear_fifos(pio, sm);

            printf(">>> CHUP THANH CONG! Dang gui du lieu len...\n");

            // 5. In dữ liệu ra màn hình (Gửi cho Python)
            for (int i = 0; i < CAPTURE_DEPTH; i++) {
                printf("%d\n", capture_buf[i]);
            }
            printf("--- KET THUC ---\n");
        }
    }
}