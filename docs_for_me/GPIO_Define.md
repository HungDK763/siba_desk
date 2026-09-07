# 📌 BẢN ĐỒ ĐỊNH NGHĨA CHÂN GPIO & HƯỚNG DẪN ĐẤU NỐI PHẦN CỨNG
> **Dự án:** AI Developer Desk Console & Universal LaunchPad  
> **Chip vi điều khiển:** ESP32-S3 (16MB Flash, 8MB Octal PSRAM)  
> **Phiên bản tài liệu:** v2.0 - Tổng hợp chuẩn chỉ Joystick & Cụm 6 Phím cơ  

---

## 1. ⚠️ CẢNH BÁO AN TOÀN NGUỒN ĐIỆN & CÁC CHÂN CẤM

### 1.1. Cảnh báo đảo cực nguồn màn hình TFT:
> [!CAUTION]
> **ĐẢO CỰC NGUỒN 5V VÀ GND Ở ĐUÔI HEADER J1:**
> * Trên ESP32-S3 Header J1: Chân 21 là **5V**, Chân 22 là **GND** *(5V trước, GND sau)*.
> * Trên Module màn hình TFT: Chân 13 là **GND**, Chân 14 là **VCC** *(GND trước, VCC sau)*.
> 
> 👉 **Nếu cắm thẳng 1:1 không để ý sẽ bị chập nguồn / đảo cực gây cháy màn hình!**
> * Rút riêng 2 sợi nguồn ở đuôi cáp bẹ và cắm đúng: **VCC màn hình vào 5V (J1-21)**, **GND màn hình vào GND (J1-22)**.

### 1.2. Các chân cấm tuyệt đối không được hàn / đấu nối:
Kit ESP32-S3 sử dụng chip tích hợp sẵn **8MB Octal Flash** và **8MB Octal PSRAM**, các chân sau đấu ngầm bên trong vi mạch:
* **GPIO 26 đến GPIO 32**: Dành riêng cho Octal SPI Flash nội.
* **GPIO 33 đến GPIO 37**: Dành riêng cho Octal PSRAM 8MB nội.
* **GPIO 19, 20**: Cổng USB Native D- và D+ kết nối máy tính.
* **GPIO 43, 44**: Cổng nạp & Debug UART0 Console.
* **GPIO 0**: Chân BOOT / Strapping pin hệ thống.

---

## 2. 🎮 ĐỊNH NGHĨA CHÂN JOYSTICK & 6 NÚT NHẤN CƠ (CHUẨN CHỈ)

### 2.1. Cụm Analog Joystick (2 Trục Analog + 1 Nút nhấn Thumbstick)
Nằm trọn vẹn ở nhóm đầu **Header J3 (Cạnh phải ESP32)**:
* **Nguồn cấp:** BẮT BUỘC dùng **3.3V** (Không dùng 5V vì dải đọc ADC của ESP32-S3 là 0 - 3.3V).
* **2 Trục Analog:** Dùng **ADC1** (hoàn toàn độc lập, không bị xung đột khi bật Bluetooth BLE).

| Chân trên Joystick | Chân trên ESP32-S3 | Loại chân | Chức năng kỹ thuật |
| :--- | :--- | :---: | :--- |
| **GND** | **GND (J3-3)** | Power | Nối Mass chung hệ thống |
| **+5V / VCC** | **3.3V (J3-1)** | Power | Cấp nguồn 3.3V cho 2 biến trở |
| **VRX** | **GPIO 1 (J3-4)** | Analog Input | **ADC1_CH0** (Trục ngang X: Trái=0V, Giữa=1.65V, Phải=3.3V) |
| **VRY** | **GPIO 2 (J3-5)** | Analog Input | **ADC1_CH1** (Trục dọc Y: Lên=0V, Giữa=1.65V, Xuống=3.3V) |
| **SW** | **GPIO 42 (J3-6)** | Digital Input | Nút nhấn cần gạt xuống (Bật Pull-up nội, bấm nối GND) |

---

