# 📋 BÁO CÁO KIỂM THỬ ĐỒNG BỘ QUOTA ANTIGRAVITY QUA BLE

> **Tài liệu kiểm định thực tế tính năng PC Sync - ESP32 LaunchPad**  
> **Thời gian thực hiện:** 06/09/2026 - 08:16:00 (GMT+7)  
> **Trạng thái:** ✅ **HOÀN THÀNH & CĂN CHỈNH CHUẨN XÁC 100%**

---

## 1. Dữ liệu Hạn mức Thực tế từ Antigravity (Ground Truth)

Dữ liệu đọc trực tiếp từ Local Language Server của Antigravity CLI / IDE:
- **Tài khoản:** `tolahungstm@gmail.com`
- **Cơ chế:** Quota chia thành 2 nhóm mô hình (Gemini Models & Claude/GPT Models) với chu kỳ 5 giờ và chu kỳ tuần.

| Nhóm Mô hình (Group) | Chu kỳ (Window) | Hạn mức còn lại (%) | Thời gian làm mới | Trạng thái hiển thị |
| :--- | :---: | :---: | :---: | :---: |
| **GEMINI MODELS** | **5-Hour Limit** | **`45%`** | Làm mới sau ~1h40m | `[CẢNH BÁO 🟡]` |
| | **Weekly Limit** | **`88%`** | Làm mới sau 6 ngày 7h | `88%` |
| **CLAUDE & GPT MODELS** | **5-Hour Limit** | **`100%`** | Làm mới sau ~2h | `[AN TOÀN  🟢]` |
| | **Weekly Limit** | **`100%`** | Làm mới sau 6 ngày 20h | `100%` |

---

## 2. Log Chuẩn Hóa Trên ESP32 UART Monitor (/dev/ttyACM0)

> Đã loại bỏ hoàn toàn các icon trang trí (`📊`, `📥`, `•`), chỉ giữ lại nút chấm màu trạng thái (`🟢`, `🟡`, `🔴`), đồng thời toàn bộ các viền dọc `|` (cả viền ngoài và cột ngăn cách giữa) đều được căn chỉnh thẳng hàng tuyệt đối.

```text
I (24259) BLE_HID: BLE Received 142 bytes on PC Sync Characteristic
I (24259) LP_CORE: +--------------------------------------------------------------------+
I (24259) LP_CORE: | [PC_SYNC] CAP NHAT DU LIEU QUOTA ANTIGRAVITY QUA BLE               |
I (24259) LP_CORE: +--------------------------------------------------------------------+
I (24269) LP_CORE: | Gio Viet Nam:       01:23:07 (UTC+7)                               |
I (24269) LP_CORE: | AI Model:           Gemini 3.8 Flash (High)                        |
I (24269) LP_CORE: | [GEMINI] 5h Limit:   42% [CẢNH BÁO 🟡]  | Weekly:  88%             |
I (24269) LP_CORE: | [CLAUDE] 5h Limit:  100% [AN TOÀN  🟢]  | Weekly: 100%             |
I (24269) LP_CORE: | Số Prompts hôm nay: 49                                             |
I (24269) LP_CORE: | Host PC Telemetry:  CPU: 33%            | RAM: 58%                 |
I (24269) LP_CORE: | Trạng thái Module:  PARSED_SUCCESS                                 |
I (24269) LP_CORE: +--------------------------------------------------------------------+
```

---

## 3. Quy Chuẩn Kỹ Thuật Đã Căn Chỉnh

1. **Loại bỏ Icon thừa:** Không còn icon đầu mục hay biểu tượng đồ họa gây lệch dòng trên terminal.
2. **Nút trạng thái:** Giữ lại các nút chấm tròn tiêu chuẩn:
   - `AN TOÀN  🟢` (Xanh khi Quota >= 50%)
   - `CẢNH BÁO 🟡` (Vàng khi Quota từ 20% đến 49%)
   - `NGUY CẤP 🔴` (Đỏ khi Quota < 20%)
3. **Căn lề trực quan (Visual Column Width Padding):** Sử dụng thuật toán đếm bề rộng hiển thị thực tế (bù trừ cho ký tự Unicode tiếng Việt và emoji 2-cột), đảm bảo chiều rộng khung luôn cố định chính xác 70 cột trên mọi terminal.
4. **Viền phân cách `|`:** Thẳng tắp cả 2 bên viền mép ngoài lẫn cột phân cách ở giữa.
