from PIL import Image
import numpy as np

def clean_rgba(img):
    arr = np.array(img).copy()
    # If alpha < 120 -> transparent (0, 0, 0, 0)
    # If alpha >= 120 -> opaque (r, g, b, 255)
    trans_mask = arr[:, :, 3] < 120
    arr[trans_mask] = [0, 0, 0, 0]
    opaque_mask = arr[:, :, 3] >= 120
    arr[opaque_mask, 3] = 255
    return Image.fromarray(arr)

f_l = clean_rgba(Image.open('docs_for_me/bongo_final_tap_l.png'))
f_r = clean_rgba(Image.open('docs_for_me/bongo_final_tap_r.png'))
f_rest = clean_rgba(Image.open('docs_for_me/bongo_final_rest.png'))

f_l.save('docs_for_me/clean_f_l.png')

# Create 3x clean frames
f_l_3x = f_l.resize((f_l.width * 3, f_l.height * 3), Image.NEAREST)
f_r_3x = f_r.resize((f_r.width * 3, f_r.height * 3), Image.NEAREST)
f_rest_3x = f_rest.resize((f_rest.width * 3, f_rest.height * 3), Image.NEAREST)

frames = []
durations = []
for _ in range(10):
    frames.append(f_l_3x)
    durations.append(150)
    frames.append(f_r_3x)
    durations.append(150)
frames.append(f_rest_3x)
durations.append(2000)

# Convert each frame to 'P' mode with exact transparency color
p_frames = []
for f in frames:
    # Convert RGBA with binary alpha to P mode
    alpha = f.split()[3]
    # Mask rgb
    p_img = f.convert('RGB').convert('P', palette=Image.ADAPTIVE, colors=255)
    # Set transparent color
    mask = Image.eval(alpha, lambda a: 255 if a == 0 else 0)
    p_img.paste(255, mask)
    p_img.info['transparency'] = 255
    p_frames.append(p_img)

p_frames[0].save(
    'docs_for_me/bongo_clean_trans_3x.gif',
    save_all=True,
    append_images=p_frames[1:],
    duration=durations,
    loop=0,
    disposal=2,
    transparency=255
)
print('bongo_clean_trans_3x.gif saved!')
