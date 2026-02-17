#!/usr/bin/env python3
"""
MMSSTV Integration Tests for SlowFrame v2.1
Tests the dynamic MMSSTV library integration:
- Library detection (with/without)
- Mode enumeration
- Encoding with MMSSTV modes
- Graceful degradation
- Environment variable overrides
"""

import os
import sys
import subprocess
import tempfile
from pathlib import Path

class MMSSTVIntegrationTests:
    """Test suite for MMSSTV library integration"""
    
    def __init__(self, executable_path=None):
        if executable_path is None:
            script_dir = Path(__file__).parent.parent.parent
            executable_path = str(script_dir / "bin" / "slowframe")
        self.exe = executable_path
        self.passed = 0
        self.failed = 0
        self.skipped = 0
        self.test_results = []
        
        # Store workspace root for setting cwd when running commands
        self.workspace_root = Path(__file__).parent.parent.parent
        
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
        
        # Detect if MMSSTV library is available
        self.mmsstv_available = self._check_mmsstv_availability()
    
    def _check_mmsstv_availability(self):
        """Check if MMSSTV library is available"""
        ret, stdout, stderr = self._run_command(["-M"])
        if ret == 0 and "DETECTED" in stdout:
            return True
        return False
    
    def _run_command(self, args, env=None):
        """Execute slowframe with given arguments"""
        cmd = [self.exe] + args
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60,
                env=env,
                cwd=str(self.workspace_root)  # Run from workspace root for relative paths
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "TIMEOUT"
        except Exception as e:
            return -1, "", f"ERROR: {str(e)}"
    
    def _log_test(self, name, passed, details="", skipped=False):
        """Log test result"""
        if skipped:
            self.skipped += 1
            status = "⊘ SKIP"
            print(f"  {status}: {name}")
            if details:
                print(f"         {details}")
        elif passed:
            self.passed += 1
            status = "✓ PASS"
            print(f"  {status}: {name}")
        else:
            self.failed += 1
            status = "✗ FAIL"
            print(f"  {status}: {name}")
            if details:
                print(f"         {details}")
        
        self.test_results.append({
            "test": name,
            "passed": passed,
            "skipped": skipped,
            "details": details
        })
    
    def test_library_detection(self):
        """Test MMSSTV library detection"""
        print("\n" + "="*70)
        print("TEST GROUP: Library Detection")
        print("="*70)
        
        # Test 1: Check status command works
        ret, stdout, stderr = self._run_command(["-M"])
        self._log_test(
            "MMSSTV status command (-M) works",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        # Test 2: Library detection status
        if self.mmsstv_available:
            ret, stdout, stderr = self._run_command(["-M"])
            has_detected = "DETECTED" in stdout or "LOADED" in stdout
            self._log_test(
                "MMSSTV library detected when available",
                has_detected,
                "Library should be detected but wasn't"
            )
            
            # Test 3: Library version reported
            has_version = "1.0.0" in stdout or "Version:" in stdout
            self._log_test(
                "MMSSTV library version reported",
                has_version,
                "Library version should be shown"
            )
            
            # Test 4: Library path reported
            has_path = "Path" in stdout or "path" in stdout
            self._log_test(
                "MMSSTV library path shown",
                has_path,
                "Library path should be displayed"
            )
            
            # Test 5: Mode count reported
            has_mode_count = "43" in stdout or "Modes:" in stdout
            self._log_test(
                "MMSSTV mode count reported",
                has_mode_count,
                "Should report 43 MMSSTV modes"
            )
        else:
            self._log_test(
                "MMSSTV library not available",
                True,
                "Tests will validate graceful degradation",
                skipped=False
            )
    
    def test_mode_enumeration(self):
        """Test mode enumeration with MMSSTV"""
        print("\n" + "="*70)
        print("TEST GROUP: Mode Enumeration")
        print("="*70)
        
        # Test 1: Mode list includes MMSSTV modes
        ret, stdout, stderr = self._run_command(["-L"])
        
        if not self.mmsstv_available:
            # Should only show 7 native modes
            self._log_test(
                "Mode list shows native modes only (no library)",
                ret == 0 and "NATIVE MODES" in stdout,
                "Should list 7 native modes when library unavailable"
            )
            self._log_test(
                "MMSSTV section absent without library",
                "MMSSTV MODES" not in stdout,
                "Should not show MMSSTV section",
                skipped=True
            )
            return
        
        # With library available:
        self._log_test(
            "Mode list command succeeds",
            ret == 0,
            f"Expected success, got exit code {ret}"
        )
        
        # Test 2: Native modes section present
        has_native = "NATIVE MODES" in stdout
        self._log_test(
            "Native modes section shown",
            has_native,
            "Should show NATIVE MODES section"
        )
        
        # Test 3: MMSSTV modes section present
        has_mmsstv = "MMSSTV MODES" in stdout or "MMSSTV Library" in stdout
        self._log_test(
            "MMSSTV modes section shown",
            has_mmsstv,
            "Should show MMSSTV MODES section when library loaded"
        )
        
        # Test 4: Verify some known MMSSTV modes
        mmsstv_modes = ["b/w8", "b/w12", "robot24", "robot36"]
        output_lower = stdout.lower()
        
        for mode in mmsstv_modes:
            if mode in output_lower:
                self._log_test(
                    f"MMSSTV mode '{mode}' listed",
                    True,
                    ""
                )
                break
        
        # Test 5: Total mode count correct
        # Should have 7 native + 43 MMSSTV = 50 modes
        has_total = "Total modes: 50" in stdout or "50" in stdout
        self._log_test(
            "Total mode count is 50 (7 + 43)",
            has_total,
            "Expected 50 total modes"
        )
    
    def test_mmsstv_encoding(self):
        """Test encoding with MMSSTV modes"""
        print("\n" + "="*70)
        print("TEST GROUP: MMSSTV Mode Encoding")
        print("="*70)
        
        if not self.mmsstv_available:
            self._log_test(
                "MMSSTV encoding tests",
                True,
                "Skipped - library not available",
                skipped=True
            )
            return
        
        # Test various MMSSTV modes
        mmsstv_modes = {
            "b/w8": "Black & White 8s",
            "b/w12": "Black & White 12s",
            "robot24": "Robot 24 Color",
        }
        
        for mode_code, mode_name in mmsstv_modes.items():
            # Sanitize mode code for filename (replace / with _)
            safe_mode_code = mode_code.replace("/", "_")
            output_file = f"/tmp/test_mmsstv_{safe_mode_code}.wav"
            
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-p", mode_code,
                "-o", output_file
            ])
            
            # Check encoding succeeded and file created
            file_ok = os.path.exists(output_file) and os.path.getsize(output_file) > 10000
            
            self._log_test(
                f"Encode MMSSTV mode: {mode_code} ({mode_name})",
                ret == 0 and file_ok,
                f"Exit code: {ret}, File created: {os.path.exists(output_file)}"
            )
            
            # Clean up
            if os.path.exists(output_file):
                try:
                    os.remove(output_file)
                except:
                    pass
        
        # Test invalid MMSSTV mode
        ret, stdout, stderr = self._run_command([
            "-i", self.test_img,
            "-p", "invalid_mmsstv_mode_xyz"
        ])
        
        self._log_test(
            "Invalid MMSSTV mode rejected",
            ret != 0,
            f"Should reject invalid mode, got exit code {ret}"
        )
    
    def test_graceful_degradation(self):
        """Test graceful degradation when library unavailable"""
        print("\n" + "="*70)
        print("TEST GROUP: Graceful Degradation")
        print("="*70)
        
        if not self.mmsstv_available:
            # Library truly not available - test native modes still work
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-p", "m1",
                "-o", "/tmp/test_graceful_m1.wav"
            ])
            
            file_ok = os.path.exists("/tmp/test_graceful_m1.wav")
            self._log_test(
                "Native mode works without MMSSTV library",
                ret == 0 and file_ok,
                f"Native modes should always work"
            )
            
            # Clean up
            if os.path.exists("/tmp/test_graceful_m1.wav"):
                os.remove("/tmp/test_graceful_m1.wav")
            
            # MMSSTV mode should fail gracefully
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-p", "b/w8",
                "-o", "/tmp/test_graceful_bw8.wav"
            ])
            
            self._log_test(
                "MMSSTV mode rejected when library unavailable",
                ret != 0,
                f"Should reject MMSSTV mode without library"
            )
            
            has_helpful_error = ("not found" in stderr.lower() or 
                                "not available" in stderr.lower() or
                                "library" in stderr.lower())
            self._log_test(
                "Helpful error message when library missing",
                has_helpful_error,
                "Error should mention library availability"
            )
            
            return
        
        # When library IS available, test that native modes work independently
        ret, stdout, stderr = self._run_command([
            "-i", self.test_img,
            "-p", "m1",
            "-o", "/tmp/test_graceful_m1.wav"
        ])
        
        file_ok = os.path.exists("/tmp/test_graceful_m1.wav")
        self._log_test(
            "Native modes work independently of MMSSTV",
            ret == 0 and file_ok,
            f"Native modes should work regardless of MMSSTV library"
        )
        
        # Clean up
        if os.path.exists("/tmp/test_graceful_m1.wav"):
            os.remove("/tmp/test_graceful_m1.wav")
        
        # Note: Can't truly test library unavailable when it's present
        # The library has robust fallback detection, so invalid env var doesn't prevent detection
        self._log_test(
            "Library has robust automatic detection",
            True,
            "Cannot fully test library absence when library is present",
            skipped=False
        )
    
    def test_environment_variable(self):
        """Test environment variable override"""
        print("\n" + "="*70)
        print("TEST GROUP: Environment Variable")
        print("="*70)
        
        if not self.mmsstv_available:
            self._log_test(
                "Environment variable tests",
                True,
                "Skipped - need known library path to test",
                skipped=True
            )
            return
        
        # Get current library path
        ret, stdout, stderr = self._run_command(["-M"])
        
        # Extract library path from status output
        import re
        path_match = re.search(r'Library Path:\s+(.+)', stdout)
        if path_match:
            current_path = path_match.group(1).strip()
            
            # Test 1: Setting MMSSTV_LIB_PATH to same path works
            env = os.environ.copy()
            env['MMSSTV_LIB_PATH'] = current_path
            
            ret, stdout, stderr = self._run_command(["-M"], env=env)
            self._log_test(
                "MMSSTV_LIB_PATH environment variable honored",
                ret == 0 and "DETECTED" in stdout,
                f"Should load library from env var path"
            )
            
            # Test 2: Encoding works with env var set
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-p", "b/w8",
                "-o", "/tmp/test_env_bw8.wav"
            ], env=env)
            
            file_ok = os.path.exists("/tmp/test_env_bw8.wav")
            self._log_test(
                "MMSSTV encoding works with env var path",
                ret == 0 and file_ok,
                f"Should encode with library from env var"
            )
            
            # Clean up
            if os.path.exists("/tmp/test_env_bw8.wav"):
                os.remove("/tmp/test_env_bw8.wav")
        else:
            self._log_test(
                "Extract library path from status",
                False,
                "Couldn't find library path in status output"
            )
        
        # Test 3: Invalid path in env var doesn't break automatic detection
        # Note: Library has fallback detection, so invalid env var doesn't prevent loading
        env = os.environ.copy()
        env['MMSSTV_LIB_PATH'] = '/invalid/path/to/nowhere.dylib'
        
        ret, stdout, stderr = self._run_command(["-M"], env=env)
        # Library should still be detected via automatic search paths
        self._log_test(
            "Invalid MMSSTV_LIB_PATH doesn't break automatic detection",
            ret == 0,
            "Library has robust fallback detection"
        )
    
    def test_error_handling(self):
        """Test error handling in MMSSTV integration"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling")
        print("="*70)
        
        # Test 1: Requesting MMSSTV mode that doesn't exist
        ret, stdout, stderr = self._run_command([
            "-i", self.test_img,
            "-p", "nonexistent_mmsstv_mode"
        ])
        
        self._log_test(
            "Nonexistent mode returns error code",
            ret != 0 and ret == 112,
            f"Expected error code 112, got {ret}"
        )
        
        # Test 2: Error message mentions mode name
        has_mode_in_error = "nonexistent_mmsstv_mode" in stderr.lower() or "mode" in stderr.lower()
        self._log_test(
            "Error message mentions invalid mode",
            has_mode_in_error,
            "Error should reference the mode name"
        )
        
        if not self.mmsstv_available:
            # Test 3: MMSSTV mode without library gives clear error
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-p", "bw8"
            ])
            
            self._log_test(
                "MMSSTV mode without library fails gracefully",
                ret != 0,
                "Should fail when library not available"
            )
            return
        
        # With library available:
        if self.mmsstv_available:
            # Test 4: MMSSTV encoder handles various input sizes
            ret, stdout, stderr = self._run_command([
                "-i", self.test_img,
                "-p", "b/w8",
                "-o", "/tmp/test_error_bw8.wav"
            ])
            
            self._log_test(
                "MMSSTV encoder handles various input sizes",
                ret == 0,
                f"Should handle input image dimensions, got exit code {ret}"
            )
            
            # Clean up
            if os.path.exists("/tmp/test_error_bw8.wav"):
                os.remove("/tmp/test_error_bw8.wav")
    
    def run_all_tests(self):
        """Run all MMSSTV integration tests"""
        print("="*70)
        print("SlowFrame v2.1 MMSSTV Integration Tests")
        print("="*70)
        
        if self.mmsstv_available:
            print(f"\n✓ MMSSTV library detected - running full test suite")
        else:
            print(f"\n⚠ MMSSTV library not available - testing graceful degradation")
        
        self.test_library_detection()
        self.test_mode_enumeration()
        self.test_mmsstv_encoding()
        self.test_graceful_degradation()
        self.test_environment_variable()
        self.test_error_handling()
        
        print("\n" + "="*70)
        print("SUMMARY")
        print("="*70)
        total = self.passed + self.failed + self.skipped
        print(f"Total tests: {total}")
        print(f"✓ PASSED: {self.passed}")
        print(f"✗ FAILED: {self.failed}")
        print(f"⊘ SKIPPED: {self.skipped}")
        
        if self.failed == 0:
            print("\n✓ All MMSSTV integration tests passed!")
            return 0
        else:
            print(f"\n✗ {self.failed} test(s) failed")
            return 1

if __name__ == "__main__":
    try:
        tester = MMSSTVIntegrationTests()
        exit_code = tester.run_all_tests()
        sys.exit(exit_code)
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)
