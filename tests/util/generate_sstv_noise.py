#!/usr/bin/env python3
"""
SSTV Noise Generator - Simulates real-world ham radio interference

Generates various types of noise and interference patterns that can corrupt SSTV tiles:
- White noise: General HF noise floor
- Hash lines: Synchronization errors from QRM
- Impulse noise: Single-bit corruption from QRM bursts  
- Fading flutter: Ionospheric fading effects
- Tonal interference: Narrow-band QRM at specific frequencies
- Burst noise: Short-lived interference events

Usage:
    python3 generate_sstv_noise.py --input tile.png --output corrupted.png --type qrm_light
    python3 generate_sstv_noise.py --dir tiles/ --preset heavy_qrm --noise-level 0.3
"""

import os
import sys
import argparse
import random
import math
from pathlib import Path
from dataclasses import dataclass
from typing import Optional, Tuple, List

try:
    from PIL import Image
    import numpy as np
except ImportError:
    print("Error: Required packages not found.")
    print("Install with: pip3 install pillow numpy")
    sys.exit(1)


@dataclass
class NoiseConfig:
    """Configuration for noise generation"""
    name: str
    description: str
    noise_level: float  # 0.0-1.0, amplitude of noise
    white_noise_amount: float  # 0.0-1.0
    hash_lines_amount: float  # 0.0-1.0, frequency of scan line corruption
    hash_line_width: int  # pixels high per hash line
    impulse_count: int  # number of impulse noise events
    impulse_size: int  # pixels affected per impulse
    fading_enabled: bool  # apply amplitude modulation
    fading_frequency: float  # cycles per image height
    tonal_enabled: bool  # apply narrow-band tonal interference
    tonal_frequencies: List[float]  # frequencies relative to scan rate
    burst_enabled: bool  # apply burst noise events
    burst_count: int  # number of bursts
    burst_width: int  # scan lines per burst
    header_noise_boost: float  # extra noise in header region (rows 0-8)


