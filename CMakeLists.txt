cmake_minimum_required(VERSION 3.13)
cmake_minimum_required(VERSION 3.13)

# THÊM DÒNG NÀY ĐỂ CHỈ ĐƯỜNG TỚI SDK TRÊN MÁY TÍNH MỚI:
set(PICO_SDK_PATH "C:/Users/Admin/.pico-sdk/sdk/2.2.0")

# Gọi Pico SDK vào
include(pico_sdk_import.cmake)

project(Logic_analyzer_Project C CXX ASM)
# ... (Phần dưới giữ nguyên y hệt)
# Gọi Pico SDK vào (Bắt buộc phải nằm trên project)
include(pico_sdk_import.cmake)

project(Logic_analyzer_Project C CXX ASM)

# Khởi tạo SDK
pico_sdk_init()

# 1. Khai báo file chạy chính (Tên file C phải viết hoa chữ L giống của bạn)
add_executable(Logic_analyzer
    Logic_analyzer.c
)

# 2. Lệnh "Thần thánh": Bắt CMake dịch file .pio thành file header .h
pico_generate_pio_header(Logic_analyzer ${CMAKE_CURRENT_LIST_DIR}/logic_analyzer.pio)

# 3. Mở khóa sức mạnh phần cứng: Thêm thư viện PIO, DMA và Clocks
target_link_libraries(Logic_analyzer
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_clocks
)

# 4. Yêu cầu tự động tạo ra file .uf2 để nạp
pico_add_extra_outputs(Logic_analyzer)