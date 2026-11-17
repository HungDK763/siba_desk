#!/usr/bin/env python3
"""
ESP32 LaunchPad Display Studio - Backend Server
Cung cấp API chuyển đổi hình ảnh, tạo pixel art, animation GIF,
sinh mã C chuẩn LVGL (hỗ trợ CONFIG_LV_COLOR_16_SWAP=y), và nạp firmware trực tiếp vào thiết bị.
"""

import os
import sys
import io
import json
import base64
import subprocess
import threading
import http.server
import socketserver
import urllib.parse
from PIL import Image, ImageOps, ImageEnhance

PORT = 8088
STUDIO_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.abspath(os.path.join(STUDIO_DIR, "../.."))
DISPLAY_UI_DIR = os.path.join(PROJECT_DIR, "components/display_ui")
PREVIEW_DIR = os.path.join(PROJECT_DIR, "docs_for_me/bongo_dog_preview")
IDF_EXPORT = "/home/hungdk4/.espressif/v5.3.5/esp-idf/export.sh"

# Trạng thái tiến trình Flash đang chạy
g_flash_state = {
    "running": False,
    "log": [],
    "success": None
}

def img_to_b64(img, fmt="PNG"):
    """Chuyển đổi PIL Image sang chuỗi base64 data URI."""
    buf = io.BytesIO()
    img.save(buf, format=fmt)
    return "data:image/png;base64," + base64.b64encode(buf.getvalue()).decode("utf-8")

def b64_to_img(b64_str):
    """Chuyển chuỗi base64 thành PIL Image RGBA."""
    if "," in b64_str:
        b64_str = b64_str.split(",", 1)[1]
    raw = base64.b64decode(b64_str)
    return Image.open(io.BytesIO(raw)).convert("RGBA")

def apply_floyd_steinberg_rgb565(img):
    """
    Khuếch tán sai số Floyd-Steinberg cho không gian màu RGB565 (16-bit).
    Tạo hiệu ứng mịn màng cho màn hình LCD ILI9341, khử hoàn toàn vệt banding.
    """
    w, h = img.size
    # Chuyển sang mảng float để tính sai số
    rgba_data = list(img.getdata())
    r_buf = [p[0] for p in rgba_data]
    g_buf = [p[1] for p in rgba_data]
    b_buf = [p[2] for p in rgba_data]
    a_buf = [p[3] for p in rgba_data]

    def get_idx(x, y):
        return y * w + x

    out_img = Image.new("RGBA", (w, h))
    out_pixels = []

    for y in range(h):
        for x in range(w):
            idx = get_idx(x, y)
            a = a_buf[idx]
            if a < 32:
                out_pixels.append((0, 0, 0, 0))
                continue

            old_r = max(0, min(255, r_buf[idx]))
            old_g = max(0, min(255, g_buf[idx]))
            old_b = max(0, min(255, b_buf[idx]))

            # Lượng tử hóa về 5-6-5 bits
            new_r = round((old_r >> 3) * 255.0 / 31.0)
            new_g = round((old_g >> 2) * 255.0 / 63.0)
            new_b = round((old_b >> 3) * 255.0 / 31.0)

            out_pixels.append((new_r, new_g, new_b, a))

            err_r = old_r - new_r
            err_g = old_g - new_g
            err_b = old_b - new_b

            # Khuếch tán sai số sang 4 pixel lân cận (Floyd-Steinberg)
            neighbors = [
                (x + 1, y,     7 / 16.0),
                (x - 1, y + 1, 3 / 16.0),
                (x,     y + 1, 5 / 16.0),
                (x + 1, y + 1, 1 / 16.0)
            ]
            for nx, ny, weight in neighbors:
                if 0 <= nx < w and 0 <= ny < h:
                    n_idx = get_idx(nx, ny)
                    if a_buf[n_idx] >= 32:
                        r_buf[n_idx] += int(err_r * weight)
                        g_buf[n_idx] += int(err_g * weight)
                        b_buf[n_idx] += int(err_b * weight)

    out_img.putdata(out_pixels)
    return out_img

