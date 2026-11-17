from PIL import Image
import numpy as np

# Load split forearm and split paw
forearm = Image.open('docs_for_me/split_forearm_r.png').convert('RGBA')
paw = Image.open('docs_for_me/split_paw_r.png').convert('RGBA')

# In forearm: size is (44, 78) approximately
fw, fh = forearm.size
# Top-left is shoulder. Bottom-right is wrist.
# Let's rotate forearm up by 50 deg CCW
c1 = Image.new('RGBA', (300, 300), (0,0,0,0))
c1.paste(forearm, (100, 100), forearm)
# Shoulder is at (105, 105)
rot_f = c1.rotate(50, resample=Image.Resampling.BICUBIC, center=(105, 105))
rf_arr = np.array(rot_f)
rf_arr[:,:,3] = np.where(rf_arr[:,:,3] > 110, 255, 0)
rot_f_clean = Image.fromarray(rf_arr)
bbox_f = rot_f_clean.getbbox()
print('Forearm bbox in c1:', bbox_f)

# The wrist of the rotated forearm is at the upper-right of the forearm
# Let's find the wrist pixels:
nz_y, nz_x = np.where(rf_arr[:,:,3] > 0)
# Top-rightmost points of rotated forearm:
# Wrist center is roughly at (nz_x.max() - 5, nz_y[nz_x > nz_x.max() - 10].mean())
wrist_x = int(nz_x.max() - 5)
wrist_y = int(nz_y[nz_x > nz_x.max() - 10].mean())
print(f'Wrist center: ({wrist_x}, {wrist_y})')

# Now Paw:
# In split_paw_r, the paw has its claws at the bottom.
# When attached to the wrist, we want the paw to hang DOWNWARDS!
# In split_paw_r, claws are at the bottom, rounded top is at the top.
# So if we keep paw upright or tilt it slightly (-15 to -30 deg),
# the claws are facing directly DOWN towards the desk!
c_arm = Image.new('RGBA', (400, 400), (0,0,0,0))
c_arm.paste(rot_f_clean, (50, 50), rot_f_clean)

# Paste paw at wrist
pw, ph = paw.size
c_arm.paste(paw, (50 + wrist_x - 15, 50 + wrist_y - 10), paw)

c_arm_crop = c_arm.crop(c_arm.getbbox())
c_arm_crop.save('docs_for_me/bent_arm_r_hd.png')
print('bent_arm_r_hd.png saved!')
