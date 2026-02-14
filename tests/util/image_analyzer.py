#!/usr/bin/env python3
"""
Image Analyzer for PiSSTVpp2 Output Verification

Analyzes SSTV output images to verify:
- Text overlays were applied correctly
- Colors match specifications
- Placement matches specifications
- No corruption or rendering errors

Usage:
    python3 image_analyzer.py <image_file> [options]
    python3 image_analyzer.py <image_file> --overlay-spec "text|alignment=center|color=white"
    python3 image_analyzer.py <actual.png> <expected.png> --compare
"""

import sys
import os
import json
import argparse
from pathlib import Path
from dataclasses import dataclass
from typing import Optional, Tuple, List, Dict, Any
import struct

try:
    from PIL import Image, ImageDraw, ImageStat, ImageOps
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


@dataclass
class ColorRGB:
    """Represents an RGB color"""
    r: int
    g: int
    b: int
    
    def __post_init__(self):
        self.r = max(0, min(255, self.r))
        self.g = max(0, min(255, self.g))
        self.b = max(0, min(255, self.b))
    
    def to_tuple(self) -> Tuple[int, int, int]:
        return (self.r, self.g, self.b)
    
    def to_hex(self) -> str:
        return f"#{self.r:02X}{self.g:02X}{self.b:02X}"
    
    def distance_to(self, other: 'ColorRGB') -> float:
        """Calculate Euclidean distance to another color"""
        return ((self.r - other.r)**2 + (self.g - other.g)**2 + (self.b - other.b)**2) ** 0.5


@dataclass
class ImageRegion:
    """Represents a rectangular region in an image"""
    x: int
    y: int
    width: int
    height: int
    
    def contains_point(self, px: int, py: int) -> bool:
        """Check if point is within region"""
        return (self.x <= px < self.x + self.width and 
                self.y <= py < self.y + self.height)
    
    def overlaps(self, other: 'ImageRegion') -> bool:
        """Check if regions overlap"""
        return not (self.x + self.width <= other.x or 
                   other.x + other.width <= self.x or
                   self.y + self.height <= other.y or
                   other.y + other.height <= self.y)
    
    def to_dict(self) -> Dict[str, int]:
        return {"x": self.x, "y": self.y, "width": self.width, "height": self.height}


@dataclass
class TextOverlaySpec:
    """Specification for text overlay from CLI flags"""
    text: str
    font_size: int = 24
    color: ColorRGB = None
    alignment: str = "left"  # left, center, right
    placement: str = "top"   # top, bottom, left, right, center
    bg_color: Optional[ColorRGB] = None
    bg_mode: str = "opaque"  # opaque, transparent, semi-transparent
    padding: int = 10
    
    def __post_init__(self):
        if self.color is None:
            self.color = ColorRGB(255, 255, 255)  # White


