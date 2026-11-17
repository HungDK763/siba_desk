import numpy as np
from PIL import Image

# -----------------------------------------------------------------------------
# Color Constants (Signature Bongo Dog Palette)
# -----------------------------------------------------------------------------
C_OUT     = np.array([55, 8, 2, 255], dtype=np.uint8)       # #370802 signature dark outline
C_OUT_ALT = np.array([45, 12, 6, 255], dtype=np.uint8)      # deep outline
C_OR_HI   = np.array([255, 195, 105, 255], dtype=np.uint8)  # fur highlight
C_OR_MID  = np.array([254, 160, 50, 255], dtype=np.uint8)   # fur base orange
C_OR_DK   = np.array([215, 105, 20, 255], dtype=np.uint8)   # fur deep shadow orange
C_WH_HI   = np.array([255, 255, 255, 255], dtype=np.uint8)  # pure white fur
C_WH_SH1  = np.array([244, 238, 230, 255], dtype=np.uint8)  # soft warm white shadow
C_WH_SH2  = np.array([225, 215, 202, 255], dtype=np.uint8)  # deep white shadow
C_RED     = np.array([205, 38, 42, 255], dtype=np.uint8)    # red scarf
C_RED_DK  = np.array([138, 24, 28, 255], dtype=np.uint8)   # red scarf shadow
C_CLEAR   = np.array([0, 0, 0, 0], dtype=np.uint8)

# Load base dog (102x76) and place on 104x78 canvas
raw_base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
canvas = Image.new('RGBA', (104, 78), (0,0,0,0))
canvas.paste(raw_base, (1, 1), raw_base)
base = np.array(canvas)

# Clean below desk line (y > 74)
for y in range(75, 78):
    for x in range(104):
        base[y, x] = C_CLEAR

# Clean desk line at y=74: only dog body touches desk
for x in range(104):
    if x < 40 or (x > 67 and x < 74) or x > 98:
        base[74, x] = C_CLEAR

# =============================================================================
# Helper to draw pixel matrices
# '#' = Outline, 'L' = Orange Highlight, 'O' = Orange Mid, 'D' = Orange Dark
# '.' = White Pure, 's' = White Shadow 1, 'S' = White Shadow 2, 'C' = Claw/Outline
# ' ' = No change
# =============================================================================
def apply_pixel_matrix(arr, start_y, start_x, grid):
    for r_idx, row in enumerate(grid):
        y = start_y + r_idx
        for c_idx, ch in enumerate(row):
            x = start_x + c_idx
            if 0 <= y < 78 and 0 <= x < 104:
                if ch == '#': arr[y, x] = C_OUT
                elif ch == 'L': arr[y, x] = C_OR_HI
                elif ch == 'O': arr[y, x] = C_OR_MID
                elif ch == 'D': arr[y, x] = C_OR_DK
                elif ch == '.': arr[y, x] = C_WH_HI
                elif ch == 's': arr[y, x] = C_WH_SH1
                elif ch == 'S': arr[y, x] = C_WH_SH2
                elif ch == 'C': arr[y, x] = C_OUT

# =============================================================================
# 1. High-Quality Resting Right Paw (down on desk)
# =============================================================================
paw_r_down_grid = [
    # y=60
    "       ###        ",
    # y=61
    "     ##OOO##      ",
    # y=62
    "   ##OOOOOOO##    ",
    # y=63
    "  #OOOOOOOOOOO#   ",
    # y=64
    " #OOOOOOOOOO...#  ",
    # y=65
    "#OOOOOOOO.......# ",
    # y=66
    "#OOOOOOO.........#",
    # y=67
    "#OOOOOO..........#",
    # y=68
    "#OOOOO...........#",
    # y=69
    "#OOOO............#",
    # y=70
    "#OOOO...C....C...#",
    # y=71
    "#ODDD.s.C.ss.C.s.#",
    # y=72
    "#DDDD.s.C.ss.C.s.#",
    # y=73
    " #DDD.S.C.SS.C.S.#",
    # y=74
    "  ################",
]