# Noise presets representing real-world scenarios
NOISE_PRESETS = {
    "clean": NoiseConfig(
        name="clean",
        description="Clean signal, minimal noise",
        noise_level=0.02,
        white_noise_amount=0.00,
        hash_lines_amount=0.0,
        hash_line_width=1,
        impulse_count=0,
        impulse_size=2,
        fading_enabled=False,
        fading_frequency=0.5,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=False,
        burst_count=0,
        burst_width=1,
        header_noise_boost=1.0,
    ),
    "light_qrm": NoiseConfig(
        name="light_qrm",
        description="Light QRM - occasional visible interference bands",
        noise_level=0.20,
        white_noise_amount=0.18,  # Clearly visible isolated noise bands
        hash_lines_amount=0.15,   # 1-2 corruption bands
        hash_line_width=1,
        impulse_count=2,
        impulse_size=3,
        fading_enabled=False,
        fading_frequency=0.3,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=False,
        burst_count=0,
        burst_width=1,
        header_noise_boost=1.0,
    ),
    "medium_qrm": NoiseConfig(
        name="medium_qrm",
        description="Medium QRM - noticeable noise scattered throughout",
        noise_level=0.50,
        white_noise_amount=0.48,  # Heavy visible noise bands
        hash_lines_amount=0.42,   # 3-4 corruption bands
        hash_line_width=2,
        impulse_count=7,
        impulse_size=5,
        fading_enabled=False,
        fading_frequency=0.5,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=True,
        burst_count=3,
        burst_width=3,
        header_noise_boost=1.0,
    ),
    "heavy_qrm": NoiseConfig(
        name="heavy_qrm",
        description="Heavy QRM - severe interference, image degraded",
        noise_level=0.65,
        white_noise_amount=0.62,  # Very heavy noise bands throughout
        hash_lines_amount=0.58,   # 4-5 corruption bands
        hash_line_width=3,
        impulse_count=12,
        impulse_size=6,
        fading_enabled=False,
        fading_frequency=0.8,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=True,
        burst_count=6,
        burst_width=4,
        header_noise_boost=1.0,
    ),
    "extreme_qrm": NoiseConfig(
        name="extreme_qrm",
        description="Extreme QRM - severe degradation, very corrupted",
        noise_level=0.70,
        white_noise_amount=0.65,  # Extremely heavy noise
        hash_lines_amount=0.60,   # 4-5 corruption bands
        hash_line_width=4,
        impulse_count=12,
        impulse_size=6,
        fading_enabled=False,
        fading_frequency=1.2,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=True,
        burst_count=6,
        burst_width=4,
        header_noise_boost=1.0,
    ),
    "fading_only": NoiseConfig(
        name="fading_only",
        description="Minimal noise - clean signal reference",
        noise_level=0.02,
        white_noise_amount=0.01,
        hash_lines_amount=0.0,
        hash_line_width=1,
        impulse_count=0,
        impulse_size=2,
        fading_enabled=False,
        fading_frequency=1.5,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=False,
        burst_count=0,
        burst_width=1,
        header_noise_boost=1.0,
    ),
    "tonal_interference": NoiseConfig(
        name="tonal_interference",
        description="Moderate noise with sinusoidal patterns",
        noise_level=0.30,
        white_noise_amount=0.20,
        hash_lines_amount=0.25,
        hash_line_width=2,
        impulse_count=3,
        impulse_size=3,
        fading_enabled=False,
        fading_frequency=0.3,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=False,
        burst_count=0,
        burst_width=1,
        header_noise_boost=1.0,
    ),
    "burst_noise": NoiseConfig(
        name="burst_noise",
        description="Heavy burst noise causing visible corruption bands",
        noise_level=0.40,
        white_noise_amount=0.15,
        hash_lines_amount=0.20,
        hash_line_width=2,
        impulse_count=3,
        impulse_size=3,
        fading_enabled=False,
        fading_frequency=0.3,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=True,
        burst_count=6,  # Multiple burst regions
        burst_width=3,
        header_noise_boost=1.0,
    ),
    "header_corruption": NoiseConfig(
        name="header_corruption",
        description="Heavy noise throughout all scan lines including header",
        noise_level=0.45,
        white_noise_amount=0.38,
        hash_lines_amount=0.40,
        hash_line_width=2,
        impulse_count=6,
        impulse_size=3,
        fading_enabled=False,
        fading_frequency=0.3,
        tonal_enabled=False,
        tonal_frequencies=[],
        burst_enabled=True,
        burst_count=4,
        burst_width=2,
        header_noise_boost=1.0,
    ),
}


