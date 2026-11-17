from PIL import Image
import numpy as np

arm = Image.open('docs_for_me/arm_r_tight.png').convert('RGBA')
w, h = arm.size

px, py = 22, 18

for deg in [50, 55, 60]:
    canvas = Image.new('RGBA', (300, 300), (0, 0, 0, 0))
    cx, cy = 150, 150
    canvas.paste(arm, (cx - px, cy - py), arm)
    rot = canvas.rotate(deg, resample=Image.Resampling.BICUBIC, center=(cx, cy))
    
    # Save test
    bbox = rot.getbbox()
    rot.crop(bbox).save(f'docs_for_me/arm_r_rot_{deg}deg.png')
    print(f'Saved arm_r_rot_{deg}deg.png')
