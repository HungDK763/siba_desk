from PIL import Image
import numpy as np

forearm = Image.open('docs_for_me/split_forearm_r.png').convert('RGBA')
paw = Image.open('docs_for_me/split_paw_r.png').convert('RGBA')

# In forearm: top-left is shoulder.
# Let's rotate forearm by 60 degrees CCW
fw, fh = forearm.size
canvas_f = Image.new('RGBA', (200, 200), (0,0,0,0))
# Put shoulder at (50, 50)
canvas_f.paste(forearm, (50, 50), forearm)
rot_forearm = canvas_f.rotate(60, resample=Image.Resampling.BICUBIC, center=(50, 50))

# Clean alpha
arr_f = np.array(rot_forearm)
arr_f[:,:,3] = np.where(arr_f[:,:,3] > 120, 255, 0)
rot_forearm_clean = Image.fromarray(arr_f)

# The end of the forearm (wrist) moves from (50 + fw, 50 + fh) along the 60 deg arc.
# Let's see its bbox:
bbox_f = rot_forearm_clean.getbbox()
print('Forearm bbox:', bbox_f)

# Now we can attach the paw at the wrist!
# Let's tilt the paw slightly (e.g. 10..20 deg) or keep it facing down
comp_arm = Image.new('RGBA', (300, 300), (0,0,0,0))
comp_arm.paste(rot_forearm_clean, (50, 50), rot_forearm_clean)

# Wrist location in comp_arm is near the top right of the rotated forearm:
# Let's place the paw there
# Paw size is around 60x60
# Let's rotate paw slightly so claws angle downwards/forward
rot_paw = paw.rotate(15, resample=Image.Resampling.BICUBIC)
arr_p = np.array(rot_paw)
arr_p[:,:,3] = np.where(arr_p[:,:,3] > 120, 255, 0)
rot_paw_clean = Image.fromarray(arr_p)

# Paste paw at wrist
comp_arm.paste(rot_paw_clean, (50 + bbox_f[2] - 25, 50 + bbox_f[1] - 5), rot_paw_clean)
comp_arm_crop = comp_arm.crop(comp_arm.getbbox())
comp_arm_crop.save('docs_for_me/natural_arm_r_60.png')
print('natural_arm_r_60.png saved!')
