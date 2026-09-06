# 📋 PHIẾU CHẨN ĐOÁN HIỂN THỊ MÀN HÌNH ILI9341 (ESP-IDF OFFICIAL EXAMPLE)

> **Hướng dẫn:** Bạn chỉ cần mở file này lên, gõ `x` vào ô `[ ]` tương ứng (ví dụ: `[x]`) hoặc điền vài chữ vào chỗ trống, sau đó lưu file lại (`Ctrl + S`) và nhắn tôi đọc.

---

### 1. VỊ TRÍ HÀNG CHÂN CÁP 14-PIN (Khi bạn đặt màn hình trước mặt)

- [X] Bên PHẢI (Màn hình nằm ngang)
- [ ] Bên TRÁI (Màn hình nằm ngang)
- [ ] Phía DƯỚI đáy (Màn hình dựng dọc)
- [ ] Phía TRÊN đỉnh (Màn hình dựng dọc)

---

### 2. CHU KỲ TỰ ĐỘNG XOAY GÓC (Mỗi 4 giây đổi 1 lần)

Màn hình có đang tự động đổi góc quay sau mỗi 4 giây không:

- [X] Có, tự động xoay liên tục
- [ ] Không xoay / đứng yên một chỗ

Dòng chữ thông báo trên đỉnh màn hình (`ROT: ...`), **góc nào bạn đọc XUÔI MẮT NHẤT** (chữ không bị lộn ngược đầu, đọc xuôi từ trái sang phải):

- [ ] `ROT: NONE (0 deg)`
- [ ] `ROT: 90 deg`
- [X] `ROT: 180 deg`
- [ ] `ROT: 270 deg`

---

### 3. ĐÁNH GIÁ KHUNG HÌNH & HIỆN TƯỢNG VỠ / LỆCH HÌNH (GEOMETRY)

*(Quan sát tại góc bạn thấy xuôi mắt nhất ở câu 2)*

**3.1. Độ phủ kín màn hình:**

- [ ] Phủ kín 100% diện tích kính, sắc nét, không có dải nhiễu nào
- [ ] Có một dải sọc nhiễu hột é (vị trí dải nhiễu: ....................)
- [X] Bị mất / bị cắt mất một phần viền (vị trí bị cắt: ví dụ khi nó quay 180 độ thì nó bị cắt khoảng 80Px bên phải, khi quay 0 độ bị cắt khoảng 80px bên trái ( so với góc nhìn và khung  đồng hồ không nằm chính giữ màn hình thực tế nó đang bị lênh xuống theo chiều dọc và bị lộ một tí đáy dưới đồng hồ lên trên khung hình., ở các góc 90/270 màn hình luôn bị không update phần trên/ dưới hình ảnh bị sọc lung tung, kim quay đến đâu thì chõ đó mới được updatw.

  hãy nhớ khi khởi động lên nó ở góc 0 độ ( nằm ngang lộn ngược 180 độ so với hướng nhìn của tôi hiện tại ) chư ROT: ... nằm ngang)

**3.2. Hình dạng đồng hồ tròn (Meter Widget):**

- [ ] Tròn trịa nguyên vẹn nằm ở giữa
- [X] Bị "chém" làm đôi (một phần nằm bên trái, một phần bị trôi sang mép đối diện) như tôi đã mô tả phần lớn hiể nthij đúng một phần đồng hộ bị chèn lên trên.
- [ ] Khác: ................................................................

---

### 4. ĐÁNH GIÁ MÀU SẮC THỰC TẾ (COLOR FIDELITY)

**4.1. Nền màn hình (Background):**

- [ ] Màu ĐEN / Xám than tối (bình thường)
- [X] Màu TRẮNG xóa lóa mắt  ( màu kiểu dám xanh lá rất nhạt )
- [ ] Màu khác: ....................

**4.2. Vòng cung nhỏ (vạch từ 0 đến 20):**

- [ ] Đúng màu XANH DƯƠNG
- [ ] Bị biến thành màu ĐỎ
- [X] Màu khác: ........màu xám...........

**4.3. Vòng cung lớn (vạch từ 80 đến 100) & Cây kim quét:**

- [ ] Đúng màu ĐỎ TƯƠI
- [ ] Bị biến thành màu XANH DƯƠNG
- [ ] Màu khác: ....hồng rất nhạt................

---

### 5. GHI CHÚ THÊM CỦA BẠN (Nếu có):

> Điền tại đây:
