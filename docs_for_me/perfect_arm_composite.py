from PIL import Image, ImageDraw
import numpy as np

# Load perfect_dog_master (347x268)
dog = Image.open('docs_for_me/perfect_dog_master.png').convert('RGBA')
darr = np.array(dog)
H, W, _ = darr.shape

# Clean left border (x < 6 is stray line)
darr[:, :6] = [0,0,0,0]
# Clean bottom (y > 238 is table)
darr[239:, :] = [0,0,0,0]

dog_clean = Image.fromarray(darr)
bbox = dog_clean.getbbox()
print('Cleaned dog bbox:', bbox)
# Save cleaned dog
dog_clean.crop(bbox).save('docs_for_me/dog_base_clean.png')
print('dog_base_clean.png saved, size:', dog_clean.crop(bbox).size)
