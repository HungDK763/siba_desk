# 🎨 ESP32 LaunchPad Display Studio

> **Bộ công cụ toàn diện thiết kế đồ họa Pixel Art dạng MS Paint chuyên nghiệp, xử lý hình ảnh, tách phông nền, tạo hoạt họa (Animation) và xuất mã nguồn C tương thích 100% chuẩn LVGL v8 dành riêng cho kit ESP32-S3 LaunchPad Display (Màn hình LCD 320x240 ILI9341). Chạy như một ứng dụng Native Desktop độc lập.**

---

## 🌟 Tính Năng Mới Cập Nhật

### 1. 🖥️ Native Desktop App Độc Lập (GTK3 & WebKit2)
- Khởi chạy trực tiếp như ứng dụng Desktop trên hệ thống Ubuntu / Linux mà không phụ thuộc vào việc phải mở tab trình duyệt thủ công.
- Đã tạo sẵn **Desktop Shortcut** trên màn hình Desktop (`~/Desktop/esp32_studio.desktop`) và trong menu ứng dụng hệ thống (`~/.local/share/applications/esp32_studio.desktop`).
- Double-click để mở ngay cửa sổ ứng dụng độc lập với thanh tiêu đề riêng, icon sắc nét và hiệu năng đồ họa tăng tốc phần cứng.

---

### 2. 🖌️ Phòng Vẽ Pixel Art & Animation Studio Nâng Cấp Như MS Paint
- **Bộ công cụ hình học chuyên nghiệp đầy đủ**:
  - ✏️ **Bút chì (Pencil)**
  - 🧹 **Tẩy (Eraser)**
  - 📏 **Đường thẳng (Line)**
  - ⬜ **Hình chữ nhật (Rectangle)**
  - 📐 **Tam giác vuông (Right Triangle)**
  - ⚪ **Hình tròn / Elip (Circle / Ellipse)**
  - 🪣 **Thùng sơn (Bucket Fill)**
  - 🧪 **Hút màu (Eyedropper)**
  - ↩️ / ↪️ **Undo / Redo (`Ctrl + Z` / `Ctrl + Y`)**: Lịch sử hoàn tác đa cấp mượt mà, lưu ảnh chụp trạng thái (snapshot) cho từng frame.
  - ☑️ **Tùy chọn Tô đặc (Filled Shape)**: Cho phép vẽ hình rỗng hoặc đổ màu đặc ngay khi vẽ hình học.
  - 👁️ **Live Preview khi kéo chuột**: Xem trước hình dáng thực tế của đường thẳng, chữ nhật, tam giác, hình tròn ngay trong khi kéo rê chuột trước khi thả tay.
  - ✋ **Công cụ Bàn tay / Kéo trang (Pan Tool)**: Di chuyển, trượt khung vẽ qua lại tự do theo mọi hướng.
- **🔍 Thu Phóng (Zoom In / Out) & Kéo Khung Vẽ (Pan / Drag) Đa Năng**:
  - 🖱️ **Cuộn chuột (Mouse Wheel)**: Tự động Zoom In / Zoom Out mượt mà tại chính xác vị trí con trỏ chuột.
  - ✋ **Kéo trượt tự do (Pan / Drag)**:
    - *Cách 1*: Giữ phím <kbd>Space</kbd> + Kéo chuột trái.
    - *Cách 2*: Bấm giữ con lăn chuột (Middle Mouse Click) hoặc Chuột phải rồi kéo.
    - *Cách 3*: Chọn công cụ Bàn tay ✋ (<kbd>H</kbd>) trên thanh công cụ và kéo chuột.
    - *Cách 4*: Sử dụng cụm phím điều hướng nổi ◀ ▶ ▲ ▼ ở góc dưới bên phải khung vẽ.
    - *Cách 5*: Giữ <kbd>Shift</kbd> + Cuộn chuột hoặc vuốt 2 ngón tay trên Trackpad để trượt ngang.
  - 🎛️ **Nút điều khiển Topbar**: Nút `+`, `-`, ô tỉ lệ zoom `%`, nút **`Vừa khung`** (Fit to screen) và **`⌖ Căn giữa`** (Center).
- **Thanh trượt điều khiển Paint linh hoạt**:
  - 🎚️ **Thanh trượt Độ phủ / Trong suốt (Opacity / Alpha 0% - 100%)**: Điều chỉnh độ đậm nhạt của nét vẽ, hỗ trợ hòa trộn alpha (Porter-Duff blending).
  - 🖌️ **Thanh trượt Kích thước cọ / tẩy (Brush Size 1px - 16px)**: Điều chỉnh cỡ cọ vẽ hoặc đầu tẩy tức thì.
- **Tùy biến kích thước Canvas / Frame theo ý muốn**:
  - Ô nhập trực tiếp **Chiều rộng (W)** và **Chiều cao (H)**.
  - Menu kích thước chuẩn: `128x128` (Icon/Sprite), `240x240` (Màn tròn/vuông), `240x280` (Smartwatch), `320x240` (Toàn màn hình LaunchPad), `480x320` (Màn hình lớn).
  - Nút **"Đổi Cỡ"**: Cập nhật kích thước canvas tức thì và giữ nguyên hoặc điều chỉnh tỉ lệ khung vẽ.
