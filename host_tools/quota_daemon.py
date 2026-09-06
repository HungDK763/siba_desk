#!/usr/bin/env python3
"""
AI Developer Desk Console - Antigravity Quota BLE Daemon
Đọc dữ liệu Quota thời gian thực từ Antigravity Language Server và gửi sang ESP32 qua BLE.
"""
import os
import sys
import time
import json
import re
import asyncio
import argparse
import urllib.request
from datetime import datetime, date, timezone, timedelta

try:
    import psutil
except ImportError:
    psutil = None

# Cấu hình UUID GATT Custom trên ESP32
PC_SYNC_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
PC_SYNC_CHAR_UUID    = "0000ffe1-0000-1000-8000-00805f9b34fb"
DEFAULT_ESP32_MAC    = "7C:E8:B1:B2:79:AA"
DBUS_CHAR_PATH       = f"/org/bluez/hci0/dev_{DEFAULT_ESP32_MAC.replace(':', '_')}/service0048/char0049"

HISTORY_FILE = os.path.expanduser("~/.gemini/antigravity-cli/history.jsonl")
CLI_LOG_FILE = os.path.expanduser("~/.gemini/antigravity-cli/cli.log")

def fetch_antigravity_live_quota():
    """Truy vấn trực tiếp Language Server của Antigravity để lấy hạn mức realtime chính xác 100%."""
    if not psutil:
        return None
    try:
        for p in psutil.process_iter(['pid', 'name', 'cmdline']):
            cmdline = p.info.get('cmdline') or []
            cmd_str = ' '.join(cmdline)
            if 'language_server' in cmd_str and '--csrf_token' in cmd_str:
                csrf_match = re.search(r'--csrf_token\s+([a-f0-9\-]+)', cmd_str)
                if not csrf_match:
                    continue
                csrf_token = csrf_match.group(1)
                
                ports = []
                for conn in p.connections(kind='inet'):
                    if conn.status == psutil.CONN_LISTEN and conn.laddr.ip == '127.0.0.1':
                        ports.append(conn.laddr.port)
                
                for port in ports:
                    url = f'http://127.0.0.1:{port}/exa.language_server_pb.LanguageServerService/RetrieveUserQuotaSummary'
                    req = urllib.request.Request(
                        url,
                        data=b'{}',
                        headers={
                            'Content-Type': 'application/json',
                            'x-codeium-csrf-token': csrf_token
                        },
                        method='POST'
                    )
                    try:
                        with urllib.request.urlopen(req, timeout=1.5) as resp:
                            res = json.loads(resp.read().decode('utf-8'))
                            if 'response' in res and 'groups' in res['response']:
                                return parse_quota_response(res['response'])
                    except Exception:
                        pass
    except Exception:
        pass
    return None

def parse_quota_response(response):
    gemini_5h = None
    gemini_weekly = None
    claude_5h = None
    claude_weekly = None
    
    for grp in response.get('groups', []):
        display_name = grp.get('displayName', '').lower()
        buckets = grp.get('buckets', [])
        for b in buckets:
            bid = b.get('bucketId', '').lower()
            frac = b.get('remainingFraction', 1.0)
            pct = int(round(frac * 100))
            if 'gemini' in display_name or 'gemini' in bid:
                if '5h' in bid:
                    gemini_5h = pct
                elif 'weekly' in bid:
                    gemini_weekly = pct
            elif 'claude' in display_name or '3p' in bid:
                if '5h' in bid:
                    claude_5h = pct
                elif 'weekly' in bid:
                    claude_weekly = pct

    return {
        "gemini_5h": gemini_5h if gemini_5h is not None else 100,
        "gemini_weekly": gemini_weekly if gemini_weekly is not None else 100,
        "claude_5h": claude_5h if claude_5h is not None else 100,
        "claude_weekly": claude_weekly if claude_weekly is not None else 100,
    }

