#!/usr/bin/env python3
"""
Mode Registry Test Suite
========================

Comprehensive test coverage for the SSTV mode registry system.

Test Categories:
1. Registry Lifecycle (create/free)
2. Mode Registration (add modes, duplicates, capacity)
3. Lookup Operations (by code, name, VIS)
4. Enumeration (list all, list by source, count)
5. Edge Cases (NULL handling, empty registry)
6. Integration (realistic mode definitions)

Author: SlowFrame Contributors
Version: 2.1.0
Date: February 2026
"""

import subprocess
import sys
import os

# Add project root to path
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, PROJECT_ROOT)

def compile_test():
    """Compile the mode registry test program."""
    print("=" * 80)
    print("COMPILING MODE REGISTRY TEST")
    print("=" * 80)
    
    cmd = [
        "gcc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-O0",
        "-g",
        "-I", f"{PROJECT_ROOT}/src/include",
        "-I", f"{PROJECT_ROOT}/src/util",
        "-o", f"{PROJECT_ROOT}/bin/test_mode_registry",
        f"{PROJECT_ROOT}/tests/util/test_mode_registry.c",
        f"{PROJECT_ROOT}/src/sstv/mode_registry.c",
        f"{PROJECT_ROOT}/src/util/error.c"
    ]
    
    print(" ".join(cmd))
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.returncode != 0:
        print("❌ COMPILATION FAILED")
        print("STDERR:", result.stderr)
        print("STDOUT:", result.stdout)
        return False
    
    print("✅ Compilation successful")
    return True

def run_test():
    """Run the compiled test program."""
    print("\n" + "=" * 80)
    print("RUNNING MODE REGISTRY TESTS")
    print("=" * 80 + "\n")
    
    test_binary = f"{PROJECT_ROOT}/bin/test_mode_registry"
    
    if not os.path.exists(test_binary):
        print(f"❌ Test binary not found: {test_binary}")
        return False
    
    result = subprocess.run([test_binary], capture_output=True, text=True)
    
    # Print output
    if result.stdout:
        print(result.stdout)
    if result.stderr:
        print("STDERR:", result.stderr, file=sys.stderr)
    
    return result.returncode == 0

def main():
    """Main test execution."""
    print("Mode Registry Test Suite")
    print("SlowFrame v2.1 - Phase 3.1")
    print()
    
    # Step 1: Compile
    if not compile_test():
        print("\n❌ TEST SUITE FAILED - Compilation error")
        return 1
    
    # Step 2: Run
    if not run_test():
        print("\n❌ TEST SUITE FAILED - Runtime errors detected")
        return 1
    
    print("\n" + "=" * 80)
    print("✅ ALL MODE REGISTRY TESTS PASSED")
    print("=" * 80)
    return 0

if __name__ == "__main__":
    sys.exit(main())
