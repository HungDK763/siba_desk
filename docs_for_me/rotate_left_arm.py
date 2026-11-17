from PIL import Image
import numpy as np

arm = Image.open('docs_for_me/real_left_arm.png').convert('RGBA')
# Shoulder in real_left_arm is at roughly (15, 10)
# (relative to the 100x76 image)
sx, sy = 15, 10

cw, ch = 300, 300
cx, cy = 150, 150

canvas = Image.new('RGBA', (cw, ch), (0,0,0,0))
canvas.paste(arm, (cx - sx, cy - sy), arm)

for ang in [-35, -45, -55, -60]:
    rot = canvas.rotate(ang, resample=Image.Resampling.BICUBIC, center=(cx, cy))
    rarr = np.array(rot)
    rarr[:,:,3] = np.where(rarr[:,:,3] > 110, 255, 0)
    rot_clean = Image.fromarray(rarr)
    crop = rot_clean.crop(rot_clean.getbbox())
    crop.save(f'docs_for_me/rot_left_{abs(ang)}.png')

print('Left arm rotations saved!')
