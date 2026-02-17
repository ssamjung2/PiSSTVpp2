#!/usr/bin/env python3
"""
Image Format Validator for SlowFrame

Tests that all supported image formats can be loaded and processed correctly.

Week 3 Enhancements:
- Integrated TestReportGenerator for HTML reports with embedded test details
- Integrated HumanVerifier for interactive verification workflow
- Auto-generated verification checklists for image_format category
- Command-line flags: --report, --verify for enhanced output

Usage:
    from format_validator import FormatValidator
    
    validator = FormatValidator(enable_reports=True)
    results = validator.test_all_formats("tests/images")
    print(f"Passed: {results['passed']}/{results['total']}")
"""

import sys
import subprocess
import os
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional, Tuple
import shlex
import json

# Add util directory to path for imports
util_dir = Path(__file__).parent
if util_dir not in sys.path:
    sys.path.insert(0, str(util_dir))

# Week 1 Infrastructure - Report Generation and Verification
from test_report_generator import TestReportGenerator
from human_verification_framework import HumanVerifier
from test_metadata import TestMetadata


@dataclass
class FormatTestResult:
    """Result of testing a single format"""
    format: str
    file_path: str
    passed: bool
    message: str
    dimensions: Tuple[int, int] = (0, 0)
    command: str = ""
    purpose: str = ""
    expected: str = ""
    output_audio_path: str = ""
    output_audio_created: bool = False
    output_image_path: str = ""
    output_image_created: bool = False
    details: dict = None
    
    def __post_init__(self):
        if self.details is None:
            self.details = {}


