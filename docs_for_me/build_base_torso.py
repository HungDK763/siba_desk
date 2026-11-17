from PIL import Image
import numpy as np

# Load bongo_dog_crop.png (360x268)
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
arr = np.array(orig)
H, W, _ = arr.shape

# Let's inspect the dog mask from background
# Background is dark space:
# Blue channel > Red channel and Blue > 50, or very dark (all < 35)
is_bg = ((arr[:,:,2] > arr[:,:,0] + 15) & (arr[:,:,2] > 40)) | \
        ((arr[:,:,0] < 30) & (arr[:,:,1] < 30) & (arr[:,:,2] < 35))

# Desk at bottom:
# Dark table bar across bottom: y > 235 and r < 60 and g < 60 and b < 60
is_desk = (np.arange(H)[:, None] >= 238)

dog_mask = ~is_bg & ~is_desk

clean_dog = np.zeros((H, W, 4), dtype=np.uint8)
clean_dog[dog_mask] = arr[dog_mask]

Image.fromarray(clean_dog).save('docs_for_me/clean_dog_raw.png')
print('clean_dog_raw.png saved!')
