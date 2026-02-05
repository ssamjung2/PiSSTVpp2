#!/bin/bash
#
# test_aspect_modes.sh
# Test script for aspect ratio correction modes in PiSSTVpp2
#
# Tests three modes with various image sizes:
# - CENTER: Center-crop to exact target dimensions (320x256 for Martin 1)
# - PAD:    Add black padding while preserving original aspect ratio
# - STRETCH: Direct resize allowing aspect ratio distortion
#
# Usage: ./test_aspect_modes.sh [output_dir]
#

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( dirname "$SCRIPT_DIR" )"
BIN="$PROJECT_ROOT/bin/pisstvpp2"
TEST_RESULTS_DIR="$PROJECT_ROOT/tests/test_results"
TEST_DIR="${1:-$TEST_RESULTS_DIR}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if binary exists
if [ ! -f "$BIN" ]; then
    echo -e "${RED}[ERROR] Binary not found: $BIN${NC}"
    exit 1
fi

echo "=========================================="
echo "PiSSTVpp2 Aspect Ratio Mode Tests"
echo "=========================================="
echo "Binary: $BIN"
echo "Results directory: $TEST_DIR"
echo ""

# Create test images if they don't exist
create_test_images() {
    echo "Creating test images in $TEST_DIR..."
    python3 << PYTHON_EOF
from PIL import Image, ImageDraw
import os

os.chdir('$TEST_DIR')

# Test 1: Small image (160x120) - will be padded by PAD mode
print("  Creating small_160x120.png (160x120)...")
img = Image.new('RGB', (160, 120), color='white')
draw = ImageDraw.Draw(img)
draw.rectangle([20, 20, 140, 100], fill='red', outline='black', width=2)
draw.text((70, 55), "SMALL", fill='black')
draw.text((5, 5), "TL", fill='black')
draw.text((150, 5), "TR", fill='black')
draw.text((5, 110), "BL", fill='black')
draw.text((140, 110), "BR", fill='black')
img.save('small_160x120.png')

# Test 2: Medium image (400x300) - similar aspect to target but larger
print("  Creating medium_400x300.png (400x300)...")
img = Image.new('RGB', (400, 300), color='lightblue')
draw = ImageDraw.Draw(img)
draw.rectangle([50, 50, 350, 250], fill='green', outline='black', width=2)
draw.text((175, 140), "MEDIUM", fill='black')
draw.text((5, 5), "TL", fill='black')
draw.text((390, 5), "TR", fill='black')
draw.text((5, 290), "BL", fill='black')
draw.text((380, 290), "BR", fill='black')
img.save('medium_400x300.png')

# Test 3: Large image (640x480) - will be center-cropped by CENTER mode
print("  Creating large_640x480.png (640x480)...")
img = Image.new('RGB', (640, 480), color='white')
draw = ImageDraw.Draw(img)
# Quadrants with different colors to test center-crop
draw.rectangle([0, 0, 320, 240], fill='red', outline='black', width=2)
draw.rectangle([320, 0, 640, 240], fill='green', outline='black', width=2)
draw.rectangle([0, 240, 320, 480], fill='blue', outline='black', width=2)
draw.rectangle([320, 240, 640, 480], fill='yellow', outline='black', width=2)
draw.text((10, 10), "TL", fill='white')
draw.text((620, 10), "TR", fill='black')
draw.text((10, 460), "BL", fill='white')
draw.text((600, 460), "BR", fill='black')
img.save('large_640x480.png')

# Test 4: Tall image (256x512) - portrait orientation
print("  Creating portrait_256x512.png (256x512)...")
img = Image.new('RGB', (256, 512), color='lightyellow')
draw = ImageDraw.Draw(img)
draw.rectangle([30, 50, 226, 462], fill='purple', outline='black', width=2)
draw.text((100, 250), "TALL", fill='white')
draw.text((5, 5), "TL", fill='black')
draw.text((245, 5), "TR", fill='black')
draw.text((5, 502), "BL", fill='black')
draw.text((235, 502), "BR", fill='black')
img.save('portrait_256x512.png')

# Test 5: Wide image (1024x256) - landscape orientation
print("  Creating landscape_1024x256.png (1024x256)...")
img = Image.new('RGB', (1024, 256), color='lightcyan')
draw = ImageDraw.Draw(img)
draw.rectangle([100, 30, 924, 226], fill='orange', outline='black', width=2)
draw.text((450, 125), "WIDE", fill='black')
draw.text((5, 5), "TL", fill='black')
draw.text((1010, 5), "TR", fill='black')
draw.text((5, 246), "BL", fill='black')
draw.text((1000, 246), "BR", fill='black')
img.save('landscape_1024x256.png')

print("All test images created successfully!")

PYTHON_EOF

    # Replace placeholder with actual directory
    sed -i "s|TEST_DIR_PLACEHOLDER|$TEST_DIR|g" /tmp/test_create.py 2>/dev/null || python3 << PYTHON_EOF
from PIL import Image, ImageDraw
import os

os.chdir('$TEST_DIR')

# Test 1: Small image (160x120)
img = Image.new('RGB', (160, 120), color='white')
draw = ImageDraw.Draw(img)
draw.rectangle([20, 20, 140, 100], fill='red', outline='black', width=2)
draw.text((70, 55), "SMALL", fill='black')
draw.text((5, 5), "TL", fill='black')
draw.text((150, 5), "TR", fill='black')
draw.text((5, 110), "BL", fill='black')
draw.text((140, 110), "BR", fill='black')
img.save('small_160x120.png')

# Test 2: Medium image (400x300)
img = Image.new('RGB', (400, 300), color='lightblue')
draw = ImageDraw.Draw(img)
draw.rectangle([50, 50, 350, 250], fill='green', outline='black', width=2)
draw.text((175, 140), "MEDIUM", fill='black')
draw.text((5, 5), "TL", fill='black')
draw.text((390, 5), "TR", fill='black')
draw.text((5, 290), "BL", fill='black')
draw.text((380, 290), "BR", fill='black')
img.save('medium_400x300.png')

# Test 3: Large image (640x480)
img = Image.new('RGB', (640, 480), color='white')
draw = ImageDraw.Draw(img)
draw.rectangle([0, 0, 320, 240], fill='red', outline='black', width=2)
draw.rectangle([320, 0, 640, 240], fill='green', outline='black', width=2)
draw.rectangle([0, 240, 320, 480], fill='blue', outline='black', width=2)
draw.rectangle([320, 240, 640, 480], fill='yellow', outline='black', width=2)
draw.text((10, 10), "TL", fill='white')
draw.text((620, 10), "TR", fill='black')
draw.text((10, 460), "BL", fill='white')
draw.text((600, 460), "BR", fill='black')
img.save('large_640x480.png')

# Test 4: Tall image (256x512)
img = Image.new('RGB', (256, 512), color='lightyellow')
draw = ImageDraw.Draw(img)
draw.rectangle([30, 50, 226, 462], fill='purple', outline='black', width=2)
draw.text((100, 250), "TALL", fill='white')
draw.text((5, 5), "TL", fill='black')
draw.text((245, 5), "TR", fill='black')
draw.text((5, 502), "BL", fill='black')
draw.text((235, 502), "BR", fill='black')
img.save('portrait_256x512.png')

# Test 5: Wide image (1024x256)
img = Image.new('RGB', (1024, 256), color='lightcyan')
draw = ImageDraw.Draw(img)
draw.rectangle([100, 30, 924, 226], fill='orange', outline='black', width=2)
draw.text((450, 125), "WIDE", fill='black')
draw.text((5, 5), "TL", fill='black')
draw.text((1010, 5), "TR", fill='black')
draw.text((5, 246), "BL", fill='black')
draw.text((1000, 246), "BR", fill='black')
img.save('landscape_1024x256.png')

print("All test images created successfully!")
PYTHON_EOF
}

