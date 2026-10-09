#!/usr/bin/env python3
"""skin_diff.py — compare a rendered skin screen with a reference screenshot.

    tools/skin_diff.py rendered.png reference.png -o diff.png [--json]

Writes a side-by-side sheet (rendered | reference | heat map of differences)
and prints a score so an edit -> render -> compare loop can tell whether a
skin change moved closer to the reference:

    mae      mean absolute error per channel, 0..255 (lower is better)
    ssim     structural similarity on luma, 0..1 (higher is better)
    palette  the 6 dominant colours of each image (to copy into "palette")
    regions  a 4x4 grid of per-cell MAE, to say *where* it differs

The reference is resized to the rendered size first (render the skin at the
reference's resolution for the best signal: r4l-skin-render --size WxH).
Needs Pillow (pip install pillow).
"""
import argparse
import json
import sys

try:
    from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageOps, ImageStat
except ImportError:
    sys.exit("skin_diff.py needs Pillow: pip install pillow")


def ssim(a, b):
    a = ImageOps.grayscale(a)
    b = ImageOps.grayscale(b)
    a = a.resize((a.width // 2 or 1, a.height // 2 or 1))
    b = b.resize(a.size)
    pa, pb = list(a.tobytes()), list(b.tobytes())
    n = len(pa)
    ma, mb = sum(pa) / n, sum(pb) / n
    va = sum((x - ma) ** 2 for x in pa) / n
    vb = sum((x - mb) ** 2 for x in pb) / n
    cov = sum((x - ma) * (y - mb) for x, y in zip(pa, pb)) / n
    c1, c2 = (0.01 * 255) ** 2, (0.03 * 255) ** 2
    return ((2 * ma * mb + c1) * (2 * cov + c2)) / ((ma * ma + mb * mb + c1) * (va + vb + c2))


def palette(img, k=6):
    q = img.convert("RGB").resize((160, 90)).quantize(colors=k, method=Image.Quantize.MEDIANCUT)
    pal = q.getpalette()[: k * 3]
    counts = sorted(q.getcolors(), reverse=True)
    return ["#%02x%02x%02x" % tuple(pal[i * 3: i * 3 + 3]) for _, i in counts]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("rendered")
    ap.add_argument("reference")
    ap.add_argument("-o", "--out", default="skin_diff.png")
    ap.add_argument("--json", action="store_true", help="print the scores as JSON")
    args = ap.parse_args()

    r = Image.open(args.rendered).convert("RGB")
    ref = Image.open(args.reference).convert("RGB").resize(r.size, Image.LANCZOS)
    diff = ImageChops.difference(r, ref)
    mae = sum(ImageStat.Stat(diff).mean) / 3
    heat = ImageOps.autocontrast(ImageOps.grayscale(diff)).filter(ImageFilter.GaussianBlur(2))
    heat = ImageOps.colorize(heat, black="#000000", white="#ff3040", mid="#ffb000")

    cells = []
    gw, gh = r.width // 4, r.height // 4
    for gy in range(4):
        row = []
        for gx in range(4):
            box = (gx * gw, gy * gh, (gx + 1) * gw, (gy + 1) * gh)
            row.append(round(sum(ImageStat.Stat(diff.crop(box)).mean) / 3, 1))
        cells.append(row)

    sheet = Image.new("RGB", (r.width * 3, r.height + 28), "#101010")
    for i, (im, label) in enumerate([(r, "rendered"), (ref, "reference"), (heat, "difference")]):
        sheet.paste(im, (i * r.width, 28))
        ImageDraw.Draw(sheet).text((i * r.width + 8, 8), label, fill="#ffffff")
    sheet.save(args.out)

    result = {
        "mae": round(mae, 2),
        "ssim": round(ssim(r, ref), 4),
        "palette_rendered": palette(r),
        "palette_reference": palette(ref),
        "regions_mae_4x4": cells,
        "sheet": args.out,
    }
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print("mae %.2f  ssim %.4f  -> %s" % (result["mae"], result["ssim"], args.out))
        print("reference palette:", " ".join(result["palette_reference"]))
        worst = max((v, x, y) for y, row in enumerate(cells) for x, v in enumerate(row))
        print("worst cell (col %d, row %d of 4x4): mae %.1f" % (worst[1], worst[2], worst[0]))


if __name__ == "__main__":
    main()
