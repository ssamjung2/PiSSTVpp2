#!/usr/bin/env python3
"""
Master Test Runner for SlowFrame Comprehensive Testing

NOTE: This test requires Pillow for image validation.
Run with: venv/bin/python3 tests/run_master_tests.py

Orchestrates and runs all test suites:- Aspect ratio testing (CENTER, PAD, STRETCH modes)
- Image format support (PNG, JPEG, GIF, BMP, TIFF, WebP)
- Text overlay functionality

Generates comprehensive report with pass/fail status and identified issues.

Usage:
    python3 run_master_tests.py
    python3 run_master_tests.py --verbose
    python3 run_master_tests.py --suite aspect --suite formats
    python3 run_master_tests.py --report-file test_report.json
"""

import sys
import os
import json
import time
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional
import subprocess

# Add util to path
sys.path.insert(0, str(Path(__file__).parent))


@dataclass
class TestSuiteResult:
    """Result of a test suite"""
    name: str
    passed: int
    failed: int
    total: int
    execution_time: float
    details: dict
    issues: List[str]


class MasterTestRunner:
    """Orchestrates all SlowFrame test suites"""
    
    def __init__(self, 
                 image_dir: str = "tests/images",
                 output_dir: str = "tests/test_outputs",
                 slowframe_bin: str = "./bin/slowframe",
                 verbose: bool = False):
        """
        Initialize master test runner
        
        Args:
            image_dir: Directory with test images
            output_dir: Base output directory for test results
            slowframe_bin: Path to slowframe binary
            verbose: Enable verbose output
        """
        self.image_dir = Path(image_dir)
        self.output_dir = Path(output_dir)
        self.slowframe_bin = Path(slowframe_bin)
        self.verbose = verbose
        
        # Create output directory structure
        self.output_dir.mkdir(parents=True, exist_ok=True)
        (self.output_dir / "aspect").mkdir(exist_ok=True)
        (self.output_dir / "formats").mkdir(exist_ok=True)
        (self.output_dir / "overlay").mkdir(exist_ok=True)
        
        # Verify binary exists
        if not self.slowframe_bin.exists():
            raise FileNotFoundError(f"SlowFrame binary not found: {self.slowframe_bin}")
    
    def run_aspect_tests(self) -> TestSuiteResult:
        """Run comprehensive aspect ratio tests"""
        print("\n" + "="*70)
        print("ASPECT RATIO TEST SUITE")
        print("="*70)
        
        start_time = time.time()
        issues = []
        
        # Import and run aspect test suite
        try:
            sys.path.insert(0, str(Path(__file__).parent))
            from test_aspect_comprehensive import AspectTestSuite
            
            suite = AspectTestSuite(
                str(self.image_dir),
                str(self.output_dir / "aspect"),
                str(self.slowframe_bin),
                self.verbose
            )
            
            results = suite.run_all_tests()
            
            # Collect issues from failures
            for result in results['results']:
                if not result.passed:
                    issues.append(f"Aspect {result.test_case.mode.upper()}: {result.message}")
            
            return TestSuiteResult(
                name="Aspect Ratio",
                passed=results['passed'],
                failed=results['failed'],
                total=results['total'],
                execution_time=time.time() - start_time,
                details={
                    'by_mode': results['by_mode'],
                    'results': results['results'],  # Store full results for annotation
                },
                issues=issues
            )
            
        except Exception as e:
            return TestSuiteResult(
                name="Aspect Ratio",
                passed=0,
                failed=0,
                total=0,
                execution_time=time.time() - start_time,
                details={},
                issues=[f"Suite execution failed: {e}"]
            )
    
    def run_format_tests(self) -> TestSuiteResult:
        """Run image format support tests"""
        print("\n" + "="*70)
        print("IMAGE FORMAT TEST SUITE")
        print("="*70)
        
        start_time = time.time()
        issues = []
        
        try:
            from format_validator import FormatValidator
            
            validator = FormatValidator(str(self.slowframe_bin), self.verbose)
            results = validator.test_all_formats(
                str(self.image_dir),
                str(self.output_dir / "formats")
            )
            
            # Collect issues from failures
            for result in results['results']:
                if not result.passed:
                    issues.append(f"Format {result.format}: {result.message} ({Path(result.file_path).name})")
            
            # Check format coverage
            missing_formats = set(results['supported_formats']) - set(results['format_coverage'])
            if missing_formats:
                issues.append(f"Missing test coverage for formats: {', '.join(missing_formats)}")
            
            return TestSuiteResult(
                name="Image Formats",
                passed=results['passed'],
                failed=results['failed'],
                total=results['total'],
                execution_time=time.time() - start_time,
                details={
                    'format_coverage': results['format_coverage'],
                    'supported_formats': results['supported_formats'],
                },
                issues=issues
            )
            
        except Exception as e:
            return TestSuiteResult(
                name="Image Formats",
                passed=0,
                failed=0,
                total=0,
                execution_time=time.time() - start_time,
                details={},
                issues=[f"Suite execution failed: {e}"]
            )
    
    def run_overlay_tests(self) -> TestSuiteResult:
        """Run text overlay functionality tests"""
        print("\n" + "="*70)
        print("TEXT OVERLAY TEST SUITE")
        print("="*70)
        
        start_time = time.time()
        issues = []
        
        # Simple overlay tests using slowframe directly
        test_cases = [
            {
                'name': 'simple_text',
                'spec': 'N0CALL',
                'description': 'Simple text overlay'
            },
            {
                'name': 'colored_text',
                'spec': 'TEST|color=yellow',
                'description': 'Yellow text overlay'
            },
            {
                'name': 'positioned_text',
                'spec': 'BOTTOM|placement=bottom',
                'description': 'Bottom-positioned text'
            },
            {
                'name': 'sized_text',
                'spec': 'BIG|size=32',
                'description': 'Large text overlay'
            },
        ]
        
        passed = 0
        failed = 0
        
        # Get a test image
        test_image = None
        for img in self.image_dir.glob("*.png"):
            test_image = img
            break
        
        if not test_image:
            return TestSuiteResult(
                name="Text Overlay",
                passed=0,
                failed=0,
                total=0,
                execution_time=time.time() - start_time,
                details={},
                issues=["No test images found"]
            )
        
        for test_case in test_cases:
            if self.verbose:
                print(f"\nTesting: {test_case['description']}")
            
            output_file = self.output_dir / "overlay" / f"{test_case['name']}.wav"
            
            cmd = [
                str(self.slowframe_bin),
                '-i', str(test_image),
                '-p', 'm1',
                '-T', test_case['spec'],
                '-o', str(output_file),
                '-K',  # Keep intermediate images
            ]
            
            try:
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                
                if result.returncode == 0:
                    passed += 1
                    if self.verbose:
                        print(f"  ✓ {test_case['description']}")
                else:
                    failed += 1
                    error = self._extract_error(result.stderr + result.stdout)
                    issues.append(f"Overlay '{test_case['spec']}': {error}")
                    if self.verbose:
                        print(f"  ✗ {test_case['description']}: {error}")
                        
            except subprocess.TimeoutExpired:
                failed += 1
                issues.append(f"Overlay '{test_case['spec']}': Timeout")
                if self.verbose:
                    print(f"  ✗ {test_case['description']}: Timeout")
            except Exception as e:
                failed += 1
                issues.append(f"Overlay '{test_case['spec']}': {e}")
                if self.verbose:
                    print(f"  ✗ {test_case['description']}: {e}")
        
        total = len(test_cases)
        
        return TestSuiteResult(
            name="Text Overlay",
            passed=passed,
            failed=failed,
            total=total,
            execution_time=time.time() - start_time,
            details={'test_cases': len(test_cases)},
            issues=issues
        )
    
    def run_all_suites(self, suites: Optional[List[str]] = None) -> Dict:
        """
        Run selected or all test suites
        
        Args:
            suites: List of suite names to run, or None for all
            
        Returns:
            Dictionary with overall results
        """
        if suites is None:
            suites = ['aspect', 'formats', 'overlay']
        
        print("="*70)
        print("SLOWFRAME COMPREHENSIVE TEST SUITE")
        print("="*70)
        print(f"\nRunning suites: {', '.join(suites)}")
        print(f"Image directory: {self.image_dir}")
        print(f"Output directory: {self.output_dir}")
        print(f"Binary: {self.slowframe_bin}")
        
        start_time = time.time()
        suite_results = []
        
        # Run selected suites
        if 'aspect' in suites:
            aspect_result = self.run_aspect_tests()
            suite_results.append(aspect_result)
            
            # Annotate aspect test images with test numbers and CLI commands
            print("\nAnnotating aspect test images...")
            try:
                sys.path.insert(0, str(Path(__file__).parent))
                from annotate_test_images import TestImageAnnotator
                
                annotator = TestImageAnnotator(str(self.output_dir / "aspect"))
                annotated = 0
                
                # Annotate images from aspect test results
                if 'results' in aspect_result.details:
                    for result in aspect_result.details['results']:
                        if hasattr(result, 'details') and result.details:
                            output_image = result.details.get('output_image')
                            if output_image:
                                if annotator.annotate_image(Path(output_image), result.details):
                                    annotated += 1
                
                if annotated > 0:
                    print(f"  ✓ Annotated {annotated} aspect test images")
            except Exception as e:
                print(f"  Warning: Could not annotate images: {e}", file=sys.stderr)
        
        if 'formats' in suites:
            suite_results.append(self.run_format_tests())
        
        if 'overlay' in suites:
            suite_results.append(self.run_overlay_tests())
        
        total_time = time.time() - start_time
        
        # Compile overall summary
        total_passed = sum(r.passed for r in suite_results)
        total_failed = sum(r.failed for r in suite_results)
        total_tests = sum(r.total for r in suite_results)
        all_issues = []
        for r in suite_results:
            all_issues.extend(r.issues)
        
        return {
            'overall': {
                'passed': total_passed,
                'failed': total_failed,
                'total': total_tests,
                'pass_rate': (total_passed / total_tests * 100) if total_tests > 0 else 0,
                'total_time': total_time,
            },
            'suites': suite_results,
            'issues': all_issues,
        }
    
    def _extract_error(self, output: str) -> str:
        """Extract error message from output"""
        lines = output.split('\n')
        for line in lines:
            if '[ERROR]' in line:
                # Extract the error message part
                if 'Error code' in line:
                    return line.split(':', 1)[1].strip() if ':' in line else line.strip()
                return line.strip()
        return "Unknown error"


