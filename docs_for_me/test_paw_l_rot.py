from PIL import Image

arm_l = Image.open('docs_for_me/arm_l_tight.png').convert('RGBA')
w, h = arm_l.size

# Pivot is at top left (px=12, py=12)
px, py = 12, 12

for deg in [-45, -55, -60, 45, 55, 60]:
    canvas = Image.new('RGBA', (300, 300), (0, 0, 0, 0))
    cx, cy = 150, 150
    canvas.paste(arm_l, (cx - px, cy - py), arm_l)
    rot = canvas.rotate(deg, resample=Image.Resampling.BICUBIC, center=(cx, cy))
    bbox = rot.getbbox()
    rot.crop(bbox).save(f'docs_for_me/paw_l_rot_{deg}.png')

print('Left paw rotations generated')
