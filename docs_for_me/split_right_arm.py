from PIL import Image
import numpy as np

arm = Image.open('docs_for_me/isolated_arm_r_tight.png').convert('RGBA')
arr = np.array(arm)
h, w, _ = arr.shape
print(f'Arm size: {w}x{h}')

# In isolated_arm_r_tight:
# White paw is x > 25, y > 35 where color is white or claw
# Orange forearm is x < 50, y < 50 where color is orange
is_white_paw = np.zeros((h, w), dtype=bool)
is_orange_forearm = np.zeros((h, w), dtype=bool)

for y in range(h):
    for x in range(w):
        r, g, b, a = arr[y, x]
        if a == 0:
            continue
        # white paw or claws or paw outline
        if (r > 170 and g > 150 and b > 130) or (x >= 35 and y >= 25):
            is_white_paw[y, x] = True
        else:
            is_orange_forearm[y, x] = True

# Let's save both
paw_img = np.zeros((h, w, 4), dtype=np.uint8)
paw_img[is_white_paw] = arr[is_white_paw]
Image.fromarray(paw_img).crop(Image.fromarray(paw_img).getbbox()).save('docs_for_me/split_paw_r.png')

forearm_img = np.zeros((h, w, 4), dtype=np.uint8)
forearm_img[is_orange_forearm] = arr[is_orange_forearm]
Image.fromarray(forearm_img).crop(Image.fromarray(forearm_img).getbbox()).save('docs_for_me/split_forearm_r.png')
print('Split arm saved!')
