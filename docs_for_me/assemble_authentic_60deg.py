from PIL import Image
import numpy as np

# Load the authentic pieces extracted from bongo_dog_crop.png
# 1. Clean dog body with flank
body = Image.open('docs_for_me/body_no_arm_r.png').convert('RGBA')
barr = np.array(body)

# Fix the flank in barr so it's a smooth continuation from neck down to desk:
# Shoulder at y=160, x=262
# At y=238, x=270
for y in range(160, 238):
    t = (y - 160) / (238 - 160)
    fx = int(260 + 10 * np.sin(t * np.pi * 0.75))
    for x in range(235, fx):
        if x < fx - 4:
            barr[y, x] = [255, 163, 52, 255] # orange
        else:
            barr[y, x] = [220, 107, 29, 255] # shadow orange
    barr[y, fx] = [58, 16, 6, 255]
    barr[y, fx + 1] = [58, 16, 6, 255]
    # Clear anything beyond fx+1
    barr[y, fx + 2:] = [0, 0, 0, 0]

body_smooth = Image.fromarray(barr)

# 2. Forearm: let's rotate forearm by 55 degrees
forearm = Image.open('docs_for_me/split_forearm_r.png').convert('RGBA')
# Canvas for rotating forearm
fc = Image.new('RGBA', (300, 300), (0,0,0,0))
# Place shoulder at (80, 80)
fc.paste(forearm, (80, 80), forearm)
# Rotate 55 deg CCW around shoulder (80, 80)
rot_f = fc.rotate(55, resample=Image.Resampling.BICUBIC, center=(80, 80))
rf_arr = np.array(rot_f)
rf_arr[:,:,3] = np.where(rf_arr[:,:,3] > 110, 255, 0)
rot_f_clean = Image.fromarray(rf_arr)
bbox_f = rot_f_clean.getbbox()
print('Rotated forearm bbox in fc:', bbox_f)

# 3. Authentic Paw:
# In split_paw_r.png, the paw has its claws facing down!
paw = Image.open('docs_for_me/split_paw_r.png').convert('RGBA')

# Now composite the full raised arm onto an arm canvas:
arm_canvas = Image.new('RGBA', (400, 400), (0,0,0,0))
# Paste rotated forearm so shoulder is at (100, 150)
arm_canvas.paste(rot_f_clean, (100 - 80, 150 - 80), rot_f_clean)

# The wrist end of the rotated forearm:
# Shoulder was at (100, 150).
# The forearm extends up-right to bbox_f[2] in x and bbox_f[1] in y
wrist_x = 100 - 80 + bbox_f[2]
wrist_y = 150 - 80 + bbox_f[1]
print(f'Wrist position: ({wrist_x}, {wrist_y})')

# Paste the authentic paw at the wrist, slightly rotated so it curves naturally
paw_rot = paw.rotate(-10, resample=Image.Resampling.BICUBIC)
parr = np.array(paw_rot)
parr[:,:,3] = np.where(parr[:,:,3] > 110, 255, 0)
paw_clean = Image.fromarray(parr)

# Paste paw at wrist with a natural overlap (6-8px)
arm_canvas.paste(paw_clean, (wrist_x - 18, wrist_y - 8), paw_clean)

# Now paste this entire authentic raised arm onto the dog body!
# Shoulder in body is at (248, 166).
# In arm_canvas, shoulder is at (100, 150).
# So offset is (248 - 100, 166 - 150) = (148, 16)
comp1 = body_smooth.copy()
comp1.paste(arm_canvas, (148, 16), arm_canvas)

# Re-apply scarf knot over the shoulder
master = Image.open('docs_for_me/dog_master_clean.png').convert('RGBA')
marr = np.array(master)
carr = np.array(comp1)
for y in range(150, 180):
    for x in range(235, 260):
        mr, mg, mb, ma = marr[y, x]
        if mr > 130 and mg < 70 and mb < 70:
            carr[y, x] = marr[y, x]

final_comp1 = Image.fromarray(carr)
bbox = final_comp1.getbbox()
final_crop = final_comp1.crop(bbox)
final_crop.save('docs_for_me/authentic_action1_assembled.png')
print('authentic_action1_assembled.png saved, size:', final_crop.size)
