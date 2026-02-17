#!/usr/bin/env python3
"""
Comprehensive test metadata definitions for all test cases.

Provides detailed information about each test including:
- Test title and purpose
- Expected outcomes
- Input/output image specifications
- Test category and classification
"""

from dataclasses import dataclass
from typing import Optional, Dict, List


@dataclass
class TestMetadata:
    """Complete metadata for a test case"""
    test_id: str  # unique identifier
    test_number: int  # sequential number
    title: str  # test case title
    category: str  # "aspect_ratio", "text_overlay", "image_format"
    purpose: str  # what the test validates
    expected_outcome: str  # what should happen
    input_image: str  # input filename
    input_width: int  # input width
    input_height: int  # input height
    output_width: int  # expected output width
    output_height: int  # expected output height
    mode: Optional[str] = None  # "center", "pad", "stretch" for aspect tests
    protocol: Optional[str] = None  # "m1", "r36", etc. for SSTV tests
    test_type: Optional[str] = None  # detailed type info
    verification_checklist: List[str] = None  # Human verification checklist items
    
    def __post_init__(self):
        """Initialize default verification checklist based on category"""
        if self.verification_checklist is None:
            self.verification_checklist = self._generate_default_checklist()
    
    def _generate_default_checklist(self) -> List[str]:
        """Generate default verification checklist based on test category"""
        if self.category == "aspect_ratio":
            checklist = [
                f"Output dimensions are exactly {self.output_width}x{self.output_height}",
                "Image quality is acceptable (no severe artifacts)",
            ]
            
            if self.mode == "center":
                checklist.extend([
                    "Image is centered (equal cropping on all sides)",
                    "No black padding bars present"
                ])
            elif self.mode == "pad":
                checklist.extend([
                    "Black padding bars are present where expected",
                    "Padding color is pure black (0,0,0)",
                    "Image is centered within padded area"
                ])
            elif self.mode == "stretch":
                checklist.extend([
                    "Image fills entire frame",
                    "Distortion is as expected (non-uniform scaling)"
                ])
            
            return checklist
        
        elif self.category == "text_overlay":
            return [
                "Text is rendered and visible",
                "Text color matches specification",
                "Text placement is correct (top/bottom/custom)",
                "Text is readable and clear",
                "No clipping or artifacts around text",
                "Background/styling applied correctly if specified"
            ]
        
        elif self.category == "image_format":
            return [
                "Image loaded successfully",
                "Colors appear correct (no corruption)",
                "No visual artifacts from format conversion",
                f"Dimensions preserved ({self.output_width}x{self.output_height})"
            ]
        
        else:
            return [
                "Output appears correct",
                "No unexpected errors or artifacts"
            ]