def print_summary(results: Dict, verbose: bool = False):
    """Print comprehensive test summary"""
    print("\n" + "="*70)
    print("COMPREHENSIVE TEST SUMMARY")
    print("="*70)
    
    overall = results['overall']
    print(f"\nOverall Results:")
    print(f"  Total Tests: {overall['total']}")
    print(f"  Passed:      {overall['passed']} ({overall['pass_rate']:.1f}%)")
    print(f"  Failed:      {overall['failed']}")
    print(f"  Execution:   {overall['total_time']:.2f}s")
    
    print(f"\nSuite Breakdown:")
    for suite in results['suites']:
        status = "✓" if suite.failed == 0 else "✗"
        print(f"  {status} {suite.name:15s}: {suite.passed}/{suite.total} passed ({suite.execution_time:.2f}s)")
        
        if verbose and suite.details:
            for key, value in suite.details.items():
                print(f"      {key}: {value}")
    
    # Show issues
    if results['issues']:
        print(f"\nIssues Found ({len(results['issues'])}):")
        for i, issue in enumerate(results['issues'][:20], 1):  # Limit to first 20
            print(f"  {i}. {issue}")
        
        if len(results['issues']) > 20:
            print(f"  ... and {len(results['issues']) - 20} more issues")
    else:
        print("\n✓ No issues found - all tests passed!")
    
    print("="*70)


