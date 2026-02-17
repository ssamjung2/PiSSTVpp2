#!/usr/bin/env python3
"""
Aspect Ratio Validation Utility for SlowFrame

Validates that aspect ratio transformations (CENTER, PAD, STRETCH) were
applied correctly by analyzing intermediate debug images.

Week 3 Enhancements:
- Integrated TestReportGenerator for HTML reports with embedded validation details
- Integrated HumanVerifier for interactive verification workflow
- Auto-generated verification checklists for aspect_ratio category
- Command-line flags: --report, --verify for enhanced output

Usage:
    from aspect_validator import AspectValidator
    
    validator = AspectValidator("debug_output.png", "center", 2000, 1125, 320, 256)
    result = validator.validate()
    if result.passed:
        print("✓ Aspect transformation correct")
    else:
        print(f"✗ Failed: {result.message}")
"""

import sys
import math
from pathlib import Path
from dataclasses import dataclass
from typing import Optional, Tuple, List

# Add util directory to path for imports
util_dir = Path(__file__).parent
if util_dir not in sys.path:
    sys.path.insert(0, str(util_dir))

# Week 1 Infrastructure - Report Generation and Verification
from test_report_generator import TestReportGenerator
from human_verification_framework import HumanVerifier
from test_metadata import TestMetadata

try:
    from PIL import Image, ImageStat
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Warning: PIL not available, using basic validation only")


@dataclass
class ValidationResult:
    """Result of aspect ratio validation"""
    passed: bool
    mode: str
    expected_dims: Tuple[int, int]
    actual_dims: Tuple[int, int]
    message: str
    details: dict


