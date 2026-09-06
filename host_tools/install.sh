#!/usr/bin/env bash
# ==============================================================================
# Trình Cài Đặt Tự Động: ESP Quota Telemetry Daemon & CLI (esp_quota)
# Hỗ trợ cài đặt trên mọi bản phân phối Linux (Ubuntu, Debian, Fedora, Arch...)
# ==============================================================================
set -e

# Màu sắc hiển thị
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

echo -e "${CYAN}${BOLD}╔════════════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}${BOLD}║        🚀 ESP QUOTA TELEMETRY - TRÌNH CÀI ĐẶT TỰ ĐỘNG                 ║${NC}"
echo -e "${CYAN}${BOLD}║        Cài đặt Daemon & Công cụ CLI 'esp_quota' siêu nhẹ (<0.05% CPU)  ║${NC}"
echo -e "${CYAN}${BOLD}╚════════════════════════════════════════════════════════════════════════╝${NC}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="$HOME/.local/share/esp_quota"
BIN_DIR="$HOME/.local/bin"
CONFIG_DIR="$HOME/.config/esp_quota"
CONFIG_FILE="$CONFIG_DIR/config.json"
SYSTEMD_USER_DIR="$HOME/.config/systemd/user"
SERVICE_FILE="$SYSTEMD_USER_DIR/esp_quota.service"

# 1. Kiểm tra môi trường hệ thống
echo -e "\n${BOLD}[1/6] Kiểm tra môi trường hệ thống...${NC}"
if ! command -v python3 &>/dev/null; then
    echo -e "${RED}❌ Không tìm thấy Python 3! Vui lòng cài đặt Python 3 trước.${NC}"
    exit 1
