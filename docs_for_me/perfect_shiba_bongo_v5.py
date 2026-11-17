import numpy as np
from PIL import Image

# -----------------------------------------------------------------------------
# 1. Exact Palette
# -----------------------------------------------------------------------------
C_OUT    = np.array([55, 8, 2, 255], dtype=np.uint8)       # #370802 dark outline
C_OR_HI  = np.array([255, 185, 95, 255], dtype=np.uint8)   # fur highlight
C_OR_MID = np.array([254, 160, 50, 255], dtype=np.uint8)   # fur base orange
C_OR_DK  = np.array([215, 105, 20, 255], dtype=np.uint8)   # fur deep orange shadow
C_WH_HI  = np.array([255, 255, 255, 255], dtype=np.uint8)  # pure white fur
C_WH_SH1 = np.array([244, 238, 230, 255], dtype=np.uint8)  # soft warm white shadow
C_WH_SH2 = np.array([225, 215, 202, 255], dtype=np.uint8)  # deeper white shadow
C_CLEAR  = np.array([0, 0, 0, 0], dtype=np.uint8)

# -----------------------------------------------------------------------------
# 2. Base Canvas (104x78)
# -----------------------------------------------------------------------------
raw = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
canvas = Image.new('RGBA', (104, 78), (0,0,0,0))
canvas.paste(raw, (1, 1), raw)
b = np.array(canvas)

# Clean below desk line y=74
for y in range(75, 78):
    for x in range(104):
        b[y, x] = C_CLEAR

# Clean desk line at y=74
for x in range(104):
    if x < 45 or (x > 67 and x < 76) or x > 96:
        b[74, x] = C_CLEAR

# Clear right motion arcs
for y in range(48, 68):
    for x in range(94, 104):
        b[y, x] = C_CLEAR

# -----------------------------------------------------------------------------
# 3. Extract & Protect Scarf 100% (NEVER COVER THE SCARF!)
# -----------------------------------------------------------------------------
scarf_pixels = {}
for y in range(8, 65):
    for x in range(18, 78):
        r, g, bc, a = b[y, x]
        if a > 100:
            if r > 130 and g < 65 and bc < 65:
                scarf_pixels[(y, x)] = b[y, x].copy()
                for dy, dx in [(-1,0), (1,0), (0,-1), (0,1)]:
                    ny, nx = y + dy, x + dx
                    if 0 <= ny < 78 and 0 <= nx < 104:
                        nr, ng, nb, na = b[ny, nx]
                        if na > 100 and nr < 70 and ng < 45 and nb < 40:
                            scarf_pixels[(ny, nx)] = b[ny, nx].copy()

def ensure_scarf(target):
    for (y, x), val in scarf_pixels.items():
        target[y, x] = val

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
# FRAME 3: REST (Both paws down on desk)
# =============================================================================
f_rest = b.copy()

# Sleeve descending to right paw
sleeve_r_rest = [
    (58, 74, 76), (59, 74, 78), (60, 75, 80),
    (61, 75, 82), (62, 75, 83), (63, 76, 84),
]
for y, x1, x2 in sleeve_r_rest:
    for x in range(x1, x2 + 1):
        f_rest[y, x] = C_OR_MID if x < x2 else C_OR_DK

# Clean resting right paw: smooth rounded arch, subtle claw notches
paw_r_rest_grid = [
    # y=63
    "      ######      ",
    # y=64
    "    ##......##    ",
    # y=65
    "   #..........#   ",
    # y=66
    "  #............#  ",
    # y=67
    " #..............# ",
    # y=68
    " #..............# ",
    # y=69
    "#................#",
    # y=70
    "#................#",
    # y=71
    "#.s....s....s...s#",
    # y=72
    "#.s....C....C...s#",
    # y=73
    " #S....C....C...S#",
    # y=74
    "  ################",
]
apply_grid(f_rest, 63, 77, paw_r_rest_grid)

# Clear right of paw
for y in range(60, 78):
    for x in range(95, 104):
        f_rest[y, x] = C_CLEAR

# Left paw desk contact
for x in range(46, 68):
    if f_rest[74, x, 3] > 0:
        f_rest[74, x] = C_OUT

ensure_scarf(f_rest)

# =============================================================================
# FRAME 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# =============================================================================
f1 = f_rest.copy()

# Clear resting right arm
for y in range(54, 78):
    for x in range(74, 104):
        f1[y, x] = C_CLEAR

# Smooth natural right flank
flank_pts = [
    (56, 75), (57, 75), (58, 76), (59, 76), (60, 76),
    (61, 77), (62, 77), (63, 77), (64, 78), (65, 78),
    (66, 78), (67, 78), (68, 79), (69, 79), (70, 79),
    (71, 79), (72, 79), (73, 79), (74, 79)
]
for y, fx in flank_pts:
    for x in range(68, fx):
        f1[y, x] = C_OR_MID if x < fx - 2 else C_OR_DK
    f1[y, fx] = C_OUT
f1[74, 78] = C_OUT
f1[74, 79] = C_OUT