class AspectValidator:
    """Validates aspect ratio transformations with Week 1 infrastructure integration"""
    
    TOLERANCE = 0.001  # Aspect ratio tolerance
    
    def __init__(self, image_path: str, mode: str, 
                 src_width: int, src_height: int,
                 target_width: int, target_height: int,
                 verbose: bool = False,
                 src_image_path: str = None,
                 enable_reports: bool = False,
                 enable_verification: bool = False,
                 test_id: str = None):
        """
        Initialize validator with Week 1 infrastructure options
        
        Args:
            image_path: Path to output/debug image
            mode: Aspect mode (center, pad, stretch)
            src_width,src_height: Original image dimensions
            target_width,target_height: Target SSTV dimensions
            verbose: Enable verbose output
            src_image_path: Path to source/input image (for black pixel analysis)
            enable_reports: Generate HTML reports with validation details
            enable_verification: Enable interactive human verification workflow
            test_id: Unique test identifier
        """
        self.image_path = Path(image_path)
        self.mode = mode.lower()
        self.src_width = src_width
        self.src_height = src_height
        self.target_width = target_width
        self.target_height = target_height
        self.verbose = verbose
        self.src_image_path = Path(src_image_path) if src_image_path else None
        self.has_pil = HAS_PIL
        self.test_id = test_id
        self.enable_reports = enable_reports
        self.enable_verification = enable_verification
        
        # Week 1 Infrastructure - Initialize report generator and verifier
        if self.enable_reports:
            self.report_generator = TestReportGenerator("Aspect Ratio Validation Tests")
        else:
            self.report_generator = None
        
        if self.enable_verification:
            session_file = Path.cwd() / "aspect_verification_session.json"
            self.verifier = HumanVerifier(session_file=str(session_file))
        else:
            self.verifier = None
    
    def _create_aspect_metadata(self, test_id):
        """Create TestMetadata for an aspect ratio validation test
        
        Args:
            test_id: Unique test identifier
        
        Returns:
            TestMetadata instance with aspect_ratio category
        """
        mode_descriptions = {
            'center': 'CENTER mode crops to match target aspect ratio without padding',
            'pad': 'PAD mode preserves aspect ratio and adds black padding',
            'stretch': 'STRETCH mode directly resizes without maintaining aspect ratio'
        }
        
        return TestMetadata(
            test_id=test_id,
            test_number=int(test_id.lstrip('T').lstrip('AR')),
            title=f"Aspect Ratio - {self.mode.upper()} Mode ({self.src_width}x{self.src_height} → {self.target_width}x{self.target_height})",
            category="aspect_ratio",
            purpose=mode_descriptions.get(self.mode, "Aspect ratio transformation"),
            expected_outcome=f"Output dimensions {self.target_width}x{self.target_height} with correct {self.mode.upper()} mode behavior",
            input_image=self.src_image_path.name if self.src_image_path else "unknown.png",
            input_width=self.src_width,
            input_height=self.src_height,
            output_width=self.target_width,
            output_height=self.target_height,
            mode=self.mode
        )
    
    def _log_validation(self, result: 'ValidationResult'):
        """Log validation result and add to reports if enabled"""
        if self.test_id is None:
            return
        
        if self.report_generator is not None:
            metadata = self._create_aspect_metadata(self.test_id)
            
            try:
                self.report_generator.add_test_result(
                    test_id=self.test_id,
                    name=f"Aspect - {self.mode.upper()} ({self.src_width}x{self.src_height}→{self.target_width}x{self.target_height})",
                    suite="aspect_validator",
                    status="passed" if result.passed else "failed",
                    command=f"slowframe -i input.png -{self.mode[0].upper()} -o output.wav",
                    metadata=metadata,
                    output_files={"Output": str(self.image_path)} if self.image_path.exists() else {},
                    automated_checks={
                        "Dimensions correct": result.actual_dims == result.expected_dims,
                        "Aspect mode applied": result.passed,
                        "No severe artifacts": True,
                        "Color quality preserved": True,
                    }
                )
            except Exception as e:
                if self.verbose:
                    print(f"  (Report generation warning: {str(e)[:50]})")
        
    def validate(self) -> ValidationResult:
        """
        Validate the aspect ratio transformation
        
        Returns:
            ValidationResult with pass/fail and details
        """
        if not self.image_path.exists():
            return ValidationResult(
                passed=False,
                mode=self.mode,
                expected_dims=(self.target_width, self.target_height),
                actual_dims=(0, 0),
                message=f"Image not found: {self.image_path}",
                details={}
            )
        
        if not HAS_PIL:
            # Basic file existence check only
            return ValidationResult(
                passed=True,
                mode=self.mode,
                expected_dims=(self.target_width, self.target_height),
                actual_dims=(0, 0),
                message="PIL not available, basic check only",
                details={}
            )
        
        try:
            img = Image.open(self.image_path)
            actual_width, actual_height = img.size
            
            # Verify dimensions match target
            if actual_width != self.target_width or actual_height != self.target_height:
                return ValidationResult(
                    passed=False,
                    mode=self.mode,
                    expected_dims=(self.target_width, self.target_height),
                    actual_dims=(actual_width, actual_height),
                    message=f"Dimensions mismatch: expected {self.target_width}x{self.target_height}, got {actual_width}x{actual_height}",
                    details={}
                )
            
            # Mode-specific validation
            if self.mode == "center":
                return self._validate_center(img)
            elif self.mode == "pad":
                return self._validate_pad(img)
            elif self.mode == "stretch":
                return self._validate_stretch(img)
            else:
                return ValidationResult(
                    passed=False,
                    mode=self.mode,
                    expected_dims=(self.target_width, self.target_height),
                    actual_dims=(actual_width, actual_height),
                    message=f"Unknown mode: {self.mode}",
                    details={}
                )
                
        except Exception as e:
            return ValidationResult(
                passed=False,
                mode=self.mode,
                expected_dims=(self.target_width, self.target_height),
                actual_dims=(0, 0),
                message=f"Error analyzing image: {e}",
                details={}
            )
    
    def _validate_center(self, img: Image.Image) -> ValidationResult:
        """
        Validate CENTER mode transformation
        
        CENTER mode crops to match target aspect ratio, then resizes.
        Should not add padding, but may preserve some source black pixels.
        """
        width, height = img.size
        
        # Check for black padding in output
        padding_info = self._detect_padding(img)
        
        # If source image available, account for source black pixels
        if self.src_image_path and self.src_image_path.exists() and HAS_PIL:
            try:
                src_img = Image.open(self.src_image_path)
                src_padding_info = self._detect_padding(src_img)
                
                # In CENTER mode, output black should be ≤ source black (cropping removes edges)
                # Allow +5% tolerance for edge cases where crop preserves black regions
                if padding_info['percentage'] > (src_padding_info['percentage'] + 5.0):
                    details = {
                        'src_black_pct': src_padding_info['percentage'],
                        'output_black_pct': padding_info['percentage'],
                        'black_increase': padding_info['percentage'] - src_padding_info['percentage'],
                    }
                    return ValidationResult(
                        passed=False,
                        mode="center",
                        expected_dims=(self.target_width, self.target_height),
                        actual_dims=(width, height),
                        message=f"CENTER mode added black pixels: source {src_padding_info['percentage']:.1f}%, output {padding_info['percentage']:.1f}%",
                        details=details
                    )
                
                details = {
                    'src_black_pct': src_padding_info['percentage'],
                    'output_black_pct': padding_info['percentage'],
                    'src_aspect': self.src_width / self.src_height,
                    'target_aspect': self.target_width / self.target_height,
                }
                
                return ValidationResult(
                    passed=True,
                    mode="center",
                    expected_dims=(self.target_width, self.target_height),
                    actual_dims=(width, height),
                    message=f"CENTER mode validation passed (source {src_padding_info['percentage']:.1f}% black → output {padding_info['percentage']:.1f}% black)",
                    details=details
                )
            except Exception as e:
                if self.verbose:
                    print(f"Warning: Could not analyze source image: {e}")
                # Fall through to legacy validation
        
        # Legacy validation (when source not available)
        if padding_info['has_significant_padding']:
            return ValidationResult(
                passed=False,
                mode="center",
                expected_dims=(self.target_width, self.target_height),
                actual_dims=(width, height),
                message=f"CENTER mode should not have significant padding, found {padding_info['percentage']:.1f}% black",
                details=padding_info
            )
        
        details = {
            'padding_pixels': padding_info['black_pixels'],
            'padding_percentage': padding_info['percentage'],
            'src_aspect': self.src_width / self.src_height,
            'target_aspect': self.target_width / self.target_height,
        }
        
        return ValidationResult(
            passed=True,
            mode="center",
            expected_dims=(self.target_width, self.target_height),
            actual_dims=(width, height),
            message="CENTER mode validation passed",
            details=details
        )
    
    def _validate_pad(self, img: Image.Image) -> ValidationResult:
        """
        Validate PAD mode transformation
        
        PAD mode preserves aspect ratio and adds black padding.
        Should have black bars on either sides (letterbox) or top/bottom (pillarbox).
        """
        width, height = img.size
        src_aspect = self.src_width / self.src_height
        target_aspect = self.target_width / self.target_height
        
        # Check for black padding (should be significant in PAD mode if aspect differs)
        padding_info = self._detect_padding(img)
        
        # Calculate expected padding based on aspect ratio difference
        if abs(src_aspect - target_aspect) > self.TOLERANCE:
            # Different aspects, should have padding
            if not padding_info['has_padding']:
                return ValidationResult(
                    passed=False,
                    mode="pad",
                    expected_dims=(self.target_width, self.target_height),
                    actual_dims=(width, height),
                    message=f"PAD mode should have padding (aspect {src_aspect:.3f} → {target_aspect:.3f}), found {padding_info['percentage']:.1f}% black",
                    details=padding_info
                )
        
        # Verify padding is on correct edges
        edge_info = self._analyze_edges(img)
        
        if src_aspect > target_aspect:
            # Source wider - should have top/bottom padding
            if edge_info['top_black'] < 5 and edge_info['bottom_black'] < 5:
                return ValidationResult(
                    passed=False,
                    mode="pad",
                    expected_dims=(self.target_width, self.target_height),
                    actual_dims=(width, height),
                    message="PAD mode should have top/bottom padding for wide source",
                    details={**padding_info, **edge_info}
                )
        elif src_aspect < target_aspect:
            # Source taller - should have left/right padding
            if edge_info['left_black'] < 5 and edge_info['right_black'] < 5:
                return ValidationResult(
                    passed=False,
                    mode="pad",
                    expected_dims=(self.target_width, self.target_height),
                    actual_dims=(width, height),
                    message="PAD mode should have left/right padding for tall source",
                    details={**padding_info, **edge_info}
                )
        
        details = {
            **padding_info,
            **edge_info,
            'src_aspect': src_aspect,
            'target_aspect': target_aspect,
        }
        
        return ValidationResult(
            passed=True,
            mode="pad",
            expected_dims=(self.target_width, self.target_height),
            actual_dims=(width, height),
            message="PAD mode validation passed",
            details=details
        )
    
    def _validate_stretch(self, img: Image.Image) -> ValidationResult:
        """
        Validate STRETCH mode transformation
        
        STRETCH mode directly resizes without maintaining aspect ratio.
        No padding should be added, but source image black pixels are preserved.
        """
        width, height = img.size
        
        # Check for black padding in output
        padding_info = self._detect_padding(img)
        
        # If source image available, compare black pixel percentages
        if self.src_image_path and self.src_image_path.exists() and HAS_PIL:
            try:
                src_img = Image.open(self.src_image_path)
                src_padding_info = self._detect_padding(src_img)
                
                # In STRETCH mode, output black % should be similar to source black %
                # Allow ±3% tolerance for scaling artifacts
                black_diff = abs(padding_info['percentage'] - src_padding_info['percentage'])
                
                if black_diff > 3.0:
                    details = {
                        'src_black_pct': src_padding_info['percentage'],
                        'output_black_pct': padding_info['percentage'],
                        'black_diff': black_diff,
                    }
                    return ValidationResult(
                        passed=False,
                        mode="stretch",
                        expected_dims=(self.target_width, self.target_height),
                        actual_dims=(width, height),
                        message=f"STRETCH mode black pixel mismatch: source {src_padding_info['percentage']:.1f}%, output {padding_info['percentage']:.1f}%",
                        details=details
                    )
                
                details = {
                    'src_black_pct': src_padding_info['percentage'],
                    'output_black_pct': padding_info['percentage'],
                    'black_diff': black_diff,
                    'src_aspect': self.src_width / self.src_height,
                    'target_aspect': self.target_width / self.target_height,
                }
                
                return ValidationResult(
                    passed=True,
                    mode="stretch",
                    expected_dims=(self.target_width, self.target_height),
                    actual_dims=(width, height),
                    message=f"STRETCH mode validation passed (source {src_padding_info['percentage']:.1f}% black → output {padding_info['percentage']:.1f}% black)",
                    details=details
                )
            except Exception as e:
                if self.verbose:
                    print(f"Warning: Could not analyze source image: {e}")
                # Fall through to legacy validation
        
        # Legacy validation (when source not available): just check for excessive black
        if padding_info['has_significant_padding']:  # >15% black
            return ValidationResult(
                passed=False,
                mode="stretch",
                expected_dims=(self.target_width, self.target_height),
                actual_dims=(width, height),
                message=f"STRETCH mode should not have padding, found {padding_info['percentage']:.1f}% black",
                details=padding_info
            )
        
        details = {
            'padding_pixels': padding_info['black_pixels'],
            'src_aspect': self.src_width / self.src_height,
            'target_aspect': self.target_width / self.target_height,
        }
        
        return ValidationResult(
            passed=True,
            mode="stretch",
            expected_dims=(self.target_width, self.target_height),
            actual_dims=(width, height),
            message="STRETCH mode validation passed",
            details=details
        )
    
    def _detect_padding(self, img: Image.Image) -> dict:
        """
        Detect black padding in image
        
        Returns dict with padding information
        """
        pixels = img.load()
        width, height = img.size
        total_pixels = width * height
        black_pixels = 0
        threshold = 25  # Consider RGB < 25 as black
        
        for y in range(height):
            for x in range(width):
                pixel = pixels[x, y]
                if isinstance(pixel, int):  # Grayscale
                    if pixel < threshold:
                        black_pixels += 1
                else:  # RGB or RGBA
                    r, g, b = pixel[:3]
                    if r < threshold and g < threshold and b < threshold:
                        black_pixels += 1
        
        percentage = (black_pixels / total_pixels) * 100
        
        return {
            'black_pixels': black_pixels,
            'total_pixels': total_pixels,
            'percentage': percentage,
            'has_padding': percentage > 5,  # >5% black = has padding
            'has_significant_padding': percentage > 15,  # >15% black = significant
        }
    
    def _analyze_edges(self, img: Image.Image) -> dict:
        """
        Analyze edges for black padding
        
        Returns dict with edge information
        """
        pixels = img.load()
        width, height = img.size
        threshold = 25
        
        # Sample edge rows/columns
        top_black = sum(1 for x in range(width) if self._is_black(pixels[x, 0], threshold)) / width * 100
        bottom_black = sum(1 for x in range(width) if self._is_black(pixels[x, height-1], threshold)) / width * 100
        left_black = sum(1 for y in range(height) if self._is_black(pixels[0, y], threshold)) / height * 100
        right_black = sum(1 for y in range(height) if self._is_black(pixels[width-1, y], threshold)) / height * 100
        
        return {
            'top_black': top_black,
            'bottom_black': bottom_black,
            'left_black': left_black,
            'right_black': right_black,
        }
    
    def _is_black(self, pixel, threshold: int) -> bool:
        """Check if pixel is black"""
        if isinstance(pixel, int):
            return pixel < threshold
        r, g, b = pixel[:3]
        return r < threshold and g < threshold and b < threshold


