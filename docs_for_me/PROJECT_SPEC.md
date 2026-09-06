# 🚀 AI Developer Desk Console & Universal Gateway
> **Tài liệu đặc tả kiến trúc, tính năng và tổ chức mã nguồn (Firmware Architecture)**
> **Phiên bản:** 1.1.0 | **Ngày cập nhật:** 2026-09-06
> **Mục tiêu:** Xây dựng trạm điều khiển bàn làm việc đa năng tích hợp giám sát AI Quota, Gamepad/Macro, cần Joystick, cổng tay cầm PS2 và giao tiếp mở rộng RS485.

---

## 1. TỔNG QUAN HỆ THỐNG & Ý TƯỞNG CỐT LÕI

Dự án phát triển một thiết bị phần cứng nhúng cao cấp sử dụng chip **ESP32-S3 (8MB PSRAM)** làm trung tâm, đặt trên bàn làm việc của kỹ sư lập trình / AI maker:

```
+-----------------------------------------------------------------------------------+
|                           AI DEVELOPER DESK CONSOLE                               |
|                                                                                   |
|  [📺 MÀN HÌNH 2.8" TFT CẢM ỨNG]                     [🕹️ CẦN GẠT ANALOG JOYSTICK]   |
|  - Realtime Quota Claude Code & Antigravity        - 2 trục analog (VRx, VRy)      |
|  - Visualizer tay cầm PS2 thời gian thực           - Nút nhấn Thumbstick (SW)      |
|  - Menu chọn Mode (Gamepad / Keyboard / Mouse)     - Điều khiển Chuột / Gamepad    |
|                                                                                   |
|  [🎮 CỔNG CẮM TAY CẦM PS2]                         [🔘 4 PHÍM BẤM MACRO]          |
|  - Cắm tay PS2 không dây/có dây                    - Git commit, Format, Terminal  |
|  - Chuyển đổi sang Bluetooth Xbox Controller       - Phím nhanh tùy chọn           |
|                                                                                   |
|  [💡 ĐÈN LED RGB WS2812 (GPIO 48)]                  [🔌 CỔNG CHỜ RS485]            |
|  - Cảnh báo cạn Quota (<15% chớp đỏ)                - Giao tiếp Robot / Modbus     |
|  - Chỉ báo Mode hoạt động                           - Dây xoắn công nghiệp         |
+-----------------------------------------------------------------------------------+
                                         |
                       [Cáp USB-C (Nguồn + USB-CDC Serial)]
                                         |
                                         v
                         [🖥️ MÁY TÍNH CÁ NHÂN / WORKSTATION]
                  - Chạy daemon nền đọc Quota Claude & Antigravity
                  - Nhận tín hiệu BLE Gamepad / Phím / Chuột
```

### 1.1. Bốn Trụ Cột Tính Năng Chính:
1. **AI Quota & System Telemetry Monitor:**
   - Màn hình 2.8" cập nhật thời gian thực hạn mức token / quota còn lại của **Claude Code** và **Antigravity**.
   - Cảnh báo trực quan bằng thanh đo đồ họa và đèn LED RGB khi quota sắp cạn.
2. **Universal Wireless Gamepad Bridge (PS2 to BLE Xbox):**
   - Đọc tín hiệu từ tay cầm PlayStation 2 (có dây hoặc receiver 2.4GHz).
   - Chuyển đổi thành chuẩn **Xbox Wireless Controller Bluetooth** kết nối với PC hoặc kit robot chạy **Bluepad32**.
3. **Desk Controller & Macro Deck:**
   - Cần gạt Analog Joystick (2 trục X/Y + nút nhấn SW) điều khiển chuột mượt mà ở Mouse Mode, hoặc lái xe / điều khiển nhân vật ở Gamepad Mode.
   - 4 phím cơ vật lý bấm phím tắt tốc độ cao (Macro) không cần nhìn màn hình.
4. **Industrial Automation Bus (RS485):**
   - Tích hợp cổng chờ RS485 phần cứng (UART1), hỗ trợ giao thức Modbus RTU điều khiển cánh tay robot, xe tự hành AGV, động cơ bước/servo khoảng cách xa.

---

## 2. TỔ CHỨC BỐ CỤC MÃ NGUỒN (FIRMWARE ARCHITECTURE)

Dự án được tổ chức theo tiêu chuẩn **ESP-IDF Modular Component Architecture**, tách biệt rõ ràng giữa tầng phần cứng (HAL/Driver), tầng giao thức (Protocol), tầng đồ họa (UI/LVGL) và tầng ứng dụng trung tâm (Core).

### 2.1. Cấu trúc cây thư mục (Directory Structure)

