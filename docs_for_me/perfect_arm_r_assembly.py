from PIL import Image, ImageDraw
import numpy as np

# 1. Load clean master dog without stray borders
master = Image.open('docs_for_me/perfect_dog_master.png').convert('RGBA')
marr = np.array(master)
# Clean left border (x < 6)
marr[:, :6] = [0,0,0,0]
# Clean bottom table (y >= 238)
marr[238:, :] = [0,0,0,0]
# Clean top-left space frame
marr[:145, :45] = [0,0,0,0]
# Clean top-right space frame
marr[:150, 285:] = [0,0,0,0]

# 2. Base dog with flank:
# Erase old down-arm (x >= 240, y >= 165)
base_f1 = marr.copy()
for y in range(165, 238):
    for x in range(240, marr.shape[1]):
        r, g, b, a = base_f1[y, x]
        if r > 140 and g < 70 and b < 70 and y < 178:
            continue
        base_f1[y, x] = [0,0,0,0]

# Beautiful natural flank:
# Continues from neck at (264, 158) down to desk at (272, 238)
for y in range(158, 238):
    t = (y - 158) / (238 - 158)
    # gentle organic ribcage curve
    fx = int(264 + 8 * np.sin(t * np.pi * 0.8) + t * 4)
    for x in range(235, fx):
        if x < fx - 5:
            base_f1[y, x] = [255, 163, 52, 255] # orange
        elif x < fx - 2:
            base_f1[y, x] = [248, 139, 38, 255] # mid orange
        else:
            base_f1[y, x] = [220, 107, 29, 255] # shadow orange
    base_f1[y, fx] = [58, 16, 6, 255]
    base_f1[y, fx + 1] = [58, 16, 6, 255]
    base_f1[y, fx + 2:] = [0, 0, 0, 0]

# 3. Clean up the authentic right arm sprite:
arm_raw = Image.open('docs_for_me/isolated_arm_r_tight.png').convert('RGBA')
aarr = np.array(arm_raw)
ah, aw, _ = aarr.shape

# Clean the flat bottom desk edge on the paw (y >= 70)
# Replace flat desk cut with rounded white paw and claws
for y in range(68, ah):
    for x in range(aw):
        r, g, b, a = aarr[y, x]
        if a > 0 and (r < 60 and g < 50 and b < 50): # black desk line
            # if inside paw, make it white or outline
            if x < 75 and y < 74:
                aarr[y, x] = [255, 254, 250, 255]
            else:
                aarr[y, x] = [0, 0, 0, 0]

arm_clean = Image.fromarray(aarr)

# 4. Rotate arm by 55 degrees CCW
# Pivot in arm_clean: top shoulder is around (x=10, y=10)
# In master dog coordinates, shoulder is at (252, 166)
cw, ch = 500, 500
canvas = Image.new('RGBA', (cw, ch), (0,0,0,0))
cx, cy = 250, 250
# Place arm so shoulder (10, 10) is at (cx, cy)
canvas.paste(arm_clean, (cx - 10, cy - 10), arm_clean)

# Rotate around (cx, cy) by 55 degrees
rot_arm = canvas.rotate(55, resample=Image.Resampling.BICUBIC, center=(cx, cy))
rarr = np.array(rot_arm)
rarr[:,:,3] = np.where(rarr[:,:,3] > 110, 255, 0)
rot_clean = Image.fromarray(rarr)

# 5. Composite onto base_f1:
# Place shoulder (cx, cy) at (248, 166) in base_f1
comp1 = Image.fromarray(base_f1)
comp1.paste(rot_clean, (248 - cx, 166 - cy), rot_clean)

# 6. Re-apply red scarf over the shoulder joint
carr = np.array(comp1)
for y in range(150, 180):
    for x in range(235, 265):
        mr, mg, mb, ma = marr[y, x]
        if mr > 130 and mg < 70 and mb < 70:
            carr[y, x] = marr[y, x]

final1 = Image.fromarray(carr)
bbox = final1.getbbox()
final1.crop(bbox).save('docs_for_me/perfect_action1_hd.png')
print('perfect_action1_hd.png saved, size:', final1.crop(bbox).size)
