from PIL import Image
import numpy as np
from collections import deque

img = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
arr = np.array(img)
H, W, _ = arr.shape

# Left arm in bongo_dog_crop:
# Orange sleeve: x in [125, 175], y in [170, 215]
# White paw: x in [155, 225], y in [200, 242]

mask_l = np.zeros((H, W), dtype=bool)

for y in range(165, 242):
    for x in range(125, 225):
        r, g, b, a = arr[y, x]
        # Red scarf check
        if r > 130 and g < 65 and b < 65 and y < 178:
            continue
        # Desk check below y=238
        if y >= 238:
            continue
            
        # Left flank to the left of the arm (x < 132 and orange):
        # We want the arm sleeve, which starts around x=135
        if x < 132:
            continue
            
        # Color match for paw/arm
        if (r > 160 and g > 70 and b < 110) or \
           (r > 160 and g > 150 and b > 130) or \
           (r < 80 and g < 55 and b < 50):
            mask_l[y, x] = True

# Flood fill from seeds inside left arm
seeds = [(185, 145), (195, 155), (215, 180), (225, 190)]
visited = np.zeros((H, W), dtype=bool)
clean_mask_l = np.zeros((H, W), dtype=bool)

q = deque()
for sy, sx in seeds:
    if mask_l[sy, sx]:
        q.append((sy, sx))
        visited[sy, sx] = True
        clean_mask_l[sy, sx] = True

while q:
    cy, cx = q.popleft()
    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
        ny, nx = cy + dy, cx + dx
        if 165 <= ny < 242 and 125 <= nx < 225 and not visited[ny, nx]:
            visited[ny, nx] = True
            if mask_l[ny, nx]:
                clean_mask_l[ny, nx] = True
                q.append((ny, nx))

arm_l_img = np.zeros((H, W, 4), dtype=np.uint8)
arm_l_img[clean_mask_l] = arr[clean_mask_l]
img_l = Image.fromarray(arm_l_img)
bbox = img_l.getbbox()
print('Left arm bbox:', bbox)
crop_l = img_l.crop(bbox)
crop_l.save('docs_for_me/isolated_arm_l_tight.png')
print('isolated_arm_l_tight.png saved!')
