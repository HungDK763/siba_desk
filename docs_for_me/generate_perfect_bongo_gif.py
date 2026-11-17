import numpy as np
from PIL import Image, ImageDraw

def run():
    # Load bongo_base_102x76
    base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
    W, H = 102, 76

    # Colors
    C_OUTLINE = (45, 20, 10, 255)
    C_ORANGE = (250, 158, 48, 255)
    C_ORANGE_DARK = (215, 115, 25, 255)
    C_ORANGE_LIGHT = (254, 185, 90, 255)
    C_WHITE = (255, 252, 248, 255)
    C_WHITE_SHADOW = (230, 220, 210, 255)
    C_CLAW = (45, 20, 10, 255)
    C_MOTION = (255, 255, 255, 240)
    C_MOTION_OUT = (45, 20, 10, 180)

    # -------------------------------------------------------------
    # 1. Torso Base (Everything except moving arms)
    # Head, ears, scarf, tail, back fur, and natural chest/flank
    # -------------------------------------------------------------
    torso = base.copy()
    t_arr = np.array(torso)

    # Clear paw areas (x in [42, 102], y in [52, 76])
    # But preserve scarf (y < 62, red pixels)
    # Scarf check
    for y in range(52, 76):
        for x in range(42, 102):
            r, g, b, a = t_arr[y, x]
            # If red scarf pixel:
            if r > 140 and g < 65 and b < 65 and y < 62:
                continue
            # Orange fur from left flank (x in 36..44):
            if x <= 44:
                continue
            # Natural white chest in center: x in 45..68
            if x <= 68:
                if y < 74:
                    # Chest shading
                    if x < 48 or y > 70:
                        t_arr[y, x] = C_WHITE_SHADOW
                    else:
                        t_arr[y, x] = C_WHITE
                elif y in (74, 75):
                    t_arr[y, x] = C_OUTLINE
                else:
                    t_arr[y, x] = [0, 0, 0, 0]
            # Right flank curving down: x in 69..82
            elif x <= 82:
                # Curving flank boundary
                flank_bound = int(76 + 5 * np.sin((y - 50) / 24 * np.pi))
                if x < flank_bound - 1:
                    t_arr[y, x] = C_ORANGE if y < 71 else C_ORANGE_DARK
                elif x <= flank_bound:
                    t_arr[y, x] = C_OUTLINE
                else:
                    t_arr[y, x] = [0, 0, 0, 0]
            else:
                t_arr[y, x] = [0, 0, 0, 0]

    # Torso image
    clean_torso = Image.fromarray(t_arr)

    # -------------------------------------------------------------
    # 2. Arm Drawing Functions
    # -------------------------------------------------------------
    def draw_paw_l_down(canvas):
        """Left paw slammed down on desk (y=75)"""
        d = ImageDraw.Draw(canvas)
        # Bicep/shoulder connection
        d.rectangle([45, 58, 48, 64], fill=C_WHITE)
        # White chubby paw
        d.ellipse([44, 60, 66, 75], fill=C_OUTLINE)
        d.ellipse([45, 61, 65, 74], fill=C_WHITE)
        # Inner shadow at top of paw
        d.arc([46, 61, 64, 68], 200, 340, fill=C_WHITE_SHADOW, width=2)
        # 3 distinct claw slits
        d.line([(50, 68), (50, 74)], fill=C_CLAW, width=1)
        d.line([(55, 67), (55, 74)], fill=C_CLAW, width=1)
        d.line([(60, 68), (60, 74)], fill=C_CLAW, width=1)
        # Flat desk contact
        d.line([(45, 75), (65, 75)], fill=C_OUTLINE, width=1)

    def draw_paw_l_up(canvas):
        """Left paw LIFTED HIGH UP near chest/scarf (y=44..58)"""
        d = ImageDraw.Draw(canvas)
        # Raised paw in the air!
        # Position: x in [42, 64], y in [44, 58] -> Gap to desk (y=75) is 17 PIXELS!
        d.ellipse([42, 44, 64, 58], fill=C_OUTLINE)
        d.ellipse([43, 45, 63, 57], fill=C_WHITE)
        d.arc([44, 45, 62, 52], 200, 340, fill=C_WHITE_SHADOW, width=1)
        # 3 claw slits pointing slightly downward/forward
        d.line([(47, 51), (47, 57)], fill=C_CLAW, width=1)
        d.line([(52, 50), (52, 57)], fill=C_CLAW, width=1)
        d.line([(57, 51), (57, 57)], fill=C_CLAW, width=1)
        # Motion arcs << above the paw
        d.arc([35, 38, 45, 52], 110, 250, fill=C_MOTION, width=1)
        d.arc([31, 35, 41, 55], 110, 250, fill=C_MOTION, width=1)

    def draw_paw_r_down(canvas):
        """Right arm & paw slammed down on desk (y=75)"""
        d = ImageDraw.Draw(canvas)
        # Orange arm descending diagonally from shoulder (68, 50) to (84, 66)
        # Outline polygon
        arm_out = [(68, 52), (78, 48), (92, 60), (86, 68), (70, 58)]
        d.polygon(arm_out, fill=C_OUTLINE)
        # Arm fill
        arm_fill = [(69, 53), (77, 49), (90, 59), (85, 67), (71, 57)]
        d.polygon(arm_fill, fill=C_ORANGE)
        # Highlight on top edge of arm
        d.line([(70, 52), (76, 49), (88, 58)], fill=C_ORANGE_LIGHT, width=1)
        # Shadow on bottom edge of arm
        d.line([(72, 57), (84, 66)], fill=C_ORANGE_DARK, width=1)

        # White paw pressed on desk: x in [76, 98], y in [61, 75]
        d.ellipse([76, 61, 98, 75], fill=C_OUTLINE)
        d.ellipse([77, 62, 97, 74], fill=C_WHITE)
        d.arc([78, 62, 96, 68], 200, 340, fill=C_WHITE_SHADOW, width=1)
        # 3 claw slits
        d.line([(82, 68), (82, 74)], fill=C_CLAW, width=1)
        d.line([(87, 67), (87, 74)], fill=C_CLAW, width=1)
        d.line([(92, 68), (92, 74)], fill=C_CLAW, width=1)
        # Flat desk contact
        d.line([(78, 75), (96, 75)], fill=C_OUTLINE, width=1)

    def draw_paw_r_up(canvas):
        """Right arm & paw LIFTED HIGH UP IN THE AIR (y=34..50)"""
        d = ImageDraw.Draw(canvas)
        # Arm reaches UPWARDS-RIGHT towards mouth/cheek height!
        # Shoulder at (68, 50), arm extends to (88, 38)
        arm_out = [(68, 52), (72, 44), (88, 36), (96, 44), (76, 58)]
        d.polygon(arm_out, fill=C_OUTLINE)
        arm_fill = [(69, 51), (73, 45), (87, 37), (94, 43), (75, 57)]
        d.polygon(arm_fill, fill=C_ORANGE)
        d.line([(73, 45), (86, 37)], fill=C_ORANGE_LIGHT, width=1)
        d.line([(70, 52), (75, 57)], fill=C_ORANGE_DARK, width=1)

        # White paw held high in the air: x in [80, 101], y in [34, 49]
        # Distance to desk (y=75) is 26 PIXELS!
        d.ellipse([80, 34, 101, 49], fill=C_OUTLINE)
        d.ellipse([81, 35, 100, 48], fill=C_WHITE)
        d.arc([82, 35, 99, 42], 200, 340, fill=C_WHITE_SHADOW, width=1)
        # 3 claw slits
        d.line([(86, 42), (86, 48)], fill=C_CLAW, width=1)
        d.line([(91, 41), (91, 48)], fill=C_CLAW, width=1)
        d.line([(96, 42), (96, 48)], fill=C_CLAW, width=1)
        # Motion arcs >> above right paw
        d.arc([97, 26, 104, 40], 290, 70, fill=C_MOTION, width=1)
        d.arc([100, 23, 107, 43], 290, 70, fill=C_MOTION, width=1)

    # -------------------------------------------------------------
    # 3. Create the 3 Master Frames
    # -------------------------------------------------------------
    # Frame 1: Tap Left (Left paw DOWN, Right paw UP HIGH)
    f_tap_l = clean_torso.copy()
    draw_paw_r_up(f_tap_l)
    draw_paw_l_down(f_tap_l)

    # Frame 2: Tap Right (Right paw DOWN, Left paw UP HIGH)
    f_tap_r = clean_torso.copy()
    draw_paw_l_up(f_tap_r)
    draw_paw_r_down(f_tap_r)

    # Frame 3: Rest (Both paws DOWN, smiling happy eyes ^_^)
    f_rest = clean_torso.copy()
    draw_paw_r_down(f_rest)
    draw_paw_l_down(f_rest)
    # Happy eyes ^ ^
    d_rest = ImageDraw.Draw(f_rest)
    # Left eye area: (52..61, 26..36)
    d_rest.ellipse([51, 26, 61, 36], fill=C_WHITE)
    d_rest.arc([51, 27, 61, 35], 190, 350, fill=C_OUTLINE, width=2)
    # Right eye area: (70..79, 26..36)
    d_rest.ellipse([70, 26, 80, 36], fill=C_WHITE)
    d_rest.arc([70, 27, 80, 35], 190, 350, fill=C_OUTLINE, width=2)

    # Save individual master frames
    f_tap_l.save('docs_for_me/bongo_perfect_tap_l.png')
    f_tap_r.save('docs_for_me/bongo_perfect_tap_r.png')
    f_rest.save('docs_for_me/bongo_perfect_rest.png')

    # Save 3x enlarged individual frames
    f_tap_l.resize((W*3, H*3), Image.NEAREST).save('docs_for_me/bongo_perfect_tap_l_3x.png')
    f_tap_r.resize((W*3, H*3), Image.NEAREST).save('docs_for_me/bongo_perfect_tap_r_3x.png')
    f_rest.resize((W*3, H*3), Image.NEAREST).save('docs_for_me/bongo_perfect_rest_3x.png')

    # -------------------------------------------------------------
    # 4. Generate Animation Sequences
    # 3s tapping: 20 frames * 150ms = 3000ms
    # 2s resting: 1 frame * 2000ms = 2000ms
    # -------------------------------------------------------------
    anim_frames = []
    durations = []
    for _ in range(10):
        anim_frames.append(f_tap_l)
        durations.append(150)
        anim_frames.append(f_tap_r)
        durations.append(150)
    anim_frames.append(f_rest)
    durations.append(2000)

    # 4A. Transparent GIF (disposal=2 so zero ghosting)
    anim_frames[0].save(
        'docs_for_me/bongo_shiba_transparent.gif',
        save_all=True,
        append_images=anim_frames[1:],
        duration=durations,
        loop=0,
        disposal=2
    )

    # 4B. Desk Rendered GIF (Exact hardware simulation with mini quota desk)
    BG_COLOR = (15, 17, 26, 255)
    DESK_COLOR = (26, 32, 44, 255) # Launcher quota panel background
    DESK_BORDER = (45, 55, 75, 255) # Quota panel border

    desk_frames = []
    for f in anim_frames:
        canvas = Image.new('RGBA', (110, 84), BG_COLOR)
        cd = ImageDraw.Draw(canvas)
        # Desk surface bar at bottom
        cd.rectangle([0, 77, 110, 84], fill=DESK_COLOR)
        cd.line([(0, 76), (110, 76)], fill=DESK_BORDER, width=1)
        # Dog paws sit on y=76
        canvas.paste(f, (4, 1), f)
        desk_frames.append(canvas)

    desk_frames[0].save(
        'docs_for_me/bongo_shiba_desk_1x.gif',
        save_all=True,
        append_images=desk_frames[1:],
        duration=durations,
        loop=0
    )

    # 4C. High-Res 3X Desk GIF for user review
    desk_3x = [c.resize((c.width * 3, c.height * 3), Image.NEAREST) for c in desk_frames]
    desk_3x[0].save(
        'docs_for_me/bongo_shiba_desk_3x.gif',
        save_all=True,
        append_images=desk_3x[1:],
        duration=durations,
        loop=0
    )

    # 4D. High-Res 3X Transparent GIF for user review
    trans_3x = [f.resize((W * 3, H * 3), Image.NEAREST) for f in anim_frames]
    trans_3x[0].save(
        'docs_for_me/bongo_shiba_transparent_3x.gif',
        save_all=True,
        append_images=trans_3x[1:],
        duration=durations,
        loop=0,
        disposal=2
    )

    print('ALL PERFECT BONGO GIFS GENERATED SUCCESSFULLY!')

run()
