from PIL import Image
import numpy as np

# Load original 360x268
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGB')
arr = np.array(orig)
h, w, _ = arr.shape

# 1. Isolate the Right Arm & Paw from bongo_dog_crop.png
# Right arm region: x in [240, 335], y in [155, 255]
# Let's inspect pixels in this box
arm_r_rgba = np.zeros((95, 95, 4), dtype=np.uint8)
ox, oy = 240, 160

for y in range(160, 255):
    for x in range(240, 335):
        r, g, b = arr[y, x]
        # Space background: dark blue/black (r < 55, g < 60, b < 120 and b > r)
        # Note: stars are tiny white dots, but outside arm boundary
        # Keyboard at bottom: y > 235 and r > 210, g > 210, b > 210
        # Check if inside arm
        # Arm orange: r > 180, g > 90, b < 90
        # Arm outline: r < 75, g < 45, b < 40
        # Paw white: r > 200, g > 190, b > 190, with y < 240 (or inside paw outline)
        # Paw shadow: r in 180..230, g in 160..210, b in 140..190
        # Let's check flood fill or exact polygon mask
        pass

print('Script ready')
