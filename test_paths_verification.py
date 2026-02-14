#!/usr/bin/env python3
"""
Verify that all test utility files have correct path resolution
"""

import sys
import os
from pathlib import Path

# Add tests/util to path
sys.path.insert(0, str(Path(__file__).parent / "tests" / "util"))

test_classes = [
    ('run_all_tests', 'TestRunner'),
    ('test_suite', 'TestSuite'),
    ('test_edge_cases', 'EdgeCaseTestSuite'),
    ('test_error_codes', 'ErrorTestSuite'),
    ('test_security_exploits', 'SecurityExploitTestSuite'),
    ('test_file_io_errors', 'FileIOTestSuite'),
    ('test_performance_stress', 'PerformanceTestSuite'),
    ('test_integration', 'IntegrationTestSuite'),
    ('test_text_overlay_comprehensive', 'TextOverlayComprehensiveTests'),
]

print("Verifying all test suite class path resolution:")
print("=" * 75)

all_passed = True

for module_name, class_name in test_classes:
    try:
        module = __import__(module_name)
        cls = getattr(module, class_name)
        instance = cls()
        
        # Check executable path
        exe_ok = False
        exe_path = None
        if hasattr(instance, 'exe'):
            exe_path = instance.exe
            exe_ok = Path(exe_path).exists()
        
        # Check images dir path
        img_ok = False
        img_path = None
        if hasattr(instance, 'images_dir'):
            img_path = instance.images_dir
            img_ok = Path(img_path).exists()
        
        status = "✓" if exe_ok and img_ok else "✓ (partial)" if exe_ok else "✗"
        print(f"{status} {class_name}")
        
        if exe_path:
            exists_str = "✓ exists" if exe_ok else "✗ NOT FOUND"
            print(f"    Executable: {exists_str}")
            print(f"                {exe_path}")
        
        if img_path:
            exists_str = "✓ exists" if img_ok else "✗ NOT FOUND"
            print(f"    Images dir: {exists_str}")
            print(f"                {img_path}")
        
        if not (exe_ok or (not hasattr(instance, 'exe'))):
            all_passed = False
        
    except Exception as e:
        all_passed = False
        print(f"✗ {class_name}")
        print(f"    Error: {str(e)[:80]}")

print("=" * 75)
if all_passed:
    print("✓ ALL TESTS PASSED - All test utilities have correct path resolution!")
    sys.exit(0)
else:
    print("⚠ Some tests failed - check paths above")
    sys.exit(1)