### 2.2. Cụm 6 Nút Nhấn Cơ (Mechanical Tactile Buttons)
Toàn bộ 6 phím cơ hoạt động theo nguyên lý **Active-LOW** (kích hoạt mức 0):
* ESP32-S3 bật điện trở kéo lên nội (**Internal Pull-up Enable**).
* **KHÔNG CẦN HÀN THÊM ĐIỆN TRỞ NGOÀI!**
* **Một chân của mỗi nút nối với chân GPIO tương ứng.**
* **Chân còn lại của tất cả các nút nối chung về mass (GND).**

Bố cục phân bổ chuẩn tối ưu đường dây hàn:
* **4 Nút chính (Phím 1 -> 4)**: Nằm trên **Header J1 (Cạnh trái)** tạo thành 1 dải 4 chân liên tục (`GPIO 4, 5, 6, 7`).
* **2 Nút phụ (Phím 5, 6)**: Nằm ngay dưới nút Joystick trên **Header J3 (Cạnh phải)** liên tục (`GPIO 41, 40`).
*(Hoặc nếu muốn dải 5 nút bên trái: Phím 5 dùng `GPIO 15` tại J1-8, Phím 6 dùng `GPIO 41` tại J3-7)*.

#### Bảng phân bổ chi tiết 6 phím cơ (Khuyến nghị chuẩn nhất):

| Tên nút | Vị trí Pin | GPIO ESP32 | Chế độ IO | Cách đấu nối phần cứng | Chức năng đề xuất |
| :--- | :---: | :---: | :---: | :--- | :--- |
| **Nút 1 (BTN 1)** | **J1-4** | **GPIO 4** | Input Pullup | Chân A vào GPIO 4, Chân B vào GND | Macro Key 1 / Phím Action A |
| **Nút 2 (BTN 2)** | **J1-5** | **GPIO 5** | Input Pullup | Chân A vào GPIO 5, Chân B vào GND | Macro Key 2 / Phím Action B |
| **Nút 3 (BTN 3)** | **J1-6** | **GPIO 6** | Input Pullup | Chân A vào GPIO 6, Chân B vào GND | Macro Key 3 / Phím Action X |
| **Nút 4 (BTN 4)** | **J1-7** | **GPIO 7** | Input Pullup | Chân A vào GPIO 7, Chân B vào GND | Macro Key 4 / Phím Action Y |
| **Nút 5 (BTN 5)** | **J3-7** | **GPIO 41** | Input Pullup | Chân A vào GPIO 41, Chân B vào GND | Phím L1 / Select / Back |
| **Nút 6 (BTN 6)** | **J3-8** | **GPIO 40** | Input Pullup | Chân A vào GPIO 40, Chân B vào GND | Phím R1 / Start / Mode |

> [!TIP]
> **Tùy chọn gom 5 phím liên tục trên Header J1:**  
> Nếu làm cụm phím 5 nút 1 hàng: bạn có thể dùng **GPIO 15 (J1-8)** làm Nút 5, và **GPIO 41 (J3-7)** làm Nút 6. Cả 2 cách đều sử dụng GPIO hoàn toàn tự do!

---

## 3. 🔌 SƠ ĐỒ ĐẤU NỐI CỤM MÀN HÌNH TFT 2.8" & TOUCH (1:1 HEADER J1)

Cụm 14 chân màn hình TFT + cảm ứng XPT2046 được giữ nguyên vẹn 100% khớp nối tiếp trên **Header J1**:

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

## 4. 📊 BẢNG TỔNG HỢP TOÀN BỘ CHÂN GPIO HỆ THỐNG