# Aspect ratio test metadata
ASPECT_TESTS = [
    # CENTER mode - M1 protocol (320x256)
    TestMetadata(
        test_id="center_m1_wide",
        test_number=1,
        title="Aspect Ratio - Center Mode with Wide Image (M1)",
        category="aspect_ratio",
        purpose="Verify CENTER mode crops wide images to target aspect ratio without padding",
        expected_outcome="Output should be 320x256 (M1), image centered with equal cropping on sides",
        mode="center",
        protocol="m1",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=256,
        test_type="aspect_preservation"
    ),
    TestMetadata(
        test_id="center_m1_tall",
        test_number=2,
        title="Aspect Ratio - Center Mode with Tall Image (M1)",
        category="aspect_ratio",
        purpose="Verify CENTER mode crops tall images to target aspect ratio without padding",
        expected_outcome="Output should be 320x256 (M1), image centered with equal cropping on top/bottom",
        mode="center",
        protocol="m1",
        input_image="alt2_color_bars_680×1209.png",
        input_width=680,
        input_height=1209,
        output_width=320,
        output_height=256,
        test_type="aspect_preservation"
    ),
    TestMetadata(
        test_id="center_m1_square",
        test_number=3,
        title="Aspect Ratio - Center Mode with Square Image (M1)",
        category="aspect_ratio",
        purpose="Verify CENTER mode handles square images correctly",
        expected_outcome="Output should be 320x256 (M1), properly scaled from square input",
        mode="center",
        protocol="m1",
        input_image="alt3_color_bars_1370×1080.png",
        input_width=1370,
        input_height=1080,
        output_width=320,
        output_height=256,
        test_type="aspect_preservation"
    ),
    TestMetadata(
        test_id="center_m1_target",
        test_number=4,
        title="Aspect Ratio - Center Mode with Target Aspect Image (M1)",
        category="aspect_ratio",
        purpose="Verify CENTER mode preserves already-correct aspect ratio",
        expected_outcome="Output should be 320x256 (M1), identical to input aspect ratio",
        mode="center",
        protocol="m1",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="aspect_preservation"
    ),
    
    # CENTER mode - R36 protocol (320x240)
    TestMetadata(
        test_id="center_r36_wide",
        test_number=5,
        title="Aspect Ratio - Center Mode with Wide Image (R36)",
        category="aspect_ratio",
        purpose="Verify CENTER mode crops wide images to R36 target aspect ratio",
        expected_outcome="Output should be 320x240 (R36), centered with equal cropping on sides",
        mode="center",
        protocol="r36",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=240,
        test_type="aspect_preservation"
    ),
    TestMetadata(
        test_id="center_r36_tall",
        test_number=6,
        title="Aspect Ratio - Center Mode with Tall Image (R36)",
        category="aspect_ratio",
        purpose="Verify CENTER mode crops tall images to R36 target aspect ratio",
        expected_outcome="Output should be 320x240 (R36), centered with equal cropping top/bottom",
        mode="center",
        protocol="r36",
        input_image="alt2_color_bars_680×1209.png",
        input_width=680,
        input_height=1209,
        output_width=320,
        output_height=240,
        test_type="aspect_preservation"
    ),
    TestMetadata(
        test_id="center_r36_square",
        test_number=7,
        title="Aspect Ratio - Center Mode with Square Image (R36)",
        category="aspect_ratio",
        purpose="Verify CENTER mode handles square images for R36 protocol",
        expected_outcome="Output should be 320x240 (R36), properly scaled from square input",
        mode="center",
        protocol="r36",
        input_image="alt3_color_bars_1370×1080.png",
        input_width=1370,
        input_height=1080,
        output_width=320,
        output_height=240,
        test_type="aspect_preservation"
    ),
    TestMetadata(
        test_id="center_r36_target",
        test_number=8,
        title="Aspect Ratio - Center Mode with Target Aspect Image (R36)",
        category="aspect_ratio",
        purpose="Verify CENTER mode preserves R36 aspect ratio",
        expected_outcome="Output should be 320x240 (R36), identical to input aspect ratio",
        mode="center",
        protocol="r36",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=240,
        test_type="aspect_preservation"
    ),
    
    # PAD mode - M1 protocol (320x256)
    TestMetadata(
        test_id="pad_m1_wide",
        test_number=9,
        title="Aspect Ratio - Pad Mode with Wide Image (M1)",
        category="aspect_ratio",
        purpose="Verify PAD mode adds black bars to wide images to reach target aspect",
        expected_outcome="Output should be 320x256 (M1) with black bars on top/bottom",
        mode="pad",
        protocol="m1",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=256,
        test_type="aspect_padding"
    ),
    TestMetadata(
        test_id="pad_m1_tall",
        test_number=10,
        title="Aspect Ratio - Pad Mode with Tall Image (M1)",
        category="aspect_ratio",
        purpose="Verify PAD mode adds black bars to tall images for M1 protocol",
        expected_outcome="Output should be 320x256 (M1) with black bars on sides",
        mode="pad",
        protocol="m1",
        input_image="alt2_color_bars_680×1209.png",
        input_width=680,
        input_height=1209,
        output_width=320,
        output_height=256,
        test_type="aspect_padding"
    ),
    TestMetadata(
        test_id="pad_m1_square",
        test_number=11,
        title="Aspect Ratio - Pad Mode with Square Image (M1)",
        category="aspect_ratio",
        purpose="Verify PAD mode handles square images with appropriate padding",
        expected_outcome="Output should be 320x256 (M1) with black bars",
        mode="pad",
        protocol="m1",
        input_image="alt3_color_bars_1370×1080.png",
        input_width=1370,
        input_height=1080,
        output_width=320,
        output_height=256,
        test_type="aspect_padding"
    ),
    TestMetadata(
        test_id="pad_m1_target",
        test_number=12,
        title="Aspect Ratio - Pad Mode with Target Aspect Image (M1)",
        category="aspect_ratio",
        purpose="Verify PAD mode preserves images already at target aspect",
        expected_outcome="Output should be 320x256 (M1) without additional padding",
        mode="pad",
        protocol="m1",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="aspect_padding"
    ),
    
    # PAD mode - R36 protocol (320x240)
    TestMetadata(
        test_id="pad_r36_wide",
        test_number=13,
        title="Aspect Ratio - Pad Mode with Wide Image (R36)",
        category="aspect_ratio",
        purpose="Verify PAD mode adds bars to wide images for R36 protocol",
        expected_outcome="Output should be 320x240 (R36) with appropriate padding",
        mode="pad",
        protocol="r36",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=240,
        test_type="aspect_padding"
    ),
    TestMetadata(
        test_id="pad_r36_tall",
        test_number=14,
        title="Aspect Ratio - Pad Mode with Tall Image (R36)",
        category="aspect_ratio",
        purpose="Verify PAD mode adds bars to tall images for R36 protocol",
        expected_outcome="Output should be 320x240 (R36) with padding on sides",
        mode="pad",
        protocol="r36",
        input_image="alt2_color_bars_680×1209.png",
        input_width=680,
        input_height=1209,
        output_width=320,
        output_height=240,
        test_type="aspect_padding"
    ),
    TestMetadata(
        test_id="pad_r36_square",
        test_number=15,
        title="Aspect Ratio - Pad Mode with Square Image (R36)",
        category="aspect_ratio",
        purpose="Verify PAD mode handles square images for R36",
        expected_outcome="Output should be 320x240 (R36) with black bars",
        mode="pad",
        protocol="r36",
        input_image="alt3_color_bars_1370×1080.png",
        input_width=1370,
        input_height=1080,
        output_width=320,
        output_height=240,
        test_type="aspect_padding"
    ),
    TestMetadata(
        test_id="pad_r36_target",
        test_number=16,
        title="Aspect Ratio - Pad Mode with Target Aspect Image (R36)",
        category="aspect_ratio",
        purpose="Verify PAD mode preserves R36 target aspect ratio",
        expected_outcome="Output should be 320x240 (R36) without additional padding",
        mode="pad",
        protocol="r36",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=240,
        test_type="aspect_padding"
    ),
    
    # STRETCH mode - M1 protocol (320x256)
    TestMetadata(
        test_id="stretch_m1_wide",
        test_number=17,
        title="Aspect Ratio - Stretch Mode with Wide Image (M1)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode scales wide images to target without padding",
        expected_outcome="Output should be 320x256 (M1), stretched to fill without black bars",
        mode="stretch",
        protocol="m1",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=256,
        test_type="aspect_stretch"
    ),
    TestMetadata(
        test_id="stretch_m1_tall",
        test_number=18,
        title="Aspect Ratio - Stretch Mode with Tall Image (M1)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode scales tall images to target aspect",
        expected_outcome="Output should be 320x256 (M1), stretched without padding",
        mode="stretch",
        protocol="m1",
        input_image="alt2_color_bars_680×1209.png",
        input_width=680,
        input_height=1209,
        output_width=320,
        output_height=256,
        test_type="aspect_stretch"
    ),
    TestMetadata(
        test_id="stretch_m1_square",
        test_number=19,
        title="Aspect Ratio - Stretch Mode with Square Image (M1)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode handles square images",
        expected_outcome="Output should be 320x256 (M1), stretched appropriately",
        mode="stretch",
        protocol="m1",
        input_image="alt3_color_bars_1370×1080.png",
        input_width=1370,
        input_height=1080,
        output_width=320,
        output_height=256,
        test_type="aspect_stretch"
    ),
    TestMetadata(
        test_id="stretch_m1_target",
        test_number=20,
        title="Aspect Ratio - Stretch Mode with Target Aspect Image (M1)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode preserves target aspect ratio",
        expected_outcome="Output should be 320x256 (M1), no distortion needed",
        mode="stretch",
        protocol="m1",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="aspect_stretch"
    ),
    
    # STRETCH mode - R36 protocol (320x240)
    TestMetadata(
        test_id="stretch_r36_wide",
        test_number=21,
        title="Aspect Ratio - Stretch Mode with Wide Image (R36)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode scales wide images for R36 protocol",
        expected_outcome="Output should be 320x240 (R36), stretched without padding",
        mode="stretch",
        protocol="r36",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=240,
        test_type="aspect_stretch"
    ),
    TestMetadata(
        test_id="stretch_r36_tall",
        test_number=22,
        title="Aspect Ratio - Stretch Mode with Tall Image (R36)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode scales tall images for R36",
        expected_outcome="Output should be 320x240 (R36), stretched without padding",
        mode="stretch",
        protocol="r36",
        input_image="alt2_color_bars_680×1209.png",
        input_width=680,
        input_height=1209,
        output_width=320,
        output_height=240,
        test_type="aspect_stretch"
    ),
    TestMetadata(
        test_id="stretch_r36_square",
        test_number=23,
        title="Aspect Ratio - Stretch Mode with Square Image (R36)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode handles square images for R36",
        expected_outcome="Output should be 320x240 (R36), stretched appropriately",
        mode="stretch",
        protocol="r36",
        input_image="alt3_color_bars_1370×1080.png",
        input_width=1370,
        input_height=1080,
        output_width=320,
        output_height=240,
        test_type="aspect_stretch"
    ),
    TestMetadata(
        test_id="stretch_r36_target",
        test_number=24,
        title="Aspect Ratio - Stretch Mode with Target Aspect Image (R36)",
        category="aspect_ratio",
        purpose="Verify STRETCH mode preserves R36 target aspect ratio",
        expected_outcome="Output should be 320x240 (R36), no distortion",
        mode="stretch",
        protocol="r36",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=240,
        test_type="aspect_stretch"
    ),
]


