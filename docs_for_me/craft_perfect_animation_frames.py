import numpy as np
from PIL import Image

# -----------------------------------------------------------------------------
# Color Constants
# -----------------------------------------------------------------------------
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

# Load base dog (102x76) on 104x78 canvas
raw_base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
canvas = Image.new('RGBA', (104, 78), (0,0,0,0))
canvas.paste(raw_base, (1, 1), raw_base)
b = np.array(canvas)

# Clean below desk line (y > 74)
for y in range(75, 78):
    for x in range(104):
        b[y, x] = C_CLEAR

# Clean desk line at y=74
for x in range(104):
    if x < 45 or (x > 67 and x < 76) or x > 96:
        b[74, x] = C_CLEAR

# Remove motion arcs on the far right
for y in range(48, 68):
    for x in range(94, 104):
        b[y, x] = C_CLEAR

# -----------------------------------------------------------------------------
# Extract Scarf (Red fabric + its outline)
# MUST BE PRESERVED 100% IN ALL FRAMES!
# -----------------------------------------------------------------------------
scarf_mask = np.zeros((78, 104), dtype=bool)
for y in range(8, 65):
    for x in range(18, 78):
        r, g, bc, a = b[y, x]
        if a > 100:
            if r > 130 and g < 65 and bc < 65:
                scarf_mask[y, x] = True

scarf_pixels = {}
for y in range(8, 65):
    for x in range(18, 78):
        if scarf_mask[y, x]:
            scarf_pixels[(y, x)] = b[y, x].copy()
            for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
                ny, nx = y + dy, x + dx
                if 0 <= ny < 78 and 0 <= nx < 104:
                    nr, ng, nb, na = b[ny, nx]
                    if na > 100 and nr < 70 and ng < 45 and nb < 40:
                        scarf_pixels[(ny, nx)] = b[ny, nx].copy()

def protect_scarf(img_arr):
    for (y, x), val in scarf_pixels.items():
        img_arr[y, x] = val

# -----------------------------------------------------------------------------
# Helper to apply pixel grid
# -----------------------------------------------------------------------------
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

# =============================================================================
# 1. ACTION 3: REST (Both Paws Down on Desk)
# =============================================================================
f_rest = b.copy()

# Clean right paw on desk (remove noise, crisp outlines)
for y in range(63, 75):
    for x in range(78, 97):
        if y < 74:
            # fill interior with clean white if white
            r, g, bc, a = f_rest[y, x]
            if r > 180 and g > 180 and bc > 180:
                f_rest[y, x] = C_WH_HI if y < 72 else C_WH_SH1
        elif y == 74:
            f_rest[y, x] = C_OUT

# Right paw claws on desk
for y in range(70, 74):
    f_rest[y, 83] = C_OUT
    f_rest[y, 89] = C_OUT

# Right paw outer right outline
for y in range(64, 74):
    f_rest[y, 96] = C_OUT
for x in range(97, 104):
    for y in range(60, 78):
        f_rest[y, x] = C_CLEAR

# Left paw desk contact
for x in range(46, 68):
    if f_rest[74, x, 3] > 0:
        f_rest[74, x] = C_OUT

protect_scarf(f_rest)

# =============================================================================
# 2. ACTION 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# Right arm raises to x in [76..100], y in [35..52] - completely away from scarf!
# =============================================================================
f1 = f_rest.copy()

# Erase resting right arm
for y in range(54, 78):
    for x in range(74, 104):
        f1[y, x] = C_CLEAR

# Natural dog right flank behind raised arm
flank_profile = [
    (56, 75), (57, 75), (58, 76), (59, 76), (60, 76),
    (61, 77), (62, 77), (63, 77), (64, 78), (65, 78),
    (66, 78), (67, 79), (68, 79), (69, 79), (70, 79),
    (71, 79), (72, 79), (73, 79), (74, 79)
]
for y, fx in flank_profile:
    for x in range(68, fx):
        f1[y, x] = C_OR_MID if x < fx - 2 else C_OR_DK
    f1[y, fx] = C_OUT
f1[74, 78] = C_OUT
f1[74, 79] = C_OUT