### 4.1. Header J1 (Cạnh Trái):
| Vị trí Pin | GPIO | Tên tín hiệu | Khối thiết bị | Chức năng đấu nối |
| :---: | :---: | :---: | :---: | :--- |
| **J1-1** | `3V3` | 3.3V | Nguồn | Nguồn 3.3V |
| **J1-2** | `3V3` | 3.3V | Nguồn | Nguồn 3.3V dự phòng |
| **J1-3** | `EN` | Reset | Hệ thống | Nút bấm Reset vi điều khiển |
| **J1-4** | **GPIO 4** | `BTN_1` | **Nút nhấn cơ 1** | **Nút cơ 1 (Chân kia nối GND)** |
| **J1-5** | **GPIO 5** | `BTN_2` | **Nút nhấn cơ 2** | **Nút cơ 2 (Chân kia nối GND)** |
| **J1-6** | **GPIO 6** | `BTN_3` | **Nút nhấn cơ 3** | **Nút cơ 3 (Chân kia nối GND)** |
| **J1-7** | **GPIO 7** | `BTN_4` | **Nút nhấn cơ 4** | **Nút cơ 4 (Chân kia nối GND)** |
| **J1-8** | **GPIO 15**| `SPARE_IO` | Dự phòng | Chân tự do (Nút cơ 5 tuỳ chọn / Còi Buzzer) |
| **J1-9** | **GPIO 16**| `TOUCH_IRQ` | Cảm ứng Touch | Chân 1 TFT: IRQ |
| **J1-10**| **GPIO 17**| `TOUCH_DO` | Cảm ứng Touch | Chân 2 TFT: DO (T_DO) |
| **J1-11**| **GPIO 18**| `TOUCH_DIN` | Cảm ứng Touch | Chân 3 TFT: DIN (T_DIN) |
| **J1-12**| **GPIO 8** | `TOUCH_CS` | Cảm ứng Touch | Chân 4 TFT: CS (T_CS) |
| **J1-13**| **GPIO 3** | `TOUCH_CLK` | Cảm ứng Touch | Chân 5 TFT: CLK (T_CLK) |
| **J1-14**| **GPIO 46**| `LCD_MISO` | Màn hình LCD | Chân 6 TFT: MISO (SDO) |
| **J1-15**| **GPIO 9** | `LCD_BL` | Màn hình LCD | Chân 7 TFT: LED (Đèn nền) |
| **J1-16**| **GPIO 10**| `SPI_CLK` | Màn hình LCD | Chân 8 TFT: SCK |
| **J1-17**| **GPIO 11**| `SPI_MOSI` | Màn hình LCD | Chân 9 TFT: MOSI (SDI) |
| **J1-18**| **GPIO 12**| `LCD_DC` | Màn hình LCD | Chân 10 TFT: DC |
| **J1-19**| **GPIO 13**| `LCD_RST` | Màn hình LCD | Chân 11 TFT: RESET |
| **J1-20**| **GPIO 14**| `LCD_CS` | Màn hình LCD | Chân 12 TFT: CS |
| **J1-21**| `5V` | 5V Power | Nguồn màn hình | **Chân 14 TFT: VCC (5V)** |
| **J1-22**| `GND` | Ground | Mass màn hình | **Chân 13 TFT: GND** |

---