```
esp_lanchpad_hid/
├── CMakeLists.txt                      # Project root CMake
├── sdkconfig.defaults                  # Cấu hình chuẩn (Bật PSRAM, Bluetooth, TinyUSB)
├── docs_for_me/                        # Tài liệu dự án
│   ├── PROJECT_SPEC.md                 # Tài liệu đặc tả & kiến trúc hệ thống
│   └── GPIO_Define.md                  # Bản đồ chân GPIO & sơ đồ đấu nối chi tiết
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml               # Quản lý dependency (lvgl, led_strip, esp_timer)
│   └── main.c                          # Điểm khởi chạy app_main(), khởi tạo các Task
├── components/
│   ├── ble_hid/                        # [Component 1] Quản lý Bluetooth BLE HID
│   │   ├── include/ble_hid.h           # API: send_xbox, send_keyboard, send_mouse
│   │   └── src/ble_hid.c               # GATT Database, SMP Bonding, Advertising
│   │
│   ├── display_ui/                     # [Component 2] Quản lý Màn hình 2.8" & LVGL
│   │   ├── include/display_ui.h        # API: cập nhật quota, cập nhật visualizer
│   │   ├── src/display_driver.c        # Khởi tạo bus SPI, điều khiển ST7789/ILI9341 & XPT2046
│   │   ├── src/ui_dashboard.c          # Giao diện Tab 1: AI Quota & System Telemetry
│   │   ├── src/ui_gamepad.c            # Giao diện Tab 2: Visualizer nút bấm PS2
│   │   └── src/ui_macro.c              # Giao diện Tab 3: Cài đặt phím & Joystick
│   │
│   ├── ps2_controller/                 # [Component 3] Driver đọc tay cầm PS2
│   │   ├── include/ps2_controller.h    # API: ps2_read_buttons(), ps2_read_analog()
│   │   └── src/ps2_controller.c        # Giao thức truyền thông nối tiếp SPI mềm với tay PS2
│   │
│   ├── analog_joystick/                # [Component 4] Driver đọc cần gạt Joystick ADC1
│   │   ├── include/analog_joystick.h   # API: đọc trục X/Y (-32768..32767), nút nhấn SW
│   │   └── src/analog_joystick.c       # Đọc ADC1_CH0 (GPIO 1), ADC1_CH1 (GPIO 2), SW (GPIO 3)
│   │
│   ├── rs485_bus/                      # [Component 5] Driver giao tiếp RS485
│   │   ├── include/rs485_bus.h         # API: rs485_send_packet(), rs485_receive()
│   │   └── src/rs485_bus.c             # Cấu hình UART1, Auto Direction Control DE/RE
│   │
│   ├── pc_sync/                        # [Component 6] Giao tiếp đồng bộ dữ liệu với PC
│   │   ├── include/pc_sync.h           # API: nhận quota JSON từ máy tính qua USB CDC
│   │   └── src/pc_sync.c               # Parser cJSON bóc tách token Claude & Antigravity
│   │
│   └── launchpad_core/                 # [Component 7] Bộ não trung tâm (Core Coordinator)
│       ├── include/launchpad_core.h    # Event Bus & State Machine
│       └── src/
│           ├── launchpad_core.c        # Điều phối sự kiện giữa các component
│           ├── mode_manager.c          # Quản lý 3 Mode (Gamepad, Keyboard, Mouse)
│           └── led_manager.c           # Điều khiển hiệu ứng LED WS2812 trên GPIO 48
└── host_tools/                         # Công cụ chạy trên máy tính (PC Companion)
    ├── quota_daemon.py                 # Script Python chạy ngầm đọc quota & gửi xuống ESP32
    └── requirements.txt                # pyserial, psutil
```

---

## 3. PHÂN CHIA TÀI NGUYÊN PHẦN CỨNG & DUAL-CORE FREERTOS

ESP32-S3 sở hữu 2 nhân vi xử lý Xtensa LX7 chạy ở xung nhịp 240 MHz và 8MB Octal PSRAM. Firmware được phân bổ đa nhiệm để đạt độ mượt mà tối đa:

```
========================= CORE 0 (Communication & I/O) =========================
- Priority 10: BLE Stack & GATT Server (Xử lý gói tin Bluetooth)
- Priority 8:  PS2 Controller Task (Đọc liên tục 60Hz trạng thái tay cầm PS2)
- Priority 7:  PC Sync Task (Lắng nghe Serial USB-CDC nhận dữ liệu Quota JSON)
- Priority 5:  RS485 Bus Task (Lắng nghe mạng công nghiệp khi cần)
- Priority 4:  Analog Joystick Sampling & Button Debounce Task (Đọc ADC1 mượt)

============================ CORE 1 (Graphics & UI) ============================
- Priority 9:  LVGL Rendering Task (Xử lý vẽ đồ thị, animation 60 FPS từ PSRAM)
- Priority 6:  Touch Screen Read Task (Đọc tọa độ cảm ứng XPT2046)
- Priority 3:  LED WS2812 Effects Manager (Đổi màu theo Mode & Quota status)
```