# Text overlay test metadata
OVERLAY_TESTS = [
    TestMetadata(
        test_id="overlay_basic",
        test_number=25,
        title="Text Overlay - Basic Text Overlay",
        category="text_overlay",
        purpose="Verify basic text overlay functionality on SSTV images",
        expected_outcome="Image should have readable text overlay with default styling",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="text_rendering"
    ),
    TestMetadata(
        test_id="overlay_positioning",
        test_number=26,
        title="Text Overlay - Custom Text Positioning",
        category="text_overlay",
        purpose="Verify text overlay respects custom x,y positioning",
        expected_outcome="Text should appear at specified coordinates",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="text_positioning"
    ),
    TestMetadata(
        test_id="overlay_multi_line",
        test_number=27,
        title="Text Overlay - Multi-Line Text",
        category="text_overlay",
        purpose="Verify multi-line text overlay rendering",
        expected_outcome="All text lines should be rendered with proper spacing",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="text_rendering"
    ),
    TestMetadata(
        test_id="overlay_background_bar",
        test_number=28,
        title="Text Overlay - Background Bar Support",
        category="text_overlay",
        purpose="Verify text overlay with semi-transparent background bar",
        expected_outcome="Text should have readable background bar for contrast",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="text_styling"
    ),
]


# Image format test metadata
FORMAT_TESTS = [
    TestMetadata(
        test_id="format_png",
        test_number=29,
        title="Image Format - PNG Output",
        category="image_format",
        purpose="Verify PNG format export with proper encoding",
        expected_outcome="Output should be valid PNG file",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="format_export"
    ),
    TestMetadata(
        test_id="format_jpeg",
        test_number=30,
        title="Image Format - JPEG Output",
        category="image_format",
        purpose="Verify JPEG format export with proper compression",
        expected_outcome="Output should be valid JPEG file",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="format_export"
    ),
    TestMetadata(
        test_id="format_gif",
        test_number=31,
        title="Image Format - GIF Output",
        category="image_format",
        purpose="Verify GIF format export with color palette",
        expected_outcome="Output should be valid GIF file",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="format_export"
    ),
    TestMetadata(
        test_id="format_bmp",
        test_number=32,
        title="Image Format - BMP Output",
        category="image_format",
        purpose="Verify BMP format export",
        expected_outcome="Output should be valid BMP file",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="format_export"
    ),
    TestMetadata(
        test_id="format_tiff",
        test_number=33,
        title="Image Format - TIFF Output",
        category="image_format",
        purpose="Verify TIFF format export",
        expected_outcome="Output should be valid TIFF file",
        input_image="alt_color_bars_320x256.png",
        input_width=320,
        input_height=256,
        output_width=320,
        output_height=256,
        test_type="format_export"
    ),
]


def get_all_test_metadata() -> Dict[str, TestMetadata]:
    """Get all test metadata as dictionary keyed by test_id"""
    all_tests = ASPECT_TESTS + OVERLAY_TESTS + FORMAT_TESTS
    return {test.test_id: test for test in all_tests}


def get_metadata_by_test_id(test_id: str) -> Optional[TestMetadata]:
    """Get metadata for a specific test"""
    all_tests = get_all_test_metadata()
    return all_tests.get(test_id)


def get_metadata_by_category(category: str) -> List[TestMetadata]:
    """Get all tests in a specific category"""
    all_tests = ASPECT_TESTS + OVERLAY_TESTS + FORMAT_TESTS
    return [t for t in all_tests if t.category == category]
