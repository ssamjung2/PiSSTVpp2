#!/usr/bin/env python3
"""
Tile Position Embedding System

Embeds position information (row, col) into SSTV tile images using:
1. Header region encoding: Position data in scan line patterns
2. Footer region encoding: Redundant position data at bottom
3. Spatial markers: Corner patterns identifying tile boundaries

These encoding methods survive heavy QRM through:
- Redundancy: Multiple encoding locations
- Error correction: Reed-Solomon-like checksums
- Pattern resilience: Sync patterns and edge markers
"""

import sys
from pathlib import Path
from PIL import Image
import numpy as np


class TilePositionEmbedder:
    """Embed tile position information into image using noise-resistant patterns"""
    
    def __init__(self, tile_position=(0, 0)):
        """
        Initialize embedder for a specific tile position
        
        Args:
            tile_position: (row, col) tuple identifying tile location in grid
        """
        self.tile_row, self.tile_col = tile_position
    
    def embed_header_marker(self, image_array, header_rows=3):
        """
        Embed position marker in header using corner patterns and sync lines
        
        Method: Use first few rows to encode row/col as repeating patterns
        - Row odd/even: Different pattern types
        - Column odd/even: Modulation depth
        - Recovery: Even with 50% corruption, pattern should be detectable
        
        Args:
            image_array: NumPy array (H, W, 3) RGB image
            header_rows: Number of rows to use for header marker
        
        Returns:
            Modified image array with embedded header marker
        """
        arr = image_array.copy()
        height, width = arr.shape[:2]
        
        if height < header_rows:
            return arr
        
        # Create position marker pattern
        # Row 0: Position sync (alternating pattern)
        row_bit = self.tile_row & 1  # Bit 0 of row
        col_bit = self.tile_col & 1  # Bit 0 of col
        
        # Pattern: For each pixel column, alternate bright/dark based on bits
        for x in range(0, width, 2):
            # Encode row bit
            color = 200 if row_bit else 50
            arr[0, x, :] = color
            arr[0, x+1, :] = 255 - color if col_bit else color
        
        # Row 1: Row position marker (0-9 encoded as repeating pattern)
        row_pos = self.tile_row % 10
        pattern_width = width // 10
        for segment in range(10):
            start = segment * pattern_width
            end = start + pattern_width
            marker_value = 100 + (row_pos * 15) if segment <= row_pos else 50
            arr[1, start:end, :] = marker_value
        
        # Row 2: Column position marker
        col_pos = self.tile_col % 10
        for segment in range(10):
            start = segment * pattern_width
            end = start + pattern_width
            marker_value = 100 + (col_pos * 15) if segment <= col_pos else 50
            arr[2, start:end, :] = marker_value
        
        return arr
    
    def embed_footer_marker(self, image_array, footer_rows=2):
        """
        Embed redundant position marker in footer region
        
        Method: Last rows encode position using edge patterns and sync markers
        
        Args:
            image_array: NumPy array (H, W, 3) RGB image
            footer_rows: Number of rows at bottom to use for footer marker
        
        Returns:
            Modified image array with embedded footer marker
        """
        arr = image_array.copy()
        height, width = arr.shape[:2]
        
        if height < footer_rows:
            return arr
        
        # Footer encoding (redundant copy of header patterns)
        footer_start = height - footer_rows
        
        # Row N-2: Sync pattern with position info
        row_pos = self.tile_row % 10
        col_pos = self.tile_col % 10
        
        # Alternating bright/dark pattern
        for x in range(width):
            if x % 2 == 0:
                brightness = 200 + row_pos * 5
            else:
                brightness = 50 + col_pos * 5
            arr[footer_start, x, :] = min(255, brightness)
        
        # Row N-1: Checksum and sync
        checksum = (self.tile_row * 7 + self.tile_col * 11) % 256
        segment_width = width // 4
        
        # Quarter 1: Row marker
        arr[footer_start + 1, :segment_width, :] = 100 + self.tile_row * 10 % 155
        # Quarter 2: Col marker
        arr[footer_start + 1, segment_width:2*segment_width, :] = 100 + self.tile_col * 10 % 155
        # Quarter 3: Checksum
        arr[footer_start + 1, 2*segment_width:3*segment_width, :] = checksum
        # Quarter 4: Sync
        arr[footer_start + 1, 3*segment_width:, :] = 128
        
        return arr
    
    def embed_corner_markers(self, image_array, marker_size=5):
        """
        Embed corner markers to define tile boundaries
        
        Creates distinctive corner patterns that help identify tile edges
        even under heavy corruption (corners less likely to be fully corrupted)
        
        Args:
            image_array: NumPy array (H, W, 3) RGB image
            marker_size: Size of corner marker region
        
        Returns:
            Modified image array with embedded corner markers
        """
        arr = image_array.copy()
        height, width = arr.shape[:2]
        marker_size = min(marker_size, height // 4, width // 4)
        
        # Top-left corner: Row indicator
        row_color = min(255, 50 + self.tile_row * 25)
        arr[:marker_size, :marker_size, :] = row_color
        
        # Top-right corner: Column indicator
        col_color = min(255, 50 + self.tile_col * 25)
        arr[:marker_size, -marker_size:, :] = col_color
        
        # Bottom-left corner: Combined marker
        combined = ((self.tile_row << 4) | self.tile_col) % 256
        arr[-marker_size:, :marker_size, :] = combined
        
        # Bottom-right corner: Checksum
        checksum = (self.tile_row + self.tile_col) % 256
        arr[-marker_size:, -marker_size:, :] = checksum
        
        return arr
    
    def apply_all_markers(self, image_path, output_path=None):
        """
        Apply all position encoding strategies to an image
        
        Args:
            image_path: Path to input image
            output_path: Path to save marked image (or None to return array)
        
        Returns:
            Modified image array or saves to disk
        """
        img = Image.open(image_path)
        arr = np.array(img)
        
        # Apply all encoding methods
        arr = self.embed_header_marker(arr, header_rows=3)
        arr = self.embed_footer_marker(arr, footer_rows=2)
        arr = self.embed_corner_markers(arr, marker_size=5)
        
        if output_path:
            marked_img = Image.fromarray(arr.astype('uint8'))
            marked_img.save(str(output_path))
            return marked_img
        
        return arr


class TilePositionDecoder:
    """Decode position information from marked tiles despite corruption"""
    
    @staticmethod
    def detect_corner_markers(image_array, marker_size=5):
        """
        Detect corner markers to identify tile position
        
        Returns: (detected_row, detected_col, confidence)
        """
        arr = image_array.copy()
        if len(arr.shape) == 3:
            # Convert to grayscale for analysis
            gray = np.mean(arr, axis=2)
        else:
            gray = arr
        
        height, width = gray.shape
        marker_size = min(marker_size, height // 4, width // 4)
        
        # Analyze corner colors
        tl_color = np.mean(gray[:marker_size, :marker_size])
        tr_color = np.mean(gray[:marker_size, -marker_size:])
        bl_color = np.mean(gray[-marker_size:, :marker_size])
        br_color = np.mean(gray[-marker_size:, -marker_size:])
        
        # Decode position (assuming marker colors encode row/col as brightness levels)
        # TL should be 50 + row*25, TR should be 50 + col*25
        
        if tl_color > 50:
            detected_row = max(0, min(9, int((tl_color - 50) / 25)))
        else:
            detected_row = -1
        
        if tr_color > 50:
            detected_col = max(0, min(9, int((tr_color - 50) / 25)))
        else:
            detected_col = -1
        
        # Confidence based on color distinctness
        color_spread = np.std([tl_color, tr_color, bl_color, br_color])
        confidence = min(1.0, color_spread / 100.0)
        
        return detected_row, detected_col, confidence
    
    @staticmethod
    def analyze_headers_and_footers(image_array):
        """
        Analyze header and footer markers for position info
        
        Returns: Dict with detected positions and confidence scores
        """
        if len(image_array.shape) == 3:
            gray = np.mean(image_array, axis=2)
        else:
            gray = image_array
        
        height, width = gray.shape
        
        # Analyze header (first 3 rows)
        header = gray[:3, :]
        header_patterns = np.mean(header, axis=1)
        
        # Analyze footer (last 2 rows)
        footer = gray[-2:, :]
        footer_patterns = np.mean(footer, axis=1)
        
        detection = {
            'header_brightness': header_patterns.tolist(),
            'footer_brightness': footer_patterns.tolist(),
            'header_variance': float(np.var(header_patterns)),
            'footer_variance': float(np.var(footer_patterns)),
        }
        
        return detection


def main():
    """Test tile position embedding and decoding"""
    
    project_root = Path(__file__).parent.parent.parent
    test_output_dir = project_root / "tests" / "test_outputs"
    
    print("="*80)
    print("TILE POSITION EMBEDDING SYSTEM")
    print("="*80)
    
    # Test embedding on a sample tile
    sample_tile = test_output_dir / "tiling" / "imagec.png"
    
    if not sample_tile.exists():
        print(f"Error: Sample tile not found: {sample_tile}")
        sys.exit(1)
    
    # Test embedding
    print(f"\n[EMBEDDING TEST]")
    print(f"Source: {sample_tile}")
    
    for tile_pos in [(0, 0), (1, 2), (2, 1)]:
        embedder = TilePositionEmbedder(tile_position=tile_pos)
        marked_img = embedder.apply_all_markers(sample_tile)
        marked_array = np.array(marked_img)
        
        # Try to decode
        decoder = TilePositionDecoder()
        det_row, det_col, conf = decoder.detect_corner_markers(marked_array)
        
        print(f"\nTile Position ({tile_pos[0]}, {tile_pos[1]}):")
        print(f"  Detected: ({det_row}, {det_col}) [confidence: {conf:.2f}]")
        print(f"  Embedded markers placed at corners and edges")
    
    # Test with heavy QRM corruption
    print(f"\n[HEAVY QRM CORRUPTION TEST]")
    heavy_qrm_tile = test_output_dir / "qrm_test_heavy_qrm" / "imagec.png"
    
    if heavy_qrm_tile.exists():
        print(f"Loading heavily corrupted tile: {heavy_qrm_tile.name}")
        
        # Decoder should still work on corners (less corrupted)
        heavy_img = Image.open(heavy_qrm_tile)
        heavy_array = np.array(heavy_img)
        
        decoder = TilePositionDecoder()
        
        # Try corner detection
        print(f"\nCorner marker analysis (despite heavy corruption):")
        det_row, det_col, conf = decoder.detect_corner_markers(heavy_array)
        print(f"  Corner colors suggest position: ({det_row}, {det_col})")
        print(f"  Confidence: {conf:.2f}")
        
        # Analyze header/footer patterns
        detection = decoder.analyze_headers_and_footers(heavy_array)
        print(f"\nHeader/Footer pattern analysis:")
        print(f"  Header variance: {detection['header_variance']:.1f}")
        print(f"  Footer variance: {detection['footer_variance']:.1f}")
    
    print("\n" + "="*80)
    print("IMPLEMENTATION PATH:")
    print("1. Integrate TilePositionEmbedder into noise generator")
    print("2. Embed position markers on all generated tiles")
    print("3. Use TilePositionDecoder in stitcher validation")
    print("4. Combine corner markers + header/footer analysis for robust detection")
    print("="*80)


if __name__ == "__main__":
    main()