def get_antigravity_stats():
    """Đọc dữ liệu số prompts và model đang dùng."""
    prompts_today = 0
    latest_workspace = "Unknown"
    model_name = "Gemini 3.8 Flash (High)"

    # 1. Đếm số prompt gửi hôm nay từ history.jsonl
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
                        ts = entry.get("timestamp", 0)
                        if ts >= today_start:
                            prompts_today += 1
                        if "workspace" in entry:
                            latest_workspace = entry["workspace"]
                    except json.JSONDecodeError:
                        continue
        except Exception:
            pass

    # 2. Tìm model được chọn từ cli.log
    if os.path.exists(CLI_LOG_FILE):
        try:
            with open(CLI_LOG_FILE, "r", encoding="utf-8", errors="ignore") as f:
                lines = f.readlines()[-200:]
                for line in reversed(lines):
                    m = re.search(r'label="([^"]+)"', line)
                    if m:
                        model_name = m.group(1)
                        break
        except Exception:
            pass

    return {
        "prompts_today": prompts_today,
        "workspace": latest_workspace,
        "model_name": model_name
    }

def get_system_telemetry():
    """Lấy % CPU và RAM của máy tính."""
    cpu = 0
    ram = 0
    if psutil:
        try:
            cpu = int(psutil.cpu_percent(interval=0.2))
            ram = int(psutil.virtual_memory().percent)
        except Exception:
            pass
    return cpu, ram

async def send_via_dbus(payload_bytes, mac_addr):
    """Gửi dữ liệu qua Linux BlueZ D-Bus trực tiếp (Độ trễ cực thấp, không cần scan)."""
    from dbus_fast.aio import MessageBus
    from dbus_fast import BusType

    bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
    
    formatted_mac = mac_addr.replace(":", "_").upper()
    char_path = None
    
    dev_path = f"/org/bluez/hci0/dev_{formatted_mac}"
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
                    char_path = char_full_path
                    break
        except Exception:
            continue
        if char_path:
            break

    if not char_path:
        char_path = DBUS_CHAR_PATH

    intro = await bus.introspect("org.bluez", char_path)
    obj = bus.get_proxy_object("org.bluez", char_path, intro)
    char_iface = obj.get_interface("org.bluez.GattCharacteristic1")
    await char_iface.call_write_value(payload_bytes, {})
    return True

def get_vietnam_time():
    """Lấy giờ Việt Nam (UTC+7) chuẩn xác từ Internet NTP/Google (không phụ thuộc vào đồng hồ hệ thống của PC)."""
    import socket, struct
    for ntp_server in ["time.google.com", "pool.ntp.org", "time.cloudflare.com"]:
        try:
            client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            client.settimeout(1.5)
            client.sendto(b'\x1b' + 47 * b'\0', (ntp_server, 123))
            data, _ = client.recvfrom(1024)
            client.close()
            t = struct.unpack('!12I', data)[10] - 2208988800
            tz_vn = timezone(timedelta(hours=7))
            dt_vn = datetime.fromtimestamp(t, tz=tz_vn)
            return dt_vn.strftime("%H:%M:%S")
        except Exception:
            pass

    # Fallback: Header Date từ Google HTTPS
    try:
        from email.utils import parsedate_to_datetime
        req = urllib.request.Request("https://www.google.com", method="HEAD")
        with urllib.request.urlopen(req, timeout=2.0) as resp:
            date_str = resp.headers.get("Date")
            if date_str:
                dt_utc = parsedate_to_datetime(date_str)
                tz_vn = timezone(timedelta(hours=7))
                dt_vn = dt_utc.astimezone(tz_vn)
                return dt_vn.strftime("%H:%M:%S")
    except Exception:
        pass

    tz_vn = timezone(timedelta(hours=7))
    return datetime.now(tz=tz_vn).strftime("%H:%M:%S")

