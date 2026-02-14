#!/bin/bash

# PiSSTVpp Test Suite (Bash)
# Quick tests for basic functionality without Python dependency

set -e  # Exit on error

TEST_DIR="test_outputs"
EXECUTABLE="${1:-./../bin}/pisstvpp2"
FAILED=0
PASSED=0

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
run_test() {
    local test_name="$1"
    local expected_status="$2"
    shift 2
    local cmd=("$@")
    
    echo -n "Testing: $test_name ... "
    
    if output=$("${cmd[@]}" 2>&1); then
        local status=$?
    else
        local status=$?
        local output=$!
    fi
    
    if [ $status -eq $expected_status ]; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}FAIL${NC} (exit code: $status, expected: $expected_status)"
        ((FAILED++))
        return 1
    fi
}

check_output_file() {
    local file="$1"
    if [ -f "$file" ] && [ -s "$file" ]; then
        return 0
    else
        return 1
    fi
}

# Setup
mkdir -p "$TEST_DIR"
cd "$TEST_DIR" || exit 1

echo "========================================================================"
echo "PiSSTVpp Quick Test Suite (Bash Version)"
echo "========================================================================"
echo "Executable: $EXECUTABLE"
echo "Output directory: $TEST_DIR"
echo ""

# Find a test image
TEST_IMAGE="../images/test_320x240.png"
if [ ! -f "$TEST_IMAGE" ]; then
    # Try other image names
    for img in ../images/test*.{jpg,jpeg,png,bmp}; do
        if [ -f "$img" ]; then
            TEST_IMAGE="$img"
            break
        fi
    done
fi

if [ ! -f "$TEST_IMAGE" ]; then
    echo "ERROR: No test image found"
    exit 1
fi

echo "Using test image: $TEST_IMAGE"
echo ""

# =========================================================================
# BASIC FUNCTIONALITY TESTS
# =========================================================================
echo "========== BASIC FUNCTIONALITY =========="

# Test missing input
echo -n "Testing: Missing input file (-i required) ... "
if "$EXECUTABLE" 2>&1 | grep -q -i "required\|error"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Test help
echo -n "Testing: Help output (-h) ... "
if "$EXECUTABLE" -h 2>&1 | grep -q -i "usage\|options\|protocol"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =========================================================================
# PROTOCOL TESTS
# =========================================================================
echo ""
echo "========== PROTOCOL TESTS =========="

for protocol in m1 m2 s1 s2 sdx r36 r72; do
    output="test_protocol_${protocol}.wav"
    echo -n "Testing protocol: $protocol ... "
    if "$EXECUTABLE" -i "$TEST_IMAGE" -p "$protocol" -o "$output" -v >/dev/null 2>&1; then
        if check_output_file "$output"; then
            size=$(stat -f%z "$output" 2>/dev/null || stat -c%s "$output" 2>/dev/null || echo "unknown")
            echo -e "${GREEN}PASS${NC} ($size bytes)"
            ((PASSED++))
        else
            echo -e "${RED}FAIL${NC} (no output file)"
            ((FAILED++))
        fi
    else
        echo -e "${RED}FAIL${NC} (execution error)"
        ((FAILED++))
    fi
done

# =========================================================================
# SAMPLE RATE TESTS
# =========================================================================
echo ""
echo "========== SAMPLE RATE TESTS =========="

for rate in 8000 11025 22050 44100 48000; do
    output="test_rate_${rate}.wav"
    echo -n "Testing sample rate: $rate Hz ... "
    if "$EXECUTABLE" -i "$TEST_IMAGE" -r "$rate" -o "$output" >/dev/null 2>&1; then
        if check_output_file "$output"; then
            echo -e "${GREEN}PASS${NC}"
            ((PASSED++))
        else
            echo -e "${RED}FAIL${NC}"
            ((FAILED++))
        fi
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
done

# =========================================================================
# FORMAT TESTS
# =========================================================================
echo ""
echo "========== AUDIO FORMAT TESTS =========="