def convert_to_rgb565_exact(img):
    """Lượng tử hóa chuẩn RGB565 (Không dithering)."""
    w, h = img.size
    out_img = Image.new("RGBA", (w, h))
    pixels = []
    for p in img.getdata():
        if p[3] < 32:
            pixels.append((0, 0, 0, 0))
        else:
            r5 = (p[0] >> 3) & 0x1F
            g6 = (p[1] >> 2) & 0x3F
            b5 = (p[2] >> 3) & 0x1F
            r_norm = round(r5 * 255.0 / 31.0)
            g_norm = round(g6 * 255.0 / 63.0)
            b_norm = round(b5 * 255.0 / 31.0)
            pixels.append((r_norm, g_norm, b_norm, p[3]))
    out_img.putdata(pixels)
    return out_img

def remove_background_flood_edge(img, target_color, thresh=25):
    """
    Xóa màu nền liên tục xuất phát từ các cạnh biên và 4 góc (Magic Wand từ viền).
    Không làm thủng các chi tiết cùng màu ở bên trong nhân vật/sprite.
    """
    img = img.convert("RGBA")
    w, h = img.size
    pixels = img.load()
    tr, tg, tb = target_color[:3]
    visited = bytearray(w * h)

    def is_match(x, y):
        p = pixels[x, y]
        if p[3] == 0:
            return True
        diff = abs(p[0] - tr) + abs(p[1] - tg) + abs(p[2] - tb)
        return diff <= thresh * 3

    queue = []
    # Biên ngang trên và dưới
    for x in range(w):
        if is_match(x, 0):
            queue.append((x, 0))
            visited[0 * w + x] = 1
        if is_match(x, h - 1):
            queue.append((x, h - 1))
            visited[(h - 1) * w + x] = 1
    # Biên dọc trái và phải
    for y in range(h):
        if is_match(0, y) and not visited[y * w + 0]:
            queue.append((0, y))
            visited[y * w + 0] = 1
        if is_match(w - 1, y) and not visited[y * w + (w - 1)]:
            queue.append((w - 1, y))
            visited[y * w + (w - 1)] = 1

    head = 0
    while head < len(queue):
        cx, cy = queue[head]
        head += 1
        pixels[cx, cy] = (0, 0, 0, 0)

        for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
            if 0 <= nx < w and 0 <= ny < h:
                idx = ny * w + nx
                if not visited[idx]:
                    visited[idx] = 1
                    if is_match(nx, ny):
                        queue.append((nx, ny))
    return img

def remove_background_global(img, target_color, thresh=25):
    """Xóa mọi pixel có màu tương đồng với target_color trên toàn ảnh."""
    img = img.convert("RGBA")
    tr, tg, tb = target_color[:3]
    new_data = []
    for p in img.getdata():
        if p[3] == 0:
            new_data.append((0, 0, 0, 0))
        else:
            diff = abs(p[0] - tr) + abs(p[1] - tg) + abs(p[2] - tb)
            if diff <= thresh * 3:
                new_data.append((0, 0, 0, 0))
            else:
                new_data.append(p)
    img.putdata(new_data)
    return img

def detect_corner_color(img):
    """Phát hiện màu nền chiếm ưu thế tại 4 góc ảnh."""
    w, h = img.size
    pix = img.convert("RGBA").load()
    corners = [pix[0, 0], pix[w - 1, 0], pix[0, h - 1], pix[w - 1, h - 1]]
    return corners[0][:3]

