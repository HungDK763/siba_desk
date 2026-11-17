from PIL import Image
import numpy as np
from collections import deque

img = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGB')
arr = np.array(img)
H, W, _ = arr.shape

# Flood fill background
is_bg = np.zeros((H, W), dtype=bool)
visited = np.zeros((H, W), dtype=bool)

# Add all border pixels
q = deque()
for x in range(W):
    q.append((0, x))
    visited[0, x] = True
for y in range(H):
    q.append((y, 0))
    visited[y, 0] = True
    q.append((y, W - 1))
    visited[y, W - 1] = True

while q:
    cy, cx = q.popleft()
    r, g, b = arr[cy, cx]
    
    # Is this background?
    # Space background:
    # 1. Dark blue/purple or black: r < 40 and g < 40 and b < 80
    # 2. Blue channel high: b > r + 8 and b > 35
    # 3. Motion arcs in top right: cy < 110 and cx > 270 and (r > 150 and g > 150 and b > 150)
    # 4. Desk: cy >= 238 and (r < 65 and g < 65 and b < 65)
    # 5. Paper on desk: cy >= 238 and not dog paw
    bg = False
    if (cy < 120 and cx > 270) and (r > 140 and g > 140 and b > 140):
        bg = True
    elif (b > int(r) + 8 and b > 32):
        bg = True
    elif (r < 40 and g < 45 and b < 85):
        bg = True
    elif cy >= 238 and (r < 70 and g < 70 and b < 70):
        bg = True
    elif cy >= 238 and not ((150 <= cx <= 220) or (260 <= cx <= 330)):
        bg = True
        
    if bg:
        is_bg[cy, cx] = True
        for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
            ny, nx = cy + dy, cx + dx
            if 0 <= ny < H and 0 <= nx < W and not visited[ny, nx]:
                visited[ny, nx] = True
                q.append((ny, nx))

# Now mask the dog
clean_arr = np.zeros((H, W, 4), dtype=np.uint8)
clean_arr[~is_bg] = np.dstack([arr, np.full((H, W), 255, dtype=np.uint8)])[~is_bg]

dog = Image.fromarray(clean_arr)
# Crop to dog
bbox = dog.getbbox()
dog_crop = dog.crop(bbox)
dog_crop.save('docs_for_me/perfect_dog_master.png')
print('perfect_dog_master.png saved, bbox:', bbox, 'size:', dog_crop.size)
