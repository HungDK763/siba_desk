# 📌 BẢN ĐỒ ĐỊNH NGHĨA CHÂN GPIO & HƯỚNG DẪN ĐẤU NỐI PHẦN CỨNG
> **Dự án:** AI Developer Desk Console & Universal Gateway  
> **Chip vi điều khiển:** ESP32-S3 (8MB Flash, 8MB Octal PSRAM)  
> **Cập nhật mới:** Đã tối ưu hóa cụm chân **Màn hình TFT & Touch** thành **MỘT HÀNG THẲNG LIÊN TỤC 1:1** trên Header J1 theo đúng module phần cứng thực tế!

---

## 1. ⚠️ CẢNH BÁO AN TOÀN NGUỒN ĐIỆN (BẮT BUỘC ĐỌC)

> [!CAUTION]
> **ĐẢO CỰC NGUỒN 5V VÀ GND Ở ĐUÔI HEADER J1:**
> * Trên ESP32-S3 Header J1: Chân 21 là **5V**, Chân 22 là **GND** *(5V trước, GND sau)*.
> * Trên Module màn hình TFT: Chân 13 là **GND**, Chân 14 là **VCC** *(GND trước, VCC sau)*.
> 
> 👉 **Nếu cắm thẳng 1-1 không để ý sẽ bị chập nguồn / đảo cực gây cháy màn hình!**
> * **Khi làm mạch in (PCB):** Bắt chéo 2 đường nguồn (hoặc đổi chỗ 2 lỗ cắm VCC và GND).
> * **Khi cắm cáp bẹ:** Cắm thẳng 12 chân tín hiệu từ `GPIO 16` đến `GPIO 14`, riêng 2 sợi nguồn ở đuôi rút ra và cắm đúng: **VCC màn hình vào 5V**, **GND màn hình vào GND**.

---

## 2. 🔌 SƠ ĐỒ ĐẤU NỐI THẲNG HÀNG 1:1 CỤM MÀN HÌNH TFT & TOUCH

Toàn bộ **14 chân** của Module TFT 2.8" SPI (kèm cảm ứng XPT2046) được map khớp nối tiếp vào **Header J1 (Cạnh trái ESP32-S3)**:

```text
       HEADER J1 (ESP32-S3)                    MODULE TFT 2.8" SPI (XPT2046)
+-----------------------------------+     +-----------------------------------+
| Vị trí Pin | Tên Chân ESP32-S3    |     | Tên Chân In Trên Lưng Module TFT  |
+------------+----------------------+     +-----------------------------------+
| J1-9       | GPIO 16 (Touch IRQ)  | <-> | 1.  IRQ   (Ngắt cảm ứng)          |
| J1-10      | GPIO 17 (Touch DO)   | <-> | 2.  DO    (T_DO / MISO Touch)     |
| J1-11      | GPIO 18 (Touch DIN)  | <-> | 3.  DIN   (T_DIN / MOSI Touch)    |
| J1-12      | GPIO 8  (Touch CS)   | <-> | 4.  CS    (T_CS Touch Select)     |
| J1-13      | GPIO 3  (Touch CLK)  | <-> | 5.  CLK   (T_CLK Touch Clock)     |
| J1-14      | GPIO 46 (LCD MISO)   | <-> | 6.  MISO  (SDO LCD / Read data)   |
| J1-15      | GPIO 9  (LCD BL/LED) | <-> | 7.  LED   (Đèn nền LCD PWM)       |
| J1-16      | GPIO 10 (LCD SCK)    | <-> | 8.  SCK   (Xung nhịp SPI LCD)     |
| J1-17      | GPIO 11 (LCD MOSI)   | <-> | 9.  MOSI  (Dữ liệu SPI LCD)       |
| J1-18      | GPIO 12 (LCD DC)     | <-> | 10. DC    (Lệnh/Dữ liệu LCD)      |
| J1-19      | GPIO 13 (LCD RST)    | <-> | 11. RESET (Reset phần cứng LCD)   |
| J1-20      | GPIO 14 (LCD CS)     | <-> | 12. CS    (Chip Select LCD)       |
| J1-21      | 5V (Power)           | <X> | 13. GND   (Mass chung) [ĐẢO CHÉO] |
| J1-22      | GND (Ground)         | <X> | 14. VCC   (Nguồn 5V)   [ĐẢO CHÉO] |
+------------+----------------------+     +-----------------------------------+
```

---

## 3. 📊 BẢNG TỔNG HỢP TOÀN BỘ CHÂN GPIO HỆ THỐNG

