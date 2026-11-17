from PIL import Image
import numpy as np

# Load bongo_dog_crop
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')

# In docs_for_me/isolated_arm_r_tight.png (size ~90x78)
# Let's inspect its pivot point:
# The arm connects at the shoulder under the scarf.
# In the original image bongo_dog_crop:
# Shoulder pivot is at approximately x=250, y=172.
# Let's verify this!
