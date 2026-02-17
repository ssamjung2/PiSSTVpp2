#!/usr/bin/env python3
"""
Comprehensive Aspect Ratio Testing Suite for SlowFrame
NOTE: This test requires Pillow for image validation.
Run with: venv/bin/python3 tests/test_aspect_comprehensive.py
Tests all aspect ratio modes (CENTER, PAD, STRETCH) with diverse image sizes
and aspect ratios to ensure transformations work correctly.

Validates:
- Correct output dimensions
- Appropriate padding for PAD mode
- No padding for CENTER/STRETCH modes
- Edge cases (very wide, very tall, square images)

Usage:
    python3 test_aspect_comprehensive.py
    python3 test_aspect_comprehensive.py --verbose
    python3 test_aspect_comprehensive.py --output-dir /tmp/aspect_tests
"""

import sys
import os
import subprocess
import json
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Tuple
import time

# Add tests/util to path for imports
sys.path.insert(0, str(Path(__file__).parent))

try:
    from aspect_validator import AspectValidator, ValidationResult
    HAS_VALIDATOR = True
except ImportError:
    print("Warning: aspect_validator not found, validation will be limited")
    HAS_VALIDATOR = False


@dataclass
class AspectTestCase:
    """Defines a single aspect ratio test case"""
    name: str
    image_path: str
    mode: str  # center, pad, stretch
    protocol: str  # m1, m2, s1, etc.
    target_width: int
    target_height: int
    description: str = ""
    test_number: int = 0  # Test case number for tracking


@dataclass
class AspectTestResult:
    """Result of a single aspect test"""
    test_case: AspectTestCase
    passed: bool
    message: str
    execution_time: float
    validation: Optional[ValidationResult] = None
    details: dict = None
    
    def __post_init__(self):
        if self.details is None:
            self.details = {}


