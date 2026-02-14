#!/usr/bin/env python3
"""
Overlay Verification Test Helper for PiSSTVpp2

Validates that text overlay specifications were correctly applied to SSTV output images.
Integrates with the test suite to verify overlay features work correctly.

Usage:
    python3 verify_overlay.py output.png --text "N0CALL" --color white
    python3 verify_overlay.py output.png --spec "text=N0CALL|color=white|placement=bottom"
    python3 verify_overlay_cmd.py output.png --config "overlay=N0CALL|color=white"
"""

import sys
import json
import argparse
from pathlib import Path
from dataclasses import dataclass
from typing import Optional, List
from image_analyzer import ImageAnalyzer, TextOverlaySpec, ColorRGB


@dataclass
class OverlayCheck:
    """A single check to perform on an overlay"""
    name: str
    description: str
    passed: bool
    message: str = ""


class OverlayVerifier:
    """Verifies overlay specifications against actual images"""
    
    def __init__(self, image_path: str, verbose: bool = False):
        """Initialize verifier"""
        self.image_path = image_path
        self.analyzer = ImageAnalyzer(image_path, verbose=verbose)
        self.checks = []
        self.verbose = verbose
    
    def verify_text_presence(self) -> bool:
        """Verify that text was rendered in image"""
        if not self.analyzer.check_valid():
            self.checks.append(OverlayCheck("text_presence", "Text rendered", False, "Image invalid"))
            return False
        
        regions = self.analyzer.detect_text_regions()
        
        if regions:
            passed = len(regions) > 0
            check = OverlayCheck(
                "text_presence",
                "Text regions detected",
                passed,
                f"Found {len(regions)} text regions"
            )
        else:
            check = OverlayCheck(
                "text_presence",
                "Text regions detected",
                False,
                "No text regions detected"
            )
        
        self.checks.append(check)
        return check.passed
    
    def verify_color(self, expected_color: ColorRGB, tolerance: int = 50) -> bool:
        """Verify that specified color appears in image"""
        if not self.analyzer.check_valid():
            self.checks.append(OverlayCheck(
                "color_presence",
                f"Color {expected_color.to_hex()} present",
                False,
                "Image invalid"
            ))
            return False
        
        region = self.analyzer.find_color_region(expected_color, tolerance=tolerance)
        
        if region:
            check = OverlayCheck(
                "color_presence",
                f"Color {expected_color.to_hex()} present",
                True,
                f"Found at {region.to_dict()}"
            )
        else:
            check = OverlayCheck(
                "color_presence",
                f"Color {expected_color.to_hex()} present",
                False,
                f"Color not found (tolerance={tolerance})"
            )
        
        self.checks.append(check)
        return check.passed
    
    def verify_placement(self, placement: str) -> bool:
        """Verify text placement in image"""
        if not self.analyzer.check_valid():
            self.checks.append(OverlayCheck("placement", f"Placement {placement}", False, "Image invalid"))
            return False
        
        regions = self.analyzer.detect_text_regions()
        if not regions:
            check = OverlayCheck("placement", f"Text in {placement}", False, "No text regions found")
            self.checks.append(check)
            return False
        
        height = self.analyzer.image.height
        
        if placement == "top":
            # Text should be in top 30% of image
            filtered = [r for r in regions if r.y < height * 0.3]
        elif placement == "bottom":
            # Text should be in bottom 30% of image
            filtered = [r for r in regions if r.y > height * 0.7]
        elif placement == "center":
            # Text should be in middle 40% of image
            filtered = [r for r in regions if height * 0.3 <= r.y <= height * 0.7]
        elif placement == "left":
            # Text should be in left 30% of image
            width = self.analyzer.image.width
            filtered = [r for r in regions if r.x < width * 0.3]
        elif placement == "right":
            # Text should be in right 30% of image
            width = self.analyzer.image.width
            filtered = [r for r in regions if r.x > width * 0.7]
        else:
            check = OverlayCheck("placement", f"Placement {placement}", False, f"Unknown placement: {placement}")
            self.checks.append(check)
            return False
        
        if filtered:
            check = OverlayCheck("placement", f"Text in {placement}", True,
                                f"Found {len(filtered)} regions in {placement}")
        else:
            check = OverlayCheck("placement", f"Text in {placement}", False,
                                f"No regions found in {placement}")
        
        self.checks.append(check)
        return check.passed
    
    def verify_full_spec(self, spec: TextOverlaySpec) -> bool:
        """Verify complete overlay specification"""
        passed = True
        
        # Check text presence
        passed &= self.verify_text_presence()
        
        # Check color
        if spec.color:
            passed &= self.verify_color(spec.color)
        
        # Check placement
        if spec.placement:
            passed &= self.verify_placement(spec.placement)
        
        return passed
    
    def print_summary(self):
        """Print verification summary"""
        print(f"\nOverlay Verification Summary")
        print(f"{'='*60}")
        print(f"Image: {self.image_path}\n")
        
        if not self.checks:
            print("No checks performed")
            return
        
        passed_count = sum(1 for c in self.checks if c.passed)
        total_count = len(self.checks)
        
        for check in self.checks:
            status = "✓ PASS" if check.passed else "✗ FAIL"
            print(f"{status}: {check.name}")
            print(f"       {check.description}")
            if check.message:
                print(f"       {check.message}")
        
        print(f"\nResult: {passed_count}/{total_count} checks passed")
        
        if passed_count == total_count:
            print("Status: ✓ All checks passed")
        else:
            print(f"Status: ✗ {total_count - passed_count} checks failed")
        
        print(f"{'='*60}\n")
        
        return passed_count == total_count
    
    def get_results_json(self) -> dict:
        """Get verification results as JSON"""
        return {
            "image": str(self.image_path),
            "checks": [
                {
                    "name": c.name,
                    "description": c.description,
                    "passed": c.passed,
                    "message": c.message
                }
                for c in self.checks
            ],
            "summary": {
                "total": len(self.checks),
                "passed": sum(1 for c in self.checks if c.passed),
                "failed": sum(1 for c in self.checks if not c.passed)
            }
        }


