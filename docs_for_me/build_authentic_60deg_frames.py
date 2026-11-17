from PIL import Image, ImageDraw
import numpy as np

# 1. Load pristine master dog
base_img = Image.open('docs_for_me/perfect_dog_master.png').convert('RGBA')
barr = np.array(base_img)
H, W, _ = barr.shape

# Clean borders:
# Remove top-left UI frame (y < 145 and x < 45)
barr[:145, :45] = [0,0,0,0]
# Remove top-right space artifacts (y < 150 and x > 285)
barr[:150, 285:] = [0,0,0,0]
# Clean bottom table at y >= 238
barr[238:, :] = [0,0,0,0]

master_clean = Image.fromarray(barr)
master_clean.save('docs_for_me/dog_master_clean.png')

# -------------------------------------------------------------
# Color Palette from original bongo_dog_crop.png
# -------------------------------------------------------------
C_OUTLINE    = (58, 16, 6, 255)       # Dark brown outline #3a1006
C_ORANGE     = (255, 163, 52, 255)    # Vibrant Shiba Orange #ffa334
C_ORANGE_MID = (248, 139, 38, 255)    # Mid orange #f88b26
C_ORANGE_DRK = (220, 107, 29, 255)    # Shadow orange #dc6b1d
C_ORANGE_LGT = (255, 185, 90, 255)    # Highlight orange
C_WHITE      = (255, 254, 250, 255)   # Cream white #fffefa
C_WHITE_SHD  = (238, 222, 204, 255)   # Paw shadow #eedecc
C_WHITE_DRK  = (200, 185, 170, 255)   # Dark white shadow
C_CLAW       = (45, 8, 0, 255)        # Claw notch #2d0800
C_SCARF      = (217, 61, 49, 255)     # Scarf red #d93d31
C_SCARF_DRK  = (136, 37, 31, 255)     # Scarf dark red

print('Master clean ready, size:', master_clean.size)
