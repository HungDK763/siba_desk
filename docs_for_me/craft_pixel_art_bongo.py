import numpy as np
from PIL import Image

# Palette from bongo_cleaned_102x76.png
C_OUTLINE  = np.array([55, 18, 12, 255], dtype=np.uint8)     # ## Dark outline
C_OUT_SOFT = np.array([105, 45, 25, 255], dtype=np.uint8)    # Anti-alias outline
C_ORANGE_L = np.array([255, 192, 100, 255], dtype=np.uint8)  # Highlight orange
C_ORANGE   = np.array([254, 160, 50, 255], dtype=np.uint8)   # oo Vibrant fur
C_ORANGE_M = np.array([238, 136, 32, 255], dtype=np.uint8)   # Mid orange
C_ORANGE_D = np.array([205, 100, 20, 255], dtype=np.uint8)   # Deep shadow fur
C_WHITE    = np.array([255, 254, 250, 255], dtype=np.uint8)  # .. Clean paw white
C_WHITE_S1 = np.array([242, 232, 220, 255], dtype=np.uint8)  # Soft shadow white
C_WHITE_S2 = np.array([225, 210, 195, 255], dtype=np.uint8)  # :: Paw shadow
C_CLAW     = np.array([48, 12, 5, 255], dtype=np.uint8)      # Claw slit
C_CLEAR    = np.array([0, 0, 0, 0], dtype=np.uint8)

# 1. Load base 104x78
base102 = Image.open('docs_for_me/bongo_cleaned_102x76.png').convert('RGBA')
base = Image.new('RGBA', (104, 78), (0,0,0,0))
base.paste(base102, (1, 1), base102)
barr = np.array(base)

# Clean desk line: desk is y=74
for y in range(74, 78):
    for x in range(104):
        if y > 74:
            barr[y, x] = C_CLEAR
        elif y == 74:
            r, g, b, a = barr[y, x]
            # Clean non-paw table pixels
            if a > 0 and (x < 46 or (x > 67 and x < 76) or x > 96):
                barr[y, x] = C_CLEAR

# =========================================================================
# FRAME 3: REST (Both Paws DOWN)
# =========================================================================
f_rest = barr.copy()

# =========================================================================
# FRAME 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# =========================================================================
f_tap_l = barr.copy()

# Step 1.1: Erase old down-arm (x >= 73, y >= 50)
for y in range(50, 75):
    for x in range(73, 104):
        r, g, b, a = f_tap_l[y, x]
        # Preserve red scarf (x <= 75, y <= 58, r > 150)
        if r > 150 and g < 70 and b < 70 and x <= 75 and y <= 58:
            continue
        f_tap_l[y, x] = C_CLEAR

# Step 1.2: Fill natural right flank (smooth curve connecting neck to desk)
# Flank curve:
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
            f_tap_l[y, x] = C_ORANGE
        elif x < fx - 1:
            f_tap_l[y, x] = C_ORANGE_M
        else:
            f_tap_l[y, x] = C_ORANGE_D
    f_tap_l[y, fx] = C_OUTLINE

# Step 1.3: Paint the RAISED RIGHT ARM (~60 deg upwards, arched paw)
# Shoulder is at (73, 50)
# Arm trajectory:
# y=48..49: x in [73..77]
# y=45..47: x in [76..82]
# y=42..44: x in [80..88]
# y=38..41: x in [84..93]
# y=35..37: x in [87..97] -> peak of arm!
# Paw arches down:
# y=38..44: x in [87..99]
# y=45..48: x in [88..98] -> paw bottom ready to strike!

# Forearm & Paw Pixel Matrix Map:
# '#' = outline, 'L' = highlight, 'O' = orange, 'D' = shadow orange,
# '.' = white, 'S' = white shadow, 'C' = claw, ' ' = transparent
arm_r_map = {
    34: "         ####   ",
    35: "       ##LLLL## ",
    36: "     ##LLOOOO..#",
    37: "    #LLOOOO....#",
    38: "   #LLOOOO......#",
    39: "  #LLOOOO...S...#",
    40: " #LLOOOO...S.S..#",
    41: "#LLOOOO...S...S.#",
    42: "#OOOOOO...S...S.#",
    43: " #OOOOOD..SS.SS.#",
    44: "  #OOOOOD..S.S..#",
    45: "   #OOOOOD.S.S.#",
    46: "    #DDDD##C#C##",
    47: "     #### ######",
    48: "      ##        ",
    49: "       #        ",
}

