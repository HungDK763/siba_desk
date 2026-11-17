from PIL import Image
import numpy as np

# 1. Load clean dog master
master = Image.open('docs_for_me/dog_master_clean.png').convert('RGBA')
marr = np.array(master)

# =========================================================================
# ACTION 3: REST (Both paws DOWN on the desk, authentic original pose)
# =========================================================================
rest_arr = marr.copy()
rest_arr[238:, :] = [0,0,0,0]
rest_arr[:145, :45] = [0,0,0,0]
rest_arr[:150, 285:] = [0,0,0,0]

action_3 = Image.fromarray(rest_arr)
action_3_crop = action_3.crop(action_3.getbbox())
action_3_crop.save('docs_for_me/action_3_rest_hd.png')
print('Action 3 HD saved, size:', action_3_crop.size)

# =========================================================================
# ACTION 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# =========================================================================
base1 = rest_arr.copy()

# Erase old down-arm (x >= 258, y >= 175)
for y in range(175, 238):
    for x in range(258, marr.shape[1]):
        r, g, b, a = base1[y, x]
        if r > 140 and g < 70 and b < 70 and y < 185:
            continue
        base1[y, x] = [0,0,0,0]

# Complete flank from neck down to desk
for y in range(175, 238):
    t = (y - 175) / (238 - 175)
    fx = int(260 + 6 * np.sin(t * np.pi * 0.8) + t * 2)
    for x in range(250, fx):
        if x < fx - 4:
            base1[y, x] = [255, 163, 52, 255]
        elif x < fx - 2:
            base1[y, x] = [248, 139, 38, 255]
        else:
            base1[y, x] = [220, 107, 29, 255]
    base1[y, fx] = [58, 16, 6, 255]
    base1[y, fx + 1] = [58, 16, 6, 255]

body1_img = Image.fromarray(base1)

# Arched raised right arm:
arm_snap = Image.open('docs_for_me/snap_-50_-45.png').convert('RGBA')
sarr = np.array(arm_snap)
sh, sw, _ = sarr.shape
for y in range(1, sh - 1):
    for x in range(1, sw - 1):
        r, g, b, a = sarr[y, x]
        if a > 0:
            if r < 60 and g < 40 and b < 30:
                nbrs = [sarr[y-1, x], sarr[y+1, x], sarr[y, x-1], sarr[y, x+1]]
                c = sum(1 for nb in nbrs if nb[3] > 0 and nb[0] > 150)
                if c >= 3:
                    sarr[y, x] = [254, 160, 50, 255]
            if x > 70 and y > 65 and (r < 60 and g < 40 and b < 30):
                nbrs = [sarr[y-1, x], sarr[y+1, x], sarr[y, x-1], sarr[y, x+1]]
                solid_count = sum(1 for nb in nbrs if nb[3] > 0)
                if solid_count <= 1:
                    sarr[y, x] = [0, 0, 0, 0]

arm_clean1 = Image.fromarray(sarr)
comp1 = body1_img.copy()
comp1.paste(arm_clean1, (244, 137), arm_clean1)

# Re-apply red scarf over shoulder
carr1 = np.array(comp1)
for y in range(150, 185):
    for x in range(235, 265):
        mr, mg, mb, ma = marr[y, x]
        if mr > 130 and mg < 70 and mb < 70:
            carr1[y, x] = marr[y, x]

action_1 = Image.fromarray(carr1)
action_1_crop = action_1.crop(action_1.getbbox())
action_1_crop.save('docs_for_me/action_1_tap_l_hd.png')
print('Action 1 HD saved, size:', action_1_crop.size)

# =========================================================================
# ACTION 2: TAP RIGHT (Right arm DOWN, Left paw RAISED ~60 deg)
# =========================================================================
base2 = rest_arr.copy()

# Erase resting left paw (x in [140, 225], y >= 200)
for y in range(200, 238):
    for x in range(140, 225):
        r, g, b, a = base2[y, x]
        if r > 140 and g < 70 and b < 70 and y < 216:
            continue
        base2[y, x] = [0,0,0,0]

# Fill dog white chest down to the desk line (y=238)
for y in range(200, 238):
    for x in range(140, 225):
        if y < 233:
            base2[y, x] = [255, 254, 250, 255] if y < 225 else [238, 222, 204, 255]
        elif y in (233, 234, 235):
            base2[y, x] = [225, 210, 195, 255]
        elif y in (236, 237):
            base2[y, x] = [58, 16, 6, 255]

body2_img = Image.fromarray(base2)

# Lifted Left Paw:
paw_l_orig = action_3.crop((140, 205, 225, 238))
pl_arr = np.array(paw_l_orig)
for y in range(pl_arr.shape[0]-3, pl_arr.shape[0]):
    for x in range(pl_arr.shape[1]):
        r, g, b, a = pl_arr[y, x]
        if r < 60 and g < 40 and b < 30:
            pl_arr[y, x] = [58, 16, 6, 255]

lifted_paw_l = Image.fromarray(pl_arr)

# Paste lifted left paw at (142, 168) -> 42 pixels above desk!
comp2 = body2_img.copy()
comp2.paste(lifted_paw_l, (142, 168), lifted_paw_l)

# Add orange sleeve connection from left shoulder (x=130..148, y=175..198)
carr2 = np.array(comp2)
for y in range(175, 198):
    for x in range(130, 148):
        if carr2[y, x, 3] == 0:
            carr2[y, x] = [255, 163, 52, 255]
    if carr2[y, 129, 3] == 0:
        carr2[y, 129] = [58, 16, 6, 255]

action_2 = Image.fromarray(carr2)
action_2_crop = action_2.crop(action_2.getbbox())
action_2_crop.save('docs_for_me/action_2_tap_r_hd.png')
print('Action 2 HD saved, size:', action_2_crop.size)
