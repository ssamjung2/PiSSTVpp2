#!/usr/bin/env python3
"""
SSTV Sticher Robustness Test Suite

Tests the stitch_tiles binary's ability to handle corrupted/noisy SSTV tiles.
Uses generate_sstv_noise.py to create systematic test scenarios representing
real-world ham radio interference conditions.

Scenarios tested:
1. Clean signal (baseline)
2. Light QRM (occasional interference)
3. Medium QRM (noticeable corruption)
4. Heavy QRM (severe degradation)
5. Extreme QRM (near-unrecoverable)
6. Fading only (ionospheric effects)
7. Header-focused corruption (header data loss)

Usage:
    python3 test_stitcher_robustness.py [options]

Examples:
    # Run all tests
    python3 test_stitcher_robustness.py
    
    # Run specific test preset
    python3 test_stitcher_robustness.py --preset medium_qrm
    
    # Generate fresh noise, then test
    python3 test_stitcher_robustness.py --regen-noise
    
    # Verbose output with detailed diagnostics
    python3 test_stitcher_robustness.py --verbose
"""

import os
import sys
import subprocess
import json
from pathlib import Path
from datetime import datetime
from dataclasses import dataclass
from typing import Optional, List, Dict, Tuple


@dataclass
class RobustnessTest:
    """Single robustness test case"""
    name: str
    preset: str
    description: str
    success: bool
    error_msg: Optional[str] = None
    output_file: Optional[str] = None
    output_size: Optional[int] = None
    duration_sec: Optional[float] = None
    stitcher_output: str = ""


