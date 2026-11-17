from PIL import Image
import numpy as np

# Load full dog (319x241)
dog = Image.open('docs_for_me/dog_perfect_master2.png').convert('RGBA')
# Right arm sprite
arm = Image.open('docs_for_me/arm_r_tight.png').convert('RGBA')

# In dog_perfect_master2:
# Shoulder pivot is at x=245, y=165
# Pivot in arm_r_tight is at px=22, py=18
px, py = 22, 18
sx, sy = 245, 165

# 1. Base dog with right arm erased:
# We erase the right arm in dog (x in 235..319, y in 160..241)
# and fill the right flank with natural dog fur from x in 215..245
d_arr = np.array(dog)

# Erase right arm area
for y in range(160, 241):
    for x in range(235, 319):
        # preserve scarf
        r, g, b, a = d_arr[y, x]
        if r > 140 and g < 65 and b < 65 and y < 195:
            continue
        # Fill flank curving down
        flank_limit = int(260 - (y - 160) * 0.25)
        if x < flank_limit:
            # fur color from dog flank at x=220
            d_arr[y, x] = d_arr[y, 220]
        elif x == flank_limit:
            d_arr[y, x] = [45, 20, 10, 255]
        else:
            d_arr[y, x] = [0, 0, 0, 0]

body_no_arm_r = Image.fromarray(d_arr)

# 2. Rotate arm by 55 degrees CCW
canvas = Image.new('RGBA', (400, 400), (0, 0, 0, 0))
cx, cy = 200, 200
canvas.paste(arm, (cx - px, cy - py), arm)
rot_arm = canvas.rotate(55, resample=Image.Resampling.BICUBIC, center=(cx, cy))

# Paste onto body_no_arm_r at (sx, sy)
f_tap_l = body_no_arm_r.copy()
f_tap_l.paste(rot_arm, (sx - cx, sy - cy), rot_arm)

f_tap_l.save('docs_for_me/test_tap_l_rot55.png')
print('test_tap_l_rot55.png saved!')
