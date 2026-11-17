from PIL import Image, ImageDraw
import numpy as np

img = Image.open('docs_for_me/raw_paw_r.png').convert('RGBA')
arr = np.array(img)
h, w, _ = arr.shape

# Let's define the mask of the arm
# Arm consists of:
# 1. Orange sleeve & outline
# 2. White paw & outline & claw notches
mask = np.zeros((h, w), dtype=bool)

# Let's inspect the boundary of the paw
# Paw bottom edge: where the dark outline touches the desk
# Claws: dark notches around y=68..80
# Let's find all pixels connected to arm and paw
# Any pixel with:
# - Orange: r > 180, g in 90..210, b < 90
# - White/Cream paw: r > 180, g > 160, b > 140, y in 25..82
# - Dark outline of arm/paw: r < 75, g < 50, b < 45, adjacent to arm/paw
# Space pixels (blue): b > 70 or (r < 50 and g < 70)
# Keyboard white paper (below paw): y > 80

for y in range(h):
    for x in range(w):
        r, g, b, _ = arr[y, x]
        # Skip space
        if (b > 65 and b > r) or (r < 40 and g < 50 and b > 40):
            continue
        # Skip keyboard paper below y=82
        if y > 82:
            continue
        # Check if orange sleeve
        if r > 170 and g > 80 and b < 100:
            mask[y, x] = True
        # Check if white/cream paw
        elif r > 170 and g > 150 and b > 130 and y >= 30:
            mask[y, x] = True
        # Check if dark outline/claw
        elif r < 70 and g < 50 and b < 50 and y < 82 and x >= 5:
            mask[y, x] = True

# Clean mask with morphology (fill tiny holes)
from scipy.ndimage import binary_fill_holes
clean_mask = binary_fill_holes(mask)

# Save segmented right arm
arm_rgba = arr.copy()
arm_rgba[~clean_mask] = [0, 0, 0, 0]

res = Image.fromarray(arm_rgba)
res.save('docs_for_me/extracted_paw_r.png')
print('extracted_paw_r.png saved!')
