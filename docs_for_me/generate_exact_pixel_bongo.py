import numpy as np
from PIL import Image, ImageDraw

# Colors
C_OUTLINE = (52, 18, 12, 255)       # #34120c Dark outline
C_ORANGE_L = (255, 188, 92, 255)    # Top highlight
C_ORANGE = (254, 160, 50, 255)      # Vibrant fur
C_ORANGE_M = (240, 135, 32, 255)    # Mid orange
C_ORANGE_D = (210, 100, 20, 255)    # Deep shadow fur
C_WHITE = (255, 254, 250, 255)     # Clean paw white
C_WHITE_SH = (235, 222, 210, 255)   # Under-paw shadow
C_CLAW = (48, 12, 5, 255)          # Claw slit
C_SCARF = (217, 61, 49, 255)       # Scarf red

# 1. Base dog from bongo_cleaned_102x76.png
raw_base = Image.open('docs_for_me/bongo_cleaned_102x76.png').convert('RGBA')
# Pad to 104x78
base104 = Image.new('RGBA', (104, 78), (0,0,0,0))
base104.paste(raw_base, (1, 1), raw_base)

# Clean table streaks at bottom of right paw (y >= 70, x >= 74)
barr = np.array(base104)
# Clean desk line: desk is y=74
for y in range(74, 78):
    for x in range(104):
        # preserve paw outlines at y=74
        if y > 74:
            barr[y, x] = [0,0,0,0]
        elif y == 74:
            # clean any white streak outside paws
            r, g, b, a = barr[y, x]
            if a > 0 and (r > 180 and g > 180 and b > 180) and x > 95:
                barr[y, x] = [0,0,0,0]

clean_rest = Image.fromarray(barr)

# -------------------------------------------------------------------------
# FRAME 3: REST POSE (Both paws DOWN on desk)
# -------------------------------------------------------------------------
frame_rest = clean_rest.copy()
# Save Rest frame
frame_rest.save('docs_for_me/bongo_rest_104x78.png')

# -------------------------------------------------------------------------
# FRAME 1: TAP LEFT (Left paw DOWN, Right arm RAISED ~60 deg)
# -------------------------------------------------------------------------
f1_arr = np.array(clean_rest)

# Erase old right down-arm:
# Arm is at x in [72, 103], y in [50, 75]
for y in range(50, 75):
    for x in range(72, 104):
        r, g, b, a = f1_arr[y, x]
        # Preserve red scarf (x <= 75, y <= 58, r > 150)
        if r > 150 and g < 70 and b < 70 and x <= 75 and y <= 58:
            continue
        f1_arr[y, x] = [0,0,0,0]

# Draw natural dog flank (body behind arm) from scarf down to desk
# y=52 to 74
flank_points = [
    (52, 74), (53, 74), (54, 75), (55, 75), (56, 75),
    (57, 76), (58, 76), (59, 76), (60, 77), (61, 77),
    (62, 77), (63, 78), (64, 78), (65, 78), (66, 78),
    (67, 79), (68, 79), (69, 79), (70, 79), (71, 79),
    (72, 79), (73, 79), (74, 79)
]
for y, fx in flank_points:
    # fill from x=68 to fx
    for x in range(68, fx):
        if x < fx - 2:
            f1_arr[y, x] = C_ORANGE
        else:
            f1_arr[y, x] = C_ORANGE_D
    f1_arr[y, fx] = C_OUTLINE

# Now draw the RAISED RIGHT ARM (~60 deg up):
# Shoulder connects at (73, 50) under scarf
# Forearm goes UP-RIGHT: from (73, 50) to (87, 36)
# Arm thickness is 5-6 pixels.
# Top edge: (73, 49) -> (77, 44) -> (82, 39) -> (87, 36) -> (91, 36)
# Bottom edge: (75, 54) -> (79, 49) -> (84, 44) -> (88, 41)
img_f1 = Image.fromarray(f1_arr)
d1 = ImageDraw.Draw(img_f1)

# Forearm outline polygon
forearm_poly = [
    (73, 48), (78, 42), (83, 37), (89, 34), (94, 34),
    (96, 38), (91, 43), (85, 47), (80, 52), (74, 54)
]
d1.polygon(forearm_poly, fill=C_OUTLINE)

# Forearm inner
forearm_in = [
    (74, 49), (78, 43), (83, 38), (89, 35), (93, 35),
    (95, 38), (90, 42), (84, 46), (79, 51), (75, 53)
]
d1.polygon(forearm_in, fill=C_ORANGE)
# Forearm highlight line (top edge)
d1.line([(74, 49), (78, 43), (83, 38), (89, 35)], fill=C_ORANGE_L, width=1)
# Forearm shadow line (bottom edge)
d1.line([(75, 53), (79, 51), (84, 46), (90, 42)], fill=C_ORANGE_D, width=1)