- **Timeline Đa Frame & Hoạt Họa (Animation)**:
  - Thêm frame trống, nhân bản frame, xóa frame.
  - 🧅 Onion Skinning: Hiển thị mờ các frame lân cận để căn chuyển động nhân vật.
  - Trình phát hoạt họa mini (1 đến 60 FPS).
  - Xuất ảnh GIF động chất lượng cao.

---

### 3. 🖼️ Chuyển Đổi & Xử Lý Ảnh Thông Minh (Image Converter & Background Removal)
- **Thuật toán Tách / Xóa Nền Chuyên Dụng**:
  - 🪄 **Magic Wand từ viền ngoài (Flood-fill Outer)**: Quét và loại bỏ nền từ các cạnh ngoài của ảnh, **giữ nguyên toàn bộ chi tiết màu trắng/đen bên trong nhân vật** (như bụng cún Shiba, mắt, bàn chân).
  - 🌐 **Xóa theo màu toàn ảnh (Global Color)**: Xóa mọi pixel trùng màu trong ảnh theo ngưỡng sai số.
  - 🔲 **Tự động dò màu 4 góc**: Phân tích màu ở 4 góc ảnh để tự động tách phông nền không cần chọn tay.
  - 🧪 **Hút màu nền trực tiếp (Interactive Click Eyedropper)**: Click chuột trực tiếp vào bất kỳ vị trí nào trên ảnh gốc để chỉ định màu nền cần xóa!
  - 🎚️ **Ngưỡng nhận diện (Threshold 0 - 150)**: Tinh chỉnh độ nhạy khi xóa phông.
- **Bộ Lọc Hình Ảnh Tích Hợp**:
  - ☀️ **Độ sáng (Brightness 50% - 200%)**.
  - 🌓 **Độ tương phản (Contrast 50% - 200%)**.
  - 🔄 **Xoay 90°**, ↔️ **Lật ngang**, ↕️ **Lật dọc**, ☯️ **Đảo màu**, ⚪ **Trắng đen**.
  - **Live Preview tức thì**: Mọi thanh trượt và nút bấm cập nhật kết quả xem trước ngay lập tức trên nền bàn cờ caro (Checkerboard) tương phản cao.
- **Lượng tử hóa màu & Khử vệt màu**:
  - *Floyd-Steinberg Dithering RGB565*: Khử hoàn toàn hiện tượng vệt màu (color banding) trên LCD 16-bit.

---

### 4. ⚡ Xuất Mã Nguồn C Chuẩn LVGL v8 & Nạp Lên ESP32-S3
- **Tương thích 100% cấu hình màu LaunchPad**:
  - Byte swap chuẩn `CONFIG_LV_COLOR_16_SWAP=y`.
  - Không bao giờ bị đảo màu Đỏ ↔ Xanh lam khi nạp vào phần cứng thật.
- **Chế độ ghi đè linh kiện tích hợp**:
  - Ghi đè Sprite Shiba 3 frame vào [`components/display_ui/src/bongo_shiba_sprite.c`](file:///home/hungdk4/Documents/Code/for-me/lanchPad/esp_lanchpad_hid/components/display_ui/src/bongo_shiba_sprite.c).
  - Ghi đè Wallpaper vào [`components/display_ui/src/earth_wallpaper.c`](file:///home/hungdk4/Documents/Code/for-me/lanchPad/esp_lanchpad_hid/components/display_ui/src/earth_wallpaper.c).
  - Tạo file module C/H mới độc lập.
- **Nạp Firmware Trực Tiếp (One-Click Flash)**:
  - Tự động biên dịch `idf.py build && idf.py flash` và hiển thị console theo dõi thời gian thực.

---

## 🚀 Cách Sử Dụng Ứng Dụng Desktop

### Cách 1: Mở từ màn hình Desktop (Khuyên dùng)
- Double-click vào biểu tượng **ESP32 Display Studio** trên Desktop (`~/Desktop/esp32_studio.desktop`).

### Cách 2: Mở bằng lệnh Terminal
```bash
cd /home/hungdk4/Documents/Code/for-me/lanchPad/esp_lanchpad_hid/tools/esp_display_studio
./run.sh
```
> Script sẽ tự động phát hiện màn hình và mở cửa sổ Native Desktop App ngay lập tức.

### Cách 3: Mở ở chế độ Web Browser truyền thống (nếu muốn)
```bash
./run.sh --browser
```

---

## 📁 Cấu Trúc Thư Mục Studio

```
tools/esp_display_studio/
├── app_desktop.py    # Ứng dụng Desktop Native (GTK3 + WebKit2 WebView)
├── esp32_studio.desktop # Shortcut chạy ứng dụng trên Desktop Linux
├── icon.png          # Icon ứng dụng Desktop độ phân giải cao
├── index.html        # Giao diện Studio hiện đại (Pixel Editor, Image Converter, LVGL C)
├── server.py         # Backend REST API xử lý đồ họa (Pillow) & Flasher
├── run.sh            # Script khởi chạy tự động
└── README.md         # Hướng dẫn sử dụng chi tiết
```
