from PIL import Image

img = Image.open('/home/dksoren/aau_workspace/asr_flightstack/src/asr_gcs/images/sun.png').convert('RGBA')
r, g, b, a = img.split()

# Invert only RGB channels, keep alpha (transparency) untouched
r = r.point(lambda x: 255 - x)
g = g.point(lambda x: 255 - x)
b = b.point(lambda x: 255 - x)

inverted = Image.merge('RGBA', (r, g, b, a))
inverted.save('sun_white.png')