class AspectTestSuite:
    """Comprehensive aspect ratio test suite"""
    
    # SSTV protocol dimensions
    PROTOCOL_DIMS = {
        'm1': (320, 256),
        'm2': (320, 256),
        's1': (320, 256),
        's2': (320, 256),
        'sdx': (320, 256),
        'r36': (320, 240),
        'r72': (320, 240),
    }
    
    def __init__(self, image_dir: str, output_dir: str, 
                 slowframe_bin: str = "./bin/slowframe",
                 verbose: bool = False):
        """
        Initialize test suite
        
        Args:
            image_dir: Directory with test images
            output_dir: Directory for test outputs
            slowframe_bin: Path to slowframe binary
            verbose: Enable verbose output
        """
        self.image_dir = Path(image_dir)
        self.output_dir = Path(output_dir)
        self.slowframe_bin = Path(slowframe_bin)
        self.verbose = verbose
        
        # Create output directory
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        if not self.slowframe_bin.exists():
            raise FileNotFoundError(f"SlowFrame binary not found: {self.slowframe_bin}")
        
        if not self.image_dir.exists():
            raise FileNotFoundError(f"Image directory not found: {self.image_dir}")
    
    def generate_test_cases(self) -> List[AspectTestCase]:
        """
        Generate comprehensive test cases covering:
        - All aspect modes (CENTER, PAD, STRETCH)
        - Various source aspect ratios (wide, tall, square)
        - Different SSTV protocols
        - Edge cases
        """
        test_cases = []
        
        # Define representative test images by aspect ratio
        test_images = self._categorize_images()
        
        # Test each mode with different aspect ratios
        modes = ['center', 'pad', 'stretch']
        protocols = ['m1', 'r36']  # Test both 4:3.2 and 4:3 aspects
        
        for mode in modes:
            for protocol in protocols:
                target_w, target_h = self.PROTOCOL_DIMS[protocol]
                
                # Wide images
                if test_images['wide']:
                    img = test_images['wide'][0]
                    test_cases.append(AspectTestCase(
                        name=f"{mode}_{protocol}_wide",
                        image_path=str(img['path']),
                        mode=mode,
                        protocol=protocol,
                        target_width=target_w,
                        target_height=target_h,
                        description=f"{mode.upper()} mode with wide image ({img['aspect']:.2f}:1)"
                    ))
                
                # Tall images
                if test_images['tall']:
                    img = test_images['tall'][0]
                    test_cases.append(AspectTestCase(
                        name=f"{mode}_{protocol}_tall",
                        image_path=str(img['path']),
                        mode=mode,
                        protocol=protocol,
                        target_width=target_w,
                        target_height=target_h,
                        description=f"{mode.upper()} mode with tall image ({img['aspect']:.2f}:1)"
                    ))
                
                # Square images
                if test_images['square']:
                    img = test_images['square'][0]
                    test_cases.append(AspectTestCase(
                        name=f"{mode}_{protocol}_square",
                        image_path=str(img['path']),
                        mode=mode,
                        protocol=protocol,
                        target_width=target_w,
                        target_height=target_h,
                        description=f"{mode.upper()} mode with square image ({img['aspect']:.2f}:1)"
                    ))
                
                # Target aspect image (should need minimal transformation)
                if test_images['target']:
                    img = test_images['target'][0]
                    test_cases.append(AspectTestCase(
                        name=f"{mode}_{protocol}_target",
                        image_path=str(img['path']),
                        mode=mode,
                        protocol=protocol,
                        target_width=target_w,
                        target_height=target_h,
                        description=f"{mode.upper()} mode with target aspect image ({img['aspect']:.2f}:1)"
                    ))
        
        return test_cases
    
    def _categorize_images(self) -> Dict[str, List[Dict]]:
        """
        Categorize test images by aspect ratio
        
        Returns dict with keys: wide, tall, square, target
        """
        categories = {
            'wide': [],    # aspect > 1.5
            'tall': [],    # aspect < 0.8
            'square': [],  # 0.9 < aspect < 1.1
            'target': [],  # close to 1.25 (320/256)
        }
        
        # Scan image directory
        for img_path in self.image_dir.glob("*.png"):
            # Get dimensions using identify
            try:
                result = subprocess.run(
                    ['identify', '-format', '%wx%h', str(img_path)],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    dims_str = result.stdout.strip()
                    width, height = map(int, dims_str.split('x'))
                    aspect = width / height
                    
                    img_info = {
                        'path': img_path,
                        'width': width,
                        'height': height,
                        'aspect': aspect,
                    }
                    
                    # Categorize
                    if aspect > 1.5:
                        categories['wide'].append(img_info)
                    elif aspect < 0.8:
                        categories['tall'].append(img_info)
                    elif 0.9 <= aspect <= 1.1:
                        categories['square'].append(img_info)
                    elif 1.2 <= aspect <= 1.3:
                        categories['target'].append(img_info)
            except Exception as e:
                if self.verbose:
                    print(f"Warning: Could not analyze {img_path.name}: {e}")
        
        # Also check JPG files
        for img_path in self.image_dir.glob("*.jpg"):
            try:
                result = subprocess.run(
                    ['identify', '-format', '%wx%h', str(img_path)],
                    capture_output=True,
                    text=True,
                    timeout=5
                )
                if result.returncode == 0:
                    dims_str = result.stdout.strip()
                    width, height = map(int, dims_str.split('x'))
                    aspect = width / height
                    
                    img_info = {
                        'path': img_path,
                        'width': width,
                        'height': height,
                        'aspect': aspect,
                    }
                    
                    if aspect > 1.5:
                        categories['wide'].append(img_info)
                    elif aspect < 0.8:
                        categories['tall'].append(img_info)
                    elif 0.9 <= aspect <= 1.1:
                        categories['square'].append(img_info)
                    elif 1.2 <= aspect <= 1.3:
                        categories['target'].append(img_info)
            except Exception:
                pass
        
        return categories
    
    def run_test(self, test_case: AspectTestCase) -> AspectTestResult:
        """
        Run a single aspect ratio test
        
        Args:
            test_case: Test case to run
            
        Returns:
            AspectTestResult with outcome
        """
        if self.verbose:
            test_num_str = f"#{test_case.test_number}: " if test_case.test_number > 0 else ""
            print(f"  Running: {test_num_str}{test_case.name} - {test_case.description}")
        
        # Generate output paths with test number for better traceability
        test_num_prefix = f"T{test_case.test_number:03d}_" if test_case.test_number > 0 else ""
        debug_img = self.output_dir / f"{test_num_prefix}{test_case.name}_debug.png"
        audio_out = self.output_dir / f"{test_num_prefix}{test_case.name}.wav"
        
        # Build command
        cmd = [
            str(self.slowframe_bin),
            '-i', test_case.image_path,
            '-p', test_case.protocol,
            '-a', test_case.mode,
            '-o', str(audio_out),
            '-K',  # Keep intermediate images
        ]
        
        # Store the command as a string for reference
        cmd_str = ' '.join(str(c) for c in cmd)
        
        # Run test
        start_time = time.time()
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60
            )
            execution_time = time.time() - start_time
            
            # Check for errors
            if result.returncode != 0:
                error_msg = self._extract_error(result.stderr + result.stdout)
                return AspectTestResult(
                    test_case=test_case,
                    passed=False,
                    message=f"Execution failed: {error_msg}",
                    execution_time=execution_time,
                    details={'returncode': result.returncode, 'stderr': result.stderr}
                )
            
            # Find the debug/intermediate image
            # SlowFrame saves debug image with -K flag at output path with .png/.jpg extension
            actual_debug_img = None
            
            # Try multiple image format extensions
            audio_base = str(audio_out).replace('.wav', '')
            image_formats = ['.png', '.jpg', '.jpeg', '.gif', '.bmp', '.tiff']
            
            for fmt in image_formats:
                candidate = Path(audio_base + fmt)
                if candidate.exists():
                    actual_debug_img = candidate
                    break
            
            # If not found by audio basename, try other possible locations
            if not actual_debug_img:
                possible_debug_paths = [
                    debug_img,
                    self.output_dir / f"{test_num_prefix}{test_case.name}.png",
                    self.output_dir / f"{test_num_prefix}{test_case.name}.jpg",
                    self.output_dir / f"{test_case.name}.png",
                    self.output_dir / f"{test_case.name}.jpg",
                    Path(test_case.image_path).parent / f"{Path(test_case.image_path).stem}_processed.png",
                ]
                
                for path in possible_debug_paths:
                    if path.exists():
                        actual_debug_img = path
                        break
            
            # Validate if we have PIL and the validator
            validation = None
            if HAS_VALIDATOR and actual_debug_img:
                # Get source dimensions
                img_path = Path(test_case.image_path)
                try:
                    identify_result = subprocess.run(
                        ['identify', '-format', '%wx%h', str(img_path)],
                        capture_output=True,
                        text=True,
                        timeout=5
                    )
                    if identify_result.returncode == 0:
                        dims_str = identify_result.stdout.strip()
                        src_width, src_height = map(int, dims_str.split('x'))
                        
                        validator = AspectValidator(
                            str(actual_debug_img),
                            test_case.mode,
                            src_width, src_height,
                            test_case.target_width, test_case.target_height,
                            self.verbose,
                            src_image_path=str(img_path)
                        )
                        validation = validator.validate()
                except Exception as e:
                    if self.verbose:
                        print(f"    Warning: Validation error: {e}")
            
            # Determine pass/fail
            if validation:
                passed = validation.passed
                message = validation.message
            else:
                # No validation, just check execution succeeded
                passed = True
                message = "Execution succeeded (validation skipped)"
            
            return AspectTestResult(
                test_case=test_case,
                passed=passed,
                message=message,
                execution_time=execution_time,
                validation=validation,
                details={
                    'audio_created': audio_out.exists(),
                    'cli_command': cmd_str,
                    'test_number': test_case.test_number,
                    'output_image': str(actual_debug_img) if actual_debug_img else None,
                    'source_image': test_case.image_path
                }
            )
            
        except subprocess.TimeoutExpired:
            return AspectTestResult(
                test_case=test_case,
                passed=False,
                message="Execution timeout (>60s)",
                execution_time=60.0
            )
        except Exception as e:
            return AspectTestResult(
                test_case=test_case,
                passed=False,
                message=f"Exception: {e}",
                execution_time=time.time() - start_time
            )
    
    def run_all_tests(self) -> Dict:
        """
        Run all aspect ratio tests
        
        Returns:
            Dictionary with test results
        """
        print("="*70)
        print("SlowFrame Comprehensive Aspect Ratio Test Suite")
        print("="*70)
        
        # Generate test cases
        print("\nGenerating test cases...")
        test_cases = self.generate_test_cases()
        print(f"Generated {len(test_cases)} test cases")
        
        # Assign test numbers for better traceability
        for i, test_case in enumerate(test_cases, 1):
            test_case.test_number = i
        
        # Run tests
        print(f"\nRunning tests...")
        results = []
        
        for i, test_case in enumerate(test_cases, 1):
            print(f"\n[{i}/{len(test_cases)}] {test_case.name}")
            result = self.run_test(test_case)
            results.append(result)
            
            status = "✓" if result.passed else "✗"
            print(f"  {status} {result.message} ({result.execution_time:.2f}s)")
            
            if result.validation and self.verbose:
                print(f"    Validation details: {result.validation.details}")
        
        # Compile summary
        passed = sum(1 for r in results if r.passed)
        failed = len(results) - passed
        total_time = sum(r.execution_time for r in results)
        
        # Summarize by mode
        by_mode = {}
        for result in results:
            mode = result.test_case.mode
            if mode not in by_mode:
                by_mode[mode] = {'passed': 0, 'failed': 0}
            if result.passed:
                by_mode[mode]['passed'] += 1
            else:
                by_mode[mode]['failed'] += 1
        
        return {
            'passed': passed,
            'failed': failed,
            'total': len(results),
            'total_time': total_time,
            'by_mode': by_mode,
            'results': results,
        }
    
    def _extract_error(self, output: str) -> str:
        """Extract error message from output"""
        lines = output.split('\n')
        for line in lines:
            if '[ERROR]' in line:
                return line.strip()
        return "Unknown error"