class FormatValidator:
    """Tests image format support with Week 1 infrastructure integration"""
    
    SUPPORTED_FORMATS = {
        'png': 'PNG',
        'jpg': 'JPEG',
        'jpeg': 'JPEG',
        'gif': 'GIF',
        'bmp': 'BMP',
        'tiff': 'TIFF',
        'tif': 'TIFF',
        'webp': 'WebP',
        'ppm': 'PPM',
    }
    
    def __init__(self, slowframe_bin: str = "./bin/slowframe", verbose: bool = False,
                 enable_reports: bool = False, enable_verification: bool = False,
                 detailed_output: bool = True, pause_between: bool = False,
                 encode_audio: bool = False):
        """
        Initialize format validator with Week 1 infrastructure options
        
        Args:
            slowframe_bin: Path to slowframe binary
            verbose: Enable verbose output
            enable_reports: Generate HTML reports with test details
            enable_verification: Enable interactive human verification workflow
        """
        self.slowframe_bin = Path(slowframe_bin)
        self.verbose = verbose
        self.enable_reports = enable_reports
        self.enable_verification = enable_verification
        self.detailed_output = detailed_output
        self.pause_between = pause_between
        self.encode_audio = encode_audio
        
        if not self.slowframe_bin.exists():
            raise FileNotFoundError(f"SlowFrame binary not found: {self.slowframe_bin}")
        
        # Week 1 Infrastructure - Initialize report generator and verifier
        if self.enable_reports:
            self.report_generator = TestReportGenerator("Image Format Validator Tests")
        else:
            self.report_generator = None
        
        if self.enable_verification:
            session_file = Path.cwd() / "format_verification_session.json"
            self.verifier = HumanVerifier(session_file=str(session_file))
        else:
            self.verifier = None
        
        # Test counter for unique IDs
        self.test_counter = 0
        self.all_results = []
    
    def _create_format_metadata(self, test_id, format_name, file_path):
        """Create TestMetadata for a format validation test
        
        Args:
            test_id: Unique test identifier
            format_name: Format being tested (PNG, JPEG, etc.)
            file_path: Path to test file
        
        Returns:
            TestMetadata instance with image_format category
        """
        return TestMetadata(
            test_id=test_id,
            test_number=int(test_id.lstrip('T')),
            title=f"Format Test - {format_name}",
            category="image_format",
            purpose=f"Validate {format_name} format loading and processing",
            expected_outcome="Image processes successfully with correct dimensions",
            input_image=Path(file_path).name,
            input_width=0,  # Will be updated with actual dimensions
            input_height=0,
            output_width=0,
            output_height=0
        )

    def _format_command(self, cmd: List[str]) -> str:
        return " ".join(shlex.quote(part) for part in cmd)
    
    def _log_test(self, test_id, format_name, file_path, result: 'FormatTestResult'):
        """Log test result and add to reports if enabled"""
        metadata = self._create_format_metadata(test_id, format_name, file_path)
        
        # Add to report generator if enabled
        if self.report_generator is not None:
            try:
                self.report_generator.add_test_result(
                    test_id=test_id,
                    name=f"{format_name} - {Path(file_path).name}",
                    suite="format_validator",
                    status="passed" if result.passed else "failed",
                    command=result.command,
                    metadata=metadata,
                    output_files={
                        "output_audio": result.output_audio_path,
                        "output_image": result.output_image_path
                    } if result.output_audio_path or result.output_image_path else {},
                    automated_checks={
                        "File found": True,
                        "Format recognized": True,
                        "Processing successful": result.passed,
                        "Dimensions valid": result.dimensions != (0, 0),
                        "Output image created": result.output_image_created,
                        "Output audio created": result.output_audio_created,
                    }
                )
            except Exception as e:
                if self.verbose:
                    print(f"  (Report generation warning: {str(e)[:50]})")
    
    def _run_command(self, args):
        """Execute slowframe with given arguments"""
        cmd = [str(self.slowframe_bin)] + args
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "TIMEOUT: Command exceeded 30 seconds"
        except Exception as e:
            return -1, "", f"ERROR: {str(e)}"
    
    def test_format(self, image_path: str, output_dir: str = "tests/test_outputs/format_validator") -> FormatTestResult:
        """
        Test loading and processing a single image format
        
        Args:
            image_path: Path to test image
            output_dir: Directory for output files
            
        Returns:
            FormatTestResult with test outcome
        """
        # Generate test ID
        self.test_counter += 1
        test_id = f"T{self.test_counter:03d}"
        
        img_path = Path(image_path)
        
        if not img_path.exists():
            result = FormatTestResult(
                format="unknown",
                file_path=str(img_path),
                passed=False,
                message=f"File not found: {img_path}"
            )
            self._log_test(test_id, "unknown", str(img_path), result)
            return result
        
        # Determine format from extension
        ext = img_path.suffix.lstrip('.').lower()
        format_name = self.SUPPORTED_FORMATS.get(ext, ext.upper())

        purpose = f"Validate {format_name} format loading and processing"
        expected = "Process succeeds without errors; input dimensions detected"
        
        # Get image dimensions using identify
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
                dimensions = (width, height)
            else:
                dimensions = (0, 0)
        except Exception as e:
            dimensions = (0, 0)
        
        # Test with slowframe
        output_audio_path = Path(output_dir) / f"test_format_{ext}_{img_path.stem}.wav"
        output_image_suffix = "_overlay" if not self.encode_audio else ""
        output_image_path = Path(output_dir) / f"test_format_{ext}_{img_path.stem}{output_image_suffix}{img_path.suffix}"
        
        cmd = [
            str(self.slowframe_bin),
            '-i', str(img_path),
            '-p', 'm1',
            '-o', str(output_audio_path),
            '-K'
        ]
        if not self.encode_audio:
            cmd.append('-N')  # Skip audio encoding for speed

        command_str = self._format_command(cmd)
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30
            )
            
            # Check for errors
            if result.returncode != 0:
                error_msg = self._extract_error(result.stderr + result.stdout)
                test_result = FormatTestResult(
                    format=format_name,
                    file_path=str(img_path),
                    passed=False,
                    message=f"Processing failed: {error_msg}",
                    dimensions=dimensions,
                    command=command_str,
                    purpose=purpose,
                    expected=expected,
                    output_audio_path=str(output_audio_path),
                    output_audio_created=output_audio_path.exists(),
                    output_image_path=str(output_image_path),
                    output_image_created=output_image_path.exists(),
                    details={'returncode': result.returncode, 'stderr': result.stderr}
                )
                self._log_test(test_id, format_name, str(img_path), test_result)
                return test_result
            
            # Check output was created (if not using -N flag)
            # With -N flag, we just verify no errors occurred
            
            test_result = FormatTestResult(
                format=format_name,
                file_path=str(img_path),
                passed=True,
                message="Format processed successfully",
                dimensions=dimensions,
                command=command_str,
                purpose=purpose,
                expected=expected,
                output_audio_path=str(output_audio_path),
                output_audio_created=output_audio_path.exists(),
                output_image_path=str(output_image_path),
                output_image_created=output_image_path.exists(),
                details={'returncode': 0}
            )
            self._log_test(test_id, format_name, str(img_path), test_result)
            return test_result
            
        except subprocess.TimeoutExpired:
            test_result = FormatTestResult(
                format=format_name,
                file_path=str(img_path),
                passed=False,
                message="Processing timeout (>30s)",
                dimensions=dimensions,
                command=command_str,
                purpose=purpose,
                expected=expected,
                output_audio_path=str(output_audio_path),
                output_audio_created=output_audio_path.exists(),
                output_image_path=str(output_image_path),
                output_image_created=output_image_path.exists(),
            )
            self._log_test(test_id, format_name, str(img_path), test_result)
            return test_result
        except Exception as e:
            test_result = FormatTestResult(
                format=format_name,
                file_path=str(img_path),
                passed=False,
                message=f"Exception: {e}",
                dimensions=dimensions,
                command=command_str,
                purpose=purpose,
                expected=expected,
                output_audio_path=str(output_audio_path),
                output_audio_created=output_audio_path.exists(),
                output_image_path=str(output_image_path),
                output_image_created=output_image_path.exists(),
            )
            self._log_test(test_id, format_name, str(img_path), test_result)
            return test_result
    
    def test_all_formats(self, image_dir: str, output_dir: str = "tests/test_outputs/format_validator") -> Dict:
        """
        Test all image formats in a directory
        
        Args:
            image_dir: Directory containing test images
            output_dir: Directory for output files
            
        Returns:
            Dictionary with test results
        """
        test_dir = Path(image_dir)
        results = []
        format_coverage = set()
        
        # Find all image files
        for ext in self.SUPPORTED_FORMATS.keys():
            pattern = f"*.{ext}"
            for img_path in test_dir.glob(pattern):
                if self.detailed_output:
                    test_num = f"T{self.test_counter + 1:03d}"
                    print("\n" + "-" * 70)
                    print(f"Test {test_num}: {img_path.name}")
                    print(f"Format: {self.SUPPORTED_FORMATS.get(ext, ext.upper())}")
                    print("Purpose: Validate format loading and processing")
                    print("Expected: Process succeeds without errors; input dimensions detected")
                    output_audio_path = Path(output_dir) / f"test_format_{ext}_{img_path.stem}.wav"
                    output_image_suffix = "_overlay" if not self.encode_audio else ""
                    output_image_path = Path(output_dir) / f"test_format_{ext}_{img_path.stem}{output_image_suffix}{img_path.suffix}"
                    cmd_preview = [
                        str(self.slowframe_bin),
                        '-i', str(img_path),
                        '-p', 'm1',
                        '-o', str(output_audio_path),
                        '-K'
                    ]
                    if not self.encode_audio:
                        cmd_preview.append('-N')
                    print(f"Command: {self._format_command(cmd_preview)}")
                    print(f"Output image: {output_image_path}")
                    print(f"Output audio: {output_audio_path}")
                    print("-" * 70)
                elif self.verbose:
                    print(f"Testing {img_path.name}...")
                
                result = self.test_format(str(img_path), output_dir)
                results.append(result)
                
                if result.passed:
                    format_coverage.add(result.format)
                
                if self.detailed_output:
                    status = "PASS" if result.passed else "FAIL"
                    print(f"Result: {status} - {result.message}")
                    print(f"Dimensions: {result.dimensions[0]}x{result.dimensions[1]}")
                    print(f"Output image created: {'yes' if result.output_image_created else 'no'}")
                    print(f"Output audio created: {'yes' if result.output_audio_created else 'no'}")
                    if self.pause_between:
                        input("Press Enter to continue to next test...")
                elif self.verbose:
                    status = "✓" if result.passed else "✗"
                    print(f"  {status} {result.format}: {result.message}")
        
        # Compile summary
        passed = sum(1 for r in results if r.passed)
        total = len(results)
        
        return {
            'passed': passed,
            'failed': total - passed,
            'total': total,
            'format_coverage': sorted(format_coverage),
            'supported_formats': sorted(self.SUPPORTED_FORMATS.values()),
            'results': results,
        }

    def generate_html_report(self, results: Dict, output_dir: Path) -> Path:
        """Generate HTML report with side-by-side image comparisons."""
        report_path = output_dir / "format_validator_report.html"

        def rel_path(path: str) -> str:
            if not path:
                return ""
            return os.path.relpath(path, output_dir)

        rows = []
        for idx, result in enumerate(results['results'], start=1):
            status = "PASS" if result.passed else "FAIL"
            input_img = rel_path(result.file_path)
            output_img = rel_path(result.output_image_path)
            rows.append(f"""
            <tr class=\"{status.lower()}\">
                <td>T{idx:03d}</td>
                <td>{result.format}</td>
                <td>{result.purpose}</td>
                <td>{result.expected}</td>
                <td><code>{result.command}</code></td>
                <td>{status}</td>
                <td>
                    <div class=\"img-pair\">
                        <div>
                            <div class=\"label\">Input</div>
                            <img src=\"{input_img}\" alt=\"input\">
                        </div>
                        <div>
                            <div class=\"label\">Output</div>
                            <img src=\"{output_img}\" alt=\"output\">
                        </div>
                    </div>
                </td>
                <td>
                    <div><code>{result.output_audio_path}</code></div>
                    <div>Created: {'yes' if result.output_audio_created else 'no'}</div>
                </td>
            </tr>
            """)

        html = f"""
<!DOCTYPE html>
<html>
<head>
    <meta charset=\"utf-8\">
    <title>SlowFrame Format Validator Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; color: #111; }}
        h1 {{ margin-bottom: 4px; }}
        .summary {{ margin-bottom: 16px; }}
        table {{ border-collapse: collapse; width: 100%; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; vertical-align: top; }}
        th {{ background: #f3f3f3; }}
        tr.pass {{ background: #f6fff6; }}
        tr.fail {{ background: #fff4f4; }}
        code {{ font-family: monospace; white-space: pre-wrap; }}
        .img-pair {{ display: flex; gap: 10px; }}
        .img-pair img {{ max-width: 260px; max-height: 180px; border: 1px solid #ccc; }}
        .label {{ font-size: 12px; color: #555; margin-bottom: 4px; }}
    </style>
</head>
<body>
    <h1>SlowFrame Image Format Validator Report</h1>
    <div class=\"summary\">
        <div>Results: {results['passed']}/{results['total']} passed</div>
        <div>Format Coverage: {len(results['format_coverage'])}/{len(results['supported_formats'])}</div>
        <div>Tested: {', '.join(results['format_coverage'])}</div>
    </div>
    <table>
        <thead>
            <tr>
                <th>Test #</th>
                <th>Format</th>
                <th>Purpose</th>
                <th>Expected</th>
                <th>Command</th>
                <th>Status</th>
                <th>Before/After</th>
                <th>Audio Output</th>
            </tr>
        </thead>
        <tbody>
            {''.join(rows)}
        </tbody>
    </table>
</body>
</html>
"""

        report_path.write_text(html, encoding="utf-8")
        return report_path
    
    def _extract_error(self, output: str) -> str:
        """Extract error message from output"""
        lines = output.split('\n')
        for line in lines:
            if '[ERROR]' in line or 'Error' in line or 'error' in line:
                return line.strip()
        return "Unknown error"


