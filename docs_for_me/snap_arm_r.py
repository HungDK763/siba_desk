from PIL import Image
import numpy as np

forearm = Image.open('docs_for_me/split_forearm_r.png').convert('RGBA')
paw = Image.open('docs_for_me/split_paw_r.png').convert('RGBA')

# Rotate forearm up 50 deg CCW
c1 = Image.new('RGBA', (300, 300), (0,0,0,0))
c1.paste(forearm, (100, 100), forearm)
rot_f = c1.rotate(50, resample=Image.Resampling.BICUBIC, center=(105, 105))
rf_arr = np.array(rot_f)
rf_arr[:,:,3] = np.where(rf_arr[:,:,3] > 110, 255, 0)
rot_f_clean = Image.fromarray(rf_arr)

# Test various paw offsets to find the perfect seamless snap
for dy_off in [-45, -40, -35, -30]:
    for dx_off in [-45, -40, -35, -30]:
        c = Image.new('RGBA', (300, 300), (0,0,0,0))
        c.paste(rot_f_clean, (0, 0), rot_f_clean)
        # Paw at (170 + dx_off, 128 + dy_off)
        c.paste(paw, (170 + dx_off, 128 + dy_off), paw)
        crop = c.crop(c.getbbox())
        crop.save(f'docs_for_me/snap_{dx_off}_{dy_off}.png')

print('Test snaps generated!')
