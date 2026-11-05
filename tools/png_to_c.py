#!/usr/bin/env python3
"""Convert a non-interlaced, 8-bit PNG into a freestanding C pixel array.

Supported PNG color types: grayscale, RGB, indexed, grayscale+alpha, RGBA.
The output pixels are packed as 0x00RRGGBB for the framebuffer.
"""

import argparse
import struct
import zlib

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MAX_WIDTH = 640
MAX_HEIGHT = 480


def decode_png(path):
    data = open(path, "rb").read()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("input is not a PNG file")

    offset = len(PNG_SIGNATURE)
    width = height = color_type = bit_depth = interlace = None
    compressed = bytearray()
    palette = None
    transparency = None
    seen_end = False

    while offset < len(data):
        if offset + 12 > len(data):
            raise ValueError("truncated PNG chunk")
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset + 4:offset + 8]
        start = offset + 8
        end = start + length
        if end + 4 > len(data):
            raise ValueError("truncated PNG chunk data")
        payload = data[start:end]
        expected_crc = struct.unpack_from(">I", data, end)[0]
        if zlib.crc32(kind + payload) & 0xFFFFFFFF != expected_crc:
            raise ValueError("PNG chunk CRC mismatch")

        if kind == b"IHDR":
            if length != 13 or width is not None:
                raise ValueError("invalid IHDR")
            width, height, bit_depth, color_type, compression, filtering, interlace = struct.unpack(">IIBBBBB", payload)
            if not width or not height or width > MAX_WIDTH or height > MAX_HEIGHT:
                raise ValueError("image dimensions must be between 1x1 and 640x480")
            if bit_depth != 8 or color_type not in (0, 2, 3, 4, 6):
                raise ValueError("only 8-bit grayscale, RGB, indexed, grayscale-alpha, and RGBA PNGs are supported")
            if compression != 0 or filtering != 0 or interlace != 0:
                raise ValueError("unsupported PNG compression, filter method, or interlacing")
        elif kind == b"PLTE":
            if length == 0 or length % 3 or length > 768:
                raise ValueError("invalid PNG palette")
            palette = [tuple(payload[i:i + 3]) for i in range(0, length, 3)]
        elif kind == b"tRNS":
            transparency = payload
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            seen_end = True
            break
        elif kind[0] & 0x20 == 0:
            raise ValueError("unsupported critical PNG chunk: " + kind.decode("ascii", "replace"))
        offset = end + 4

    if width is None or not seen_end or not compressed:
        raise ValueError("PNG is missing required image data")

    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[color_type]
    stride = width * channels
    raw = zlib.decompress(compressed)
    if len(raw) != height * (stride + 1):
        raise ValueError("PNG decompressed data has an unexpected size")

    rows = []
    previous = bytearray(stride)
    cursor = 0
    for _ in range(height):
        filter_type = raw[cursor]
        cursor += 1
        current = bytearray(raw[cursor:cursor + stride])
        cursor += stride
        for i in range(stride):
            left = current[i - channels] if i >= channels else 0
            above = previous[i]
            upper_left = previous[i - channels] if i >= channels else 0
            if filter_type == 1:
                current[i] = (current[i] + left) & 0xFF
            elif filter_type == 2:
                current[i] = (current[i] + above) & 0xFF
            elif filter_type == 3:
                current[i] = (current[i] + ((left + above) // 2)) & 0xFF
            elif filter_type == 4:
                p = left + above - upper_left
                pa, pb, pc = abs(p - left), abs(p - above), abs(p - upper_left)
                predictor = left if pa <= pb and pa <= pc else above if pb <= pc else upper_left
                current[i] = (current[i] + predictor) & 0xFF
            elif filter_type != 0:
                raise ValueError("invalid PNG row filter")
        rows.append(current)
        previous = current

    pixels = []
    for row in rows:
        for x in range(width):
            pixel = row[x * channels:(x + 1) * channels]
            alpha = 255
            if color_type == 0:
                red = green = blue = pixel[0]
                if transparency is not None and len(transparency) == 2 and pixel[0] == transparency[1]:
                    alpha = 0
            elif color_type == 2:
                red, green, blue = pixel
                if transparency is not None and len(transparency) == 6:
                    tr, tg, tb = struct.unpack(">HHH", transparency)
                    if (red, green, blue) == (tr, tg, tb):
                        alpha = 0
            elif color_type == 3:
                if palette is None or pixel[0] >= len(palette):
                    raise ValueError("indexed PNG has a missing or invalid palette")
                red, green, blue = palette[pixel[0]]
                if transparency is not None and pixel[0] < len(transparency):
                    alpha = transparency[pixel[0]]
            elif color_type == 4:
                red = green = blue = pixel[0]
                alpha = pixel[1]
            else:
                red, green, blue, alpha = pixel
            # The Pi framebuffer is configured with an opaque display; composite
            # transparent pixels against black before creating the RGB pixel.
            red = red * alpha // 255
            green = green * alpha // 255
            blue = blue * alpha // 255
            pixels.append((red << 16) | (green << 8) | blue)
    return width, height, pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_png")
    parser.add_argument("output_c", nargs="?", default="src/png_image.c")
    args = parser.parse_args()

    width, height, pixels = decode_png(args.input_png)
    with open(args.output_c, "w", encoding="ascii") as output:
        output.write('#include "png_image.h"\n\n')
        output.write("const uint32_t png_image_width = %du;\n" % width)
        output.write("const uint32_t png_image_height = %du;\n" % height)
        output.write("const uint32_t png_image_pixels[%d] = {\n" % len(pixels))
        for start in range(0, len(pixels), 8):
            output.write("    " + ", ".join("0x%06Xu" % value for value in pixels[start:start + 8]))
            output.write(",\n" if start + 8 < len(pixels) else "\n")
        output.write("};\n")
    print("Wrote %dx%d PNG pixels to %s" % (width, height, args.output_c))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, zlib.error) as error:
        raise SystemExit("png_to_c.py: %s" % error)
