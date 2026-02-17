#!/usr/bin/env python3
"""
Generate sample reports from Format and Aspect validators.

This script demonstrates the report generation capabilities of the
Week 3 enhanced validators (Format Validator, Aspect Validator).
"""

import sys
import json
from pathlib import Path
from datetime import datetime

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

from test_report_generator import TestReportGenerator
from test_metadata import TestMetadata
from human_verification_framework import HumanVerifier


def generate_format_validator_sample_report():
    """Generate sample HTML/JSON reports for Format Validator"""
    print("\n" + "="*90)
    print("FORMAT VALIDATOR - Sample Report Generation")
    print("="*90)
    
    # Create report generator
    report_gen = TestReportGenerator("Format Validation Test Suite")
    
    # Sample test data for different formats
    formats = [
        {
            "test_id": "FMT01",
            "format": "PNG",
            "width": 320,
            "height": 256,
            "status": "passed",
            "command": "slowframe -i test.jpg -f png -o output.png",
        },
        {
            "test_id": "FMT02",
            "format": "JPEG",
            "width": 640,
            "height": 480,
            "status": "passed",
            "command": "slowframe -i test.png -f jpeg -o output.jpg",
        },
        {
            "test_id": "FMT03",
            "format": "GIF",
            "width": 320,
            "height": 240,
            "status": "passed",
            "command": "slowframe -i test.png -f gif -o output.gif",
        },
        {
            "test_id": "FMT04",
            "format": "BMP",
            "width": 1024,
            "height": 768,
            "status": "passed",
            "command": "slowframe -i test.jpg -f bmp -o output.bmp",
        },
    ]
    
    for fmt in formats:
        # Create metadata
        metadata = TestMetadata(
            test_id=fmt["test_id"],
            test_number=int(fmt["test_id"][3:]),
            title=f"Format Support - {fmt['format']}",
            category="image_format",
            purpose=f"Validate {fmt['format']} format support",
            expected_outcome=f"Successfully load and process {fmt['format']} image",
            input_image=f"test.{'jpg' if fmt['format'] != 'JPEG' else 'png'}",
            input_width=2000,
            input_height=1500,
            output_width=fmt['width'],
            output_height=fmt['height'],
            test_type=fmt['format']
        )
        
        # Add test result
        report_gen.add_test_result(
            test_id=fmt["test_id"],
            name=f"Format Test - {fmt['format']}",
            suite="format_validator",
            status=fmt["status"],
            command=fmt["command"],
            metadata=metadata,
            output_files={},
            automated_checks={
                "Format loaded successfully": True,
                "Color accuracy preserved": True,
                "No visual artifacts": True,
                f"Dimensions correct ({fmt['width']}x{fmt['height']})": True,
            }
        )
        print(f"  ✓ {fmt['test_id']}: {fmt['format']} format ({fmt['width']}x{fmt['height']})")
    
    # Generate HTML report
    html_path = Path.cwd() / f"sample_format_validation_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.html"
    report_gen.generate_html_report(str(html_path))
    print(f"\n✓ HTML Report: {html_path}")
    print(f"  Size: {html_path.stat().st_size / 1024:.1f} KB")
    
    # Generate JSON report
    json_path = Path.cwd() / f"sample_format_validation_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    report_gen.generate_json_report(str(json_path))
    print(f"✓ JSON Report: {json_path}")
    print(f"  Size: {json_path.stat().st_size / 1024:.1f} KB")
    
    return str(html_path), str(json_path)


