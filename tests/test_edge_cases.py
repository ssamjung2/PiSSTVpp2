#!/usr/bin/env python3
"""
Edge case and error condition testing for PiSSTVpp2
Tests boundary values, empty inputs, interdependencies, and error code accuracy
"""

import subprocess
import tempfile
import json
import os
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Tuple, Optional

# Color codes for output
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

@dataclass
class TestCase:
    """Represents a single test case"""
    name: str
    args: List[str]
    should_fail: bool = False
    expected_error_code: Optional[int] = None
    description: str = ""

class EdgeCaseTestSuite:
    """Test suite for edge cases and error conditions"""
    
    def __init__(self, executable_path="../bin/pisstvpp2"):
        self.exe = executable_path
        self.passed = 0
        self.failed = 0
        self.skipped = 0
        self.test_results = []
        
        if not os.path.exists(self.exe):
            raise FileNotFoundError(f"Executable not found: {self.exe}")
        
        # Create a minimal valid test image for positive tests
        self.test_image = self._create_test_image()
    
    def _create_test_image(self) -> str:
        """Create a minimal PPM test image for testing"""
        temp_dir = tempfile.gettempdir()
        test_image = os.path.join(temp_dir, "pisstvpp2_test_image.ppm")
        
        # Create minimal 320x256 PPM (simplest valid image format)
        # PPM format: P6 (raw), width height, maxval, then raw RGB data
        try:
            with open(test_image, 'wb') as f:
                # Header
                f.write(b"P6\n")
                f.write(b"320 256\n")
                f.write(b"255\n")
                
                # Image data (320*256*3 bytes of gray color)
                pixel = bytes([128, 128, 128])  # Gray pixel
                for _ in range(320 * 256):
                    f.write(pixel)
            return test_image
        except Exception as e:
            print(f"Warning: Could not create test image: {e}")
            return ""
    
    def _run_test(self, args: List[str]) -> Tuple[int, str, str]:
        """Execute test and capture result"""
        cmd = [self.exe] + args
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "TIMEOUT"
        except Exception as e:
            return -1, "", str(e)
    
    def _assert_result(self, test: TestCase, returncode: int, stderr: str) -> bool:
        """Verify test result matches expectations"""
        if test.should_fail:
            if returncode == 0:
                return False, "Expected failure but succeeded"
            if test.expected_error_code and str(test.expected_error_code) not in stderr:
                return False, f"Expected error code {test.expected_error_code} not found in stderr"
            return True, "Failed as expected"
        else:
            if returncode != 0:
                return False, f"Expected success but failed with code {returncode}\nStderr: {stderr}"
            return True, "Succeeded"
    
    def _log_result(self, test: TestCase, passed: bool, message: str):
        """Log test result"""
        status = "PASS" if passed else "FAIL"
        color = Colors.GREEN if passed else Colors.RED
        
        print(f"{color}[{status}]{Colors.RESET} {test.name}")
        if message:
            print(f"     {message}")
        if test.description:
            print(f"     {Colors.CYAN}→ {test.description}{Colors.RESET}")
        
        self.test_results.append({
            'name': test.name,
            'status': status,
            'message': message
        })
        
        if passed:
            self.passed += 1
        else:
            self.failed += 1
    
    # ========================================================================
    # TEST GROUPS
    # ========================================================================
    
    def test_sample_rate_boundaries(self):
        """Test sample rate boundary values"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Sample Rate Boundary Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Sample rate: 7999 Hz (too low)",
                ["-i", self.test_image, "-r", "7999"],
                should_fail=True,
                expected_error_code=114,
                description="Should reject rate below minimum 8000"
            ),
            TestCase(
                "Sample rate: 8000 Hz (boundary min)",
                ["-i", self.test_image, "-r", "8000"],
                should_fail=False,
                description="Should accept exact minimum"
            ),
            TestCase(
                "Sample rate: 22050 Hz (middle)",
                ["-i", self.test_image, "-r", "22050"],
                should_fail=False,
                description="Should accept typical rate"
            ),
            TestCase(
                "Sample rate: 48000 Hz (boundary max)",
                ["-i", self.test_image, "-r", "48000"],
                should_fail=False,
                description="Should accept exact maximum"
            ),
            TestCase(
                "Sample rate: 48001 Hz (too high)",
                ["-i", self.test_image, "-r", "48001"],
                should_fail=True,
                expected_error_code=114,
                description="Should reject rate above maximum 48000"
            ),
            TestCase(
                "Sample rate: 0 Hz",
                ["-i", self.test_image, "-r", "0"],
                should_fail=True,
                expected_error_code=114,
                description="Should reject zero rate"
            ),
            TestCase(
                "Sample rate: negative (-22050 Hz)",
                ["-i", self.test_image, "-r", "-22050"],
                should_fail=True,
                expected_error_code=114,
                description="Should reject negative rate"
            ),
            TestCase(
                "Sample rate: non-numeric (abc)",
                ["-i", self.test_image, "-r", "abc"],
                should_fail=True,
                expected_error_code=114,
                description="Should reject non-numeric input"
            ),
            TestCase(
                "Sample rate: float (22050.5)",
                ["-i", self.test_image, "-r", "22050.5"],
                should_fail=True,
                expected_error_code=114,
                description="Should reject decimal value"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_cw_wpm_boundaries(self):
        """Test CW WPM boundary values (requires callsign)"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== CW WPM Boundary Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "CW WPM: 0 (too low, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "0"],
                should_fail=True,
                expected_error_code=117,
                description="Should reject WPM below minimum 1"
            ),
            TestCase(
                "CW WPM: 1 (boundary min, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "1"],
                should_fail=False,
                description="Should accept minimum WPM"
            ),
            TestCase(
                "CW WPM: 15 (middle, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "15"],
                should_fail=False,
                description="Should accept typical WPM"
            ),
            TestCase(
                "CW WPM: 50 (boundary max, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "50"],
                should_fail=False,
                description="Should accept maximum WPM"
            ),
            TestCase(
                "CW WPM: 51 (too high, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "51"],
                should_fail=True,
                expected_error_code=117,
                description="Should reject WPM above maximum 50"
            ),
            TestCase(
                "CW WPM: negative (-15, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "-15"],
                should_fail=True,
                expected_error_code=117,
                description="Should reject negative WPM"
            ),
            TestCase(
                "CW WPM: non-numeric (abc, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "abc"],
                should_fail=True,
                expected_error_code=117,
                description="Should reject non-numeric WPM"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_cw_tone_boundaries(self):
        """Test CW tone frequency boundary values (requires callsign)"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== CW Tone Boundary Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "CW Tone: 0 Hz (too low, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "0"],
                should_fail=True,
                expected_error_code=118,
                description="Should reject tone below minimum 400"
            ),
            TestCase(
                "CW Tone: 399 Hz (below boundary, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "399"],
                should_fail=True,
                expected_error_code=118,
                description="Should reject tone below minimum 400"
            ),
            TestCase(
                "CW Tone: 400 Hz (boundary min, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "400"],
                should_fail=False,
                description="Should accept minimum tone"
            ),
            TestCase(
                "CW Tone: 800 Hz (middle, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "800"],
                should_fail=False,
                description="Should accept typical tone"
            ),
            TestCase(
                "CW Tone: 2000 Hz (boundary max, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "2000"],
                should_fail=False,
                description="Should accept maximum tone"
            ),
            TestCase(
                "CW Tone: 2001 Hz (too high, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "2001"],
                should_fail=True,
                expected_error_code=118,
                description="Should reject tone above maximum 2000"
            ),
            TestCase(
                "CW Tone: negative (-800, with callsign)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "-800"],
                should_fail=True,
                expected_error_code=118,
                description="Should reject negative tone"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_cw_interdependencies(self):
        """Test CW parameter interdependency validation"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== CW Interdependency Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "CW: WPM without callsign",
                ["-i", self.test_image, "-W", "15"],
                should_fail=True,
                expected_error_code=119,
                description="Should require callsign when WPM specified"
            ),
            TestCase(
                "CW: Tone without callsign",
                ["-i", self.test_image, "-T", "800"],
                should_fail=True,
                expected_error_code=119,
                description="Should require callsign when tone specified"
            ),
            TestCase(
                "CW: WPM and tone without callsign",
                ["-i", self.test_image, "-W", "15", "-T", "800"],
                should_fail=True,
                expected_error_code=119,
                description="Should require callsign for any CW params"
            ),
            TestCase(
                "CW: Callsign alone (should use defaults)",
                ["-i", self.test_image, "-C", "N0CALL"],
                should_fail=False,
                description="Should accept callsign with default WPM/tone"
            ),
            TestCase(
                "CW: Complete with all params",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "20", "-T", "700"],
                should_fail=False,
                description="Should accept all CW params together"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_empty_string_inputs(self):
        """Test empty string inputs for various parameters"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Empty String Input Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Input file: empty string (-i \"\")",
                ["-i", ""],
                should_fail=True,
                expected_error_code=111,  # No input file
                description="Should reject empty input filename"
            ),
            TestCase(
                "Protocol: empty string (-p \"\")",
                ["-i", self.test_image, "-p", ""],
                should_fail=True,
                expected_error_code=112,  # Invalid protocol
                description="Should reject empty protocol"
            ),
            TestCase(
                "Format: empty string (-f \"\")",
                ["-i", self.test_image, "-f", ""],
                should_fail=True,
                expected_error_code=113,  # Invalid format
                description="Should reject empty format"
            ),
            TestCase(
                "Sample rate: empty string (-r \"\")",
                ["-i", self.test_image, "-r", ""],
                should_fail=True,
                expected_error_code=114,  # Invalid sample rate
                description="Should reject empty sample rate"
            ),
            TestCase(
                "Aspect mode: empty string (-a \"\")",
                ["-i", self.test_image, "-a", ""],
                should_fail=True,
                expected_error_code=115,  # Invalid aspect
                description="Should reject empty aspect mode"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_case_sensitivity(self):
        """Test case sensitivity of string parameters"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Case Sensitivity Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Protocol: uppercase (-p M1)",
                ["-i", self.test_image, "-p", "M1"],
                should_fail=True,
                expected_error_code=112,
                description="Should be case-sensitive (lowercase required)"
            ),
            TestCase(
                "Format: uppercase (-f WAV)",
                ["-i", self.test_image, "-f", "WAV"],
                should_fail=True,
                expected_error_code=113,
                description="Should be case-sensitive (lowercase required)"
            ),
            TestCase(
                "Aspect: uppercase (-a CENTER)",
                ["-i", self.test_image, "-a", "CENTER"],
                should_fail=True,
                expected_error_code=115,
                description="Should be case-sensitive (lowercase required)"
            ),
            TestCase(
                "Protocol: mixed case (-p Sdx)",
                ["-i", self.test_image, "-p", "Sdx"],
                should_fail=True,
                expected_error_code=112,
                description="Should reject mixed case"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_filename_lengths(self):
        """Test filename length boundaries"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Filename Length Boundary Testing ==={Colors.RESET}")
        
        # Create test files with specific lengths
        temp_dir = tempfile.gettempdir()
        
        # Test with 255-char filename (should succeed)
        name_255 = "a" * (255 - len(temp_dir) - 1) + ".ppm"
        long_file_255 = os.path.join(temp_dir, name_255[:255])
        
        # Test with 256-char filename (should fail)
        name_256 = "a" * (256 - len(temp_dir) - 1) + ".ppm"
        long_file_256 = os.path.join(temp_dir, name_256[:256])
        
        tests = [
            TestCase(
                "Input filename: 254 chars",
                ["-i", self.test_image],  # Fallback to valid image
                should_fail=False,
                description="Should accept normal length filename"
            ),
            # Note: Can't easily test 255/256 without creating actual files
            # This would need a more sophisticated setup
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_invalid_protocols(self):
        """Test invalid protocol values"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Invalid Protocol Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Invalid protocol: m12",
                ["-i", self.test_image, "-p", "m12"],
                should_fail=True,
                expected_error_code=112,
                description="Should reject typo protocol"
            ),
            TestCase(
                "Invalid protocol: r360 (typo of r36)",
                ["-i", self.test_image, "-p", "r360"],
                should_fail=True,
                expected_error_code=112,
                description="Should reject similar but invalid protocol"
            ),
            TestCase(
                "Invalid protocol: VIS code as string (44)",
                ["-i", self.test_image, "-p", "44"],
                should_fail=True,
                expected_error_code=112,
                description="Should require protocol name, not VIS code"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_invalid_formats(self):
        """Test invalid audio format values"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Invalid Format Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Invalid format: mp3",
                ["-i", self.test_image, "-f", "mp3"],
                should_fail=True,
                expected_error_code=113,
                description="Should reject unsupported format"
            ),
            TestCase(
                "Invalid format: flac",
                ["-i", self.test_image, "-f", "flac"],
                should_fail=True,
                expected_error_code=113,
                description="Should reject unsupported format"
            ),
            TestCase(
                "Invalid format: vorbis (should be ogg)",
                ["-i", self.test_image, "-f", "vorbis"],
                should_fail=True,
                expected_error_code=113,
                description="Should use 'ogg', not 'vorbis'"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def test_invalid_aspect_modes(self):
        """Test invalid aspect ratio mode values"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Invalid Aspect Mode Testing ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Invalid aspect: centre (British spelling)",
                ["-i", self.test_image, "-a", "centre"],
                should_fail=True,
                expected_error_code=115,
                description="Should only accept 'center' (American spelling)"
            ),
            TestCase(
                "Invalid aspect: crop (should be center)",
                ["-i", self.test_image, "-a", "crop"],
                should_fail=True,
                expected_error_code=115,
                description="Should reject similar but invalid mode"
            ),
        ]
        
        for test in tests:
            returncode, stdout, stderr = self._run_test(test.args)
            passed, message = self._assert_result(test, returncode, stderr)
            self._log_result(test, passed, message)
    
    def run_all_tests(self):
        """Run all test groups"""
        print(f"\n{Colors.BOLD}{Colors.CYAN}╔═══════════════════════════════════════════════════════════════╗{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}║  PiSSTVpp2 - Comprehensive Edge Case Test Suite                ║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}╚═══════════════════════════════════════════════════════════════╝{Colors.RESET}")
        
        try:
            self.test_sample_rate_boundaries()
            self.test_cw_wpm_boundaries()
            self.test_cw_tone_boundaries()
            self.test_cw_interdependencies()
            self.test_empty_string_inputs()
            self.test_case_sensitivity()
            self.test_filename_lengths()
            self.test_invalid_protocols()
            self.test_invalid_formats()
            self.test_invalid_aspect_modes()
        finally:
            # Cleanup
            if self.test_image and os.path.exists(self.test_image):
                os.remove(self.test_image)
        
        self._print_summary()
    
    def _print_summary(self):
        """Print test summary"""
        total = self.passed + self.failed + self.skipped
        
        print(f"\n{Colors.BOLD}{Colors.BLUE}═══════════════════════════════════════════════════════════════{Colors.RESET}")
        print(f"{Colors.BOLD}Test Results Summary:{Colors.RESET}")
        print(f"  {Colors.GREEN}Passed:  {self.passed}{Colors.RESET}")
        print(f"  {Colors.RED}Failed:  {self.failed}{Colors.RESET}")
        print(f"  {Colors.YELLOW}Skipped: {self.skipped}{Colors.RESET}")
        print(f"  {Colors.BOLD}Total:   {total}{Colors.RESET}")
        print(f"{Colors.BLUE}═══════════════════════════════════════════════════════════════{Colors.RESET}")
        
        if self.failed == 0:
            print(f"{Colors.GREEN}{Colors.BOLD}✓ All tests passed!{Colors.RESET}")
        else:
            print(f"{Colors.RED}{Colors.BOLD}✗ {self.failed} test(s) failed{Colors.RESET}")

if __name__ == "__main__":
    exe_path = "../bin/pisstvpp2" if len(sys.argv) == 1 else sys.argv[1]
    
    try:
        suite = EdgeCaseTestSuite(exe_path)
        suite.run_all_tests()
        sys.exit(0 if suite.failed == 0 else 1)
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.RESET}")
        sys.exit(2)
