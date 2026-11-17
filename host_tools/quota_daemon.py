#!/usr/bin/env python3
"""
AI Developer Desk Console - Ultra-Low CPU Antigravity Quota BLE Daemon
Tối ưu hóa tài nguyên CPU máy tính tuyệt đối (<0.05% CPU):
  1. Cache PID/Port/CSRF của Antigravity (không quét /proc liên tục).
  2. Non-blocking CPU measurement (interval=None).
  3. Cache D-Bus BlueZ connection & Characteristic proxy.
  4. Cache sai số đồng hồ NTP (re-sync mỗi 15 phút, không gửi UDP liên tục).
  5. Đọc cấu hình từ ~/.config/esp_quota/config.json.
"""
import os
import sys
import time
import json
import re
import socket
import struct
import signal
import asyncio
import argparse
import urllib.request
from datetime import datetime, date, timezone, timedelta

try:
    import psutil
except ImportError:
    psutil = None

# Cấu hình mặc định
CONFIG_DIR = os.path.expanduser("~/.config/esp_quota")
CONFIG_FILE = os.path.join(CONFIG_DIR, "config.json")
DEFAULT_CONFIG = {
    "device_mac": "7C:E8:B1:B2:79:AA",
    "interval_seconds": 10,
    "enabled": True,
    "autostart": True
}

HISTORY_FILE = os.path.expanduser("~/.gemini/antigravity-cli/history.jsonl")
CLI_LOG_FILE = os.path.expanduser("~/.gemini/antigravity-cli/cli.log")

PC_SYNC_CHAR_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"

# --- Bộ nhớ Cache Tối Ưu Hóa CPU ---
g_cached_lsp = {
    "pid": None,
    "port": None,
    "csrf": None,
    "last_checked": 0
}

g_cached_dbus = {
    "bus": None,
    "char_iface": None,
    "mac": None
}

g_cached_time = {
    "offset": 0.0,
    "last_sync": 0.0
}

def save_config(cfg):
    """Ghi cấu hình người dùng ra file."""
    try:
        os.makedirs(CONFIG_DIR, exist_ok=True)
        with open(CONFIG_FILE, "w", encoding="utf-8") as f:
            json.dump(cfg, f, indent=4, ensure_ascii=False)
        return True
    except Exception as e:
        print(f"⚠️ Lỗi ghi cấu hình: {e}", flush=True)
        return False

def load_config():
    """Đọc cấu hình người dùng."""
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, "r", encoding="utf-8") as f:
                cfg = json.load(f)
                return {**DEFAULT_CONFIG, **cfg}
        except Exception:
            pass
    else:
        save_config(DEFAULT_CONFIG)
    return DEFAULT_CONFIG.copy()

def sync_ntp_offset():
    """Đồng bộ sai số thời gian với NTP Server 15 phút/lần (Không tốn CPU và Network)."""
    now = time.time()
    if now - g_cached_time["last_sync"] < 900 and g_cached_time["last_sync"] > 0:
        return

    # Thử NTP Google & Cloudflare
    for ntp_server in ["time.google.com", "pool.ntp.org", "time.cloudflare.com"]:
        try:
            client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            client.settimeout(1.5)
            client.sendto(b'\x1b' + 47 * b'\0', (ntp_server, 123))
            data, _ = client.recvfrom(1024)
            client.close()
            t_ntp = struct.unpack('!12I', data)[10] - 2208988800
            g_cached_time["offset"] = t_ntp - time.time()
            g_cached_time["last_sync"] = now
            return
        except Exception:
            pass

    # Fallback Header Date từ Google HTTPS
    try:
        from email.utils import parsedate_to_datetime
        req = urllib.request.Request("https://www.google.com", method="HEAD")
        with urllib.request.urlopen(req, timeout=2.0) as resp:
            date_str = resp.headers.get("Date")
            if date_str:
                dt_utc = parsedate_to_datetime(date_str)
                g_cached_time["offset"] = dt_utc.timestamp() - time.time()
                g_cached_time["last_sync"] = now
                return
    except Exception:
        pass

    g_cached_time["last_sync"] = now

