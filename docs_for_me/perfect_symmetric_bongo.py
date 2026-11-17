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
# ACTION 3: REST POSE (Both paws DOWN on desk)
# =========================================================================
f3_arr = clean_master_arr.copy()
for y in range(242, 246):
    for x in range(W):
        if not (150 <= x <= 220 or 265 <= x <= 335):
            f3_arr[y, x] = [0,0,0,0]

img_rest = Image.fromarray(f3_arr)
bbox = img_rest.getbbox()
img_rest_crop = img_rest.crop(bbox)

# =========================================================================
# ACTION 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# =========================================================================
f1_arr = clean_master_arr.copy()
for y in range(175, 246):
    for x in range(256, W):
        r, g, b, a = f1_arr[y, x]
        if r > 130 and g < 70 and b < 70 and y < 186:
            continue
        f1_arr[y, x] = [0,0,0,0]

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

arm_snap = Image.open('docs_for_me/snap_-50_-45.png').convert('RGBA')
sarr = np.array(arm_snap)
sh, sw, _ = sarr.shape
for y in range(sh):
    for x in range(sw):
        r, g, b, a = sarr[y, x]
        if a > 0:
            if r < 60 and g < 40 and b < 30 and y < 45 and x < 45:
                sarr[y, x] = [254, 160, 50, 255]
            if x > 70 and y > 65 and (r < 60 and g < 40 and b < 30):
                sarr[y, x] = [0, 0, 0, 0]

arm_clean1 = Image.fromarray(sarr)
comp1 = body1_img.copy()
comp1.paste(arm_clean1, (244, 138), arm_clean1)

for y in range(150, 190):
    for x in range(235, 268):
        mr, mg, mb, ma = oarr[y, x]
        if mr > 130 and mg < 70 and mb < 70:
            carr1 = np.array(comp1)
            carr1[y, x] = [mr, mg, mb, 255]
            comp1 = Image.fromarray(carr1)

img_tap_l = comp1
img_tap_l_crop = img_tap_l.crop(bbox)

# =========================================================================
# ACTION 2: TAP RIGHT (Right arm DOWN, Left paw RAISED ~60 deg)
# =========================================================================
f2_arr = clean_master_arr.copy()

# Erase resting left paw (x in [130, 225], y in [195, 246])
for y in range(195, 246):
    for x in range(130, 225):
        r, g, b, a = f2_arr[y, x]
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

# Lifted Left Paw higher and more arched:
# Use rot_left_55, position at (126, 148) so height matches right arm!
rot_l = Image.open('docs_for_me/rot_left_55.png').convert('RGBA')
rarr = np.array(rot_l)
for y in range(len(rarr)):
    for x in range(len(rarr[0])):
        if x > 55 and y < 55:
            rarr[y, x] = [0,0,0,0]
rot_l_clean = Image.fromarray(rarr)

comp2 = body2_img.copy()
# Paste lifted left paw at (126, 150) -> lifted 65 pixels above desk!
comp2.paste(rot_l_clean, (126, 150), rot_l_clean)

# Re-apply red scarf knot
for y in range(165, 215):
    for x in range(130, 180):
        mr, mg, mb, ma = oarr[y, x]
        if mr > 140 and mg < 70 and mb < 70:
            comp2.putpixel((x, y), (mr, mg, mb, 255))

img_tap_r = comp2
img_tap_r_crop = img_tap_r.crop(bbox)

# =========================================================================
# Export 3X and 1X Previews
# =========================================================================
TARGET_W, TARGET_H = 104, 78
s1 = img_tap_l_crop.resize((TARGET_W, TARGET_H), Image.Resampling.LANCZOS)
s2 = img_tap_r_crop.resize((TARGET_W, TARGET_H), Image.Resampling.LANCZOS)
s3 = img_rest_crop.resize((TARGET_W, TARGET_H), Image.Resampling.LANCZOS)

for s in [s1, s2, s3]:
    s_arr = np.array(s)
    s_arr[:,:,3] = np.where(s_arr[:,:,3] > 80, 255, 0)
    s.paste(Image.fromarray(s_arr))

# 3X Previews (312 x 234)
p1 = s1.resize((TARGET_W * 3, TARGET_H * 3), Image.NEAREST)
p2 = s2.resize((TARGET_W * 3, TARGET_H * 3), Image.NEAREST)
p3 = s3.resize((TARGET_W * 3, TARGET_H * 3), Image.NEAREST)

# Save into docs_for_me/bongo_dog_preview/
p1.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_3x.png')
p2.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_3x.png')
p3.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_3x.png')

s1.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_1x.png')
s2.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_1x.png')
s3.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_1x.png')

# Save HD frames
img_tap_l_crop.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_hd.png')
img_tap_r_crop.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_hd.png')
img_rest_crop.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_hd.png')

# Animated GIF (3s tapping @ 150ms + 2s rest @ 2000ms)
frames_3x = []
durations = []
for _ in range(10):
    frames_3x.append(p1)
    durations.append(150)
    frames_3x.append(p2)
    durations.append(150)
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

# Brain folder copy
brain_dir = '/home/hungdk4/.gemini/antigravity-cli/brain/3aced826-248d-4994-94da-3362ffe3e8e7'
for fname in [
    'action_1_tap_left_right_up_60deg_3x.png',
    'action_2_tap_right_left_up_60deg_3x.png',
    'action_3_rest_both_down_3x.png',
    'bongo_60deg_animation_3x.gif'
]:
    shutil.copy(f'docs_for_me/bongo_dog_preview/{fname}', f'{brain_dir}/{fname}')

print('Symmetric bongo preview generation complete!')
