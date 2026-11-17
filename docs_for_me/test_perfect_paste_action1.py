from PIL import Image
import numpy as np

# Load clean dog master
master = Image.open('docs_for_me/dog_master_clean.png').convert('RGBA')
marr = np.array(master)

# We only erase the OLD DOWN-ARM:
# The down-arm in master is at x >= 265, y >= 195 (paw) and the lower sleeve (x >= 260, y >= 180)
base = marr.copy()
for y in range(175, 238):
    for x in range(258, marr.shape[1]):
        # preserve red scarf
        r, g, b, a = base[y, x]
        if r > 140 and g < 70 and b < 70 and y < 185:
            continue
        # Only clear pixels that are part of the old down arm / paw
        base[y, x] = [0,0,0,0]

# Complete the flank outline from the neck (x=260, y=175) down to desk (x=268, y=238)
for y in range(175, 238):
    t = (y - 175) / (238 - 175)
    fx = int(260 + 6 * np.sin(t * np.pi * 0.8) + t * 2)
    for x in range(250, fx):
        if x < fx - 4:
            base[y, x] = [255, 163, 52, 255]
        elif x < fx - 2:
            base[y, x] = [248, 139, 38, 255]
        else:
            base[y, x] = [220, 107, 29, 255]
    base[y, fx] = [58, 16, 6, 255]
    base[y, fx + 1] = [58, 16, 6, 255]

body_img = Image.fromarray(base)

# Load the arched arm
arm_snap = Image.open('docs_for_me/snap_-50_-45.png').convert('RGBA')
sarr = np.array(arm_snap)
# Clean internal seam
for y in range(1, len(sarr)-1):
    for x in range(1, len(sarr[0])-1):
        r, g, b, a = sarr[y, x]
        if a > 0 and r < 60 and g < 40 and b < 30:
            nbrs = [sarr[y-1, x], sarr[y+1, x], sarr[y, x-1], sarr[y, x+1]]
            c = sum(1 for nb in nbrs if nb[3] > 0 and nb[0] > 150)
            if c >= 3:
                sarr[y, x] = [254, 160, 50, 255]

arm_clean = Image.fromarray(sarr)

# Test offsets around (240..255, 130..145)
for ox in [238, 243, 248]:
    for oy in [132, 137, 142]:
        comp = body_img.copy()
        comp.paste(arm_clean, (ox, oy), arm_clean)
        # Re-apply scarf knot over shoulder
        carr = np.array(comp)
        for y in range(150, 185):
            for x in range(235, 265):
                mr, mg, mb, ma = marr[y, x]
                if mr > 130 and mg < 70 and mb < 70:
                    carr[y, x] = marr[y, x]
        final = Image.fromarray(carr)
        crop = final.crop(final.getbbox())
        crop.save(f'docs_for_me/arm_fit_{ox}_{oy}.png')

print('Fit tests generated!')