# Paste arm_r_map at offset x=71, y=34
for dy, row in arm_r_map.items():
    y = dy
    for dx, ch in enumerate(row):
        x = 71 + dx
        if 0 <= y < 78 and 0 <= x < 104:
            if ch == '#': f_tap_l[y, x] = C_OUTLINE
            elif ch == 'L': f_tap_l[y, x] = C_ORANGE_L
            elif ch == 'O': f_tap_l[y, x] = C_ORANGE
            elif ch == 'D': f_tap_l[y, x] = C_ORANGE_D
            elif ch == '.': f_tap_l[y, x] = C_WHITE
            elif ch == 'S': f_tap_l[y, x] = C_WHITE_S2
            elif ch == 'C': f_tap_l[y, x] = C_CLAW

# Re-apply red scarf knot at shoulder
for y in range(48, 56):
    for x in range(68, 76):
        mr, mg, mb, ma = barr[y, x]
        if mr > 150 and mg < 70 and mb < 70:
            f_tap_l[y, x] = barr[y, x]

# =========================================================================
# FRAME 2: TAP RIGHT (Right arm DOWN, Left paw RAISED ~60 deg)
# =========================================================================
f_tap_r = barr.copy()

# Step 2.1: Erase old down-paw on left (x in [45, 68], y in [62, 75])
for y in range(62, 75):
    for x in range(45, 68):
        f_tap_r[y, x] = C_CLEAR

# Step 2.2: Fill dog white chest down to the desk line (y=74)
for y in range(62, 75):
    for x in range(46, 68):
        if y < 72:
            f_tap_r[y, x] = C_WHITE if y < 69 else C_WHITE_S1
        elif y in (72, 73):
            f_tap_r[y, x] = C_WHITE_S2
        elif y == 74:
            f_tap_r[y, x] = C_OUTLINE

# Step 2.3: Paint the RAISED LEFT ARM / PAW (~60 deg raised)
# In canonical Bongo Cat: left paw is raised in an inverted U arch next to the scarf!
# Paw is located at x in [42, 58], y in [44, 57]
# Lifted ~20px off desk!
paw_l_map = {
    44: "    ####    ",
    45: "  ##....##  ",
    46: " #........# ",
    47: "#..........#",
    48: "#....S.....#",
    49: "#...S.S....#",
    50: "#..S...S...#",
    51: "#..S...S...#",
    52: "#.SS...SS..#",
    53: " #.S...S..# ",
    54: "  ##C#C#C#  ",
    55: "   ######   ",
}

# Also sleeve connection under scarf (x in [42, 48], y in [52, 58])
sleeve_l_map = {
    54: "  #OO#      ",
    55: " #OOOO#     ",
    56: "#OOOOOO#    ",
    57: " #ODD#      ",
    58: "  ###       ",
}

for dy, row in paw_l_map.items():
    y = dy
    for dx, ch in enumerate(row):
        x = 44 + dx
        if 0 <= y < 78 and 0 <= x < 104:
            if ch == '#': f_tap_r[y, x] = C_OUTLINE
            elif ch == '.': f_tap_r[y, x] = C_WHITE
            elif ch == 'S': f_tap_r[y, x] = C_WHITE_S2
            elif ch == 'C': f_tap_r[y, x] = C_CLAW

for dy, row in sleeve_l_map.items():
    y = dy
    for dx, ch in enumerate(row):
        x = 40 + dx
        if 0 <= y < 78 and 0 <= x < 104:
            if ch == '#': f_tap_r[y, x] = C_OUTLINE
            elif ch == 'O': f_tap_r[y, x] = C_ORANGE
            elif ch == 'D': f_tap_r[y, x] = C_ORANGE_D

# Save all frames: 1X (104x78) and 3X (312x234)
img_f1 = Image.fromarray(f_tap_l)
img_f2 = Image.fromarray(f_tap_r)
img_f3 = Image.fromarray(f_rest)

img_f1.save('docs_for_me/bongo_action1_tap_l_1x.png')
img_f2.save('docs_for_me/bongo_action2_tap_r_1x.png')
img_f3.save('docs_for_me/bongo_action3_rest_1x.png')

img_f1.resize((104*3, 78*3), Image.NEAREST).save('docs_for_me/bongo_action1_tap_l_3x.png')
img_f2.resize((104*3, 78*3), Image.NEAREST).save('docs_for_me/bongo_action2_tap_r_3x.png')
img_f3.resize((104*3, 78*3), Image.NEAREST).save('docs_for_me/bongo_action3_rest_3x.png')

print('Crafted authentic pixel art bongo frames saved!')
