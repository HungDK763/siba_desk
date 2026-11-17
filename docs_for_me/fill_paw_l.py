from PIL import Image
import numpy as np
from collections import deque

raw_img = Image.open('docs_for_me/raw_paw_l.png').convert('RGB')
raw_arr = np.array(raw_img)
h, w, _ = raw_arr.shape

# Outline is dark brown: r in 20..85, g in 10..55, b in 10..55
out_arr = (raw_arr[:, :, 0] < 85) & (raw_arr[:, :, 1] < 55) & (raw_arr[:, :, 2] < 55) & (raw_arr[:, :, 0] > 15)

# In raw_paw_l:
# Left paw is centered around x in 30..80, y in 20..70
# Desk line at bottom: y >= 68
# Red scarf at top right: r > 140, g < 60, b < 60
inside = np.zeros((h, w), dtype=bool)
inside[out_arr] = True

# Seed inside paw
q = deque([(45, 55), (40, 50), (50, 60), (35, 45)])
for y, x in q:
    inside[y, x] = True

while q:
    cy, cx = q.popleft()
    for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
        ny, nx = cy + dy, cx + dx
        if 0 <= ny < h and 0 <= nx < w and not inside[ny, nx]:
            # Stop if desk (y > 67)
            if ny > 67:
                continue
            # Stop if scarf red
            r, g, b = raw_arr[ny, nx]
            if r > 150 and g < 60 and b < 60:
                continue
            # Stop if space / gray desk on left
            if r < 50 and g < 50 and b < 50 and nx < 15:
                continue
            inside[ny, nx] = True
            q.append((ny, nx))

paw_l_rgba = np.zeros((h, w, 4), dtype=np.uint8)
for y in range(h):
    for x in range(w):
        if inside[y, x] and y <= 67:
            paw_l_rgba[y, x] = list(raw_arr[y, x]) + [255]

res = Image.fromarray(paw_l_rgba)
res.save('docs_for_me/perfect_paw_l.png')
print('perfect_paw_l.png saved!')