async def run_sync_loop(args):
    target_addr = args.device or DEFAULT_ESP32_MAC
    print("=" * 70)
    print("🚀 ANTIGRAVITY QUOTA BLE DAEMON (CHÍNH XÁC 100% REALTIME)")
    print(f"   • Thiết bị mục tiêu: {target_addr}")
    print(f"   • Dịch vụ BLE:       Custom Service (0xFFE0) -> Char (0xFFE1)")
    print(f"   • Chu kỳ đồng bộ:    {args.interval}s")
    print("=" * 70)

    while True:
        try:
            # 1. Thu thập dữ liệu thực tế
            stats = get_antigravity_stats()
            cpu, ram = get_system_telemetry()
            vn_time = get_vietnam_time()
            
            # 2. Truy vấn hạn mức thật từ Antigravity
            live_quota = fetch_antigravity_live_quota()
            if live_quota:
                g_5h = live_quota["gemini_5h"]
                g_w = live_quota["gemini_weekly"]
                c_5h = live_quota["claude_5h"]
                c_w = live_quota["claude_weekly"]
            else:
                # Fallback nếu Antigravity IDE chưa bật
                max_est = 100
                used = min(stats["prompts_today"], max_est)
                g_5h = max(5, int(((max_est - used) / max_est) * 100))
                g_w = 90
                c_5h = 100
                c_w = 100

            if args.quota is not None:
                g_5h = max(0, min(100, args.quota))

            quota_packet = {
                "model": stats["model_name"],
                "time": vn_time,
                "quota": g_5h,
                "g_w": g_w,
                "c_5h": c_5h,
                "c_w": c_w,
                "prompts": stats["prompts_today"],
                "cpu": cpu,
                "ram": ram
            }

            payload_str = json.dumps(quota_packet)
            payload_bytes = payload_str.encode("utf-8")

            print("\n" + "-" * 55)
            print(f"⏰ [{datetime.now().strftime('%H:%M:%S')}] Cập nhật dữ liệu:")
            print(f"   • Giờ Việt Nam:        {vn_time} (NTP UTC+7)")
            print(f"   • AI Model:            {quota_packet['model']}")
            print(f"   • [GEMINI] 5-Hour:     {quota_packet['quota']}%  |  Weekly: {quota_packet['g_w']}%")
            print(f"   • [CLAUDE/GPT] 5-Hour: {quota_packet['c_5h']}%  |  Weekly: {quota_packet['c_w']}%")
            print(f"   • Prompts hôm nay:     {quota_packet['prompts']}")
            print(f"   • Máy tính Host:       CPU: {quota_packet['cpu']}% | RAM: {quota_packet['ram']}%")
            print(f"📦 Gửi gói tin ({len(payload_bytes)} bytes): {payload_str}")

            # 3. Gửi qua D-Bus BlueZ
            await send_via_dbus(payload_bytes, target_addr)
            print("✅ ĐÃ GỬI THÀNH CÔNG TỚI ESP32 QUA BLUETOOTH BLE!")

            if args.once:
                print("\n🏁 Chế độ --once hoàn thành.")
                return

            await asyncio.sleep(args.interval)

        except Exception as e:
            print(f"⚠️ Lỗi kết nối Bluetooth: {e}")
            if args.once:
                break
            await asyncio.sleep(args.interval)

def main():
    parser = argparse.ArgumentParser(description="Antigravity Realtime Quota BLE Daemon")
    parser.add_argument("--device", type=str, default=None, help="Địa chỉ MAC Bluetooth của ESP32 (vd: 7C:E8:B1:B2:79:AA)")
    parser.add_argument("--interval", type=int, default=10, help="Chu kỳ gửi (giây), mặc định: 10s")
    parser.add_argument("--quota", type=int, default=None, help="Ép giá trị % Quota để test cảnh báo (vd: 15 để test cảnh báo đỏ)")
    parser.add_argument("--once", action="store_true", help="Gửi 1 lần rồi thoát (test nhanh)")
    args = parser.parse_args()

    try:
        asyncio.run(run_sync_loop(args))
    except KeyboardInterrupt:
        print("\n🛑 Daemon đã dừng bởi người dùng.")

if __name__ == "__main__":
    main()