fi
PYTHON_VERSION=$(python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
echo -e "  • Python:       ${GREEN}$PYTHON_VERSION${NC}"

if ! command -v systemctl &>/dev/null; then
    echo -e "${RED}❌ Không tìm thấy systemd! Hệ thống cần systemd để quản lý dịch vụ nền.${NC}"
    exit 1
fi
echo -e "  • Service Mgr:  ${GREEN}systemd --user${NC}"

if ! command -v bluetoothctl &>/dev/null; then
    echo -e "  • Bluetooth:    ${YELLOW}Cảnh báo: Không thấy bluetoothctl (BlueZ). Hãy đảm bảo BlueZ đã được cài.${NC}"
else
    echo -e "  • Bluetooth:    ${GREEN}BlueZ sẵn sàng${NC}"
fi

# 2. Cài đặt thư viện Python phụ thuộc
echo -e "\n${BOLD}[2/6] Kiểm tra và cài đặt thư viện Python phụ thuộc...${NC}"
REQUIREMENTS_MET=true
python3 -c "import psutil, dbus_fast" 2>/dev/null || REQUIREMENTS_MET=false

if [ "$REQUIREMENTS_MET" = false ]; then
    echo -e "  Đang cài đặt các thư viện cần thiết (${CYAN}psutil, dbus-fast, pyserial${NC})..."
    if command -v pip3 &>/dev/null; then
        pip3 install --user -r "$SCRIPT_DIR/requirements.txt" 2>/dev/null || \
        pip3 install --user --break-system-packages -r "$SCRIPT_DIR/requirements.txt" 2>/dev/null || {
            echo -e "${YELLOW}  Thử cài đặt qua apt/package manager...${NC}"
            if command -v sudo &>/dev/null; then
                sudo apt update && sudo apt install -y python3-psutil python3-pip
            fi
        }
    else
        echo -e "${YELLOW}  Không có pip3, thử cài qua apt...${NC}"
        if command -v sudo &>/dev/null; then
            sudo apt update && sudo apt install -y python3-psutil python3-pip
        fi
    fi
else
    echo -e "  • Thư viện:     ${GREEN}Đầy đủ (psutil, dbus-fast)${NC}"
fi

# 3. Tạo cấu trúc thư mục và sao chép mã nguồn
echo -e "\n${BOLD}[3/6] Thiết lập thư mục và công cụ CLI...${NC}"
mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$CONFIG_DIR"
mkdir -p "$SYSTEMD_USER_DIR"

cp "$SCRIPT_DIR/quota_daemon.py" "$INSTALL_DIR/quota_daemon.py"
cp "$SCRIPT_DIR/esp_quota" "$INSTALL_DIR/esp_quota"
chmod +x "$INSTALL_DIR/quota_daemon.py"
chmod +x "$INSTALL_DIR/esp_quota"

# Tạo liên kết tượng trưng (symlink) vào ~/.local/bin/esp_quota
ln -sf "$INSTALL_DIR/esp_quota" "$BIN_DIR/esp_quota"
echo -e "  • CLI Symlink:  ${GREEN}$BIN_DIR/esp_quota${NC}"

# Nếu có quyền sudo, tạo thêm symlink vào /usr/local/bin để tất cả người dùng dùng được
if [ -w "/usr/local/bin" ]; then
    ln -sf "$INSTALL_DIR/esp_quota" "/usr/local/bin/esp_quota" 2>/dev/null || true
    echo -e "  • Global Link:  ${GREEN}/usr/local/bin/esp_quota${NC}"
fi

# Kiểm tra PATH
if [[ ":$PATH:" != *":$BIN_DIR:"* ]]; then
    echo -e "  • PATH check:   ${YELLOW}$BIN_DIR chưa có trong PATH.${NC}"
    echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> "$HOME/.bashrc"
    if [ -f "$HOME/.zshrc" ]; then
        echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> "$HOME/.zshrc"
    fi
    echo -e "    ${GREEN}Đã tự động thêm vào ~/.bashrc và ~/.zshrc${NC}"
fi

# 4. Khởi tạo tệp cấu hình mặc định (nếu chưa có)
echo -e "\n${BOLD}[4/6] Thiết lập tệp cấu hình...${NC}"
if [ ! -f "$CONFIG_FILE" ]; then
    cat <<EOF > "$CONFIG_FILE"
{
    "device_mac": "7C:E8:B1:B2:79:AA",
    "interval_seconds": 10,
    "enabled": true,
    "autostart": true
}
EOF
    echo -e "  • Cấu hình:     ${GREEN}Tạo mới tại $CONFIG_FILE${NC}"
else
    echo -e "  • Cấu hình:     ${GREEN}Đã tồn tại tại $CONFIG_FILE (giữ nguyên cấu hình cũ)${NC}"
fi

# 5. Cài đặt và kích hoạt Systemd User Service (Mặc định tự khởi động khi mở máy)
echo -e "\n${BOLD}[5/6] Thiết lập dịch vụ nền systemd (Autostart on Boot)...${NC}"
cat <<EOF > "$SERVICE_FILE"
[Unit]
Description=ESP Quota Telemetry BLE Daemon
Documentation=https://github.com
After=bluetooth.target network-online.target
Wants=bluetooth.target

[Service]
Type=simple
Environment=PYTHONUNBUFFERED=1
ExecStart=/usr/bin/python3 $INSTALL_DIR/quota_daemon.py
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Tối ưu hóa tài nguyên CPU và phân phối tải
Nice=10
CPUSchedulingPolicy=other

[Install]
WantedBy=default.target
EOF

# Dừng các tiến trình quota_daemon.py chạy thủ công cũ nếu có
pkill -f "$INSTALL_DIR/quota_daemon.py" 2>/dev/null || true
pkill -f "quota_daemon.py" 2>/dev/null || true
sleep 0.5

systemctl --user daemon-reload
# Mặc định bật tự khởi động cùng máy tính
systemctl --user enable esp_quota.service
systemctl --user restart esp_quota.service

# Bật linger cho user hiện tại để service chạy liên tục kể cả sau boot
loginctl enable-linger "$USER" 2>/dev/null || true

echo -e "  • Dịch vụ:      ${GREEN}esp_quota.service đã kích hoạt và đang chạy nền!${NC}"
echo -e "  • Tự khởi động: ${GREEN}[BẬT] Mặc định luôn khởi động sau khi máy tính bật.${NC}"

# 6. Hoàn tất và hướng dẫn sử dụng
echo -e "\n${GREEN}${BOLD}════════════════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}${BOLD}🎉 CÀI ĐẶT THÀNH CÔNG! BẠN CÓ THỂ GỌI 'esp_quota' Ở MỌI NƠI TRONG TERMINAL.${NC}"
echo -e "${GREEN}${BOLD}════════════════════════════════════════════════════════════════════════${NC}"
echo -e "\n${BOLD}Các lệnh điều khiển nhanh:${NC}"
echo -e "  ${CYAN}esp_quota${NC}                 - Mở menu điều khiển trực quan"
echo -e "  ${CYAN}esp_quota status${NC}          - Xem trạng thái dịch vụ, CPU, RAM & Quota"
echo -e "  ${CYAN}esp_quota autostart on${NC}    - BẬT tự khởi động cùng máy tính"
echo -e "  ${CYAN}esp_quota autostart off${NC}   - TẮT tự khởi động cùng máy tính"
echo -e "  ${CYAN}esp_quota config${NC}          - Cấu hình địa chỉ MAC ESP32 hoặc chu kỳ gửi"
echo -e "  ${CYAN}esp_quota test${NC}            - Gửi 1 gói tin thử nghiệm ngay lập tức"
echo -e "  ${CYAN}esp_quota log -f${NC}          - Xem nhật ký hoạt động thời gian thực"
echo -e "  ${CYAN}esp_quota stop / restart${NC}  - Dừng hoặc khởi động lại dịch vụ"
echo
