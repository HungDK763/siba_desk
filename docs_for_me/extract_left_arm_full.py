from PIL import Image
import numpy as np

img = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
arr = np.array(img)
H, W, _ = arr.shape

mask_l = np.zeros((H, W), dtype=bool)

for y in range(175, 238):
    for x in range(125, 222):
        r, g, b, a = arr[y, x]
        # Red scarf
        if r > 130 and g < 65 and b < 65 and y < 182:
            continue
        # Table below y=237
        if y >= 237:
            continue
        # Left flank boundary (x < 130)
        if x < 130:
            continue
        # Chest fur above paw:
        # If x > 175 and y < 210, that's the white chest fur, NOT the paw
        if x > 175 and y < 211 and (r > 200 and g > 200 and b > 200):
            continue
        # Check if orange arm or white paw or dark outline
        is_orange = (r > 160 and g > 70 and b < 120)
        is_white = (r > 170 and g > 160 and b > 140)
        is_dark = (r < 80 and g < 60 and b < 50)
        if is_orange or is_white or is_dark:
            mask_l[y, x] = True

arm_l_img = np.zeros((H, W, 4), dtype=np.uint8)
arm_l_img[mask_l] = arr[mask_l]
img_l = Image.fromarray(arm_l_img)
crop_l = img_l.crop(img_l.getbbox())
crop_l.save('docs_for_me/isolated_arm_l_full.png')
print('isolated_arm_l_full.png saved, size:', crop_l.size)
