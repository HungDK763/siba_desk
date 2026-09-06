# 📌 BẢN ĐỒ ĐỊNH NGHĨA CHÂN GPIO & HƯỚNG DẪN ĐẤU NỐI PHẦN CỨNG
> **Dự án:** AI Developer Desk Console & Universal Gateway
> **Chip vi điều khiển:** ESP32-S3 (8MB Flash, 8MB Octal PSRAM)
> **Cập nhật:** Đã thay thế Rotary Encoder bằng **Analog Joystick 2 trục (VRx, VRy, SW)**.

---

## 1. BẢNG TỔNG HỢP TOÀN BỘ CHÂN GPIO

| GPIO | Tên tín hiệu | Khối ngoại vi | Chiều I/O | Chức năng phần cứng | Mức điện áp | Ghi chú kỹ thuật |
| :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **GPIO 0** | `BOOT_KEY` | Onboard Button | Input | GPIO | 3.3V | Nút BOOT có sẵn trên mạch (Kéo trở lên nội) |
| **GPIO 1** | `JOY_VRX` | Analog Joystick | Input | **ADC1_CH0** | 0 - 3.3V | Đọc điện áp trục ngang X (0 - 4095) |
| **GPIO 2** | `JOY_VRY` | Analog Joystick | Input | **ADC1_CH1** | 0 - 3.3V | Đọc điện áp trục dọc Y (0 - 4095) |
| **GPIO 3** | `JOY_SW` | Analog Joystick | Input | GPIO | 3.3V | Nút nhấn cần gạt Thumbstick (Nhấn thẳng xuống) |
| **GPIO 4** | `MACRO_KEY1` | Phím cơ Macro | Input | GPIO | 3.3V | Phím tắt 1 (Kéo trở Pull-up nội, bấm nối GND) |
| **GPIO 5** | `MACRO_KEY2` | Phím cơ Macro | Input | GPIO | 3.3V | Phím tắt 2 (Kéo trở Pull-up nội, bấm nối GND) |
| **GPIO 6** | `MACRO_KEY3` | Phím cơ Macro | Input | GPIO | 3.3V | Phím tắt 3 (Kéo trở Pull-up nội, bấm nối GND) |
| **GPIO 7** | `MACRO_KEY4` | Phím cơ Macro | Input | GPIO | 3.3V | Phím tắt 4 (Kéo trở Pull-up nội, bấm nối GND) |
| **GPIO 8** | `SPARE_1` | **DỰ PHÒNG 1** | - | GPIO / ADC1 | 3.3V | **Chân trống hoàn toàn** (Có thể dùng làm Buzzer/Còi/Pin) |
| **GPIO 9** | `LCD_DC` | Màn hình TFT 2.8" | Output | GPIO | 3.3V | Chọn Data / Command cho màn hình LCD |
| **GPIO 10**| `LCD_CS` | Màn hình TFT 2.8" | Output | SPI2_CS0 | 3.3V | Chip Select màn hình LCD (Tích cực mức Thấp) |
| **GPIO 11**| `SPI_MOSI` | SPI Chung (Bus 2)| Output | SPI2_MOSI| 3.3V | Đường truyền dữ liệu chung cho LCD & Touch |
| **GPIO 12**| `SPI_SCK` | SPI Chung (Bus 2)| Output | SPI2_CLK | 3.3V | Xung Clock chung cho LCD & Touch (Up to 40MHz)|
| **GPIO 13**| `SPI_MISO` | SPI Chung (Bus 2)| Input | SPI2_MISO| 3.3V | Nhận dữ liệu cảm ứng Touch XPT2046 về ESP32 |
| **GPIO 14**| `LCD_RST` | Màn hình TFT 2.8" | Output | GPIO | 3.3V | Reset phần cứng màn hình LCD |
| **GPIO 15**| `PS2_DATA` | Cổng tay cầm PS2 | Input | GPIO | 3.3V | Nhận dữ liệu từ tay PS2 (**Bắt buộc trở kéo 10k lên 3.3V**)|
| **GPIO 16**| `PS2_CMD` | Cổng tay cầm PS2 | Output | GPIO | 3.3V | Gửi lệnh từ ESP32 sang tay cầm PS2 |
| **GPIO 17**| `PS2_ATT` | Cổng tay cầm PS2 | Output | GPIO | 3.3V | Attention / Chip Select tay cầm PS2 |
| **GPIO 18**| `PS2_CLK` | Cổng tay cầm PS2 | Output | GPIO | 3.3V | Xung Clock đồng bộ với tay cầm PS2 |
| **GPIO 19**| `USB_D-` | USB Native | Bi-dir | USB D- | 3.3V | Cổng USB OTG / Serial / JTAG nạp code & cấp nguồn |
| **GPIO 20**| `USB_D+` | USB Native | Bi-dir | USB D+ | 3.3V | Cổng USB OTG / Serial / JTAG nạp code & cấp nguồn |
| **GPIO 21**| `LCD_BL` | Màn hình TFT 2.8" | Output | LEDC/PWM | 3.3V | Điều khiển đèn nền màn hình (PWM chỉnh độ sáng) |
| **GPIO 38**| `TOUCH_CS` | Cảm ứng Touch | Output | GPIO | 3.3V | Chip Select cảm ứng XPT2046 |
| **GPIO 39**| `TOUCH_IRQ`| Cảm ứng Touch | Input | GPIO | 3.3V | Ngắt khi có ngón tay chạm màn hình |
| **GPIO 40**| `RS485_TX` | Module RS485 | Output | UART1_TX | 3.3V | UART1 TX truyền dữ liệu sang MAX485 (chân DI) |
| **GPIO 41**| `RS485_RX` | Module RS485 | Input | UART1_RX | 3.3V | UART1 RX nhận dữ liệu từ MAX485 (chân RO) |
| **GPIO 42**| `RS485_DIR`| Module RS485 | Output | GPIO | 3.3V | Điều khiển hướng DE/RE (High: TX, Low: RX) |
| **GPIO 45**| `SPARE_2` | **DỰ PHÒNG 2** | - | GPIO | 3.3V | **Chân trống hoàn toàn** (User GPIO) |
| **GPIO 46**| `SPARE_3` | **DỰ PHÒNG 3** | Input | GPIO | 3.3V | **Chân trống hoàn toàn** (Input-only, Boot ROM log) |
| **GPIO 47**| `SPARE_4` | **DỰ PHÒNG 4** | - | GPIO | 3.3V | **Chân trống hoàn toàn** (User GPIO) |
| **GPIO 48**| `WS2812_LED`| Onboard RGB LED | Output | RMT | 3.3V | Đèn LED RGB địa chỉ WS2812 có sẵn trên kit |

