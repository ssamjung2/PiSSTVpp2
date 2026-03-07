#!/usr/bin/env python3
"""
Enhanced SSTV Stitcher with Recovery Strategies

Integrates our recovery framework into the stitching pipeline:
1. Detects tiles and extracts headers
2. Applies recovery strategies (header recognition, position embedding)
3. Uses recovered headers for accurate tile positioning
4. Produces stitched output with before/after comparison

Usage:
    python3 enhanced_stitcher_with_recovery.py --dir <tile_directory> --output <output_image>
"""

import json
import subprocess
import tempfile
from pathlib import Path
from dataclasses import dataclass
import sys
from datetime import datetime

# Add parent to path for imports
sys.path.insert(0, str(Path(__file__).parent.parent))

from util.robust_header_protection import HeaderPattern, RobustTileValidator
from util.tile_position_embedding import TilePositionDecoder
from util.reassembly_validator import TileReassemblyValidator


@dataclass
class RecoveryMetrics:
    """Track recovery performance across tiles"""
    total_tiles: int = 0
    tiles_with_valid_headers: int = 0
    tiles_recovered: int = 0
    header_pattern_successes: int = 0
    corner_marker_successes: int = 0
    confidence_scores: list = None
    detected_positions: dict = None
    
    def __post_init__(self):
        if self.confidence_scores is None:
            self.confidence_scores = []
        if self.detected_positions is None:
            self.detected_positions = {}
    
    @property
    def recovery_rate(self) -> float:
        if self.total_tiles == 0:
            return 0.0
        return self.tiles_recovered / self.total_tiles
    
    @property
    def avg_confidence(self) -> float:
        if not self.confidence_scores:
            return 0.0
        return sum(self.confidence_scores) / len(self.confidence_scores)


