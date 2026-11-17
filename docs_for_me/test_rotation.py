from PIL import Image
import numpy as np

# Load bongo_dog_crop
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
W, H = orig.size # 360, 268

# 1. Clean background of bongo_dog_crop:
# Dark space background in orig:
# Let's inspect orig
print('Original size:', W, H)
