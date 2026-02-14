#!/usr/bin/env python3
"""Display test results summary"""

import json
import sys
from pathlib import Path

# Load the test results
# Load the most recent test results
results_dir = Path(__file__).parent.parent / "test_outputs"
results_files = list(results_dir.glob('test_results_*.json'))
if not results_files:
    print("No test results found in", results_dir)
    exit(1)
results_file = max(results_files, key=lambda p: p.stat().st_mtime)
with open(results_file) as f:
    results = json.load(f)

print("=" * 70)
print("TEXT OVERLAY TEST RESULTS SUMMARY")
print("=" * 70)
print(f"\nTotal Tests: {results['summary']['total']}")
print(f"✓ Passed:   {results['summary']['passed']}")
print(f"✗ Failed:   {results['summary']['failed']}")
print(f"⊘ Skipped:  {results['summary']['skipped']}")
print(f"Timestamp:  {results['summary']['timestamp']}")

print("\n" + "=" * 70)
print("OVERLAY-SPECIFIC TEST RESULTS")
print("=" * 70)

overlay_results = [t for t in results['results'] if 'overlay' in t['test'].lower()]

for i, test in enumerate(overlay_results, 1):
    status_icon = "✓" if test['status'] == 'PASSED' else "⊘" if test['status'] == 'SKIPPED' else "✗"
    print(f"\n{i}. {status_icon} {test['test']}")
    print(f"   Status: {test['status']}")
    if test['details']:
        # Truncate long details
        details = test['details'][:100] + "..." if len(test['details']) > 100 else test['details']
        print(f"   Details: {details}")

print("\n" + "=" * 70)
print("CORE FUNCTIONALITY TEST CATEGORIES")
print("=" * 70)

categories = {}
for test in results['results']:
    # Extract category from test name
    if '(M1)' in test['test'] or '(M2)' in test['test'] or '(S1)' in test['test']:
        cat = 'Protocol Tests'
    elif 'Sample rate' in test['test']:
        cat = 'Sample Rate Tests'
    elif 'format' in test['test'].lower():
        cat = 'Audio Format Tests'
    elif 'Aspect' in test['test']:
        cat = 'Aspect Ratio Tests'
    elif 'CW' in test['test']:
        cat = 'CW Signature Tests'
    elif 'overlay' in test['test'].lower():
        cat = 'Text Overlay Tests'
    else:
        cat = 'Error Handling & Validation'
    
    if cat not in categories:
        categories[cat] = {'passed': 0, 'failed': 0, 'skipped': 0}
    
    if test['status'] == 'PASSED':
        categories[cat]['passed'] += 1
    elif test['status'] == 'FAILED':
        categories[cat]['failed'] += 1
    else:
        categories[cat]['skipped'] += 1

for cat in sorted(categories.keys()):
    stats = categories[cat]
    total = stats['passed'] + stats['failed'] + stats['skipped']
    output = f"\n{cat}:"
    output += f"\n  ✓ {stats['passed']} passed"
    if stats['failed'] > 0:
        output += f", ✗ {stats['failed']} failed"
    if stats['skipped'] > 0:
        output += f", ⊘ {stats['skipped']} skipped"
    output += f" (total: {total})"
    print(output)

print("\n" + "=" * 70)
print(f"DETAILED RESULTS FILE: test_outputs/test_results_20260211_173449.json")
print(f"ARTIFACTS DIRECTORY: test_outputs/text_overlay/ (16 test directories)")
print("=" * 70 + "\n")
