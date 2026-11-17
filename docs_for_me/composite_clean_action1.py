from PIL import Image
import numpy as np

# Load clean dog master
master = Image.open('docs_for_me/dog_master_clean.png').convert('RGBA')
marr = np.array(master)

# Prepare body: erase right arm (x >= 240, y >= 165)
base = marr.copy()
for y in range(165, 238):
    for x in range(240, marr.shape[1]):
        r, g, b, a = base[y, x]
        if r > 140 and g < 70 and b < 70 and y < 178:
            continue
        base[y, x] = [0,0,0,0]

# Fill dog flank behind the arm:
# Neck is at x=262, y=156
# Flank curves gently:
for y in range(156, 238):
    t = (y - 156) / (238 - 156)
    fx = int(262 + 7 * np.sin(t * np.pi * 0.8) + t * 3)
    for x in range(232, fx):
        if x < fx - 5:
            base[y, x] = [255, 163, 52, 255] # orange
        elif x < fx - 2:
            base[y, x] = [248, 139, 38, 255] # mid orange
        else:
            base[y, x] = [220, 107, 29, 255] # shadow orange
    base[y, fx] = [58, 16, 6, 255]
    base[y, fx + 1] = [58, 16, 6, 255]

body_img = Image.fromarray(base)

# Load the seamlessly snapped arm from snap_-50_-45.png
arm_snap = Image.open('docs_for_me/snap_-50_-45.png').convert('RGBA')

# Clean interior seam artifacts inside arm_snap:
sarr = np.array(arm_snap)
# Remove the stray black line inside the arm (around the forearm-paw junction)
for y in range(len(sarr)):
    for x in range(len(sarr[0])):
        r, g, b, a = sarr[y, x]
        # If it's a dark pixel surrounded by orange or white, fill with orange
        if a > 0 and r < 60 and g < 40 and b < 30:
            # check neighbors
            if 0 < y < len(sarr)-1 and 0 < x < len(sarr[0])-1:
                nbrs = [sarr[y-1, x], sarr[y+1, x], sarr[y, x-1], sarr[y, x+1]]
                orange_or_white = sum(1 for nb in nbrs if nb[3] > 0 and nb[0] > 150)
                if orange_or_white >= 3:
                    sarr[y, x] = [254, 160, 50, 255]

arm_clean = Image.fromarray(sarr)
# Now let's paste arm_clean onto body_img!
# Shoulder in arm_snap is at the top-left of the forearm.
# In forearm, shoulder was at (105, 105).
# In snap, rotated forearm had top-left at (103, 86).
# So in arm_snap bbox, shoulder is near (25, 25).
# In body_img, shoulder is at (250, 164).
# So paste at (250 - 25, 164 - 25) = (225, 139)
comp = body_img.copy()
comp.paste(arm_clean, (225, 130), arm_clean)

# Re-apply red scarf over the shoulder joint
carr = np.array(comp)
for y in range(150, 180):
    for x in range(235, 265):
        mr, mg, mb, ma = marr[y, x]
        if mr > 130 and mg < 70 and mb < 70:
            carr[y, x] = marr[y, x]

final = Image.fromarray(carr)
bbox = final.getbbox()
final.crop(bbox).save('docs_for_me/preview_action1_snap.png')
print('preview_action1_snap.png saved!')
