from PIL import Image
import numpy as np

# Load bongo_dog_crop
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')

# 1. Clean background of bongo_dog_crop:
# Convert space background to transparent
arr = np.array(orig)
# Space background: r < 50, g < 60, b > r
# Let's inspect
is_space = (arr[:, :, 2] > 70) & (arr[:, :, 2] > arr[:, :, 0] + 15) & (arr[:, :, 1] < 120)
is_window = (arr[:, :, 0] < 60) & (arr[:, :, 1] < 60) & (arr[:, :, 2] < 70) & (np.arange(arr.shape[1])[None, :] < 25)

# Desk at bottom: y > 238
is_desk = np.arange(arr.shape[0])[:, None] > 238

# Right arm crop: x in [245, 335], y in [160, 245]
# Shoulder pivot in bongo_dog_crop is at (x=255, y=180)
# Left paw crop: x in [145, 230], y in [185, 240]
# Left shoulder pivot is at (x=160, y=200)

print('Testing arm rotation angles...')
