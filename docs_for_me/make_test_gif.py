from PIL import Image

f_tap_l = Image.open('docs_for_me/frame_tap_l_v1.png').convert('RGBA')
f_tap_r = Image.open('docs_for_me/frame_tap_r_v1.png').convert('RGBA')
f_rest = Image.open('docs_for_me/frame_rest_v1.png').convert('RGBA')

frames = []
durations = []

# 3 seconds tapping: alternating tap_l and tap_r (150ms each)
# 20 frames * 150ms = 3000ms
for i in range(10):
    frames.append(f_tap_l)
    durations.append(150)
    frames.append(f_tap_r)
    durations.append(150)

# 2 seconds rest: 1 frame of 2000ms
frames.append(f_rest)
durations.append(2000)

# Save with disposal=2 so frames do not stack/ghost!
frames[0].save(
    'docs_for_me/bongo_motion_test_transparent.gif',
    save_all=True,
    append_images=frames[1:],
    duration=durations,
    loop=0,
    disposal=2
)

# Also create a version rendered on dark background (with desk / quota panel)
# Dark launcher background: (15, 17, 26)
BG_COLOR = (15, 17, 26, 255)
DESK_COLOR = (30, 36, 48, 255) # Color of the mini quota panel top
DESK_LINE = (55, 65, 85, 255)

bg_frames = []
for f in frames:
    # Canvas with desk: 110 x 86 (padded)
    canvas = Image.new('RGBA', (110, 86), BG_COLOR)
    # Draw desk / quota panel below y=78
    from PIL import ImageDraw
    d = ImageDraw.Draw(canvas)
    # Desk bar: y from 79 to 86
    d.rectangle([0, 79, 110, 86], fill=DESK_COLOR)
    d.line([(0, 78), (110, 78)], fill=DESK_LINE, width=1)
    # Paste dog so paws touch y=78 (dog y=4..79)
    canvas.paste(f, (4, 4), f)
    bg_frames.append(canvas)

bg_frames[0].save(
    'docs_for_me/bongo_motion_test_desk.gif',
    save_all=True,
    append_images=bg_frames[1:],
    duration=durations,
    loop=0
)

# Also create x3 scaled versions so user can see every pixel clearly on desktop/phone!
scale_frames = [canvas.resize((canvas.width * 3, canvas.height * 3), Image.NEAREST) for canvas in bg_frames]
scale_frames[0].save(
    'docs_for_me/bongo_motion_test_desk_3x.gif',
    save_all=True,
    append_images=scale_frames[1:],
    duration=durations,
    loop=0
)

print('GIFs created successfully!')
