#!/usr/bin/env python3
"""
Week 2 Integration Demo: Text Overlay with Report Generator and Verification

Demonstrates:
- Enhanced TextOverlayComprehensiveTests with --report flag
- HTML report generation with embedded images
- Integration with Week 1 infrastructure (TestReportGenerator, HumanVerifier)
- TestMetadata auto-generated verification checklists
"""

import os
import sys
import subprocess
import json
from pathlib import Path
from datetime import datetime

# Add util directory to path
util_dir = Path(__file__).parent
if util_dir not in sys.path:
    sys.path.insert(0, str(util_dir))

from test_report_generator import TestReportGenerator
from test_metadata import TestMetadata


def demo_basic_test_with_report():
    """Demo 1: Basic overlay test generating report"""
    print("="*90)
    print("DEMO 1: Basic Overlay Test with Report Generation")
    print("="*90)
    
    # Create report generator
    generator = TestReportGenerator("Text Overlay Integration Demo")
    
    # Create metadata for a text overlay test
    metadata = TestMetadata(
        test_id="DEMO001",
        test_number=1,
        title="XY Positioning - Stress Test",
        category="text_overlay",
        purpose="Test absolute x,y coordinate positioning",
        expected_outcome="Text overlays render at specified coordinates with correct styling",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=256
    )
    
    # Display auto-generated verification checklist
    print("\n✓ Text Overlay Metadata Created:")
    print(f"  Test ID: {metadata.test_id}")
    print(f"  Title: {metadata.title}")
    print(f"  Category: {metadata.category}")
    print(f"\n✓ Auto-Generated Verification Checklist ({len(metadata.verification_checklist)} items):")
    for i, item in enumerate(metadata.verification_checklist, 1):
        print(f"  {i}. {item}")
    
    # Add test result to generator
    generator.add_test_result(
        test_id="DEMO001",
        name="XY Positioning - Stress Test",
        suite="text_overlay",
        status="passed",
        command="slowframe -i test.png -o out.wav -K -T 'text|size=10|color=white|x=10|y=20'",
        metadata=metadata,
        automated_checks={
            "Executed": True,
            "No crashes": True,
            "Output generated": True,
            "Debug image created": True,
            "All overlays rendered": True
        }
    )
    
    print("\n✓ Test result added to report generator")
    print("✓ HTML report can be generated with: generator.generate_html_report()")


def demo_metadata_checklist_variants():
    """Demo 2: Show auto-generated checklists for different test categories"""
    print("\n" + "="*90)
    print("DEMO 2: Category-Specific Verification Checklists")
    print("="*90)
    
    categories = [
        ("text_overlay", "Text Overlay Feature Test"),
        ("aspect_ratio", "Aspect Ratio Test (center mode)"),
        ("image_format", "Image Format Compatibility Test")
    ]
    
    for category, title in categories:
        if category == "text_overlay":
            metadata = TestMetadata(
                test_id="DEMO_TEXT",
                test_number=1,
                title=title,
                category=category,
                purpose="Test text overlay rendering",
                expected_outcome="Text renders correctly",
                input_image="test.png",
                input_width=320,
                input_height=256,
                output_width=320,
                output_height=256
            )
        elif category == "aspect_ratio":
            metadata = TestMetadata(
                test_id="DEMO_ASPECT",
                test_number=2,
                title=title,
                category=category,
                purpose="Test aspect ratio handling",
                expected_outcome="Correct dimensions preserved",
                input_image="test.png",
                input_width=400,
                input_height=600,
                output_width=320,
                output_height=256,
                mode="center"
            )
        else:  # image_format
            metadata = TestMetadata(
                test_id="DEMO_FORMAT",
                test_number=3,
                title=title,
                category=category,
                purpose="Test image format loading",
                expected_outcome="Image loads and converts correctly",
                input_image="test.jpg",
                input_width=320,
                input_height=256,
                output_width=320,
                output_height=256
            )
        
        print(f"\n{category.upper()} - {title}:")
        print(f"  Verification Checklist ({len(metadata.verification_checklist)} items):")
        for i, item in enumerate(metadata.verification_checklist, 1):
            print(f"    {i}. {item}")