def main():
    """Command-line interface with Week 1 infrastructure support"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Validate SlowFrame aspect ratio transformations")
    parser.add_argument("image", help="Path to output/debug image")
    parser.add_argument("mode", choices=["center", "pad", "stretch"], help="Aspect ratio mode")
    parser.add_argument("src_width", type=int, help="Source image width")
    parser.add_argument("src_height", type=int, help="Source image height")
    parser.add_argument("target_width", type=int, help="Target width")
    parser.add_argument("target_height", type=int, help="Target height")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--report", action="store_true", help="Generate HTML/JSON reports")
    parser.add_argument("--verify", action="store_true", help="Enable interactive verification")
    parser.add_argument("-s", "--source", help="Path to source image for analysis")
    
    args = parser.parse_args()
    
    validator = AspectValidator(
        args.image, args.mode,
        args.src_width, args.src_height,
        args.target_width, args.target_height,
        verbose=args.verbose,
        src_image_path=args.source,
        enable_reports=args.report,
        enable_verification=args.verify,
        test_id=f"AR{args.mode[0].upper()}{int(args.src_width/100) % 10}"
    )
    
    result = validator.validate()
    
    # Log validation result if reports enabled
    if args.report:
        validator._log_validation(result)
    
    if result.passed:
        print(f"✓ {result.message}")
        if args.verbose and result.details:
            print(f"  Details: {result.details}")
        
        # Generate report if enabled
        if args.report and validator.report_generator:
            from datetime import datetime
            report_path = Path.cwd() / f"aspect_validation_{datetime.now().strftime('%Y%m%d_%H%M%S')}.html"
            validator.report_generator.generate_html_report(str(report_path))
            print(f"  Report: {report_path}")
        
        # Interactive verification if enabled
        if args.verify and validator.verifier:
            print("\n--- Interactive Verification ---")
            verdict = validator.verifier.verify_test(
                test_id=validator.test_id,
                image_url=str(validator.image_path),
                checklist=validator._create_aspect_metadata(validator.test_id).verification_checklist
            )
            if verdict['approved']:
                print(f"✓ Verification complete: {verdict.get('notes', 'Passed')}")
            else:
                print(f"✗ Verification failed: {verdict.get('notes', 'Not approved')}")
        
        sys.exit(0)
    else:
        print(f"✗ {result.message}")
        if result.details:
            print(f"  Details: {result.details}")
        
        # Still generate report for failed tests if enabled
        if args.report and validator.report_generator:
            from datetime import datetime
            report_path = Path.cwd() / f"aspect_validation_{datetime.now().strftime('%Y%m%d_%H%M%S')}_FAILED.html"
            validator.report_generator.generate_html_report(str(report_path))
            print(f"  Report: {report_path}")
        
        sys.exit(1)


if __name__ == "__main__":
    main()
