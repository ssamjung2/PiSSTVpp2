#!/usr/bin/env python3
"""Show text overlay test results"""

import json
from pathlib import Path

# Load the test results
results_file = list((Path(__file__).parent.parent / "test_outputs").glob('test_results_*.json'))
if not results_file:
    print("No test results found")
    exit(1)

latest = max(results_file, key=lambda p: p.stat().st_mtime)

with open(latest) as f:
    results = json.load(f)

print(f'Test Summary from {latest.name}:')
print(f'  Total: {results["summary"]["total"]}')
print(f'  Passed: {results["summary"]["passed"]}')
print(f'  Failed: {results["summary"]["failed"]}')
print(f'  Skipped: {results["summary"]["skipped"]}')
print()

# Find text overlay test results
print('Text Overlay Tests:')
for result in results['results']:
    if 'overlay' in result.get('name', '').lower() or 'text' in result.get('name', '').lower():
        status = result.get('status', '?')
        name = result.get('name', 'Unknown')
        notes = result.get('notes', '')
        print(f'  [{status}] {name}')
        if notes:
            print(f'       {notes[:100] if len(notes) > 100 else notes}')
