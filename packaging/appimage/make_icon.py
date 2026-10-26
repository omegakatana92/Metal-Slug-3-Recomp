#!/usr/bin/env python3
"""Draw an original 256x256 icon for the Metal Slug 3 recompilation.

No SNK artwork is used: the mark is a simple green field, a yellow diagonal
band, and block letters drawn from a tiny bitmap font.
"""
import os, struct, zlib

W = H = 256
px = bytearray(W * H * 3)

def put(x, y, r, g, b):
    if 0 <= x < W and 0 <= y < H:
        i = (y * W + x) * 3
        px[i] = r; px[i+1] = g; px[i+2] = b

# background: deep green with a subtle vertical gradient
for y in range(H):
    for x in range(W):
        put(x, y, 6 + y // 16, 38 + y // 8, 18 + y // 24)

# yellow diagonal band
for y in range(H):
    for x in range(W):
        if 48 <= (x + y) % 256 <= 78:
            put(x, y, 238, 190, 30)

# black drop panel behind the letters
for y in range(74, 190):
    for x in range(26, 230):
        put(x, y, 10, 14, 12)

# 5x7 block font, scaled
FONT = {
    'M': ["10001","11011","10101","10101","10001","10001","10001"],
    'S': ["01111","10000","10000","01110","00001","00001","11110"],
    '3': ["11110","00001","00001","01110","00001","00001","11110"],
}
text = "MS3"
scale = 13
tw = (len(text) * 5 + (len(text) - 1)) * scale
tx = (W - tw) // 2
ty = (H - 7 * scale) // 2
for ci, ch in enumerate(text):
    for gy, row in enumerate(FONT[ch]):
        for gx, bit in enumerate(row):
            if bit == '1':
                for yy in range(scale):
                    for xx in range(scale):
                        put(tx + (ci * 6 + gx) * scale + xx, ty + gy * scale + yy, 245, 245, 245)

# red roundel accent at lower right
cx, cy, rad = 206, 206, 26
for y in range(cy - rad, cy + rad + 1):
    for x in range(cx - rad, cx + rad + 1):
        d = (x - cx) ** 2 + (y - cy) ** 2
        if d <= rad * rad:
            put(x, y, 200, 32, 28 if d > (rad - 5) ** 2 else 240)

raw = bytearray()
for y in range(H):
    raw.append(0)
    raw += px[y*W*3:(y+1)*W*3]

def chunk(t, d):
    return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

out = b'\x89PNG\r\n\x1a\n'
out += chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0))
out += chunk(b'IDAT', zlib.compress(bytes(raw), 9))
out += chunk(b'IEND', b'')
path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "metal-slug-3.png")
open(path, 'wb').write(out)
print(path)