### Header J1 (Cạnh Trái):
| Vị trí Pin | GPIO | Tên tín hiệu | Khối ngoại vi | Chiều I/O | Chức năng kỹ thuật |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **J1-1** | `3V3` | 3.3V Power | Nguồn | Output | Nguồn 3.3V cấp cho cảm biến / phím |
| **J1-2** | `3V3` | 3.3V Power | Nguồn | Output | Nguồn 3.3V dự phòng |
| **J1-3** | `EN` | Reset Pin | Hệ thống | Input | Nút Reset vi điều khiển |
| **J1-4** | **GPIO 4** | `MACRO_KEY1` | Phím cơ Macro | Input | Phím tắt 1 (Pullup nội, bấm nối GND) |
| **J1-5** | **GPIO 5** | `MACRO_KEY2` | Phím cơ Macro | Input | Phím tắt 2 (Pullup nội, bấm nối GND) |
| **J1-6** | **GPIO 6** | `MACRO_KEY3` | Phím cơ Macro | Input | Phím tắt 3 (Pullup nội, bấm nối GND) |
| **J1-7** | **GPIO 7** | `MACRO_KEY4` | Phím cơ Macro | Input | Phím tắt 4 (Pullup nội, bấm nối GND) |
| **J1-8** | **GPIO 15**| `SPARE_1` | **DỰ PHÒNG 1** | - | **Chân trống hoàn toàn** (Còi/Buzzer/LED) |
| **J1-9** | **GPIO 16**| `TOUCH_IRQ` | Cảm ứng Touch | Input | Ngắt khi có ngón tay chạm màn hình |
| **J1-10**| **GPIO 17**| `TOUCH_DO` | Cảm ứng Touch | Input | Đọc tọa độ chạm XPT2046 về ESP32 |
| **J1-11**| **GPIO 18**| `TOUCH_DIN` | Cảm ứng Touch | Output | Gửi lệnh đọc kênh X/Y sang XPT2046 |
| **J1-12**| **GPIO 8** | `TOUCH_CS` | Cảm ứng Touch | Output | Chip Select bộ cảm ứng XPT2046 |
| **J1-13**| **GPIO 3** | `TOUCH_CLK` | Cảm ứng Touch | Output | Xung nhịp SPI cho cảm ứng |
| **J1-14**| **GPIO 46**| `LCD_MISO` | Màn hình TFT | Input | Đọc dữ liệu / ID từ ILI9341 |
| **J1-15**| **GPIO 9** | `LCD_BL` | Màn hình TFT | Output | LEDC PWM điều khiển độ sáng màn hình |
| **J1-16**| **GPIO 10**| `SPI_CLK` | Màn hình TFT | Output | SPI Master Clock (40MHz) |
| **J1-17**| **GPIO 11**| `SPI_MOSI` | Màn hình TFT | Output | SPI Master Data Out |
| **J1-18**| **GPIO 12**| `LCD_DC` | Màn hình TFT | Output | Data / Command Select |
| **J1-19**| **GPIO 13**| `LCD_RST` | Màn hình TFT | Output | Reset phần cứng ILI9341 |
| **J1-20**| **GPIO 14**| `LCD_CS` | Màn hình TFT | Output | Chip Select màn hình hiển thị |
| **J1-21**| `5V` | 5V Power | Nguồn | Output | Nguồn 5V cấp cho VCC màn hình |
| **J1-22**| `GND` | Ground | Nguồn | - | Nối Mass chung hệ thống |

---

### Header J3 (Cạnh Phải):
| Vị trí Pin | GPIO | Tên tín hiệu | Khối ngoại vi | Chiều I/O | Chức năng kỹ thuật |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **J3-1** | `3V3` | 3.3V Power | Nguồn | Output | Nguồn 3.3V cấp cho Joystick & PS2 |
| **J3-2** | `5V` | 5V Power | Nguồn | Output | Nguồn 5V USB |
| **J3-3** | `GND` | Ground | Nguồn | - | Mass chung |
| **J3-4** | **GPIO 1** | `JOY_VRX` | Analog Joystick | Input | **ADC1_CH0** (Đọc trục ngang X: 0-3.3V) |
| **J3-5** | **GPIO 2** | `JOY_VRY` | Analog Joystick | Input | **ADC1_CH1** (Đọc trục dọc Y: 0-3.3V) |
| **J3-6** | **GPIO 42**| `JOY_SW` | Analog Joystick | Input | Nút nhấn cần gạt Thumbstick (Pullup) |
| **J3-7** | **GPIO 41**| `PS2_DATA` | Cổng tay cầm PS2 | Input | Nhận dữ liệu tay PS2 (**Trở kéo 10k lên 3.3V**)|
| **J3-8** | **GPIO 40**| `PS2_CMD` | Cổng tay cầm PS2 | Output | Gửi lệnh sang tay cầm PS2 |
| **J3-9** | **GPIO 39**| `PS2_ATT` | Cổng tay cầm PS2 | Output | Chip Select tay cầm PS2 |
| **J3-10**| **GPIO 38**| `PS2_CLK` | Cổng tay cầm PS2 | Output | Xung đồng bộ với tay cầm PS2 |
| **J3-15**| **GPIO 45**| `RS485_DIR`| Module RS485 | Output | Điều khiển hướng DE/RE (High: TX, Low: RX) |
| **J3-16**| **GPIO 48**| `WS2812_LED`| Onboard RGB LED | Output | LED RGB địa chỉ có sẵn trên kit ESP32-S3 |
| **J3-17**| **GPIO 47**| `RS485_TX` | Module RS485 | Output | UART1 TX truyền dữ liệu sang MAX485 |
| **J3-18**| **GPIO 21**| `RS485_RX` | Module RS485 | Input | UART1 RX nhận dữ liệu từ MAX485 |

