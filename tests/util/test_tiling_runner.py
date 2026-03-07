#!/usr/bin/env python3
"""
Tiling Unit Test Runner
=======================

Compiles and runs tests/util/test_tiling.c against the SlowFrame image module.

Tests covered:
  Group 1 — CRC-8/CCITT algorithm
  Group 2 — SFTileInfo struct layout / SF_TILE_HEADER_ROWS constant
  Group 3 — Tile crop-box math (grid/overlap combinator)
  Group 4 — image_apply_tile_header pixel verification (sync + data rows)
  Group 5 — image_crop_region dimension verification
  Group 6 — Error-path handling (no image loaded)
  Group 7 — Full metadata round-trip

Usage:
    python3 tests/util/test_tiling_runner.py
    python3 tests/util/test_tiling_runner.py --run-only   # skip recompile

Author: SlowFrame Contributors
Version: 2.1.0
Date: February 2026
"""

import subprocess
import sys
import os
import argparse

# ---------------------------------------------------------------------------
# Project paths
# ---------------------------------------------------------------------------
PROJECT_ROOT = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC_DIR      = os.path.join(PROJECT_ROOT, "src")
BIN_DIR      = os.path.join(PROJECT_ROOT, "bin")
TEST_BINARY  = os.path.join(BIN_DIR, "test_tiling")
TEST_SOURCE  = os.path.join(PROJECT_ROOT, "tests", "util", "test_tiling.c")


def pkg_flags(packages: str) -> list[str]:
    """Return a list of compiler or linker flags from pkg-config."""
    result = subprocess.run(
        ["pkg-config", "--cflags", "--libs", packages],
        capture_output=True, text=True)
    if result.returncode != 0:
        print(f"❌ pkg-config failed for '{packages}'")
        print(result.stderr)
        sys.exit(1)
    return result.stdout.split()


def compile_test() -> bool:
    print("=" * 70)
    print("COMPILING: test_tiling (C unit tests)")
    print("=" * 70)

    vips_cflags  = subprocess.run(
        ["pkg-config", "--cflags", "vips", "glib-2.0", "gobject-2.0"],
        capture_output=True, text=True).stdout.split()
    vips_libs    = subprocess.run(
        ["pkg-config", "--libs",   "vips", "glib-2.0", "gobject-2.0"],
        capture_output=True, text=True).stdout.split()

    # Include the same extra Homebrew paths the main Makefile uses on macOS
    extra_inc   = ["-I/opt/homebrew/include"] if sys.platform == "darwin" else []
    extra_lib   = ["-L/opt/homebrew/lib"]     if sys.platform == "darwin" else []

    img_dir  = os.path.join(SRC_DIR, "image")
    util_dir = os.path.join(SRC_DIR, "util")

    source_files = [
        TEST_SOURCE,
        os.path.join(SRC_DIR,    "slowframe_image.c"),
        os.path.join(img_dir,    "image_aspect.c"),
        os.path.join(img_dir,    "image_loader.c"),
        os.path.join(img_dir,    "image_processor.c"),
        os.path.join(img_dir,    "image_text_overlay.c"),
        os.path.join(SRC_DIR,    "overlay_spec.c"),
        os.path.join(util_dir,   "error.c"),
    ]

    cmd = (
        ["cc", "-std=c11", "-Wall", "-Wextra", "-O0", "-g"]
        + vips_cflags
        + extra_inc
        + [
            f"-I{SRC_DIR}/include",
            f"-I{SRC_DIR}",
            f"-I{img_dir}",
            f"-I{util_dir}",
        ]
        + source_files
        + vips_libs
        + extra_lib
        + ["-lm", "-o", TEST_BINARY]
    )

    print("$ " + " ".join(cmd[:4]) + " ... [truncated]")
    result = subprocess.run(cmd, capture_output=True, text=True,
                            cwd=PROJECT_ROOT)
    if result.returncode != 0:
        print("❌ COMPILATION FAILED")
        if result.stderr:
            print("STDERR:")
            print(result.stderr)
        if result.stdout:
            print("STDOUT:")
            print(result.stdout)
        return False

    size_kb = os.path.getsize(TEST_BINARY) // 1024
    print(f"✅ Compilation successful  ({size_kb} KB → {TEST_BINARY})")
    return True


def run_tests() -> bool:
    print("\n" + "=" * 70)
    print("RUNNING: test_tiling")
    print("=" * 70 + "\n")

    if not os.path.exists(TEST_BINARY):
        print(f"❌ Binary not found: {TEST_BINARY}")
        return False

    result = subprocess.run([TEST_BINARY], capture_output=True, text=True,
                            cwd=PROJECT_ROOT)
    if result.stdout:
        print(result.stdout)
    if result.stderr:
        print("STDERR:", result.stderr, file=sys.stderr)

    return result.returncode == 0


def main() -> None:
    parser = argparse.ArgumentParser(description="Tiling unit test runner")
    parser.add_argument("--run-only", action="store_true",
                        help="Skip compilation, just run the existing binary")
    args = parser.parse_args()

    if not args.run_only:
        ok = compile_test()
        if not ok:
            sys.exit(1)

    ok = run_tests()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