def generate_aspect_validator_sample_report():
    """Generate sample HTML/JSON reports for Aspect Validator"""
    print("\n" + "="*90)
    print("ASPECT RATIO VALIDATOR - Sample Report Generation")
    print("="*90)
    
    # Create report generator
    report_gen = TestReportGenerator("Aspect Ratio Validation Test Suite")
    
    # Sample test data for different aspect modes
    aspects = [
        {
            "test_id": "AR01",
            "mode": "CENTER",
            "src_width": 2000,
            "src_height": 1600,
            "target_width": 320,
            "target_height": 256,
            "status": "passed",
            "command": "slowframe -i test.jpg -C -o output.wav",
        },
        {
            "test_id": "AR02",
            "mode": "PAD",
            "src_width": 1024,
            "src_height": 768,
            "target_width": 320,
            "target_height": 256,
            "status": "passed",
            "command": "slowframe -i test.jpg -P -o output.wav",
        },
        {
            "test_id": "AR03",
            "mode": "STRETCH",
            "src_width": 800,
            "src_height": 600,
            "target_width": 320,
            "target_height": 256,
            "status": "passed",
            "command": "slowframe -i test.jpg -S -o output.wav",
        },
    ]
    
    mode_descriptions = {
        "CENTER": "Crops to match target aspect ratio without padding",
        "PAD": "Preserves aspect ratio and adds black padding",
        "STRETCH": "Directly resizes without maintaining aspect ratio"
    }
    
    for aspect in aspects:
        # Create metadata
        metadata = TestMetadata(
            test_id=aspect["test_id"],
            test_number=int(aspect["test_id"][2:]),
            title=f"Aspect Ratio - {aspect['mode']} Mode ({aspect['src_width']}x{aspect['src_height']} → {aspect['target_width']}x{aspect['target_height']})",
            category="aspect_ratio",
            purpose=mode_descriptions[aspect["mode"]],
            expected_outcome=f"Output dimensions {aspect['target_width']}x{aspect['target_height']} with correct {aspect['mode']} mode behavior",
            input_image="input.jpg",
            input_width=aspect['src_width'],
            input_height=aspect['src_height'],
            output_width=aspect['target_width'],
            output_height=aspect['target_height'],
            mode=aspect['mode'],
        )
        
        # Add test result
        report_gen.add_test_result(
            test_id=aspect["test_id"],
            name=f"Aspect Ratio - {aspect['mode']} Mode",
            suite="aspect_validator",
            status=aspect["status"],
            command=aspect["command"],
            metadata=metadata,
            output_files={},
            automated_checks={
                "Output dimensions correct": True,
                "Aspect mode applied properly": True,
                "No severe visual artifacts": True,
                "Color quality preserved": True,
                "Black padding quality (if applicable)": True,
            }
        )
        print(f"  ✓ {aspect['test_id']}: {aspect['mode']} Mode ({aspect['src_width']}x{aspect['src_height']} → {aspect['target_width']}x{aspect['target_height']})")
    
    # Generate HTML report
    html_path = Path.cwd() / f"sample_aspect_validation_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.html"
    report_gen.generate_html_report(str(html_path))
    print(f"\n✓ HTML Report: {html_path}")
    print(f"  Size: {html_path.stat().st_size / 1024:.1f} KB")
    
    # Generate JSON report
    json_path = Path.cwd() / f"sample_aspect_validation_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    report_gen.generate_json_report(str(json_path))
    print(f"✓ JSON Report: {json_path}")
    print(f"  Size: {json_path.stat().st_size / 1024:.1f} KB")
    
    return str(html_path), str(json_path)


def generate_unified_sample_report():
    """Generate unified sample report combining both validators"""
    print("\n" + "="*90)
    print("UNIFIED VALIDATION - Sample Report Generation")
    print("="*90)
    
    # Create report generator
    report_gen = TestReportGenerator("SlowFrame Unified Validation Test Suite")
    
    # Add Format Validator tests
    formats = ["PNG", "JPEG", "GIF", "BMP"]
    for i, fmt in enumerate(formats, 1):
        test_id = f"FMT{i:02d}"
        metadata = TestMetadata(
            test_id=test_id,
            test_number=i,
            title=f"Format Support - {fmt}",
            category="image_format",
            purpose=f"Validate {fmt} format support",
            expected_outcome=f"Successfully process {fmt} image",
            input_image="test.jpg",
            input_width=2000,
            input_height=1500,
            output_width=320,
            output_height=256,
            test_type=fmt
        )
        
        report_gen.add_test_result(
            test_id=test_id,
            name=f"Format - {fmt}",
            suite="format_validator",
            status="passed",
            command=f"slowframe -i test.jpg -f {fmt.lower()} -o output.{fmt.lower() if fmt != 'JPEG' else 'jpg'}",
            metadata=metadata,
            output_files={},
            automated_checks={
                "Format loaded successfully": True,
                "Color accuracy preserved": True,
                "No visual artifacts": True,
                "Dimensions correct": True,
            }
        )
    
    # Add Aspect Validator tests
    aspects = [
        ("CENTER", "C", 2000, 1600),
        ("PAD", "P", 1024, 768),
        ("STRETCH", "S", 800, 600),
    ]
    
    for i, (mode, flag, width, height) in enumerate(aspects, 1):
        test_id = f"ASP{i:02d}"
        metadata = TestMetadata(
            test_id=test_id,
            test_number=i,
            title=f"Aspect Ratio - {mode} Mode ({width}x{height} → 320x256)",
            category="aspect_ratio",
            purpose=f"{mode} mode aspect ratio handling",
            expected_outcome=f"Output dimensions 320x256 with correct {mode} mode behavior",
            input_image="input.jpg",
            input_width=width,
            input_height=height,
            output_width=320,
            output_height=256,
            mode=mode,
        )
        
        report_gen.add_test_result(
            test_id=test_id,
            name=f"Aspect - {mode}",
            suite="aspect_validator",
            status="passed",
            command=f"slowframe -i test.jpg -{flag} -o output.wav",
            metadata=metadata,
            output_files={},
            automated_checks={
                "Output dimensions correct": True,
                "Aspect mode applied": True,
                "No severe artifacts": True,
                "Color quality preserved": True,
                "Padding quality (if applicable)": True,
            }
        )
    
    # Generate HTML report
    html_path = Path.cwd() / f"sample_unified_validation_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.html"
    report_gen.generate_html_report(str(html_path))
    print(f"✓ HTML Report: {html_path}")
    print(f"  Total tests: {len(formats) + len(aspects)}")
    print(f"  Size: {html_path.stat().st_size / 1024:.1f} KB")
    
    # Generate JSON report
    json_path = Path.cwd() / f"sample_unified_validation_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    report_gen.generate_json_report(str(json_path))
    print(f"✓ JSON Report: {json_path}")
    print(f"  Size: {json_path.stat().st_size / 1024:.1f} KB")
    
    return str(html_path), str(json_path)


