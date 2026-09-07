import numpy as np
from PIL import Image, ImageDraw

def build():
    # Load base clean body
    base = Image.open('docs_for_me/test_clean_body_102.png').convert('RGBA')
    W, H = base.size # 102, 76

    C_OUTLINE = (45, 20, 10, 255)
    C_ORANGE = (250, 158, 48, 255)
    C_ORANGE_DARK = (215, 115, 25, 255)
    C_WHITE = (255, 248, 240, 255)
    C_WHITE_SHADOW = (228, 215, 205, 255)
    C_CLAW = (45, 20, 10, 255)
    C_MOTION = (255, 255, 255, 230)

    # Re-refine body chest shading
    body = base.copy()
    b_arr = np.array(body)
    # Add subtle shading on chest
    for y in range(56, 75):
        for x in range(42, 68):
            if b_arr[y, x, 3] > 0 and b_arr[y, x, 0] > 200:
                # shadow on left near flank
                if x < 48 or y > 71:
                    b_arr[y, x] = C_WHITE_SHADOW
                else:
                    b_arr[y, x] = C_WHITE
    body = Image.fromarray(b_arr)

    # -------------------------------------------------------------
    # 1. Left Paw DOWN (slammed on desk at y=75)
    # -------------------------------------------------------------
    def draw_left_paw_down(img):
        draw = ImageDraw.Draw(img)
        # Arm connecting from shoulder (44, 56) to paw (44..66, 62..75)
        # Paw body (white ellipse/polygon)
        draw.ellipse([44, 60, 66, 75], fill=C_WHITE, outline=C_OUTLINE, width=1)
        # Fill interior to ensure solid white
        draw.ellipse([45, 61, 65, 74], fill=C_WHITE)
        # 3 Claw notches at bottom edge (y=69..74)
        draw.line([(50, 69), (50, 74)], fill=C_CLAW, width=1)
        draw.line([(55, 68), (55, 74)], fill=C_CLAW, width=1)
        draw.line([(60, 69), (60, 74)], fill=C_CLAW, width=1)
        # Bottom outline flat on desk
        draw.line([(45, 75), (65, 75)], fill=C_OUTLINE, width=1)

    # -------------------------------------------------------------
    # 2. Left Paw UP (raised high in the air at y=46..58)
    # -------------------------------------------------------------
    def draw_left_paw_up(img):
        draw = ImageDraw.Draw(img)
        # Paw lifted up near chest/scarf (x: 43..64, y: 46..60)
        # In the air! The bottom is at y=59, desk is at y=75 -> 16px gap!
        draw.ellipse([43, 46, 64, 59], fill=C_WHITE, outline=C_OUTLINE, width=1)
        draw.ellipse([44, 47, 63, 58], fill=C_WHITE)
        # Claws pointing slightly out/down
        draw.line([(48, 54), (48, 58)], fill=C_CLAW, width=1)
        draw.line([(53, 53), (53, 58)], fill=C_CLAW, width=1)
        draw.line([(58, 54), (58, 58)], fill=C_CLAW, width=1)
        # Motion arcs above left paw <<
        draw.arc([38, 42, 46, 54], 100, 260, fill=C_MOTION, width=1)
        draw.arc([35, 39, 43, 57], 100, 260, fill=C_MOTION, width=1)

    # -------------------------------------------------------------
    # 3. Right Arm & Paw DOWN (slammed on desk at y=75)
    # -------------------------------------------------------------
    def draw_right_paw_down(img):
        draw = ImageDraw.Draw(img)
        # Orange arm descending from shoulder (70, 50) to (84, 66)
        arm_pts = [(68, 52), (80, 48), (92, 60), (84, 68), (70, 58)]
        draw.polygon(arm_pts, fill=C_ORANGE, outline=C_OUTLINE)
        draw.polygon([(69, 53), (79, 49), (90, 59), (83, 67), (71, 57)], fill=C_ORANGE)
        
        # White paw at end: x in [76, 98], y in [60, 75]
        draw.ellipse([76, 61, 98, 75], fill=C_WHITE, outline=C_OUTLINE, width=1)
        draw.ellipse([77, 62, 97, 74], fill=C_WHITE)
        # 3 Claw notches
        draw.line([(82, 68), (82, 74)], fill=C_CLAW, width=1)
        draw.line([(87, 67), (87, 74)], fill=C_CLAW, width=1)
        draw.line([(92, 68), (92, 74)], fill=C_CLAW, width=1)
        # Bottom outline flat on desk
        draw.line([(78, 75), (96, 75)], fill=C_OUTLINE, width=1)

    # -------------------------------------------------------------
    # 4. Right Arm & Paw UP (LIFTED HIGH IN THE AIR at y=36..52)
    # -------------------------------------------------------------
    def draw_right_paw_up(img):
        draw = ImageDraw.Draw(img)
        # Arm swinging UPWARDS from shoulder (70, 50) towards (92, 40)
        # Paw is at y in [36, 52]! Way up near snout!
        arm_pts = [(68, 52), (72, 46), (88, 38), (94, 46), (76, 58)]
        draw.polygon(arm_pts, fill=C_ORANGE, outline=C_OUTLINE)
        draw.polygon([(69, 51), (73, 47), (87, 39), (92, 45), (75, 57)], fill=C_ORANGE)

        # White paw held high in the air
        draw.ellipse([80, 36, 100, 50], fill=C_WHITE, outline=C_OUTLINE, width=1)
        draw.ellipse([81, 37, 99, 49], fill=C_WHITE)
        # Claws
        draw.line([(85, 43), (85, 48)], fill=C_CLAW, width=1)
        draw.line([(90, 42), (90, 48)], fill=C_CLAW, width=1)
        draw.line([(95, 43), (95, 48)], fill=C_CLAW, width=1)
        # Motion arcs above right paw >>
        draw.arc([97, 30, 103, 44], 280, 80, fill=C_MOTION, width=1)
        draw.arc([100, 27, 106, 47], 280, 80, fill=C_MOTION, width=1)

    # -------------------------------------------------------------
    # Assemble Frames
    # -------------------------------------------------------------
    # Frame 1: Tap Left (Left paw DOWN, Right paw UP HIGH)
    f_tap_l = body.copy()
    draw_right_paw_up(f_tap_l)
    draw_left_paw_down(f_tap_l)
    f_tap_l.save('docs_for_me/frame_tap_l_v1.png')

    # Frame 2: Tap Right (Right paw DOWN, Left paw UP HIGH)
    f_tap_r = body.copy()
    draw_left_paw_up(f_tap_r)
    draw_right_paw_down(f_tap_r)
    f_tap_r.save('docs_for_me/frame_tap_r_v1.png')

    # Frame 3: Rest (Both paws DOWN, smiling eyes)
    f_rest = body.copy()
    draw_right_paw_down(f_rest)
    draw_left_paw_down(f_rest)
    # Happy eyes ^ ^
    d_rest = ImageDraw.Draw(f_rest)
    # Left eye: arc at [52, 26, 61, 36]
    d_rest.ellipse([52, 27, 61, 36], fill=C_WHITE)
    d_rest.arc([52, 28, 61, 36], 190, 350, fill=C_OUTLINE, width=2)
    # Right eye: arc at [70, 26, 79, 36]
    d_rest.ellipse([70, 27, 79, 36], fill=C_WHITE)
    d_rest.arc([70, 28, 79, 36], 190, 350, fill=C_OUTLINE, width=2)
    f_rest.save('docs_for_me/frame_rest_v1.png')

    print('Draft frames generated!')

build()