# RAISED RIGHT ARM (~60 DEGREES UP):
# Forearm extends upwards-right from shoulder (x=73, y=52).
# Paw is arched at the top, pointing down-left ready to strike.
# Completely outlined in C_OUT (#370802).
arm_r_up_grid = [
    # y=35
    "             #######     ",
    # y=36
    "           ##.......##   ",
    # y=37
    "         ##...........#  ",
    # y=38
    "       ##LL...........#  ",
    # y=39
    "      #LLOO............# ",
    # y=40
    "     #LLOOOO...C...C...# ",
    # y=41
    "    #LLOOOO...sC..sC.s.# ",
    # y=42
    "   #LLOOOO..s..C...C.s.# ",
    # y=43
    "  #LLOOOO..s...C...C.S.# ",
    # y=44
    " #LLOOOO..s....######### ",
    # y=45
    "#LLOOOO..S.....#         ",
    # y=46
    "#OOOOOO..S...##          ",
    # y=47
    "#OOOOODD...##            ",
    # y=48
    " #ODDDDD###              ",
    # y=49
    "  ######                 ",
]
apply_grid(f1, 35, 75, arm_r_up_grid)

protect_scarf(f1)

# =============================================================================
# 3. ACTION 2: TAP RIGHT (Right paw DOWN, Left paw RAISED ~60 deg)
# Left paw raises to x in [42..58], y in [56..67]
# Stays completely below and to the left of the scarf bandana!
# =============================================================================
f2 = f_rest.copy()

# Erase resting left paw (x in [46, 68], y in [62, 75])
for y in range(62, 78):
    for x in range(46, 68):
        f2[y, x] = C_CLEAR

# Natural white chest fur contour revealed behind raised left paw
for y in range(62, 75):
    # Rounded belly contour
    left_x = 44
    right_x = 68 if y < 70 else 66
    for x in range(left_x, right_x + 1):
        if y == 74:
            f2[y, x] = C_OUT
        elif y in (72, 73):
            f2[y, x] = C_WH_SH2 if x in (left_x, right_x) else C_WH_SH1
        elif x in (left_x, right_x):
            f2[y, x] = C_WH_SH1
        else:
            f2[y, x] = C_WH_HI

# Left flank boundary behind chest
for y in range(62, 75):
    f2[y, 44] = C_OR_DK
    f2[y, 43] = C_OR_MID

# RAISED LEFT PAW (~60 DEGREES UP):
# Paw is lifted in the air at y in [55..67], x in [40..58]
# It is lifted off the desk (desk is at y=74 -> 7 to 19 px above desk)
# It is below the scarf (scarf at x=40..52 is y <= 53)
# It is to the left of the triangular bandana point (x=59..64)
# Full continuous dark outline #370802 so it NEVER loses edge!
paw_l_up_grid = [
    # y=54
    "       #####       ",
    # y=55
    "     ##.....##     ",
    # y=56
    "    #.........#    ",
    # y=57
    "   #...........#   ",
    # y=58
    "  #....s...s....#  ",
    # y=59
    " #....s.s.s.s....# ",
    # y=60
    "#.....s.s.s.s.....#",
    # y=61
    "#....s..C...C..s..#",
    # y=62
    "#....s..C...C..s..#",
    # y=63
    " #...s..C...C..s.# ",
    # y=64
    "  #..S..C...C..S#  ",
    # y=65
    "   #############   ",
]
apply_grid(f2, 54, 40, paw_l_up_grid)

# Orange shoulder sleeve connecting knot to paw:
sleeve_l_grid = [
    # y=52
    "  ##  ",
    # y=53
    " #LL# ",
    # y=54
    "#LLOO#",
    # y=55
    "#OOOO#",
    # y=56
    " #DD# ",
    # y=57
    "  ##  ",
]
apply_grid(f2, 52, 36, sleeve_l_grid)

# MUST PROTECT SCARF: Red scarf is 100% visible on top!
protect_scarf(f2)

# =============================================================================
# Save 1X and 3X Images
# =============================================================================
im1 = Image.fromarray(f1)
im2 = Image.fromarray(f2)
im3 = Image.fromarray(f_rest)

# Save 1X
im1.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_1x.png')
im2.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_1x.png')
im3.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_1x.png')

# Save 3X
im1_3x = im1.resize((104*3, 78*3), Image.NEAREST)
im2_3x = im2.resize((104*3, 78*3), Image.NEAREST)
im3_3x = im3.resize((104*3, 78*3), Image.NEAREST)

im1_3x.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_3x.png')
im2_3x.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_3x.png')
im3_3x.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_3x.png')

# Copy to brain
import shutil
for f in ['action_1_tap_left_right_up_60deg_3x.png', 'action_2_tap_right_left_up_60deg_3x.png', 'action_3_rest_both_down_3x.png']:
    shutil.copy(f'docs_for_me/bongo_dog_preview/{f}', f'/home/hungdk4/.gemini/antigravity-cli/brain/3aced826-248d-4994-94da-3362ffe3e8e7/{f}')

print("All preview PNGs generated and copied to brain successfully.")
