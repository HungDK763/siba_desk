# 🐾 Bongo Shiba Pixel Art Editor & Animator

Công cụ trực quan chạy cục bộ trên nền Web giúp bạn vẽ, chỉnh sửa từng điểm ảnh (pixel art), căn chỉnh animation và xuất trực tiếp mã C (LVGL) cho chú cún **Bongo Shiba** trên màn hình bàn phím ESP32.

---

## 📂 Cấu trúc thư mục

```text
tools/bongo_shiba_editor/
├── index.html          # Giao diện Web Editor (Canvas, công cụ vẽ, bảng màu, preview live)
├── server.py           # Python HTTP server (xử lý API lưu ảnh 1X/3X, sinh GIF, xuất mã C)
├── run.sh              # Script khởi chạy nhanh tiện lợi
├── requirements.txt    # Các thư viện Python cần thiết (Pillow)
└── README.md           # Hướng dẫn sử dụng chi tiết
```

---

## 🚀 Cách khởi chạy

### Cách 1: Chạy từ thư mục tool
```bash
cd tools/bongo_shiba_editor
./run.sh
```

### Cách 2: Chạy từ thư mục gốc của dự án
```bash
./run_editor.sh
```

Sau khi chạy lệnh trên:
1. Mở trình duyệt web bất kỳ (Chrome, Edge, Firefox, Brave,...).
2. Truy cập địa chỉ: **[http://localhost:8088](http://localhost:8088)**.

---

## 🎨 Các tính năng chính

| Tính năng | Mô tả |
| :--- | :--- |
| **Canvas Pixel Art 104x78** | Tỷ lệ chuẩn của sprite trên màn hình LCD ESP32. Hỗ trợ phóng to từ 8x đến 14x kèm lưới chia pixel rõ nét. |
| **Bộ công cụ vẽ** | ✏️ **Bút vẽ (Pencil)**, 🧹 **Tẩy (Eraser)**, 🪣 **Đổ màu (Bucket Fill)**, 💧 **Hút màu (Color Picker)**. |
| **Bảng màu chuẩn Shiba** | Bộ màu cố định được trích xuất từ chú cún gốc: Vàng Shiba `#E08930`, Kem ngực/má `#FBE1BF`, Mũi/mắt `#231815`, Đỏ khăn `#D92525`, Đỏ đậm `#9E1B1B`, Trắng bàn `#FFFFFF`, Trong suốt, v.v. |
| **🛡️ Khóa khăn đỏ (Lock Scarf)** | Chế độ an toàn: khi bật, toàn bộ vùng khăn quàng đỏ signature ở ngực cún sẽ được bảo vệ, ngăn không cho nét vẽ tay hay tẩy đè lên hoặc làm mất khăn. |
| **🧅 Onion Skinning** | Hiển thị mờ (ghost overlay) của các khung hình khác để dễ dàng căn chỉnh quỹ đạo chuyển động nhấc tay 60° và đập bàn. |
| **🖼️ Lớp ảnh tham chiếu (Reference)** | Cho phép bật lớp ảnh mẫu `bongo_dog_crop.png` đè mờ lên canvas với thanh trượt chỉnh độ trong suốt. |
| **⏪ Phím tắt Undo / Redo** | Hỗ trợ thao tác quay lại `Ctrl + Z` và khôi phục `Ctrl + Y` thuận tiện. |
| **🎬 Cửa sổ Live Animation Preview** | Xem trước hoạt ảnh đập tay trái / đập tay phải / nghỉ ngơi thời gian thực ngay trong lúc vẽ; có thanh chỉnh tốc độ (FPS / ms). |
| **💾 Nút "Lưu khung hình & GIF"** | Tự động lưu 3 file PNG 1X (`104x78`), tự động phóng to 3X sắc nét chuẩn Nearest Neighbor (`312x234`) và tạo file hoạt ảnh `bongo_60deg_animation_3x.gif` trong `docs_for_me/bongo_dog_preview/`. |
| **⚡ Nút "Xuất mã C (LVGL)"** | Tự động chuyển đổi toàn bộ 3 khung hình sang mảng byte C `LV_IMG_CF_TRUE_COLOR_ALPHA` (RGB565 + 8-bit Alpha) và ghi thẳng vào file `components/display_ui/src/bongo_shiba_sprite.c`. |

---

## 🛠️ Quy trình cập nhật hình ảnh vào ESP32

1. Khởi động editor: `./run_editor.sh`.
2. Mở `http://localhost:8088`, chỉnh sửa hoặc vẽ lại các tư thế tay của chú cún theo ý muốn.
3. Quan sát khung **Live Animation Preview** để đảm bảo chuyển động tự nhiên, tay nhấc đẹp và khăn quàng đỏ hiển thị 100%.
4. Nhấn **"Lưu khung hình & GIF"** để cập nhật các file ảnh và GIF xem trước.
5. Nhấn **"Xuất mã C (LVGL)"** để tool tự động cập nhật mã nguồn C.
6. Quay lại terminal dự án và biên dịch / nạp vào ESP32:
   ```bash
   idf.py build flash monitor
   ```
