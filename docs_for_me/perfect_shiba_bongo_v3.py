import numpy as np
from PIL import Image

# -----------------------------------------------------------------------------
# 1. Palette
# -----------------------------------------------------------------------------
C_OUT    = np.array([55, 8, 2, 255], dtype=np.uint8)       # #370802 solid dark outline
C_OR_HI  = np.array([255, 185, 95, 255], dtype=np.uint8)   # fur highlight
C_OR_MID = np.array([254, 160, 50, 255], dtype=np.uint8)   # fur base orange
C_OR_DK  = np.array([215, 105, 20, 255], dtype=np.uint8)   # fur shadow orange
C_WH_HI  = np.array([255, 255, 255, 255], dtype=np.uint8)  # pure white
C_WH_SH1 = np.array([244, 238, 230, 255], dtype=np.uint8)  # soft warm white shadow
C_WH_SH2 = np.array([225, 215, 202, 255], dtype=np.uint8)  # deeper white shadow
C_RED    = np.array([205, 38, 42, 255], dtype=np.uint8)    # red scarf
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
# 3. Extract & Protect Scarf 100%
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

# Helper to apply grid
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

# Craft right resting paw: clean, organic, matching left paw
# Orange sleeve descending
sleeve_r_rest = [
    # y=58
    (58, 74, 76),
    (59, 74, 78),
    (60, 75, 80),
    (61, 75, 81),
    (62, 75, 82),
    (63, 76, 83),
]
for y, x1, x2 in sleeve_r_rest:
    for x in range(x1, x2 + 1):
        f_rest[y, x] = C_OR_MID if x < x2 else C_OR_DK

# White resting right paw: y=63..74, x=78..95
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
    "#...s..C...C..s..#",
    # y=71
    "#..ss..C...C..ss.#",
    # y=72
    "#..ss..C...C..ss.#",
    # y=73
    " #.SS..C...C..SS# ",
    # y=74
    "  ################",
]
apply_grid(f_rest, 63, 77, paw_r_rest_grid)

# Clear right of paw
for y in range(60, 78):
    for x in range(94, 104):
        if y > 74 or x > 94:
            f_rest[y, x] = C_CLEAR

# Left paw desk contact
for x in range(46, 68):
    if f_rest[74, x, 3] > 0:
        f_rest[74, x] = C_OUT

ensure_scarf(f_rest)

# =============================================================================
# FRAME 1: TAP LEFT (Left paw DOWN on desk, Right arm RAISED ~60 deg)
# =============================================================================
f1 = f_rest.copy()

# Clear resting right arm
for y in range(54, 78):
    for x in range(74, 104):
        f1[y, x] = C_CLEAR

# Right flank behind raised arm
flank_pts = [
    (56, 75), (57, 75), (58, 76), (59, 76), (60, 76),
    (61, 77), (62, 77), (63, 77), (64, 78), (65, 78),
    (66, 78), (67, 79), (68, 79), (69, 79), (70, 79),
    (71, 79), (72, 79), (73, 79), (74, 79)
]
for y, fx in flank_pts:
    for x in range(68, fx):
        f1[y, x] = C_OR_MID if x < fx - 2 else C_OR_DK
    f1[y, fx] = C_OUT
f1[74, 78] = C_OUT
f1[74, 79] = C_OUT

# Raised right arm: ~60 degrees raised, arched paw pointing down, fully outlined
arm_r_up_grid = [
    # y=36
    "             #######     ",
    # y=37
    "           ##.......##   ",
    # y=38
    "         ##...........#  ",
    # y=39
    "       ##LL...........#  ",
    # y=40
    "      #LLOO............# ",
    # y=41
    "     #LLOOOO...C...C...# ",
    # y=42
    "    #LLOOOO...sC..sC.s.# ",
    # y=43
    "   #LLOOOO..s..C...C.s.# ",
    # y=44
    "  #LLOOOO..s...C...C.S.# ",
    # y=45
    " #LLOOOO..s....######### ",
    # y=46
    "#LLOOOO..S.....#         ",
    # y=47
    "#OOOOOO..S...##          ",
    # y=48
    "#OOOOODD...##            ",
    # y=49
    " #ODDDDD###              ",
    # y=50
    "  ######                 ",
]
apply_grid(f1, 36, 75, arm_r_up_grid)

# Orange shoulder connecting to arm
for y in range(50, 56):
    for x in range(73, 78):
        if f1[y, x, 3] == 0:
            f1[y, x] = C_OR_MID

ensure_scarf(f1)

# =============================================================================
# FRAME 2: TAP RIGHT (Right paw DOWN on desk, Left paw RAISED ~60 deg)
# Red scarf 100% visible, left paw does NOT cover scarf!
# =============================================================================
f2 = f_rest.copy()

# Clear resting left paw (x in [46, 68], y in [63, 78])
for y in range(63, 78):
    for x in range(46, 68):
        f2[y, x] = C_CLEAR

# Natural chest contour revealed behind raised left paw
for y in range(63, 75):
    # Rounded natural belly
    for x in range(45, 68):
        if y == 74:
            f2[y, x] = C_OUT
        elif y in (72, 73):
            f2[y, x] = C_WH_SH2 if x in (45, 67) else C_WH_SH1
        elif x in (45, 67):
            f2[y, x] = C_WH_SH1
        else:
            f2[y, x] = C_WH_HI

# Left flank boundary
for y in range(63, 75):
    f2[y, 44] = C_OR_DK
    f2[y, 43] = C_OR_MID

# Raised left paw: placed at y in [56..67], x in [42..58]
# Raised 7 to 18px above desk, stays below scarf knot & left of bandana point!
# 100% continuous dark outline #370802 so it NEVER loses sharpness!
paw_l_up_grid = [
    # y=55
    "       #####       ",
    # y=56
    "     ##.....##     ",
    # y=57
    "    #.........#    ",
    # y=58
    "   #...........#   ",
    # y=59
    "  #....s...s....#  ",
    # y=60
    " #....s.s.s.s....# ",
    # y=61
    "#.....s.s.s.s.....#",
    # y=62
    "#....s..C...C..s..#",
    # y=63
    "#....s..C...C..s..#",
    # y=64
    " #...s..C...C..s.# ",
    # y=65
    "  #..S..C...C..S#  ",
    # y=66
    "   #############   ",
]
apply_grid(f2, 55, 41, paw_l_up_grid)

# Orange shoulder sleeve connecting knot to paw:
sleeve_l_grid = [
    # y=54
    "  ##  ",
    # y=55
    " #OO# ",
    # y=56
    "#OOOO#",
    # y=57
    "#OOOO#",
    # y=58
    " #DD# ",
    # y=59
    "  ##  ",
]
apply_grid(f2, 54, 38, sleeve_l_grid)

# SCARF MUST BE 100% VISIBLE: Restored on top of everything!
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

# Create Animated GIF (3s tapping + 2s resting)
# 130ms per frame: 3000ms / 130ms ≈ 23 frames tap alternating
# Resting 2000ms: frame 3 held for 2000ms
tap_frames_1x = []
durations_1x = []
tap_frames_3x = []
durations_3x = []

for i in range(12):
    tap_frames_1x.append(im1)
    durations_1x.append(130)
    tap_frames_3x.append(im1_3x)
    durations_3x.append(130)

    tap_frames_1x.append(im2)
    durations_1x.append(130)
    tap_frames_3x.append(im2_3x)
    durations_3x.append(130)

# Rest 2 seconds
tap_frames_1x.append(im3)
durations_1x.append(2000)
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

print("DONE! All files generated and copied.")
