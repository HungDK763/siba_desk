from PIL import Image, ImageDraw
import numpy as np

# Load master dog (319x241)
dog_orig = Image.open('docs_for_me/dog_perfect_master2.png').convert('RGBA')
MW, MH = dog_orig.size # 319, 241

# Load clean right arm
arm_r = Image.open('docs_for_me/arm_r_tight.png').convert('RGBA')

# 1. Prepare Left Paw Sprite from master:
# Left paw is at x in [130, 215], y in [175, 241]
# Let's crop it cleanly
paw_l_crop = dog_orig.crop((130, 175, 215, 241))
l_arr = np.array(paw_l_crop)
# Remove scarf red (y < 20, r > 150)
for y in range(l_arr.shape[0]):
    for x in range(l_arr.shape[1]):
        r, g, b, a = l_arr[y, x]
        if r > 150 and g < 65 and b < 65 and y < 22:
            l_arr[y, x] = [0, 0, 0, 0]
arm_l = Image.fromarray(l_arr)
bbox_l = arm_l.getbbox()
arm_l_tight = arm_l.crop(bbox_l)
arm_l_tight.save('docs_for_me/arm_l_tight.png')
print('arm_l_tight saved, size:', arm_l_tight.size)

