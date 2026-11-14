# -*- coding: utf-8 -*-
# Decode raw OLED framebuffers (8x128) into scaled OLED-style PNGs.
import os, glob
from PIL import Image, ImageDraw

SRC = "out"
OUT = "png"
SCALE = 6
PIX = (215, 240, 255)
BG = (4, 6, 9)
BORDER = (30, 40, 55)

os.makedirs(OUT, exist_ok=True)

def decode_png(bin_path, scale=SCALE):
    data = open(bin_path, "rb").read()
    img = Image.new("RGB", (128*scale, 64*scale), BG)
    px = img.load()
    for page in range(8):
        for col in range(128):
            byte = data[page*128 + col]
            for bit in range(8):
                if byte & (1 << bit):
                    y = page*8 + bit
                    for dy in range(scale):
                        for dx in range(scale):
                            px[col*scale+dx, y*scale+dy] = PIX
    ImageDraw.Draw(img).rectangle([0, 0, img.width-1, img.height-1], outline=BORDER)
    return img

for f in sorted(glob.glob(os.path.join(SRC, "*.bin"))):
    name = os.path.splitext(os.path.basename(f))[0]
    decode_png(f).save(os.path.join(OUT, name + ".png"))
    print("saved", name + ".png")
print("done")