class ImageAnalyzer:
    """Analyzes SSTV output images for correctness"""
    
    MIN_CONTRAST = 30  # Minimum contrast to detect text
    COLOR_MATCH_THRESHOLD = 50  # Maximum color distance to match
    
    def __init__(self, image_path: str, verbose: bool = False):
        """Initialize analyzer with image file"""
        self.image_path = Path(image_path)
        self.verbose = verbose
        self.errors = []
        self.warnings = []
        self.info = []
        
        if not HAS_PIL:
            self.errors.append("PIL/Pillow not installed. Install with: pip install Pillow")
            self.image = None
            return
        
        if not self.image_path.exists():
            self.errors.append(f"Image file not found: {self.image_path}")
            self.image = None
            return
        
        try:
            self.image = Image.open(self.image_path)
            self.log_info(f"Loaded {self.image.format} image: {self.image.width}x{self.image.height}")
        except Exception as e:
            self.errors.append(f"Failed to load image: {e}")
            self.image = None
    
    def log_info(self, msg: str):
        """Log informational message"""
        self.info.append(msg)
        if self.verbose:
            print(f"[INFO] {msg}")
    
    def log_warning(self, msg: str):
        """Log warning message"""
        self.warnings.append(msg)
        if self.verbose:
            print(f"[WARN] {msg}")
    
    def log_error(self, msg: str):
        """Log error message"""
        self.errors.append(msg)
        if self.verbose:
            print(f"[ERR] {msg}")
    
    def check_valid(self) -> bool:
        """Check if image loaded successfully"""
        if self.image is None:
            return False
        if len(self.errors) > 0:
            return False
        return True
    
    def get_image_format(self) -> Dict[str, Any]:
        """Get image format information"""
        if not self.check_valid():
            return {}
        
        return {
            "format": self.image.format,
            "mode": self.image.mode,
            "width": self.image.width,
            "height": self.image.height,
            "has_alpha": self.image.mode in ['RGBA', 'LA', 'PA'],
        }
    
    def detect_text_regions(self) -> List[ImageRegion]:
        """Detect regions that likely contain text (high contrast areas)"""
        if not self.check_valid():
            return []
        
        regions = []
        
        try:
            # Convert to grayscale for analysis
            gray = self.image.convert('L')
            pixels = gray.load()
            
            # Simple edge detection: find regions with high local variance
            width, height = self.image.size
            visited = [[False] * width for _ in range(height)]
            
            for y in range(1, height - 1):
                for x in range(1, width - 1):
                    if visited[y][x]:
                        continue
                    
                    # Calculate local variance (9-pixel neighborhood)
                    center = pixels[x, y]
                    neighbors = [
                        pixels[x-1, y-1], pixels[x, y-1], pixels[x+1, y-1],
                        pixels[x-1, y],   center,          pixels[x+1, y],
                        pixels[x-1, y+1], pixels[x, y+1], pixels[x+1, y+1],
                    ]
                    
                    variance = sum((p - center)**2 for p in neighbors) / 9
                    
                    # If high variance, might be edge of text
                    if variance > self.MIN_CONTRAST:
                        # Flood fill to find connected region
                        region = self._flood_fill_region(x, y, visited)
                        if region and region.width > 5 and region.height > 5:
                            regions.append(region)
            
            self.log_info(f"Detected {len(regions)} potential text regions")
            
        except Exception as e:
            self.log_warning(f"Text detection failed: {e}")
        
        return regions
    
    def _flood_fill_region(self, start_x: int, start_y: int, 
                          visited: List[List[bool]]) -> Optional[ImageRegion]:
        """Flood fill to find connected region starting from point"""
        from collections import deque
        
        gray = self.image.convert('L')
        pixels = gray.load()
        width, height = self.image.size
        
        # BFS to find connected high-variance pixels
        queue = deque([(start_x, start_y)])
        min_x = max_x = start_x
        min_y = max_y = start_y
        count = 0
        
        while queue and count < 10000:
            x, y = queue.popleft()
            
            if not (0 <= x < width and 0 <= y < height):
                continue
            if visited[y][x]:
                continue
            
            visited[y][x] = True
            min_x = min(min_x, x)
            max_x = max(max_x, x)
            min_y = min(min_y, y)
            max_y = max(max_y, y)
            count += 1
            
            # Add neighbors
            for nx, ny in [(x-1, y), (x+1, y), (x, y-1), (x, y+1)]:
                if 0 <= nx < width and 0 <= ny < height and not visited[ny][nx]:
                    neighbor = pixels[nx, ny]
                    center = pixels[x, y]
                    if abs(neighbor - center) > 20:  # Similar contrast
                        queue.append((nx, ny))
        
        if count > 0:
            return ImageRegion(
                x=min_x,
                y=min_y,
                width=max_x - min_x + 1,
                height=max_y - min_y + 1
            )
        
        return None
    
    def detect_colors(self, num_colors: int = 10) -> List[Tuple[ColorRGB, float]]:
        """Detect dominant colors in image"""
        if not self.check_valid():
            return []
        
        try:
            # Reduce image to find dominant colors
            small = self.image.resize((50, 50))
            colors = small.getcolors(num_colors * 2)
            
            # Convert to (ColorRGB, percentage) tuples
            total = sum(count for count, _ in colors)
            result = []
            
            for count, rgb_tuple in colors:
                if len(rgb_tuple) >= 3:
                    color = ColorRGB(rgb_tuple[0], rgb_tuple[1], rgb_tuple[2])
                    percentage = (count / total) * 100
                    result.append((color, percentage))
            
            result.sort(key=lambda x: x[1], reverse=True)
            
            self.log_info(f"Detected {len(result)} dominant colors")
            
            return result[:num_colors]
        
        except Exception as e:
            self.log_warning(f"Color detection failed: {e}")
            return []
    
    def find_color_region(self, target_color: ColorRGB, tolerance: int = 50) -> Optional[ImageRegion]:
        """Find region with specified color"""
        if not self.check_valid():
            return None
        
        try:
            pixels = self.image.load()
            width, height = self.image.size
            
            min_x = min_y = width  
            max_x = max_y = 0
            found = False
            
            for y in range(height):
                for x in range(width):
                    pixel = pixels[x, y]
                    
                    # Extract RGB
                    if isinstance(pixel, tuple):
                        r, g, b = pixel[0], pixel[1], pixel[2]
                    else:
                        r = g = b = pixel
                    
                    color = ColorRGB(r, g, b)
                    
                    if color.distance_to(target_color) <= tolerance:
                        min_x = min(min_x, x)
                        max_x = max(max_x, x)
                        min_y = min(min_y, y)
                        max_y = max(max_y, y)
                        found = True
            
            if found:
                return ImageRegion(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1)
            
            return None
        
        except Exception as e:
            self.log_warning(f"Color region detection failed: {e}")
            return None
    
    def verify_overlay_spec(self, spec: TextOverlaySpec) -> bool:
        """Verify that an overlay specification was applied to image"""
        if not self.check_valid():
            return False
        
        self.log_info(f"Verifying overlay: '{spec.text}'")
        
        # Check 1: Look for text color in image
        color_region = self.find_color_region(spec.color, tolerance=self.COLOR_MATCH_THRESHOLD)
        if color_region:
            self.log_info(f"  ✓ Found text color {spec.color.to_hex()} at {color_region.to_dict()}")
        else:
            self.log_warning(f"  ✗ Text color {spec.color.to_hex()} not found in image")
            return False
        
        # Check 2: If background specified, look for background color
        if spec.bg_color and spec.bg_mode != "transparent":
            bg_region = self.find_color_region(spec.bg_color, tolerance=self.COLOR_MATCH_THRESHOLD)
            if bg_region:
                self.log_info(f"  ✓ Found background color {spec.bg_color.to_hex()} at {bg_region.to_dict()}")
            else:
                self.log_warning(f"  ✗ Background color {spec.bg_color.to_hex()} not found")
                return False
        
        # Check 3: Detect text regions
        regions = self.detect_text_regions()
        if regions:
            self.log_info(f"  ✓ Detected {len(regions)} text regions")
            
            # Check placement
            if spec.placement == "top":
                top_regions = [r for r in regions if r.y < (self.image.height * 0.3)]
                if top_regions:
                    self.log_info(f"  ✓ Found text in top region")
                else:
                    self.log_warning(f"  ✗ Expected text in top, but not found")
                    return False
            elif spec.placement == "bottom":
                bottom_regions = [r for r in regions if r.y > (self.image.height * 0.7)]
                if bottom_regions:
                    self.log_info(f"  ✓ Found text in bottom region")
                else:
                    self.log_warning(f"  ✗ Expected text in bottom, but not found")
                    return False
        else:
            self.log_warning(f"  ✗ No text regions detected")
            return False
        
        self.log_info("  Overlay verification passed")
        return True
    
    def compare_images(self, other_path: str) -> Dict[str, Any]:
        """Compare this image with another image"""
        if not self.check_valid():
            return {"status": "error", "message": "First image failed to load"}
        
        try:
            other = Image.open(other_path)
        except Exception as e:
            return {"status": "error", "message": f"Failed to load comparison image: {e}"}
        
        # Check dimensions
        if self.image.size != other.size:
            return {
                "status": "different",
                "reason": f"Different sizes: {self.image.size} vs {other.size}"
            }
        
        # Check mode
        if self.image.mode != other.mode:
            return {
                "status": "different",
                "reason": f"Different modes: {self.image.mode} vs {other.mode}"
            }
        
        # Compare pixels
        try:
            diff = ImageOps.difference(self.image.convert('RGB'), 
                                      other.convert('RGB'))
            stat = ImageStat.Stat(diff)
            
            # Calculate mean pixel difference
            mean_diff = sum(stat.mean) / len(stat.mean)
            
            if mean_diff == 0:
                return {"status": "identical", "pixel_diff": 0}
            elif mean_diff < 5:
                return {"status": "very_similar", "pixel_diff": mean_diff}
            elif mean_diff < 20:
                return {"status": "similar", "pixel_diff": mean_diff}
            else:
                return {"status": "different", "pixel_diff": mean_diff}
        
        except Exception as e:
            return {"status": "error", "message": f"Comparison failed: {e}"}
    
    def save_analysis_report(self, output_path: str):
        """Save analysis report to JSON file"""
        report = {
            "image_file": str(self.image_path),
            "status": "valid" if self.check_valid() else "error",
            "format": self.get_image_format(),
            "errors": self.errors,
            "warnings": self.warnings,
            "info": self.info,
            "colors": [(c.to_hex(), p) for c, p in self.detect_colors(5)],
            "text_regions": [r.to_dict() for r in self.detect_text_regions()],
        }
        
        with open(output_path, 'w') as f:
            json.dump(report, f, indent=2)
        
        self.log_info(f"Report saved to {output_path}")
    
    def print_summary(self):
        """Print summary of analysis"""
        print(f"\n{'='*60}")
        print(f"Image Analysis Report: {self.image_path}")
        print(f"{'='*60}")
        
        if not self.check_valid():
            print("ERROR: Failed to load image")
            for err in self.errors:
                print(f"  - {err}")
            return
        
        # Format info
        fmt = self.get_image_format()
        print(f"\nFormat: {fmt['format']} ({fmt['mode']}) {fmt['width']}x{fmt['height']}")
        
        # Colors
        colors = self.detect_colors(5)
        if colors:
            print(f"\nDominant Colors:")
            for color, pct in colors:
                print(f"  {color.to_hex()} - {pct:.1f}%")
        
        # Text regions
        regions = self.detect_text_regions()
        if regions:
            print(f"\nDetected Text Regions: {len(regions)}")
            for i, region in enumerate(regions[:5]):  # Show first 5
                print(f"  Region {i+1}: {region.to_dict()}")
        
        # Messages
        if self.warnings:
            print(f"\nWarnings:")
            for warn in self.warnings[:5]:  # Show first 5
                print(f"  - {warn}")
        
        if self.errors:
            print(f"\nErrors:")
            for err in self.errors[:5]:  # Show first 5
                print(f"  - {err}")
        
        print(f"\n{'='*60}\n")


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Analyze SSTV output images for text overlays and content"
    )
    parser.add_argument("image", help="Image file to analyze")
    parser.add_argument("--compare", metavar="OTHER_IMAGE",
                       help="Compare with another image")
    parser.add_argument("--overlay-spec", metavar="SPEC",
                       help="Verify overlay specification (format: text|color=white|placement=top)")
    parser.add_argument("--json-report", metavar="FILE",
                       help="Save detailed JSON report")
    parser.add_argument("-v", "--verbose", action="store_true",
                       help="Verbose output")
    
    args = parser.parse_args()
    
    # Create analyzer
    analyzer = ImageAnalyzer(args.image, verbose=args.verbose)
    
    # Print summary
    analyzer.print_summary()
    
    # Compare if requested
    if args.compare:
        print(f"Comparing with: {args.compare}")
        result = analyzer.compare_images(args.compare)
        print(f"Comparison result: {result}")
    
    # Verify spec if requested
    if args.overlay_spec:
        # Parse spec (simple format: text|key=value|key=value)
        parts = args.overlay_spec.split('|')
        spec = TextOverlaySpec(text=parts[0] if parts else "")
        
        for part in parts[1:]:
            if '=' in part:
                key, value = part.split('=', 1)
                if key == "color":
                    # Parse color (name or hex)
                    if value.startswith('#'):
                        r, g, b = int(value[1:3], 16), int(value[3:5], 16), int(value[5:7], 16)
                        spec.color = ColorRGB(r, g, b)
                    # Could add color name parsing here
                elif key == "placement":
                    spec.placement = value
                elif key == "size":
                    spec.font_size = int(value)
        
        success = analyzer.verify_overlay_spec(spec)
        sys.exit(0 if success else 1)
    
    # Save JSON report if requested
    if args.json_report:
        analyzer.save_analysis_report(args.json_report)
    
    sys.exit(0 if analyzer.check_valid() else 1)


if __name__ == "__main__":
    main()
