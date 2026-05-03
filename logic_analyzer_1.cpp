#include <stdio.h>
#include "pico/stdlib.h"

// --- CẤU HÌNH THÔNG SỐ ---
#define INPUT_PIN 0         // Nối chân GP0 của Pico với chân PA8 của STM32
#define NUM_SAMPLES 500     // Số lượng mẫu chụp trong 1 lần (để gửi cho Python)

// Mảng chứa dữ liệu thu thập được (chỉ chứa số 0 hoặc 1)
uint8_t capture_buffer[NUM_SAMPLES];

int main() {
    // 1. Khởi tạo hệ thống giao tiếp USB (Bắt buộc để gửi dữ liệu lên máy tính)
    stdio_init_all();

    // 2. Cấu hình chân GP0 làm chân Đầu vào (Input)
    gpio_init(INPUT_PIN);
    gpio_set_dir(INPUT_PIN, GPIO_IN);

    // Vòng lặp vô hạn của vi điều khiển
    while (true) {
        // 3. Chờ tín hiệu từ máy tính (Python sẽ gửi ký tự 'c' xuống)
        int command = getchar_timeout_us(0); // Đọc cổng USB mà không làm treo mạch

        if (command == 'c') {
            // --- BƯỚC 1: BẮT TRIGGER (Bắt sườn xung) ---
            // Để đồ thị lúc nào vẽ ra cũng bắt đầu từ một chu kỳ mới, 
            // ta chờ tín hiệu ở mức Cao, sau đó rớt xuống mức Thấp rồi mới bắt đầu ghi.
            while (gpio_get(INPUT_PIN) == 1); // Chờ nếu đang ở mức 1
            while (gpio_get(INPUT_PIN) == 0); // Chờ cho đến khi nhảy lên 1 lại

            // --- BƯỚC 2: CHỤP DỮ LIỆU TỐC ĐỘ CAO ---
            for (int i = 0; i < NUM_SAMPLES; i++) {
                // Đọc trạng thái chân GP0 (hàm này trả về 0 hoặc 1)
                capture_buffer[i] = gpio_get(INPUT_PIN);
                
                // Nghỉ 2 micro-giây giữa mỗi lần đọc.
                // Tín hiệu 10kHz có chu kỳ là 100us. Lấy mẫu mỗi 2us giúp ta 
                // có 50 điểm ảnh cho mỗi một chu kỳ sóng vuông, đồ thị sẽ rất sắc nét.
                sleep_us(2); 
            }

            // --- BƯỚC 3: GỬI DỮ LIỆU LÊN MÁY TÍNH ---
            // In từng mẫu dữ liệu qua cổng USB ảo để Python đọc
            for (int i = 0; i < NUM_SAMPLES; i++) {
                printf("%d\n", capture_buffer[i]);
            }
        }

        // Nghỉ ngơi 10ms để nhường tài nguyên cho hệ thống USB xử lý
        sleep_ms(10);
    }

    return 0;
}