def process_uploaded_image(img, target_w, target_h, scale_mode="fit", dither="none",
                           bg_transparent=False, bg_mode="flood_edge", trans_color=(0,0,0), trans_thresh=25,
                           brightness=1.0, contrast=1.0, flip_h=False, flip_v=False,
                           rotate=0, invert=False, grayscale=False):
    """Chuyển đổi ảnh upload theo kích thước và bộ lọc yêu cầu."""
    img = img.convert("RGBA")

    # 1. Biến đổi hình học (Rotate, Flip)
    if rotate in (90, 180, 270):
        img = img.rotate(-rotate, expand=True, resample=Image.Resampling.NEAREST)
    if flip_h:
        img = img.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
    if flip_v:
        img = img.transpose(Image.Transpose.FLIP_TOP_BOTTOM)

    # 2. Điều chỉnh màu sắc (Brightness, Contrast, Grayscale, Invert)
    if grayscale:
        r, g, b, a = img.split()
        gray = ImageOps.grayscale(img)
        img = Image.merge("RGBA", (gray, gray, gray, a))
    if brightness != 1.0:
        r, g, b, a = img.split()
        rgb = Image.merge("RGB", (r, g, b))
        rgb = ImageEnhance.Brightness(rgb).enhance(brightness)
        r, g, b = rgb.split()
        img = Image.merge("RGBA", (r, g, b, a))
    if contrast != 1.0:
        r, g, b, a = img.split()
        rgb = Image.merge("RGB", (r, g, b))
        rgb = ImageEnhance.Contrast(rgb).enhance(contrast)
        r, g, b = rgb.split()
        img = Image.merge("RGBA", (r, g, b, a))
    if invert:
        r, g, b, a = img.split()
        rgb = Image.merge("RGB", (r, g, b))
        rgb = ImageOps.invert(rgb)
        r, g, b = rgb.split()
        img = Image.merge("RGBA", (r, g, b, a))

    # 3. Xử lý xóa nền
    if bg_transparent:
        if bg_mode == "auto_corners":
            t_col = detect_corner_color(img)
            img = remove_background_flood_edge(img, t_col, trans_thresh)
        elif bg_mode == "flood_edge":
            img = remove_background_flood_edge(img, trans_color, trans_thresh)
        else: # "color" / global
            img = remove_background_global(img, trans_color, trans_thresh)

    # 4. Xử lý Scale
    src_w, src_h = img.size
    resample_filter = Image.Resampling.NEAREST if scale_mode == "pixel_art" else Image.Resampling.LANCZOS

    if scale_mode == "stretch" or (src_w == target_w and src_h == target_h):
        scaled = img.resize((target_w, target_h), resample_filter)
    elif scale_mode == "cover":
        ratio = max(target_w / src_w, target_h / src_h)
        new_size = (max(1, int(src_w * ratio)), max(1, int(src_h * ratio)))
        scaled = img.resize(new_size, resample_filter)
        left = (scaled.width - target_w) // 2
        top = (scaled.height - target_h) // 2
        scaled = scaled.crop((left, top, left + target_w, top + target_h))
    elif scale_mode == "pixel_art":
        ratio = min(target_w / src_w, target_h / src_h)
        new_size = (max(1, int(src_w * ratio)), max(1, int(src_h * ratio)))
        scaled_small = img.resize(new_size, Image.Resampling.NEAREST)
        scaled = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
        offset = ((target_w - scaled_small.width) // 2, (target_h - scaled_small.height) // 2)
        scaled.paste(scaled_small, offset)
    else: # "fit"
        ratio = min(target_w / src_w, target_h / src_h)
        new_size = (max(1, int(src_w * ratio)), max(1, int(src_h * ratio)))
        scaled_img = img.resize(new_size, resample_filter)
        scaled = Image.new("RGBA", (target_w, target_h), (0, 0, 0, 0))
        offset = ((target_w - scaled_img.width) // 2, (target_h - scaled_img.height) // 2)
        scaled.paste(scaled_img, offset)

    # 5. Áp dụng Dithering
    if dither == "floyd":
        processed = apply_floyd_steinberg_rgb565(scaled)
    else:
        processed = convert_to_rgb565_exact(scaled)

    return processed

def export_c_source(frames, asset_name, has_alpha=True, color_16_swap=True):
    """
    Sinh mã C và Header chuẩn LVGL v8:
    - has_alpha=True -> LV_IMG_CF_TRUE_COLOR_ALPHA (3 bytes/pixel: [high, low, a])
    - has_alpha=False -> LV_IMG_CF_TRUE_COLOR (2 bytes/pixel: [high, low])
    """
    first_img = frames[0]
    w, h = first_img.size
    px_bytes = 3 if has_alpha else 2
    data_size = w * h * px_bytes

    c_lines = [
        f'/* Auto-generated by ESP32 LaunchPad Display Studio */',
        f'/* Asset: {asset_name} ({w}x{h}, {len(frames)} frames) */',
        f'#include "lvgl.h"',
        f'#include "{asset_name}.h"\n',
    ]

    for idx, frame in enumerate(frames):
        map_name = f'{asset_name}_frame_{idx}_map' if len(frames) > 1 else f'{asset_name}_map'
        arr = bytearray()
        for y in range(h):
            for x in range(w):
                r, g, b, a = frame.getpixel((x, y))
                if has_alpha and a == 0:
                    arr.extend([0x00, 0x00, 0x00])
                else:
                    r5 = (r >> 3) & 0x1F
                    g6 = (g >> 2) & 0x3F
                    b5 = (b >> 3) & 0x1F
                    rgb565 = (r5 << 11) | (g6 << 5) | b5
                    high = (rgb565 >> 8) & 0xFF
                    low = rgb565 & 0xFF

                    if color_16_swap:
                        # CONFIG_LV_COLOR_16_SWAP=y: byte HIGH trước, LOW sau
                        color_bytes = [high, low]
                    else:
                        color_bytes = [low, high]

                    if has_alpha:
                        arr.extend(color_bytes + [a])
                    else:
                        arr.extend(color_bytes)

        # Định dạng mảng C
        c_lines.append(f'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t {map_name}[] = {{')
        chunk_size = 16 if not has_alpha else 12
        for i in range(0, len(arr), chunk_size):
            chunk = arr[i:i + chunk_size]
            hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
            c_lines.append(f'    {hex_str},')
        c_lines.append('};\n')

    cf_type = "LV_IMG_CF_TRUE_COLOR_ALPHA" if has_alpha else "LV_IMG_CF_TRUE_COLOR"

    for idx in range(len(frames)):
        dsc_name = f'img_{asset_name}_{idx}' if len(frames) > 1 else f'img_{asset_name}'
        map_name = f'{asset_name}_frame_{idx}_map' if len(frames) > 1 else f'{asset_name}_map'
        c_lines.extend([
            f'const lv_img_dsc_t {dsc_name} = {{',
            f'    .header.always_zero = 0,',
            f'    .header.w = {w},',
            f'    .header.h = {h},',
            f'    .data_size = {data_size},',
            f'    .header.cf = {cf_type},',
            f'    .data = {map_name},',
            f'}};\n'
        ])

    if len(frames) > 1:
        # Tạo mảng danh sách frame để dùng trực tiếp với lv_animimg
        c_lines.append(f'const lv_img_dsc_t * const {asset_name}_anim_frames[{len(frames)}] = {{')
        for idx in range(len(frames)):
            c_lines.append(f'    &img_{asset_name}_{idx},')
        c_lines.append('};\n')

    # Header File (.h)
    h_lines = [
        f'#ifndef {asset_name.upper()}_H',
        f'#define {asset_name.upper()}_H\n',
        f'#include "lvgl.h"\n',
        f'#ifdef __cplusplus',
        f'extern "C" {{',
        f'#endif\n',
    ]

    for idx in range(len(frames)):
        dsc_name = f'img_{asset_name}_{idx}' if len(frames) > 1 else f'img_{asset_name}'
        h_lines.append(f'extern const lv_img_dsc_t {dsc_name};')

    if len(frames) > 1:
        h_lines.append(f'extern const lv_img_dsc_t * const {asset_name}_anim_frames[{len(frames)}];')

    h_lines.extend([
        f'\n#ifdef __cplusplus',
        f'}}',
        f'#endif\n',
        f'#endif // {asset_name.upper()}_H\n'
    ])

    return '\n'.join(c_lines), '\n'.join(h_lines)

def export_bongo_shiba_c(frames, color_16_swap=True):
    """
    Sinh mã C tương thích 100% với components/display_ui/src/bongo_shiba_sprite.c
    (3 frames: img_bongo_tap_l, img_bongo_tap_r, img_bongo_rest - 104x78 TRUE_COLOR_ALPHA)
    """
    target_frames = list(frames)
    if len(target_frames) == 1:
        target_frames = [target_frames[0], target_frames[0], target_frames[0]]
    elif len(target_frames) == 2:
        target_frames = [target_frames[0], target_frames[1], target_frames[0]]
    else:
        target_frames = target_frames[:3]

    resized_frames = []
    for f in target_frames:
        if f.size != (104, 78):
            resized_frames.append(f.resize((104, 78), Image.Resampling.NEAREST))
        else:
            resized_frames.append(f)

    maps = [
        ('bongo_tap_l_map', 'img_bongo_tap_l', resized_frames[0]),
        ('bongo_tap_r_map', 'img_bongo_tap_r', resized_frames[1]),
        ('bongo_rest_map',  'img_bongo_rest',  resized_frames[2]),
    ]

    c_lines = [
        '/* Auto-generated by ESP32 LaunchPad Display Studio for Bongo Shiba */',
        '#include "bongo_shiba_sprite.h"\n',
    ]

    for map_name, dsc_name, frame in maps:
        w, h = frame.size
        arr = bytearray()
        for y in range(h):
            for x in range(w):
                r, g, b, a = frame.getpixel((x, y))
                if a == 0:
                    arr.extend([0x00, 0x00, 0x00])
                else:
                    r5 = (r >> 3) & 0x1F
                    g6 = (g >> 2) & 0x3F
                    b5 = (b >> 3) & 0x1F
                    rgb565 = (r5 << 11) | (g6 << 5) | b5
                    high = (rgb565 >> 8) & 0xFF
                    low = rgb565 & 0xFF

                    if color_16_swap:
                        color_bytes = [high, low]
                    else:
                        color_bytes = [low, high]

                    arr.extend(color_bytes + [a])

        c_lines.append(f'static const uint8_t {map_name}[] = {{')
        for i in range(0, len(arr), 12):
            chunk = arr[i:i + 12]
            hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
            c_lines.append(f'    {hex_str},')
        c_lines.append('};\n')

    for map_name, dsc_name, frame in maps:
        c_lines.extend([
            f'const lv_img_dsc_t {dsc_name} = {{',
            f'    .header.always_zero = 0,',
            f'    .header.w = 104,',
            f'    .header.h = 78,',
            f'    .data_size = sizeof({map_name}),',
            f'    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,',
            f'    .data = {map_name},',
            f'}};\n'
        ])

    h_path = os.path.join(DISPLAY_UI_DIR, 'include/bongo_shiba_sprite.h')
    h_code = ""
    if os.path.exists(h_path):
        with open(h_path, 'r', encoding='utf-8') as hf:
            h_code = hf.read()

    return '\n'.join(c_lines), h_code

def export_earth_wallpaper_c(frames, color_16_swap=True):
    """
    Sinh mã C tương thích 100% với components/display_ui/src/earth_wallpaper.c
    (320x240 RGB565 TRUE_COLOR không alpha)
    """
    frame = frames[0]
    if frame.size != (320, 240):
        frame = frame.resize((320, 240), Image.Resampling.LANCZOS)

    w, h = 320, 240
    arr = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b, a = frame.getpixel((x, y))
            r5 = (r >> 3) & 0x1F
            g6 = (g >> 2) & 0x3F
            b5 = (b >> 3) & 0x1F
            rgb565 = (r5 << 11) | (g6 << 5) | b5
            high = (rgb565 >> 8) & 0xFF
            low = rgb565 & 0xFF

            if color_16_swap:
                arr.extend([high, low])
            else:
                arr.extend([low, high])

    c_lines = [
        '/* Auto-generated by ESP32 LaunchPad Display Studio for Earth Wallpaper (320x240 RGB565) */',
        '#include "lvgl.h"',
        '#include "earth_wallpaper.h"\n',
        'const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t earth_wallpaper_map[] = {'
    ]

    for i in range(0, len(arr), 16):
        chunk = arr[i:i + 16]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        c_lines.append(f'    {hex_str},')
    c_lines.append('};\n')

    c_lines.extend([
        'const lv_img_dsc_t img_earth_wallpaper = {',
        '    .header.always_zero = 0,',
        '    .header.w = 320,',
        '    .header.h = 240,',
        '    .data_size = sizeof(earth_wallpaper_map),',
        '    .header.cf = LV_IMG_CF_TRUE_COLOR,',
        '    .data = earth_wallpaper_map,',
        '};\n'
    ])

    h_path = os.path.join(DISPLAY_UI_DIR, 'include/earth_wallpaper.h')
    h_code = ""
    if os.path.exists(h_path):
        with open(h_path, 'r', encoding='utf-8') as hf:
            h_code = hf.read()

    return '\n'.join(c_lines), h_code

def run_flash_worker():
    """Luồng chạy lệnh build & flash trong background."""
    global g_flash_state
    g_flash_state["running"] = True
    g_flash_state["success"] = None
    g_flash_state["log"] = []

    def log(msg):
        g_flash_state["log"].append(msg)
        print("[Studio Flash]", msg, flush=True)

    try:
        log("🚀 Bắt đầu quá trình nạp Firmware ESP32-S3...")
        cmd = f"source {IDF_EXPORT} && idf.py build && idf.py -p /dev/ttyACM0 flash"
        proc = subprocess.Popen(
            cmd,
            shell=True,
            executable="/bin/bash",
            cwd=PROJECT_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        for line in proc.stdout:
            line_str = line.strip()
            if line_str:
                log(line_str)

        proc.wait()
        if proc.returncode == 0:
            log("🎉 FLASH HOÀN TẤT THÀNH CÔNG 100%!")
            g_flash_state["success"] = True
        else:
            log(f"❌ Lỗi nạp firmware (Return code: {proc.returncode})")
            g_flash_state["success"] = False

    except Exception as e:
        log(f"❌ Ngoại lệ hệ thống: {e}")
        g_flash_state["success"] = False
    finally:
        g_flash_state["running"] = False

class StudioRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == '/' or parsed.path == '/index.html':
            html_file = os.path.join(STUDIO_DIR, 'index.html')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            with open(html_file, 'rb') as f:
                self.wfile.write(f.read())
            return

        elif parsed.path == '/api/device_status':
            has_device = os.path.exists('/dev/ttyACM0')
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({
                'connected': has_device,
                'port': '/dev/ttyACM0' if has_device else None
            }).encode('utf-8'))
            return

        elif parsed.path == '/api/flash_status':
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(g_flash_state).encode('utf-8'))
            return

        elif parsed.path == '/api/load_project_assets':
            # Tải các asset hiện có của dự án để người dùng chỉnh sửa
            resp = {'assets': []}
            
            # 1. Bongo Shiba 3 Frames
            f1 = os.path.join(PREVIEW_DIR, 'action_1_tap_left_right_up_60deg_1x.png')
            f2 = os.path.join(PREVIEW_DIR, 'action_2_tap_right_left_up_60deg_1x.png')
            f3 = os.path.join(PREVIEW_DIR, 'action_3_rest_both_down_1x.png')
            if os.path.exists(f1) and os.path.exists(f2) and os.path.exists(f3):
                resp['assets'].append({
                    'id': 'bongo_shiba',
                    'name': 'Bongo Shiba Tapping (104x78, 3 Frames)',
                    'width': 104,
                    'height': 78,
                    'has_alpha': True,
                    'target_c': 'components/display_ui/src/bongo_shiba_sprite.c',
                    'frames': [
                        img_to_b64(Image.open(f1)),
                        img_to_b64(Image.open(f2)),
                        img_to_b64(Image.open(f3)),
                    ]
                })

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(resp).encode('utf-8'))
            return

        else:
            super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        content_len = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_len).decode('utf-8')
        data = json.loads(body) if body else {}

        if parsed.path == '/api/convert_image':
            raw_img = b64_to_img(data['image'])
            tw = int(data.get('width', 104))
            th = int(data.get('height', 78))
            scale_mode = data.get('scale_mode', 'fit')
            dither = data.get('dither', 'none')
            bg_trans = bool(data.get('bg_transparent', False))
            bg_mode = data.get('bg_mode', 'flood_edge')
            trans_col = data.get('trans_color', [0, 0, 0])
            trans_thresh = int(data.get('trans_thresh', 25))
            brightness = float(data.get('brightness', 1.0))
            contrast = float(data.get('contrast', 1.0))
            flip_h = bool(data.get('flip_h', False))
            flip_v = bool(data.get('flip_v', False))
            rotate = int(data.get('rotate', 0))
            grayscale = bool(data.get('grayscale', False))
            invert = bool(data.get('invert', False))

            processed = process_uploaded_image(
                raw_img, tw, th,
                scale_mode=scale_mode,
                dither=dither,
                bg_transparent=bg_trans,
                bg_mode=bg_mode,
                trans_color=tuple(trans_col),
                trans_thresh=trans_thresh,
                brightness=brightness,
                contrast=contrast,
                flip_h=flip_h,
                flip_v=flip_v,
                rotate=rotate,
                grayscale=grayscale,
                invert=invert
            )

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({
                'status': 'ok',
                'result': img_to_b64(processed),
                'width': tw,
                'height': th
            }).encode('utf-8'))
            return

        elif parsed.path == '/api/export_c':
            frames_b64 = data.get('frames', [])
            asset_name = data.get('asset_name', 'my_custom_sprite').strip() or 'my_custom_sprite'
            has_alpha = bool(data.get('has_alpha', True))
            color_swap = bool(data.get('color_16_swap', True))
            apply_target = data.get('apply_target', 'none') # 'none', 'bongo_shiba', 'earth_wallpaper', 'custom_file'

            frames = [b64_to_img(b) for b in frames_b64]
            if not frames:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(json.dumps({'error': 'No frames provided'}).encode('utf-8'))
                return

            applied_path = None
            if apply_target == 'bongo_shiba':
                c_code, h_code = export_bongo_shiba_c(frames, color_16_swap=color_swap)
                target_file = os.path.join(DISPLAY_UI_DIR, 'src/bongo_shiba_sprite.c')
                with open(target_file, 'w', encoding='utf-8') as f:
                    f.write(c_code)
                applied_path = target_file
            elif apply_target == 'earth_wallpaper':
                c_code, h_code = export_earth_wallpaper_c(frames, color_16_swap=color_swap)
                target_file = os.path.join(DISPLAY_UI_DIR, 'src/earth_wallpaper.c')
                with open(target_file, 'w', encoding='utf-8') as f:
                    f.write(c_code)
                applied_path = target_file
            else:
                c_code, h_code = export_c_source(frames, asset_name, has_alpha=has_alpha, color_16_swap=color_swap)
                if apply_target == 'custom_file':
                    c_path = os.path.join(DISPLAY_UI_DIR, f'src/{asset_name}.c')
                    h_path = os.path.join(DISPLAY_UI_DIR, f'include/{asset_name}.h')
                    with open(c_path, 'w', encoding='utf-8') as f:
                        f.write(c_code)
                    with open(h_path, 'w', encoding='utf-8') as f:
                        f.write(h_code)
                    applied_path = f"{c_path} & {h_path}"

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({
                'status': 'ok',
                'c_code': c_code,
                'h_code': h_code,
                'applied_path': applied_path
            }).encode('utf-8'))
            return

        elif parsed.path == '/api/generate_gif':
            frames_b64 = data.get('frames', [])
            fps = int(data.get('fps', 8))
            scale = int(data.get('scale', 3))

            frames = [b64_to_img(b) for b in frames_b64]
            if not frames:
                self.send_response(400)
                self.end_headers()
                return

            if scale > 1:
                frames = [f.resize((f.width * scale, f.height * scale), Image.Resampling.NEAREST) for f in frames]

            duration_ms = int(1000 / max(1, fps))
            buf = io.BytesIO()
            frames[0].save(
                buf,
                format='GIF',
                save_all=True,
                append_images=frames[1:],
                duration=duration_ms,
                loop=0,
                disposal=2
            )
            gif_b64 = "data:image/gif;base64," + base64.b64encode(buf.getvalue()).decode('utf-8')

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({'status': 'ok', 'gif': gif_b64}).encode('utf-8'))
            return

        elif parsed.path == '/api/flash_device':
            if g_flash_state["running"]:
                self.send_response(400)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({'error': 'Quá trình nạp đang chạy'}).encode('utf-8'))
                return

            t = threading.Thread(target=run_flash_worker, daemon=True)
            t.start()

            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({'status': 'started'}).encode('utf-8'))
            return

        else:
            self.send_response(404)
            self.end_headers()

def run_server():
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(('0.0.0.0', PORT), StudioRequestHandler) as httpd:
        print("=" * 70)
        print(f"🚀 ESP32 LAUNCHPAD DISPLAY STUDIO ĐANG HOẠT ĐỘNG:")
        print(f"   👉 Truy cập Web UI tại:  http://localhost:{PORT}")
        print("=" * 70)
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n🛑 Server đã dừng.")

if __name__ == '__main__':
    run_server()
