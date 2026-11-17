from PIL import Image
import numpy as np

# Load bongo_dog_crop
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
arr = np.array(orig)

# Left arm in bongo_dog_crop:
# Shoulder is under red scarf: x in [128, 155], y in [172, 195]
# Paw is at x in [150, 220], y in [200, 252]
# Let's create clean mask for left arm
lh, lw, _ = arr.shape
mask_l = np.zeros((lh, lw), dtype=bool)

for y in range(175, 252):
    for x in range(125, 225):
        r, g, b, a = arr[y, x]
        # Red scarf check
        if r > 130 and g < 65 and b < 65 and y < 185:
            continue
        # Table below y=250
        if y > 250:
            continue
        # Left flank (x < 132)
        if x < 132 and y < 210:
            continue
        # Colors: orange sleeve or white paw or dark outline
        if (r > 160 and g > 70 and b < 120) or \
           (r > 160 and g > 150 and b > 130) or \
           (r < 80 and g < 60 and b < 50):
            mask_l[y, x] = True

# Also remove chest fur on the right of the paw (x > 185, y < 215 where r > 200, g > 200, b > 200)
for y in range(175, 215):
    for x in range(185, 225):
        r, g, b = arr[y, x, :3]
        if r > 200 and g > 200 and b > 200:
            mask_l[y, x] = False

arm_l = np.zeros((lh, lw, 4), dtype=np.uint8)
arm_l[mask_l] = arr[mask_l]
img_l = Image.fromarray(arm_l)
bbox_l = img_l.getbbox()
print('Left arm bbox:', bbox_l)
crop_l = img_l.crop(bbox_l)
crop_l.save('docs_for_me/real_left_arm.png')
print('real_left_arm.png saved, size:', crop_l.size)
