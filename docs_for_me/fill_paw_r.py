from PIL import Image
import numpy as np
from collections import deque

outline_img = Image.open('docs_for_me/debug_paw_r_outline.png')
out_arr = np.array(outline_img) > 0 # True where outline
raw_img = Image.open('docs_for_me/raw_paw_r.png').convert('RGB')
raw_arr = np.array(raw_img)
h, w = out_arr.shape

# Close any 1px gap in the outline boundary:
# Let's see: top left connects to shoulder, bottom right connects to desk
# Flood fill inside from (y=40, x=45)
inside = np.zeros((h, w), dtype=bool)
# Add outline itself to the arm
inside[out_arr] = True

# Fill interior:
q = deque([(40, 45), (50, 50), (60, 40), (45, 60)])
for y, x in q:
    inside[y, x] = True

while q:
    cy, cx = q.popleft()
    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
        ny, nx = cy + dy, cx + dx
        if 0 <= ny < h and 0 <= nx < w and not inside[ny, nx]:
            # If not outside desk / keyboard:
            # Desk is y > 82, space is x > 85 and y < 35
            if ny > 82:
                continue
            if nx > 85 and ny < 35:
                continue
            # If color is not space blue
            r, g, b = raw_arr[ny, nx]
            if (b > 65 and b > r) or (r < 30 and g < 40 and b > 40):
                continue
            inside[ny, nx] = True
            q.append((ny, nx))

# Now create RGBA image of the EXACT right arm
arm_rgba = np.zeros((h, w, 4), dtype=np.uint8)
for y in range(h):
    for x in range(w):
        if inside[y, x] and y <= 82:
            arm_rgba[y, x] = list(raw_arr[y, x]) + [255]

res = Image.fromarray(arm_rgba)
res.save('docs_for_me/perfect_paw_r.png')
print('perfect_paw_r.png saved!')
