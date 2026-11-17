from PIL import Image
import numpy as np
import os
import shutil

os.makedirs('docs_for_me/bongo_dog_preview', exist_ok=True)

# -------------------------------------------------------------------------
# 1. Base Pristine Master
# -------------------------------------------------------------------------
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
oarr = np.array(orig)
H, W, _ = oarr.shape

# Clean space background:
clean_master_arr = oarr.copy()
for y in range(H):
    for x in range(W):
        r, g, b, a = clean_master_arr[y, x]
        # Space background:
        if (b > int(r) + 8 and b > 32) or (r < 40 and g < 45 and b < 85):
            clean_master_arr[y, x] = [0,0,0,0]
        # Top right motion arcs in sky:
        if y < 120 and x > 270 and r > 140 and g > 140 and b > 140:
            clean_master_arr[y, x] = [0,0,0,0]
        # Top-left UI frame line:
        if y < 145 and x < 45:
            clean_master_arr[y, x] = [0,0,0,0]
        # Desk surface below y=246:
        if y >= 246:
            clean_master_arr[y, x] = [0,0,0,0]

# =========================================================================
# 2. ACTION 3: REST POSE (Both paws DOWN on desk)
# =========================================================================
f3_arr = clean_master_arr.copy()
# Clean any desk line artifacts below paws (y >= 242)
for y in range(242, 246):
    for x in range(W):
        r, g, b, a = f3_arr[y, x]
        # keep paw outlines (r < 70 and g < 50 and b < 40)
        if not (150 <= x <= 220 or 265 <= x <= 335):
            f3_arr[y, x] = [0,0,0,0]

img_rest = Image.fromarray(f3_arr)
bbox = img_rest.getbbox()
img_rest_crop = img_rest.crop(bbox)

# =========================================================================
# 3. ACTION 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# =========================================================================
f1_arr = clean_master_arr.copy()

# Erase old down-arm (x >= 256, y >= 175)
for y in range(175, 246):
    for x in range(256, W):
        r, g, b, a = f1_arr[y, x]
        # Preserve red scarf (x <= 265, y <= 186, r > 130)
        if r > 130 and g < 70 and b < 70 and y < 186:
            continue
        f1_arr[y, x] = [0,0,0,0]

# Complete dog flank smoothly down to desk
for y in range(175, 246):
    t = (y - 175) / (246 - 175)
    fx = int(260 + 6 * np.sin(t * np.pi * 0.8) + t * 2)
    for x in range(250, fx):
        if x < fx - 4:
            f1_arr[y, x] = [255, 163, 52, 255]
        elif x < fx - 2:
            f1_arr[y, x] = [248, 139, 38, 255]
        else:
            f1_arr[y, x] = [220, 107, 29, 255]
    f1_arr[y, fx] = [58, 16, 6, 255]
    f1_arr[y, fx + 1] = [58, 16, 6, 255]

body1_img = Image.fromarray(f1_arr)

# Arched raised right arm:
arm_snap = Image.open('docs_for_me/snap_-50_-45.png').convert('RGBA')
sarr = np.array(arm_snap)
sh, sw, _ = sarr.shape
for y in range(sh):
    for x in range(sw):
        r, g, b, a = sarr[y, x]
        if a > 0:
            # fill shoulder seam dark lines with orange
            if r < 60 and g < 40 and b < 30 and y < 45 and x < 45:
                sarr[y, x] = [254, 160, 50, 255]
            # Clean stray black tick at bottom-right of paw (x > 70, y > 65)
            if x > 70 and y > 65 and (r < 60 and g < 40 and b < 30):
                sarr[y, x] = [0, 0, 0, 0]

arm_clean1 = Image.fromarray(sarr)
comp1 = body1_img.copy()
comp1.paste(arm_clean1, (244, 138), arm_clean1)

# Re-apply red scarf over shoulder
carr1 = np.array(comp1)
for y in range(150, 190):
    for x in range(235, 268):
        mr, mg, mb, ma = oarr[y, x]
        if mr > 130 and mg < 70 and mb < 70:
            carr1[y, x] = [mr, mg, mb, 255]

img_tap_l = Image.fromarray(carr1)
img_tap_l_crop = img_tap_l.crop(bbox) # Use same bbox for exact alignment!

# =========================================================================
# 4. ACTION 2: TAP RIGHT (Right arm DOWN, Left paw RAISED ~60 deg)
# =========================================================================
f2_arr = clean_master_arr.copy()

# Erase resting left paw (x in [130, 225], y in [195, 246])
for y in range(195, 246):
    for x in range(130, 225):
        r, g, b, a = f2_arr[y, x]
        # preserve red scarf
        if r > 140 and g < 70 and b < 70 and y < 216:
            continue
        f2_arr[y, x] = [0,0,0,0]