def save_report(results: Dict, report_file: str):
    """Save test results to JSON file"""
    output = {
        'overall': results['overall'],
        'suites': [
            {
                'name': s.name,
                'passed': s.passed,
                'failed': s.failed,
                'total': s.total,
                'execution_time': s.execution_time,
                'details': s.details,
                'issues': s.issues,
            }
            for s in results['suites']
        ],
        'issues': results['issues'],
    }
    
    with open(report_file, 'w') as f:
        json.dump(output, f, indent=2)
    
    print(f"\nDetailed report saved to: {report_file}")


def main():
    """Command-line interface"""
    import argparse
    
    parser = argparse.ArgumentParser(description="SlowFrame comprehensive test suite")
    parser.add_argument("--image-dir", default="tests/images", help="Test image directory")
    parser.add_argument("--output-dir", default="tests/test_outputs", help="Output directory")
    parser.add_argument("--binary", default="./bin/slowframe", help="Path to slowframe binary")
    parser.add_argument("--suite", action="append", choices=['aspect', 'formats', 'overlay'],
                       help="Specific suite(s) to run (can be repeated)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--report-file", help="Save JSON report to file")
    
    args = parser.parse_args()
    
    try:
        runner = MasterTestRunner(
            args.image_dir,
            args.output_dir,
            args.binary,
            args.verbose
        )
        
        results = runner.run_all_suites(args.suite)
        
        print_summary(results, args.verbose)
        
        if args.report_file:
            save_report(results, args.report_file)
        
        # Exit with failure code if any tests failed
        sys.exit(0 if results['overall']['failed'] == 0 else 1)
        
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(2)
    except Exception as e:
        print(f"Fatal error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(2)


if __name__ == "__main__":
    main()
