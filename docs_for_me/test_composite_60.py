from PIL import Image
import numpy as np

# Load clean master dog
body = Image.open('docs_for_me/dog_clean_master_hd.png').convert('RGBA')
bw, bh = body.size

# Load rotated arm (60 deg)
arm = Image.open('docs_for_me/isolated_arm_r.png').convert('RGBA')

# Shoulder pivot in bongo_dog_crop coordinates:
px, py = 250, 166

# Rotate arm around (px, py)
cw, ch = 600, 600
canvas = Image.new('RGBA', (cw, ch), (0,0,0,0))
cx, cy = cw//2, ch//2
canvas.paste(arm, (cx - px, cy - py), arm)

for ang in [45, 55, 60]:
    rot = canvas.rotate(ang, resample=Image.Resampling.BICUBIC, center=(cx, cy))
    # clean alpha
    rarr = np.array(rot)
    rarr[:,:,3] = np.where(rarr[:,:,3] > 120, 255, 0)
    rot_clean = Image.fromarray(rarr)
    
    # We want (cx, cy) to align with (px, py) on body:
    # paste at (px - cx, py - cy)
    comp = body.copy()
    
    # Before pasting rotated arm, we must erase the old arm from body:
    # Old arm is in x in [245, 335], y in [165, 245]
    barr = np.array(comp)
    for y in range(165, bh):
        for x in range(240, bw):
            r, g, b, a = barr[y, x]
            # preserve scarf
            if r > 130 and g < 65 and b < 65 and y < 175:
                continue
            # erase old arm
            barr[y, x] = [0, 0, 0, 0]
            
    # Fill in dog's flank behind the old arm:
    # Dog's flank is orange fur curving down from shoulder (x=248, y=166) to desk (x=255, y=238)
    for y in range(166, 238):
        # Flank right contour:
        flank_edge = int(252 + 5 * np.sin((y - 166) / (238 - 166) * np.pi))
        for x in range(235, flank_edge):
            barr[y, x] = [248, 139, 38, 255] # Shiba orange
        # Dark outline
        barr[y, flank_edge] = [60, 22, 10, 255]
        barr[y, flank_edge+1] = [60, 22, 10, 255]
        
    comp_clean = Image.fromarray(barr)
    # Paste rotated arm
    comp_clean.paste(rot_clean, (px - cx, py - cy), rot_clean)
    comp_clean.save(f'docs_for_me/comp_dog_r_{ang}.png')
    print(f'comp_dog_r_{ang}.png saved!')
