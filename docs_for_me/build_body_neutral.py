from PIL import Image, ImageDraw
import numpy as np

# Load master clean
master = Image.open('docs_for_me/dog_master_clean.png').convert('RGBA')
marr = np.array(master)
H, W, _ = marr.shape

# 1. Neutral Body with RIGHT ARM erased & flank filled:
body_no_arm_r = marr.copy()

# Erase right arm area
for y in range(165, 238):
    for x in range(242, W):
        r, g, b, a = body_no_arm_r[y, x]
        # Preserve red scarf pixels
        if r > 140 and g < 70 and b < 70 and y < 178:
            continue
        body_no_arm_r[y, x] = [0, 0, 0, 0]

# Fill dog right flank curving naturally down to the desk:
# At y=168: flank edge is around x=254
# At y=200: flank edge curves to x=262
# At y=237: flank edge is at x=265
for y in range(168, 238):
    # Smooth natural flank curve
    t = (y - 168) / (238 - 168)
    flank_x = int(252 + 14 * np.sin(t * np.pi * 0.7))
    for x in range(238, flank_x):
        # fur color with depth shading
        if x < flank_x - 4:
            body_no_arm_r[y, x] = [255, 163, 52, 255] # orange
        else:
            body_no_arm_r[y, x] = [220, 107, 29, 255] # shadow orange
    # Outline (2 pixels thick)
    body_no_arm_r[y, flank_x] = [58, 16, 6, 255]
    body_no_arm_r[y, flank_x + 1] = [58, 16, 6, 255]

Image.fromarray(body_no_arm_r).save('docs_for_me/body_no_arm_r.png')
print('body_no_arm_r.png saved!')
