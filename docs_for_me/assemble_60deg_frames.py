from PIL import Image, ImageDraw
import numpy as np

# Master image: 319x241
master = Image.open('docs_for_me/dog_perfect_master2.png').convert('RGBA')
MW, MH = master.size # 319, 241

# Base dog body:
# We need a clean background for when each arm lifts up.
# When Right Arm lifts: what's behind it?
# In master, behind right arm (x in 240..319, y in 160..241) is the space background and the dog's right flank.
# Right flank curves down at x=240..260. To the right of x=260 is transparent space!
# When Left Arm lifts: what's behind it?
# In master, behind left paw (x in 140..215, y in 180..241) is the white chest fur!
# Let's create two clean backgrounds or one base background.

# 1. Load authentic arm sprites:
arm_r_down = Image.open('docs_for_me/arm_r_tight.png').convert('RGBA')
arm_l_down = Image.open('docs_for_me/arm_l_tight.png').convert('RGBA')

# Rotated arms (60 degrees)
# Right arm: pivot at shoulder px=22, py=18 in arm_r_tight
# Shoulder in master is at sx=248, sy=164
px_r, py_r = 22, 18
sx_r, sy_r = 248, 164

canvas_r = Image.new('RGBA', (400, 400), (0, 0, 0, 0))
canvas_r.paste(arm_r_down, (200 - px_r, 200 - py_r), arm_r_down)
arm_r_up = canvas_r.rotate(58, resample=Image.Resampling.BICUBIC, center=(200, 200))

# Left arm: pivot at shoulder px=12, py=12 in arm_l_tight
# Shoulder in master is at sx=142, sy=184
px_l, py_l = 12, 12
sx_l, sy_l = 142, 184

canvas_l = Image.new('RGBA', (400, 400), (0, 0, 0, 0))
canvas_l.paste(arm_l_down, (200 - px_l, 200 - py_l), arm_l_down)
arm_l_up = canvas_l.rotate(52, resample=Image.Resampling.BICUBIC, center=(200, 200))

# -------------------------------------------------------------
# Clean Base Torso
# -------------------------------------------------------------
m_arr = np.array(master)

# Clean behind left paw: fill chest with authentic chest color
# Chest white: (254, 248, 240, 255)
# Chest shadow: (228, 218, 208, 255)
torso_arr = m_arr.copy()

# Behind right arm:
# Clear arm down pixels (x in 238..319, y in 162..241)
# Fill flank: flank curves from (250, 162) to (260, 210) to (248, 236)
for y in range(162, 241):
    for x in range(238, 319):
        r, g, b, a = torso_arr[y, x]
        if r > 140 and g < 65 and b < 65 and y < 196:
            continue # keep scarf
        flank_limit = int(258 - (y - 162) * 0.2)
        if x < flank_limit - 2:
            torso_arr[y, x] = torso_arr[y, 230] # clone flank fur
        elif x <= flank_limit:
            torso_arr[y, x] = [45, 20, 10, 255] # dark outline
        else:
            torso_arr[y, x] = [0, 0, 0, 0]

# Behind left paw:
# Clear left paw down pixels (x in 135..215, y in 180..241)
# Fill with white chest
for y in range(180, 237):
    for x in range(135, 225):
        r, g, b, a = torso_arr[y, x]
        if r > 140 and g < 65 and b < 65 and y < 198:
            continue # keep scarf
        if y > 228 or x < 145:
            torso_arr[y, x] = [228, 218, 208, 255]
        else:
            torso_arr[y, x] = [254, 250, 244, 255]

for x in range(135, 225):
    torso_arr[236, x] = [45, 20, 10, 255] # bottom outline

clean_base = Image.fromarray(torso_arr)

# -------------------------------------------------------------
# FRAME 1: Tap Left (Left paw DOWN, Right paw UP 60 deg)
# -------------------------------------------------------------
f1 = clean_base.copy()
# Paste right arm UP
f1.paste(arm_r_up, (sx_r - 200, sy_r - 200), arm_r_up)
# Paste left paw DOWN at original position
f1.paste(arm_l_down, (130, 175), arm_l_down)
# Motion arcs for left paw tap (slap lines on table)
d1 = ImageDraw.Draw(f1)
# Motion arcs << above table on left
d1.arc([115, 205, 135, 235], 110, 250, fill=(255, 255, 255, 240), width=3)
d1.arc([105, 195, 125, 240], 110, 250, fill=(255, 255, 255, 200), width=3)
f1.save('docs_for_me/frame_60deg_tap_l.png')

# -------------------------------------------------------------
# FRAME 2: Tap Right (Right paw DOWN, Left paw UP 60 deg)
# -------------------------------------------------------------
f2 = clean_base.copy()
# Paste left paw UP
f2.paste(arm_l_up, (sx_l - 200, sy_l - 200), arm_l_up)
# Paste right arm DOWN at original position
f2.paste(arm_r_down, (230, 150), arm_r_down)
# Motion arcs for right paw tap (the authentic )) marks from bongo_dog_crop)
d2 = ImageDraw.Draw(f2)
d2.arc([305, 140, 335, 180], 290, 70, fill=(255, 255, 255, 240), width=3)
d2.arc([315, 130, 345, 185], 290, 70, fill=(255, 255, 255, 200), width=3)
f2.save('docs_for_me/frame_60deg_tap_r.png')

# -------------------------------------------------------------
# FRAME 3: Rest (Both paws DOWN, smiling happy eyes ^_^)
# -------------------------------------------------------------
f3 = clean_base.copy()
f3.paste(arm_r_down, (230, 150), arm_r_down)
f3.paste(arm_l_down, (130, 175), arm_l_down)
d3 = ImageDraw.Draw(f3)
# Eyes in master (319x241):
# Left eye around (165, 85), Right eye around (225, 85)
d3.ellipse([160, 82, 190, 115], fill=(254, 250, 244, 255))
d3.arc([160, 85, 190, 112], 190, 350, fill=(45, 20, 10, 255), width=5)
d3.ellipse([220, 82, 250, 115], fill=(254, 250, 244, 255))
d3.arc([220, 85, 250, 112], 190, 350, fill=(45, 20, 10, 255), width=5)
f3.save('docs_for_me/frame_60deg_rest.png')

print('All 3 frames generated at master resolution!')
