#!/bin/bash

echo "Running CW Audio Fix Test Suite..."
echo "=================================="

# Get the path to the executable
EXECUTABLE="../bin/pisstvpp2"

# Test 1: Standard callsign at 13 WPM
echo -e "\n[Test 1/3] Standard callsign - K4ABC at 13 WPM"
"$EXECUTABLE" -i images/test_320x240.png -p r36 -C "K4ABC" -W 13 -T 800 -o test_outputs/test1.wav 2>&1 | tail -3

# Test 2: Portable callsign at 20 WPM  
echo -e "\n[Test 2/3] Portable callsign - N0CALL/MOBILE at 20 WPM"
"$EXECUTABLE" -i images/test_320x240.png -p r36 -C "N0CALL/MOBILE" -W 20 -T 1000 -o test_outputs/test2.wav 2>&1 | tail -3

# Test 3: Maritime callsign at 10 WPM with alternate tone
echo -e "\n[Test 3/3] Maritime callsign - K5ABC/MM at 10 WPM, 1200 Hz"
"$EXECUTABLE" -i images/test_320x240.png -p r36 -C "K5ABC/MM" -W 10 -T 1200 -o test_outputs/test3.wav 2>&1 | tail -3

echo -e "\n=================================="
echo "Test suite completed!"
ls -lh test_outputs/test1.wav test_outputs/test2.wav test_outputs/test3.wav 2>/dev/null || echo "No test files generated"
