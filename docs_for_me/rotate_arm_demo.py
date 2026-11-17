from PIL import Image
import numpy as np

# Load the authentic right arm sprite
arm_r = Image.open('docs_for_me/perfect_paw_r.png').convert('RGBA')
w, h = arm_r.size # 110, 105

# In raw_paw_r (110x105), the shoulder is around (x=25, y=30)
# Let's rotate arm_r around (25, 30) by 50, 60, and 70 degrees CCW
# (In PIL rotate, positive angle rotates CCW)
for angle in [45, 55, 60, 65]:
    # Expand canvas so it doesn't get clipped when rotated
    big_canvas = Image.new('RGBA', (w * 3, h * 3), (0, 0, 0, 0))
    # Place pivot at center of big_canvas (w*1.5, h*1.5)
    # Pivot in arm_r is (25, 30)
    px, py = 25, 30
    big_canvas.paste(arm_r, (int(w*1.5 - px), int(h*1.5 - py)), arm_r)
    
    # Rotate around center
    rotated = big_canvas.rotate(angle, resample=Image.Resampling.BICUBIC, center=(w*1.5, h*1.5))
    
    # Crop bounding box of non-zero alpha
    bbox = rotated.getbbox()
    if bbox:
        cropped = rotated.crop(bbox)
        cropped.save(f'docs_for_me/arm_r_rot_{angle}.png')
        print(f'Angle {angle} saved, size: {cropped.size}')

