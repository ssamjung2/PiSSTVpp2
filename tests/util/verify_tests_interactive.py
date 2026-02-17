#!/usr/bin/env python3
"""
Interactive Terminal-Based Test Verification

Allows human verification of test results one-by-one with:
- Test case description and intent
- Expected behavior
- Prompt to view output image
- Pass/Fail verdict entry
- Notes capture
- Results saved to JSON

Usage:
    venv/bin/python3 tests/verify_tests_interactive.py
    venv/bin/python3 tests/verify_tests_interactive.py --suite aspect
"""

import sys
import json
import subprocess
from pathlib import Path
from datetime import datetime
from typing import List, Dict

# Import test intent data
sys.path.insert(0, str(Path(__file__).parent))

# Test definitions (same as HTML report)
from generate_verification_report import TEST_INTENT


class InteractiveVerifier:
    """Terminal-based interactive test verification"""
    
    def __init__(self, results_file: Path = Path("verification_results.json")):
        self.results_file = results_file
        self.results = self.load_existing_results()
        self.session_start = datetime.now()
        
    def load_existing_results(self) -> dict:
        """Load existing verification results if available"""
        if self.results_file.exists():
            with open(self.results_file) as f:
                return json.load(f)
        return {}
    
    def save_results(self):
        """Save verification results to JSON"""
        with open(self.results_file, 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"\n💾 Results saved to: {self.results_file}")
    
    def display_test_info(self, test_name: str, test_data: dict):
        """Display test information"""
        intent = TEST_INTENT.get(test_name, {})
        
        print("\n" + "="*70)
        print(f"🧪 TEST: {test_name.replace('_', ' ').upper()}")
        print("="*70)
        
        print(f"\n🎯 Intent:")
        print(f"   {intent.get('intent', 'N/A')}")
        
        print(f"\n📋 Parameters:")
        print(f"   Input:  {intent.get('input', test_data.get('input', 'N/A'))}")
        print(f"   Target: {intent.get('target', test_data.get('target', 'N/A'))}")
        if 'params' in intent:
            print(f"   Params: {intent['params']}")
        
        print(f"\n✅ Expected Behavior:")
        print(f"   {intent.get('expected', 'N/A')}")
        
        if intent.get('verify'):
            print(f"\n🔍 Verification Checklist:")
            for i, item in enumerate(intent['verify'], 1):
                print(f"   {i}. {item}")
    
    def verify_test(self, test_name: str, test_data: dict) -> dict:
        """Verify a single test interactively"""
        
        self.display_test_info(test_name, test_data)
        
        # Check if already verified
        if test_name in self.results:
            print(f"\n⚠️  Previous verification found:")
            print(f"   Verdict: {self.results[test_name].get('verdict', 'unknown').upper()}")
            print(f"   Notes: {self.results[test_name].get('notes', 'None')}")
            
            response = input("\n   Re-verify this test? [y/N]: ").strip().lower()
            if response != 'y':
                print("   Skipping...")
                return self.results[test_name]
        
        # Prompt to view output
        output_img = test_data.get('output_image')
        if output_img and Path(output_img).exists():
            print(f"\n📷 Output image: {output_img}")
            view = input("   View image? [Y/n]: ").strip().lower()
            if view != 'n':
                # Try to open image
                try:
                    subprocess.run(['open', output_img], check=False)
                except:
                    try:
                        subprocess.run(['xdg-open', output_img], check=False)
                    except:
                        print(f"   Could not auto-open. Please view manually: {output_img}")
        
        # Get verdict
        print("\n" + "-"*70)
        while True:
            verdict = input("🔍 Verdict [PASS/FAIL/SKIP]: ").strip().upper()
            if verdict in ['PASS', 'FAIL', 'SKIP']:
                break
            print("   Invalid input. Please enter PASS, FAIL, or SKIP")
        
        if verdict == 'SKIP':
            return {'verdict': 'skipped', 'timestamp': datetime.now().isoformat()}
        
        # Get notes
        print("\n📝 Notes (press Enter twice to finish):")
        notes_lines = []
        empty_count = 0
        while True:
            line = input("   ")
            if not line:
                empty_count += 1
                if empty_count >= 2:
                    break
            else:
                empty_count = 0
                notes_lines.append(line)
        
        notes = "\n".join(notes_lines)
        
        result = {
            'verdict': verdict.lower(),
            'notes': notes,
            'timestamp': datetime.now().isoformat(),
            'test_data': test_data
        }
        
        self.results[test_name] = result
        self.save_results()
        
        return result
    
    def run_verification(self, test_list: List[tuple]):
        """Run verification for list of tests"""
        total = len(test_list)
        
        print("\n" + "="*70)
        print(f"🔍 INTERACTIVE TEST VERIFICATION")
        print(f"   Total tests: {total}")
        print(f"   Results: {self.results_file}")
        print("="*70)
        
        for idx, (test_name, test_data) in enumerate(test_list, 1):
            print(f"\n[{idx}/{total}]")
            
            try:
                result = self.verify_test(test_name, test_data)
                
                if result['verdict'] == 'pass':
                    print("\n✅ Marked as PASS")
                elif result['verdict'] == 'fail':
                    print("\n❌ Marked as FAIL")
                else:
                    print("\n⏭️  Skipped")
                
                # Ask to continue
                if idx < total:
                    cont = input("\nContinue to next test? [Y/n]: ").strip().lower()
                    if cont == 'n':
                        print("\n⏸️  Verification paused. Run again to continue.")
                        break
                        
            except KeyboardInterrupt:
                print("\n\n⏸️  Verification interrupted. Results saved.")
                break
        
        # Summary
        self.print_summary()
    
    def print_summary(self):
        """Print verification summary"""
        print("\n" + "="*70)
        print("📊 VERIFICATION SUMMARY")
        print("="*70)
        
        passed = sum(1 for r in self.results.values() if r.get('verdict') == 'pass')
        failed = sum(1 for r in self.results.values() if r.get('verdict') == 'fail')
        skipped = sum(1 for r in self.results.values() if r.get('verdict') == 'skipped')
        
        print(f"\n  ✅ Passed:  {passed}")
        print(f"  ❌ Failed:  {failed}")
        print(f"  ⏭️  Skipped: {skipped}")
        print(f"  📝 Total:   {len(self.results)}")
        
        if failed > 0:
            print(f"\n⚠️  Failed Tests:")
            for test_name, result in self.results.items():
                if result.get('verdict') == 'fail':
                    print(f"   - {test_name}")
                    if result.get('notes'):
                        print(f"     Notes: {result['notes'][:100]}...")
        
        print(f"\n💾 Results saved to: {self.results_file}")
        print("="*70)


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Interactive test verification")
    parser.add_argument('--suite', '-s', choices=['aspect', 'overlay', 'formats', 'all'],
                       default='all', help='Test suite to verify')
    parser.add_argument('--output', '-o', default='verification_results.json',
                       help='Output JSON file for results')
    
    args = parser.parse_args()
    
    verifier = InteractiveVerifier(Path(args.output))
    
    # Build test list
    test_outputs = Path("test_outputs")
    test_images = Path("images")
    test_list = []
    
    # Aspect ratio tests
    if args.suite in ['aspect', 'all']:
        aspect_tests = [
            'center_m1_wide', 'center_m1_tall', 'center_m1_square', 'center_m1_target',
            'pad_m1_wide', 'pad_m1_tall', 'pad_m1_square', 'pad_m1_target',
            'stretch_m1_wide', 'stretch_m1_tall', 'stretch_m1_square', 'stretch_m1_target',
        ]
        for test in aspect_tests:
            mode, protocol, aspect = test.split('_', 2)
            
            input_map = {
                'wide': 'alt2_color_bars_2000x1125.png',
                'tall': 'alt2_color_bars_680×1209.png',
                'square': 'alt3_color_bars_1370×1080.png',
                'target': 'alt2_color_bars_320x256.png'
            }
            
            input_img = test_images / input_map.get(aspect, 'unknown.png')
            output_img = test_outputs / f"{test}.png"
            
            test_list.append((test, {
                'input': input_img.name,
                'target': 'M1 320x256' if protocol == 'm1' else 'R36 320x240',
                'input_image': str(input_img),
                'output_image': str(output_img)
            }))
    
    # Run verification
    verifier.run_verification(test_list)


if __name__ == '__main__':
    main()
