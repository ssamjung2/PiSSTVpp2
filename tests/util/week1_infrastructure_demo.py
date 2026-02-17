#!/usr/bin/env python3
"""
Week 1 Infrastructure Demo - Test Report Generator + Human Verification

Demonstrates the new testing infrastructure:
1. TestReportGenerator - HTML/JSON reports with embedded images
2. HumanVerifier - Interactive verification workflow
3. Updated TestMetadata - Automatic verification checklists

This shows how future test suites should integrate these tools.

Usage:
    python3 tests/util/week1_infrastructure_demo.py
"""

import sys
import os
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

from test_report_generator import TestReportGenerator, TestResult
from human_verification_framework import HumanVerifier
from test_metadata import TestMetadata


def create_sample_test_metadata():
    """Create sample test metadata to demonstrate functionality"""
    return TestMetadata(
        test_id="demo_pad_test",
        test_number=1,
        title="Demo: PAD Mode with Wide Image",
        category="aspect_ratio",
        purpose="Demonstrate new reporting infrastructure",
        expected_outcome="Black padding bars on top/bottom, image centered",
        input_image="alt2_color_bars_2000x1125.png",
        input_width=2000,
        input_height=1125,
        output_width=320,
        output_height=256,
        mode="pad",
        protocol="m1",
        test_type="aspect_preservation",
        # Checklist will be auto-generated from category and mode
    )


def demo_report_generator():
    """Demonstrate TestReportGenerator usage"""
    print("\n" + "="*70)
    print("DEMO 1: Test Report Generator")
    print("="*70)
    
    # Create generator
    generator = TestReportGenerator("Week 1 Demo - Report Generator")
    
    # Create sample metadata
    metadata = create_sample_test_metadata()
    
    print(f"\n✓ Created test metadata: {metadata.test_id}")
    print(f"  Auto-generated checklist with {len(metadata.verification_checklist)} items:")
    for i, item in enumerate(metadata.verification_checklist, 1):
        print(f"    {i}. {item}")
    
    # Add test result
    result = generator.add_test_result(
        test_id="T001",
        name="demo_pad_test",
        suite="aspect_ratio",
        status="passed",
        command="./bin/slowframe -i tests/images/alt2_color_bars_2000x1125.png -p m1 -a pad -o output.wav",
        metadata=metadata,
        output_files={
            "Input Image": "tests/images/alt2_color_bars_2000x1125.png",
            "Output Image": "tests/test_outputs/aspect/T009_pad_m1_wide.png"
        },
        automated_checks={
            "Dimensions correct (320x256)": True,
            "Output file exists": True,
            "No errors during encoding": True,
            "Padding color is black": True
        },
        execution_time=0.15
    )
    
    print(f"\n✓ Added test result: {result.test_id}")
    print(f"  Status: {result.status}")
    print(f"  Automated checks: {len(result.automated_checks)} passed")
    
    # Generate reports
    html_file = "tests/demo_report.html"
    json_file = "tests/demo_report.json"
    
    generator.generate_html_report(html_file)
    generator.generate_json_report(json_file)
    
    print(f"\n✓ Generated reports:")
    print(f"  HTML: {html_file}")
    print(f"  JSON: {json_file}")
    
    return generator


def demo_human_verification():
    """Demonstrate HumanVerifier usage"""
    print("\n" + "="*70)
    print("DEMO 2: Human Verification Framework")
    print("="*70)
    
    # Create verifier
    verifier = HumanVerifier(
        session_file="tests/demo_verification_session.json",
        auto_preview=False,  # Don't auto-open for demo
        verifier_name="demo_user"
    )
    
    print(f"\n✓ Created verifier (session: {verifier.session_file})")
    print(f"  Verifier: {verifier.verifier_name}")
    
    # Create sample metadata
    metadata = create_sample_test_metadata()
    
    print("\n" + "-"*70)
    print("EXAMPLE: What interactive verification looks like")
    print("-"*70)
    print(f"\n🔍 VERIFICATION: T001 - {metadata.title}")
    print(f"\n📝 Description: {metadata.purpose}")
    print(f"✓  Expected: {metadata.expected_outcome}")
    print(f"\n💻 Command: ./bin/slowframe -i input.png -a pad -o output.wav")
    print(f"\n📋 Verification Checklist:")
    for i, item in enumerate(metadata.verification_checklist, 1):
        print(f"   {i}. □ {item}")
    
    print("\n" + "-"*70)
    print("In actual use, you would:")
    print("  1. Images would auto-open for review")
    print("  2. You'd be prompted: [a=approve, r=reject, s=skip, d=defer, q=quit]")
    print("  3. Your verdict would be saved to session file")
    print("  4. Session persists - you can resume interrupted verifications")
    print("-"*70)
    
    # For demo, create a mock verification result
    from human_verification_framework import VerificationResult
    from datetime import datetime
    
    mock_result = VerificationResult(
        test_id="T001",
        verdict="approved",
        verifier_name="demo_user",
        timestamp=datetime.now().isoformat(),
        notes="Demo: Padding looks correct, image centered properly",
        checklist_results={
            "Dimensions correct": True,
            "Padding is black": True,
            "Image centered": True
        }
    )
    
    verifier.results["T001"] = mock_result
    verifier.save_session()
    
    print(f"\n✓ Created mock verification result")
    print(f"  Verdict: {mock_result.verdict}")
    print(f"  Notes: {mock_result.notes}")
    
    verifier.print_summary()
    
    return verifier