---

## 2. 📊 THỐNG KÊ TỔNG SỐ CHÂN & CHÂN CÒN DƯ

* **Tổng chân GPIO khả dụng trên ESP32-S3 (bản 8MB PSRAM):** 28 chân
* **Tổng chân ngoại vi đang sử dụng:** **22 chân**
  * Màn hình TFT 2.8" & Touch: **8 chân** (`GPIO 9, 10, 11, 12, 13, 14, 21, 38, 39` - trong đó 11, 12, 13 dùng chung)
  * Analog Joystick: **3 chân** (`GPIO 1, 2, 3`)
  * 4 Phím cơ Macro: **4 chân** (`GPIO 4, 5, 6, 7`)
  * Cổng tay cầm PS2: **4 chân** (`GPIO 15, 16, 17, 18`)
  * Cổng chờ RS485: **3 chân** (`GPIO 40, 41, 42`)
* **Chân tích hợp trên mạch (Onboard):** 2 chân (`GPIO 0` BOOT, `GPIO 48` WS2812 LED)
* **🎉 SỐ CHÂN DỰ PHÒNG CÒN DƯ:** **4 CHÂN HOÀN TOÀN TRỐNG**
  * `GPIO 8` *(Đặc biệt hỗ trợ cả ADC1_CH7)*
  * `GPIO 45`
  * `GPIO 46` *(Input only)*
  * `GPIO 47`
  *(Nếu sau này bạn dùng module RS485 loại tự động chuyển hướng Auto-Direction thì sẽ bỏ được chân GPIO 42, lúc đó sẽ **dư tới 5 chân GPIO**!)*

