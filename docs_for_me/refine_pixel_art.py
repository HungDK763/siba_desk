import numpy as np
from PIL import Image

C_OUT     = np.array([55, 8, 2, 255], dtype=np.uint8)       # #370802 dark outline
C_OR_HI   = np.array([255, 185, 95, 255], dtype=np.uint8)   # fur highlight
C_OR_MID  = np.array([254, 160, 50, 255], dtype=np.uint8)   # fur base orange
C_OR_DK   = np.array([215, 105, 20, 255], dtype=np.uint8)   # fur deep orange
C_WH_HI   = np.array([255, 255, 255, 255], dtype=np.uint8)  # pure white
C_WH_SH1  = np.array([244, 238, 230, 255], dtype=np.uint8)  # soft warm white
C_WH_SH2  = np.array([225, 215, 202, 255], dtype=np.uint8)  # deep paw shadow
C_RED     = np.array([205, 38, 42, 255], dtype=np.uint8)    # red scarf
C_CLEAR   = np.array([0, 0, 0, 0], dtype=np.uint8)

# -----------------------------------------------------------------------------
# 1. Study the LEFT RESTING PAW from bongo_base_102x76.png
# Let's inspect its exact pixels and replicate its beauty for the right paw!
# -----------------------------------------------------------------------------
raw_base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
b_arr = np.array(raw_base)

# Left resting paw: x in [45..67], y in [63..75]
print("Left resting paw dimensions in bongo_base:")
for y in range(63, 76):
    row_chars = ""
    for x in range(45, 68):
        r, g, b, a = b_arr[y, x]
        if a < 30: row_chars += "  "
        elif r < 60 and g < 40: row_chars += "##"
        elif r > 230 and g > 220: row_chars += ".."
        elif r > 190 and g > 180: row_chars += "::"
        elif r > 180 and g > 80: row_chars += "OO"
        elif r > 140 and g < 60: row_chars += "RR"
        else: row_chars += "++"
    print(f"{y:2d}: {row_chars}")