# White Paw (curving DOWNWARD towards desk from wrist):
# Wrist is at (88..94, 34..40)
# Paw body extends down: x in [86, 99], y in [38, 49]
# Paw outline: rounded rectangle
d1.rounded_rectangle([86, 38, 99, 49], radius=4, fill=C_OUTLINE)
# Paw white fill
d1.rounded_rectangle([87, 39, 98, 48], radius=3, fill=C_WHITE)
# Paw shadow at bottom
d1.line([(88, 47), (97, 47)], fill=C_WHITE_SH, width=1)
d1.line([(88, 48), (97, 48)], fill=C_WHITE_SH, width=1)

# 3 cute claw notches pointing DOWN towards desk
d1.line([(89, 46), (89, 48)], fill=C_CLAW, width=1)
d1.line([(92, 45), (92, 48)], fill=C_CLAW, width=1)
d1.line([(95, 46), (95, 48)], fill=C_CLAW, width=1)

# Motion lines for the striking paw (cute anime arcs above the raised paw):
# Indicating ready to strike!
d1.arc([99, 32, 103, 40], 280, 80, fill=(255, 255, 255, 200), width=1)
d1.arc([97, 28, 103, 38], 280, 80, fill=(255, 255, 255, 160), width=1)

# Re-enforce scarf over shoulder:
f1_final_arr = np.array(img_f1)
r_arr = np.array(clean_rest)
for y in range(48, 56):
    for x in range(68, 76):
        mr, mg, mb, ma = r_arr[y, x]
        if mr > 150 and mg < 70 and mb < 70:
            f1_final_arr[y, x] = r_arr[y, x]

frame_tap_l = Image.fromarray(f1_final_arr)
frame_tap_l.save('docs_for_me/bongo_tap_l_104x78.png')

# -------------------------------------------------------------------------
# FRAME 2: TAP RIGHT (Right arm DOWN, Left paw RAISED ~60 deg)
# -------------------------------------------------------------------------
f2_arr = np.array(clean_rest)

# In f2, right arm is DOWN (as in clean_rest).
# We erase the resting left paw (x in [45, 68], y in [62, 75])
for y in range(62, 75):
    for x in range(45, 68):
        f2_arr[y, x] = [0,0,0,0]

# Fill dog white chest down to the desk line (y=74)
for y in range(62, 75):
    for x in range(46, 68):
        # White chest fur
        if y < 73:
            f2_arr[y, x] = C_WHITE if y < 70 else C_WHITE_SH
        elif y == 73:
            f2_arr[y, x] = C_WHITE_SH
        elif y == 74:
            f2_arr[y, x] = C_OUTLINE

# Now draw the RAISED LEFT ARM / PAW (~60 deg raised):
# Shoulder starts at x=45, y=54 under scarf.
# In resting pose, paw was at (55, 72).
# In raised 60 deg pose: paw is lifted up to y in [48, 59]!
img_f2 = Image.fromarray(f2_arr)
d2 = ImageDraw.Draw(img_f2)

# Left arm sleeve coming from shoulder (x: 42..49, y: 52..58)
arm_l_poly = [(43, 53), (48, 50), (52, 53), (48, 60), (42, 57)]
d2.polygon(arm_l_poly, fill=C_OUTLINE)
arm_l_in = [(44, 54), (48, 51), (51, 53), (47, 59), (43, 56)]
d2.polygon(arm_l_in, fill=C_ORANGE)

# Raised Left Paw (white paw curved ready to strike):
# Paw box: x in [48, 63], y in [50, 60]
d2.rounded_rectangle([48, 50, 63, 60], radius=4, fill=C_OUTLINE)
d2.rounded_rectangle([49, 51, 62, 59], radius=3, fill=C_WHITE)
d2.line([(50, 58), (61, 58)], fill=C_WHITE_SH, width=1)

# 3 cute claws pointing DOWN towards desk
d2.line([(52, 56), (52, 58)], fill=C_CLAW, width=1)
d2.line([(55, 55), (55, 58)], fill=C_CLAW, width=1)
d2.line([(58, 56), (58, 58)], fill=C_CLAW, width=1)

# Motion lines for the left paw striking:
d2.arc([43, 44, 49, 53], 100, 260, fill=(255, 255, 255, 200), width=1)
d2.arc([41, 41, 48, 55], 100, 260, fill=(255, 255, 255, 160), width=1)

frame_tap_r = img_f2
frame_tap_r.save('docs_for_me/bongo_tap_r_104x78.png')

# -------------------------------------------------------------------------
# 3X PREVIEWS FOR USER EVALUATION (312 x 234 px)
# -------------------------------------------------------------------------
frame_tap_l.resize((104*3, 78*3), Image.NEAREST).save('docs_for_me/action_1_tap_left_right_up_60deg.png')
frame_tap_r.resize((104*3, 78*3), Image.NEAREST).save('docs_for_me/action_2_tap_right_left_up_60deg.png')
frame_rest.resize((104*3, 78*3), Image.NEAREST).save('docs_for_me/action_3_rest_both_down.png')

print('All 3 action frames generated successfully at 1X and 3X!')
