#!/usr/bin/env python3
"""
Image Comparison Tool for PiSSTVpp2 Testing

Compares actual output images against expected reference images
to validate that overlays, colors, and content match specifications.

Usage:
    python3 compare_images.py actual.png expected.png
    python3 compare_images.py actual.png expected.png --threshold 10
    python3 compare_images.py actual.png expected.png --save-diff diff.png
"""

import sys
import os
from pathlib import Path
from typing import Tuple, Dict, Any
import argparse

try:
    from PIL import Image, ImageDraw, ImageStat, ImageOps, ImageChops
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


class ImageComparator:
    """Compares two images for similarity"""
    
    def __init__(self, img1_path: str, img2_path: str, verbose: bool = False):
        """Load two images for comparison"""
        self.img1_path = Path(img1_path)
        self.img2_path = Path(img2_path)
        self.verbose = verbose
        self.img1 = None
        self.img2 = None
        self.errors = []
        
        if not HAS_PIL:
            self.errors.append("PIL/Pillow not installed. Install with: pip install Pillow")
            return
        
        # Load images
        self._load_images()
    
    def log(self, msg: str):
        """Log message if verbose"""
        if self.verbose:
            print(f"[INFO] {msg}")
    
    def _load_images(self):
        """Load both images from disk"""
        if not self.img1_path.exists():
            self.errors.append(f"Image 1 not found: {self.img1_path}")
            return
        
        if not self.img2_path.exists():
            self.errors.append(f"Image 2 not found: {self.img2_path}")
            return
        
        try:
            self.img1 = Image.open(self.img1_path)
            self.log(f"Loaded image 1: {self.img1.format} {self.img1.size} {self.img1.mode}")
        except Exception as e:
            self.errors.append(f"Failed to load image 1: {e}")
            return
        
        try:
            self.img2 = Image.open(self.img2_path)
            self.log(f"Loaded image 2: {self.img2.format} {self.img2.size} {self.img2.mode}")
        except Exception as e:
            self.errors.append(f"Failed to load image 2: {e}")
            return
    
    def is_valid(self) -> bool:
        """Check if both images loaded successfully"""
        return self.img1 is not None and self.img2 is not None and len(self.errors) == 0
    
    def get_dimensions(self) -> Tuple[Dict[str, Any], Dict[str, Any]]:
        """Get dimension information for both images"""
        if not self.is_valid():
            return {}, {}
        
        return {
            "width": self.img1.width,
            "height": self.img1.height,
            "size": self.img1.size,
            "mode": self.img1.mode,
            "format": self.img1.format
        }, {
            "width": self.img2.width,
            "height": self.img2.height,
            "size": self.img2.size,
            "mode": self.img2.mode,
            "format": self.img2.format
        }
    
    def check_dimensions(self) -> Tuple[bool, str]:
        """Check if images have same dimensions"""
        if not self.is_valid():
            return False, "Images not loaded"
        
        if self.img1.size != self.img2.size:
            return False, f"Different sizes: {self.img1.size} vs {self.img2.size}"
        
        return True, "Dimensions match"
    
    def check_mode(self) -> Tuple[bool, str]:
        """Check if images have same mode"""
        if not self.is_valid():
            return False, "Images not loaded"
        
        if self.img1.mode != self.img2.mode:
            return False, f"Different modes: {self.img1.mode} vs {self.img2.mode}"
        
        return True, "Modes match"
    
    def calculate_pixel_difference(self) -> Dict[str, Any]:
        """Calculate pixel-by-pixel difference metrics"""
        if not self.is_valid():
            return {"error": "Images not loaded"}
        
        # Check dimensions
        dim_ok, msg = self.check_dimensions()
        if not dim_ok:
            return {"error": msg}
        
        try:
            # Convert to RGB for comparison
            img1_rgb = self.img1.convert('RGB')
            img2_rgb = self.img2.convert('RGB')
            
            # Calculate difference
            diff = ImageChops.difference(img1_rgb, img2_rgb)
            stat = ImageStat.Stat(diff)
            
            # Calculate statistics
            mean_diff = sum(stat.mean) / len(stat.mean)
            max_diff = max(stat.extrema, key=lambda x: x[1])
            
            result = {
                "identical": mean_diff == 0,
                "mean_pixel_difference": mean_diff,
                "max_pixel_difference": max(v[1] for v in stat.extrema),
                "std_deviation": sum(stat.stddev) / len(stat.stddev),
                "total_pixels": img1_rgb.width * img1_rgb.height,
            }
            
            self.log(f"Mean pixel difference: {mean_diff:.2f}")
            self.log(f"Max pixel difference: {max(v[1] for v in stat.extrema)}")
            
            return result
        
        except Exception as e:
            return {"error": f"Difference calculation failed: {e}"}
    
    def get_different_pixels(self, threshold: int = 5) -> Dict[str, Any]:
        """Find pixels that differ beyond threshold"""
        if not self.is_valid():
            return {"error": "Images not loaded"}
        
        dim_ok, msg = self.check_dimensions()
        if not dim_ok:
            return {"error": msg}
        
        try:
            img1_rgb = self.img1.convert('RGB')
            img2_rgb = self.img2.convert('RGB')
            
            pixels1 = img1_rgb.load()
            pixels2 = img2_rgb.load()
            
            diff_pixels = []
            diff_count = 0
            
            for y in range(img1_rgb.height):
                for x in range(img1_rgb.width):
                    p1 = pixels1[x, y]
                    p2 = pixels2[x, y]
                    
                    # Calculate max channel difference
                    max_diff = max(abs(p1[i] - p2[i]) for i in range(3))
                    
                    if max_diff > threshold:
                        diff_count += 1
                        if len(diff_pixels) < 100:  # Store first 100 different pixels
                            diff_pixels.append({
                                "x": x,
                                "y": y,
                                "pixel1": p1,
                                "pixel2": p2,
                                "diff": max_diff
                            })
            
            percentage = (diff_count / (img1_rgb.width * img1_rgb.height)) * 100
            
            self.log(f"Pixels different (threshold {threshold}): {diff_count} ({percentage:.2f}%)")
            
            return {
                "threshold": threshold,
                "different_pixel_count": diff_count,
                "percentage_different": percentage,
                "samples": diff_pixels
            }
        
        except Exception as e:
            return {"error": f"Pixel analysis failed: {e}"}
    
    def classify_similarity(self) -> Tuple[str, str]:
        """Classify similarity between images"""
        if not self.is_valid():
            return "error", "Images not loaded"
        
        dim_ok, _ = self.check_dimensions()
        if not dim_ok:
            return "different", "Different dimensions"
        
        mode_ok, _ = self.check_mode()
        if not mode_ok:
            return "different", "Different color modes"
        
        diff = self.calculate_pixel_difference()
        if "error" in diff:
            return "error", diff["error"]
        
        mean_diff = diff.get("mean_pixel_difference", 0)
        
        if mean_diff == 0:
            return "identical", "Pixel-perfect match"
        elif mean_diff < 1:
            return "virtually_identical", "Imperceptible difference"
        elif mean_diff < 5:
            return "very_similar", "Minor differences (compression, etc)"
        elif mean_diff < 20:
            return "similar", "Noticeable but minor differences"
        elif mean_diff < 50:
            return "different", "Significant differences"
        else:
            return "very_different", "Major differences"
    
    def save_difference_image(self, output_path: str, highlight: int = 50) -> bool:
        """Save a visualization of differences between images"""
        if not self.is_valid():
            print("Error: Images not loaded")
            return False
        
        dim_ok, _ = self.check_dimensions()
        if not dim_ok:
            print("Error: Images have different dimensions")
            return False
        
        try:
            img1_rgb = self.img1.convert('RGB')
            img2_rgb = self.img2.convert('RGB')
            
            # Create difference image with highlighted differences
            pixels1 = img1_rgb.load()
            pixels2 = img2_rgb.load()
            
            diff_img = Image.new('RGB', img1_rgb.size, color=(0, 0, 0))
            diff_pixels = diff_img.load()
            
            for y in range(img1_rgb.height):
                for x in range(img1_rgb.width):
                    p1 = pixels1[x, y]
                    p2 = pixels2[x, y]
                    
                    # Calculate difference
                    diff = tuple(abs(a - b) for a, b in zip(p1, p2))
                    max_diff = max(diff)
                    
                    if max_diff > highlight:
                        # Highlight differences in red
                        diff_pixels[x, y] = (255, 0, 0)
                    else:
                        # Show as grayscale based on difference magnitude
                        gray = int(max_diff * 255 / highlight)
                        diff_pixels[x, y] = (gray, gray, gray)
            
            diff_img.save(output_path)
            self.log(f"Saved difference image to {output_path}")
            return True
        
        except Exception as e:
            print(f"Error saving difference image: {e}")
            return False
    
    def print_report(self):
        """Print comprehensive comparison report"""
        print(f"\n{'='*70}")
        print(f"Image Comparison Report")
        print(f"{'='*70}\n")
        
        print(f"Image 1: {self.img1_path}")
        print(f"Image 2: {self.img2_path}\n")
        
        if not self.is_valid():
            print("ERROR: Failed to load one or both images")
            for err in self.errors:
                print(f"  - {err}")
            return
        
        # Dimensions
        dim1, dim2 = self.get_dimensions()
        print(f"Dimensions:")
        print(f"  Image 1: {dim1['size']} ({dim1['mode']})")
        print(f"  Image 2: {dim2['size']} ({dim2['mode']})")
        
        dim_ok, msg = self.check_dimensions()
        print(f"  Status: {'✓ MATCH' if dim_ok else '✗ ' + msg}\n")
        
        # Similarity classification
        classification, detail = self.classify_similarity()
        print(f"Similarity Classification: {classification.upper()}")
        print(f"  {detail}\n")
        
        # Pixel differences
        diff = self.calculate_pixel_difference()
        if "error" not in diff:
            print(f"Pixel Differences:")
            print(f"  Identical: {diff.get('identical', False)}")
            print(f"  Mean: {diff.get('mean_pixel_difference', 0):.2f}")
            print(f"  Max: {diff.get('max_pixel_difference', 0)}")
            print(f"  Std Dev: {diff.get('std_deviation', 0):.2f}\n")
        
        # Different pixels
        different = self.get_different_pixels(threshold=5)
        if "error" not in different:
            pct = different.get('percentage_different', 0)
            print(f"Pixels Different (threshold 5):")
            print(f"  Count: {different.get('different_pixel_count', 0)}")
            print(f"  Percentage: {pct:.2f}%")
            
            if different.get('samples'):
                print(f"  Sample differences:")
                for sample in different['samples'][:3]:
                    print(f"    ({sample['x']}, {sample['y']}): "
                          f"{sample['pixel1']} -> {sample['pixel2']} (diff {sample['diff']})")
        
        print(f"\n{'='*70}\n")


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Compare two images for overlay and content verification"
    )
    parser.add_argument("image1", help="First image (actual)")
    parser.add_argument("image2", help="Second image (expected)")
    parser.add_argument("--threshold", type=int, default=5,
                       help="Pixel difference threshold (default: 5)")
    parser.add_argument("--save-diff", metavar="FILE",
                       help="Save difference visualization")
    parser.add_argument("-v", "--verbose", action="store_true",
                       help="Verbose output")
    parser.add_argument("--json", action="store_true",
                       help="Output results as JSON")
    
    args = parser.parse_args()
    
    # Create comparator
    comp = ImageComparator(args.image1, args.image2, verbose=args.verbose)
    
    # Print report
    comp.print_report()
    
    # Save difference image if requested
    if args.save_diff:
        comp.save_difference_image(args.save_diff, highlight=args.threshold)
    
    # Exit with appropriate code
    classification, _ = comp.classify_similarity()
    if classification == "error":
        sys.exit(2)
    elif classification.startswith("different"):
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
