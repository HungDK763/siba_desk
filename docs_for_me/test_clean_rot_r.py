from PIL import Image, ImageDraw
import numpy as np

# Load clean base
body = Image.open('docs_for_me/body_no_arm_r.png').convert('RGBA')
bw, bh = body.size

# Load raw right arm
arm_raw = Image.open('docs_for_me/isolated_arm_r.png').convert('RGBA')

# Shoulder pivot: in 360x268 coords, px=250, py=166
# In body_no_arm_r: the coords match 347x268 (x is same, y is same)
px, py = 250, 166

cw, ch = 600, 600
canvas = Image.new('RGBA', (cw, ch), (0,0,0,0))
cx, cy = cw//2, ch//2
canvas.paste(arm_raw, (cx - px, cy - py), arm_raw)

# Try angles: 50, 55, 60
for ang in [50, 55, 60]:
    rot = canvas.rotate(ang, resample=Image.Resampling.BICUBIC, center=(cx, cy))
    rarr = np.array(rot)
    
    # Sharp alpha
    rarr[:,:,3] = np.where(rarr[:,:,3] > 110, 255, 0)
    
    # Clean up non-arm pixels (e.g. flat bottom desk remnants)
    # In rot, the flat bottom desk edge might have some black artifacts
    # Any near-black pixel at the outer tip can be cleaned
    rot_img = Image.fromarray(rarr)
    
    comp = body.copy()
    # Paste rotated arm
    comp.paste(rot_img, (px - cx, py - cy), rot_img)
    
    # Red scarf overlap:
    # Ensure red scarf stays in front of the shoulder attachment!
    master = Image.open('docs_for_me/dog_master_clean.png').convert('RGBA')
    marr = np.array(master)
    carr = np.array(comp)
    for y in range(150, 180):
        for x in range(235, 265):
            mr, mg, mb, ma = marr[y, x]
            if mr > 130 and mg < 65 and mb < 65: # scarf
                carr[y, x] = marr[y, x]
                
    final_comp = Image.fromarray(carr)
    bbox = final_comp.getbbox()
    final_comp.crop(bbox).save(f'docs_for_me/test_frame_r_{ang}.png')
    print(f'test_frame_r_{ang}.png saved, size:', final_comp.crop(bbox).size)