---

## 3. ⚠️ CÁC CHÂN CẤM SỬ DỤNG TRÊN ESP32-S3

Do phiên bản chip của bạn tích hợp sẵn **8MB Flash** và **8MB Octal PSRAM**, các chân sau được kết nối ngầm bên trong module và **TUYỆT ĐỐI KHÔNG ĐƯỢC HÀN/ĐẤU NỐI NGOẠI VI**:
* **GPIO 26 đến GPIO 32**: Dành riêng cho giao tiếp Octal/Quad SPI Flash nội.
* **GPIO 33 đến GPIO 37**: Dành riêng cho bộ nhớ mở rộng Octal PSRAM 8MB nội.
* **GPIO 43, 44**: Đang làm UART0 Debug Console mặc định kết nối máy tính.

---

## 4. HƯỚNG DẪN ĐẤU NỐI CHI TIẾT TỪNG NGOẠI VI

### 4.1. Cần gạt Analog Joystick (Thumbstick 2 trục + Nút nhấn)
Module cần gạt Joystick tiêu chuẩn (loại như tay cầm PS2 / Arduino KY-023) có 5 chân:

```
+-------------------+--------------------+-----------------------------------------------+
| Chân trên Joystick| Chân trên ESP32-S3 | Ghi chú & Chức năng                           |
+-------------------+--------------------+-----------------------------------------------+
| GND               | GND                | Nối Mass chung                                |
| +5V / VCC         | 3.3V               | BẮT BUỘC NỐI 3.3V để dải điện áp ADC là 0-3.3V|
| VRx               | GPIO 1 (ADC1_CH0)  | Trục ngang X: Giữa = 1.65V (2048), Trái=0V, Phải=3.3V |
| VRy               | GPIO 2 (ADC1_CH1)  | Trục dọc Y: Giữa = 1.65V (2048), Lên=0V, Xuống=3.3V   |
| SW                | GPIO 3             | Nút nhấn cần gạt (Bật trở kéo nội, bấm nối GND)|
+-------------------+--------------------+-----------------------------------------------+
```
> [!TIP]
> `GPIO 1` và `GPIO 2` thuộc khối **ADC1** của ESP32-S3. Khối ADC1 hoạt động hoàn toàn độc lập, không bị ảnh hưởng hay xung đột khi bật Wi-Fi / Bluetooth!

---

### 4.2. Màn hình 2.8" TFT SPI (ILI9341 / ST7789) & Cảm ứng (XPT2046)

```
+-------------------+--------------------+-----------------------------------------------+
| Chân trên Module  | Chân trên ESP32-S3 | Ghi chú & Chức năng                           |
+-------------------+--------------------+-----------------------------------------------+
| VCC               | 3.3V hoặc 5V       | Khuyên dùng 5V nếu mạch có IC ổn áp 3.3V AMS  |
| GND               | GND                | Nối chung Mass                                |
| CS (LCD)          | GPIO 10            | Chip Select màn hình hiển thị                 |
| RESET             | GPIO 14            | Reset màn hình                                |
| DC / RS           | GPIO 9             | Data / Command select                         |
| SDI (MOSI)        | GPIO 11            | Đường truyền dữ liệu SPI                      |
| SCK (CLK)         | GPIO 12            | Đường xung nhịp SPI                           |
| LED (BL)          | GPIO 21            | Đèn nền (hoặc nối thẳng 3.3V nếu sáng cố định)|
| SDO (MISO)        | GPIO 13            | Đường nhận dữ liệu SPI                        |
| T_CLK             | GPIO 12            | Nối chung với chân SCK của màn hình           |
| T_CS              | GPIO 38            | Chip Select bộ cảm ứng XPT2046                |
| T_DIN             | GPIO 11            | Nối chung với chân SDI/MOSI của màn hình      |
| T_DO              | GPIO 13            | Nối chung với chân SDO/MISO của màn hình      |
| T_IRQ             | GPIO 39            | Báo ngắt khi có chạm cảm ứng                  |
+-------------------+--------------------+-----------------------------------------------+
```

