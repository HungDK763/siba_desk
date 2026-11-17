from PIL import Image
import numpy as np
from collections import deque

orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGB')
arr = np.array(orig)
H, W, _ = arr.shape

# ---------------------------------------------------------
# 1. Segment Right Arm
# ---------------------------------------------------------
# Crop area in bongo_dog_crop: x in [245, 340], y in [155, 250]
# Pivot/shoulder is around (255, 175) in bongo_dog_crop
crop_r = arr[155:250, 245:340]
ch, cw, _ = crop_r.shape

# In this crop:
# Orange arm seed: around (x=15, y=25) -> global (260, 180)
# White paw seed: around (x=50, y=55) -> global (295, 210)
visited = np.zeros((ch, cw), dtype=bool)
is_arm = np.zeros((ch, cw), dtype=bool)

# Seed list (points definitely inside arm or paw)
q = deque([(25, 15), (55, 50), (40, 30), (60, 60)])
for y, x in q:
    visited[y, x] = True
    is_arm[y, x] = True

while q:
    cy, cx = q.popleft()
    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
        ny, nx = cy + dy, cx + dx
        if 0 <= ny < ch and 0 <= nx < cw and not visited[ny, nx]:
            visited[ny, nx] = True
            r, g, b = crop_r[ny, nx]
            # Stop condition:
            # 1. Space background: blue / black (b > 60 and b > r) or (r < 35 and g < 40 and b > 35)
            if (b > 60 and b > r) or (r < 35 and g < 40 and b > 35):
                continue
            # 2. Desk / Keyboard white sheet below paw:
            # Paw ends around y=78 in crop. The white sheet below paw is at y > 76 with x > 25
            # and dark outline at y=76 separates them.
            if ny > 77 and (r > 190 and g > 190 and b > 190):
                continue
            # 3. Desk dark line below arm:
            if ny > 80:
                continue

            # If pixel is arm orange, white paw, paw shadow, or dark outline:
            is_orange = (r > 170 and g > 80 and b < 100)
            is_white = (r > 170 and g > 150 and b > 130)
            is_outline = (r < 75 and g < 50 and b < 50)

            if is_orange or is_white or is_outline:
                is_arm[ny, nx] = True
                q.append((ny, nx))

# Create RGBA for right arm
arm_r_rgba = np.zeros((ch, cw, 4), dtype=np.uint8)
arm_r_rgba[is_arm] = np.dstack([crop_r, np.full((ch, cw), 255, dtype=np.uint8)])[is_arm]

img_arm_r = Image.fromarray(arm_r_rgba)
img_arm_r.save('docs_for_me/authentic_paw_r.png')
print('authentic_paw_r.png saved, size:', img_arm_r.size)

# ---------------------------------------------------------
# 2. Segment Left Arm/Paw
# ---------------------------------------------------------
# Crop area in bongo_dog_crop: x in [130, 220], y in [185, 255]
crop_l = arr[185:255, 130:220]
lh, lw, _ = crop_l.shape

visited_l = np.zeros((lh, lw), dtype=bool)
is_paw_l = np.zeros((lh, lw), dtype=bool)

# Paw seed: center of white paw at (x=45, y=35)
q_l = deque([(35, 45), (30, 40), (20, 25)])
for y, x in q_l:
    visited_l[y, x] = True
    is_paw_l[y, x] = True

while q_l:
    cy, cx = q_l.popleft()
    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
        ny, nx = cy + dy, cx + dx
        if 0 <= ny < lh and 0 <= nx < lw and not visited_l[ny, nx]:
            visited_l[ny, nx] = True
            r, g, b = crop_l[ny, nx]
            # Stop if red scarf
            if r > 140 and g < 60 and b < 60:
                continue
            # Stop if desk below paw (y > 52 in crop)
            if ny > 53:
                continue
            # Paw colors:
            is_orange = (r > 170 and g > 80 and b < 100)
            is_white = (r > 170 and g > 150 and b > 130)
            is_outline = (r < 75 and g < 50 and b < 50)
            if is_orange or is_white or is_outline:
                is_paw_l[ny, nx] = True
                q_l.append((ny, nx))

arm_l_rgba = np.zeros((lh, lw, 4), dtype=np.uint8)
arm_l_rgba[is_paw_l] = np.dstack([crop_l, np.full((lh, lw), 255, dtype=np.uint8)])[is_paw_l]

img_arm_l = Image.fromarray(arm_l_rgba)
img_arm_l.save('docs_for_me/authentic_paw_l.png')
print('authentic_paw_l.png saved, size:', img_arm_l.size)
