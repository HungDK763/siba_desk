from PIL import Image, ImageDraw
import numpy as np

# Load authentic right arm
arm = Image.open('docs_for_me/isolated_arm_r_tight.png').convert('RGBA')
arr = np.array(arm)
h, w, _ = arr.shape
print(f'Arm size: {w}x{h}')

# In isolated_arm_r_tight:
# The bottom edge (y around h-1 to h-6) was resting on the desk.
# Let's inspect the bottom edge:
# We want the paw to have a rounded bottom edge with 3 claws,
# just like the left paw in bongo_dog_crop!