---

### 4.3. Cụm 4 Phím cơ Macro (Tactile / Mechanical Switches)
* **Chân 1 của 4 phím**: Nối chung với nhau rồi nối vào **GND**.
* **Chân 2 của từng phím**:
  * Phím 1 -> **GPIO 4**
  * Phím 2 -> **GPIO 5**
  * Phím 3 -> **GPIO 6**
  * Phím 4 -> **GPIO 7**
*(Bên trong ESP32 sẽ bật trở kéo lên nội `GPIO_PULLUP_ENABLE`, khi nhấn phím sẽ kéo chân về mức 0V).*

---

### 4.4. Cổng cắm tay cầm PS2 (PlayStation 2 Controller)

```
Sơ đồ nhìn trực diện vào đầu cắm tay cầm PS2:
    -----------------------------------
PIN: | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
     -----------------------------------
     (1: DATA, 2: COMMAND, 4: GND, 5: VCC, 6: ATT, 7: CLOCK)
```

```
+-------------------+--------------------+-----------------------------------------------+
| Chân đầu cắm PS2  | Chân trên ESP32-S3 | Ghi chú kỹ thuật                              |
+-------------------+--------------------+-----------------------------------------------+
| Pin 1: DATA       | GPIO 15            | BẮT BUỘC: Hàn 1 trở 10k kéo từ chân này lên 3.3V|
| Pin 2: COMMAND    | GPIO 16            | Tín hiệu lệnh gửi sang tay                    |
| Pin 3: VIBRATION  | Nối nguồn 7-9V     | (Tùy chọn) Chỉ cần nếu muốn bật motor rung    |
| Pin 4: GND        | GND                | Nối Mass chung                                |
| Pin 5: VCC        | 3.3V               | Nguồn cấp cho chip xử lý tay cầm (3.3V)       |
| Pin 6: ATTENTION  | GPIO 17            | Chân chọn thiết bị (CS)                       |
| Pin 7: CLOCK      | GPIO 18            | Xung đồng bộ truyền tin                       |
| Pin 8: N/C        | Để trống           | Không sử dụng                                 |
| Pin 9: ACK        | Để trống           | Không cần dùng                                |
+-------------------+--------------------+-----------------------------------------------+
```

---

### 4.5. Module giao tiếp RS485 (MAX485 / SP3485)

```
+-------------------+--------------------+-----------------------------------------------+
| Chân trên MAX485  | Chân trên ESP32-S3 | Ghi chú                                       |
+-------------------+--------------------+-----------------------------------------------+
| VCC               | 3.3V (hoặc 5V)     | Nếu dùng module SP3485 dùng 3.3V, MAX485 dùng 5V|
| GND               | GND                | Nối Mass chung                                |
| DI (Driver Input) | GPIO 40 (TX1)      | Dữ liệu truyền đi từ ESP32                    |
| RO (Recv Output)  | GPIO 41 (RX1)      | Dữ liệu nhận về ESP32                         |
| DE & RE (Nối chập)| GPIO 42            | Điều khiển hướng truyền (1: Phát, 0: Nhận)    |
| A & B             | Ra giắc vặn ốc     | Cặp dây xoắn RS485 dẫn ra thiết bị ngoài      |
+-------------------+--------------------+-----------------------------------------------+
```

---

## 5. BẢNG PHÂN CHIA NGUỒN CẤP (POWER DISTRIBUTION)

* **Nguồn 5V USB Type-C:** Cấp trực tiếp cho chân VCC của Màn hình (nếu màn hình có IC 3.3V trên bo) và cấp cho module RS485.
* **Nguồn 3.3V từ ESP32-S3:** Cấp cho Joystick (để đọc ADC chuẩn 0-3.3V), tay cầm PS2, và các chân kéo trở.
* **Chân GND:** Tất cả các module phải được nối chung một đường GND duy nhất (Star Grounding) để chống nhiễu ADC và SPI.