class StitcherRobustnessTestSuite:
    """Test suite for stitcher robustness against noise/corruption"""

    def __init__(self, project_root: Optional[str] = None):
        """Initialize test suite"""
        if project_root is None:
            script_dir = Path(__file__).parent.parent.parent
            self.project_root = script_dir
        else:
            self.project_root = Path(project_root)

        self.bin_dir = self.project_root / "bin"
        self.stitch_binary = self.bin_dir / "stitch_tiles"
        self.test_util_dir = self.project_root / "tests" / "util"
        self.test_data_dir = self.project_root / "tests" / "test_outputs" / "tiling"
        self.test_output_base = self.project_root / "tests" / "test_outputs"
        self.noise_generator = self.test_util_dir / "generate_sstv_noise.py"

        # Get Python executable from venv
        venv_bin = self.project_root / "venv" / "bin"
        if venv_bin.exists():
            self.python = str(venv_bin / "python")
        else:
            self.python = "python3"

        self.tests: List[RobustnessTest] = []

    def validate_environment(self) -> bool:
        """Verify required files exist"""
        checks = [
            (self.stitch_binary, "stitch_tiles binary"),
            (self.test_data_dir, "test data directory"),
            (self.noise_generator, "noise generator"),
        ]

        all_ok = True
        for path, name in checks:
            if not path.exists():
                print(f"✗ Missing: {name} at {path}")
                all_ok = False
            else:
                print(f"✓ Found: {name}")

        return all_ok

    def generate_noise_variants(self, preset: str, force: bool = False) -> bool:
        """Generate noise-corrupted tile variants using specified preset"""
        output_dir = self.test_output_base / f"noisy_tiles_{preset}"

        # Skip if already exists and not forcing regeneration
        if output_dir.exists() and not force:
            tile_count = len(list(output_dir.glob("*.png")))
            if tile_count > 0:
                print(f"  Using existing {preset} variants ({tile_count} tiles)")
                return True

        print(f"  Generating {preset} variants...")
        output_dir.mkdir(parents=True, exist_ok=True)

        cmd = [
            self.python,
            str(self.noise_generator),
            "--dir", str(self.test_data_dir),
            "--output-dir", str(output_dir),
            "--preset", preset,
        ]

        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=60
            )
            if result.returncode == 0:
                tile_count = len(list(output_dir.glob("*.png")))
                print(f"  ✓ Generated {tile_count} noisy tiles")
                return True
            else:
                print(f"  ✗ Generation failed: {result.stderr}")
                return False
        except subprocess.TimeoutExpired:
            print(f"  ✗ Generation timeout")
            return False

    def run_stitch_test(
        self, test_dir: Path, session_id: str, test_name: str
    ) -> Tuple[bool, Optional[str], Optional[float]]:
        """
        Run stitch_tiles on a test directory
        
        Returns: (success, output_path, duration_sec)
        """
        output_file = self.test_output_base / f"robustness_test_{test_name}.png"

        cmd = [
            str(self.stitch_binary),
            "--dir", str(test_dir),
            "--session", session_id,
            "-o", str(output_file),
            "--verbose",
        ]

        start_time = datetime.now()
        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True, timeout=30
            )
            duration = (datetime.now() - start_time).total_seconds()

            if result.returncode == 0 and output_file.exists():
                return True, str(output_file), duration
            else:
                return False, None, duration
        except subprocess.TimeoutExpired:
            return False, None, 30.0

    def run_test_preset(self, preset: str, session_id: str = "0x69A39092") -> RobustnessTest:
        """Run stitching test on a specific noise preset"""
        noise_dir = self.test_output_base / f"noisy_tiles_{preset}"

        if not noise_dir.exists():
            return RobustnessTest(
                name=preset,
                preset=preset,
                description=f"Test with {preset} noise",
                success=False,
                error_msg=f"Noise directory not found: {noise_dir}",
            )

        test_name = f"robustness_{preset}_{datetime.now().strftime('%s')}"
        success, output_file, duration = self.run_stitch_test(
            noise_dir, session_id, test_name
        )

        output_size = None
        if output_file and Path(output_file).exists():
            output_size = Path(output_file).stat().st_size

        test = RobustnessTest(
            name=preset,
            preset=preset,
            description=f"Stitching with {preset} noise interference",
            success=success,
            output_file=output_file,
            output_size=output_size,
            duration_sec=duration,
        )

        if not success:
            test.error_msg = "Stitching failed"

        return test

    def run_all_tests(
        self,
        presets: Optional[List[str]] = None,
        regen_noise: bool = False,
        verbose: bool = False,
    ) -> List[RobustnessTest]:
        """
        Run complete robustness test suite
        
        Args:
            presets: List of noise presets to test (None = all)
            regen_noise: Force regeneration of noise tiles
            verbose: Verbose output
        
        Returns:
            List of test results
        """
        if presets is None:
            presets = [
                "clean",
                "light_qrm",
                "medium_qrm",
                "heavy_qrm",
                "header_corruption",
            ]

        print("\n" + "=" * 75)
        print("SSTV Stitcher Robustness Test Suite")
        print("=" * 75)

        if not self.validate_environment():
            print("\n✗ Environment validation failed")
            return []

        print("\n" + "=" * 75)
        print("Preparing Test Data")
        print("=" * 75)

        for preset in presets:
            if not self.generate_noise_variants(preset, force=regen_noise):
                print(f"Warning: Failed to generate {preset} data")

        print("\n" + "=" * 75)
        print("Running Tests")
        print("=" * 75)

        self.tests = []
        for preset in presets:
            print(f"\nTesting {preset}...")
            test = self.run_test_preset(preset)
            self.tests.append(test)

            if test.success:
                size_mb = test.output_size / (1024 * 1024) if test.output_size else 0
                print(f"  ✓ SUCCESS ({size_mb:.2f} MB, {test.duration_sec:.2f}s)")
            else:
                print(f"  ✗ FAILED: {test.error_msg}")

        return self.tests

    def print_summary(self):
        """Print test results summary"""
        if not self.tests:
            return

        print("\n" + "=" * 75)
        print("Test Results Summary")
        print("=" * 75)

        passed = sum(1 for t in self.tests if t.success)
        failed = sum(1 for t in self.tests if not t.success)

        print(f"\nTotal Tests: {len(self.tests)}")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")
        print(f"Success Rate: {passed/len(self.tests)*100:.1f}%")

        print("\nDetailed Results:")
        print(f"{'Preset':<25} {'Status':<10} {'Size (MB)':<12} {'Time (s)':<10}")
        print("-" * 60)

        for test in self.tests:
            status = "✓ PASS" if test.success else "✗ FAIL"
            size_str = f"{test.output_size / (1024*1024):.2f}" if test.output_size else "N/A"
            time_str = f"{test.duration_sec:.2f}" if test.duration_sec else "N/A"
            print(f"{test.preset:<25} {status:<10} {size_str:<12} {time_str:<10}")

        print("\n" + "=" * 75)
        print("Robustness Assessment")
        print("=" * 75)

        if passed == len(self.tests):
            print("✓ EXCELLENT: Stitcher handles all corruption scenarios gracefully")
        elif passed >= len(self.tests) * 0.8:
            print("✓ GOOD: Stitcher handles most scenarios, minor issues with extreme cases")
        elif passed >= len(self.tests) * 0.5:
            print("⚠ FAIR: Stitcher handles common cases, struggles with heavy corruption")
        else:
            print("✗ POOR: Stitcher unable to handle moderate corruption")

        # Print failure details
        failures = [t for t in self.tests if not t.success]
        if failures:
            print("\nFailed Tests:")
            for test in failures:
                print(f"  - {test.preset}: {test.error_msg}")

    def save_report(self, output_file: Optional[str] = None) -> str:
        """Save test results to JSON file"""
        if output_file is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_file = str(
                self.project_root / f"tests/robustness_report_{timestamp}.json"
            )

        report = {
            "timestamp": datetime.now().isoformat(),
            "total_tests": len(self.tests),
            "passed": sum(1 for t in self.tests if t.success),
            "failed": sum(1 for t in self.tests if not t.success),
            "tests": [
                {
                    "preset": t.preset,
                    "success": t.success,
                    "error": t.error_msg,
                    "output_size": t.output_size,
                    "duration_sec": t.duration_sec,
                }
                for t in self.tests
            ],
        }

        with open(output_file, "w") as f:
            json.dump(report, f, indent=2)

        print(f"\nReport saved: {output_file}")
        return output_file


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description="Test SSTV stitcher robustness against corruption/noise",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run all tests with fresh noise generation
  python3 test_stitcher_robustness.py --regen-noise
  
  # Test specific preset
  python3 test_stitcher_robustness.py --preset medium_qrm heavy_qrm
  
  # Verbose output with detailed diagnostics
  python3 test_stitcher_robustness.py --verbose
  
  # Save results to JSON report
  python3 test_stitcher_robustness.py --report results.json
        """,
    )

    parser.add_argument(
        "--preset",
        nargs="+",
        help="Run specific presets (clean, light_qrm, medium_qrm, heavy_qrm, etc.)",
    )
    parser.add_argument(
        "--regen-noise", action="store_true", help="Regenerate noise tiles"
    )
    parser.add_argument(
        "--report", help="Save JSON report to file"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Verbose output"
    )

    args = parser.parse_args()

    suite = StitcherRobustnessTestSuite()
    results = suite.run_all_tests(
        presets=args.preset,
        regen_noise=args.regen_noise,
        verbose=args.verbose,
    )

    suite.print_summary()

    if args.report:
        suite.save_report(args.report)
    else:
        suite.save_report()


if __name__ == "__main__":
    main()
