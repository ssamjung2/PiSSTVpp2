#!/usr/bin/env python3
"""
Comprehensive Text Overlay Test Suite for SlowFrame
Tests all styling options, colors, placements, and QSO exchange scenarios
All tests save intermediate debug images for visual verification

Week 2 Enhancements:
- Integrated TestReportGenerator for HTML reports with embedded images
- Integrated HumanVerifier for interactive verification workflow
- Auto-generated verification checklists for text overlay feature
- Command-line flags: --report, --verify for enhanced output
"""

import os
import sys
import subprocess
import json
import time
from pathlib import Path
from datetime import datetime
import shutil

# Add util directory to path for imports
util_dir = Path(__file__).parent
if util_dir not in sys.path:
    sys.path.insert(0, str(util_dir))

# Week 1 Infrastructure - Report Generation and Verification
from test_report_generator import TestReportGenerator, TestResult
from human_verification_framework import HumanVerifier
from test_metadata import TestMetadata


class TextOverlayComprehensiveTests:
    """Comprehensive text overlay test suite with Week 1 infrastructure integration"""
    
    def __init__(self, executable_path=None, verbose=False, enable_reports=False, enable_verification=False):
        """Initialize test suite with Week 1 infrastructure
        
        Args:
            executable_path: Path to slowframe executable
            verbose: Enable verbose output
            enable_reports: Generate HTML reports with embedded images
            enable_verification: Enable interactive human verification workflow
        """
        if executable_path is None:
            script_dir = Path(__file__).parent.parent.parent
            executable_path = str(script_dir / "bin" / "slowframe")
        self.exe = executable_path
        self.verbose = verbose
        self.enable_reports = enable_reports
        self.enable_verification = enable_verification
        self.test_dir = Path(__file__).parent.parent / "test_outputs" / "text_overlay_comprehensive"
        self.test_dir.mkdir(parents=True, exist_ok=True)
        
        # Get test images
        images_dir = Path(__file__).parent.parent / "images"
        self.test_images = list(images_dir.glob("test_*.png"))
        if not self.test_images:
            self.test_images = list(images_dir.glob("alt_*.png"))
        
        if not self.test_images:
            raise FileNotFoundError(f"No test images found in {images_dir}")
        
        self.test_image = str(self.test_images[0])
        self.passed = 0
        self.failed = 0
        self.results = []
        
        # Week 1 Infrastructure - Initialize report generator and verifier
        if self.enable_reports:
            self.report_generator = TestReportGenerator("Text Overlay Comprehensive Tests")
        else:
            self.report_generator = None
        
        if self.enable_verification:
            session_file = self.test_dir / "overlay_verification_session.json"
            self.verifier = HumanVerifier(session_file=str(session_file))
        else:
            self.verifier = None
        
        # Named colors to test
        self.named_colors = [
            'red', 'lime', 'blue', 'cyan', 'magenta', 'yellow',
            'white', 'black', 'orange', 'purple', 'pink', 'green'
        ]
        
        # Hex colors to test
        self.hex_colors = [
            '#FF0000', '#00FF00', '#0000FF', '#00FFFF',
            '#FF00FF', '#FFFF00', '#FFFFFF', '#000000',
            '#FFA500', '#800080', '#FFC0CB', '#008000'
        ]
        
        # Test counter for unique IDs
        self.test_counter = 0
    
    def _create_overlay_metadata(self, test_id, title, description, text_spec=""):
        """Create TestMetadata for a text overlay test
        
        Args:
            test_id: Unique test identifier
            title: Test title
            description: Test description
            text_spec: Text specification being tested
        
        Returns:
            TestMetadata instance with text_overlay category
        """
        # Create minimal metadata for text overlay tests
        # Using placeholder values for required fields
        return TestMetadata(
            test_id=test_id,
            test_number=int(test_id.lstrip('T')),
            title=title,
            category="text_overlay",
            purpose=description,
            expected_outcome="Text overlays render correctly with specified styling",
            input_image=Path(self.test_image).name,
            input_width=320,
            input_height=256,
            output_width=320,
            output_height=256
        )
    
    def _run_command(self, args):
        """Execute slowframe with given arguments
        
        Returns:
            (returncode, stdout, stderr)
        """
        cmd = [self.exe] + args
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "TIMEOUT: Command exceeded 60 seconds"
        except Exception as e:
            return -1, "", f"ERROR: {str(e)}"
    
    def _log_test(self, test_name, status, message="", debug_image=None, test_id=None):
        """Log test result and optionally add to report
        
        Args:
            test_name: Name of the test
            status: "PASSED", "FAILED", or "SKIPPED"
            message: Additional message
            debug_image: Path to debug/output image for reporting
            test_id: Unique test ID
        """
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{timestamp}] {status:8} | {test_name:60} | {message[:50]}")
        
        # Generate test ID if not provided
        if test_id is None:
            self.test_counter += 1
            test_id = f"T{self.test_counter:03d}"
        
        result = {
            'test_id': test_id,
            'name': test_name,
            'status': status,
            'message': message,
            'timestamp': timestamp
        }
        self.results.append(result)
        
        # Add to report generator if enabled
        if self.report_generator is not None:
            output_files = {}
            if debug_image and Path(debug_image).exists():
                output_files["Output"] = debug_image
            
            # Create metadata for this test using helper
            metadata = self._create_overlay_metadata(
                test_id=test_id,
                title=test_name,
                description=message
            )
            
            try:
                self.report_generator.add_test_result(
                    test_id=test_id,
                    name=test_name,
                    suite="text_overlay",
                    status="passed" if status == "PASSED" else "failed" if status == "FAILED" else "skipped",
                    command=f"slowframe text overlay test",
                    metadata=metadata,
                    output_files=output_files,
                    automated_checks={
                        "Executed": True,
                        "No crashes": status != "FAILED",
                        "Output generated": debug_image is not None and Path(debug_image).exists(),
                    }
                )
            except Exception as e:
                if self.verbose:
                    print(f"  (Report generation warning: {str(e)[:50]})")
        
        if status == "PASSED":
            self.passed += 1
        elif status == "FAILED":
            self.failed += 1
    
    # ===========================================================================
    # STRESS TESTS - All styling options
    # ===========================================================================
    
    def test_absolute_xy_positioning_stress(self):
        """Stress test: Absolute x,y coordinate positioning"""
        print("\n" + "="*90)
        print("STRESS TEST: Absolute X,Y Coordinate Positioning")
        print("="*90)
        
        self.test_counter += 1
        test_id = f"T{self.test_counter:03d}"
        test_name = f"xy_positions_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        # Test grid of x,y positions across the image
        x_positions = [10, 50, 100, 150, 200, 250]
        y_positions = [20, 60, 120, 180]
        
        overlay_count = 0
        for x in x_positions:
            for y in y_positions[:2]:  # Limit to avoid too many overlays
                args.append('-T')
                args.append(f"({x},{y})|size=10|color=white|x={x}|y={y}")
                overlay_count += 1
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        debug_img_path = None
        if debug_img.exists():
            dest = test_dir / f"{test_name}.png"
            shutil.copy(debug_img, dest)
            debug_img_path = str(dest)
            self._log_test(
                f"Absolute X,Y positioning ({len(x_positions)} x {2} = {overlay_count} overlays)",
                "PASSED" if ret == 0 else "FAILED",
                f"Grid positions (10-250x, 20-60y) - {debug_img.stat().st_size // 1024}KB",
                debug_image=debug_img_path,
                test_id=test_id
            )
        else:
            self._log_test(
                f"Absolute X,Y positioning stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})",
                test_id=test_id
            )
    
    def test_corner_xy_positions_stress(self):
        """Stress test: Absolute x,y positioning at corners and edges"""
        print("\n" + "="*90)
        print("STRESS TEST: X,Y Positioning at Corners and Edges")
        print("="*90)
        
        test_name = f"xy_corners_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        # Corner positions with x,y coordinates
        positions = [
            (5, 5, 'TL', 'lime'),      # Top-left
            (305, 5, 'TR', 'cyan'),     # Top-right
            (5, 245, 'BL', 'yellow'),   # Bottom-left
            (305, 245, 'BR', 'magenta'),# Bottom-right
            (150, 10, 'T', 'red'),      # Top center
            (150, 245, 'B', 'green'),   # Bottom center
            (10, 125, 'L', 'orange'),   # Left center
            (310, 125, 'R', 'pink'),    # Right center
            (150, 125, 'C', 'white'),   # Center
        ]
        
        for x, y, label, color in positions:
            args.append('-T')
            args.append(f"{label}|size=12|color={color}|x={x}|y={y}")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"X,Y positioning at corners/edges (9 positions with colors)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"X,Y positioning corners stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_xy_with_alignment_stress(self):
        """Stress test: Absolute x,y positioning combined with alignment"""
        print("\n" + "="*90)
        print("STRESS TEST: X,Y Positioning with Text Alignment")
        print("="*90)
        
        test_name = f"xy_align_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        aligns = ['left', 'center', 'right']
        x_coord = 100
        y_coord = 50
        
        for i, align in enumerate(aligns):
            args.append('-T')
            y = y_coord + (i * 50)
            args.append(f"Align{align[:1].upper()}|size=14|color=white|x={x_coord}|y={y}|align={align}|bg=navy")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"X,Y with alignment (3 alignments at x={x_coord}, y=50/100/150)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"X,Y with alignment stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_all_named_colors_stress(self):
        """Stress test: Apply all named colors with various positions"""
        print("\n" + "="*90)
        print("STRESS TEST: All Named Colors with Positions and Sizes")
        print("="*90)
        
        test_name = f"colors_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        # Use temp directory for output, check there for debug image
        output_file = f"/tmp/{test_name}.wav"
        
        # Build command with all colors
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        positions = ['top', 'center', 'bottom']
        sizes = [12, 16, 20, 24]
        
        color_idx = 0
        for pos in positions:
            for size in sizes:
                if color_idx < len(self.named_colors):
                    color = self.named_colors[color_idx]
                    args.append('-T')
                    args.append(f"{color.upper()}|size={size}|color={color}|pos={pos}")
                    color_idx += 1
        
        ret, stdout, stderr = self._run_command(args)
        
        # Debug image is saved as /tmp/{test_name}.png
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            # Copy to test directory for archival
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"Named colors stress ({len(self.named_colors)} colors, 3 positions, 4 sizes)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"Named colors stress ({len(self.named_colors)} colors)",
                "FAILED",
                f"Debug image not found at {debug_img} (stderr: {stderr[:100]})"
            )
    
    def test_all_hex_colors_stress(self):
        """Stress test: Apply all hex colors"""
        print("\n" + "="*90)
        print("STRESS TEST: All Hexadecimal Colors")
        print("="*90)
        
        test_name = f"hex_colors_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        for i, color in enumerate(self.hex_colors):
            pos = ['top', 'center', 'bottom'][i % 3]
            args.append('-T')
            args.append(f"HEX_{color}|size=16|color={color}|pos={pos}|align={'left' if i % 2 == 0 else 'right'}")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"Hex colors stress ({len(self.hex_colors)} colors, 3 positions, 2 alignments)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"Hex colors stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_all_positions_stress(self):
        """Stress test: All 9 placement options"""
        print("\n" + "="*90)
        print("STRESS TEST: All 9 Placement Positions")
        print("="*90)
        
        test_name = f"positions_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        positions = [
            ('top', 'Top'),
            ('bottom', 'Bottom'),
            ('left', 'Left'),
            ('right', 'Right'),
            ('center', 'Center'),
        ]
        
        for pos, label in positions:
            args.append('-T')
            args.append(f"{label}|size=18|pos={pos}|color=white")
            args.append('-T')
            args.append(f"{label}|size=14|pos={pos}|color=yellow|pad=2")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"All positions ({len(positions)} placements, 2 sizes/colors each)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"All positions stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_all_alignments_stress(self):
        """Stress test: All text alignments and vertical alignments"""
        print("\n" + "="*90)
        print("STRESS TEST: All Text Alignments and Vertical Alignments")
        print("="*90)
        
        test_name = f"alignments_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        h_aligns = ['left', 'center', 'right']
        v_aligns = ['top', 'center', 'bottom']
        
        overlay_count = 0
        for h_align in h_aligns:
            for v_align in v_aligns:
                args.append('-T')
                args.append(f"{h_align[:1].upper()}{v_align[:1].upper()}|pos=center|align={h_align}|v-align={v_align}|size=12|color=cyan|bg=navy")
                overlay_count += 1
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"All alignments (9 h-align x v-align combinations with background)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"All alignments stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_all_font_sizes_stress(self):
        """Stress test: All font sizes from 8px to 32px"""
        print("\n" + "="*90)
        print("STRESS TEST: All Font Sizes")
        print("="*90)
        
        test_name = f"sizes_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        colors_cycle = ['red', 'lime', 'blue', 'cyan', 'yellow', 'magenta']
        sizes = [8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32]
        
        for i, size in enumerate(sizes):
            color = colors_cycle[i % len(colors_cycle)]
            pos = ['top', 'center', 'bottom'][i % 3]
            args.append('-T')
            args.append(f"S{size}px|size={size}|color={color}|pos={pos}|pad=1")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"All font sizes (8px to 32px, {len(sizes)} variations)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"All font sizes stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_all_padding_sizes_stress(self):
        """Stress test: All padding sizes"""
        print("\n" + "="*90)
        print("STRESS TEST: All Padding Sizes with Background")
        print("="*90)
        
        test_name = f"padding_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        paddings = [0, 1, 2, 3, 4, 6, 8, 12, 16]
        positions = ['top', 'center', 'bottom']
        
        for i, pad in enumerate(paddings):
            pos = positions[i % len(positions)]
            args.append('-T')
            args.append(f"Pad{pad:02d}|size=14|pos={pos}|pad={pad}|color=white|bg=red")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"All padding sizes (0-16px, {len(paddings)} variations with red background)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"All padding sizes stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_all_border_styles_stress(self):
        """Stress test: All border widths and colors"""
        print("\n" + "="*90)
        print("STRESS TEST: All Border Styles")
        print("="*90)
        
        test_name = f"borders_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        borders = [
            (0, 'none'),
            (1, 'thin'),
            (2, 'normal'),
            (3, 'thick'),
            (4, 'extra'),
        ]
        border_colors = ['black', 'white', 'red', 'lime', 'cyan']
        
        for i, (width, label) in enumerate(borders):
            color = border_colors[i % len(border_colors)]
            pos = ['top', 'center', 'bottom'][i % 3]
            args.append('-T')
            args.append(f"B{width}|size=14|pos={pos}|border={width}|color=white|bg=blue")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"All border widths (0-4px, {len(borders)} variations with blue background)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"All border styles stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    def test_background_colors_combinations_stress(self):
        """Stress test: All background colors with text colors"""
        print("\n" + "="*90)
        print("STRESS TEST: Background Color Combinations")
        print("="*90)
        
        test_name = f"bg_colors_stress_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        text_colors = ['white', 'black', 'yellow']
        bg_colors = ['red', 'green', 'blue', 'cyan', 'magenta', 'yellow']
        
        combo_count = 0
        for text_color in text_colors:
            for bg_color in bg_colors:
                args.append('-T')
                pos = ['top', 'center', 'bottom'][combo_count % 3]
                args.append(f"{text_color[:1]}{bg_color[:1]}|size=12|pos={pos}|color={text_color}|bg={bg_color}")
                combo_count += 1
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"Background color combinations ({len(text_colors)} text x {len(bg_colors)} bg = {combo_count} overlays)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"Background color combinations stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )

    def test_background_bar_basic_stress(self):
        """Stress test: Background bar feature with different colors and margins"""
        print("\n" + "="*90)
        print("STRESS TEST: Background Bar - Basic")
        print("="*90)
        
        test_name = f"bg_bar_basic_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        # Test background bar with different colors and margins
        bar_configs = [
            ('HF Signal|size=14|pos=top|color=white|bg=black|bgbar=true|bgbar-margin=2', 'HF-black-m2'),
            ('VHF Report|size=14|pos=center|color=yellow|bg=blue|bgbar=true|bgbar-margin=4', 'VHF-blue-m4'),
            ('UHF+|size=14|pos=bottom|color=white|bg=red|bgbar=true|bgbar-margin=6', 'UHF-red-m6'),
        ]
        
        for text_spec, label in bar_configs:
            args.append('-T')
            args.append(text_spec)
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"Background bar basic configurations ({len(bar_configs)} overlays)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"Background bar basic stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )

    def test_background_bar_margin_variations_stress(self):
        """Stress test: Background bar with varying margin sizes"""
        print("\n" + "="*90)
        print("STRESS TEST: Background Bar - Margin Variations")
        print("="*90)
        
        test_name = f"bg_bar_margins_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        # Test background bar with different margin sizes: 0, 2, 4, 6, 8, 10
        margins = [0, 2, 4, 6, 8, 10]
        
        for idx, margin in enumerate(margins):
            args.append('-T')
            x_pos = 20 + (idx * 40)
            y_pos = 30
            args.append(f"M{margin}|size=10|color=white|bg=navy|bgbar=true|bgbar-margin={margin}|x={x_pos}|y={y_pos}")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"Background bar margin variations ({len(margins)} different margins)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"Background bar margins stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )

    def test_background_bar_visibility_stress(self):
        """Stress test: Background bars for visibility on weak signals (HF scenario)"""
        print("\n" + "="*90)
        print("STRESS TEST: Background Bar - Visibility (HF Scenario)")
        print("="*90)
        
        test_name = f"bg_bar_visibility_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        args = ['-i', self.test_image, '-o', output_file, '-K']
        
        # High-contrast background bars for visibility on weak signals
        visibility_configs = [
            # White text on dark backgrounds
            ('W5ABC|size=16|color=white|bg=black|bgbar=true|bgbar-margin=4', 'top'),
            ('EM12AB|size=16|color=white|bg=black|bgbar=true|bgbar-margin=4', 'center'),
            # Yellow text on dark blue
            ('RST 559|size=14|color=yellow|bg=darkblue|bgbar=true|bgbar-margin=3', 'bottom'),
        ]
        
        for idx, (text_spec, pos) in enumerate(visibility_configs):
            args.append('-T')
            args.append(f"{text_spec}|pos={pos}")
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                f"Background bar visibility for HF weak signals ({len(visibility_configs)} overlays)",
                "PASSED" if ret == 0 else "FAILED",
                f"Debug image: {test_name}.png ({debug_img.stat().st_size // 1024}KB)"
            )
        else:
            self._log_test(
                f"Background bar visibility stress",
                "FAILED",
                f"Debug image not found (stderr: {stderr[:50]})"
            )
    
    # ===========================================================================
    # QSO EXCHANGE UNIT TESTS
    # ===========================================================================
    
    def test_qso_basic_exchange(self):
        """Unit test: Basic QSO exchange with callsign and grid"""
        print("\n" + "="*90)
        print("UNIT TEST: Basic QSO Exchange")
        print("="*90)
        
        test_name = f"qso_basic_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        
        args = [
            '-i', self.test_image,
            '-o', output_file,
            '-K',
            '-T', 'W5ZZZ|size=20|color=white|pos=top|align=center',
            '-T', 'EM12ab|size=16|color=yellow|pos=top|align=center'
        ]
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                "QSO Basic (Callsign + Grid)",
                "PASSED" if ret == 0 else "FAILED",
                f"W5ZZZ / EM12ab - {debug_img.stat().st_size // 1024}KB"
            )
        else:
            self._log_test("QSO Basic", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def test_qso_with_rst_report(self):
        """Unit test: QSO with RST report"""
        print("\n" + "="*90)
        print("UNIT TEST: QSO with RST Report")
        print("="*90)
        
        test_name = f"qso_rst_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        
        args = [
            '-i', self.test_image,
            '-o', output_file,
            '-K',
            '-T', 'W5ZZZ|size=18|color=lime|pos=top',
            '-T', 'EM12ab|size=14|color=cyan|pos=top',
            '-T', 'RST 579|size=16|color=yellow|pos=center',
            '-T', '2026-02-11|size=12|color=white|pos=bottom'
        ]
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                "QSO with RST (Callsign, Grid, Report, Date)",
                "PASSED" if ret == 0 else "FAILED",
                f"Complete exchange - {debug_img.stat().st_size // 1024}KB"
            )
        else:
            self._log_test("QSO with RST", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def test_qso_detailed_station_info(self):
        """Unit test: Detailed station information"""
        print("\n" + "="*90)
        print("UNIT TEST: Detailed Station Information")
        print("="*90)
        
        test_name = f"qso_detailed_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        
        args = [
            '-i', self.test_image,
            '-o', output_file,
            '-K',
            # Station ID
            '-T', 'W5ZZZ|size=18|color=white|pos=top|align=left',
            '-T', 'EM12ab|size=14|color=yellow|pos=top|align=right',
            # QSO Details
            '-T', 'SSB 80M|size=14|color=lime|pos=center|align=left',
            '-T', 'RST 579|size=14|color=cyan|pos=center|align=right',
            # Equipment & Power
            '-T', '100W|size=12|color=orange|pos=center',
            # Antenna
            '-T', 'Dipole|size=12|color=pink|pos=bottom|align=left',
            # Time
            '-T', '17:30 UTC|size=12|color=white|pos=bottom|align=right'
        ]
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                "Detailed Station Info (7 overlays: callsign, grid, mode, RST, power, antenna, time)",
                "PASSED" if ret == 0 else "FAILED",
                f"Full station info - {debug_img.stat().st_size // 1024}KB"
            )
        else:
            self._log_test("Detailed Station Info", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def test_qso_portable_mobile_maritime(self):
        """Unit test: Various callsign types (portable, mobile, maritime)"""
        print("\n" + "="*90)
        print("UNIT TEST: Callsign Variants")
        print("="*90)
        
        callsign_variants = [
            ('W5ZZZ/P', 'CM97bj', 'Portable'),
            ('K4ABC/M', 'EM75', 'Mobile'),
            ('N0CALL/MM', 'AN75', 'Maritime Mobile'),
            ('VE3XYZ/VE2', 'FN25', 'Cross-border'),
        ]
        
        for callsign, grid, variant_type in callsign_variants:
            test_name = f"qso_variant_{callsign.replace('/', '_')}_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
            test_dir = self.test_dir / test_name
            test_dir.mkdir(exist_ok=True)
            
            output_file = f"/tmp/{test_name}.wav"
            
            args = [
                '-i', self.test_image,
                '-o', output_file,
                '-K',
                '-T', f"{callsign}|size=16|color=white|pos=top|align=center",
                '-T', f"{grid}|size=14|color=yellow|pos=top|align=center",
                '-T', f"{variant_type}|size=12|color=cyan|pos=center"
            ]
            
            ret, stdout, stderr = self._run_command(args)
            
            debug_img = Path(f"/tmp/{test_name}.png")
            if debug_img.exists():
                shutil.copy(debug_img, test_dir / f"{test_name}.png")
                self._log_test(
                    f"QSO Variant: {variant_type} ({callsign})",
                    "PASSED" if ret == 0 else "FAILED",
                    f"Grid {grid} - {debug_img.stat().st_size // 1024}KB"
                )
            else:
                self._log_test(f"QSO Variant: {variant_type}", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def test_qso_various_modes(self):
        """Unit test: Various SSTV modes"""
        print("\n" + "="*90)
        print("UNIT TEST: Various SSTV Modes")
        print("="*90)
        
        modes = [
            ('SSTV M1', 'VIS 44'),
            ('SSTV M2', 'VIS 30'),
            ('SSTV M3', 'VIS 34'),
            ('Robot 36', 'Color'),
            ('Robot 72', 'Color'),
            ('Scottie 1', 'BW'),
            ('Scottie 2', 'BW'),
        ]
        
        for mode_name, mode_type in modes:
            test_name = f"qso_mode_{mode_name.replace(' ', '_')}_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
            test_dir = self.test_dir / test_name
            test_dir.mkdir(exist_ok=True)
            
            output_file = f"/tmp/{test_name}.wav"
            
            args = [
                '-i', self.test_image,
                '-o', output_file,
                '-K',
                '-T', f"W5ZZZ|size=16|color=white|pos=top",
                '-T', f"{mode_name}|size=14|color=lime|pos=center",
                '-T', f"{mode_type}|size=12|color=cyan|pos=bottom"
            ]
            
            ret, stdout, stderr = self._run_command(args)
            
            debug_img = Path(f"/tmp/{test_name}.png")
            if debug_img.exists():
                shutil.copy(debug_img, test_dir / f"{test_name}.png")
                self._log_test(
                    f"QSO Mode: {mode_name} ({mode_type})",
                    "PASSED" if ret == 0 else "FAILED",
                    f"W5ZZZ - {debug_img.stat().st_size // 1024}KB"
                )
            else:
                self._log_test(f"QSO Mode: {mode_name}", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def test_qso_power_antenna_combinations(self):
        """Unit test: Power and antenna combinations"""
        print("\n" + "="*90)
        print("UNIT TEST: Power and Antenna Combinations")
        print("="*90)
        
        power_levels = ['5W', '10W', '50W', '100W', '500W', '1000W']
        antennas = ['Dipole', 'Yagi', 'Vertical', 'Loop', 'End-fed', 'Mobile whip']
        
        for power in power_levels:
            for antenna in antennas[:2]:  # Limit to avoid too many overlays
                test_name = f"qso_equip_{power.replace(' ', '_')}_{antenna}_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
                test_dir = self.test_dir / test_name
                test_dir.mkdir(exist_ok=True)
                
                output_file = f"/tmp/{test_name}.wav"
                
                args = [
                    '-i', self.test_image,
                    '-o', output_file,
                    '-K',
                    '-T', f'W5ZZZ|size=14|color=white|pos=top',
                    '-T', f'{power}|size=14|color=yellow|pos=center|align=left',
                    '-T', f'{antenna}|size=14|color=lime|pos=center|align=right'
                ]
                
                ret, stdout, stderr = self._run_command(args)
                
                debug_img = Path(f"/tmp/{test_name}.png")
                if debug_img.exists():
                    shutil.copy(debug_img, test_dir / f"{test_name}.png")
                    self._log_test(
                        f"QSO Equipment: {power} + {antenna}",
                        "PASSED" if ret == 0 else "FAILED",
                        f"{debug_img.stat().st_size // 1024}KB"
                    )
                else:
                    self._log_test(f"QSO Equipment: {power} + {antenna}", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def test_qso_contest_format(self):
        """Unit test: Classic contest format (Callsign/Grid/RST/Power/Antenna)"""
        print("\n" + "="*90)
        print("UNIT TEST: Contest Exchange Format")
        print("="*90)
        
        test_name = f"qso_contest_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        
        args = [
            '-i', self.test_image,
            '-o', output_file,
            '-K',
            # Line 1: Callsign
            '-T', 'W5ZZZ|size=20|color=white|pos=top|align=center|pad=2',
            # Line 2: Grid Locator
            '-T', 'EM12ab|size=16|color=yellow|pos=top|align=center|pad=1',
            # Line 3: Mode and Band
            '-T', 'SSB 40M|size=14|color=lime|pos=center|align=left|pad=2',
            '-T', 'RST 599|size=14|color=cyan|pos=center|align=right|pad=2',
            # Line 4: Equipment
            '-T', '100W Yagi|size=12|color=orange|pos=bottom|align=left',
            # Line 5: Time
            '-T', 'UTC 14:32|size=12|color=pink|pos=bottom|align=right'
        ]
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                "Contest Format (6-line exchange with all details)",
                "PASSED" if ret == 0 else "FAILED",
                f"Full contest format - {debug_img.stat().st_size // 1024}KB"
            )
        else:
            self._log_test("Contest Format", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def test_qso_multi_color_scenario(self):
        """Unit test: Multi-color realistic scenario"""
        print("\n" + "="*90)
        print("UNIT TEST: Multi-Color Realistic QSO")
        print("="*90)
        
        test_name = f"qso_multicolor_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        test_dir = self.test_dir / test_name
        test_dir.mkdir(exist_ok=True)
        
        output_file = f"/tmp/{test_name}.wav"
        
        # Build a realistic multi-colored display
        args = [
            '-i', self.test_image,
            '-o', output_file,
            '-K',
            # Station info with white text
            '-T', 'STATION|size=24|color=white|pos=top|align=center|bg=navy',
            '-T', 'W5ZZZ|size=22|color=yellow|pos=top|align=center|pad=2',
            # Grid with green
            '-T', 'GRID SQUARE|size=14|color=lime|pos=center|align=left|pad=2',
            '-T', 'EM12ab|size=20|color=green|pos=center|align=left|pad=4',
            # Signal report with cyan
            '-T', 'SIGNAL REPORT|size=14|color=cyan|pos=center|align=right|pad=2',
            '-T', 'RST 579|size=20|color=cyan|pos=center|align=right|pad=4',
            # Equipment with orange
            '-T', 'EQUIPMENT|size=12|color=orange|pos=bottom|align=left',
            '-T', '100W Dipole|size=14|color=#FFA500|pos=bottom|align=left',
            # Location with magenta
            '-T', 'LOCATION|size=12|color=magenta|pos=bottom|align=right',
            '-T', 'Texas, USA|size=14|color=pink|pos=bottom|align=right'
        ]
        
        ret, stdout, stderr = self._run_command(args)
        
        debug_img = Path(f"/tmp/{test_name}.png")
        if debug_img.exists():
            shutil.copy(debug_img, test_dir / f"{test_name}.png")
            self._log_test(
                "Multi-Color QSO (9 overlays with background colors)",
                "PASSED" if ret == 0 else "FAILED",
                f"Professional format - {debug_img.stat().st_size // 1024}KB"
            )
        else:
            self._log_test("Multi-Color QSO", "FAILED", f"Debug image not found (stderr: {stderr[:50]})")
    
    def run_all_tests(self):
        """Execute all tests"""
        print(f"\n{'='*90}")
        print(f"SlowFrame Comprehensive Text Overlay Test Suite")
        print(f"Start: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"Test images: {len(self.test_images)}")
        print(f"Test output: {self.test_dir}")
        if self.enable_reports:
            print(f"HTML Reports: ENABLED")
        if self.enable_verification:
            print(f"Human Verification: ENABLED")
        print(f"{'='*90}")
        
        # Run all tests
        test_methods = [
            # Stress tests
            ('STRESS TESTS', [
                self.test_absolute_xy_positioning_stress,
                self.test_corner_xy_positions_stress,
                self.test_xy_with_alignment_stress,
                self.test_all_named_colors_stress,
                self.test_all_hex_colors_stress,
                self.test_all_positions_stress,
                self.test_all_alignments_stress,
                self.test_all_font_sizes_stress,
                self.test_all_padding_sizes_stress,
                self.test_all_border_styles_stress,
                self.test_background_colors_combinations_stress,
                self.test_background_bar_basic_stress,
                self.test_background_bar_margin_variations_stress,
                self.test_background_bar_visibility_stress,
            ]),
            # QSO unit tests
            ('QSO UNIT TESTS', [
                self.test_qso_basic_exchange,
                self.test_qso_with_rst_report,
                self.test_qso_detailed_station_info,
                self.test_qso_portable_mobile_maritime,
                self.test_qso_various_modes,
                self.test_qso_power_antenna_combinations,
                self.test_qso_contest_format,
                self.test_qso_multi_color_scenario,
            ]),
        ]
        
        for category, methods in test_methods:
            print(f"\n{'='*90}")
            print(f"{category}")
            print(f"{'='*90}")
            
            for test_method in methods:
                try:
                    test_method()
                except Exception as e:
                    print(f"ERROR in {test_method.__name__}: {str(e)[:100]}")
                    self.failed += 1
        
        # Generate reports if enabled
        if self.enable_reports and self.report_generator:
            try:
                report_path = self.test_dir / f"text_overlay_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.html"
                self.report_generator.generate_html_report(str(report_path))
                print(f"\n{'='*90}")
                print(f"HTML Report generated: {report_path}")
                print(f"{'='*90}")
            except Exception as e:
                print(f"\nWarning: Could not generate HTML report: {str(e)}")
        
        # Interactive verification if enabled
        if self.enable_verification and self.verifier:
            print(f"\n{'='*90}")
            print(f"HUMAN VERIFICATION WORKFLOW")
            print(f"{'='*90}")
            print(f"Review {len(self.results)} test results interactively...")
            print(f"Commands: [a=approve] [r=reject] [s=skip] [d=defer] [q=quit]")
            print(f"{'='*90}\n")
            
            # Simple verification prompt for overlay tests
            verified_count = 0
            for result in self.results:
                try:
                    test_id = result.get('test_id', result.get('name', 'unknown'))
                    test_name = result.get('name', 'unknown')
                    
                    verdict = input(f"\nVerify test {test_id} ({test_name})? [a/r/s/q]: ").strip().lower()
                    
                    if verdict == 'q':
                        print("Stopping verification (partial)")
                        break
                    elif verdict == 'a':
                        result['verification'] = 'approved'
                        verified_count += 1
                    elif verdict == 'r':
                        result['verification'] = 'rejected'
                        verified_count += 1
                    elif verdict == 's':
                        result['verification'] = 'skipped'
                    elif verdict == 'd':
                        result['verification'] = 'deferred'
                except Exception as e:
                    print(f"Error during verification: {str(e)}")
                    continue
            
            if verified_count > 0:
                print(f"\n{verified_count} tests verified")
        
        self.print_summary()
    
    def print_summary(self):
        """Print test summary"""
        print(f"\n{'='*90}")
        print(f"Test Summary")
        print(f"{'='*90}")
        
        total = self.passed + self.failed
        pct = (self.passed / total * 100) if total > 0 else 0
        
        print(f"Total tests:    {total}")
        print(f"✓ PASSED:       {self.passed} ({pct:.1f}%)")
        print(f"✗ FAILED:       {self.failed}")
        print(f"Test directory: {self.test_dir}")
        print(f"Timestamp:      {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"{'='*90}\n")
        
        # Save results to JSON
        results_file = self.test_dir / f"comprehensive_results_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(results_file, 'w') as f:
            json.dump({
                'summary': {
                    'passed': self.passed,
                    'failed': self.failed,
                    'total': total,
                    'percentage': pct,
                    'timestamp': datetime.now().isoformat(),
                    'test_dir': str(self.test_dir)
                },
                'results': self.results
            }, f, indent=2)
        
        print(f"Results saved: {results_file}\n")


if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(
        description="SlowFrame Comprehensive Text Overlay Test Suite",
        epilog="Week 2 enhancements: --report generates HTML with images, --verify enables interactive verification"
    )
    parser.add_argument("--exe", default=None, help="Path to slowframe executable")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output")
    parser.add_argument("--report", action="store_true", help="Generate HTML report with embedded images")
    parser.add_argument("--verify", action="store_true", help="Enable interactive human verification workflow")
    
    args = parser.parse_args()
    
    # Set default executable path if not provided
    if args.exe is None:
        script_dir = Path(__file__).parent.parent.parent
        args.exe = str(script_dir / "bin" / "slowframe")
    
    try:
        suite = TextOverlayComprehensiveTests(
            executable_path=args.exe,
            verbose=args.verbose,
            enable_reports=args.report,
            enable_verification=args.verify
        )
        suite.run_all_tests()
    except FileNotFoundError as e:
        print(f"FATAL: {str(e)}")
        sys.exit(1)
