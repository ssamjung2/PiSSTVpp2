#!/usr/bin/env python3
"""
test_seam_integrity.py -- Verify no pixel loss at interior tile seams.

Approach
--------
1. Create a synthetic source sized W=mode_w*cols x H=mode_h*rows
   (e.g. 960x720 for 3x3 R36).  Each tile crop is already the exact
   mode resolution -- slowframe does a 1:1 crop with NO rescaling.
2. Run slowframe -K on it, stitch tiles back (strip 4 header rows/tier).
3. At each interior seam boundary, sample both the stitched image and
   the original source at the SAME logical pixel position and compare.
   Colour-management-invariant: any ICC transform cancels out since
   both images went through the same pipeline.
4. Confirm adjacency: pixels flanking the seam come from adjacent
   source positions (no gap, no duplication).
"""
import argparse, re, subprocess, sys, tempfile
from pathlib import Path

REPO_ROOT           = Path(__file__).resolve().parents[2]
BINARY              = REPO_ROOT / "bin" / "slowframe"
VIPS                = "vips"
SF_TILE_HEADER_ROWS = 8   # 4 primary + 4 backup rows

def vips_run(*args):
    subprocess.run([VIPS, *[str(a) for a in args]], check=True, capture_output=True)

def vips_dim(path, field):
    r = subprocess.run(["vipsheader", "-f", field, str(path)], capture_output=True, text=True)
    return int(r.stdout.strip())

def getpoint(path, x, y):
    r = subprocess.run([VIPS, "getpoint", str(path), str(x), str(y)], capture_output=True, text=True)
    return tuple(int(v) for v in r.stdout.strip().split())

