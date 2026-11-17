#!/usr/bin/env bash
# ==============================================================================
# ESP32 LaunchPad Display Studio - Quick Launcher
# Tự động phát hiện môi trường:
# - Môi trường Desktop (GUI): Khởi chạy Native Desktop Window (GTK3 + WebKit2)
# - Môi trường Console / Headless: Khởi chạy HTTP Server và mở Browser
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT=8088
URL="http://localhost:${PORT}"

echo "=================================================================="
echo "   🎨 ESP32 LAUNCHPAD DISPLAY STUDIO - KHỞI ĐỘNG ỨNG DỤNG"
echo "=================================================================="

# Kiểm tra Python 3
if ! command -v python3 &> /dev/null; then
    echo "❌ Lỗi: Không tìm thấy python3 trên hệ thống!"
    exit 1
fi

# Kiểm tra thư viện Pillow
python3 -c "import PIL" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "⚠️ Đang cài đặt thư viện Pillow cần thiết..."
    pip install Pillow || pip3 install Pillow
fi

# Kiểm tra và giải phóng port 8088 nếu đang bị chiếm dụng bởi tiến trình cũ
PID_PORT=$(lsof -ti :${PORT} 2>/dev/null)
if [ -n "$PID_PORT" ]; then
    echo "⚠️ Port ${PORT} đang được sử dụng (PID: $PID_PORT). Đang giải phóng..."
    kill -9 $PID_PORT 2>/dev/null
    sleep 0.5
fi

# Chế độ ép buộc qua cờ lệnh nếu có:
# ./run.sh --server  -> chỉ chạy server backend
# ./run.sh --browser -> mở bằng trình duyệt web
MODE="auto"
if [ "$1" == "--server" ]; then
    MODE="server"
elif [ "$1" == "--browser" ]; then
    MODE="browser"
fi

if [ "$MODE" != "server" ] && [ "$MODE" != "browser" ] && { [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; }; then
    echo "🖥️ Phát hiện môi trường đồ họa ($DISPLAY $WAYLAND_DISPLAY)."
    echo "🚀 Đang khởi động Native Desktop App..."
    exec python3 "${SCRIPT_DIR}/app_desktop.py"
else
    echo "🌐 Đang khởi chạy chế độ Web Server trên cổng ${PORT}..."
    echo "👉 Đường dẫn: ${URL}"
    echo "------------------------------------------------------------------"
    if [ "$MODE" != "server" ]; then
        (
            sleep 1.2
            if command -v xdg-open &> /dev/null; then
                xdg-open "$URL" &> /dev/null &
            elif command -v google-chrome &> /dev/null; then
                google-chrome "$URL" &> /dev/null &
            elif command -v firefox &> /dev/null; then
                firefox "$URL" &> /dev/null &
            fi
        ) &
    fi
    exec python3 "${SCRIPT_DIR}/server.py"
fi
