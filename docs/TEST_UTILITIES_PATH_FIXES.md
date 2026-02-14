# Test Utility Path Fixes - Complete Summary

## Overview
All Python test utility files in `/tests/util/` have been updated to use relative path resolution instead of hardcoded absolute paths. This makes the test suite portable and location-independent.

## Changes Made

### 1. Path Resolution Strategy
**Before:**
- Hardcoded absolute paths like `/Users/ssamjung/Desktop/WIP/PiSSTVpp2/bin/pisstvpp2`
- Relative paths like `../bin/pisstvpp2` that assumed tests were run from `/tests/` directory

**After:**
- Dynamic path resolution using `Path(__file__).parent` 
- Works correctly from `/tests/util/` location
- Falls back to automatic defaults if no path provided
- Can accept custom paths as arguments

### 2. Files Updated

#### Executable Path Resolution
All of the following now use:
```python
if executable_path is None:
    script_dir = Path(__file__).parent.parent.parent
    executable_path = str(script_dir / "bin" / "pisstvpp2")
```

**Files:**
- `test_suite.py` - line 21-24, 1227
- `test_edge_cases.py` - line 38-40, 614-619
- `test_error_codes.py` - line 52-55, 380-384
- `test_security_exploits.py` - line 36-40, 649-653
- `test_integration.py` - line 43-56, 455-465
- `test_file_io_errors.py` - line 29-39, 625-636
- `test_performance_stress.py` - line 30-40, 807-820
- `test_text_overlay_comprehensive.py` - line 21-31, 1106-1110
- `run_all_tests.py` - line 28-31, 270-277
- `run_comprehensive_tests.py` - line 28-36, 278-286

#### Test Images Path Resolution
Files using images directory now use:
```python
if images_dir is None:
    script_dir = Path(__file__).parent.parent
    images_dir = str(script_dir / "images")
```

**Files:**
- `test_integration.py` - line 43-56
- `test_file_io_errors.py` - line 29-39
- `test_performance_stress.py` - line 30-40

#### Test Outputs Path Resolution
Files referencing test_outputs now use:
```python
self.test_dir = Path(__file__).parent.parent / "test_outputs"
```

**Files:**
- `test_suite.py` - line 30
- `test_text_overlay_comprehensive.py` - line 32
- `show_overlay_tests.py` - line 8

#### Import Fixes
Added missing `from pathlib import Path` imports to:
- `run_comprehensive_tests.py` - line 13
- `show_results.py` - line 6

#### Show Results Path Updates
Fixed to find most recent test results:
```python
results_dir = Path(__file__).parent.parent / "test_outputs"
results_files = list(results_dir.glob('test_results_*.json'))
# Use max() to find most recent
```

### 3. Directory Structure Assumptions

The path resolution assumes:
```
/
├── bin/
│   └── pisstvpp2          # Executable
├── tests/
│   ├── images/            # Test image fixtures
│   ├── test_outputs/      # Generated test outputs
│   └── util/              # Test scripts (THIS DIRECTORY)
└── (other src, docs, etc.)
```

### 4. Usage Examples

#### Run test from any directory:
```bash
cd /Users/ssamjung/Desktop/WIP/PiSSTVpp2

# Works from project root
python3 tests/util/test_suite.py

# Works from tests directory
cd tests/util
python3 test_suite.py

# Works from anywhere with full path
python3 /Users/ssamjung/Desktop/WIP/PiSSTVpp2/tests/util/test_suite.py
```

#### Override executable path:
```bash
# All test files accept --exe or as first argument
python3 tests/util/test_suite.py --exe /custom/path/to/pisstvpp2

# Integration test format
python3 tests/util/test_integration.py /custom/bin/pisstvpp2 /custom/images/dir
```

### 5. Verification

All test utility files have been tested and verified:
- ✓ Executable paths resolve correctly
- ✓ Test image directory paths resolve correctly  
- ✓ Test output directory paths resolve correctly
- ✓ Test results viewer finds most recent results
- ✓ Scripts can be run from any directory
- ✓ Cannot run successfully from `/tests/util/` as originally designed

### 6. Backward Compatibility

- ✓ Existing command-line argument handling preserved
- ✓ New defaults don't break existing workflows
- ✓ Custom paths can still be provided as arguments
- ✓ All test functionality maintained

### 7. Benefits

1. **Portability**: Tests work from any directory
2. **CI/CD Ready**: Tests can run in any environment
3. **Cleaner Code**: No hardcoded user-specific paths
4. **Maintainability**: Path logic centralized in each class
5. **Flexibility**: Paths can be overridden when needed

## Testing Status

- ✓ test_suite.py - WORKING
- ✓ show_overlay_tests.py - WORKING
- ✓ show_results.py - WORKING
- ✓ test_integration.py - WORKING
- ✓ test_performance_stress.py - WORKING
- ✓ test_file_io_errors.py - WORKING
- ✓ test_edge_cases.py - WORKING
- ✓ test_security_exploits.py - WORKING
- ✓ test_text_overlay_comprehensive.py - WORKING
- ✓ run_all_tests.py - WORKING
- ✓ run_comprehensive_tests.py - WORKING

## Quick Verification

Run this to verify all test utilities work:
```bash
python3 test_paths_verification.py
```

This checks that:
1. All test classes can be imported
2. All test classes initialize with correct defaults
3. All required files are found (executable, images, etc.)
