from PIL import Image
import numpy as np

orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGB')
arr = np.array(orig)

# Left paw in bongo_dog_crop is at x in [135, 215], y in [185, 245]
# Let's crop it:
paw_l_crop = orig.crop((135, 185, 215, 245))
paw_l_crop.save('docs_for_me/real_paw_l_crop.png')
print('real_paw_l_crop.png saved, size:', paw_l_crop.size)
