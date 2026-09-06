# 🖥️ Host Tools - Quota Telemetry Daemon & CLI Controller

Thư mục này chứa toàn bộ công cụ chạy trên máy tính Linux để giám sát hạn mức (Quota) Antigravity/Gemini/Claude, chỉ số CPU/RAM và đồng bộ thời gian thực Việt Nam xuống ESP32 Desk Console qua BLE không dây.

---

## 🌟 Tính Năng Nổi Bật

1. **Siêu Nhẹ Tài Nguyên (< 0.05% CPU trung bình):**
   - **LSP Cache:** Cache trực tiếp PID, Port và CSRF token của Antigravity; không quét lại toàn bộ `/proc` mỗi chu kỳ.
   - **Zero-wait CPU Sampling:** Dùng `psutil.cpu_percent(interval=None)` không gây sleep/block luồng.
   - **D-Bus BlueZ Object Cache:** Tái sử dụng proxy kết nối GATT BLE qua D-Bus, truyền gói tin chỉ trong <1ms.
   - **NTP Drift Cache:** Đồng bộ sai số giờ với NTP Server (Google/Cloudflare) 15 phút/lần, tính giờ Việt Nam (UTC+7) tức thì với 0 chi phí mạng/CPU.
2. **Cài Đặt Đơn Giản Trên Mọi Máy Linux (`install.sh`):**
   - Tự động kiểm tra Python, cài đặt dependencies (`dbus-fast`, `psutil`, `pyserial`).
   - Cài đặt dịch vụ nền `systemd --user`.
   - **Mặc định tự khởi động sau khi máy tính bật.**
3. **Lệnh Toàn Cục `esp_quota`:**
   - Người dùng có thể gọi `esp_quota` ở bất kỳ thư mục/terminal nào.
   - Tích hợp Menu giao diện trực quan và đầy đủ subcommands.
   - Cho phép bật/tắt tính năng tự khởi động (`esp_quota autostart on|off`).
4. **Trình Gỡ Bỏ Sạch Sẽ (`uninstall.sh`):**
   - Tắt và xóa dịch vụ systemd, dọn dẹp lệnh thực thi và thư mục cài đặt chỉ trong 1 thao tác.

---

## 📦 Cài Đặt (Chỉ 1 lệnh duy nhất)

Chạy script cài đặt trong thư mục `host_tools`:
```bash
./install.sh
```
Script sẽ tự động:
- Cài đặt dependencies.
- Tạo lệnh `esp_quota` toàn cục tại `~/.local/bin/esp_quota`.
- Thiết lập dịch vụ `systemd --user` tự khởi động cùng máy tính.
- Khởi động dịch vụ chạy nền ngay lập tức.

---

## 🛠️ Hướng Dẫn Sử Dụng Lệnh `esp_quota`

### 1. Mở Menu Điều Khiển Trực Quan
Chỉ cần gõ trong terminal:
```bash
esp_quota
```
Hệ thống sẽ hiển thị menu tương tác để bạn chọn thao tác bằng phím số:
```text
╔════════════════════════════════════════════════════════════════════════╗
║         🚀 ESP QUOTA TELEMETRY CONTROLLER (esp_quota)                 ║
║         Quản lý dịch vụ đồng bộ Antigravity Quota BLE cho ESP32        ║
╚════════════════════════════════════════════════════════════════════════╝
Trạng thái: Đang chạy | Tự khởi động: BẬT | ESP32 MAC: 7C:E8:B1:B2:79:AA
------------------------------------------------------------------------
 [1] Xem trạng thái chi tiết (Status)
 [2] Bật dịch vụ chạy nền (Start)
 [3] Dừng dịch vụ chạy nền (Stop)
 [4] Khởi động lại dịch vụ (Restart)
 [5] BẬT tự khởi động cùng máy tính (Autostart ON)
 [6] TẮT tự khởi động cùng máy tính (Autostart OFF)
 [7] Đổi cấu hình MAC ESP32 & chu kỳ gửi (Config)
 [8] Gửi thử 1 lần tới ESP32 ngay lập tức (Test sync)
 [9] Xem nhật ký hoạt động thời gian thực (Live logs)
 [0] Thoát (Exit)
```

### 2. Các Lệnh Nhanh (CLI Commands)

| Lệnh | Chức năng |
| :--- | :--- |
| `esp_quota status` | Xem trạng thái dịch vụ, % CPU/RAM daemon, cấu hình và Quota hiện tại |
| `esp_quota autostart on` | **BẬT** tự khởi động cùng máy tính |
| `esp_quota autostart off` | **TẮT** tự khởi động cùng máy tính |
| `esp_quota start` | Bật dịch vụ chạy nền |
| `esp_quota stop` | Dừng dịch vụ chạy nền |
| `esp_quota restart` | Khởi động lại dịch vụ chạy nền |
| `esp_quota config` | Xem hoặc thay đổi địa chỉ MAC ESP32 và chu kỳ gửi (giây) |
| `esp_quota test` | Gửi 1 gói tin BLE thử nghiệm tới ESP32 ngay lập tức |
| `esp_quota log -f` | Xem nhật ký hoạt động thời gian thực (live journalctl) |

---

## ⚙️ Tệp Cấu Hình

Cấu hình được lưu tại `~/.config/esp_quota/config.json`:
```json
{
    "device_mac": "7C:E8:B1:B2:79:AA",
    "interval_seconds": 10,
    "enabled": true,
    "autostart": true
}
```
Khi bạn sửa tệp này (hoặc qua lệnh `esp_quota config`), daemon sẽ **tự động nhận diện và cập nhật** mà không làm gián đoạn hệ thống.

---

## 🛑 Gỡ Bỏ Cài Đặt (Uninstall)

Khi không còn sử dụng:
```bash
./uninstall.sh
```
Nếu muốn xóa luôn cả tệp cấu hình cũ:
```bash
./uninstall.sh --purge
```
