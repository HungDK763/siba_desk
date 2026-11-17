from PIL import Image, ImageDraw
import numpy as np

# Load clean dog master
master = Image.open('docs_for_me/dog_master_clean.png').convert('RGBA')
marr = np.array(master)
H, W, _ = marr.shape

# Clean borders
marr[:145, :45] = [0,0,0,0]
marr[:150, 285:] = [0,0,0,0]
marr[238:, :] = [0,0,0,0]

# -------------------------------------------------------------------------
# Colors
# -------------------------------------------------------------------------
C_OUTLINE    = (58, 16, 6, 255)       # #3a1006 Dark outline
C_ORANGE_LGT = (255, 185, 90, 255)    # Top highlight
C_ORANGE     = (255, 163, 52, 255)    # Vibrant Shiba Orange
C_ORANGE_MID = (248, 139, 38, 255)    # Mid orange
C_ORANGE_DRK = (215, 105, 25, 255)    # Shadow orange
C_WHITE      = (255, 254, 250, 255)   # White fur
C_WHITE_SHD  = (238, 222, 204, 255)   # Paw shadow
C_WHITE_DRK  = (205, 190, 175, 255)   # Dark white shadow
C_CLAW       = (45, 8, 0, 255)        # Claw notch

# -------------------------------------------------------------------------
# 1. Prepare Base Dog with Flank
# -------------------------------------------------------------------------
body_f1 = marr.copy()

# Clear old down-arm (x >= 242, y >= 165)
for y in range(165, 238):
    for x in range(242, W):
        r, g, b, a = body_f1[y, x]
        if r > 140 and g < 70 and b < 70 and y < 178:
            continue
        body_f1[y, x] = [0,0,0,0]

# Draw natural dog flank (body behind arm)
# From shoulder (252, 168) down to desk (265, 238)
for y in range(166, 238):
    t = (y - 166) / (238 - 166)
    # Natural seated dog flank: slight outward curve
    edge_x = int(252 + 15 * np.sin(t * np.pi * 0.8) + t * 4)
    for x in range(238, edge_x):
        # fur shading
        if x < edge_x - 5:
            body_f1[y, x] = C_ORANGE
        elif x < edge_x - 2:
            body_f1[y, x] = C_ORANGE_MID
        else:
            body_f1[y, x] = C_ORANGE_DRK
    # Smooth outline
    body_f1[y, edge_x] = C_OUTLINE
    body_f1[y, edge_x + 1] = C_OUTLINE

img_f1 = Image.fromarray(body_f1)
draw = ImageDraw.Draw(img_f1)

# -------------------------------------------------------------------------
# 2. Draw the Arched 60-Degree Raised Right Arm
# -------------------------------------------------------------------------
# Arm trajectory:
# Shoulder: (248, 170)
# Elbow (peak of lift, ~60 deg): (290, 130)
# Wrist: (312, 148)
# Paw center: (316, 168)
# Paw tip: (316, 185) -> Lifted 53px above desk!

# We construct the arm using pixel-art layers:
# Layer 1: Dark outline (thick polygon)
# Layer 2: Orange body with shading
# Layer 3: White paw with rounded toe lobes & claws

# Forearm polygon (outer outline)
forearm_outline = [
    (246, 168), (252, 160), (288, 122), (298, 124),
    (322, 146), (318, 164), (292, 148), (258, 184), (246, 178)
]
draw.polygon(forearm_outline, fill=C_OUTLINE)

# Forearm inner (orange fill)
forearm_inner = [
    (248, 168), (253, 162), (288, 124), (296, 126),
    (319, 148), (315, 161), (291, 145), (259, 181), (248, 175)
]
draw.polygon(forearm_inner, fill=C_ORANGE)

# Forearm top highlight line
draw.line([(253, 162), (288, 124), (296, 126), (316, 146)], fill=C_ORANGE_LGT, width=2)
# Forearm underside shadow line
draw.line([(249, 175), (260, 180), (290, 145), (314, 160)], fill=C_ORANGE_DRK, width=2)

# Paw Outline (curving downwards from wrist, rounded paw)
# Bounding box of paw: x in [295, 332], y in [145, 186]
draw.rounded_rectangle([296, 144, 332, 184], radius=12, fill=C_OUTLINE)
# White paw inner
draw.rounded_rectangle([298, 146, 330, 182], radius=10, fill=C_WHITE)
# Paw underside shadow
draw.arc([299, 155, 329, 181], 0, 180, fill=C_WHITE_SHD, width=3)

# 3 cute claw notches (angled slightly down-forward, ready to smack)
# Claw 1 (left)
draw.line([(304, 174), (304, 181)], fill=C_CLAW, width=2)
# Claw 2 (middle)
draw.line([(313, 173), (313, 182)], fill=C_CLAW, width=2)
# Claw 3 (right)
draw.line([(322, 174), (322, 181)], fill=C_CLAW, width=2)

# Restore red scarf knot over shoulder
sarr = np.array(img_f1)
for y in range(150, 180):
    for x in range(235, 260):
        mr, mg, mb, ma = marr[y, x]
        if mr > 130 and mg < 70 and mb < 70:
            sarr[y, x] = marr[y, x]

final_f1 = Image.fromarray(sarr)
bbox = final_f1.getbbox()
final_f1.crop(bbox).save('docs_for_me/authentic_action1_test.png')
print('authentic_action1_test.png saved, size:', final_f1.crop(bbox).size)