def parse_color(color_str: str) -> Optional[ColorRGB]:
    """Parse color from string (name or hex)"""
    if color_str.startswith('#'):
        try:
            # Hex format: #RRGGBB
            r = int(color_str[1:3], 16)
            g = int(color_str[3:5], 16)
            b = int(color_str[5:7], 16)
            return ColorRGB(r, g, b)
        except:
            return None
    else:
        # Color names
        colors = {
            "white": ColorRGB(255, 255, 255),
            "black": ColorRGB(0, 0, 0),
            "red": ColorRGB(255, 0, 0),
            "green": ColorRGB(0, 255, 0),
            "blue": ColorRGB(0, 0, 255),
            "yellow": ColorRGB(255, 255, 0),
            "cyan": ColorRGB(0, 255, 255),
            "magenta": ColorRGB(255, 0, 255),
            "gray": ColorRGB(128, 128, 128),
            "grey": ColorRGB(128, 128, 128),
        }
        return colors.get(color_str.lower())


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Verify text overlay specifications in SSTV output images"
    )
    parser.add_argument("image", help="Output image to verify")
    parser.add_argument("--text", help="Expected text in overlay")
    parser.add_argument("--color", help="Expected overlay color (name or #RRGGBB)")
    parser.add_argument("--placement", help="Expected text placement (top, bottom, center, left, right)")
    parser.add_argument("--spec", metavar="SPEC",
                       help="Full spec format: text=...|color=...|placement=...")
    parser.add_argument("--json", action="store_true",
                       help="Output results as JSON")
    parser.add_argument("-v", "--verbose", action="store_true",
                       help="Verbose output")
    
    args = parser.parse_args()
    
    if not Path(args.image).exists():
        print(f"Error: Image not found: {args.image}")
        sys.exit(1)
    
    # Create verifier
    verifier = OverlayVerifier(args.image, verbose=args.verbose)
    
    # Build spec from arguments
    spec = TextOverlaySpec(text=args.text or "")
    
    if args.color:
        color = parse_color(args.color)
        if color:
            spec.color = color
        else:
            print(f"Warning: Could not parse color: {args.color}")
    
    if args.placement:
        spec.placement = args.placement
    
    # Parse full spec if provided
    if args.spec:
        parts = args.spec.split('|')
        for part in parts:
            if '=' in part:
                key, value = part.split('=', 1)
                if key == "text":
                    spec.text = value
                elif key == "color":
                    color = parse_color(value)
                    if color:
                        spec.color = color
                elif key == "placement":
                    spec.placement = value
    
    # Run verification
    verifier.verify_full_spec(spec)
    
    # Output results
    if args.json:
        print(json.dumps(verifier.get_results_json(), indent=2))
    else:
        all_passed = verifier.print_summary()
    
    # Exit with appropriate code
    results = verifier.get_results_json()
    passed_all = results['summary']['failed'] == 0
    sys.exit(0 if passed_all else 1)


if __name__ == "__main__":
    main()
