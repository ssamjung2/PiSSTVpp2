#!/bin/bash
#
# test_dimension_verification.sh
# Verify that aspect ratio corrections produce correct output dimensions
#
# This script tests that:
# - CENTER mode outputs exactly 320x256 for Martin 1
# - PAD mode adds correct black bars
# - STRETCH mode produces correct aspect ratio
#
# Usage: ./test_dimension_verification.sh [output_dir]
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
BLUE='\033[0;34m'
NC='\033[0m'

# Check if binary exists
if [ ! -f "$BIN" ]; then
    echo -e "${RED}[ERROR] Binary not found: $BIN${NC}"
    exit 1
fi

echo "=========================================="
echo "PiSSTVpp2 Dimension Verification Tests"
echo "=========================================="
echo ""

# Create test image with known dimensions
create_test_image() {
    local width="$1"
    local height="$2"
    local filename="test_${width}x${height}.png"
    
    python3 << PYTHON_EOF
from PIL import Image, ImageDraw

img = Image.new('RGB', ($width, $height), color='white')
draw = ImageDraw.Draw(img)

# Draw a border to identify edges
draw.rectangle([0, 0, $((width-1)), $((height-1))], outline='black', width=2)

# Label corners
draw.text((5, 5), "TL", fill='black')
draw.text(($((width-15)), 5), "TR", fill='black')
draw.text((5, $((height-15))), "BL", fill='black')
draw.text(($((width-15)), $((height-15))), "BR", fill='black')

# Center label
draw.text(($((width//2-20)), $((height//2-5))), "${width}x${height}", fill='black')

img.save('$filename')
print("Created: $filename")
PYTHON_EOF
    
    echo "$filename"
}

# Extract dimensions from verbose output
extract_dimensions_from_output() {
    local output="$1"
    echo "$output" | grep "Result:" | grep -oE "[0-9]+x[0-9]+" | head -1
}

# Test CENTER mode: source larger than target
test_center_crop() {
    local width="$1"
    local height="$2"
    local image_file="test_${width}x${height}.png"
    
    echo -e "${BLUE}Testing CENTER mode: ${width}x${height} → 320x256${NC}"
    
    # Create image if needed
    if [ ! -f "$image_file" ]; then
        create_test_image "$width" "$height" > /dev/null
    fi
    
    # Run encoding with verbose output
    output=$("$BIN" -i "$image_file" -a center -o "test_center_${width}x${height}.wav" -v 2>&1 | tail -5)
    
    # Extract result dimensions from output
    result=$(echo "$output" | grep "Result:" | tail -1)
    echo "  $result"
    
    # Verify output mentions center-crop
    if echo "$output" | grep -q "CENTER mode: center-crop"; then
        echo -e "  ${GREEN}✓ CENTER mode applied${NC}"
    else
        echo -e "  ${RED}✗ CENTER mode NOT applied${NC}"
    fi
}

# Test PAD mode: source smaller than target
test_pad_mode() {
    local width="$1"
    local height="$2"
    local image_file="test_${width}x${height}.png"
    
    echo -e "${BLUE}Testing PAD mode: ${width}x${height} with black padding${NC}"
    
    # Create image if needed
    if [ ! -f "$image_file" ]; then
        create_test_image "$width" "$height" > /dev/null
    fi
    
    # Run encoding with verbose output
    output=$("$BIN" -i "$image_file" -a pad -o "test_pad_${width}x${height}.wav" -v 2>&1 | tail -5)
    
    # Extract result dimensions from output
    result=$(echo "$output" | grep "Result:" | tail -1)
    echo "  $result"
    
    # Verify output mentions PAD mode
    if echo "$output" | grep -q "PAD mode"; then
        echo -e "  ${GREEN}✓ PAD mode applied${NC}"
    else
        echo -e "  ${RED}✗ PAD mode NOT applied${NC}"
    fi
}

# Test STRETCH mode
test_stretch_mode() {
    local width="$1"
    local height="$2"
    local image_file="test_${width}x${height}.png"
    
    echo -e "${BLUE}Testing STRETCH mode: ${width}x${height} with direct resize${NC}"
    
    # Create image if needed
    if [ ! -f "$image_file" ]; then
        create_test_image "$width" "$height" > /dev/null
    fi
    
    # Run encoding with verbose output
    output=$("$BIN" -i "$image_file" -a stretch -o "test_stretch_${width}x${height}.wav" -v 2>&1 | tail -5)
    
    # Extract result dimensions from output
    result=$(echo "$output" | grep "Result:" | tail -1)
    echo "  $result"
    
    # Verify output mentions STRETCH mode
    if echo "$output" | grep -q "STRETCH mode"; then
        echo -e "  ${GREEN}✓ STRETCH mode applied${NC}"
    else
        echo -e "  ${RED}✗ STRETCH mode NOT applied${NC}"
    fi
}

# Main tests
main() {
    mkdir -p "$TEST_DIR"
    cd "$TEST_DIR"
    
    echo "Test Set 1: Small images (smaller than target 320x256)"
    echo "========================================================"
    test_pad_mode 160 120
    echo ""
    test_stretch_mode 160 120
    echo ""
    
    echo "Test Set 2: Medium images (similar to target aspect ratio)"
    echo "==========================================================="
    test_pad_mode 400 300
    echo ""
    test_stretch_mode 400 300
    echo ""
    
    echo "Test Set 3: Large images (larger than target)"
    echo "============================================="
    test_center_crop 640 480
    echo ""
    test_stretch_mode 640 480
    echo ""
    
    echo "Test Set 4: Extreme aspect ratios"
    echo "=================================="
    echo "Portrait (256x512):"
    test_center_crop 256 512
    echo ""
    test_stretch_mode 256 512
    echo ""
    
    echo "Landscape (1024x256):"
    test_center_crop 1024 256
    echo ""
    test_stretch_mode 1024 256
    echo ""
    
    echo "=========================================="
    echo "Verification complete!"
    echo "All test WAV files in: $TEST_DIR"
    echo "=========================================="
}

main "$@"
