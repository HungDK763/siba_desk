#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

echo "🐾 =========================================="
echo "🐾  BONGO SHIBA PIXEL EDITOR & ANIMATOR"
echo "🐾 =========================================="
echo ""
echo "🚀 Đang khởi động server tại cổng 8088..."
echo "🌐 Vui lòng mở trình duyệt tại: http://localhost:8088"
echo "⏹️  Nhấn Ctrl+C để dừng server."
echo ""

# Kiểm tra python3 và Pillow
if ! python3 -c "import PIL" 2>/dev/null; then
    echo "⚠️  Thư viện 'Pillow' chưa được cài đặt. Đang tiến hành cài đặt..."
    pip3 install -r requirements.txt || pip install -r requirements.txt
fi

python3 server.py