for fmt in wav aiff; do
    output="test_format.${fmt}"
    echo -n "Testing format: $fmt ... "
    if "$EXECUTABLE" -i "$TEST_IMAGE" -f "$fmt" -o "$output" >/dev/null 2>&1; then
        if check_output_file "$output"; then
            echo -e "${GREEN}PASS${NC}"
            ((PASSED++))
        else
            echo -e "${RED}FAIL${NC}"
            ((FAILED++))
        fi
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
done

# =========================================================================
# ASPECT RATIO TESTS
# =========================================================================
echo ""
echo "========== ASPECT RATIO TESTS =========="

for aspect in "4:3" "fit" "stretch"; do
    output="test_aspect_${aspect//:/_}.wav"
    echo -n "Testing aspect mode: $aspect ... "
    if "$EXECUTABLE" -i "$TEST_IMAGE" -a "$aspect" -o "$output" >/dev/null 2>&1; then
        if check_output_file "$output"; then
            echo -e "${GREEN}PASS${NC}"
            ((PASSED++))
        else
            echo -e "${RED}FAIL${NC}"
            ((FAILED++))
        fi
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
done

# =========================================================================
# CW SIGNATURE TESTS
# =========================================================================
echo ""
echo "========== CW SIGNATURE TESTS =========="

# Standard CW test
output="test_cw_standard.wav"
echo -n "Testing CW signature: K4ABC @ 15 WPM ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -p r36 -C "K4ABC" -W 15 -T 800 -o "$output" >/dev/null 2>&1; then
    if check_output_file "$output"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Portable callsign
output="test_cw_portable.wav"
echo -n "Testing CW signature: N0CALL/MOBILE @ 20 WPM ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -p r36 -C "N0CALL/MOBILE" -W 20 -T 1000 -o "$output" >/dev/null 2>&1; then
    if check_output_file "$output"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Maritime callsign
output="test_cw_maritime.wav"
echo -n "Testing CW signature: K5ABC/MM @ 10 WPM ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -p r36 -C "K5ABC/MM" -W 10 -T 1200 -o "$output" >/dev/null 2>&1; then
    if check_output_file "$output"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =========================================================================
# ERROR HANDLING TESTS
# =========================================================================
echo ""
echo "========== ERROR HANDLING TESTS =========="

# Invalid protocol
echo -n "Testing error: Invalid protocol rejection ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -p invalid_proto 2>&1 | grep -q -i "error\|unrecognized"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Invalid sample rate
echo -n "Testing error: Out-of-range sample rate rejection ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -r 4000 2>&1 | grep -q -i "error\|must be"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# Invalid aspect mode
echo -n "Testing error: Invalid aspect mode rejection ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -a invalid 2>&1 | grep -q -i "error\|aspect"; then
    echo -e "${GREEN}PASS${NC}"
    ((PASSED++))
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =========================================================================
# COMBINED OPTION TESTS
# =========================================================================
echo ""
echo "========== COMBINED OPTION TESTS =========="

output="test_combined_r72_44k_aiff.aiff"
echo -n "Testing combined: Robot 72 + 44kHz + AIFF ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -p r72 -r 44100 -f aiff -o "$output" >/dev/null 2>&1; then
    if check_output_file "$output"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

output="test_combined_m2_stretch.wav"
echo -n "Testing combined: Martin 2 + stretch aspect ... "
if "$EXECUTABLE" -i "$TEST_IMAGE" -p m2 -a stretch -o "$output" >/dev/null 2>&1; then
    if check_output_file "$output"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
    fi
else
    echo -e "${RED}FAIL${NC}"
    ((FAILED++))
fi

# =========================================================================
# SUMMARY
# =========================================================================
echo ""
echo "========================================================================"
echo "Test Summary"
echo "========================================================================"
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo "Total:  $((PASSED + FAILED))"
echo "========================================================================"

# List generated files
echo ""
echo "Generated test files:"
ls -lh *.wav *.aiff 2>/dev/null | head -20 || echo "  (no files found)"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