# Fill white chest fur behind erased paw down to desk
for y in range(195, 246):
    for x in range(140, 225):
        if f2_arr[y, x, 3] == 0:
            if y < 240:
                f2_arr[y, x] = [255, 254, 250, 255] if y < 232 else [238, 222, 204, 255]
            elif y in (240, 241, 242):
                f2_arr[y, x] = [225, 210, 195, 255]
            else:
                f2_arr[y, x] = [58, 16, 6, 255]

body2_img = Image.fromarray(f2_arr)

# Load rot_left_55
rot_l = Image.open('docs_for_me/rot_left_55.png').convert('RGBA')
rarr = np.array(rot_l)
# Clean stray line on top-right of rot_left_55
for y in range(len(rarr)):
    for x in range(len(rarr[0])):
        if x > 55 and y < 55:
            rarr[y, x] = [0,0,0,0]
rot_l_clean = Image.fromarray(rarr)

comp2 = body2_img.copy()
comp2.paste(rot_l_clean, (126, 162), rot_l_clean)

# Re-apply red scarf knot
carr2 = np.array(comp2)
for y in range(165, 215):
    for x in range(130, 180):
        mr, mg, mb, ma = oarr[y, x]
        if mr > 140 and mg < 70 and mb < 70:
            carr2[y, x] = [mr, mg, mb, 255]

img_tap_r = Image.fromarray(carr2)
img_tap_r_crop = img_tap_r.crop(bbox)

# =========================================================================
# 5. Save Master HD PNGs (around 340 x 240)
# =========================================================================
img_tap_l_crop.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_hd.png')
img_tap_r_crop.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_hd.png')
img_rest_crop.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_hd.png')

# =========================================================================
# 6. Save Target 1X Hardware Sprites (104 x 78) & 3X Previews (312 x 234)
# =========================================================================
TARGET_W, TARGET_H = 104, 78
s1 = img_tap_l_crop.resize((TARGET_W, TARGET_H), Image.Resampling.LANCZOS)
s2 = img_tap_r_crop.resize((TARGET_W, TARGET_H), Image.Resampling.LANCZOS)
s3 = img_rest_crop.resize((TARGET_W, TARGET_H), Image.Resampling.LANCZOS)

# Clean alpha thresholding to maintain crisp pixel art
for s in [s1, s2, s3]:
    s_arr = np.array(s)
    s_arr[:,:,3] = np.where(s_arr[:,:,3] > 80, 255, 0)
    s.paste(Image.fromarray(s_arr))

s1.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_1x.png')
s2.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_1x.png')
s3.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_1x.png')

# 3X Previews (nearest neighbor to show clean crisp pixels)
p1 = s1.resize((TARGET_W * 3, TARGET_H * 3), Image.NEAREST)
p2 = s2.resize((TARGET_W * 3, TARGET_H * 3), Image.NEAREST)
p3 = s3.resize((TARGET_W * 3, TARGET_H * 3), Image.NEAREST)

p1.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_3x.png')
p2.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_3x.png')
p3.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_3x.png')

# =========================================================================
# 7. Animated GIFs: 3s alternating taps (150ms) + 2s rest (2000ms)
# =========================================================================
frames_3x = []
durations = []
for _ in range(10): # 10 * 300ms = 3000ms (3 seconds)
    frames_3x.append(p1)
    durations.append(150)
    frames_3x.append(p2)
    durations.append(150)

# Rest for 2 seconds
frames_3x.append(p3)
durations.append(2000)

frames_3x[0].save(
    'docs_for_me/bongo_dog_preview/bongo_60deg_animation_3x.gif',
    save_all=True,
    append_images=frames_3x[1:],
    duration=durations,
    loop=0,
    disposal=2
)

# Also create HD GIF
frames_hd = []
for _ in range(10):
    frames_hd.append(img_tap_l_crop)
    frames_hd.append(img_tap_r_crop)
frames_hd.append(img_rest_crop)

frames_hd[0].save(
    'docs_for_me/bongo_dog_preview/bongo_60deg_animation_hd.gif',
    save_all=True,
    append_images=frames_hd[1:],
    duration=durations,
    loop=0,
    disposal=2
)

# Copy to brain artifact directory for instant user viewing
brain_dir = '/home/hungdk4/.gemini/antigravity-cli/brain/3aced826-248d-4994-94da-3362ffe3e8e7'
for fname in [
    'action_1_tap_left_right_up_60deg_3x.png',
    'action_2_tap_right_left_up_60deg_3x.png',
    'action_3_rest_both_down_3x.png',
    'bongo_60deg_animation_3x.gif',
    'action_1_tap_left_right_up_60deg_hd.png',
    'action_2_tap_right_left_up_60deg_hd.png',
    'action_3_rest_both_down_hd.png',
    'bongo_60deg_animation_hd.gif'
]:
    shutil.copy(f'docs_for_me/bongo_dog_preview/{fname}', f'{brain_dir}/{fname}')

print('All previews and GIFs successfully generated and copied to brain!')
