import numpy as np
from PIL import Image

# -------------------------------------------------------------------------
# Exact Palette
# -------------------------------------------------------------------------
C_OUTLINE  = np.array([48, 16, 10, 255], dtype=np.uint8)     # #30100a Solid dark outline
C_OUT_DARK = np.array([38, 12, 8, 255], dtype=np.uint8)      # Deepest outline
C_ORANGE_L = np.array([255, 195, 105, 255], dtype=np.uint8)  # Highlight fur
C_ORANGE   = np.array([254, 158, 48, 255], dtype=np.uint8)   # Vibrant orange fur
C_ORANGE_M = np.array([238, 134, 30, 255], dtype=np.uint8)   # Mid orange
C_ORANGE_D = np.array([205, 100, 20, 255], dtype=np.uint8)   # Deep shadow fur
C_WHITE    = np.array([255, 255, 252, 255], dtype=np.uint8)  # Pure paw white
C_WHITE_S1 = np.array([242, 234, 222, 255], dtype=np.uint8)  # Soft white shadow
C_WHITE_S2 = np.array([225, 212, 198, 255], dtype=np.uint8)  # Deep white shadow
C_CLAW     = np.array([45, 10, 5, 255], dtype=np.uint8)      # Dark claw slit
C_CLEAR    = np.array([0, 0, 0, 0], dtype=np.uint8)

# -------------------------------------------------------------------------
# 1. Base Dog (Action 3: REST)
# -------------------------------------------------------------------------
base_raw = Image.open('docs_for_me/bongo_cleaned_102x76.png').convert('RGBA')
base = Image.new('RGBA', (104, 78), (0,0,0,0))
base.paste(base_raw, (1, 1), base_raw)
barr = np.array(base)

# Clean desk line: desk is y=74
for y in range(74, 78):
    for x in range(104):
        if y > 74:
            barr[y, x] = C_CLEAR
        elif y == 74:
            r, g, b, a = barr[y, x]
            if a > 0 and (x < 46 or (x > 67 and x < 76) or x > 96):
                barr[y, x] = C_CLEAR

# Crisp outline on resting paws desk contact
for x in range(46, 68):
    if barr[74, x, 3] > 0:
        barr[74, x] = C_OUTLINE
for x in range(76, 97):
    if barr[74, x, 3] > 0:
        barr[74, x] = C_OUTLINE

f_rest = barr.copy()

# =========================================================================
# 2. ACTION 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# =========================================================================
f1 = barr.copy()

# Step 2.1: Erase old down-arm (x >= 73, y >= 50)
for y in range(50, 75):
    for x in range(73, 104):
        r, g, b, a = f1[y, x]
        # Preserve red scarf (x <= 75, y <= 58, r > 150)
        if r > 150 and g < 70 and b < 70 and x <= 75 and y <= 58:
            continue
        f1[y, x] = C_CLEAR

# Step 2.2: Natural dog flank behind arm with crisp dark outline
flank_profile = [
    (52, 74), (53, 74), (54, 75), (55, 75), (56, 75),
    (57, 76), (58, 76), (59, 76), (60, 77), (61, 77),
    (62, 77), (63, 78), (64, 78), (65, 78), (66, 78),
    (67, 79), (68, 79), (69, 79), (70, 79), (71, 79),
    (72, 79), (73, 79), (74, 79)
]
for y, fx in flank_profile:
    for x in range(68, fx):
        if x < fx - 3:
            f1[y, x] = C_ORANGE
        elif x < fx - 1:
            f1[y, x] = C_ORANGE_M
        else:
            f1[y, x] = C_ORANGE_D
    f1[y, fx] = C_OUTLINE

# Step 2.3: CRAFT CRISP RAISED RIGHT ARM (~60 deg raised)
# All pixels explicitly drawn with solid 1px dark outlines:
# Matrix overlay on 104x78 canvas
# '#' = OUTLINE, 'L' = HIGHLIGHT, 'O' = ORANGE, 'D' = SHADOW ORANGE,
# '.' = WHITE, 'S' = WHITE SHADOW, 'C' = CLAW, ' ' = transparent
arm_r_grid = [
    # y=34
    "         ######         ",
    # y=35
    "       ##LLLLLL##       ",
    # y=36
    "     ##LLOOOOOO..##     ",
    # y=37
    "    #LLOOOOOO......#    ",
    # y=38
    "   #LLOOOOOO........#   ",
    # y=39
    "  #LLOOOOOO....S.....#  ",
    # y=40
    " #LLOOOOOO....S.S.....# ",
    # y=41
    "#LLOOOOOO....S...S....# ",
    # y=42
    "#OOOOOOOO...S.....S...# ",
    # y=43
    "#OOOOOOOD..S.......S..# ",
    # y=44
    " #OOOOODD..S...S...S..# ",
    # y=45
    "  #OOODDD..S...S...S.#  ",
    # y=46
    "   #ODDDD#..S.S.S.S.##  ",
    # y=47
    "    #DDDD##.C.C.C.C##   ",
    # y=48
    "     ####  #########    ",
    # y=49
    "      ##                ",
]

start_y = 34
start_x = 72

for row_idx, row in enumerate(arm_r_grid):
    y = start_y + row_idx
    for col_idx, ch in enumerate(row):
        x = start_x + col_idx
        if 0 <= y < 78 and 0 <= x < 104:
            if ch == '#': f1[y, x] = C_OUTLINE
            elif ch == 'L': f1[y, x] = C_ORANGE_L
            elif ch == 'O': f1[y, x] = C_ORANGE
            elif ch == 'D': f1[y, x] = C_ORANGE_D
            elif ch == '.': f1[y, x] = C_WHITE
            elif ch == 'S': f1[y, x] = C_WHITE_S2
            elif ch == 'C': f1[y, x] = C_CLAW

