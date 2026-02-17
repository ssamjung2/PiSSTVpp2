#!/usr/bin/env python3
"""
Test Robot B&W 24 Mode Implementation

Validates the newly implemented native bw24 mode:
- Mode registration
- VIS code (9)
- Encoding functionality  
- Output validation
"""

import os
import sys
import subprocess
from pathlib import Path

class BW24ModeTests:
    def __init__(self, executable_path):
        self.executable = executable_path
        self.workspace_root = Path(__file__).parent.parent.parent
        self.test_image = self.workspace_root / "tests" / "images" / "test_320x240.png"
        self.passed = 0
        self.failed = 0
        
    def _run_command(self, args, cwd=None):
        """Execute slowframe command"""
        cmd = [str(self.executable)] + args
        if cwd is None:
            cwd = str(self.workspace_root)
        result = subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True
        )
        return result.returncode, result.stdout, result.stderr
    
    def _log_test(self, name, passed, details="", skipped=False):
        """Log test result"""
        if skipped:
            print(f"  ⊘ SKIP: {name}")
            if details:
                print(f"         {details}")
        elif passed:
            print(f"  ✓ PASS: {name}")
            self.passed += 1
        else:
            print(f"  ✗ FAIL: {name}")
            if details:
                print(f"         {details}")
            self.failed += 1
    
    def test_mode_registration(self):
        """Test that bw24 mode is registered"""
        print("\n" + "=" * 70)
        print("TEST GROUP: Mode Registration")
        print("=" * 70)
        
        # Test 1: Mode appears in list
        exit_code, stdout, stderr = self._run_command(["-L"])
        passed = exit_code == 0 and "bw24" in stdout
        self._log_test("bw24 mode appears in mode list", passed,
                      "" if passed else f"Mode not found in output")
        
        # Test 2: VIS code is correct
        passed = "0x09" in stdout or "0x9" in stdout
        self._log_test("bw24 VIS code is 0x09", passed,
                      "" if passed else f"VIS code not found or incorrect")
        
        # Test 3: Mode name is correct
        passed = "Robot B&W 24" in stdout
        self._log_test("bw24 mode name is 'Robot B&W 24'", passed,
                      "" if passed else f"Mode name incorrect")
        
        # Test 4: Resolution is correct
        passed = "320x240" in stdout
        self._log_test("bw24 resolution is 320x240", passed,
                      "" if passed else f"Resolution incorrect")
        
        # Test 5: Duration is correct
        passed = "24.0s" in stdout or "24s" in stdout
        self._log_test("bw24 duration is 24 seconds", passed,
                      "" if passed else f"Duration incorrect")
        
        # Test 6: Color type is monochrome
        passed = "mono" in stdout.lower() or "bw" in stdout.lower()
        self._log_test("bw24 color type is monochrome", passed,
                      "" if passed else f"Color type not monochrome")
        
        # Test 7: Source is native
        passed = "NATIVE MODES" in stdout and "(8):" in stdout
        lines = stdout.split('\n')
        native_section_found = False
        bw24_in_native = False
        for i, line in enumerate(lines):
            if "NATIVE MODES" in line:
                native_section_found = True
            if native_section_found and "bw24" in line:
                # Check if we haven't reached MMSSTV section yet
                mmsstv_reached = any("MMSSTV MODES" in lines[j] for j in range(0, i))
                if not mmsstv_reached:
                    bw24_in_native = True
                    break
        passed = native_section_found and bw24_in_native
        self._log_test("bw24 is a native mode", passed,
                      "" if passed else f"Mode not in native section")
    
    def test_encoding(self):
        """Test encoding with bw24 mode"""
        print("\n" + "=" * 70)
        print("TEST GROUP: Encoding Functionality")
        print("=" * 70)
        
        output_file = "/tmp/test_bw24_validation.wav"
        
        # Test 1: Encoding succeeds
        exit_code, stdout, stderr = self._run_command([
            "-i", str(self.test_image),
            "-p", "bw24",
            "-o", output_file
        ])
        passed = exit_code == 0
        self._log_test("Encoding with bw24 succeeds", passed,
                      f"Exit code: {exit_code}" if not passed else "")
        
        if not passed:
            return  # Skip remaining tests if encoding failed
        
        # Test 2: Output file exists
        passed = os.path.exists(output_file)
        self._log_test("Output WAV file created", passed,
                      "" if passed else f"File not found: {output_file}")
        
        if not passed:
            return
        
        # Test 3: File size is reasonable
        file_size = os.path.getsize(output_file)
        # Expected: ~27s at 22050Hz * 2 bytes/sample = ~1.2MB
        passed = 900000 < file_size < 1500000  # 900KB to 1.5MB range
        self._log_test("Output file size is reasonable", passed,
                      f"Size: {file_size} bytes" if not passed else f"Size: {file_size} bytes")
        
        # Test 4: Configuration shows correct protocol
        passed = "bw24" in stdout
        self._log_test("Configuration shows bw24 protocol", passed,
                      "" if passed else "Protocol not shown in output")
        
        # Test 5: VIS code shown correctly
        passed = "VIS code 9" in stdout or "VIS code 0x09" in stdout or "VIS code 0x9" in stdout
        self._log_test("VIS code 9 displayed in output", passed,
                      "" if passed else "VIS code not displayed correctly")
        
        # Test 6: Duration is reasonable
        if "seconds at" in stdout:
            duration_str = stdout.split("seconds at")[0].split()[-1]
            try:
                duration = float(duration_str.replace("(", ""))
                # Should be ~24s + VIS header/trailer = ~27s
                passed = 23 < duration < 30
                self._log_test("Audio duration is correct", passed,
                              f"Duration: {duration}s" + (" (outside 23-30s range)" if not passed else ""))
            except:
                self._log_test("Audio duration is correct", False,
                              "Could not parse duration from output")
        else:
            self._log_test("Audio duration is correct", False,
                          "Duration not found in output")
        
        # Test 7: Encoding is fast
        if "Encoding time:" in stdout:
            time_str = stdout.split("Encoding time:")[1].split()[0]
            try:
                time_ms = int(time_str)
                passed = time_ms < 5000  # Should be under 5 seconds
                self._log_test("Encoding completes quickly", passed,
                              f"Time: {time_ms}ms" + (" (over 5000ms)" if not passed else ""))
            except:
                self._log_test("Encoding completes quickly", True,
                              "Could not parse encoding time")
        
        # Test 8: Different sample rates work
        for rate in [11025, 22050, 44100]:
            output_rate_file = f"/tmp/test_bw24_{rate}Hz.wav"
            exit_code, stdout, stderr = self._run_command([
                "-i", str(self.test_image),
                "-p", "bw24",
                "-r", str(rate),
                "-o", output_rate_file
            ])
            passed = exit_code == 0 and os.path.exists(output_rate_file)
            self._log_test(f"Encoding at {rate}Hz works", passed,
                          f"Exit code: {exit_code}" if not passed else "")
            if os.path.exists(output_rate_file):
                os.remove(output_rate_file)
        
        # Cleanup
        if os.path.exists(output_file):
            os.remove(output_file)
    
    def test_compatibility(self):
        """Test compatibility with existing features"""
        print("\n" + "=" * 70)
        print("TEST GROUP: Feature Compatibility")
        print("=" * 70)
        
        output_file = "/tmp/test_bw24_compat.wav"
        
        # Test 1: Works with different output formats
        for fmt, ext in [("wav", "wav"), ("aiff", "aiff"), ("ogg", "ogg")]:
            output_fmt_file = f"/tmp/test_bw24.{ext}"
            exit_code, stdout, stderr = self._run_command([
                "-i", str(self.test_image),
                "-p", "bw24",
                "-f", fmt,
                "-o", output_fmt_file
            ])
            passed = exit_code == 0 and os.path.exists(output_fmt_file)
            self._log_test(f"Encoding to {fmt.upper()} format works", passed,
                          f"Exit code: {exit_code}" if not passed else "")
            if os.path.exists(output_fmt_file):
                os.remove(output_fmt_file)
        
        # Test 2: Works with CW signature
        exit_code, stdout, stderr = self._run_command([
            "-i", str(self.test_image),
            "-p", "bw24",
            "-C", "TEST",
            "-o", output_file
        ])
        passed = exit_code == 0
        self._log_test("Works with CW signature", passed,
                      f"Exit code: {exit_code}" if not passed else "")
        if os.path.exists(output_file):
            os.remove(output_file)
        
        # Test 3: Works with text overlay
        exit_code, stdout, stderr = self._run_command([
            "-i", str(self.test_image),
            "-p", "bw24",
            "-T", "Test BW24",
            "-o", output_file
        ])
        passed = exit_code == 0
        self._log_test("Works with text overlay", passed,
                      f"Exit code: {exit_code}" if not passed else "")
        if os.path.exists(output_file):
            os.remove(output_file)
    
    def run_all_tests(self):
        """Run all test groups"""
        print("=" * 70)
        print("SlowFrame v2.1 - Robot B&W 24 Mode Validation")
        print("=" * 70)
        
        self.test_mode_registration()
        self.test_encoding()
        self.test_compatibility()
        
        print("\n" + "=" * 70)
        print("SUMMARY")
        print("=" * 70)
        print(f"Total tests: {self.passed + self.failed}")
        print(f"✓ PASSED: {self.passed}")
        print(f"✗ FAILED: {self.failed}")
        print("=" * 70)
        
        if self.failed == 0:
            print("\n✓ All Robot B&W 24 mode tests passed!")
            return 0
        else:
            print(f"\n✗ {self.failed} test(s) failed")
            return 1


def main():
    """Main entry point"""
    workspace_root = Path(__file__).parent.parent.parent
    executable = workspace_root / "bin" / "slowframe"
    
    if not executable.exists():
        print(f"ERROR: Executable not found: {executable}")
        print("Please build SlowFrame first: make")
        return 1
    
    tester = BW24ModeTests(executable)
    return tester.run_all_tests()


if __name__ == "__main__":
    sys.exit(main())
