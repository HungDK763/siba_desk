from PIL import Image
import numpy as np

img = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
arr = np.array(img)
H, W, _ = arr.shape

# Let's inspect the colors of right arm:
# Orange sleeve: x in [245, 290], y in [165, 215]
# White paw: x in [265, 330], y in [200, 240]

# Let's create an exact mask for the right arm including:
# - Orange arm
# - White paw
# - Dark outline of the arm and paw
# - Dark claws
# But NOT the desk underneath, NOT the red scarf above, NOT the space background to the right/top

mask_r = np.zeros((H, W), dtype=bool)

# Region of interest for right arm
for y in range(160, 245):
    for x in range(240, 340):
        r, g, b, a = arr[y, x]
        # Space background check:
        if (b > 60 and b > r) or (r < 40 and g < 40 and b > 40):
            continue
        # Desk check:
        # The desk is dark grey (r < 60, g < 60, b < 60) below y=235, or white paper below y=236
        if y >= 238:
            continue
        # Red scarf check:
        if r > 130 and g < 65 and b < 65 and y < 172:
            continue
        # Torso check to the left of the arm:
        # Dog's chest is x < 246 when y > 175
        if x < 246 and (r > 190 and g > 170 and b > 150):
            continue
            
        # If it's part of the arm:
        # 1. Orange fur: r > 180, g in [80..180], b < 90
        # 2. White paw: r > 180, g > 170, b > 150
        # 3. Outline / Claws: r < 75, g < 50, b < 45
        # 4. Anti-aliasing / shading on fur or paw:
        if (r > 160 and g > 70 and b < 110) or \
           (r > 160 and g > 150 and b > 130) or \
           (r < 80 and g < 55 and b < 50 and y >= 165):
            mask_r[y, x] = True

# Now flood-fill from a known arm point to remove disconnected noise
from collections import deque
visited = np.zeros((H, W), dtype=bool)
clean_mask_r = np.zeros((H, W), dtype=bool)

# Seeds on the arm:
seeds = [(180, 260), (190, 270), (215, 290), (225, 300)]
q = deque()
for sy, sx in seeds:
    if mask_r[sy, sx]:
        q.append((sy, sx))
        visited[sy, sx] = True
        clean_mask_r[sy, sx] = True

while q:
    cy, cx = q.popleft()
    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
        ny, nx = cy + dy, cx + dx
        if 160 <= ny < 245 and 240 <= nx < 340 and not visited[ny, nx]:
            visited[ny, nx] = True
            if mask_r[ny, nx]:
                clean_mask_r[ny, nx] = True
                q.append((ny, nx))

arm_r_img = np.zeros((H, W, 4), dtype=np.uint8)
arm_r_img[clean_mask_r] = arr[clean_mask_r]
Image.fromarray(arm_r_img).save('docs_for_me/isolated_arm_r.png')
print('isolated_arm_r.png saved!')
