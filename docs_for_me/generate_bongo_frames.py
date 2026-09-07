import numpy as np
from PIL import Image, ImageDraw

def create_frames():
    # Load master dog (319x241)
    master = Image.open('docs_for_me/dog_perfect_master2.png').convert('RGBA')
    MW, MH = master.size # 319, 241

    # Base colors
    COLOR_OUTLINE = (45, 20, 10, 255)
    COLOR_ORANGE = (250, 158, 48, 255)
    COLOR_ORANGE_DARK = (215, 115, 25, 255)
    COLOR_WHITE = (255, 248, 240, 255)
    COLOR_WHITE_SHADOW = (228, 215, 205, 255)
    COLOR_RED_SCARF = (185, 30, 35, 255)

    # 1. Build a clean Torso without arms/paws
    # We take the master image and clean the lower right area where paws were
    torso = master.copy()
    t_arr = np.array(torso)

    # Clear arm area: x in [135, 319], y in [170, 241]
    # But preserve red scarf: red pixels where R>140, G<70, B<70
    is_red = (t_arr[:, :, 0] > 140) & (t_arr[:, :, 1] < 70) & (t_arr[:, :, 2] < 70) & (t_arr[:, :, 3] > 100)

    # Reconstruct torso chest & right flank
    # Chest area: x in [135, 220], y in [170, 236]
    # Right flank area: x in [220, 270], y in [170, 236]
    for y in range(170, 237):
        for x in range(135, 319):
            if is_red[y, x] and y < 198:
                continue # Keep scarf
            if x <= 215:
                # White chest
                if y > 228:
                    t_arr[y, x] = COLOR_WHITE_SHADOW
                else:
                    t_arr[y, x] = COLOR_WHITE
            elif x <= 265:
                # Orange right flank (curving down naturally)
                # Max x for body curves inward as y goes down:
                # at y=170, x_max ~ 265; at y=236, x_max ~ 250
                flank_limit = int(265 - (y - 170) * (15 / 66))
                if x < flank_limit - 3:
                    if y > 225 or x > flank_limit - 8:
                        t_arr[y, x] = COLOR_ORANGE_DARK
                    else:
                        t_arr[y, x] = COLOR_ORANGE
                elif x <= flank_limit:
                    t_arr[y, x] = COLOR_OUTLINE
                else:
                    t_arr[y, x] = [0, 0, 0, 0]
            else:
                t_arr[y, x] = [0, 0, 0, 0]

    # Torso bottom edge outline:
    for x in range(115, 250):
        if t_arr[236, x, 3] > 0:
            t_arr[236, x] = COLOR_OUTLINE

    clean_torso = Image.fromarray(t_arr)
    clean_torso.save('docs_for_me/debug_clean_torso.png')
    print('Clean torso saved')

create_frames()
