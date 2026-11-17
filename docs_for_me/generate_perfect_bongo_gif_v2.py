import numpy as np
from PIL import Image, ImageDraw

def run():
    # Load original bongo_base_102x76 (the most authentic crop from bongo_dog_crop.png)
    base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
    W, H = 102, 76

    # Colors
    C_OUTLINE = (45, 20, 10, 255)
    C_ORANGE = (250, 158, 48, 255)
    C_ORANGE_DARK = (215, 115, 25, 255)
    C_ORANGE_LIGHT = (254, 185, 90, 255)
    C_WHITE = (255, 252, 248, 255)
    C_WHITE_SHD = (230, 220, 210, 255)
    C_CLAW = (45, 20, 10, 255)
    C_MOTION = (255, 255, 255, 230)

    # -------------------------------------------------------------
    # 1. Base Dog Body (Chest & Flanks naturally contoured)
    # -------------------------------------------------------------
    torso = base.copy()
    t_arr = np.array(torso)

    # Clear paw region (x >= 42, y >= 54)
    # We carefully fill the chest and right flank with a rounded contour
    for y in range(54, 76):
        for x in range(42, 102):
            r, g, b, a = t_arr[y, x]
            # Preserve red scarf pixels (y < 62)
            if r > 140 and g < 65 and b < 65 and y < 62:
                continue
            # Orange fur from left flank (x <= 44):
            if x <= 44:
                continue

            # Chest width varies with y: rounded belly
            # at y=56: chest x in 45..68
            # at y=66: chest x in 44..70
            # at y=74: chest x in 45..68
            chest_right = int(69 + 2 * np.sin((y - 54) / 20 * np.pi))
            body_right = int(77 + 4 * np.sin((y - 50) / 24 * np.pi))

            if x <= chest_right:
                # White chest
                if y < 74:
                    if x < 48 or y > 70 or x > chest_right - 2:
                        t_arr[y, x] = C_WHITE_SHD
                    else:
                        t_arr[y, x] = C_WHITE
                elif y in (74, 75):
                    t_arr[y, x] = C_OUTLINE
                else:
                    t_arr[y, x] = [0, 0, 0, 0]
            elif x <= body_right:
                # Orange right flank
                if x < body_right - 1:
                    t_arr[y, x] = C_ORANGE if y < 71 else C_ORANGE_DARK
                elif x <= body_right:
                    t_arr[y, x] = C_OUTLINE
                else:
                    t_arr[y, x] = [0, 0, 0, 0]
            else:
                t_arr[y, x] = [0, 0, 0, 0]

    clean_torso = Image.fromarray(t_arr)

    # -------------------------------------------------------------
    # 2. Pixel-art Paw & Arm Renderers
    # -------------------------------------------------------------
    def draw_paw_l(canvas, state='down'):
        d = ImageDraw.Draw(canvas)
        if state == 'down':
            # Paw on desk: x in [44, 66], y in [60, 75]
            # 3 rounded lobes for the toes!
            d.ellipse([44, 60, 66, 75], fill=C_OUTLINE)
            d.ellipse([45, 61, 65, 74], fill=C_WHITE)
            # Subtle top shadow
            d.arc([46, 61, 64, 67], 200, 340, fill=C_WHITE_SHD, width=1)
            # Toe separations (vertical notches with rounded tips)
            d.line([(50, 67), (50, 74)], fill=C_CLAW, width=1)
            d.line([(56, 66), (56, 74)], fill=C_CLAW, width=1)
            d.line([(61, 67), (61, 74)], fill=C_CLAW, width=1)
            # Desk contact line
            d.line([(45, 75), (65, 75)], fill=C_OUTLINE, width=1)
        else:
            # Paw UP in the air: x in [43, 63], y in [44, 58]
            # Lifted 17px above desk!
            d.ellipse([43, 44, 63, 58], fill=C_OUTLINE)
            d.ellipse([44, 45, 62, 57], fill=C_WHITE)
            d.arc([45, 45, 61, 51], 200, 340, fill=C_WHITE_SHD, width=1)
            # Claws angled
            d.line([(48, 51), (48, 57)], fill=C_CLAW, width=1)
            d.line([(53, 50), (53, 57)], fill=C_CLAW, width=1)
            d.line([(58, 51), (58, 57)], fill=C_CLAW, width=1)
            # Motion arcs << on the left of lifted paw
            d.arc([34, 38, 44, 52], 100, 260, fill=C_MOTION, width=1)
            d.arc([30, 35, 40, 55], 100, 260, fill=C_MOTION, width=1)

    def draw_paw_r(canvas, state='down'):
        d = ImageDraw.Draw(canvas)
        if state == 'down':
            # Orange arm reaching down-right
            arm_poly = [(66, 52), (76, 48), (92, 60), (86, 68), (68, 60)]
            d.polygon(arm_poly, fill=C_OUTLINE)
            arm_in = [(67, 53), (75, 49), (90, 59), (85, 67), (69, 59)]
            d.polygon(arm_in, fill=C_ORANGE)
            d.line([(68, 52), (74, 49), (88, 58)], fill=C_ORANGE_LIGHT, width=1)
            d.line([(70, 59), (84, 67)], fill=C_ORANGE_DARK, width=1)

            # White paw on desk: x in [76, 98], y in [61, 75]
            d.ellipse([76, 61, 98, 75], fill=C_OUTLINE)
            d.ellipse([77, 62, 97, 74], fill=C_WHITE)
            d.arc([78, 62, 96, 67], 200, 340, fill=C_WHITE_SHD, width=1)
            # 3 toe notches
            d.line([(82, 67), (82, 74)], fill=C_CLAW, width=1)
            d.line([(87, 66), (87, 74)], fill=C_CLAW, width=1)
            d.line([(92, 67), (92, 74)], fill=C_CLAW, width=1)
            # Desk contact line
            d.line([(78, 75), (96, 75)], fill=C_OUTLINE, width=1)
        else:
            # Arm reaching UPWARDS-RIGHT towards cheek/mouth!
            # Paw at y in [34, 49] -> 26px ABOVE DESK!
            arm_poly = [(66, 52), (72, 44), (88, 36), (96, 44), (74, 58)]
            d.polygon(arm_poly, fill=C_OUTLINE)
            arm_in = [(67, 51), (73, 45), (87, 37), (94, 43), (73, 57)]
            d.polygon(arm_in, fill=C_ORANGE)
            d.line([(73, 45), (86, 37)], fill=C_ORANGE_LIGHT, width=1)
            d.line([(68, 52), (72, 57)], fill=C_ORANGE_DARK, width=1)

            # White paw held high in the air
            d.ellipse([80, 34, 101, 49], fill=C_OUTLINE)
            d.ellipse([81, 35, 100, 48], fill=C_WHITE)
            d.arc([82, 35, 99, 41], 200, 340, fill=C_WHITE_SHD, width=1)
            # 3 toe notches
            d.line([(86, 41), (86, 48)], fill=C_CLAW, width=1)
            d.line([(91, 40), (91, 48)], fill=C_CLAW, width=1)
            d.line([(96, 41), (96, 48)], fill=C_CLAW, width=1)
            # Motion arcs >> on the right of lifted paw
            d.arc([98, 25, 105, 39], 290, 70, fill=C_MOTION, width=1)
            d.arc([101, 22, 108, 42], 290, 70, fill=C_MOTION, width=1)

    # -------------------------------------------------------------
    # 3. Create the 3 Master Frames
    # -------------------------------------------------------------
    # Frame 1: Tap Left (Left paw DOWN, Right paw UP HIGH)
    f_tap_l = clean_torso.copy()
    draw_paw_r(f_tap_l, state='up')
    draw_paw_l(f_tap_l, state='down')

    # Frame 2: Tap Right (Right paw DOWN, Left paw UP HIGH)
    f_tap_r = clean_torso.copy()
    draw_paw_l(f_tap_r, state='up')
    draw_paw_r(f_tap_r, state='down')

    # Frame 3: Rest (Both paws DOWN, smiling happy eyes ^_^)
    f_rest = clean_torso.copy()
    draw_paw_r(f_rest, state='down')
    draw_paw_l(f_rest, state='down')
    # Happy eyes ^ ^
    d_rest = ImageDraw.Draw(f_rest)
    d_rest.ellipse([51, 26, 61, 36], fill=C_WHITE)
    d_rest.arc([51, 27, 61, 35], 190, 350, fill=C_OUTLINE, width=2)
    d_rest.ellipse([70, 26, 80, 36], fill=C_WHITE)
    d_rest.arc([70, 27, 80, 35], 190, 350, fill=C_OUTLINE, width=2)

    # Save frames
    f_tap_l.save('docs_for_me/bongo_final_tap_l.png')
    f_tap_r.save('docs_for_me/bongo_final_tap_r.png')
    f_rest.save('docs_for_me/bongo_final_rest.png')

    # 3x views
    f_tap_l.resize((W*3, H*3), Image.NEAREST).save('docs_for_me/bongo_final_tap_l_3x.png')
    f_tap_r.resize((W*3, H*3), Image.NEAREST).save('docs_for_me/bongo_final_tap_r_3x.png')
    f_rest.resize((W*3, H*3), Image.NEAREST).save('docs_for_me/bongo_final_rest_3x.png')

    # -------------------------------------------------------------
    # 4. Generate Animated GIFs
    # Sequence: 3s tapping (20 frames @ 150ms) + 2s rest (1 frame @ 2000ms)
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

    # Desk context simulation
    BG_COLOR = (15, 17, 26, 255)
    DESK_COLOR = (26, 32, 44, 255)
    DESK_BORDER = (45, 55, 75, 255)

    desk_frames = []
    for f in anim_frames:
        canvas = Image.new('RGBA', (110, 84), BG_COLOR)
        cd = ImageDraw.Draw(canvas)
        cd.rectangle([0, 77, 110, 84], fill=DESK_COLOR)
        cd.line([(0, 76), (110, 76)], fill=DESK_BORDER, width=1)
        canvas.paste(f, (4, 1), f)
        desk_frames.append(canvas)

    # 3X Desk GIF
    desk_3x = [c.resize((c.width * 3, c.height * 3), Image.NEAREST) for c in desk_frames]
    desk_3x[0].save(
        'docs_for_me/bongo_final_desk_3x.gif',
        save_all=True,
        append_images=desk_3x[1:],
        duration=durations,
        loop=0
    )

    # 3X Transparent GIF
    trans_3x = [f.resize((W * 3, H * 3), Image.NEAREST) for f in anim_frames]
    trans_3x[0].save(
        'docs_for_me/bongo_final_transparent_3x.gif',
        save_all=True,
        append_images=trans_3x[1:],
        duration=durations,
        loop=0,
        disposal=2
    )

    # Copy to brain artifact directory so it displays immediately in the chat!
    import shutil
    brain_dir = '/home/hungdk4/.gemini/antigravity-cli/brain/3aced826-248d-4994-94da-3362ffe3e8e7'
    shutil.copy('docs_for_me/bongo_final_desk_3x.gif', f'{brain_dir}/bongo_final_desk_3x.gif')
    shutil.copy('docs_for_me/bongo_final_transparent_3x.gif', f'{brain_dir}/bongo_final_transparent_3x.gif')
    shutil.copy('docs_for_me/bongo_final_tap_l_3x.png', f'{brain_dir}/bongo_final_tap_l_3x.png')
    shutil.copy('docs_for_me/bongo_final_tap_r_3x.png', f'{brain_dir}/bongo_final_tap_r_3x.png')
    shutil.copy('docs_for_me/bongo_final_rest_3x.png', f'{brain_dir}/bongo_final_rest_3x.png')

    print('Bongo Final Animation generated & copied to brain successfully!')

run()
