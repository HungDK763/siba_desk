import numpy as np
from PIL import Image

# -----------------------------------------------------------------------------
# Color Palette
# -----------------------------------------------------------------------------
C_OUT     = np.array([55, 8, 2, 255], dtype=np.uint8)       # #370802 dark outline
C_OR_HI   = np.array([255, 195, 105, 255], dtype=np.uint8)  # fur highlight
C_OR_MID  = np.array([254, 160, 50, 255], dtype=np.uint8)   # fur base orange
C_OR_DK   = np.array([215, 105, 20, 255], dtype=np.uint8)   # fur deep orange
C_WH_HI   = np.array([255, 255, 255, 255], dtype=np.uint8)  # paw white
C_WH_SH1  = np.array([244, 238, 230, 255], dtype=np.uint8)  # soft warm white
C_WH_SH2  = np.array([225, 215, 202, 255], dtype=np.uint8)  # paw shadow
C_RED     = np.array([205, 38, 42, 255], dtype=np.uint8)    # red scarf
C_RED_DK  = np.array([138, 24, 28, 255], dtype=np.uint8)   # red scarf shadow
C_CLEAR   = np.array([0, 0, 0, 0], dtype=np.uint8)

# 1. Base dog on 104x78 canvas
raw_base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
canvas = Image.new('RGBA', (104, 78), (0,0,0,0))
canvas.paste(raw_base, (1, 1), raw_base)
base = np.array(canvas)

# Clean below desk line (y > 74)
for y in range(75, 78):
    for x in range(104):
        base[y, x] = C_CLEAR

# Clean desk line at y=74
for x in range(104):
    if x < 40 or (x > 67 and x < 74) or x > 98:
        base[74, x] = C_CLEAR

print("Base loaded successfully.")