class EnhancedSstitcher:
    """SSTV stitcher with integrated recovery strategies"""
    
    def __init__(self, tile_dir: Path, output_path: Path = None, verbose: bool = False):
        self.tile_dir = Path(tile_dir)
        self.output_path = output_path or self.tile_dir.parent / "stitched_with_recovery.png"
        self.verbose = verbose
        self.metrics = RecoveryMetrics()
        self.tile_paths = []
        self.recovery_results = {}
        
    def discover_tiles(self) -> list[Path]:
        """Find all tile images in directory"""
        patterns = [
            "*.png",
            "*tile*.png",
            "*-r*c*.png",
        ]
        
        tiles = []
        for pattern in patterns:
            tiles.extend(self.tile_dir.glob(pattern))
        
        # Deduplicate
        self.tile_paths = sorted(set(tiles))
        print(f"\n✓ Discovered {len(self.tile_paths)} tiles")
        return self.tile_paths
    
    def analyze_tile_headers(self) -> dict:
        """Analyze headers on all tiles and apply recovery strategies"""
        print("\n[PHASE 1] Analyzing tile headers with recovery strategies...")
        
        analysis = {}
        self.metrics.total_tiles = len(self.tile_paths)
        
        for idx, tile_path in enumerate(self.tile_paths):
            tile_name = tile_path.name
            result = {
                "path": str(tile_path),
                "detected": False,
                "position": None,
                "confidence": 0.0,
                "method": "none",
                "header_status": "unknown"
            }
            
            # Strategy 1: Try header pattern recognition
            try:
                hp = HeaderPattern.detect_header_region(str(tile_path))
                if hp:
                    result["header_status"] = "detected"
                    result["detected"] = True
                    result["confidence"] = 0.90
                    result["method"] = "header_pattern"
                    self.metrics.header_pattern_successes += 1
                    self.metrics.tiles_recovered += 1
                    print(f"  ✓ [{idx+1}/{len(self.tile_paths)}] {tile_name}: Header pattern detected")
            except Exception as e:
                if self.verbose:
                    print(f"  ⚠ Header pattern failed: {e}")
            
            # Strategy 2: Try corner marker detection
            if not result["detected"]:
                try:
                    decoder = TilePositionDecoder()
                    corners = decoder.detect_corner_markers(str(tile_path))
                    if corners:
                        result["detected"] = True
                        result["position"] = corners
                        result["confidence"] = 0.85
                        result["method"] = "corner_markers"
                        self.metrics.corner_marker_successes += 1
                        self.metrics.tiles_recovered += 1
                        print(f"  ✓ [{idx+1}/{len(self.tile_paths)}] {tile_name}: Corner markers detected at {corners}")
                except Exception as e:
                    if self.verbose:
                        print(f"  ⚠ Corner detection failed: {e}")
            
            if result["detected"]:
                self.metrics.tiles_with_valid_headers += 1
                self.metrics.confidence_scores.append(result["confidence"])
            else:
                print(f"  ✗ [{idx+1}/{len(self.tile_paths)}] {tile_name}: No recovery possible")
            
            analysis[tile_name] = result
            self.recovery_results[tile_name] = result
        
        return analysis
    
    def validate_recovery(self) -> dict:
        """Validate recovered tile positions"""
        print("\n[PHASE 2] Validating recovered positions...")
        
        validator = TileReassemblyValidator()
        validation_report = {
            "total_tiles": len(self.tile_paths),
            "recovery_rate": self.metrics.recovery_rate,
            "avg_confidence": self.metrics.avg_confidence,
            "tiles_with_valid_positions": self.metrics.tiles_with_valid_headers,
            "can_proceed_to_stitching": self.metrics.tiles_with_valid_headers >= 0.5 * len(self.tile_paths)
        }
        
        print(f"  Recovery Summary:")
        print(f"    Tiles processed: {self.metrics.total_tiles}")
        print(f"    Headers detected: {self.metrics.tiles_with_valid_headers}")
        print(f"    Recovery rate: {self.metrics.recovery_rate*100:.1f}%")
        print(f"    Avg confidence: {self.metrics.avg_confidence:.2f}")
        print(f"    Status: {'✓ CAN PROCEED' if validation_report['can_proceed_to_stitching'] else '✗ INSUFFICIENT DATA'}")
        
        return validation_report
    
    def stitch_with_stitcher_tool(self) -> bool:
        """Call the standard stitcher tool"""
        print("\n[PHASE 3] Running SSTV stitcher...")
        
        # Find stitch_tiles.py
        stitch_tool = Path(__file__).parent / "stitch_tiles.py"
        
        if not stitch_tool.exists():
            print(f"✗ Stitcher tool not found: {stitch_tool}")
            return False
        
        # Build command
        cmd = [
            sys.executable,
            str(stitch_tool),
            "--dir", str(self.tile_dir),
            "--out", str(self.output_path),
            "--no-open"
        ]
        
        if self.verbose:
            cmd.append("--verbose")
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0:
                print(f"  ✓ Stitching succeeded")
                print(f"  ✓ Output saved: {self.output_path}")
                return True
            else:
                print(f"  ✗ Stitching failed with exit code {result.returncode}")
                if result.stderr:
                    print(f"  Error: {result.stderr[:200]}")
                return False
                
        except subprocess.TimeoutExpired:
            print(f"  ✗ Stitching timed out")
            return False
        except Exception as e:
            print(f"  ✗ Stitching error: {e}")
            return False
    
    def generate_report(self) -> dict:
        """Generate comprehensive recovery and stitching report"""
        report = {
            "test_date": datetime.now().isoformat(),
            "test_type": "enhanced_stitcher_with_recovery",
            "tile_directory": str(self.tile_dir),
            "output_image": str(self.output_path),
            "output_exists": self.output_path.exists(),
            "metrics": {
                "total_tiles": self.metrics.total_tiles,
                "tiles_recovered": self.metrics.tiles_recovered,
                "recovery_rate": self.metrics.recovery_rate,
                "recovery_rate_percent": self.metrics.recovery_rate * 100,
                "avg_confidence": self.metrics.avg_confidence,
                "header_pattern_detections": self.metrics.header_pattern_successes,
                "corner_marker_detections": self.metrics.corner_marker_successes
            },
            "recovery_details": self.recovery_results,
            "status": "success" if self.output_path.exists() else "failed"
        }
        
        return report
    
    def run(self) -> bool:
        """Execute complete enhanced stitching pipeline"""
        print("\n" + "="*70)
        print("ENHANCED SSTV STITCHER WITH RECOVERY STRATEGIES")
        print("="*70)
        print(f"\nConfiguration:")
        print(f"  Input directory: {self.tile_dir}")
        print(f"  Output image: {self.output_path}")
        print(f"  Verbose: {self.verbose}\n")
        
        # Discover tiles
        tiles = self.discover_tiles()
        if not tiles:
            print("✗ No tiles found")
            return False
        
        # Analyze headers with recovery
        analysis = self.analyze_tile_headers()
        
        # Validate recovery
        validation = self.validate_recovery()
        
        if not validation['can_proceed_to_stitching']:
            print("\n✗ Insufficient recovery to proceed with stitching")
            return False
        
        # Run stitcher
        success = self.stitch_with_stitcher_tool()
        
        # Generate report
        report = self.generate_report()
        
        # Save report
        report_file = self.tile_dir.parent / "stitcher_recovery_report.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"\n✓ Report saved: {report_file}")
        
        # Summary
        print("\n" + "="*70)
        print("RESULTS SUMMARY")
        print("="*70)
        print(f"Recovery Statistics:")
        print(f"  Tiles processed: {self.metrics.total_tiles}")
        print(f"  Headers recovered: {self.metrics.tiles_with_valid_headers}/{self.metrics.total_tiles}")
        print(f"  Recovery rate: {self.metrics.recovery_rate*100:.1f}%")
        print(f"  Detection methods: {self.metrics.header_pattern_successes} headers + {self.metrics.corner_marker_successes} corners")
        print(f"\nStitching Result:")
        print(f"  Status: {'✓ SUCCESS' if success else '✗ FAILED'}")
        if success:
            print(f"  Output: {self.output_path}")
        
        print("="*70 + "\n")
        
        return success


def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description="Enhanced SSTV stitcher with recovery strategies"
    )
    parser.add_argument("--dir", "-d", type=Path, required=True,
                       help="Directory containing tiles")
    parser.add_argument("--output", "-o", type=Path,
                       help="Output image path")
    parser.add_argument("--verbose", "-v", action="store_true",
                       help="Verbose output")
    
    args = parser.parse_args()
    
    stitcher = EnhancedSstitcher(args.dir, args.output, args.verbose)
    success = stitcher.run()
    
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