def main():
    """Command-line interface"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Test SlowFrame image format support",
        epilog="Week 3 enhancements: --report generates HTML with test details, --verify enables interactive verification"
    )
    parser.add_argument("image_dir", help="Directory containing test images")
    parser.add_argument("-o", "--output", default="tests/test_outputs/format_validator", help="Output directory")
    parser.add_argument("-b", "--binary", default="./bin/slowframe", help="Path to slowframe binary")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--json", action="store_true", help="Output JSON results")
    parser.add_argument("--report", action="store_true", help="Generate HTML report with test details")
    parser.add_argument("--verify", action="store_true", help="Enable interactive human verification workflow")
    parser.add_argument("--detailed", action="store_true", help="Show per-test details")
    parser.add_argument("--quiet", action="store_true", help="Summary only")
    parser.add_argument("--pause", action="store_true", help="Pause after each test")
    parser.add_argument("--encode", action="store_true", help="Run full audio encoding (no -N)")
    
    args = parser.parse_args()
    
    detailed_output = args.detailed or not args.quiet
    validator = FormatValidator(
        args.binary,
        args.verbose,
        args.report,
        args.verify,
        detailed_output=detailed_output,
        pause_between=args.pause,
        encode_audio=args.encode
    )
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)

    results = validator.test_all_formats(args.image_dir, str(output_dir))
    
    # Generate reports if enabled
    if args.report:
        report_path = validator.generate_html_report(results, output_dir)
        print(f"\n✓ HTML report generated: {report_path}")
    
    if args.json:
        # JSON output
        output = {
            'summary': {
                'passed': results['passed'],
                'failed': results['failed'],
                'total': results['total'],
                'format_coverage': results['format_coverage'],
            },
            'results': [
                {
                    'format': r.format,
                    'file': r.file_path,
                    'passed': r.passed,
                    'message': r.message,
                    'dimensions': r.dimensions,
                }
                for r in results['results']
            ]
        }
        print(json.dumps(output, indent=2))
    else:
        # Human-readable output
        print("\n" + "="*70)
        print("SlowFrame Image Format Support Test")
        print("="*70)
        print(f"\nResults: {results['passed']}/{results['total']} passed")
        print(f"\nFormat Coverage: {len(results['format_coverage'])}/{len(results['supported_formats'])}")
        print(f"  Tested: {', '.join(results['format_coverage'])}")
        
        missing = set(results['supported_formats']) - set(results['format_coverage'])
        if missing:
            print(f"  Missing: {', '.join(missing)}")
        
        # Show failures
        failures = [r for r in results['results'] if not r.passed]
        if failures:
            print(f"\nFailures ({len(failures)}):")
            for r in failures:
                print(f"  ✗ {r.format} - {Path(r.file_path).name}")
                print(f"    {r.message}")
        
        print("="*70)
    
    sys.exit(0 if results['failed'] == 0 else 1)


if __name__ == "__main__":
    main()
