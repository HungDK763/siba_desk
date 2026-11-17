from PIL import Image
import numpy as np
from collections import deque

orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGB')
arr = np.array(orig)
H, W, _ = arr.shape

# Space background flood fill from edges
visited = np.zeros((H, W), dtype=bool)
is_bg = np.zeros((H, W), dtype=bool)

# Flood fill from top, left, right edges where it's space background
q = deque()

# Check top row, left col, right col
for x in range(W):
    q.append((0, x))
    visited[0, x] = True
for y in range(H):
    q.append((y, 0))
    visited[y, 0] = True
    q.append((y, W-1))
    visited[y, W-1] = True

while q:
    cy, cx = q.popleft()
    r, g, b = arr[cy, cx]
    
    # Is this background pixel?
    # Space is: dark (r<35, g<35, b<60) or bluish (b > r + 15 and b > 40) or dark cyan/purple
    bg_pixel = ((b > r + 10) and b > 35) or (r < 35 and g < 40 and b < 50) or (r < 60 and g < 60 and b > 50)
    # Motion streaks (white arcs near right top):
    # If cy in [30, 80] and cx in [290, 350] and r > 200 and g > 200 and b > 200: it's motion lines in space!
    if (20 <= cy <= 90) and (280 <= cx <= 355) and (r > 180 and g > 180 and b > 180):
        bg_pixel = True
        
    if bg_pixel:
        is_bg[cy, cx] = True
        for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
            ny, nx = cy + dy, cx + dx
            if 0 <= ny < H and 0 <= nx < W and not visited[ny, nx]:
                visited[ny, nx] = True
                q.append((ny, nx))

# Now desk removal:
# The desk line starts around y=238.
# Let's flood fill desk from bottom row:
for x in range(W):
    # Bottom row is desk
    if not is_bg[H-1, x]:
        # If it's dark desk or white keyboard below paws
        is_bg[H-1, x] = True
        
# Refine desk: any pixel at y >= 238 that is NOT dog paw outline
# Paw outlines at y >= 238 are around x in [155..215] (left paw) and x in [265..325] (right paw)
for y in range(236, H):
    for x in range(W):
        r, g, b = arr[y, x]
        # Dark desk
        if (r < 65 and g < 65 and b < 65):
            # Desk border/surface
            # Only keep if it's the bottom outline of the paws (y <= 238 and x in [155..215] or x in [265..325])
            if y > 238 or not ((155 <= x <= 215) or (265 <= x <= 325)):
                is_bg[y, x] = True
        elif y >= 238:
            # White keyboard/paper below paws
            if r > 180 and g > 180 and b > 180:
                is_bg[y, x] = True

rgba = np.zeros((H, W, 4), dtype=np.uint8)
rgba[~is_bg] = np.dstack([arr, np.full((H, W), 255, dtype=np.uint8)])[~is_bg]

master = Image.fromarray(rgba)
bbox = master.getbbox()
master_crop = master.crop(bbox)
master_crop.save('docs_for_me/dog_clean_master_hd.png')
print('dog_clean_master_hd.png saved, bbox:', bbox, 'size:', master_crop.size)
