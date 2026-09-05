# ESP32-S3 BLE Multi-HID Controller (Bàn phím + Chuột + Media + LaunchPad)

Dự án firmware ESP32-S3 điều khiển thiết bị **Multi-function BLE HID** đa năng chuyên nghiệp, hỗ trợ giao tiếp không dây với hệ điều hành **Ubuntu**, **Windows**, **macOS**, **Android**, **iOS**.

---

## 🎹 Tính năng HID Đa Năng (Composite HID Reports)

Thiết bị được thiết kế theo chuẩn BLE HID Profile (HOGP - HID Over GATT Profile) với 4 kênh điều khiển độc lập:

1. **Report ID 1 (Keyboard - Bàn phím chuẩn 8-byte)**:
   - Hỗ trợ 8 phím Modifier (`Ctrl`, `Shift`, `Alt`, `GUI`) và tối đa 6 phím nhấn cùng lúc (6KRO).
   - Dùng để gửi phím tắt, gõ văn bản, điều khiển DAW hoặc game.

2. **Report ID 2 (Consumer Media Control)**:
   - Điều khiển âm lượng và trình phát đa phương tiện: `Play/Pause`, `Stop`, `Next Track`, `Previous Track`, `Mute`, `Volume Up`, `Volume Down`.

3. **Report ID 3 (LaunchPad 16-Button Matrix)**:
   - Quản lý ma trận 16 phím bấm (Buttons 1-16) cho pad âm nhạc, DJ, hoặc Gamepad.

4. **Report ID 4 (Mouse - Chuột máy tính)**:
   - Điều khiển con trỏ chuột: Tọa độ tương đối `X`, `Y` (-127 đến 127).
   - Cuộn chuột (`Wheel`).
   - 3 nút bấm chuột: `Left Click`, `Right Click`, `Middle Click`.

---

## 🛠 Nguyên nhân & Cách khắc phục lỗi "Không tìm thấy ESP qua Bluetooth"

| Vấn đề trước đây | Nguyên nhân gốc rễ | Giải pháp đã khắc phục |
|---|---|---|
| Không thể search thấy ESP32 | Gói `raw_adv_data` dài **32 bytes**, trong khi giới hạn chuẩn của BLE Legacy Advertising là **31 bytes**. ESP-IDF từ chối cấu hình (`ESP_ERR_INVALID_ARG`) làm BLE không phát sóng. | Tách tên thiết bị sang gói **Scan Response Data** (`ESP32 Multi-HID`), gói Adv Data rút gọn còn **20 bytes** (an toàn tuyệt đối). |
| Race Condition khi bật Advertising | Gọi `esp_ble_gap_start_advertising()` trước khi các gói Adv Data / Scan Response nạp xong vào BLE Controller. | Sử dụng cờ đồng bộ `adv_config_done` đợi đầy đủ cả Service Table và Adv Data mới bắt đầu phát sóng. |
| Console Log không hiển thị | Cấu hình xuất log mặc định ra UART0 (GPIO 43/44) thay vì cổng USB Type-C native của ESP32-S3 (`/dev/ttyACM0`). | Cấu hình `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` để theo dõi log trực tiếp qua cổng USB native. |

---

## 🚀 Thao tác với ESP-IDF

### 1. Kích hoạt môi trường ESP-IDF v5.3.5
```bash
source /home/hungdk4/.espressif/v5.3.5/esp-idf/export.sh
```

### 2. Biên dịch & Nạp Firmware
```bash
cd esp_lanchpad_hid
idf.py -p /dev/ttyACM0 flash
```

### 3. Xem Log Serial Monitor
```bash
idf.py -p /dev/ttyACM0 monitor
```

---

## 📱 Hướng dẫn Kết nối & Kiểm tra Thực tế

1. Mở Cài đặt **Bluetooth** trên máy tính (Ubuntu hoặc Windows) hoặc điện thoại.
2. Tìm thiết bị có tên **`ESP32 Multi-HID`** và bấm **Connect / Pair**.
3. Thiết bị sẽ tự động ghép đôi bảo mật (Just Works Bonding).
4. **Kiểm tra chức năng Tăng Âm Lượng bằng nút BOOT (GPIO 0)**:
   - Nhấn nút **BOOT**: ESP32 gửi lệnh Media **Volume Increment (Tăng âm lượng)**. Bạn sẽ thấy thanh âm lượng trên máy tính hoặc điện thoại tăng lên!
   - Nhả nút **BOOT**: ESP32 gửi nhả phím media.