# =============================================================================
# 2. High-Quality Raised Right Arm (~60 degrees up)
# Pointing upwards and to the right, arched paw ready to strike
# =============================================================================
arm_r_up_60_grid = [
    # y=34
    "              ######     ",
    # y=35
    "            ##......##   ",
    # y=36
    "          ##..........#  ",
    # y=37
    "        ##LL...........# ",
    # y=38
    "       #LLOO...........# ",
    # y=39
    "      #LLOOOO...C...C...#",
    # y=40
    "     #LLOOOO...sC..sC.s.#",
    # y=41
    "    #LLOOOO....sC..sC.s.#",
    # y=42
    "   #LLOOOO..s..sC..sC.s.#",
    # y=43
    "  #LLOOOO..s...SC..SC.S.#",
    # y=44
    " #LLOOOO..s....##########",
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

# =============================================================================
# 3. High-Quality Raised Left Paw (~60 degrees up)
# Lifted in the air at y in [43..56], completely outlined in #370802
# =============================================================================
paw_l_up_60_grid = [
    # y=43
    "        ######        ",
    # y=44
    "      ##......##      ",
    # y=45
    "     #..........#     ",
    # y=46
    "    #............#    ",
    # y=47
    "   #..............#   ",
    # y=48
    "  #....s......s....#  ",
    # y=49
    " #....ss..s...ss....# ",
    # y=50
    "#.....ss..s...ss.....#",
    # y=51
    "#.....ss..s...ss.....#",
    # y=52
    "#....s.C.s.s..C.s....#",
    # y=53
    "#....s.C.s.s..C.s....#",
    # y=54
    " #...s.C.s.s..C.s...# ",
    # y=55
    "  #..S.C.S.S..C.S..#  ",
    # y=56
    "   ################   ",
]

sleeve_l_up_grid = [
    # y=48
    "  ##  ",
    # y=49
    " #LL# ",
    # y=50
    "#LLOO#",
    # y=51
    "#OOOO#",
    # y=52
    "#OODD#",
    # y=53
    " #DD# ",
    # y=54
    "  ##  ",
]

# =============================================================================
# BUILD FRAME 3: REST (Both Paws Down on Desk)
# =============================================================================
f_rest = base.copy()

# Clear old messy right paw area
for y in range(60, 78):
    for x in range(74, 104):
        f_rest[y, x] = C_CLEAR

# Apply clean resting right paw
apply_pixel_matrix(f_rest, 60, 75, paw_r_down_grid)

# Ensure left paw desk contact outline
for x in range(46, 68):
    if f_rest[74, x, 3] > 0:
        f_rest[74, x] = C_OUT

# =============================================================================
# BUILD FRAME 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# =============================================================================
f1 = f_rest.copy()

# Erase resting right arm
for y in range(54, 78):
    for x in range(74, 104):
        f1[y, x] = C_CLEAR

# Natural dog right flank behind raised arm
flank_pts = [
    (56, 75), (57, 75), (58, 76), (59, 76), (60, 76),
    (61, 77), (62, 77), (63, 77), (64, 78), (65, 78),
    (66, 78), (67, 79), (68, 79), (69, 79), (70, 79),
    (71, 79), (72, 79), (73, 79), (74, 79)
]
for y, fx in flank_pts:
    for x in range(68, fx):
        if x < fx - 2:
            f1[y, x] = C_OR_MID
        elif x < fx:
            f1[y, x] = C_OR_DK
    f1[y, fx] = C_OUT
f1[74, 78] = C_OUT
f1[74, 79] = C_OUT

# Apply raised right arm (angled ~60 degrees, crisp outline)
apply_pixel_matrix(f1, 34, 75, arm_r_up_60_grid)

# Re-apply red scarf edge over shoulder
for y in range(50, 58):
    for x in range(68, 76):
        r, g, b_c, a = base[y, x]
        if r > 150 and g < 60 and b_c < 60:
            f1[y, x] = base[y, x]

# =============================================================================
# BUILD FRAME 2: TAP RIGHT (Right paw DOWN, Left paw RAISED ~60 deg)
# =============================================================================
f2 = f_rest.copy()

# Erase resting left paw (x in [45, 68], y in [62, 75])
for y in range(62, 78):
    for x in range(45, 68):
        f2[y, x] = C_CLEAR

# Natural white chest fur contour behind raised left paw
for y in range(62, 75):
    # Rounded natural chest shape
    start_x = 44 if y < 70 else 45
    end_x   = 68 if y < 70 else 67
    for x in range(start_x, end_x + 1):
        if y == 74:
            f2[y, x] = C_OUT
        elif y in (72, 73):
            f2[y, x] = C_WH_SH2 if x in (start_x, end_x) else C_WH_SH1
        elif x in (start_x, end_x):
            f2[y, x] = C_WH_SH1
        else:
            f2[y, x] = C_WH_HI

# Left flank boundary
for y in range(62, 75):
    f2[y, 44] = C_OR_DK
    f2[y, 43] = C_OR_MID

# Apply left sleeve connecting to shoulder
apply_pixel_matrix(f2, 48, 40, sleeve_l_up_grid)

# Apply raised left paw (crisp outline #370802 around all 360 degrees!)
apply_pixel_matrix(f2, 43, 44, paw_l_up_60_grid)

# Re-apply red scarf knot over left shoulder
for y in range(48, 56):
    for x in range(35, 45):
        r, g, b_c, a = base[y, x]
        if r > 150 and g < 60 and b_c < 60:
            f2[y, x] = base[y, x]

# =============================================================================
# Save test images and 3X previews
# =============================================================================
im1 = Image.fromarray(f1)
im2 = Image.fromarray(f2)
im3 = Image.fromarray(f_rest)

im1_3x = im1.resize((104*3, 78*3), Image.NEAREST)
im2_3x = im2.resize((104*3, 78*3), Image.NEAREST)
im3_3x = im3.resize((104*3, 78*3), Image.NEAREST)

im1_3x.save('docs_for_me/preview_action1_test_crisp.png')
im2_3x.save('docs_for_me/preview_action2_test_crisp.png')
im3_3x.save('docs_for_me/preview_action3_test_crisp.png')

print("Saved preview_action1_test_crisp.png, preview_action2_test_crisp.png, preview_action3_test_crisp.png")
