from PIL import Image, ImageDraw
import numpy as np

# Let's inspect bongo_base_102x76.png
base = Image.open('docs_for_me/bongo_base_102x76.png').convert('RGBA')
W, H = base.size
print('Base size:', W, H)

# Let's check colors in bongo_base_102x76:
# Outline: (55, 20, 10, 255) to (70, 25, 15, 255)
# Orange fur: (254, 160, 50, 255), shadow: (220, 110, 30, 255), highlight: (255, 185, 90, 255)
# White fur: (255, 255, 255, 255), shadow: (230, 225, 215, 255)
# Scarf: (220, 35, 35, 255), shadow: (150, 20, 20, 255)
