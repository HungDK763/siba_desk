from PIL import Image
import numpy as np

# Load perfect_paw_r
img = Image.open('docs_for_me/perfect_paw_r.png').convert('RGBA')
arr = np.array(img)
h, w, _ = arr.shape

# The arm is orange, outline is dark brown, paw is white
# Remove red scarf pixels (r > 150, g < 60, b < 60)
# Remove white chest pixels on the left (x < 15, y > 20)
for y in range(h):
    for x in range(w):
        r, g, b, a = arr[y, x]
        if a == 0:
            continue
        # If red scarf
        if r > 150 and g < 65 and b < 65:
            arr[y, x] = [0, 0, 0, 0]
        # If white chest on left
        if x < 15 and r > 200 and g > 200 and b > 200:
            arr[y, x] = [0, 0, 0, 0]

arm_only = Image.fromarray(arr)
# Crop to tight bbox
bbox = arm_only.getbbox()
arm_tight = arm_only.crop(bbox)
arm_tight.save('docs_for_me/arm_r_tight.png')
print('arm_r_tight.png saved, size:', arm_tight.size)