def demo_integration():
    """Demonstrate integration of both systems"""
    print("\n" + "="*70)
    print("DEMO 3: Integrated Workflow (Report + Verification)")
    print("="*70)
    
    # Step 1: Create report with test results
    generator = TestReportGenerator("Integrated Demo Report")
    metadata = create_sample_test_metadata()
    
    result = generator.add_test_result(
        test_id="T001",
        name="integrated_demo",
        suite="aspect_ratio",
        status="passed",
        command="./bin/slowframe -i input.png -a pad -o output.wav",
        metadata=metadata,
        output_files={
            "Output": "tests/test_outputs/aspect/T009_pad_m1_wide.png"
        },
        automated_checks={
            "Dimensions": True,
            "No errors": True
        }
    )
    
    print("\n✓ Step 1: Created test report")
    
    # Step 2: Human verification
    verifier = HumanVerifier(
        session_file="tests/demo_integrated_session.json",
        auto_preview=False
    )
    
    # In real usage, this would be interactive:
    # verification = verifier.verify_test(
    #     test_id="T001",
    #     test_name=metadata.title,
    #     metadata=metadata,
    #     images={"Output": "tests/test_outputs/aspect/T009_pad_m1_wide.png"},
    #     checklist=metadata.verification_checklist
    # )
    
    # For demo, create mock verification
    from human_verification_framework import VerificationResult
    from datetime import datetime
    
    verification = VerificationResult(
        test_id="T001",
        verdict="approved",
        verifier_name="demo_user",
        timestamp=datetime.now().isoformat(),
        notes="Verified: All checklist items passed"
    )
    
    verifier.results["T001"] = verification
    
    print("✓ Step 2: Human verified test")
    
    # Step 3: Update report with verification
    result.human_verified = True
    result.human_verdict = verification.verdict
    result.human_notes = verification.notes
    result.verifier_name = verification.verifier_name
    result.verification_date = verification.timestamp
    
    print("✓ Step 3: Updated report with verification results")
    
    # Step 4: Generate final reports
    generator.generate_html_report("tests/demo_integrated_report.html")
    generator.generate_json_report("tests/demo_integrated_report.json")
    
    print("✓ Step 4: Generated final reports with verification data")
    
    print("\n" + "="*70)
    print("WORKFLOW SUMMARY:")
    print("="*70)
    print("1. Test runs → automated checks")
    print("2. TestReportGenerator creates report with embedded images")
    print("3. HumanVerifier provides interactive verification workflow")
    print("4. Verification results merged back into report")
    print("5. Final report shows both automated + human verification")
    print("="*70)


def main():
    """Run all demos"""
    print("\n" + "="*70)
    print("Week 1 Infrastructure Demo")
    print("Testing Framework Enhancement - SlowFrame")
    print("="*70)
    
    # Demo 1: Report Generator
    demo_report_generator()
    
    # Demo 2: Human Verification
    demo_human_verification()
    
    # Demo 3: Integration
    demo_integration()
    
    print("\n" + "="*70)
    print("✅ DEMO COMPLETE")
    print("="*70)
    print("\nFiles Generated:")
    print("  • tests/demo_report.html - HTML report example")
    print("  • tests/demo_report.json - JSON report example")
    print("  • tests/demo_integrated_report.html - Integrated report")
    print("  • tests/demo_verification_session.json - Verification session")
    print("\nNext Steps:")
    print("  • Review generated HTML reports in browser")
    print("  • Examine JSON structure for integration")
    print("  • Week 2: Integrate into text_overlay tests (P0 priority)")
    print("="*70)


if __name__ == "__main__":
    main()