### 4.2. Header J3 (Cạnh Phải):
| Vị trí Pin | GPIO | Tên tín hiệu | Khối thiết bị | Chức năng đấu nối |
| :---: | :---: | :---: | :---: | :--- |
| **J3-1** | `3V3` | 3.3V | Nguồn Joystick | **Cấp nguồn 3.3V cho Joystick (+5V/VCC)** |
| **J3-2** | `5V` | 5V Power | Nguồn | Nguồn 5V USB |
| **J3-3** | `GND` | Ground | Mass Joystick | **Nối chân GND của Joystick & Cụm phím** |
| **J3-4** | **GPIO 1** | `JOY_VRX` | **Analog Joystick** | **VRx Joystick (ADC1_CH0 - Trục X)** |
| **J3-5** | **GPIO 2** | `JOY_VRY` | **Analog Joystick** | **VRy Joystick (ADC1_CH1 - Trục Y)** |
| **J3-6** | **GPIO 42**| `JOY_SW` | **Analog Joystick** | **SW Joystick (Nút ấn cần gạt, chân kia nối GND)** |
| **J3-7** | **GPIO 41**| `BTN_5` | **Nút nhấn cơ 5** | **Nút cơ 5 (Chân kia nối GND)** |
| **J3-8** | **GPIO 40**| `BTN_6` | **Nút nhấn cơ 6** | **Nút cơ 6 (Chân kia nối GND)** |
| **J3-9** | **GPIO 39**| `IO_FREE_1`| Tự do | Chân GPIO tự do mở rộng |
| **J3-10**| **GPIO 38**| `IO_FREE_2`| Tự do | Chân GPIO tự do mở rộng |
| **J3-11**| `GPIO 37`| *PSRAM* | CẤM DÙNG | Dùng nội bộ cho Octal PSRAM |
| **J3-12**| `GPIO 36`| *PSRAM* | CẤM DÙNG | Dùng nội bộ cho Octal PSRAM |
| **J3-13**| `GPIO 35`| *PSRAM* | CẤM DÙNG | Dùng nội bộ cho Octal PSRAM |
| **J3-14**| `GPIO 0` | `BOOT` | CẤM DÙNG | Nút BOOT trên kit / Strapping |
| **J3-15**| `GPIO 45`| `STRAP` | Dự phòng | Strapping pin VDD_SPI |
| **J3-16**| **GPIO 48**| `WS2812` | Onboard LED | Đèn LED RGB địa chỉ tích hợp trên kit |
| **J3-17**| **GPIO 47**| `IO_FREE_3`| Tự do | Chân GPIO tự do mở rộng |
| **J3-18**| **GPIO 21**| `IO_FREE_4`| Tự do | Chân GPIO tự do mở rộng |
| **J3-19**| `GPIO 20`| *USB_D+* | CẤM DÙNG | Cổng USB Native |
| **J3-20**| `GPIO 19`| *USB_D-* | CẤM DÙNG | Cổng USB Native |
| **J3-21**| `GND` | Ground | Mass chung | Nối Mass hệ thống |
| **J3-22**| `GND` | Ground | Mass chung | Nối Mass hệ thống |

---

## 5. 🛠️ HƯỚNG DẪN HÀN VÀ ĐẤU DÂY THỰC TẾ

```text
                           CÁCH ĐẤU NỐI CỤM PHÍM CƠ
                     
            ESP32-S3                                NÚT BẤM CƠ
     +--------------------+                    +------------------+
     |  GPIO 4 (J1-4)     |------------------->| Chân A   [Nút 1] |
     |  GPIO 5 (J1-5)     |------------------->| Chân A   [Nút 2] |
     |  GPIO 6 (J1-6)     |------------------->| Chân A   [Nút 3] |
     |  GPIO 7 (J1-7)     |------------------->| Chân A   [Nút 4] |
     |  GPIO 41 (J3-7)    |------------------->| Chân A   [Nút 5] |
     |  GPIO 40 (J3-8)    |------------------->| Chân A   [Nút 6] |
     |                    |                    |                  |
     |  GND (J1-22/J3-3)  |---------+--------->| Chân B của cả    |
     +--------------------+         |          | 6 nút nối chung  |
                                    |          +------------------+
                                    v
                              (Nối Mass GND)
```

```text
                           CÁCH ĐẤU NỐI JOYSTICK
                     
            ESP32-S3                                 JOYSTICK MODULE
     +--------------------+                    +-------------------------+
     |  3.3V (J3-1)       |------------------->| +5V / VCC (Dùng 3.3V)   |
     |  GND (J3-3)        |------------------->| GND                     |
     |  GPIO 1 (J3-4)     |------------------->| VRX (Trục X - Analog)   |
     |  GPIO 2 (J3-5)     |------------------->| VRY (Trục Y - Analog)   |
     |  GPIO 42 (J3-6)    |------------------->| SW (Nút bấm cần gạt)    |
     +--------------------+                    +-------------------------+
```

### Lưu ý quan trọng khi đấu nối:
1. **Joystick:** Chân nguồn của Joystick in nhãn `+5V` nhưng **phải cắm vào chân `3.3V` (J3-1)** của ESP32. Nếu cắm vào 5V, điện áp ra của cần gạt sẽ lên đến 5V, có thể làm cháy chân ADC của ESP32-S3!
2. **6 Nút cơ:** Không cần điện trở ngoài. Firmware sử dụng `gpio_config` với `pull_up_en = GPIO_PULLUP_ENABLE` nên chỉ cần nối 1 chân nút vào GPIO, chân kia nối GND. Khi nhấn, mức logic đọc được là `0` (LOW), khi nhả là `1` (HIGH).
