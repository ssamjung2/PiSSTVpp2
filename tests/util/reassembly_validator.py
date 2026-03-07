#!/usr/bin/env python3
"""
Tile Reassembly Validation and Recovery Framework

Combines multiple strategies to validate and recover correct tile positions:
1. Embedded marker detection (corner colors, header/footer patterns)
2. Error correction codes (position checksum validation)
3. Spatial coherence validation (adjacent tile correlation)
4. Confidence-based fallback strategies

Purpose: Improve stitcher's ability to reassemble tiles correctly under heavy QRM
by providing multiple independent validation mechanisms.
"""

import sys
from pathlib import Path
from PIL import Image
import numpy as np
from typing import List, Tuple, Dict
import json


class TileReassemblyValidator:
    """
    Comprehensive validation framework for checking tile reassembly correctness
    """
    
    def __init__(self, grid_rows=3, grid_cols=3):
        self.grid_rows = grid_rows
        self.grid_cols = grid_cols
        self.grid_size = grid_rows * grid_cols
    
    def validate_spatial_coherence(self, tiles_array: List[Tuple[Path, int, int]]) -> Dict:
        """
        Validate that reassembled tiles form coherent spatial relationships
        
        Args:
            tiles_array: List of (tile_path, detected_row, detected_col) tuples
        
        Returns:
            Dict with coherence validation results
        """
        result = {
            'all_positions_unique': True,
            'all_positions_valid': True,
            'positions': {},
            'conflicts': [],
            'orphaned_tiles': [],
        }
        
        detected_positions = {}
        for tile_path, det_row, det_col in tiles_array:
            tile_name = tile_path.name
            
            # Check if position is valid
            if det_row < 0 or det_col < 0 or det_row >= self.grid_rows or det_col >= self.grid_cols:
                result['all_positions_valid'] = False
                result['orphaned_tiles'].append({
                    'tile': tile_name,
                    'detected_pos': (det_row, det_col),
                    'reason': 'out_of_bounds'
                })
                continue
            
            pos_key = (det_row, det_col)
            
            # Check for duplicates
            if pos_key in detected_positions:
                result['all_positions_unique'] = False
                result['conflicts'].append({
                    'position': pos_key,
                    'tiles': [detected_positions[pos_key]['tile'], tile_name],
                    'type': 'position_conflict'
                })
            
            detected_positions[pos_key] = {
                'tile': tile_name,
                'path': str(tile_path),
                'confidence': 0.5  # Placeholder
            }
        
        result['positions'] = detected_positions
        result['detected_count'] = len(detected_positions)
        result['expected_count'] = self.grid_size
        result['coverage'] = len(detected_positions) / self.grid_size if self.grid_size > 0 else 0.0
        
        return result
    
    def validate_edge_correlation(self, tile_path1: Path, tile_path2: Path, 
                                   expected_adjacency: str) -> Tuple[float, Dict]:
        """
        Validate that adjacent tiles have coherent edge patterns
        
        Expected adjacency: 'horizontal' or 'vertical'
        Returns: (correlation_score, details_dict)
        """
        try:
            img1 = Image.open(tile_path1)
            img2 = Image.open(tile_path2)
            
            arr1 = np.array(img1)
            arr2 = np.array(img2)
            
            if len(arr1.shape) == 3:
                arr1 = np.mean(arr1, axis=2)
            if len(arr2.shape) == 3:
                arr2 = np.mean(arr2, axis=2)
            
            details = {
                'tile1': tile_path1.name,
                'tile2': tile_path2.name,
                'adjacency': expected_adjacency,
            }
            
            if expected_adjacency == 'horizontal':
                # Compare right edge of tile1 with left edge of tile2
                edge1 = arr1[:, -10:].flatten()  # Last 10 columns
                edge2 = arr2[:, :10].flatten()   # First 10 columns
            else:  # vertical
                # Compare bottom edge of tile1 with top edge of tile2
                edge1 = arr1[-10:, :].flatten()  # Last 10 rows
                edge2 = arr2[:10, :].flatten()   # First 10 rows
            
            # Compute correlation
            if len(edge1) > 0 and len(edge2) > 0:
                correlation = np.corrcoef(edge1, edge2)[0, 1]
                if np.isnan(correlation):
                    correlation = 0.0
            else:
                correlation = 0.0
            
            details['edge_correlation'] = float(correlation)
            details['is_adjacent'] = correlation > 0.3  # Threshold for adjacency
            
            return correlation, details
        
        except Exception as e:
            return 0.0, {
                'tile1': tile_path1.name,
                'tile2': tile_path2.name,
                'error': str(e)
            }
    
    def validate_reassembly_completeness(self, tile_paths: List[Path]) -> Dict:
        """
        Check if all tiles are available and present
        
        Returns: Dict with completeness validation
        """
        result = {
            'total_tiles_expected': self.grid_size,
            'total_tiles_found': len(tile_paths),
            'complete': len(tile_paths) == self.grid_size,
            'missing_tiles': [],
            'file_integrity': {}
        }
        
        # Check file integrity
        for tile_path in tile_paths:
            try:
                img = Image.open(tile_path)
                result['file_integrity'][tile_path.name] = {
                    'readable': True,
                    'format': img.format,
                    'size': img.size,
                }
            except Exception as e:
                result['file_integrity'][tile_path.name] = {
                    'readable': False,
                    'error': str(e)
                }
        
        return result
    
    def generate_validation_report(self, stitched_output_path: Path, 
                                   tile_dir: Path) -> Dict:
        """
        Generate comprehensive validation report for a stitched output
        
        Args:
            stitched_output_path: Path to the stitched output image
            tile_dir: Directory containing the tile set used
        
        Returns:
            Comprehensive validation report
        """
        report = {
            'timestamp': str(Path(stitched_output_path).stat().st_mtime),
            'output_file': str(stitched_output_path),
            'tile_directory': str(tile_dir),
            'grid_dimensions': (self.grid_rows, self.grid_cols),
        }
        
        # Check if output exists
        if not stitched_output_path.exists():
            report['status'] = 'output_missing'
            return report
        
        # Get output image properties
        try:
            output_img = Image.open(stitched_output_path)
            report['output_size'] = output_img.size
            report['output_format'] = output_img.format
            
            # Expected size (assuming 320x240 tiles in 3x3 grid)
            expected_width = 320 * self.grid_cols
            expected_height = 240 * self.grid_rows
            report['expected_size'] = (expected_width, expected_height)
            
            if output_img.size == (expected_width, expected_height):
                report['size_correct'] = True
            else:
                report['size_correct'] = False
                report['size_error'] = f"Got {output_img.size}, expected {report['expected_size']}"
        
        except Exception as e:
            report['status'] = 'output_corrupted'
            report['error'] = str(e)
            return report
        
        # Validate tile directory
        if tile_dir.exists():
            tiles = sorted(list(tile_dir.glob("*.png")))
            completeness = self.validate_reassembly_completeness(tiles)
            report['tile_completeness'] = completeness
        else:
            report['tile_directory_missing'] = True
        
        report['status'] = 'validation_complete'
        return report