class SStvNoiseGenerator:
    """Generate realistic SSTV noise and interference patterns"""

    def __init__(self, seed: Optional[int] = None):
        """Initialize noise generator with optional random seed for reproducibility"""
        if seed is not None:
            random.seed(seed)
            np.random.seed(seed)

    def add_white_noise(
        self, image: np.ndarray, amount: float, boost: float = 1.0
    ) -> np.ndarray:
        """
        Add white noise to isolated full-width scan line bands
        Simulates RF interference affecting entire horizontal lines during transmission
        
        Args:
            image: RGB/RGBA image as numpy array
            amount: 0.0-1.0, proportion of scan lines to affect
            boost: multiplier for noise amplitude
        
        Returns:
            Image with noise in isolated scan line bands
        """
        if amount <= 0:
            return image
        
        result = image.astype(float)
        height = image.shape[0]
        
        # Total number of lines to corrupt
        lines_to_corrupt = int(height * amount)
        
        if lines_to_corrupt > 0:
            # Create 1-3 isolated bands of corrupted scan lines
            num_bands = random.randint(1, min(3, max(1, lines_to_corrupt // 10)))
            lines_per_band = lines_to_corrupt // num_bands
            
            for _ in range(num_bands):
                # Random start position for band
                band_start = random.randint(0, max(1, height - lines_per_band - 1))
                band_end = min(band_start + lines_per_band, height)
                
                # Corrupt entire width of these scan lines with Gaussian noise
                # Much higher amplitude for visibility
                for y in range(band_start, band_end):
                    noise = np.random.normal(0, amount * boost * 255, image.shape[1:])
                    result[y] = np.clip(result[y] + noise, 0, 255)
        
        return result.astype(np.uint8)

    def add_hash_lines(
        self, image: np.ndarray, frequency: float, line_width: int, boost: float = 1.0
    ) -> np.ndarray:
        """
        Add hash line corruption in isolated scan line bands
        Creates full-width bands of heavy noise simulating RF interference disrupting sync
        
        Args:
            image: RGB/RGBA image as numpy array
            frequency: 0.0-1.0, proportion of scan lines to corrupt
            line_width: (unused, kept for API compatibility)
            boost: multiplier for noise amplitude
        
        Returns:
            Image with isolated full-width corruption bands
        """
        if frequency <= 0:
            return image
        
        result = image.astype(float)
        height = image.shape[0]
        
        # Total scan lines to corrupt
        lines_to_corrupt = int(height * frequency)
        
        if lines_to_corrupt > 0:
            # Create 1-4 isolated bands of corrupted lines
            num_bands = random.randint(1, min(4, max(1, lines_to_corrupt // 15)))
            
            for _ in range(num_bands):
                # Band height: 10-40 scan lines of corruption
                band_height = min(lines_to_corrupt // num_bands + random.randint(-5, 5), height)
                band_height = max(3, band_height)  # At least 3 lines
                
                # Random start position for band
                band_start = random.randint(0, max(1, height - band_height - 1))
                band_end = min(band_start + band_height, height)
                
                # Heavy noise across entire width of scan lines in this band
                for y in range(band_start, band_end):
                    noise = np.random.normal(0, 150 * boost, image.shape[1:])
                    result[y] = np.clip(result[y] + noise, 0, 255)
        
        return result.astype(np.uint8)

    def add_impulse_noise(
        self, image: np.ndarray, count: int, size: int, boost: float = 1.0
    ) -> np.ndarray:
        """
        Add impulse noise via random corrupted scan line bands
        For line-by-line SSTV transmission, impulses manifest as full-width corrupted bands
        
        Args:
            image: RGB/RGBA image as numpy array
            count: number of separate noise burst events
            size: (in lines, not pixels) height of each burst band
            boost: multiply effect
        
        Returns:
            Image with isolated scan line burst corruption
        """
        if count <= 0:
            return image
        
        result = image.astype(float)
        height = image.shape[0]
        max_burst_height = size * 3  # Convert size (pixels) to lines
        
        for _ in range(count):
            # Random burst band height (3-20 scan lines)
            burst_height = random.randint(3, max_burst_height)
            # Random start position
            start_y = random.randint(0, max(1, height - burst_height - 1))
            
            # Full-width noise across entire burst band
            for y in range(start_y, min(start_y + burst_height, height)):
                noise = np.random.normal(0, 100 * boost, image.shape[1:])
                result[y] = np.clip(result[y] + noise, 0, 255)
        
        return result.astype(np.uint8)

    def add_fading_flutter(
        self, image: np.ndarray, frequency: float, boost: float = 1.0
    ) -> np.ndarray:
        """
        Add ionospheric fading/flutter: amplitude modulation (DISABLED)
        
        NOTE: This effect darkens the image too much for SSTV tiles.
        Keeping function for API compatibility, but it no longer modulates amplitude.
        Real fading is better represented by increased noise in affected bands.
        
        Args:
            image: RGB/RGBA image as numpy array
            frequency: (unused) modulation frequency
            boost: (unused) severity multiplier
        
        Returns:
            Image unchanged (fading disabled for SSTV test data)
        """
        # Fading effect creates unrealistic darkening - disabled for SSTV noise generation
        return image

    def add_tonal_interference(
        self, image: np.ndarray, frequencies: List[float], boost: float = 1.0
    ) -> np.ndarray:
        """
        Add narrow-band tonal interference (birdies)
        Creates sinusoidal modulation patterns across the image
        
        Args:
            image: RGB/RGBA image as numpy array
            frequencies: list of interference frequencies (0.0-1.0, relative to scan rate)
            boost: amplitude multiplier
        
        Returns:
            Image with tonal interference added
        """
        if not frequencies:
            return image
        
        result = image.astype(float)
        height = image.shape[0]
        
        for tone_freq in frequencies:
            for y in range(height):
                # Sinusoidal interference pattern
                phase = (y / height) * 2 * math.pi * tone_freq
                tone = math.sin(phase) * 50 * boost  # Higher amplitude for visibility
                
                result[y] = np.clip(result[y] + tone, 0, 255)
        
        result = result.astype(np.uint8)
        return result

    def add_burst_noise(
        self, image: np.ndarray, count: int, width: int, boost: float = 1.0
    ) -> np.ndarray:
        """
        Add isolated burst noise events as full-width scan line bands
        Simulates RF burst QRM disrupting transmission for multiple scan lines
        
        Args:
            image: RGB/RGBA image as numpy array
            count: number of separate burst events
            width: scan lines per burst (height of each burst band)
            boost: noise amplitude multiplier
        
        Returns:
            Image with isolated scan line burst corruption
        """
        if count <= 0:
            return image
        
        result = image.astype(float)
        height = image.shape[0]
        
        # Ensure we don't try to place more bursts than possible
        max_bursts = max(1, height // (width + 1))
        actual_count = min(count, max_bursts)
        
        burst_positions = random.sample(
            range(max(1, height - width)), 
            actual_count
        )
        
        for start_y in burst_positions:
            # Full-width heavy noise band
            for y in range(start_y, min(start_y + width, height)):
                noise = np.random.normal(0, 120 * boost, image.shape[1:])
                result[y] = np.clip(result[y] + noise, 0, 255)
        
        return result.astype(np.uint8)

    def apply_header_boost(
        self, image: np.ndarray, boost: float, header_rows: int = 10
    ) -> np.ndarray:
        """
        Apply extra noise boost to SSTV header region scan lines
        Header contains critical tile metadata and is equally vulnerable to corruption
        
        Args:
            image: RGB/RGBA image as numpy array
            boost: amplitude multiplier for header region (currently 1.0 = no extra boost)
            header_rows: number of scan lines to apply boost to
        
        Returns:
            Image unchanged (header gets same noise as rest of image)
        """
        # Header is no longer boosted - it gets same noise treatment as the rest of the image
        # This ensures realistic RF corruption affects all scan lines equally
        return image

    def apply_config(
        self, image: np.ndarray, config: NoiseConfig
    ) -> np.ndarray:
        """
        Apply complete noise configuration to image
        
        Args:
            image: RGB/RGBA image as numpy array
            config: NoiseConfig with all parameters
        
        Returns:
            Image with all configured noise applied
        """
        result = image.copy()
        
        # Apply noise in order of visual impact
        result = self.add_white_noise(
            result, config.white_noise_amount, boost=config.noise_level
        )
        result = self.add_hash_lines(
            result, config.hash_lines_amount, config.hash_line_width, 
            boost=config.noise_level
        )
        result = self.add_impulse_noise(
            result, config.impulse_count, config.impulse_size, 
            boost=config.noise_level
        )
        
        if config.fading_enabled:
            result = self.add_fading_flutter(
                result, config.fading_frequency, boost=config.noise_level
            )
        
        if config.tonal_enabled:
            result = self.add_tonal_interference(
                result, config.tonal_frequencies, boost=config.noise_level
            )
        
        if config.burst_enabled:
            result = self.add_burst_noise(
                result, config.burst_count, config.burst_width, 
                boost=config.noise_level
            )
        
        result = self.apply_header_boost(result, config.header_noise_boost)
        
        return result

    def process_file(
        self, input_path: str, output_path: str, config: NoiseConfig
    ) -> bool:
        """
        Load image, apply noise config, and save result
        
        Args:
            input_path: Path to input PNG
            output_path: Path to output PNG
            config: NoiseConfig to apply
        
        Returns:
            True if successful, False otherwise
        """
        try:
            # Load image
            img = Image.open(input_path)
            original_exif = img.getexif()  # Preserve EXIF
            
            if img.mode == "RGBA":
                # Keep alpha channel separate
                rgba = np.array(img)
                rgb = rgba[:, :, :3]
            else:
                rgb = np.array(img.convert("RGB"))
            
            # Apply noise
            noisy = self.apply_config(rgb, config)
            
            # Reconstruct with alpha if needed
            if img.mode == "RGBA":
                result_img = Image.fromarray(np.dstack([noisy, rgba[:, :, 3]]), "RGBA")
            else:
                result_img = Image.fromarray(noisy, "RGB")
            
            # Save with EXIF preservation
            if original_exif:
                result_img.save(output_path, "PNG", exif=original_exif)
            else:
                result_img.save(output_path, "PNG")
            return True
        except Exception as e:
            print(f"Error processing {input_path}: {e}")
            return False


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description="Generate realistic SSTV noise and interference patterns",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Single file with light QRM
  python3 generate_sstv_noise.py --input tile.png --output noisy.png --preset light_qrm
  
  # Entire directory with heavy QRM
  python3 generate_sstv_noise.py --dir tiles/ --output-dir noisy_tiles/ --preset heavy_qrm
  
  # Custom noise configuration
  python3 generate_sstv_noise.py --input tile.png --output noisy.png \\
    --white-noise 0.15 --hash-lines 0.20 --impulses 5
  
  # List available presets
  python3 generate_sstv_noise.py --list-presets
  
  # Reproducible results with seed
  python3 generate_sstv_noise.py --input tile.png --output noisy.png --seed 42

Presets:
  clean              - Minimal noise, clean signal
  light_qrm          - Light interference, occasional artifacts
  medium_qrm         - Noticeable QRM, scattered corruption
  heavy_qrm          - Severe interference, significant data loss
  extreme_qrm        - Near-unrecoverable signal
  fading_only        - Ionospheric fading without QRM
  tonal_interference - Narrow-band interference tones (birdies)
  burst_noise        - Intermittent burst noise events
  header_corruption  - Noise focused on header region
        """,
    )

    # Input/Output options
    input_group = parser.add_mutually_exclusive_group(required=False)
    input_group.add_argument(
        "-i", "--input", help="Input PNG tile path"
    )
    input_group.add_argument(
        "-d", "--dir", help="Input directory of PNG tiles"
    )
    
    parser.add_argument(
        "-o", "--output", help="Output PNG path (for single file)"
    )
    parser.add_argument(
        "--output-dir", help="Output directory for batch processing"
    )

    # Noise configuration
    noise_group = parser.add_mutually_exclusive_group(required=False)
    noise_group.add_argument(
        "-p", "--preset",
        choices=list(NOISE_PRESETS.keys()),
        help="Predefined noise configuration",
    )
    noise_group.add_argument(
        "--custom", action="store_true", help="Use custom noise parameters"
    )

    # Custom noise parameters
    parser.add_argument(
        "--noise-level", type=float, default=0.1, help="Overall noise amplitude (0.0-1.0)"
    )
    parser.add_argument(
        "--white-noise", type=float, default=0.05, help="White noise amount (0.0-1.0)"
    )
    parser.add_argument(
        "--hash-lines", type=float, default=0.10, help="Hash line frequency (0.0-1.0)"
    )
    parser.add_argument(
        "--hash-width", type=int, default=1, help="Hash line width in pixels"
    )
    parser.add_argument(
        "--impulses", type=int, default=3, help="Number of impulse noise events"
    )
    parser.add_argument(
        "--impulse-size", type=int, default=3, help="Impulse noise region size"
    )
    parser.add_argument(
        "--fading", action="store_true", help="Enable ionospheric fading effect"
    )
    parser.add_argument(
        "--fading-frequency", type=float, default=0.5, help="Fading frequency"
    )
    parser.add_argument(
        "--bursts", type=int, default=0, help="Number of burst noise events"
    )
    parser.add_argument(
        "--burst-width", type=int, default=2, help="Burst width in scan lines"
    )
    parser.add_argument(
        "--header-boost", type=float, default=1.0, help="Header region noise boost"
    )

    # Utility options
    parser.add_argument(
        "--seed", type=int, help="Random seed for reproducibility"
    )
    parser.add_argument(
        "--list-presets", action="store_true", help="List available presets"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Verbose output"
    )

    args = parser.parse_args()

    # List presets
    if args.list_presets:
        print("\nAvailable SSTV Noise Presets:")
        print("=" * 70)
        for name, config in NOISE_PRESETS.items():
            print(f"\n{name.upper()}")
            print(f"  {config.description}")
            print(f"  White Noise: {config.white_noise_amount:.2f}")
            print(f"  Hash Lines: {config.hash_lines_amount:.2f}")
            print(f"  Impulse Events: {config.impulse_count}")
            print(f"  Fading: {config.fading_enabled}")
            print(f"  Tonal: {config.tonal_enabled}")
            print(f"  Bursts: {config.burst_count}")
        print()
        return

    # Validate input/output
    if args.input and args.dir:
        print("Error: specify either --input or --dir, not both")
        sys.exit(1)

    if not args.input and not args.dir:
        print("Error: specify either --input or --dir")
        sys.exit(1)

    # Initialize generator
    gen = SStvNoiseGenerator(seed=args.seed)

    # Determine noise config
    if args.preset:
        config = NOISE_PRESETS[args.preset]
        if args.verbose:
            print(f"Using preset: {args.preset}")
            print(f"  {config.description}")
    else:
        # Use custom or default values
        config = NoiseConfig(
            name="custom",
            description="Custom noise configuration",
            noise_level=args.noise_level,
            white_noise_amount=args.white_noise,
            hash_lines_amount=args.hash_lines,
            hash_line_width=args.hash_width,
            impulse_count=args.impulses,
            impulse_size=args.impulse_size,
            fading_enabled=args.fading,
            fading_frequency=args.fading_frequency,
            tonal_enabled=False,  # Not exposed in custom CLI
            tonal_frequencies=[],
            burst_enabled=args.bursts > 0,
            burst_count=args.bursts,
            burst_width=args.burst_width,
            header_noise_boost=args.header_boost,
        )

    # Process single file
    if args.input:
        if not args.output:
            print("Error: --output required for single file processing")
            sys.exit(1)

        if args.verbose:
            print(f"Processing: {args.input} → {args.output}")

        if gen.process_file(args.input, args.output, config):
            print(f"✓ Wrote {args.output}")
        else:
            print(f"✗ Error processing {args.input}")
            sys.exit(1)

    # Process directory
    if args.dir:
        if not args.output_dir:
            args.output_dir = f"{args.dir}_noisy"

        input_dir = Path(args.dir)
        if not input_dir.exists():
            print(f"Error: directory not found: {args.dir}")
            sys.exit(1)

        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        # Find all PNG files
        png_files = list(input_dir.glob("*.png")) + list(input_dir.glob("*.PNG"))
        if not png_files:
            print(f"No PNG files found in {args.dir}")
            sys.exit(1)

        print(f"Processing {len(png_files)} files from {args.dir}")
        print(f"Output directory: {args.output_dir}")
        
        success_count = 0
        for png_file in sorted(png_files):
            output_path = output_dir / png_file.name
            if gen.process_file(str(png_file), str(output_path), config):
                if args.verbose:
                    print(f"  ✓ {png_file.name}")
                success_count += 1
            else:
                print(f"  ✗ {png_file.name}")

        print(f"\n{success_count}/{len(png_files)} files processed successfully")

        if args.verbose:
            print(f"\nTotal files processed: {success_count}")


if __name__ == "__main__":
    main()