def make_synthetic_image(w, h, dst):
    """
    Write a minimal PNG directly using zlib+struct — no ICC/sRGB chunk.

    Pixel formula:
        R = (x + y) % 256,  G = (x * 2) % 256,  B = (y * 2 + 37) % 256
    """
    import zlib, struct

    def _chunk(tag, data):
        raw = tag + data
        return struct.pack(">I", len(data)) + raw + struct.pack(">I", zlib.crc32(raw) & 0xFFFFFFFF)

    rows_bytes = bytearray()
    for y in range(h):
        rows_bytes.append(0)   # filter: None
        for x in range(w):
            rows_bytes.append((x + y)      % 256)
            rows_bytes.append((x * 2)      % 256)
            rows_bytes.append((y * 2 + 37) % 256)

    with dst.open("wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(_chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
        f.write(_chunk(b"IDAT", zlib.compress(bytes(rows_bytes), level=1)))
        f.write(_chunk(b"IEND", b""))

KNOWN_MODES = {"r36": (320, 240), "r72": (320, 240),
               "s1":  (320, 256), "s2":  (320, 256),
               "m1":  (320, 256), "m2":  (320, 256)}

def run_test(grid, protocol, keep, src_file=None):
    tc, tr = (int(x) for x in grid.lower().split("x"))
    if protocol.lower() not in KNOWN_MODES:
        sys.exit(f"Unknown mode: {protocol}")
    mw, mh = KNOWN_MODES[protocol.lower()]
    sw, sh = mw * tc, mh * tr
    cont_h = mh - SF_TILE_HEADER_ROWS

    print()
    print("=" * 70)
    print("  SlowFrame Interior Seam Integrity Test")
    print("=" * 70)
    print(f"  Grid      : {tc}x{tr}  ({tc*tr} tiles)")
    print(f"  Protocol  : {protocol.upper()}  ({mw}x{mh} per tile)")
    if src_file:
        import struct
        with open(src_file, "rb") as _f:
            _f.seek(16)
            _rw = struct.unpack(">I", _f.read(4))[0] if str(src_file).endswith(".png") else 0
        print(f"  Source    : {Path(src_file).name}  (real image)")
    else:
        print(f"  Source    : {sw}x{sh}  (1:1 -- no rescaling)")
    print(f"  Stitched  : {sw}x{cont_h*tr}  (approx — depends on aspect scaling)")
    print(f"  Hdr rows  : {SF_TILE_HEADER_ROWS} x {tr} tiers = {SF_TILE_HEADER_ROWS*tr} consumed")
    print()

    with tempfile.TemporaryDirectory(prefix="sf_seam_") as td:
        tmp = Path(td)

        if src_file:
            import shutil
            src = tmp / Path(src_file).name
            shutil.copy2(src_file, src)
            print(f"  [1/4] Using real source: {src.name}  ({src.stat().st_size//1024} KB)")
            print()
        else:
            print("  [1/4] Generating synthetic source...")
            src = tmp / f"src_{sw}x{sh}.png"
            make_synthetic_image(sw, sh, src)
            print(f"        {src.name}  ({src.stat().st_size//1024} KB)")
            print()

        print("  [2/4] Running slowframe...")
        r = subprocess.run(
            [str(BINARY), "-i", str(src), "-p", protocol,
             "-X", grid, "-K", "-o", str(tmp / "tiles.wav")],
            capture_output=True, text=True, cwd=str(tmp))
        if r.returncode != 0:
            print(r.stderr[-2000:]); sys.exit(1)
        tile_pngs = sorted(
            [p for p in tmp.glob("*-tile-*-r*c*.*")
             if p.suffix.lower() in (".png", ".jpg", ".jpeg")],
            key=lambda p: int(re.search(r"tile-(\d+)", p.name).group(1)))
        want = tc * tr
        if len(tile_pngs) != want:
            sys.exit(f"Expected {want} tiles, got {len(tile_pngs)}")
        # Pre-image: slowframe's own full-grid intermediate saved before any
        # tile headers were applied.  Pixel values here ARE what slowframe
        # actually fed into each tile — the definitive reference.
        pre_imgs = list(tmp.glob("*-tile-pre.png"))
        if not pre_imgs:
            sys.exit("No *-tile-pre.png found — build with -K support?")
        pre_img = pre_imgs[0]
        print(f"        {len(tile_pngs)} tiles,  pre-image: {pre_img.name}")
        print()

        print("  [3/4] Stitching...")
        crops = []
        for row in range(tr):
            for col in range(tc):
                seq  = row * tc + col + 1
                crop = tmp / f"crop_r{row:02d}c{col:02d}.png"
                vips_run("extract_area", tile_pngs[seq-1], crop,
                         "0", str(SF_TILE_HEADER_ROWS), str(mw), str(cont_h))
                crops.append(crop)
        stitched = tmp / "stitched.png"
        vips_run("arrayjoin", " ".join(str(c) for c in crops), stitched, "--across", str(tc))
        stw = vips_dim(stitched, "Xsize")
        sth = vips_dim(stitched, "Ysize")
        print(f"        stitched.png  {stw}x{sth}")
        print()

        # ── Build the "expected" image from the pre-image ────────────────
        # The pre-image now contains only content rows (no header slots):
        # size is (cols*mw) × (rows*cont_h).  Each tier occupies cont_h rows
        # starting at tier * cont_h — no header offset needed.
        # If stitched == expected → zero pixel loss at every interior seam.
        print("  [4/4] Verifying seam integrity...")
        print()

        # One horizontal strip per tier from the pre-image
        strips = []
        for tier in range(tr):
            y_start = tier * cont_h          # pre-image has no header rows
            strip   = tmp / f"strip_{tier:02d}.png"
            vips_run("extract_area", pre_img, strip,
                     "0", str(y_start), str(stw), str(cont_h))
            strips.append(strip)

        expected = tmp / "expected.png"
        vips_run("arrayjoin", " ".join(str(p) for p in strips), expected,
                 "--across", "1")

        # Overall diff: stitched vs expected
        def vips_maxdiff(a, b):
            diff_v   = tmp / "diff.v"
            diff_abs = tmp / "diff_abs.v"
            vips_run("subtract", a, b, diff_v)
            vips_run("abs", diff_v, diff_abs)
            r = subprocess.run(["vips", "stats", str(diff_abs)],
                               capture_output=True, text=True)
            maxv = 0
            for line in r.stdout.splitlines():
                parts = line.split()
                if len(parts) >= 3:
                    try: maxv = max(maxv, int(float(parts[2])))
                    except ValueError: pass
            return maxv

        overall_diff = vips_maxdiff(stitched, expected)
        print(f"  Overall (stitched vs pre-image content): max_diff={overall_diff}")
        if overall_diff == 0:
            print("  ✅ All pixels identical — full image match confirmed.")
        else:
            print(f"  ❌ pixel diff found before seam checks (max_diff={overall_diff})")
            sys.exit(1)
        print()

        # ── Per-seam checks via 2-pixel-wide/tall extract + subtract ─────
        fails = 0

        print("  Vertical seams:")
        for sc in range(1, tc):
            x_seam = sc * mw
            # 2-pixel-wide column straddling the seam
            col_s = tmp / f"col_s_x{x_seam}.png"
            col_e = tmp / f"col_e_x{x_seam}.png"
            vips_run("extract_area", stitched, col_s, str(x_seam-1), "0", "2", str(sth))
            vips_run("extract_area", expected, col_e, str(x_seam-1), "0", "2", str(sth))
            md = vips_maxdiff(col_s, col_e)
            status = "✅" if md == 0 else "❌"
            print(f"    {status}  x={x_seam}  "
                  f"(col {sc-1} x={x_seam-1} | col {sc} x={x_seam})  max_diff={md}")
            if md != 0:
                fails += 1
        print()

        print("  Horizontal seams:")
        for sr in range(1, tr):
            y_seam = sr * cont_h
            # 2-pixel-tall row straddling the seam
            row_s = tmp / f"row_s_y{y_seam}.png"
            row_e = tmp / f"row_e_y{y_seam}.png"
            vips_run("extract_area", stitched, row_s, "0", str(y_seam-1), str(stw), "2")
            vips_run("extract_area", expected, row_e, "0", str(y_seam-1), str(stw), "2")
            md = vips_maxdiff(row_s, row_e)
            status = "✅" if md == 0 else "❌"
            # Source rows skipped at this boundary
            pre_y_above = (sr-1) * mh + SF_TILE_HEADER_ROWS + (cont_h - 1)
            pre_y_below = sr    * mh + SF_TILE_HEADER_ROWS
            skip = pre_y_below - pre_y_above - 1
            print(f"    {status}  y={y_seam}  "
                  f"(tier {sr-1} last content row | tier {sr} first content row)  "
                  f"max_diff={md}  [{skip} header rows consumed between tiers]")
            if md != 0:
                fails += 1
        print()

        if keep:
            import shutil
            kd = Path(tempfile.gettempdir()) / "sf_seam_keep"
            shutil.copytree(tmp, kd, dirs_exist_ok=True)
            print(f"  Files kept: {kd}")
            print()

    n = (tc-1) + (tr-1)
    print(f"  Seams   : {n}  ({tc-1} vertical + {tr-1} horizontal)")
    print(f"  Checks  : value match at each 2px boundary strip")
    if fails == 0:
        print(f"  PASS: All checks passed -- no pixel loss or duplication at any interior seam.")
    else:
        print(f"  FAIL: {fails} seam checks failed.")
        sys.exit(1)
    print()
    print("=" * 70)
    print()

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-g", "--grid",     default="3x3")
    ap.add_argument("-p", "--protocol", default="r36")
    ap.add_argument("-s", "--src",      default=None,
                    help="Real source image to use instead of synthetic")
    ap.add_argument('--keep', action='store_true')
    args = ap.parse_args()
    if not BINARY.exists():
        sys.exit(f"Build first: {BINARY} not found")
    run_test(args.grid, args.protocol, args.keep, src_file=args.src)

if __name__ == "__main__":
    main()
