import http.server
import socketserver
import json
import base64
import io
import os
import shutil
from PIL import Image

PORT = 8088
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.abspath(os.path.join(BASE_DIR, '../..'))
PREVIEW_DIR = os.path.join(PROJECT_DIR, 'docs_for_me/bongo_dog_preview')
BRAIN_DIR = '/home/hungdk4/.gemini/antigravity-cli/brain/3aced826-248d-4994-94da-3362ffe3e8e7'
C_FILE_PATH = os.path.join(PROJECT_DIR, 'components/display_ui/src/bongo_shiba_sprite.c')

def img_to_b64(path):
    if not os.path.exists(path):
        return None
    with open(path, 'rb') as f:
        data = f.read()
    return 'data:image/png;base64,' + base64.b64encode(data).decode('utf-8')

def b64_to_img(b64_str):
    if ',' in b64_str:
        b64_str = b64_str.split(',', 1)[1]
    raw = base64.b64decode(b64_str)
    return Image.open(io.BytesIO(raw)).convert('RGBA')

def export_c_code(im1, im2, im3):
    # LVGL TRUE_COLOR_ALPHA (RGB565 + 8-bit Alpha, little-endian)
    # Total bytes per pixel: 3 (low byte RGB565, high byte RGB565, alpha)
    def convert_map(img):
        w, h = img.size
        arr = bytearray()
        for y in range(h):
            for x in range(w):
                r, g, b, a = img.getpixel((x, y))
                if a == 0:
                    arr.extend([0x00, 0x00, 0x00])
                else:
                    r5 = (r >> 3) & 0x1F
                    g6 = (g >> 2) & 0x3F
                    b5 = (b >> 3) & 0x1F
                    rgb565 = (r5 << 11) | (g6 << 5) | b5
                    # CONFIG_LV_COLOR_16_SWAP=y requires high byte first: [high, low, a]
                    low = rgb565 & 0xFF
                    high = (rgb565 >> 8) & 0xFF
                    arr.extend([high, low, a])
        return arr

    map_l = convert_map(im1)
    map_r = convert_map(im2)
    map_rest = convert_map(im3)

    def format_c_array(name, data):
        lines = [f'static const uint8_t {name}[] = {{']
        for i in range(0, len(data), 12):
            chunk = data[i:i+12]
            hex_chunk = ', '.join(f'0x{b:02x}' for b in chunk)
            lines.append(f'    {hex_chunk},')
        lines.append('};\n')
        return '\n'.join(lines)

    c_content = [
        '#include "bongo_shiba_sprite.h"\n',
        format_c_array('bongo_tap_l_map', map_l),
        format_c_array('bongo_tap_r_map', map_r),
        format_c_array('bongo_rest_map', map_rest),
        'const lv_img_dsc_t img_bongo_tap_l = {',
        '    .header.always_zero = 0,',
        '    .header.w = 104,',
        '    .header.h = 78,',
        '    .data_size = 24336,',
        '    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,',
        '    .data = bongo_tap_l_map,',
        '};\n',
        'const lv_img_dsc_t img_bongo_tap_r = {',
        '    .header.always_zero = 0,',
        '    .header.w = 104,',
        '    .header.h = 78,',
        '    .data_size = 24336,',
        '    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,',
        '    .data = bongo_tap_r_map,',
        '};\n',
        'const lv_img_dsc_t img_bongo_rest = {',
        '    .header.always_zero = 0,',
        '    .header.w = 104,',
        '    .header.h = 78,',
        '    .data_size = 24336,',
        '    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,',
        '    .data = bongo_rest_map,',
        '};\n',
    ]

    with open(C_FILE_PATH, 'w') as f:
        f.write('\n'.join(c_content))

class RequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/' or self.path == '/index.html':
            html_path = os.path.join(BASE_DIR, 'index.html')
            self.send_response(200)
            self.send_header('Content-type', 'text/html; charset=utf-8')
            self.end_headers()
            with open(html_path, 'rb') as f:
                self.wfile.write(f.read())
            return
        elif self.path == '/api/data':
            f1_path = os.path.join(PREVIEW_DIR, 'action_1_tap_left_right_up_60deg_1x.png')
            f2_path = os.path.join(PREVIEW_DIR, 'action_2_tap_right_left_up_60deg_1x.png')
            f3_path = os.path.join(PREVIEW_DIR, 'action_3_rest_both_down_1x.png')
            ref_path = os.path.join(PROJECT_DIR, 'docs_for_me/bongo_dog_crop.png')

            resp = {
                'frame1': img_to_b64(f1_path),
                'frame2': img_to_b64(f2_path),
                'frame3': img_to_b64(f3_path),
                'ref': img_to_b64(ref_path)
            }
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(resp).encode('utf-8'))
            return
        else:
            super().do_GET()

    def do_POST(self):
        content_len = int(self.headers.get('Content-Length', 0))
        post_body = self.rfile.read(content_len)
        data = json.loads(post_body.decode('utf-8'))

        if self.path == '/api/save':
            im1 = b64_to_img(data['frame1'])
            im2 = b64_to_img(data['frame2'])
            im3 = b64_to_img(data['frame3'])

            # Ensure 104x78
            im1 = im1.crop((0, 0, 104, 78))
            im2 = im2.crop((0, 0, 104, 78))
            im3 = im3.crop((0, 0, 104, 78))

            # Save 1X PNGs
            f1_1x = os.path.join(PREVIEW_DIR, 'action_1_tap_left_right_up_60deg_1x.png')
            f2_1x = os.path.join(PREVIEW_DIR, 'action_2_tap_right_left_up_60deg_1x.png')
            f3_1x = os.path.join(PREVIEW_DIR, 'action_3_rest_both_down_1x.png')
            im1.save(f1_1x)
            im2.save(f2_1x)
            im3.save(f3_1x)

            # Generate 3X PNGs (Nearest Neighbor)
            im1_3x = im1.resize((104*3, 78*3), Image.NEAREST)
            im2_3x = im2.resize((104*3, 78*3), Image.NEAREST)
            im3_3x = im3.resize((104*3, 78*3), Image.NEAREST)

            f1_3x = os.path.join(PREVIEW_DIR, 'action_1_tap_left_right_up_60deg_3x.png')
            f2_3x = os.path.join(PREVIEW_DIR, 'action_2_tap_right_left_up_60deg_3x.png')
            f3_3x = os.path.join(PREVIEW_DIR, 'action_3_rest_both_down_3x.png')
            im1_3x.save(f1_3x)
            im2_3x.save(f2_3x)
            im3_3x.save(f3_3x)

            # Generate Animated GIF (3s tapping + 2s rest)
            gif_frames = []
            durations = []
            for _ in range(12):
                gif_frames.append(im1_3x)
                durations.append(130)
                gif_frames.append(im2_3x)
                durations.append(130)
            gif_frames.append(im3_3x)
            durations.append(2000)

            gif_path = os.path.join(PREVIEW_DIR, 'bongo_60deg_animation_3x.gif')
            gif_frames[0].save(
                gif_path,
                save_all=True,
                append_images=gif_frames[1:],
                duration=durations,
                loop=0,
                disposal=2
            )

            # Copy to brain
            if os.path.exists(BRAIN_DIR):
                for f in ['action_1_tap_left_right_up_60deg_3x.png',
                          'action_2_tap_right_left_up_60deg_3x.png',
                          'action_3_rest_both_down_3x.png',
                          'bongo_60deg_animation_3x.gif']:
                    shutil.copy(os.path.join(PREVIEW_DIR, f), os.path.join(BRAIN_DIR, f))

            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({'status': 'ok', 'message': 'Saved PNGs and GIF successfully!'}).encode('utf-8'))
            return

        elif self.path == '/api/export_c':
            im1 = b64_to_img(data['frame1']).crop((0, 0, 104, 78))
            im2 = b64_to_img(data['frame2']).crop((0, 0, 104, 78))
            im3 = b64_to_img(data['frame3']).crop((0, 0, 104, 78))

            export_c_code(im1, im2, im3)

            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({'status': 'ok', 'message': 'Exported C code successfully to bongo_shiba_sprite.c!'}).encode('utf-8'))
            return

def start_server():
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(('', PORT), RequestHandler) as httpd:
        print(f"Bongo Shiba Pixel Editor running at: http://localhost:{PORT}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")

if __name__ == '__main__':
    start_server()