# Re-apply red scarf over shoulder
for y in range(48, 56):
    for x in range(68, 76):
        mr, mg, mb, ma = barr[y, x]
        if mr > 150 and mg < 70 and mb < 70:
            f1[y, x] = barr[y, x]

# =========================================================================
# 3. ACTION 2: TAP RIGHT (Right arm DOWN, Left paw RAISED ~60 deg)
# =========================================================================
f2 = barr.copy()

# Step 3.1: Erase old resting left paw (x in [45, 68], y in [62, 75])
for y in range(62, 75):
    for x in range(45, 68):
        f2[y, x] = C_CLEAR

# Step 3.2: Complete white chest fur down to desk line (y=74)
for y in range(62, 75):
    for x in range(46, 68):
        if y < 72:
            f2[y, x] = C_WHITE if y < 69 else C_WHITE_S1
        elif y in (72, 73):
            f2[y, x] = C_WHITE_S2
        elif y == 74:
            f2[y, x] = C_OUTLINE

# Step 3.3: CRAFT CRISP RAISED LEFT PAW (~60 deg raised)
# Has a full continuous dark outline '#' around its entire perimeter!
# Claws 'C' pointing down ready to strike!
# Sleeve 'O' connecting to shoulder!
arm_l_grid = [
    # y=44
    "      ######      ",
    # y=45
    "    ##......##    ",
    # y=46
    "   #..........#   ",
    # y=47
    "  #............#  ",
    # y=48
    " #....S....S....# ",
    # y=49
    "#....S.S..S.S....#",
    # y=50
    "#...S...SS...S...#",
    # y=51
    "#..S....SS....S..#",
    # y=52
    "#..S....SS....S..#",
    # y=53
    "#..SS..SSSS..SS..#",
    # y=54
    " #..S..S..S..S..# ",
    # y=55
    "  #..C..C..C..#   ",
    # y=56
    "   ############   ",
]

sleeve_l_grid = [
    # y=53
    "   ##   ",
    # y=54
    "  #OO#  ",
    # y=55
    " #OOOO# ",
    # y=56
    "#OOOOOO#",
    # y=57
    " #ODD#  ",
    # y=58
    "  ##    ",
]

# Paste sleeve first (behind paw)
start_sleeve_y = 53
start_sleeve_x = 40
for row_idx, row in enumerate(sleeve_l_grid):
    y = start_sleeve_y + row_idx
    for col_idx, ch in enumerate(row):
        x = start_sleeve_x + col_idx
        if 0 <= y < 78 and 0 <= x < 104:
            if ch == '#': f2[y, x] = C_OUTLINE
            elif ch == 'O': f2[y, x] = C_ORANGE
            elif ch == 'D': f2[y, x] = C_ORANGE_D

# Paste crisp raised paw with continuous outline
start_paw_y = 44
start_paw_x = 45
for row_idx, row in enumerate(arm_l_grid):
    y = start_paw_y + row_idx
    for col_idx, ch in enumerate(row):
        x = start_paw_x + col_idx
        if 0 <= y < 78 and 0 <= x < 104:
            if ch == '#': f2[y, x] = C_OUTLINE
            elif ch == '.': f2[y, x] = C_WHITE
            elif ch == 'S': f2[y, x] = C_WHITE_S2
            elif ch == 'C': f2[y, x] = C_CLAW

# Re-apply red scarf knot
for y in range(50, 60):
    for x in range(35, 45):
        mr, mg, mb, ma = barr[y, x]
        if mr > 150 and mg < 70 and mb < 70:
            f2[y, x] = barr[y, x]

# -------------------------------------------------------------------------
# 4. Save and Export
# -------------------------------------------------------------------------
img1 = Image.fromarray(f1)
img2 = Image.fromarray(f2)
img3 = Image.fromarray(f_rest)

# Save 1X hardware sprites (104x78)
img1.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_1x.png')
img2.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_1x.png')
img3.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_1x.png')

# Save 3X crisp previews (312x234) - NEAREST NEIGHBOR ensures 100% sharp pixel blocks!
p1 = img1.resize((104*3, 78*3), Image.NEAREST)
p2 = img2.resize((104*3, 78*3), Image.NEAREST)
p3 = img3.resize((104*3, 78*3), Image.NEAREST)

p1.save('docs_for_me/bongo_dog_preview/action_1_tap_left_right_up_60deg_3x.png')
p2.save('docs_for_me/bongo_dog_preview/action_2_tap_right_left_up_60deg_3x.png')
p3.save('docs_for_me/bongo_dog_preview/action_3_rest_both_down_3x.png')

# Generate updated animated GIF (3s tapping @ 150ms + 2s rest @ 2000ms)
frames = []
durations = []
for _ in range(10):
    frames.append(p1)
    durations.append(150)
    frames.append(p2)
    durations.append(150)
frames.append(p3)
durations.append(2000)

frames[0].save(
    'docs_for_me/bongo_dog_preview/bongo_60deg_animation_3x.gif',
    save_all=True,
    append_images=frames[1:],
    duration=durations,
    loop=0,
    disposal=2
)

# Copy to brain artifact directory for display
import shutil
brain_dir = '/home/hungdk4/.gemini/antigravity-cli/brain/3aced826-248d-4994-94da-3362ffe3e8e7'
for fname in [
    'action_1_tap_left_right_up_60deg_3x.png',
    'action_2_tap_right_left_up_60deg_3x.png',
    'action_3_rest_both_down_3x.png',
    'bongo_60deg_animation_3x.gif'
]:
    shutil.copy(f'docs_for_me/bongo_dog_preview/{fname}', f'{brain_dir}/{fname}')

print('Crisp outline bongo frames generated and copied successfully!')
