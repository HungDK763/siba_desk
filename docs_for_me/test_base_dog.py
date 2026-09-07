from PIL import Image, ImageDraw
import numpy as np

# Load bongo_cleaned_102x76
base = Image.open('docs_for_me/bongo_cleaned_102x76.png').convert('RGBA')
w, h = base.size

# Let's inspect the area where the paws are:
# Left paw: x in 44..66, y in 56..76
# Right paw: x in 68..102, y in 48..76

# Create a clean body by removing paws and filling chest and flank:
body = base.copy()
b_arr = np.array(body)

# Scarf protection: red pixels in scarf (x in 30..80, y in 30..62)
is_scarf = (b_arr[:, :, 0] > 150) & (b_arr[:, :, 1] < 60) & (b_arr[:, :, 2] < 60) & (b_arr[:, :, 3] > 100)

# Colors
C_OUTLINE = (45, 20, 10, 255)
C_ORANGE = (250, 158, 48, 255)
C_ORANGE_DARK = (215, 115, 25, 255)
C_WHITE = (255, 248, 240, 255)
C_WHITE_SHADOW = (228, 215, 205, 255)

# Clear arm region from body
for y in range(48, 76):
    for x in range(42, 102):
        if is_scarf[y, x] and y < 62:
            continue # preserve scarf
        # Chest area
        if x < 68:
            if y < 74:
                b_arr[y, x] = C_WHITE if y < 71 else C_WHITE_SHADOW
            elif y == 74 or y == 75:
                b_arr[y, x] = C_OUTLINE
            else:
                b_arr[y, x] = [0, 0, 0, 0]
        # Right flank area: curves naturally from x=74 at y=50 to x=80 at y=65 to x=76 at y=74
        else:
            flank_x = int(72 + 6 * np.sin((y - 48) / 26 * np.pi))
            if x < flank_x - 1:
                b_arr[y, x] = C_ORANGE if y < 71 else C_ORANGE_DARK
            elif x <= flank_x:
                b_arr[y, x] = C_OUTLINE
            else:
                b_arr[y, x] = [0, 0, 0, 0]

# Preserve the left body flank outline at x in 30..42
clean_body = Image.fromarray(b_arr)
clean_body.save('docs_for_me/test_clean_body_102.png')
print('test_clean_body_102 saved')