# Raised right arm: beautifully rounded arch paw, 100% solid outline, zero scarf overlap
arm_r_up_grid = [
    # y=35
    "                ####     ",
    # y=36
    "             ###....##   ",
    # y=37
    "           ##.........#  ",
    # y=38
    "         ##LL..........# ",
    # y=39
    "       ##LLOO..........# ",
    # y=40
    "      #LLOOOO...........#",
    # y=41
    "     #LLOOOOO...........#",
    # y=42
    "    #LLOOOOOO..s...s....#",
    # y=43
    "   #LLOOOOOO..sC..sC..s.#",
    # y=44
    "  #LLOOOOOO..s.C...C..S.#",
    # y=45
    " #LLOOOOOO..S..##########",
    # y=46
    "#LLOOOOOO..S...#         ",
    # y=47
    "#OOOOOOOD..S.##          ",
    # y=48
    "#OOOOOODD..##            ",
    # y=49
    " #ODDDDDD##              ",
    # y=50
    "  #######                ",
]
apply_grid(f1, 35, 74, arm_r_up_grid)

# Orange shoulder connecting to arm
for y in range(50, 56):
    for x in range(72, 77):
        if f1[y, x, 3] == 0:
            f1[y, x] = C_OR_MID

ensure_scarf(f1)

# =============================================================================
# FRAME 2: TAP RIGHT (Right paw DOWN, Left paw RAISED ~60 deg)
# Red scarf 100% visible, natural curved chest (no straight vertical lines!)
# =============================================================================
f2 = f_rest.copy()

# Clear resting left paw (x in [46, 68], y in [63, 78])
for y in range(63, 78):
    for x in range(46, 68):
        f2[y, x] = C_CLEAR

# Natural chest contour revealed behind raised left paw
# Fluffy, rounded chest with soft shading
chest_profile = [
    # y: (start_x, end_x)
    (63, 44, 68), (64, 44, 68), (65, 43, 68), (66, 43, 68),
    (67, 43, 68), (68, 43, 67), (69, 43, 67), (70, 44, 67),
    (71, 44, 67), (72, 44, 66), (73, 44, 66), (74, 44, 66)
]
for y, sx, ex in chest_profile:
    for x in range(sx, ex + 1):
        if y == 74:
            f2[y, x] = C_OUT
        elif x == sx:
            f2[y, x] = C_OR_DK
        elif x == sx + 1:
            f2[y, x] = C_WH_SH2
        elif x == ex:
            f2[y, x] = C_WH_SH2 if y > 71 else C_WH_SH1
        elif y in (72, 73):
            f2[y, x] = C_WH_SH1
        else:
            f2[y, x] = C_WH_HI

# Left flank boundary (seamless orange body)
for y in range(63, 75):
    f2[y, 42] = C_OR_MID
    f2[y, 41] = C_OR_MID

# Raised left paw: raised ~60 deg, high in the air at y in [51..63]!
# Bottom is at y=63, desk is at y=74 (11px of clear air above desk!)
# Positioned at x in [41..57], staying below scarf knot & left of bandana tip!
# Continuous solid dark outline #370802 so it NEVER loses sharpness!
paw_l_up_grid = [
    # y=50
    "       #####       ",
    # y=51
    "     ##.....##     ",
    # y=52
    "    #.........#    ",
    # y=53
    "   #...........#   ",
    # y=54
    "  #.............#  ",
    # y=55
    " #...............# ",
    # y=56
    "#.................#",
    # y=57
    "#......s...s......#",
    # y=58
    "#....s.C.s.C.s....#",
    # y=59
    " #...s.C.s.C.s...# ",
    # y=60
    "  #..S.C.S.C.S..#  ",
    # y=61
    "   #############   ",
]
apply_grid(f2, 50, 41, paw_l_up_grid)

# Orange sleeve under scarf knot connecting to wrist:
sleeve_l_grid = [
    (51, 38, 43, C_OR_MID),
    (52, 38, 43, C_OR_MID),
    (53, 39, 44, C_OR_MID),
    (54, 40, 44, C_OR_DK),
]
for y, x1, x2, col in sleeve_l_grid:
    for x in range(x1, x2 + 1):
        f2[y, x] = col
f2[51, 37] = C_OUT; f2[52, 37] = C_OUT; f2[53, 38] = C_OUT

# SCARF MUST BE 100% VISIBLE: Restored on top!
ensure_scarf(f2)

# =============================================================================
# EXPORT
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

# Animated GIF (3s tapping + 2s resting)
tap_frames_3x = []
durations_3x = []

for i in range(12):
    tap_frames_3x.append(im1_3x)
    durations_3x.append(130)
    tap_frames_3x.append(im2_3x)
    durations_3x.append(130)

# Rest 2 seconds
tap_frames_3x.append(im3_3x)
durations_3x.append(2000)

tap_frames_3x[0].save(
    'docs_for_me/bongo_dog_preview/bongo_60deg_animation_3x.gif',
    save_all=True,
    append_images=tap_frames_3x[1:],
    duration=durations_3x,
    loop=0,
    disposal=2
)

# Copy to brain
import shutil
brain_dir = '/home/hungdk4/.gemini/antigravity-cli/brain/3aced826-248d-4994-94da-3362ffe3e8e7'
for fname in ['action_1_tap_left_right_up_60deg_3x.png', 'action_2_tap_right_left_up_60deg_3x.png', 'action_3_rest_both_down_3x.png', 'bongo_60deg_animation_3x.gif']:
    shutil.copy(f'docs_for_me/bongo_dog_preview/{fname}', f'{brain_dir}/{fname}')

print("V5 generated and copied successfully!")