# Test a single combination
test_combination() {
    local image="$1"
    local mode="$2"
    local output="$TEST_DIR/${image%.*}_${mode}.wav"
    
    echo "Testing: $image with mode=$mode"
    if "$BIN" -i "$TEST_DIR/$image" -a "$mode" -o "$output" -v 2>&1 | tail -3; then
        if [ -f "$output" ]; then
            size=$(du -h "$output" | cut -f1)
            echo -e "${GREEN}  ✓ PASS${NC}: Created $output ($size)"
        else
            echo -e "${RED}  ✗ FAIL${NC}: Output file not created"
            return 1
        fi
    else
        echo -e "${RED}  ✗ FAIL${NC}: Encoding failed"
        return 1
    fi
    echo ""
}

# Run all tests
main() {
    mkdir -p "$TEST_DIR"
    
    # Create test images
    create_test_images
    echo ""
    
    # Test matrix: different images with different modes
    test_images=(
        "small_160x120.png"
        "medium_400x300.png"
        "large_640x480.png"
        "portrait_256x512.png"
        "landscape_1024x256.png"
    )
    
    test_modes=("center" "pad" "stretch")
    
    pass_count=0
    fail_count=0
    
    for image in "${test_images[@]}"; do
        echo "====== Testing image: $image ======"
        for mode in "${test_modes[@]}"; do
            if test_combination "$image" "$mode"; then
                ((pass_count++))
            else
                ((fail_count++))
            fi
        done
        echo ""
    done
    
    # Summary
    echo "=========================================="
    echo "Test Summary"
    echo "=========================================="
    echo -e "${GREEN}Passed: $pass_count${NC}"
    echo -e "${RED}Failed: $fail_count${NC}"
    total=$((pass_count + fail_count))
    echo "Total:  $total"
    
    if [ $fail_count -eq 0 ]; then
        echo -e "\n${GREEN}All tests passed!${NC}"
        return 0
    else
        echo -e "\n${RED}Some tests failed!${NC}"
        return 1
    fi
}

main "$@"