def demo_integration_pattern():
    """Demo 3: Show how text overlay tests use Week 1 infrastructure"""
    print("\n" + "="*90)
    print("DEMO 3: Integration Pattern - Text Overlay with Report Generator")
    print("="*90)
    
    # Create generator
    generator = TestReportGenerator("Text Overlay Suite - Week 2")
    results = []
    
    # Simulated test suite
    test_specs = [
        {
            "test_id": "T001",
            "title": "Named Colors - Red",
            "description": "Test rendering with named color 'red'",
            "status": "passed"
        },
        {
            "test_id": "T002", 
            "title": "Hex Colors - #00FF00",
            "description": "Test rendering with hex color #00FF00 (green)",
            "status": "passed"
        },
        {
            "test_id": "T003",
            "title": "XY Positioning",
            "description": "Test absolute x,y coordinate positioning",
            "status": "passed"
        }
    ]
    
    print("\nSimulating test execution with report integration...")
    
    for spec in test_specs:
        # Create metadata
        metadata = TestMetadata(
            test_id=spec["test_id"],
            test_number=int(spec["test_id"].lstrip("T")),
            title=spec["title"],
            category="text_overlay",
            purpose=spec["description"],
            expected_outcome="Text overlays render correctly",
            input_image="test.png",
            input_width=320,
            input_height=256,
            output_width=320,
            output_height=256
        )
        
        # Add to report
        generator.add_test_result(
            test_id=spec["test_id"],
            name=spec["title"],
            suite="text_overlay",
            status=spec["status"],
            command=f"slowframe -T (text overlay test)",
            metadata=metadata,
            automated_checks={
                "Executed": True,
                "No crashes": spec["status"] == "passed",
                "Output generated": spec["status"] == "passed"
            }
        )
        
        results.append({
            "test_id": spec["test_id"],
            "title": spec["title"],
            "status": spec["status"]
        })
        print(f"  ✓ {spec['test_id']}: {spec['title']}")
    
    # Generate JSON report
    json_path = Path(__file__).parent / "demo_overlay_integration_report.json"
    try:
        generator.generate_json_report(str(json_path))
        print(f"\n✓ JSON report generated: {json_path}")
        
        # Show summary of what would be in HTML report
        print("\nReport Summary (would be in HTML):")
        print(f"  Total tests: {len(results)}")
        print(f"  Passed: {sum(1 for r in results if r['status'] == 'passed')}")
        print(f"  Failed: {sum(1 for r in results if r['status'] == 'failed')}")
        print(f"\nEach test includes:")
        print(f"  • Test metadata with auto-generated verification checklist")
        print(f"  • Command executed")
        print(f"  • Automated checks results")
        print(f"  • Input/output images (embedded as base64 in HTML)")
        
    except Exception as e:
        print(f"\nWarning during report generation: {str(e)}")


def main():
    """Run all integration demos"""
    print("\n")
    print("█" * 90)
    print("█ WEEK 2: TEXT OVERLAY TEST INTEGRATION DEMO")
    print("█ Demonstrates Week 1 Infrastructure Applied to Text Overlay Tests")
    print("█" * 90)
    
    try:
        demo_basic_test_with_report()
        demo_metadata_checklist_variants()
        demo_integration_pattern()
        
        print("\n" + "="*90)
        print("ALL DEMOS COMPLETE")
        print("="*90)
        print("\nKey Takeaways:")
        print("  1. TextReportGenerator integrates seamlessly with overlay tests")
        print("  2. TestMetadata auto-generates text_overlay verification items:")
        print("     • Text is rendered and visible")
        print("     • Text color matches specification")
        print("     • Text placement is correct")
        print("     • Text is readable and clear")
        print("     • No clipping or artifacts around text")
        print("     • Background/styling applied correctly")
        print("\n  3. HTML reports include:")
        print("     • Embedded debug images")
        print("     • Test specifications and metadata")
        print("     • Automated check results")
        print("     • Human verification checklist")
        print("\n  4. Command-line flags added to test suite:")
        print("     • --report: Generate HTML report with embedded images")
        print("     • --verify: Enable interactive human verification workflow")
        print("\nTo run enhanced overlay tests:")
        print("  python3 tests/util/test_text_overlay_comprehensive.py --report --verify")
        print("\n" + "="*90 + "\n")
        
    except Exception as e:
        print(f"\nERROR in demo: {str(e)}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
