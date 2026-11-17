from PIL import Image
import numpy as np

# Load bongo_cleaned_102x76.png (or bongo_exact_rest.png)
base = Image.open('docs_for_me/bongo_cleaned_102x76.png').convert('RGBA')
W, H = base.size # 102, 76

# Pad to 104x78 to match firmware exact canvas (.w = 104, .h = 78)
base_104 = Image.new('RGBA', (104, 78), (0,0,0,0))
# Center slightly: paste at (1, 1)
base_104.paste(base, (1, 1), base)

arr_master = np.array(base_104)

# Define exact pixel colors from original art:
OUTLINE   = [52, 18, 12, 255]      # #34120c Dark outline
ORANGE_L  = [255, 188, 92, 255]    # Top arm highlight
ORANGE    = [254, 160, 50, 255]    # Vibrant fur
ORANGE_M  = [240, 135, 32, 255]    # Mid orange
ORANGE_D  = [210, 100, 20, 255]    # Deep shadow fur
WHITE     = [255, 254, 250, 255]   # Clean paw white
WHITE_SH  = [235, 222, 210, 255]   # Under-paw shadow
CLAW      = [48, 12, 5, 255]       # Dark claw slit
CLEAR     = [0, 0, 0, 0]

print('Base prepared, shape:', arr_master.shape)
