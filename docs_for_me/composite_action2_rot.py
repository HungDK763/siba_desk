from PIL import Image
import numpy as np

# Load base dog with right arm down
# Right arm down is present in bongo_dog_crop.
orig = Image.open('docs_for_me/bongo_dog_crop.png').convert('RGBA')
oarr = np.array(orig)

# In oarr:
# Clean space background:
for y in range(len(oarr)):
    for x in range(len(oarr[0])):
        r, g, b, a = oarr[y, x]
        if (b > int(r) + 8 and b > 32) or (r < 40 and g < 45 and b < 85):
            oarr[y, x] = [0,0,0,0]
        # remove top-right motion lines
        if y < 120 and x > 270 and r > 140 and g > 140 and b > 140:
            oarr[y, x] = [0,0,0,0]

# Erase resting left paw (x in [130, 225], y in [195, 255])
# while preserving red scarf
for y in range(195, 255):
    for x in range(130, 225):
        r, g, b, a = oarr[y, x]
        if r > 140 and g < 70 and b < 70 and y < 216:
            continue
        oarr[y, x] = [0,0,0,0]

# Fill white chest fur behind erased paw down to desk
for y in range(195, 255):
    for x in range(140, 225):
        if oarr[y, x, 3] == 0:
            if y < 248:
                oarr[y, x] = [255, 254, 250, 255] if y < 240 else [238, 222, 204, 255]
            elif y in (248, 249, 250):
                oarr[y, x] = [225, 210, 195, 255]
            else:
                oarr[y, x] = [58, 16, 6, 255] # desk line

base2 = Image.fromarray(oarr)

# Load rot_left_55
rot_l = Image.open('docs_for_me/rot_left_55.png').convert('RGBA')
# Clean stray line on the right of rot_left_55
rarr = np.array(rot_l)
# Any pixel with x > 55 and y < 50 where r < 60 is a stray line
for y in range(len(rarr)):
    for x in range(len(rarr[0])):
        if x > 55 and y < 55:
            rarr[y, x] = [0,0,0,0]
rot_l_clean = Image.fromarray(rarr)

# In bongo_dog_crop, shoulder was at x=135, y=180
# In rot_left_55, shoulder is top-left
# Paste at (125, 165)
comp2 = base2.copy()
comp2.paste(rot_l_clean, (125, 160), rot_l_clean)

# Re-apply red scarf knot
for y in range(165, 215):
    for x in range(130, 180):
        mr, mg, mb, ma = orig.getpixel((x, y))
        if mr > 140 and mg < 70 and mb < 70:
            comp2.putpixel((x, y), (mr, mg, mb, 255))

crop2 = comp2.crop(comp2.getbbox())
crop2.save('docs_for_me/preview_action2_clean.png')
print('preview_action2_clean.png saved, size:', crop2.size)
