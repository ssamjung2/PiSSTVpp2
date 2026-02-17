#!/usr/bin/env python3
"""
Modernization Validation Tests for SlowFrame v2.1
Tests the v2.1 modernized components:
- Error code system
- Config validation
- Mode registry
- Image module separation
- SSTV module separation
"""

import os
import sys
import subprocess
import json
from pathlib import Path

class ModernizationTests:
    """Test suite for v2.1 modernization features"""
    
    def __init__(self, executable_path=None):
        if executable_path is None:
            script_dir = Path(__file__).parent.parent.parent
            executable_path = str(script_dir / "bin" / "slowframe")
        self.exe = executable_path
        self.passed = 0
        self.failed = 0
        self.test_results = []
        
        # Verify executable exists
        if not os.path.exists(self.exe):
            raise FileNotFoundError(f"Executable not found: {self.exe}")
        
        # Find test images
        script_dir = Path(__file__).parent.parent
        self.images_dir = script_dir / "images"
        if not self.images_dir.exists():
            raise FileNotFoundError(f"Images directory not found: {self.images_dir}")
        
        self.test_img = str(self.images_dir / "test_320x240.png")
        if not os.path.exists(self.test_img):
            raise FileNotFoundError(f"Test image not found: {self.test_img}")
    
    def _run_command(self, args, expect_error=False):
        """Execute slowframe with given arguments"""
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
            return -1, "", f"ERROR: {str(e)}"
    
    def _log_test(self, name, passed, details=""):
        """Log test result"""
        if passed:
            self.passed += 1
            status = "✓ PASS"
        else:
            self.failed += 1
            status = "✗ FAIL"
        
        print(f"  {status}: {name}")
        if details and not passed:
            print(f"         {details}")
        
        self.test_results.append({
            "test": name,
            "passed": passed,
            "details": details
        })
    
    def test_error_code_system(self):
        """Test unified error code system"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Code System")
        print("="*70)
        
        # Test 1: Error codes are returned correctly
        ret, stdout, stderr = self._run_command([])
        self._log_test(
            "Missing -i returns error code 111",
            ret == 111,
            f"Expected 111, got {ret}"
        )
        
        # Test 2: Invalid protocol returns proper error code
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-p", "invalid999"])
        self._log_test(
            "Invalid protocol returns error code 112",
            ret == 112,
            f"Expected 112, got {ret}"
        )
        
        # Test 3: Invalid format returns proper error code
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-f", "mp3"])
        self._log_test(
            "Invalid format returns error code 113",
            ret == 113,
            f"Expected 113, got {ret}"
        )
        
        # Test 4: Invalid sample rate returns proper error code
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-r", "7999"])
        self._log_test(
            "Invalid sample rate returns error code 114",
            ret == 114,
            f"Expected 114, got {ret}"
        )
        
        # Test 5: Invalid aspect mode returns proper error code
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-a", "invalid"])
        self._log_test(
            "Invalid aspect mode returns error code 115",
            ret == 115,
            f"Expected 115, got {ret}"
        )
        
        # Test 6: Invalid CW WPM returns proper error code
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-C", "TEST", "-W", "99"])
        self._log_test(
            "Invalid CW WPM returns error code 117",
            ret == 117,
            f"Expected 117, got {ret}"
        )
        
        # Test 7: Invalid CW tone returns proper error code
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-C", "TEST", "-Q", "300"])
        self._log_test(
            "Invalid CW tone returns error code 118",
            ret == 118,
            f"Expected 118, got {ret}"
        )
        
        # Test 8: Missing callsign returns proper error code
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-W", "15"])
        self._log_test(
            "CW without callsign returns error code 119",
            ret == 119,
            f"Expected 119, got {ret}"
        )
        
        # Test 9: Error messages are human-readable
        ret, stdout, stderr = self._run_command([])
        has_error_msg = "ERROR" in stderr and "Error code 111" in stderr
        self._log_test(
            "Error messages include code and description",
            has_error_msg,
            "Expected '[ERROR] Error code 111:' format"
        )
    
    def test_config_validation(self):
        """Test configuration validation logic"""
        print("\n" + "="*70)
        print("TEST GROUP: Config Validation")
        print("="*70)
        
        # Test 1: Interdependent flags validated
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-O"])
        self._log_test(
            "Flag -O without -N is rejected",
            ret != 0,
            f"Should reject -O without -N, got exit code {ret}"
        )
        
        # Test 2: Valid flag combination accepted
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-O", "-N", "-o", "/tmp/test.png"])
        self._log_test(
            "Flag -O with -N is accepted",
            ret == 0,
            f"Should accept -O with -N, got exit code {ret}"
        )
        
        # Test 3: Sample rate range validation
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-r", "8000"])
        self._log_test(
            "Minimum sample rate 8000 Hz accepted",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-r", "48000"])
        self._log_test(
            "Maximum sample rate 48000 Hz accepted",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-r", "7999"])
        self._log_test(
            "Below-minimum sample rate 7999 Hz rejected",
            ret != 0,
            f"Should reject 7999 Hz, got exit code {ret}"
        )
        
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-r", "48001"])
        self._log_test(
            "Above-maximum sample rate 48001 Hz rejected",
            ret != 0,
            f"Should reject 48001 Hz, got exit code {ret}"
        )
        
        # Test 4: CW parameter ranges
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-C", "TEST", "-W", "1"])
        self._log_test(
            "Minimum CW WPM 1 accepted",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-C", "TEST", "-W", "50"])
        self._log_test(
            "Maximum CW WPM 50 accepted",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-C", "TEST", "-Q", "400"])
        self._log_test(
            "Minimum CW tone 400 Hz accepted",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-C", "TEST", "-Q", "2000"])
        self._log_test(
            "Maximum CW tone 2000 Hz accepted",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        # Test 5: Filename validation
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-o", "a" * 300])
        self._log_test(
            "Oversized filename rejected",
            ret != 0,
            f"Should reject 300-char filename, got exit code {ret}"
        )
    
    def test_mode_registry(self):
        """Test mode registry system"""
        print("\n" + "="*70)
        print("TEST GROUP: Mode Registry")
        print("="*70)
        
        # Test 1: List modes functionality
        ret, stdout, stderr = self._run_command(["-L"])
        self._log_test(
            "Mode list (-L) flag works",
            ret == 0 and "Available SSTV" in stdout,
            f"Expected mode list, got exit code {ret}"
        )
        
        # Test 2: Native modes available
        native_modes = ["m1", "m2", "s1", "s2", "sdx", "r36", "r72"]
        ret, stdout, stderr = self._run_command(["-L"])
        
        if ret == 0:
            output = stdout.lower()
            all_present = all(mode in output for mode in native_modes)
            self._log_test(
                "All 7 native modes listed",
                all_present,
                f"Missing modes in list"
            )
        else:
            self._log_test("All 7 native modes listed", False, "-L failed")
        
        # Test 3: Mode lookup works for each native mode
        for mode in native_modes:
            ret, stdout, stderr = self._run_command(["-i", self.test_img, "-p", mode, "-o", f"/tmp/test_{mode}.wav"])
            self._log_test(
                f"Mode lookup: {mode}",
                ret == 0,
                f"Mode {mode} not found or failed"
            )
        
        # Test 4: Invalid mode rejected
        ret, stdout, stderr = self._run_command(["-i", self.test_img, "-p", "invalid_mode_xyz"])
        self._log_test(
            "Invalid mode rejected",
            ret != 0 and ret == 112,
            f"Expected error code 112 for invalid mode, got {ret}"
        )
        
        # Test 5: MMSSTV status check
        ret, stdout, stderr = self._run_command(["-M"])
        self._log_test(
            "MMSSTV status (-M) flag works",
            ret == 0,
            f"Expected MMSSTV status info, got exit code {ret}"
        )
    
    def test_image_module(self):
        """Test image module separation"""
        print("\n" + "="*70)
        print("TEST GROUP: Image Module")
        print("="*70)
        
        # Test 1: Image loading from various formats
        formats = {
            "test_320x240.png": "PNG",
            "test_320x256.png": "PNG",
        }
        
        for filename, fmt in formats.items():
            img_path = self.images_dir / filename
            if img_path.exists():
                ret, stdout, stderr = self._run_command(["-i", str(img_path), "-o", "/tmp/test_img.wav"])
                self._log_test(
                    f"Image loader: {fmt} ({filename})",
                    ret == 0,
                    f"Failed to load {filename}"
                )
        
        # Test 2: Aspect ratio modes
        aspect_modes = ["center", "pad", "stretch"]
        for mode in aspect_modes:
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-a", mode,
                "-o", f"/tmp/test_aspect_{mode}.wav"
            ])
            self._log_test(
                f"Aspect ratio: {mode}",
                ret == 0,
                f"Aspect mode {mode} failed"
            )
        
        # Test 3: Non-existent file rejection
        ret, stdout, stderr = self._run_command(["-i", "/nonexistent/file.png"])
        self._log_test(
            "Non-existent image file rejected",
            ret != 0,
            f"Should reject non-existent file, got exit code {ret}"
        )
        
        # Test 4: Invalid image format
        # Create a text file and try to use it as image
        import tempfile
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
            f.write("This is not an image")
            temp_file = f.name
        
        ret, stdout, stderr = self._run_command(["-i", temp_file])
        os.unlink(temp_file)
        
        self._log_test(
            "Invalid image format rejected",
            ret != 0,
            f"Should reject text file as image, got exit code {ret}"
        )
    
    def test_sstv_module(self):
        """Test SSTV module separation"""
        print("\n" + "="*70)
        print("TEST GROUP: SSTV Module")
        print("="*70)
        
        # Test 1: SSTV encoding works for each protocol
        protocols = {
            "m1": "Martin 1",
            "m2": "Martin 2",
            "s1": "Scottie 1",
            "s2": "Scottie 2",
            "sdx": "Scottie DX",
            "r36": "Robot 36",
            "r72": "Robot 72"
        }
        
        for protocol, name in protocols.items():
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-p", protocol,
                "-o", f"/tmp/test_sstv_{protocol}.wav"
            ])
            
            # Check file was created and has content
            output_file = f"/tmp/test_sstv_{protocol}.wav"
            file_ok = os.path.exists(output_file) and os.path.getsize(output_file) > 1000
            
            self._log_test(
                f"SSTV encode: {protocol} ({name})",
                ret == 0 and file_ok,
                f"Protocol {protocol} encoding failed or no output"
            )
        
        # Test 2: CW signature integration
        ret, stdout, stderr = self._run_command([
            "-i", self.test_img,
            "-C", "W5ABC",
            "-o", "/tmp/test_cw.wav"
        ])
        
        # CW should add ~2-5 seconds to output
        output_file = "/tmp/test_cw.wav"
        if os.path.exists(output_file):
            size_with_cw = os.path.getsize(output_file)
            self._log_test(
                "CW signature encoded",
                ret == 0 and size_with_cw > 100000,
                f"CW encoding failed or output too small"
            )
        else:
            self._log_test("CW signature encoded", False, "No output file created")
        
        # Test 3: Different audio formats
        formats = ["wav", "aiff", "ogg"]
        for fmt in formats:
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-f", fmt,
                "-o", f"/tmp/test_format.{fmt}"
            ])
            
            output_file = f"/tmp/test_format.{fmt}"
            file_ok = os.path.exists(output_file) and os.path.getsize(output_file) > 1000
            
            self._log_test(
                f"Audio format: {fmt.upper()}",
                ret == 0 and file_ok,
                f"Format {fmt} failed or no output"
            )
        
        # Test 4: Different sample rates
        sample_rates = [8000, 11025, 22050, 32000, 44100, 48000]
        for rate in sample_rates:
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-r", str(rate),
                "-o", f"/tmp/test_rate_{rate}.wav"
            ])
            
            self._log_test(
                f"Sample rate: {rate} Hz",
                ret == 0,
                f"Sample rate {rate} Hz failed"
            )
    
    def run_all_tests(self):
        """Run all modernization tests"""
        print("="*70)
        print("SlowFrame v2.1 Modernization Validation Tests")
        print("="*70)
        
        self.test_error_code_system()
        self.test_config_validation()
        self.test_mode_registry()
        self.test_image_module()
        self.test_sstv_module()
        
        print("\n" + "="*70)
        print("SUMMARY")
        print("="*70)
        total = self.passed + self.failed
        print(f"Total tests: {total}")
        print(f"✓ PASSED: {self.passed}")
        print(f"✗ FAILED: {self.failed}")
        
        if self.failed == 0:
            print("\n✓ All modernization tests passed!")
            return 0
        else:
            print(f"\n✗ {self.failed} test(s) failed")
            return 1

if __name__ == "__main__":
    try:
        tester = ModernizationTests()
        exit_code = tester.run_all_tests()
        sys.exit(exit_code)
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)