---

## 4. ⚠️ CÁC CHÂN CẤM SỬ DỤNG TRÊN ESP32-S3 (OCTAL PSRAM)

Kit ESP32-S3 sử dụng chip tích hợp sẵn **8MB Flash** và **8MB Octal PSRAM**, các chân sau đấu ngầm bên trong chip, **TUYỆT ĐỐI KHÔNG ĐƯỢC HÀN / KẾT NỐI**:
* **GPIO 26 đến GPIO 32**: Dành riêng cho Octal SPI Flash nội.
* **GPIO 33 đến GPIO 37**: Dành riêng cho Octal PSRAM 8MB nội.
* **GPIO 43, 44**: Đang làm cổng nạp UART0 Debug Console mặc định.
* **GPIO 19, 20**: Cổng USB D- và D+ giao tiếp USB native máy tính.

---

## 5. HƯỚNG DẪN ĐẤU NỐI CHI TIẾT TỪNG NGOẠI VI

### 5.1. Cần gạt Analog Joystick (Thumbstick 2 trục + Nút nhấn)
Nằm trọn vẹn ở góc trên bên phải (**Header J3**):
```text
+--------------------+--------------------+-----------------------------------------------+
| Chân trên Joystick | Chân trên ESP32-S3 | Ghi chú & Chức năng                           |
+--------------------+--------------------+-----------------------------------------------+
| GND                | GND (J3-3)         | Nối Mass chung                                |
| +5V / VCC          | 3.3V (J3-1)        | BẮT BUỘC NỐI 3.3V để dải điện áp ADC là 0-3.3V|
| VRx                | GPIO 1 (J3-4)      | Trục ngang X: Giữa = 1.65V, Trái=0V, Phải=3.3V|
| VRy                | GPIO 2 (J3-5)      | Trục dọc Y: Giữa = 1.65V, Lên=0V, Xuống=3.3V  |
| SW                 | GPIO 42 (J3-6)     | Nút nhấn cần gạt (Bật trở kéo nội, bấm nối GND)|
+--------------------+--------------------+-----------------------------------------------+
```

### 5.2. Cụm 4 Phím cơ Macro
Nằm ở 4 chân đầu của **Header J1**:
* **Chân 1 của 4 phím**: Nối chung với nhau về **GND**.
* **Chân 2 của từng phím**:
  * Phím 1 -> **GPIO 4** (J1-4)
  * Phím 2 -> **GPIO 5** (J1-5)
  * Phím 3 -> **GPIO 6** (J1-6)
  * Phím 4 -> **GPIO 7** (J1-7)

### 5.3. Cổng cắm tay cầm PS2 (PlayStation 2 Controller)
Nằm ở cụm 4 chân liên tiếp trên **Header J3**:
```text
+-------------------+--------------------+-----------------------------------------------+
| Chân đầu cắm PS2  | Chân trên ESP32-S3 | Ghi chú kỹ thuật                              |
+-------------------+--------------------+-----------------------------------------------+
| Pin 1: DATA       | GPIO 41 (J3-7)     | BẮT BUỘC: Trở 10k kéo từ chân này lên 3.3V   |
| Pin 2: COMMAND    | GPIO 40 (J3-8)     | Tín hiệu lệnh gửi sang tay                    |
| Pin 4: GND        | GND (J3-3)         | Nối Mass chung                                |
| Pin 5: VCC        | 3.3V (J3-1)        | Nguồn cấp cho chip xử lý tay cầm (3.3V)       |
| Pin 6: ATTENTION  | GPIO 39 (J3-9)     | Chân chọn thiết bị (CS)                       |
| Pin 7: CLOCK      | GPIO 38 (J3-10)    | Xung đồng bộ truyền tin                       |
+-------------------+--------------------+-----------------------------------------------+
```

### 5.4. Cổng mở rộng RS485 (MAX485 / SP3485)
Nằm ở góc dưới của **Header J3**:
```text
+-------------------+--------------------+-----------------------------------------------+
| Chân trên MAX485  | Chân trên ESP32-S3 | Ghi chú                                       |
+-------------------+--------------------+-----------------------------------------------+
| VCC               | 5V (hoặc 3.3V)     | Nguồn cấp                                     |
| GND               | GND                | Mass chung                                    |
| DI (Driver Input) | GPIO 47 (J3-17)    | Dữ liệu truyền đi từ ESP32                    |
| RO (Recv Output)  | GPIO 21 (J3-18)    | Dữ liệu nhận về ESP32                         |
| DE & RE (Nối chập)| GPIO 45 (J3-15)    | Điều khiển hướng truyền (1: Phát, 0: Nhận)    |
| A & B             | Ra giắc vặn ốc     | Dây tín hiệu vi sai RS485                     |
+-------------------+--------------------+-----------------------------------------------+
```
