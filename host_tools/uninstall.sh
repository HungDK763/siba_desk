#!/usr/bin/env bash
# ==============================================================================
# Trình Gỡ Cài Đặt: ESP Quota Telemetry Daemon & CLI (esp_quota)
# ==============================================================================
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

echo -e "${YELLOW}${BOLD}╔════════════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${YELLOW}${BOLD}║        🛑 ESP QUOTA TELEMETRY - TRÌNH GỠ CÀI ĐẶT                      ║${NC}"
echo -e "${YELLOW}${BOLD}║        Dừng dịch vụ và gỡ bỏ công cụ 'esp_quota' khỏi hệ thống        ║${NC}"
echo -e "${YELLOW}${BOLD}╚════════════════════════════════════════════════════════════════════════╝${NC}"

INSTALL_DIR="$HOME/.local/share/esp_quota"
BIN_DIR="$HOME/.local/bin"
CONFIG_DIR="$HOME/.config/esp_quota"
SERVICE_FILE="$HOME/.config/systemd/user/esp_quota.service"

# 1. Dừng và vô hiệu hóa dịch vụ nền systemd
echo -e "\n${BOLD}[1/4] Dừng và vô hiệu hóa dịch vụ nền...${NC}"
if systemctl --user is-active --quiet esp_quota.service 2>/dev/null; then
    echo -e "  Đang dừng esp_quota.service..."
    systemctl --user stop esp_quota.service 2>/dev/null || true
fi

if systemctl --user is-enabled --quiet esp_quota.service 2>/dev/null; then
    echo -e "  Đang vô hiệu hóa tự khởi động..."
    systemctl --user disable esp_quota.service 2>/dev/null || true
fi

if [ -f "$SERVICE_FILE" ]; then
    rm -f "$SERVICE_FILE"
    systemctl --user daemon-reload 2>/dev/null || true
    systemctl --user reset-failed 2>/dev/null || true
    echo -e "  • Service File: ${GREEN}Đã xóa khỏi systemd user${NC}"
fi

# 2. Dọn dẹp tiến trình
echo -e "\n${BOLD}[2/4] Kiểm tra và tắt các tiến trình còn lại...${NC}"
pkill -f "quota_daemon.py" 2>/dev/null || true
echo -e "  • Tiến trình:   ${GREEN}Đã dừng toàn bộ${NC}"

# 3. Xóa các tệp thực thi và thư viện cài đặt
echo -e "\n${BOLD}[3/4] Gỡ bỏ liên kết lệnh và thư mục cài đặt...${NC}"
rm -f "$BIN_DIR/esp_quota"
if [ -w "/usr/local/bin/esp_quota" ]; then
    rm -f "/usr/local/bin/esp_quota" 2>/dev/null || true
fi
rm -rf "$INSTALL_DIR"
echo -e "  • Lệnh thực thi: ${GREEN}Đã xóa ~/.local/bin/esp_quota${NC}"
echo -e "  • Thư mục mã:    ${GREEN}Đã xóa ~/.local/share/esp_quota${NC}"

# 4. Xử lý tệp cấu hình
echo -e "\n${BOLD}[4/4] Xử lý tệp cấu hình...${NC}"
PURGE=false
for arg in "$@"; do
    if [ "$arg" == "--purge" ] || [ "$arg" == "-p" ]; then
        PURGE=true
    fi
done

if [ "$PURGE" = true ]; then
    rm -rf "$CONFIG_DIR"
    echo -e "  • Cấu hình:     ${GREEN}Đã xóa toàn bộ cấu hình (--purge)${NC}"
else
    echo -e "  • Cấu hình:     ${YELLOW}Giữ lại $CONFIG_DIR (để không mất cấu hình nếu cài lại).${NC}"
    echo -e "                  (Dùng ${CYAN}./uninstall.sh --purge${NC} nếu muốn xóa sạch hoàn toàn)."
fi

echo -e "\n${GREEN}${BOLD}════════════════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}${BOLD}✅ GỠ CÀI ĐẶT THÀNH CÔNG! Dịch vụ đã dừng và đã dọn dẹp sạch sẽ.${NC}"
echo -e "${GREEN}${BOLD}════════════════════════════════════════════════════════════════════════${NC}\n"
