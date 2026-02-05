#!/bin/bash
#
# test_visual_comparison.sh
# Compare visual output of different aspect ratio modes
#
# This script encodes the same image with all three modes and
# generates a report showing the differences in how each mode
# handles the image transformation.
#
# Usage: ./test_visual_comparison.sh <input_image> [output_dir]
#

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( dirname "$SCRIPT_DIR" )"
BIN="$PROJECT_ROOT/bin/pisstvpp2"
TEST_RESULTS_DIR="$PROJECT_ROOT/tests/test_results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Check arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <input_image> [output_dir]"
    echo ""
    echo "Examples:"
    echo "  $0 photo.jpg"
    echo "  $0 photo.jpg ./test_output"
    exit 1
fi

INPUT_IMAGE="$1"
TEST_DIR="${2:-$TEST_RESULTS_DIR}"

# Check if binary exists
if [ ! -f "$BIN" ]; then
    echo -e "${RED}[ERROR] Binary not found: $BIN${NC}"
    exit 1
fi

# Check if input image exists
if [ ! -f "$INPUT_IMAGE" ]; then
    echo -e "${RED}[ERROR] Input image not found: $INPUT_IMAGE${NC}"
    exit 1
fi

mkdir -p "$TEST_DIR"

INPUT_BASENAME=$(basename "$INPUT_IMAGE" | sed 's/\.[^.]*$//')
INPUT_DIR=$(dirname "$INPUT_IMAGE")
INPUT_NAME=$(basename "$INPUT_IMAGE")
INPUT_COPY="$TEST_DIR/${INPUT_NAME}"

echo "=========================================="
echo "Visual Comparison Test"
echo "=========================================="
echo "Input:           $INPUT_NAME"
echo "Results stored:  $TEST_DIR"
echo ""

# Copy input image to test results folder
if [ "$INPUT_IMAGE" != "$INPUT_COPY" ]; then
    cp "$INPUT_IMAGE" "$INPUT_COPY"
    echo "✓ Copied input image to: $INPUT_COPY"
fi
echo ""

# Get original image dimensions
original_dims=$(python3 << PYTHON_EOF
from PIL import Image
img = Image.open('$INPUT_IMAGE')
print(f"{img.width}x{img.height}")
PYTHON_EOF
)

echo "Original dimensions: $original_dims"
echo ""

# Test each mode
modes=("center" "pad" "stretch")

echo "Encoding with each aspect ratio mode..."
echo "========================================"

for mode in "${modes[@]}"; do
    output_file="$TEST_DIR/${INPUT_BASENAME}_${mode}.wav"
    
    echo -e "${BLUE}Mode: $mode${NC}"
    echo "  Input:  $INPUT_IMAGE ($original_dims)"
    echo "  Output: $output_file"
    
    if "$BIN" -i "$INPUT_IMAGE" -a "$mode" -o "$output_file" -v 2>&1 | \
       grep -E "(Current:|Target:|Mode:|Result:)" | sed 's/^/    /'; then
        
        if [ -f "$output_file" ]; then
            size=$(du -h "$output_file" | cut -f1)
            samples=$(stat -f%z "$output_file" 2>/dev/null || stat -c%s "$output_file" 2>/dev/null)
            seconds=$(echo "scale=2; $samples / 22050 / 4" | bc 2>/dev/null || echo "unknown")
            echo -e "  ${GREEN}✓ Success${NC}: $size (~${seconds}s)"
        else
            echo -e "  ${RED}✗ Failed${NC}: Output not created"
        fi
    else
        echo -e "  ${RED}✗ Error${NC}: Encoding failed"
    fi
    echo ""
done

# Generate comparison report
echo "=========================================="
echo "Comparison Summary"
echo "=========================================="
echo ""
echo "Mode behavior with $original_dims input:"
echo ""

python3 << 'PYTHON_EOF'
print("CENTER mode:")
print("  - Crops from center to exact 320x256 (for Martin 1)")
print("  - Discards edges, preserves center content")
print("  - Best for: Large images where center is important")
print("")
print("PAD mode:")
print("  - Adds black padding around original image")
print("  - Preserves original aspect ratio and content scale")
print("  - Best for: Small images that need to reach target size")
print("")
print("STRETCH mode:")
print("  - Direct resize to target dimensions (320x256)")
print("  - May distort image if aspect ratios don't match")
print("  - Best for: When exact dimensions matter more than aspect ratio")
print("")
PYTHON_EOF

echo "Output files created in: $TEST_DIR"
echo "  - ${INPUT_BASENAME}_center.wav"
echo "  - ${INPUT_BASENAME}_pad.wav"
echo "  - ${INPUT_BASENAME}_stretch.wav"
echo ""
echo "To decode and analyze the SSTV output, use an SSTV decoder"
echo "or compare the audio characteristics using audio analysis tools."