def display_sample_json(json_file):
    """Display a sample of the JSON report structure"""
    print("\n" + "="*90)
    print("SAMPLE JSON REPORT STRUCTURE")
    print("="*90 + "\n")
    
    try:
        with open(json_file, 'r') as f:
            data = json.load(f)
        
        # Show structure overview
        print("JSON Report Structure:")
        print(f"  • Title: {data.get('title')}")
        print(f"  • Total Tests: {data.get('total_tests')}")
        print(f"  • Passed: {data.get('passed_count')}")
        print(f"  • Failed: {data.get('failed_count')}")
        print(f"  • Generated: {data.get('generated_at')}")
        
        # Show first test as example
        if data.get('test_results'):
            print("\n  First Test Example:")
            first_test = data['test_results'][0]
            print(f"    - ID: {first_test.get('test_id')}")
            print(f"    - Name: {first_test.get('name')}")
            print(f"    - Suite: {first_test.get('suite')}")
            print(f"    - Status: {first_test.get('status')}")
            
            if first_test.get('metadata'):
                print(f"    - Title: {first_test['metadata'].get('title')}")
                print(f"    - Category: {first_test['metadata'].get('category')}")
                
                if first_test['metadata'].get('verification_checklist'):
                    print(f"    - Verification Items: {len(first_test['metadata']['verification_checklist'])}")
    except Exception as e:
        print(f"  Error reading JSON: {e}")


def main():
    """Generate and display sample reports"""
    print("\n" + "█"*90)
    print("█ SAMPLE REPORT GENERATION - Week 3 Validators")
    print("█ Demonstrating TestReportGenerator capabilities")
    print("█"*90)
    
    try:
        # Generate Format Validator reports
        fmt_html, fmt_json = generate_format_validator_sample_report()
        
        # Generate Aspect Validator reports
        asp_html, asp_json = generate_aspect_validator_sample_report()
        
        # Generate Unified reports
        uni_html, uni_json = generate_unified_sample_report()
        
        # Display sample JSON structure
        display_sample_json(uni_json)
        
        # Summary
        print("\n" + "="*90)
        print("SUMMARY - Reports Generated Successfully")
        print("="*90)
        print(f"\n✓ Format Validator Reports:")
        print(f"   HTML: {Path(fmt_html).name}")
        print(f"   JSON: {Path(fmt_json).name}")
        print(f"\n✓ Aspect Ratio Validator Reports:")
        print(f"   HTML: {Path(asp_html).name}")
        print(f"   JSON: {Path(asp_json).name}")
        print(f"\n✓ Unified Validation Reports:")
        print(f"   HTML: {Path(uni_html).name}")
        print(f"   JSON: {Path(uni_json).name}")
        
        print(f"\n📊 Reports include:")
        print(f"   • Professional HTML formatting with responsive design")
        print(f"   • Structured JSON for automated analysis")
        print(f"   • Test metadata and execution details")
        print(f"   • Automated checks and verification tracking")
        print(f"   • Category-based checklist generation")
        
        print(f"\n🎯 Next Steps:")
        print(f"   1. Open HTML reports in browser to view formatted results")
        print(f"   2. Parse JSON reports for automated CI/CD integration")
        print(f"   3. Use reports with HumanVerifier for interactive validation")
        print(f"\n" + "="*90)
        
        return 0
        
    except Exception as e:
        print(f"\n✗ Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
