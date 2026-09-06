# ESP32-S3 BLE Gamepad & Multi-HID Controller

Dự án firmware ESP32-S3 điều khiển thiết bị **Standard BLE Gamepad & Multi-HID** chuyên nghiệp, hỗ trợ giao tiếp không dây với **Bluepad32**, **Ubuntu**, **Windows**, **macOS**, **Android**, **iOS**, **Steam**.

---

## 🎮 Chuẩn Gamepad Bluetooth (HOGP - Xbox Wireless Controller)

Thiết bị được thiết kế chuẩn Xbox Wireless Controller (tương thích 100% với **Bluepad32**, **Ubuntu**, **Windows**, **macOS**, **Android**, **iOS**, **Steam**):
- **Device Name**: `Xbox Wireless Controller`
- **Appearance**: `0x03C4` (Gamepad Controller).
- **Vendor ID (VID)**: `0x045E` (Microsoft)
- **Product ID (PID)**: `0x02E0` (Xbox One Controller Bluetooth)
- **Services**:
  1. `Device Information Service (0x180A)`: Cung cấp PnP ID, Manufacturer, Model Number.
  2. `Human Interface Device Service (0x1812)`: Bảng Descriptor 335 byte chuẩn Xbox One Firmware v4.8/v5.x.
- **Tọa độ Analog Left Stick**:
  - Tâm (Center): `left_x = 32768, left_y = 32768`
  - Đẩy LÊN MAX 100%: `left_y = 0` (Trên Bluepad32 `ctl->axisY()` trả về `-512`)
  - Đẩy XUỐNG MAX 100%: `left_y = 65535` (Trên Bluepad32 `ctl->axisY()` trả về `+511`)

---

## 🕹️ Chế Độ Hoạt Động & Nút BOOT (GPIO 0)

Thiết bị tích hợp hệ thống **Chuyển Chế Độ Linh Hoạt (Multi-HID Mode)**:

### 1. Thao tác điều khiển với Nút BOOT (GPIO 0):
- **Bấm nhanh (< 2 giây)**: Kích hoạt hành vi tương ứng của Mode hiện tại.
- **Nhấn giữ 2 giây**: **Chuyển sang Mode tiếp theo** (Xoay vòng: Gamepad ➔ Keyboard ➔ Mouse ➔ Gamepad).
- **Nhấn giữ > 6 giây**: **Xóa bộ nhớ ghép đôi (Reset Bonds)** trong NVS Flash để ghép đôi với máy mới.

---

### 2. Chi tiết 3 Chế Độ (Modes):

| Chế Độ | Biểu Tượng | Hành Vi Khi Bấm Nút BOOT (<2s) | Nhả Nút BOOT | Tương Thích |
| :--- | :---: | :--- | :--- | :--- |
| **Mode 1: GAMEPAD** | 🎮 | Đẩy **Left Joystick LÊN MAX 100%** (`left_y = 0`) | Left Joystick về tâm (`32768`) | **Bluepad32** (`ctl->axisY() = -512`), PC, Steam |
| **Mode 2: KEYBOARD** | ⌨️ | Gửi phím **SPACE (Phím Cách)** | Nhả phím | Máy tính, Điện thoại (Dừng/phát Youtube, phím nhảy) |
| **Mode 3: MOUSE** | 🖱️ | **Click Chuột Trái** (`Left Click Down`) | Nhả Click Chuột Trái | Máy tính, Điện thoại (Bấm chọn link, file, click màn hình) |

---

## 🔍 Tại sao giải pháp này tối ưu?
- **Không cần ngắt kết nối Bluetooth**: Nhờ chuẩn **Composite HID**, máy tính nhận diện đồng thời cả 3 driver (Gamepad, Bàn phím, Chuột) trong 1 kết nối duy nhất. Khi bạn giữ 2s đổi mode, lệnh được chuyển tức thì 0 giây.
- **Tương thích hoàn toàn với Bluepad32**: Ở Mode Gamepad, Bluepad32 nhận diện thiết bị là `Xbox Wireless Controller` và đọc trọn vẹn tín hiệu cần Joystick / Buttons.

---

## 📱 Hướng Dẫn Kết Nối với Con ESP32 chạy Bluepad32

Trên con ESP32 chạy Bluepad32 (Arduino / ESP-IDF):

```cpp
#include <Bluepad32.h>

void onConnectedController(ControllerPtr ctl) {
    Serial.printf("🎉 Đã kết nối thành công với Gamepad: %s\n", ctl->getModelName().c_str());
}

void onDisconnectedController(ControllerPtr ctl) {
    Serial.println("Gamepad disconnected.");
}

void setup() {
    Serial.begin(115200);
    
    // Mở cửa đón nhận thiết bị mới & xóa cache cũ nếu cần
    BP32.forgetBluetoothKeys();
    BP32.enableNewBluetoothConnections(true);
    
    BP32.setup(&onConnectedController, &onDisconnectedController);
}

void loop() {
    BP32.update();
    // Đọc trạng thái nút bấm và cần analog từ ESP32 Gamepad...
}
```

---

## 🚀 Thao tác Lệnh trên Máy tính

```bash
# Kích hoạt ESP-IDF
source /home/hungdk4/.espressif/v5.3.5/esp-idf/export.sh

# Nạp Firmware
idf.py -p /dev/ttyACM0 flash

# Theo dõi Log Serial
idf.py -p /dev/ttyACM0 monitor
```
