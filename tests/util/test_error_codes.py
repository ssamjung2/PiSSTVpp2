#!/usr/bin/env python3
"""
Error Code Verification Testing for PiSSTVpp2
Verifies that error conditions return the exact expected error codes from error.h
"""

import subprocess
import tempfile
import re
import os
import sys
from pathlib import Path
from typing import Tuple, Optional
from dataclasses import dataclass

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
    """Represents a single error code test case"""
    name: str
    args: list
    expected_error_code: int
    description: str = ""

class ErrorCodeVerificationSuite:
    """Test suite for verifying exact error codes returned"""
    
    # Error codes from error.h
    ERROR_CODES = {
        111: "PISSTVPP2_ERR_NO_INPUT_FILE",
        112: "PISSTVPP2_ERR_ARG_INVALID_PROTOCOL",
        113: "PISSTVPP2_ERR_ARG_INVALID_FORMAT",
        114: "PISSTVPP2_ERR_ARG_INVALID_SAMPLE_RATE",
        115: "PISSTVPP2_ERR_ARG_INVALID_ASPECT",
        116: "PISSTVPP2_ERR_ARG_INVALID_CALLSIGN",
        117: "PISSTVPP2_ERR_ARG_INVALID_WPM",
        118: "PISSTVPP2_ERR_ARG_INVALID_TONE",
        119: "PISSTVPP2_ERR_ARG_CW_REQUIRES_CALLSIGN",
        500: "PISSTVPP2_ERR_FILE_NOT_FOUND",
        501: "PISSTVPP2_ERR_FILE_OPEN",
        504: "PISSTVPP2_ERR_FILE_NOT_FOUND",
    }
    
    def __init__(self, executable_path=None):
        if executable_path is None:
            script_dir = Path(__file__).parent.parent.parent
            executable_path = str(script_dir / "bin" / "pisstvpp2")
        self.exe = executable_path
        self.passed = 0
        self.failed = 0
        self.total = 0
        self.test_results = []
        
        if not os.path.exists(self.exe):
            raise FileNotFoundError(f"Executable not found: {self.exe}")
        
        self.test_image = self._create_test_image()
    
    def _create_test_image(self) -> str:
        """Create a minimal PPM test image"""
        temp_dir = tempfile.gettempdir()
        test_image = os.path.join(temp_dir, "pisstvpp2_errortest_image.ppm")
        
        try:
            with open(test_image, 'wb') as f:
                f.write(b"P6\n")
                f.write(b"320 256\n")
                f.write(b"255\n")
                pixel = bytes([128, 128, 128])
                for _ in range(320 * 256):
                    f.write(pixel)
            return test_image
        except Exception as e:
            print(f"Warning: Could not create test image: {e}")
            return ""
    
    def _run_command(self, args: list) -> Tuple[int, str, str]:
        """Execute the command and return exit code, stdout, stderr"""
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
    
    def _extract_error_code(self, stderr: str) -> Optional[int]:
        """Extract error code from stderr output"""
        # Look for "Error code: 123" pattern
        match = re.search(r'Error code: (\d+)', stderr)
        if match:
            return int(match.group(1))
        
        # Also try to find just the number at the end
        match = re.search(r'\b(\d{3})\b', stderr)
        if match:
            return int(match.group(1))
        
        return None
    
    def _log_result(self, test: TestCase, exit_code: int, actual_code: Optional[int], stderr: str):
        """Log test result"""
        expected_name = self.ERROR_CODES.get(test.expected_error_code, "UNKNOWN")
        actual_name = self.ERROR_CODES.get(actual_code, "UNKNOWN") if actual_code else "NOT FOUND"
        
        passed = (actual_code == test.expected_error_code)
        status = "PASS" if passed else "FAIL"
        color = Colors.GREEN if passed else Colors.RED
        
        print(f"{color}[{status}]{Colors.RESET} {test.name}")
        print(f"     Expected: {test.expected_error_code} ({expected_name})")
        print(f"     Actual:   {actual_code if actual_code is not None else 'NOT FOUND'} ({actual_name})")
        
        if test.description:
            print(f"     {Colors.CYAN}→ {test.description}{Colors.RESET}")
        
        if not passed and stderr:
            print(f"     Stderr: {stderr[:100]}")
        
        self.test_results.append({
            'name': test.name,
            'expected': test.expected_error_code,
            'actual': actual_code,
            'passed': passed
        })
        
        if passed:
            self.passed += 1
        else:
            self.failed += 1
    
    def test_sample_rate_errors(self):
        """Test error codes for sample rate validation errors"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Error Code: Sample Rate Validation ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Sample rate too low (7999 Hz)",
                ["-i", self.test_image, "-r", "7999"],
                114,
                "Should return ERR_ARG_INVALID_SAMPLE_RATE"
            ),
            TestCase(
                "Sample rate too high (50000 Hz)",
                ["-i", self.test_image, "-r", "50000"],
                114,
                "Should return ERR_ARG_INVALID_SAMPLE_RATE"
            ),
            TestCase(
                "Sample rate non-numeric",
                ["-i", self.test_image, "-r", "twenty-two-thousand"],
                114,
                "Should return ERR_ARG_INVALID_SAMPLE_RATE"
            ),
        ]
        
        for test in tests:
            exit_code, stdout, stderr = self._run_command(test.args)
            error_code = self._extract_error_code(stderr)
            self._log_result(test, exit_code, error_code, stderr)
            self.total += 1
    
    def test_protocol_errors(self):
        """Test error codes for protocol validation errors"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Error Code: Protocol Validation ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Invalid protocol (m99)",
                ["-i", self.test_image, "-p", "m99"],
                112,
                "Should return ERR_ARG_INVALID_PROTOCOL"
            ),
            TestCase(
                "Invalid protocol (SSTV)",
                ["-i", self.test_image, "-p", "SSTV"],
                112,
                "Should return ERR_ARG_INVALID_PROTOCOL"
            ),
            TestCase(
                "Invalid protocol (empty)",
                ["-i", self.test_image, "-p", ""],
                112,
                "Should return ERR_ARG_INVALID_PROTOCOL"
            ),
        ]
        
        for test in tests:
            exit_code, stdout, stderr = self._run_command(test.args)
            error_code = self._extract_error_code(stderr)
            self._log_result(test, exit_code, error_code, stderr)
            self.total += 1
    
    def test_format_errors(self):
        """Test error codes for format validation errors"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Error Code: Format Validation ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Invalid format (mp3)",
                ["-i", self.test_image, "-f", "mp3"],
                113,
                "Should return ERR_ARG_INVALID_FORMAT"
            ),
            TestCase(
                "Invalid format (AIFF uppercase)",
                ["-i", self.test_image, "-f", "AIFF"],
                113,
                "Should return ERR_ARG_INVALID_FORMAT"
            ),
            TestCase(
                "Invalid format (empty)",
                ["-i", self.test_image, "-f", ""],
                113,
                "Should return ERR_ARG_INVALID_FORMAT"
            ),
        ]
        
        for test in tests:
            exit_code, stdout, stderr = self._run_command(test.args)
            error_code = self._extract_error_code(stderr)
            self._log_result(test, exit_code, error_code, stderr)
            self.total += 1
    
    def test_aspect_errors(self):
        """Test error codes for aspect mode validation errors"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Error Code: Aspect Mode Validation ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Invalid aspect (crop)",
                ["-i", self.test_image, "-a", "crop"],
                115,
                "Should return ERR_ARG_INVALID_ASPECT"
            ),
            TestCase(
                "Invalid aspect (CENTER uppercase)",
                ["-i", self.test_image, "-a", "CENTER"],
                115,
                "Should return ERR_ARG_INVALID_ASPECT"
            ),
            TestCase(
                "Invalid aspect (empty)",
                ["-i", self.test_image, "-a", ""],
                115,
                "Should return ERR_ARG_INVALID_ASPECT"
            ),
        ]
        
        for test in tests:
            exit_code, stdout, stderr = self._run_command(test.args)
            error_code = self._extract_error_code(stderr)
            self._log_result(test, exit_code, error_code, stderr)
            self.total += 1
    
    def test_cw_errors(self):
        """Test error codes for CW parameter validation errors"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Error Code: CW Parameter Validation ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "CW WPM too high (100)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "100"],
                117,
                "Should return ERR_ARG_INVALID_WPM"
            ),
            TestCase(
                "CW WPM too low (0)",
                ["-i", self.test_image, "-C", "N0CALL", "-W", "0"],
                117,
                "Should return ERR_ARG_INVALID_WPM"
            ),
            TestCase(
                "CW Tone too high (5000)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "5000"],
                118,
                "Should return ERR_ARG_INVALID_TONE"
            ),
            TestCase(
                "CW Tone too low (0)",
                ["-i", self.test_image, "-C", "N0CALL", "-T", "0"],
                118,
                "Should return ERR_ARG_INVALID_TONE"
            ),
            TestCase(
                "CW WPM without callsign",
                ["-i", self.test_image, "-W", "15"],
                119,
                "Should return ERR_ARG_CW_REQUIRES_CALLSIGN"
            ),
            TestCase(
                "CW Tone without callsign",
                ["-i", self.test_image, "-T", "800"],
                119,
                "Should return ERR_ARG_CW_REQUIRES_CALLSIGN"
            ),
        ]
        
        for test in tests:
            exit_code, stdout, stderr = self._run_command(test.args)
            error_code = self._extract_error_code(stderr)
            self._log_result(test, exit_code, error_code, stderr)
            self.total += 1
    
    def test_missing_input_file(self):
        """Test error codes for missing input file"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Error Code: Missing/Invalid Input File ==={Colors.RESET}")
        
        tests = [
            TestCase(
                "Missing input file",
                [],
                111,
                "Should return ERR_NO_INPUT_FILE"
            ),
            TestCase(
                "No file specified with -i",
                ["-i", ""],
                111,
                "Should return ERR_NO_INPUT_FILE"
            ),
        ]
        
        for test in tests:
            exit_code, stdout, stderr = self._run_command(test.args)
            error_code = self._extract_error_code(stderr)
            self._log_result(test, exit_code, error_code, stderr)
            self.total += 1
    
    def run_all_tests(self):
        """Run all error code verification tests"""
        print(f"\n{Colors.BOLD}{Colors.CYAN}╔═══════════════════════════════════════════════════════════════╗{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}║  Error Code Verification Suite                                  ║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}╚═══════════════════════════════════════════════════════════════╗{Colors.RESET}")
        
        try:
            self.test_missing_input_file()
            self.test_sample_rate_errors()
            self.test_protocol_errors()
            self.test_format_errors()
            self.test_aspect_errors()
            self.test_cw_errors()
        finally:
            if self.test_image and os.path.exists(self.test_image):
                os.remove(self.test_image)
        
        self._print_summary()
    
    def _print_summary(self):
        """Print test summary"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}═══════════════════════════════════════════════════════════════{Colors.RESET}")
        print(f"{Colors.BOLD}Error Code Verification Results:{Colors.RESET}")
        print(f"  {Colors.GREEN}Passed:  {self.passed}/{self.total}{Colors.RESET}")
        print(f"  {Colors.RED}Failed:  {self.failed}/{self.total}{Colors.RESET}")
        print(f"{Colors.BLUE}═══════════════════════════════════════════════════════════════{Colors.RESET}")
        
        if self.failed == 0:
            print(f"{Colors.GREEN}{Colors.BOLD}✓ All error codes verified correctly!{Colors.RESET}")
        else:
            print(f"{Colors.RED}{Colors.BOLD}✗ {self.failed} error code(s) failed verification{Colors.RESET}")
            print(f"\n{Colors.BOLD}Failed Tests:{Colors.RESET}")
            for result in self.test_results:
                if not result['passed']:
                    print(f"  • {result['name']}")
                    print(f"    Expected: {result['expected']}")
                    print(f"    Got: {result['actual']}")

if __name__ == "__main__":
    if len(sys.argv) == 1:
        script_dir = Path(__file__).parent.parent.parent
        exe_path = str(script_dir / "bin" / "pisstvpp2")
    else:
        exe_path = sys.argv[1]
    
    try:
        suite = ErrorCodeVerificationSuite(exe_path)
        suite.run_all_tests()
        sys.exit(0 if suite.failed == 0 else 1)
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.RESET}")
        sys.exit(2)