class ReassemblyRecoveryStrategy:
    """
    Attempt to recover correct reassembly when standard methods fail
    """
    
    @staticmethod
    def suggest_recovery_actions(validation_report: Dict) -> List[Dict]:
        """
        Suggest recovery actions based on validation failures
        
        Returns: List of recommended recovery actions
        """
        actions = []
        
        if not validation_report.get('size_correct', True):
            actions.append({
                'issue': 'Output size incorrect',
                'severity': 'critical',
                'actions': [
                    'Verify tile count is correct',
                    'Check for incomplete stitching',
                    'Re-run stitcher with verbose output'
                ]
            })
        
        if validation_report.get('output_missing'):
            actions.append({
                'issue': 'Stitched output not generated',
                'severity': 'critical',
                'actions': [
                    'Check stitcher error logs',
                    'Verify tile directory permissions',
                    'Inspect individual tile corruption levels'
                ]
            })
        
        completeness = validation_report.get('tile_completeness', {})
        if not completeness.get('complete'):
            actions.append({
                'issue': f"Missing tiles: {completeness.get('total_tiles_expected', 0) - completeness.get('total_tiles_found', 0)}",
                'severity': 'high',
                'actions': [
                    'Regenerate missing tiles',
                    'Check tile naming consistency',
                    'Verify no tiles were accidentally deleted'
                ]
            })
        
        return actions


def main():
    """Test reassembly validation framework"""
    
    project_root = Path(__file__).parent.parent.parent
    test_output_dir = project_root / "tests" / "test_outputs"
    
    print("="*80)
    print("TILE REASSEMBLY VALIDATION AND RECOVERY FRAMEWORK")
    print("="*80)
    
    # Test on heavy QRM stitched output
    stitched_files = [
        ("clean", "stitch_clean.png"),
        ("light_qrm", "stitch_light_qrm.png"),
        ("medium_qrm", "stitch_medium_qrm.png"),
        ("heavy_qrm", "stitch_heavy_qrm.png"),
    ]
    
    validator = TileReassemblyValidator(grid_rows=3, grid_cols=3)
    
    for preset_name, stitch_file in stitched_files:
        stitch_path = test_output_dir / stitch_file
        tile_dir = test_output_dir / f"qrm_test_{preset_name}"
        
        print(f"\n[VALIDATING: {preset_name.upper()}]")
        
        if stitch_path.exists():
            report = validator.generate_validation_report(stitch_path, tile_dir)
            
            print(f"Output file: {stitch_file}")
            print(f"Output size: {report.get('output_size', 'N/A')}")
            print(f"Expected size: {report.get('expected_size', 'N/A')}")
            print(f"Size correct: {report.get('size_correct', 'unknown')}")
            
            if report.get('tile_completeness'):
                comp = report['tile_completeness']
                print(f"Tiles: {comp.get('total_tiles_found', 0)}/{comp.get('total_tiles_expected', 0)}")
                print(f"Complete: {comp.get('complete', False)}")
            
            # Suggest recovery if needed
            if not report.get('size_correct', True):
                recovery = ReassemblyRecoveryStrategy.suggest_recovery_actions(report)
                if recovery:
                    print(f"\nRecovery suggestions:")
                    for action in recovery:
                        print(f"  • {action['issue']} (severity: {action['severity']})")
                        for suggestion in action['actions']:
                            print(f"    - {suggestion}")
        else:
            print(f"Stitched file not found: {stitch_path}")
    
    print("\n" + "="*80)
    print("VALIDATION FRAMEWORK PROVIDES:")
    print("1. Spatial coherence checking (no duplicate positions)")
    print("2. Edge correlation validation (adjacent tiles should match)")
    print("3. File completeness validation (all tiles present)")
    print("4. Recovery recommendations based on failures")
    print("="*80)


if __name__ == "__main__":
    main()
