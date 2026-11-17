#!/usr/bin/env python3
"""
ESP32 LaunchPad Display Studio - Native Desktop Application
Sử dụng GTK 3 & WebKit2 để chạy giao diện Studio như ứng dụng Desktop độc lập.
"""

import os
import sys
import time
import socket
import threading
import signal
import urllib.request

# Định vị thư mục ứng dụng
APP_DIR = os.path.dirname(os.path.abspath(__file__))
ICON_PATH = os.path.join(APP_DIR, "icon.png")
PORT = 8088
URL = f"http://127.0.0.1:{PORT}"

def is_server_ready(port=PORT, timeout=0.2):
    """Kiểm tra server port có sẵn sàng nhận kết nối không."""
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=timeout):
            return True
    except (socket.timeout, ConnectionRefusedError, OSError):
        return False

def start_backend_server():
    """Khởi động server backend trong một tiến trình phụ hoặc daemon thread."""
    if is_server_ready():
        print(f"[Desktop App] Server backend đã đang chạy trên cổng {PORT}.")
        return

    print(f"[Desktop App] Đang khởi động Server backend trên cổng {PORT}...")
    # Import server module và chạy trong thread daemon
    sys.path.insert(0, APP_DIR)
    import server

    t = threading.Thread(target=server.run_server, daemon=True)
    t.start()

    # Đợi tối đa 5 giây cho server sẵn sàng
    for _ in range(50):
        if is_server_ready():
            print("[Desktop App] Server backend đã sẵn sàng!")
            return
        time.sleep(0.1)

    print("[Desktop App] Cảnh báo: Server chưa phản hồi kịp thời, tiếp tục tải giao diện...")

def main():
    # Kiểm tra biến môi trường DISPLAY
    display = os.environ.get("DISPLAY")
    wayland = os.environ.get("WAYLAND_DISPLAY")
    if not display and not wayland:
        print("❌ Lỗi: Không tìm thấy phiên màn hình hiển thị ($DISPLAY hoặc $WAYLAND_DISPLAY).")
        print("   Đang chuyển hướng sang khởi động Server chế độ Headless...")
        sys.path.insert(0, APP_DIR)
        import server
        server.run_server()
        return

    # Khởi động backend trước
    start_backend_server()

    # Import GTK và WebKit2
    try:
        import gi
        gi.require_version("Gtk", "3.0")
        # Thử WebKit2 4.1 trước (Ubuntu 22.04+), fallback sang 4.0
        try:
            gi.require_version("WebKit2", "4.1")
        except ValueError:
            gi.require_version("WebKit2", "4.0")

        from gi.repository import Gtk, Gdk, WebKit2, GLib
    except Exception as e:
        print(f"❌ Không thể khởi tạo thư viện GTK/WebKit2: {e}")
        print("👉 Đang mở qua trình duyệt mặc định...")
        import webbrowser
        webbrowser.open(URL)
        # Giữ process server chạy
        while True:
            time.sleep(1)

    # Khởi tạo GTK Application Window
    class StudioDesktopWindow(Gtk.Window):
        def __init__(self):
            super().__init__(title="ESP32 LaunchPad Display Studio")
            self.set_default_size(1400, 860)
            self.set_position(Gtk.WindowPosition.CENTER)

            # Đặt icon nếu tồn tại
            if os.path.exists(ICON_PATH):
                try:
                    self.set_icon_from_file(ICON_PATH)
                except Exception as ex:
                    print(f"Không thể tải icon: {ex}")

            # Cấu hình WebKit Settings
            settings = WebKit2.Settings()
            settings.set_enable_developer_extras(True)
            settings.set_enable_javascript(True)
            settings.set_enable_webgl(True)
            settings.set_enable_smooth_scrolling(True)
            settings.set_javascript_can_access_clipboard(True)

            # Web View
            self.webview = WebKit2.WebView.new_with_settings(settings)
            self.webview.load_uri(URL)

            # Bọc trong ScrolledWindow để cuộn mượt mà nếu cần
            scrolled = Gtk.ScrolledWindow()
            scrolled.add(self.webview)
            self.add(scrolled)

            # Xử lý sự kiện đóng cửa sổ
            self.connect("destroy", self.on_destroy)
            self.connect("delete-event", self.on_delete)

        def on_delete(self, widget, event):
            return False

        def on_destroy(self, widget):
            Gtk.main_quit()
            sys.exit(0)

    # Đăng ký signal handler để Ctrl+C tắt sạch
    signal.signal(signal.SIGINT, lambda sig, frame: Gtk.main_quit())

    window = StudioDesktopWindow()
    window.show_all()
    print("✨ Cửa sổ Desktop Native của ESP32 Display Studio đã mở!")
    Gtk.main()

if __name__ == "__main__":
    main()
