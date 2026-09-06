# 🖥️ Host Tools - PC Quota & Telemetry Daemon

Thư mục này chứa các công cụ chạy trên máy tính cá nhân (Linux / Windows / macOS) để phục vụ việc giao tiếp với ESP32 Desk Console:

- **`quota_daemon.py`**: Script chạy ngầm đọc token / quota Claude Code & Antigravity và chỉ số hệ thống (CPU, RAM), định kỳ gửi xuống ESP32 qua cổng Serial USB Type-C.
- **`requirements.txt`**: Các thư viện Python cần thiết (`pyserial`, `psutil`).
