# 📘 TỔNG QUAN TOÀN DIỆN VỀ CÁC LOẠI THIẾT BỊ HID (HUMAN INTERFACE DEVICE)
> **Tài liệu kỹ thuật chi tiết dành cho dự án Universal LaunchPad HID (ESP32-S3)**  
> *Phiên bản: 1.0 — Ngày cập nhật: 09/09/2026*

---

## 📑 MỤC LỤC
1. [Giới Thiệu Chung Về Chuẩn HID](#1-giới-thiệu-chung-về-chuẩn-hid)
2. [Cơ Chế Hoạt Động Cốt Lõi Của HID](#2-cơ-chế-hoạt-động-cốt-lõi-của-hid)
3. [Bàn Phím (Keyboard HID)](#3-bàn-phím-keyboard-hid)
4. [Chuột (Mouse HID)](#4-chuột-mouse-hid)
5. [Tay Cầm Chơi Game (Gamepad & Joystick HID)](#5-tay-cầm-chơi-game-gamepad--joystick-hid)
6. [Điều Khiển Đa Phương Tiện (Consumer Control / Media Keys)](#6-điều-khiển-đa-phương-tiện-consumer-control--media-keys)
7. [Các Loại Thiết Bị HID Mở Rộng Khác](#7-các-loại-thiết-bị-hid-mở-rộng-khác)
8. [Kiến Trúc Triển Khai Trong Dự Án LaunchPad ESP32-S3](#8-kiến-trúc-triển-khai-trong-dự-án-launchpad-esp32-s3)
9. [Hướng Dẫn Tùy Biến Cấu Hình Nút Nhấn & Joystick](#9-hướng-dẫn-tùy-biến-cấu-hình-nút-nhấn--joystick)

---

## 1. Giới Thiệu Chung Về Chuẩn HID

**HID (Human Interface Device)** là một tiêu chuẩn giao tiếp thiết bị ngoại vi do hiệp hội USB-IF phát triển, sau đó được áp dụng rộng rãi trên cả giao thức có dây (**USB HID**) lẫn không dây (**Bluetooth Classic HID / BLE HOGP - HID over GATT Profile**).

### ✨ Ưu điểm vượt trội của HID:
- **Driverless (Không cần cài Driver)**: Tất cả các hệ điều hành hiện đại (**Windows, macOS, Linux, Android, iOS, iPadOS, ChromeOS**) đều tích hợp sẵn Generic HID Driver trong kernel. Thiết bị cắm vào hoặc kết nối Bluetooth là hoạt động ngay lập tức (Plug and Play).
- **Độ trễ thấp & Tính dự đoán cao**: Giao tiếp định kỳ dạng Interrupt Transfer (USB) hoặc BLE Notification với chu kỳ cực ngắn (1ms, 8ms, 10ms).
- **Hỗ trợ Composite Device (Đa thiết bị trên 1 kết nối)**: Một thiết bị vật lý duy nhất (như kit LaunchPad của bạn) có thể đóng vai trò đồng thời là **Bàn phím + Chuột + Gamepad + Remote Media**.

---

## 2. Cơ Chế Hoạt Động Cốt Lõi Của HID

### 2.1. HID Report Descriptor (Bản đặc tả cấu trúc dữ liệu)
Trước khi PC nhận dữ liệu, thiết bị MCU gửi cho Host một bảng mã nhị phân gọi là **Report Descriptor**. Bảng này mô tả:
- Thiết bị có những báo cáo (Reports) nào (phân biệt bằng **Report ID**).
- Mỗi báo cáo gồm bao nhiêu byte, từng bit mang ý nghĩa gì (Trục X, Trục Y, Nút bấm 1..16, Phím bàn phím...).
- Đơn vị đo, giá trị cực tiểu (Logical Minimum) và cực đại (Logical Maximum).

```
+-------------------------------------------------------------+
|                  HOST PC (Windows / Linux / Mac)            |
+-------------------------------------------------------------+
                              ▲
                              │ BLE Notification (Raw Bytes)
                              │ e.g. [ReportID=2][Btns][dX][dY][Wheel]
+-------------------------------------------------------------+
|                   ESP32-S3 LaunchPad HID                     |
|  - Report ID 1: Keyboard Report                             |
|  - Report ID 2: Mouse Report                                |
|  - Report ID 3: Gamepad Report (Xbox style)                 |
|  - Report ID 4: Consumer Control (Volume, Media)            |
+-------------------------------------------------------------+
```

### 2.2. Chu kỳ Nhấn - Nhả (Press & Release Cycle)
Chuẩn HID yêu cầu nguyên tắc:
1. **Khi nhấn một nút**: Gửi báo cáo mang bitmask hoặc mã phím đang nhấn.
2. **Khi giữ nút**: Có thể gửi lại hoặc hệ điều hành tự kích hoạt cơ chế gõ lặp (Auto-repeat).
3. **Khi buông tay**: **BẮT BUỘC PHẢI GỬI BÁO CÁO GIÁ TRỊ 0 (RELEASE)** để báo cho OS biết phím đã được nhả ra. Nếu quên gửi báo cáo 0, OS sẽ hiểu là bạn đang ấn giữ phím đó mãi mãi (hiện tượng kẹt phím/stuck key).

---

## 3. Bàn Phím (Keyboard HID)

### 3.1. Cấu trúc gói tin chuẩn (Standard 8-byte 6KRO Report)
Bàn phím USB/BLE HID tiêu chuẩn sử dụng báo cáo 8 bytes, hỗ trợ bấm cùng lúc tối đa 6 phím thường + 8 phím bổ trợ (6-Key Rollover):

| Byte Offset | Trường Dữ Liệu | Kiểu Dữ Liệu | Mô Tả |
|:---:|:---|:---:|:---|
| **Byte 0** | Modifier Keys | `uint8_t` (Bitmask) | Trạng thái các phím Ctrl, Shift, Alt, GUI (Win/Cmd) |
| **Byte 1** | Reserved | `uint8_t` | Byte dự phòng (luôn bằng `0x00`) |
| **Byte 2** | Key Array [0] | `uint8_t` | Mã Usage Code phím thứ 1 đang nhấn |
| **Byte 3** | Key Array [1] | `uint8_t` | Mã Usage Code phím thứ 2 đang nhấn |
| **Byte 4** | Key Array [2] | `uint8_t` | Mã Usage Code phím thứ 3 đang nhấn |
| **Byte 5** | Key Array [3] | `uint8_t` | Mã Usage Code phím thứ 4 đang nhấn |
| **Byte 6** | Key Array [4] | `uint8_t` | Mã Usage Code phím thứ 5 đang nhấn |
| **Byte 7** | Key Array [5] | `uint8_t` | Mã Usage Code phím thứ 6 đang nhấn |

### 3.2. Bitmask của Modifier Keys (Byte 0)
Mỗi bit trong Byte 0 đại diện cho một phím đặc biệt:
- `Bit 0 (0x01)`: **Left Control**
- `Bit 1 (0x02)`: **Left Shift**
- `Bit 2 (0x04)`: **Left Alt**
- `Bit 3 (0x08)`: **Left GUI** (Windows Key / Command trên Mac)
- `Bit 4 (0x10)`: **Right Control**
- `Bit 5 (0x20)`: **Right Shift**
- `Bit 6 (0x40)`: **Right Alt** (AltGr)
- `Bit 7 (0x80)`: **Right GUI**

*Ví dụ tổ hợp `Ctrl + Shift + C`*:
`Byte 0 = 0x01 | 0x02 = 0x03`, `Byte 2 = 0x06` (chữ C), các byte còn lại = `0x00`.

### 3.3. Các mã HID Usage Code phổ biến (Page 0x07)
| Ký Tự / Phím | HID Code (Hex) | Ký Tự / Phím | HID Code (Hex) | Ký Tự / Phím | HID Code (Hex) |
|:---|:---:|:---|:---:|:---|:---:|
| **A..Z** | `0x04 .. 0x1D` | **SPACEBAR** | `0x2C` | **UP ARROW** | `0x52` |
| **1..9, 0** | `0x1E .. 0x27` | **TAB** | `0x2B` | **DOWN ARROW** | `0x51` |
| **ENTER** | `0x28` | **BACKSPACE** | `0x2A` | **LEFT ARROW** | `0x50` |
| **ESCAPE** | `0x29` | **CAPS LOCK** | `0x39` | **RIGHT ARROW** | `0x4F` |
| **F1 .. F12** | `0x3A .. 0x45` | **PAGE UP** | `0x4B` | **PAGE DOWN** | `0x4E` |

---

## 4. Chuột (Mouse HID)

### 4.1. Cấu trúc gói tin chuột tiêu chuẩn
Chuột gửi tọa độ tương đối (Relative Delta - độ dời vị trí so với lần báo cáo trước, không phải tọa độ tuyệt đối trên màn hình):

| Byte Offset | Trường Dữ Liệu | Kiểu Dữ Liệu | Dải Giá Trị | Mô Tả |
|:---:|:---|:---:|:---:|:---|
| **Byte 0** | Buttons Bitmask | `uint8_t` | `0x00 .. 0x1F` | Các nút chuột đang nhấn |
| **Byte 1** | X Delta ($dx$) | `int8_t` | `-127 .. +127` | Dịch chuyển ngang (Âm: Sang trái, Dương: Sang phải) |
| **Byte 2** | Y Delta ($dy$) | `int8_t` | `-127 .. +127` | Dịch chuyển dọc (Âm: Lên trên, Dương: Xuống dưới) |
| **Byte 3** | Wheel ($dz$) | `int8_t` | `-127 .. +127` | Con lăn cuộn dọc (Dương: Cuộn lên, Âm: Cuộn xuống) |

### 4.2. Bitmask của Buttons (Byte 0)
- `Bit 0 (0x01)`: **Left Button (Chuột trái)**
- `Bit 1 (0x02)`: **Right Button (Chuột phải)**
- `Bit 2 (0x04)`: **Middle Button (Chuột giữa / Nút con lăn)**
- `Bit 3 (0x08)`: **Backward Button (Nút lùi trang web)**
- `Bit 4 (0x10)`: **Forward Button (Nút tiến trang web)**

### 4.3. Kỹ thuật điều khiển chuột mượt bằng Analog Joystick
Trên kit ESP32-S3 này, chuột được tích hợp các giải thuật cao cấp:
1. **Biên ổn định (Configurable Deadzone $\pm 50/255$)**:
   - Khi cần gạt nằm trong vùng nghỉ quanh tâm 0, chuột đứng yên tuyệt đối.
2. **Cơ chế chống trôi khi nhả cần (Return-to-Home Freeze)**:
   - Khi phát hiện khoảng cách tới tâm đang co lại ($|\text{pos}_t| < |\text{pos}_{t-1}| - 3$): Chuột lập tức khóa đứng yên ($dx=0, dy=0$). Nhờ đó khi buông ngón tay ra, lò xo cơ khí kéo cần gạt về tâm sẽ không làm chuột bị trôi thêm hoặc giật nảy.
3. **Bộ tích lũy phần thập phân (Sub-pixel Accumulator)**:
   - Tốc độ di chuyển có thể là số lẻ (ví dụ 0.35 pixel/tick). Bộ tích lũy cộng dồn phần thập phân qua các tick quét 10ms (100Hz), khi đủ $\ge 1.0$ sẽ đẩy sang hệ điều hành 1 pixel, mang lại cảm giác rê chuột vi mô chuẩn xác từng pixel như ThinkPad TrackPoint.

---

## 5. Tay Cầm Chơi Game (Gamepad & Joystick HID)

### 5.1. DInput (DirectInput) vs XInput (Xbox)
- **DirectInput (Chuẩn HID gốc)**: Được định nghĩa bởi USB HID chuẩn. Hoạt động trên mọi thiết bị Linux, Mac, Android, Windows.
- **XInput**: Giao thức độc quyền của Microsoft cho Xbox Controller. Trên Windows, nhiều game yêu cầu XInput để rung hoặc hỗ trợ phím Xbox native.
- **Dự án LaunchPad của chúng ta**: Sử dụng **Standard HID Gamepad Descriptor với cấu trúc chuẩn Xbox Controller** (2 cần analog trái/phải, 2 cò analog LT/RT, D-Pad Hat Switch, 16 nút nhấn).

### 5.2. Cấu trúc gói tin Gamepad
| Byte Offset | Tên Trường | Định Dạng | Mô Tả |
|:---:|:---|:---:|:---|
| **Byte 0..1** | Left Stick X | `uint16_t` (0..65535, giữa 32768) | Trục ngang cần trái (Trái $\rightarrow$ Phải) |
| **Byte 2..3** | Left Stick Y | `uint16_t` (0..65535, giữa 32768) | Trục dọc cần trái (Lên $\rightarrow$ Xuống) |
| **Byte 4..5** | Right Stick X | `uint16_t` (0..65535, giữa 32768) | Trục ngang cần phải |
| **Byte 6..7** | Right Stick Y | `uint16_t` (0..65535, giữa 32768) | Trục dọc cần phải |
| **Byte 8** | Left Trigger (LT / L2) | `uint8_t` (0..255) | Cò analog trái (độ sâu nhấn phanh) |
| **Byte 9** | Right Trigger (RT / R2) | `uint8_t` (0..255) | Cò analog phải (độ sâu nhấn ga) |
| **Byte 10..11**| Gamepad Buttons | `uint16_t` (Bitmask) | 16 nút bấm chính |
| **Byte 12** | D-Pad (Hat Switch) | `uint8_t` (0..8) | Phím điều hướng 8 hướng (0: Giữa, 1: Lên, 2: Lên-Phải...) |

### 5.3. Bảng Bitmask Nút Gamepad
- `Bit 0 (0x0001)`: **Nút A** (Cross - Xanh lá)
- `Bit 1 (0x0002)`: **Nút B** (Circle - Đỏ)
- `Bit 2 (0x0004)`: **Nút X** (Square - Xanh dương)
- `Bit 3 (0x0008)`: **Nút Y** (Triangle - Vàng)
- `Bit 4 (0x0010)`: **Nút L1 / LB** (Left Bumper)
- `Bit 5 (0x0020)`: **Nút R1 / RB** (Right Bumper)
- `Bit 6 (0x0040)`: **Nút Back / View / Select**
- `Bit 7 (0x0080)`: **Nút Start / Menu**
- `Bit 8 (0x0100)`: **Nút Xbox Guide / Home**
- `Bit 9 (0x0200)`: **Nút L3 / THUMBL** (Nhấn cần trái)
- `Bit 10 (0x0400)`: **Nút R3 / THUMBR** (Nhấn cần phải)

---

## 6. Điều Khiển Đa Phương Tiện (Consumer Control / Media Keys)

### 6.1. Đặc điểm của Consumer Control
Consumer Control sử dụng **HID Usage Page 0x0C (Consumer Device)**. Đây là loại báo cáo điều khiển các chức năng toàn hệ thống độc lập với cửa sổ phần mềm đang mở:
- Tăng / Giảm âm lượng hệ thống (**Volume Up / Volume Down**)
- Tắt tiếng (**Mute**)
- Phát / Tạm dừng nhạc (**Play / Pause**)
- Bài kế tiếp / Bài trước đó (**Next / Previous Track**)
- Tăng / Giảm độ sáng màn hình (**Brightness Up / Down**)
- Mở ứng dụng nhanh: Máy tính (Calculator), Trình duyệt (Browser), Email, v.v.

### 6.2. Cấu trúc gói tin Consumer Control trong dự án
Báo cáo sử dụng `Report ID = 4`, dữ liệu gồm 1 byte bitmask:

| Bit | Hằng Số Macro | Ý Nghĩa Chức Năng |
|:---:|:---|:---|
| **Bit 0 (0x01)** | `CONSUMER_VOLUME_UP` | Tăng âm lượng hệ thống |
| **Bit 1 (0x02)** | `CONSUMER_VOLUME_DOWN` | Giảm âm lượng hệ thống |
| **Bit 2 (0x04)** | `CONSUMER_MUTE` | Bật / Tắt âm thanh (Mute) |
| **Bit 3 (0x08)** | `CONSUMER_PLAY_PAUSE` | Phát nhạc / Tạm dừng |
| **Bit 4 (0x10)** | `CONSUMER_SCAN_NEXT` | Chuyển bài hát kế tiếp (Next Track) |
| **Bit 5 (0x20)** | `CONSUMER_SCAN_PREV` | Quay lại bài hát trước (Previous Track) |

> [!IMPORTANT]
> **Quy tắc nhả phím**: Khi nút nhấn vật lý được bấm, ESP32 gửi mã bitmask tương ứng (ví dụ `0x02` cho Volume Down). Khi nút được buông ra, ESP32 **bắt buộc gửi `0x00`** để kết thúc lệnh. Bộ điều khiển trong `launchpad_core.c` đã xử lý tự động cơ chế này.

---

## 7. Các Loại Thiết Bị HID Mở Rộng Khác

Ngoài 4 loại chính trên, chuẩn HID còn quy định nhiều thiết bị chuyên dụng:

### 7.1. Bút Cảm Ứng & Bảng Vẽ (Digitizer / Pen Tablet)
- Sử dụng tọa độ tuyệt đối ($X, Y$ từ 0 đến hàng chục nghìn điểm ảnh).
- Gửi thêm thông tin **Lực nhấn (Tip Pressure)** (ví dụ 4096 hoặc 8192 mức áp lực), góc nghiêng ngòi bút (Tilt X, Tilt Y), nút xóa (Eraser) và nút trên thân bút (Barrel Button).

### 7.2. Màn Hình Cảm Ứng Đa Điểm (Multi-touch Touchscreen)
- Hỗ trợ gửi báo cáo cùng lúc nhiều ngón tay chạm (Contact ID 0..9).
- Mỗi điểm chạm gửi trạng thái: Nhấn xuống (In Range / Tip Switch), tọa độ $X, Y$, diện tích tiếp xúc (Width, Height).

### 7.3. Điều Khiển Nguồn Hệ Thống (System Control - Page 0x01)
- Cho phép gửi các lệnh phần cứng trực tiếp tới bo mạch chủ PC:
  - `System Power Down`: Tắt máy tính.
  - `System Sleep`: Đưa máy vào chế độ ngủ (Sleep/Suspend).
  - `System Wake Up`: Đánh thức máy tính từ xa qua Bluetooth/USB.

### 7.4. HID Độc Quyền (Vendor-Defined Generic HID / Raw HID)
- Sử dụng Usage Page do nhà sản xuất tự định nghĩa (`0xFF00 .. 0xFFFF`).
- **Ưu điểm cực lớn**: Cho phép truyền nhận các gói tin nhị phân tùy ý (64 bytes/gói) giữa phần mềm PC và MCU mà **không cần cài đặt driver WinUSB hay driver COM port**.
- Rất phổ biến trên các bàn phím Custom cao cấp (như QMK/VIA để nạp phím on-the-fly) hoặc đồng bộ dữ liệu PC Sync.

---

## 8. Kiến Trúc Triển Khai Trong Dự Án LaunchPad ESP32-S3

Dự án này sử dụng kiến trúc **BLE HID Composite Device** (1 kết nối BLE phục vụ đồng thời cả 4 vai trò):

```
┌────────────────────────────────────────────────────────────────────────┐
│                        LAUNCHPAD HID CORE ARCHITECTURE                 │
└────────────────────────────────────────────────────────────────────────┘
                                   │
       ┌───────────────────────────┴───────────────────────────┐
       ▼                                                       ▼
[GAMEPAD MODE 🎮]                                     [KB & MOUSE MODE ⌨️🖱️]
- Joystick VRx/VRy: Left Stick                        - Joystick: Chuột TrackPoint
- Joystick SW:      Nút A                             - Joystick SW: Click Chuột Trái
- Nút 1:            Nút A                             - Nút 1: SPACEBAR
- Nút 2:            Nút B                             - Nút 2: ENTER
- Nút 3:            Nút X                             - Nút 3: Phím 'W'
- Nút 4:            Nút Y                             - Nút 4: Phím 'S'
- Nút 5:            Bumper L1                         - Nút 5: Giảm Âm Lượng 🔉
- Nút 6:            Bumper R1                         - Nút 6: Tăng Âm Lượng 🔊
```

### Sơ đồ chân kết nối phần cứng (Hardware Pinout)
| Nút Nhấn Vật Lý | Chân GPIO ESP32-S3 | Cụm Chân Header | Trạng Thái Điện Áp | Chức Năng Hiện Tại |
|:---|:---:|:---:|:---:|:---|
| **JOY_VRX** | `GPIO 1` | J3 - Pin 4 | ADC1_CH0 (0..4095) | Trục Y Chuột / Gamepad |
| **JOY_VRY** | `GPIO 2` | J3 - Pin 5 | ADC1_CH1 (0..4095) | Trục X Chuột / Gamepad |
| **JOY_SW**  | `GPIO 42`| J3 - Pin 6 | Pull-up nội | **Click Chuột Trái (Select)** / Nút A |
| **BTN_1**   | `GPIO 4` | J1 - Pin 4 | Pull-up nội | Phím **SPACEBAR** / Nút A |
| **BTN_2**   | `GPIO 5` | J1 - Pin 5 | Pull-up nội | Phím **ENTER** / Nút B |
| **BTN_3**   | `GPIO 6` | J1 - Pin 6 | Pull-up nội | Phím **W** / Nút X |
| **BTN_4**   | `GPIO 7` | J1 - Pin 7 | Pull-up nội | Phím **S** / Nút Y |
| **BTN_5**   | `GPIO 41`| J3 - Pin 7 | Pull-up nội | **GIẢM ÂM LƯỢNG (Volume Down)** / L1 |
| **BTN_6**   | `GPIO 40`| J3 - Pin 8 | Pull-up nội | **TĂNG ÂM LƯỢNG (Volume Up)** / R1 |
| **BOOT_BTN**| `GPIO 0` | Trên board | Pull-up nội | Nhấn 2s: Đổi Mode — Nhấn 6s: Xóa Pairing |

---

## 9. Hướng Dẫn Tùy Biến Cấu Hình Nút Nhấn & Joystick

Mọi cấu hình đều được tập trung duy nhất tại file:  
👉 [`components/launchpad_core/include/input_config.h`](file:///home/hungdk4/Documents/Code/for-me/lanchPad/esp_lanchpad_hid/components/launchpad_core/include/input_config.h)

### 9.1. Đổi chức năng nút nhấn
Mỗi nút trong mảng `BUTTON_CONFIGS[]` có cấu trúc:
```c
{
    .name           = "BTN_5",
    .gpio           = PIN_BTN_5,
    .gamepad_button = XBOX_BUTTON_L1,            // Chức năng khi ở Gamepad Mode
    .gamepad_name   = "Xbox Bumper L1",
    .kbm_type       = ACTION_TYPE_CONSUMER,      // Kiểu hành động ở KB/Mouse Mode
    .kbm_code       = CONSUMER_VOLUME_DOWN,      // Mã hành động
    .kbm_modifier   = 0,                         // Phím bổ trợ (Ctrl/Shift) nếu là Keyboard
    .kbm_name       = "Media Volume DOWN",
}
```

Các giá trị `kbm_type` bạn có thể chọn:
1. `ACTION_TYPE_KEYBOARD`: Phím bàn phím (`kbm_code` là `HID_KEY_...`, có thể kèm `kbm_modifier` như `KEY_MOD_LCTRL`).
2. `ACTION_TYPE_MOUSE_BTN`: Nút chuột (`MOUSE_BUTTON_LEFT`, `MOUSE_BUTTON_RIGHT`, `MOUSE_BUTTON_MIDDLE`).
3. `ACTION_TYPE_CONSUMER`: Phím Media (`CONSUMER_VOLUME_UP`, `CONSUMER_VOLUME_DOWN`, `CONSUMER_MUTE`, `CONSUMER_PLAY_PAUSE`, `CONSUMER_SCAN_NEXT`, `CONSUMER_SCAN_PREV`).

### 9.2. Tinh chỉnh Joystick & Chuột
```c
#define JOY_SWAP_XY              true   // Hoán đổi trục X và Y
#define JOY_INVERT_X             false  // Đảo chiều trục X
#define JOY_INVERT_Y             true   // Đảo chiều trục Y (chiều lên <-> xuống)

#define JOY_MOUSE_DEADZONE       50     // Biên ổn định gốc 0 (+-50/255)
#define JOY_MOUSE_MAX_SPEED      15.0f  // Tốc độ chuột tối đa khi gạt hết cỡ (px/tick)
#define JOY_MOUSE_RETURN_THRESH  3      // Ngưỡng phát hiện buông cần về home để khóa chuột đứng yên
```

Sau khi sửa file cấu hình, bạn chỉ cần biên dịch và nạp lại vào kit:
```bash
source /home/hungdk4/.espressif/v5.3.5/esp-idf/export.sh
idf.py -p /dev/ttyACM0 flash
```