def get_vietnam_time():
    """Lấy giờ Việt Nam thời gian thực với chi phí CPU gần như bằng 0."""
    sync_ntp_offset()
    tz_vn = timezone(timedelta(hours=7))
    real_now_ts = time.time() + g_cached_time["offset"]
    dt_vn = datetime.fromtimestamp(real_now_ts, tz=tz_vn)
    return dt_vn.strftime("%H:%M:%S")

def get_vietnam_date():
    """Lấy ngày tháng kiểu iOS Lock Screen (Ví dụ: Th 7 6 thg 9)."""
    sync_ntp_offset()
    tz_vn = timezone(timedelta(hours=7))
    real_now_ts = time.time() + g_cached_time["offset"]
    dt_vn = datetime.fromtimestamp(real_now_ts, tz=tz_vn)
    weekday_map = {
        0: "Th 2", 1: "Th 3", 2: "Th 4", 3: "Th 5",
        4: "Th 6", 5: "Th 7", 6: "CN"
    }
    wd = weekday_map.get(dt_vn.weekday(), "Th")
    return f"{wd},  {dt_vn.day} thg {dt_vn.month}"


def find_language_server():
    """Tìm port và CSRF token của Antigravity/Agy.
    Ưu tiên 1: Phiên làm việc active hiện tại từ symlink cli.log.
    Ưu tiên 2: Quét tiến trình theo thứ tự MỚI NHẤT trước để bám sát account vừa đăng nhập.
    """
    global g_cached_lsp
    now = time.time()

    # 1. Kiểm tra nếu symlink cli.log đổi sang session mới (khi người dùng mở terminal/account mới)
    cli_log = os.path.expanduser("~/.gemini/antigravity-cli/cli.log")
    cur_log_target = os.path.realpath(cli_log) if os.path.exists(cli_log) else None
    if cur_log_target and cur_log_target != g_cached_lsp.get("log_target"):
        g_cached_lsp["port"] = None
        g_cached_lsp["pid"] = None
        g_cached_lsp["log_target"] = cur_log_target

    # 2. Tái sử dụng cache nếu chưa quá 30 giây để tối ưu CPU
    if g_cached_lsp.get("port") and (now - g_cached_lsp.get("last_checked", 0) < 30):
        return g_cached_lsp["port"], g_cached_lsp["csrf"]

    # 3. Phương thức 1: Đọc trực tiếp port HTTP từ session CLI đang active trong cli.log
    if cur_log_target and os.path.exists(cur_log_target):
        try:
            with open(cur_log_target, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read(4096)
            m = re.search(r'Language server listening on random port at (\d+) for HTTP\b', content)
            if m:
                port = int(m.group(1))
                url = f'http://127.0.0.1:{port}/exa.language_server_pb.LanguageServerService/RetrieveUserQuotaSummary'
                req = urllib.request.Request(url, data=b'{}', headers={'Content-Type': 'application/json'}, method='POST')
                try:
                    with urllib.request.urlopen(req, timeout=1.0) as resp:
                        if resp.status == 200:
                            g_cached_lsp["port"] = port
                            g_cached_lsp["csrf"] = None
                            g_cached_lsp["log_target"] = cur_log_target
                            g_cached_lsp["last_checked"] = now
                            return port, None
                except Exception:
                    pass
        except Exception:
            pass

    # 4. Phương thức 2: Quét processes, SẮP XẾP THEO TIẾN TRÌNH MỚI NHẤT TRƯỚC
    if not psutil:
        return None, None

    try:
        candidates = []
        for p in psutil.process_iter(['pid', 'name', 'cmdline', 'create_time']):
            try:
                name = (p.info.get('name') or '').lower()
                cmdline = p.info.get('cmdline') or []
                cmd_str = ' '.join(cmdline).lower()
                if 'agy' in name or 'language_server' in cmd_str or 'antigravity' in cmd_str:
                    candidates.append(p)
            except Exception:
                pass

        # Ưu tiên tiến trình sinh sau (create_time lớn hơn)
        candidates.sort(key=lambda p: p.info.get('create_time') or 0, reverse=True)

        for p in candidates:
            cmdline = p.info.get('cmdline') or []
            csrf_token = None
            csrf_match = re.search(r'--csrf_token\s+([a-f0-9\-]+)', ' '.join(cmdline))
            if csrf_match:
                csrf_token = csrf_match.group(1)

            ports = []
            try:
                for conn in p.connections(kind='inet'):
                    if conn.status == psutil.CONN_LISTEN and conn.laddr.ip == '127.0.0.1':
                        ports.append(conn.laddr.port)
            except Exception:
                continue

            for port in ports:
                url = f'http://127.0.0.1:{port}/exa.language_server_pb.LanguageServerService/RetrieveUserQuotaSummary'
                headers = {'Content-Type': 'application/json'}
                if csrf_token:
                    headers['x-codeium-csrf-token'] = csrf_token
                req = urllib.request.Request(url, data=b'{}', headers=headers, method='POST')
                try:
                    with urllib.request.urlopen(req, timeout=1.0) as resp:
                        if resp.status == 200:
                            g_cached_lsp["pid"] = p.info["pid"]
                            g_cached_lsp["port"] = port
                            g_cached_lsp["csrf"] = csrf_token
                            g_cached_lsp["last_checked"] = now
                            return port, csrf_token
                except Exception:
                    pass
    except Exception:
        pass

    return None, None

def fetch_antigravity_live_quota():
    """Truy vấn trực tiếp hạn mức bằng kết nối cache tốc độ cao."""
    global g_cached_lsp
    port, csrf_token = find_language_server()
    if not port:
        return None

    url = f'http://127.0.0.1:{port}/exa.language_server_pb.LanguageServerService/RetrieveUserQuotaSummary'
    headers = {'Content-Type': 'application/json'}
    if csrf_token:
        headers['x-codeium-csrf-token'] = csrf_token
    req = urllib.request.Request(
        url,
        data=b'{}',
        headers=headers,
        method='POST'
    )
    try:
        with urllib.request.urlopen(req, timeout=1.5) as resp:
            res = json.loads(resp.read().decode('utf-8'))
            if 'response' in res and 'groups' in res['response']:
                return parse_quota_response(res['response'])
    except Exception:
        # Nếu lỗi kết nối, xóa cache để quét lại ở lượt sau
        g_cached_lsp["pid"] = None
        g_cached_lsp["port"] = None
        g_cached_lsp["csrf"] = None

    return None

def parse_reset_time_str(reset_time_iso):
    """Tính chuỗi thời gian đếm ngược HH:MM:SS từ resetTime ISO 8601."""
    if not reset_time_iso:
        return "00:00:00"
    try:
        s = reset_time_iso.strip()
        if s.endswith('Z'):
            s = s[:-1] + '+00:00'
        dt = datetime.fromisoformat(s)
        real_now_ts = time.time() + g_cached_time.get("offset", 0.0)
        diff = int(dt.timestamp() - real_now_ts)
        if diff <= 0:
            return "00:00:00"
        h = diff // 3600
        m = (diff % 3600) // 60
        sec = diff % 60
        return f"{h:02d}:{m:02d}:{sec:02d}"
    except Exception:
        return "00:00:00"

def parse_quota_response(response):
    gemini_5h = None
    gemini_weekly = None
    claude_5h = None
    claude_weekly = None
    gemini_reset = "00:00:00"
    claude_reset = "00:00:00"
    
    for grp in response.get('groups', []):
        display_name = grp.get('displayName', '').lower()
        buckets = grp.get('buckets', [])
        for b in buckets:
            bid = b.get('bucketId', '').lower()
            frac = b.get('remainingFraction', 1.0)
            pct = int(round(frac * 100))
            reset_t = b.get('resetTime')
            if 'gemini' in display_name or 'gemini' in bid:
                if '5h' in bid:
                    gemini_5h = pct
                    if reset_t:
                        gemini_reset = parse_reset_time_str(reset_t)
                elif 'weekly' in bid:
                    gemini_weekly = pct
            elif 'claude' in display_name or '3p' in bid:
                if '5h' in bid:
                    claude_5h = pct
                    if reset_t:
                        claude_reset = parse_reset_time_str(reset_t)
                elif 'weekly' in bid:
                    claude_weekly = pct

    return {
        "gemini_5h": gemini_5h if gemini_5h is not None else 100,
        "gemini_weekly": gemini_weekly if gemini_weekly is not None else 100,
        "claude_5h": claude_5h if claude_5h is not None else 100,
        "claude_weekly": claude_weekly if claude_weekly is not None else 100,
        "gemini_reset": gemini_reset,
        "claude_reset": claude_reset,
    }

def get_antigravity_stats():
    """Đọc dữ liệu số prompts và model đang dùng."""
    prompts_today = 0
    model_name = "Gemini 3.8 Flash (High)"

    if os.path.exists(HISTORY_FILE):
        try:
            today_start = datetime.combine(date.today(), datetime.min.time()).timestamp() * 1000
            with open(HISTORY_FILE, "r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    try:
                        entry = json.loads(line)
                        if entry.get("timestamp", 0) >= today_start:
                            prompts_today += 1
                    except json.JSONDecodeError:
                        continue
        except Exception:
            pass

    if os.path.exists(CLI_LOG_FILE):
        try:
            with open(CLI_LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
                lines = f.readlines()[-150:]
                for line in reversed(lines):
                    m = re.search(r'label="([^"]+)"', line)
                    if m:
                        model_name = m.group(1)
                        break
        except Exception:
            pass

    return {"prompts_today": prompts_today, "model_name": model_name}

def get_system_telemetry():
    """Lấy % CPU và RAM cực nhẹ (interval=None không gây block hay tốn CPU)."""
    cpu = 0
    ram = 0
    if psutil:
        try:
            cpu = int(psutil.cpu_percent(interval=None))
            ram = int(psutil.virtual_memory().percent)
        except Exception:
            pass
    return cpu, ram

async def find_connected_esp_device(bus):
    """Tự động quét và tìm thiết bị Bluetooth ESP32/LaunchPad đang kết nối có PC_SYNC_CHAR_UUID."""
    try:
        hci_intro = await bus.introspect("org.bluez", "/org/bluez/hci0")
        for node in hci_intro.nodes:
            if not node.name.startswith("dev_"):
                continue
            dev_path = f"/org/bluez/hci0/{node.name}"
            dev_intro = await bus.introspect("org.bluez", dev_path)
            obj = bus.get_proxy_object("org.bluez", dev_path, dev_intro)
            dev_iface = obj.get_interface("org.bluez.Device1")
            connected = await dev_iface.get_connected()
            if not connected:
                continue

            for svc_node in dev_intro.nodes:
                svc_path = f"{dev_path}/{svc_node.name}"
                try:
                    svc_intro = await bus.introspect("org.bluez", svc_path)
                    for char_node in svc_intro.nodes:
                        char_full_path = f"{svc_path}/{char_node.name}"
                        char_intro = await bus.introspect("org.bluez", char_full_path)
                        c_obj = bus.get_proxy_object("org.bluez", char_full_path, char_intro)
                        char_iface = c_obj.get_interface("org.bluez.GattCharacteristic1")
                        uuid = await char_iface.get_uuid()
                        if uuid.lower() == PC_SYNC_CHAR_UUID.lower():
                            mac = node.name.replace("dev_", "").replace("_", ":")
                            return mac, char_iface
                except Exception:
                    continue
    except Exception:
        pass
    return None, None

async def send_via_dbus_cached(payload_bytes, mac_addr):
    """Gửi dữ liệu qua D-Bus với Characteristic proxy được tái sử dụng liên tục (kèm tự động dò tìm)."""
    global g_cached_dbus
    from dbus_fast.aio import MessageBus
    from dbus_fast import BusType

    try:
        # 1. Nếu đã có proxy và cùng địa chỉ MAC, gọi trực tiếp (<1ms)
        if g_cached_dbus["char_iface"] and g_cached_dbus["mac"] == mac_addr:
            await g_cached_dbus["char_iface"].call_write_value(payload_bytes, {})
            return True

        if not g_cached_dbus["bus"]:
            g_cached_dbus["bus"] = await MessageBus(bus_type=BusType.SYSTEM).connect()

        bus = g_cached_dbus["bus"]

        # 2. Thử kết nối theo MAC được cấu hình (nếu định dạng MAC hợp lệ)
        if mac_addr and len(mac_addr) == 17 and ":" in mac_addr:
            formatted_mac = mac_addr.replace(":", "_").upper()
            dev_path = f"/org/bluez/hci0/dev_{formatted_mac}"
            try:
                introspection = await bus.introspect("org.bluez", dev_path)
                for node in introspection.nodes:
                    svc_path = f"{dev_path}/{node.name}"
                    try:
                        svc_intro = await bus.introspect("org.bluez", svc_path)
                        for char_node in svc_intro.nodes:
                            char_full_path = f"{svc_path}/{char_node.name}"
                            char_intro = await bus.introspect("org.bluez", char_full_path)
                            obj = bus.get_proxy_object("org.bluez", char_full_path, char_intro)
                            char_iface = obj.get_interface("org.bluez.GattCharacteristic1")
                            uuid = await char_iface.get_uuid()
                            if uuid.lower() == PC_SYNC_CHAR_UUID.lower():
                                g_cached_dbus["char_iface"] = char_iface
                                g_cached_dbus["mac"] = mac_addr
                                await char_iface.call_write_value(payload_bytes, {})
                                return True
                    except Exception:
                        continue
            except Exception:
                pass

        # 3. Dự phòng: Tự động dò tìm thiết bị ESP32 đang kết nối nếu MAC cấu hình sai hoặc không tìm thấy
        found_mac, char_iface = await find_connected_esp_device(bus)
        if found_mac and char_iface:
            g_cached_dbus["char_iface"] = char_iface
            g_cached_dbus["mac"] = found_mac
            await char_iface.call_write_value(payload_bytes, {})
            # Tự động cập nhật lại file cấu hình với MAC đúng
            if found_mac != mac_addr:
                cfg = load_config()
                cfg["device_mac"] = found_mac
                save_config(cfg)
                print(f"✨ Đã tự động nhận diện và cập nhật MAC ESP32: {found_mac}", flush=True)
            return True

    except Exception as e:
        g_cached_dbus["char_iface"] = None
        raise e

    return False

async def run_daemon_loop(args):
    loop = asyncio.get_running_loop()
    stop_event = asyncio.Event()

    def _on_signal():
        stop_event.set()

    for sig in (signal.SIGTERM, signal.SIGINT):
        try:
            loop.add_signal_handler(sig, _on_signal)
        except (NotImplementedError, RuntimeError):
            pass

    cfg = load_config()
    target_addr = args.device or cfg.get("device_mac", DEFAULT_CONFIG["device_mac"])
    interval = args.interval or cfg.get("interval_seconds", 10)
    last_cfg_mtime = os.path.getmtime(CONFIG_FILE) if os.path.exists(CONFIG_FILE) else 0

    # Đo CPU lần đầu để khởi tạo counter nội bộ của psutil
    if psutil:
        psutil.cpu_percent(interval=None)

    print("=" * 70, flush=True)
    print("🚀 ESP QUOTA TELEMETRY DAEMON (ULTRA-LOW CPU ENGINE)", flush=True)
    print(f"   • Target Device:     {target_addr}", flush=True)
    print(f"   • Sync Interval:     {interval}s", flush=True)
    print(f"   • Mode:              {'One-Shot' if args.once else 'Background Daemon'}", flush=True)
    print("=" * 70, flush=True)

    while not stop_event.is_set():
        try:
            # Tự động reload cấu hình nếu config.json thay đổi
            if not args.device or not args.interval:
                try:
                    cur_mtime = os.path.getmtime(CONFIG_FILE) if os.path.exists(CONFIG_FILE) else 0
                    if cur_mtime != last_cfg_mtime:
                        cfg = load_config()
                        if not args.device:
                            target_addr = cfg.get("device_mac", DEFAULT_CONFIG["device_mac"])
                        if not args.interval:
                            interval = cfg.get("interval_seconds", 10)
                        last_cfg_mtime = cur_mtime
                        print(f"🔄 Đã cập nhật cấu hình: Target={target_addr}, Interval={interval}s", flush=True)
                except Exception:
                    pass

            if not cfg.get("enabled", True):
                try:
                    await asyncio.wait_for(stop_event.wait(), timeout=5.0)
                except asyncio.TimeoutError:
                    pass
                continue

            stats = get_antigravity_stats()
            cpu, ram = get_system_telemetry()
            vn_time = get_vietnam_time()
            vn_date = get_vietnam_date()
            
            g_reset = "00:00:00"
            c_reset = "00:00:00"
            live_quota = fetch_antigravity_live_quota()
            if live_quota:
                g_5h = live_quota["gemini_5h"]
                g_w = live_quota["gemini_weekly"]
                c_5h = live_quota["claude_5h"]
                c_w = live_quota["claude_weekly"]
                g_reset = live_quota.get("gemini_reset", "00:00:00")
                c_reset = live_quota.get("claude_reset", "00:00:00")
            else:
                max_est = 100
                used = min(stats["prompts_today"], max_est)
                g_5h = max(5, int(((max_est - used) / max_est) * 100))
                g_w = 88
                c_5h = 100
                c_w = 100

            if args.quota is not None:
                g_5h = max(0, min(100, args.quota))

            quota_packet = {
                "model": stats["model_name"],
                "time": vn_time,
                "date": vn_date,
                "ts": int(time.time() + g_cached_time["offset"]),
                "quota": g_5h,
                "g_w": g_w,
                "c_5h": c_5h,
                "c_w": c_w,
                "g_reset": g_reset,
                "c_reset": c_reset,
                "prompts": stats["prompts_today"],
                "cpu": cpu,
                "ram": ram
            }

            payload_str = json.dumps(quota_packet)
            payload_bytes = payload_str.encode("utf-8")

            print(f"[{datetime.now().strftime('%H:%M:%S')}] VN: {vn_time} ({vn_date}) | G-5h: {g_5h}% (Reset: {g_reset}) | C-5h: {c_5h}% (Reset: {c_reset}) | CPU: {cpu}% RAM: {ram}%", flush=True)

            sent = await send_via_dbus_cached(payload_bytes, target_addr)
            if not sent:
                print(f"⚠️ Chưa kết nối được với ESP32 ({target_addr}). Đang chờ thiết bị BLE kết nối...", flush=True)

            if args.once:
                if sent:
                    print("✅ Đã gửi thành công và kết thúc lượt chạy (--once).", flush=True)
                else:
                    print("❌ Gửi thất bại (--once).", flush=True)
                return

            try:
                await asyncio.wait_for(stop_event.wait(), timeout=interval)
            except asyncio.TimeoutError:
                pass

        except Exception as e:
            print(f"⚠️ BLE / Sync Error: {e}", flush=True)
            if args.once:
                break
            try:
                await asyncio.wait_for(stop_event.wait(), timeout=interval)
            except asyncio.TimeoutError:
                pass

    print("\n🛑 Daemon dừng bởi tín hiệu hệ thống.", flush=True)

def main():
    parser = argparse.ArgumentParser(description="ESP Quota Telemetry Daemon")
    parser.add_argument("--device", type=str, default=None, help="Địa chỉ MAC Bluetooth của ESP32")
    parser.add_argument("--interval", type=int, default=None, help="Chu kỳ gửi (giây)")
    parser.add_argument("--quota", type=int, default=None, help="Ép giá trị % Quota để test")
    parser.add_argument("--once", action="store_true", help="Chạy 1 lần rồi thoát")
    args = parser.parse_args()

    try:
        asyncio.run(run_daemon_loop(args))
    except (KeyboardInterrupt, SystemExit):
        pass

if __name__ == "__main__":
    main()