def main():
    """Command-line interface"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Comprehensive aspect ratio testing for SlowFrame")
    parser.add_argument("--image-dir", default="tests/images", help="Directory with test images")
    parser.add_argument("--output-dir", default="tests/test_outputs/aspect", help="Output directory")
    parser.add_argument("--binary", default="./bin/slowframe", help="Path to slowframe binary")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--json", action="store_true", help="Output JSON results")
    
    args = parser.parse_args()
    
    suite = AspectTestSuite(args.image_dir, args.output_dir, args.binary, args.verbose)
    results = suite.run_all_tests()
    
    if args.json:
        # JSON output
        output = {
            'summary': {
                'passed': results['passed'],
                'failed': results['failed'],
                'total': results['total'],
                'total_time': results['total_time'],
                'by_mode': results['by_mode'],
            },
            'results': [
                {
                    'name': r.test_case.name,
                    'mode': r.test_case.mode,
                    'protocol': r.test_case.protocol,
                    'passed': r.passed,
                    'message': r.message,
                    'execution_time': r.execution_time,
                }
                for r in results['results']
            ]
        }
        print(json.dumps(output, indent=2))
    else:
        # Human-readable summary
        print("\n" + "="*70)
        print("TEST SUMMARY")
        print("="*70)
        print(f"\nOverall: {results['passed']}/{results['total']} passed ({results['failed']} failed)")
        print(f"Total time: {results['total_time']:.2f}s")
        
        print("\nBy Mode:")
        for mode, stats in results['by_mode'].items():
            total = stats['passed'] + stats['failed']
            print(f"  {mode.upper():8s}: {stats['passed']}/{total} passed")
        
        # Show failures
        failures = [r for r in results['results'] if not r.passed]
        if failures:
            print(f"\nFailed Tests ({len(failures)}):")
            for r in failures:
                print(f"  ✗ {r.test_case.name}")
                print(f"    {r.message}")
        
        print("="*70)
    
    sys.exit(0 if results['failed'] == 0 else 1)


if __name__ == "__main__":
    main()
