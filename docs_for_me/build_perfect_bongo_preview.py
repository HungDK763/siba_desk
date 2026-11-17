import numpy as np
from PIL import Image

# =============================================================================
# Exact Palette Definition
# =============================================================================
C_OUT     = np.array([55, 8, 2, 255], dtype=np.uint8)       # #370802 signature dark outline
C_OR_HI   = np.array([255, 195, 105, 255], dtype=np.uint8)  # fur highlight
C_OR_MID  = np.array([254, 160, 50, 255], dtype=np.uint8)   # fur base orange
C_OR_DK   = np.array([215, 105, 20, 255], dtype=np.uint8)   # fur deep orange shadow
C_WH_HI   = np.array([255, 255, 255, 255], dtype=np.uint8)  # pure paw white
C_WH_SH1  = np.array([244, 238, 230, 255], dtype=np.uint8)  # soft warm white shadow
C_WH_SH2  = np.array([225, 215, 202, 255], dtype=np.uint8)  # deeper paw shadow
C_RED     = np.array([205, 38, 42, 255], dtype=np.uint8)    # red scarf
C_RED_DK  = np.array([138, 24, 28, 255], dtype=np.uint8)   # red scarf shadow
C_CLEAR   = np.array([0, 0, 0, 0], dtype=np.uint8)

# -----------------------------------------------------------------------------
# 1. Base dog on 104x78 canvas
# -----------------------------------------------------------------------------
raw_base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
canvas = Image.new('RGBA', (104, 78), (0,0,0,0))
canvas.paste(raw_base, (1, 1), raw_base)
b = np.array(canvas)

# Clean below desk line (y > 74)
for y in range(75, 78):
    for x in range(104):
        b[y, x] = C_CLEAR

# Clean desk line at y=74: only paws / dog body touch desk
for x in range(104):
    if x < 40 or (x > 67 and x < 76) or x > 96:
        b[74, x] = C_CLEAR

# Extract scarf mask (R > 130, G < 65, B < 65, A > 150) plus outline
scarf_mask = np.zeros((78, 104), dtype=bool)
for y in range(8, 65):
    for x in range(18, 78):
        r, g, bc, a = b[y, x]
        if a > 100:
            if r > 130 and g < 65 and bc < 65:
                scarf_mask[y, x] = True

# Also include outline of scarf
scarf_pixels = {}
for y in range(8, 65):
    for x in range(18, 78):
        if scarf_mask[y, x]:
            scarf_pixels[(y, x)] = b[y, x].copy()
            # check 4-neighbors for dark outline
            for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
                ny, nx = y + dy, x + dx
                if 0 <= ny < 78 and 0 <= nx < 104:
                    nr, ng, nb, na = b[ny, nx]
                    if na > 100 and nr < 70 and ng < 45 and nb < 40:
                        scarf_pixels[(ny, nx)] = b[ny, nx].copy()

def restore_scarf(target_arr):
    for (y, x), val in scarf_pixels.items():
        target_arr[y, x] = val

# Helper to apply pixel grid
def apply_grid(target, start_y, start_x, grid):
    for r_idx, row in enumerate(grid):
        y = start_y + r_idx
        for c_idx, ch in enumerate(row):
            x = start_x + c_idx
            if 0 <= y < 78 and 0 <= x < 104:
                if ch == '#': target[y, x] = C_OUT
                elif ch == 'L': target[y, x] = C_OR_HI
                elif ch == 'O': target[y, x] = C_OR_MID
                elif ch == 'D': target[y, x] = C_OR_DK
                elif ch == '.': target[y, x] = C_WH_HI
                elif ch == 's': target[y, x] = C_WH_SH1
                elif ch == 'S': target[y, x] = C_WH_SH2
                elif ch == 'C': target[y, x] = C_OUT
                elif ch == 'X': target[y, x] = C_CLEAR

print("Scarf pixels identified:", len(scarf_pixels))
