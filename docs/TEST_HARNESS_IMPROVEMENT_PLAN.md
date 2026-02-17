# Test Harness Improvement Plan
## Comprehensive Human-Validation and Transparency Enhancement

**Date:** February 14, 2026  
**Objective:** Update all testing harnesses to provide transparency and human verification capabilities similar to `comprehensive_test_verification.py`

---

## Current Testing Infrastructure Inventory

### 1. **Master Test Orchestrators**

| File | Purpose | Current State | Human Validation |
|------|---------|---------------|------------------|
| `tests/run_master_tests.py` | Master orchestrator for all test suites | JSON reporting | ❌ No visual reports |
| `tests/util/run_comprehensive_tests.py` | Comprehensive test runner (edge, security, etc.) | JSON output | ❌ No visual reports |
| `tests/util/comprehensive_test_verification.py` | **GOLD STANDARD** - Unified test runner with HTML reports | ✅ HTML + JSON with embedded images | ✅ Full human validation |

### 2. **Feature-Specific Test Suites**

| File | Purpose | Output Format | Needs Enhancement |
|------|---------|---------------|-------------------|
| `tests/test_aspect_comprehensive.py` | Aspect ratio mode testing (CENTER, PAD, STRETCH) | ✅ Text summary only | ⚠️ Add HTML reports with annotated images |
| `tests/util/test_text_overlay_comprehensive.py` | Text overlay styling and placement | ✅ Basic reporting | 🔴 **CRITICAL** - Requires visual validation |
| `tests/util/format_validator.py` | Image format support (PNG, JPEG, GIF, etc.) | ✅ JSON output | ⚠️ Add visual samples |

### 3. **Robustness & Security Test Suites**

| File | Purpose | Output Format | Needs Enhancement |
|------|---------|---------------|-------------------|
| `tests/util/test_edge_cases.py` | Boundary values, empty inputs | Text summary | ⚠️ Structured reporting |
| `tests/util/test_error_codes.py` | Error code validation | Text summary | ⚠️ Structured reporting |
| `tests/util/test_security_exploits.py` | Security vulnerability testing | Text summary | ⚠️ Structured reporting |
| `tests/util/test_file_io_errors.py` | File I/O error handling | Text summary | ⚠️ Structured reporting |
| `tests/util/test_performance_stress.py` | Performance and stress testing | Text summary | ⚠️ Benchmark reporting |

### 4. **Verification & Validation Tools**

| File | Purpose | Current State | Enhancement Priority |
|------|---------|---------------|---------------------|
| `tests/verify_tests_interactive.py` | Terminal-based interactive verification | ✅ User prompts | ⚠️ Integrate with HTML reports |
| `tests/generate_verification_report.py` | Generates HTML verification reports | ✅ HTML generation | ⚠️ Unify with comprehensive tool |
| `tests/util/verify_overlay.py` | Overlay specification verification | Basic checks | 🔴 **CRITICAL** - Enhance visual validation |

### 5. **Utility & Analysis Tools**

| File | Purpose | Status |
|------|---------|--------|
| `tests/util/image_analyzer.py` | Image pixel analysis and detection | ✅ Core utility |
| `tests/util/aspect_validator.py` | Aspect ratio validation logic | ✅ Core utility |
| `tests/util/compare_images.py` | Image comparison utilities | ✅ Core utility |
| `tests/util/test_metadata.py` | **GOLD STANDARD** - Test definitions and metadata | ✅ Complete |
| `tests/util/show_overlay_tests.py` | Display overlay test results | Basic viewer | ⚠️ Enhance |
| `tests/util/show_results.py` | Display test summaries | Basic viewer | ⚠️ Enhance |

---

## Enhancement Strategy

### Phase 1: Standardize Reporting Infrastructure (Week 1)

**Goal:** Create reusable reporting components used by all test harnesses

#### 1.1 Create `test_report_generator.py` (NEW)
- **Purpose:** Shared HTML/JSON report generation library
- **Features:**
  - HTML templates with embedded images
  - Test metadata integration
  - CLI command tracking
  - Image annotation overlay
  - Pass/fail visual indicators
  - Human verification checklists
  - Notes capture
  
**API:**
```python
class TestReportGenerator:
    def add_test_result(test_id, metadata, command, output_files, status)
    def annotate_image(image_path, test_info, command)
    def generate_html_report(output_file)
    def generate_json_report(output_file)
    def embed_image(image_path) -> base64_string
```

#### 1.2 Create `human_verification_framework.py` (NEW)
- **Purpose:** Standardized human verification prompts and tracking
- **Features:**
  - Verification checklist templates
  - Interactive terminal prompts
  - Automated image preview (macOS `open`, Linux `xdg-open`)
  - Results persistence (JSON)
  - Session tracking
  
**API:**
```python
class HumanVerifier:
    def verify_test(test_id, metadata, images, checklist) -> verdict
    def batch_verify(test_list) -> results
    def save_session()
    def load_previous_session()
```

---

### Phase 2: High-Priority Visual Validation (Week 2)

#### 2.1 **CRITICAL: Text Overlay Tests Enhancement**
**File:** `tests/util/test_text_overlay_comprehensive.py`

**Current Gaps:**
- No visual output embedded in reports
- No human verification workflow
- Difficult to validate styling, placement, colors

**Enhancement Plan:**
```python
# Add to test suite:
1. Generate HTML report with embedded images showing:
   - Input image
   - Expected overlay spec (text, color, placement)
   - Output image with overlay applied
   - Side-by-side comparison
   
2. Annotate each output with:
   - Test number (T001, T002...)
   - Overlay specification
   - CLI command used
   
3. Human verification checklist:
   □ Text rendered correctly
   □ Correct color applied
   □ Correct placement (top/bottom/custom)
   □ Correct styling (bold, outlined, shadow)
   □ Text readable and clear
   □ No artifacts or clipping
   
4. Interactive verification mode:
   $ python3 test_text_overlay_comprehensive.py --verify-interactive
   (Opens each image, prompts for approval, saves verdicts)
```

**Implementation:**
- Integrate `TestReportGenerator` for HTML output
- Use `HumanVerifier` for interactive validation
- Save annotated images with test metadata
- Generate comparison views (expected vs actual)

#### 2.2 **Format Validation Enhancement**
**File:** `tests/util/format_validator.py`

**Enhancement Plan:**
```python
# Add visual samples:
1. Generate HTML report showing:
   - Format name (PNG, JPEG, GIF, etc.)
   - Sample input image
   - Converted output image
   - Metadata (dimensions, color depth, compression)
   - Load success/failure status
   
2. Verification checklist:
   □ Image loaded successfully
   □ Dimensions preserved
   □ Colors preserved (no corruption)
   □ No visual artifacts
   □ Metadata correct
```

#### 2.3 **Aspect Ratio Test Enhancement**
**File:** `tests/test_aspect_comprehensive.py`

**Enhancement Plan:**
```python
# Already has PIL integration, add:
1. HTML report generation (currently only text summary)
2. Annotated output images with:
   - Test ID
   - Mode (CENTER, PAD, STRETCH)
   - Input → Output dimensions
   - CLI command
   
3. Visual validation helpers:
   - Highlight padding regions (for PAD mode)
   - Show crop boundaries (for CENTER mode)
   - Distortion indicators (for STRETCH mode)
   
4. Automated checks PLUS human verification:
   - Automated: Dimensions, padding color
   - Human: Visual quality, cropping appropriateness
```

---

### Phase 3: Robustness Test Reporting (Week 3)

#### 3.1 **Edge Case Tests Enhancement**
**File:** `tests/util/test_edge_cases.py`

**Enhancement Plan:**
```python
# Structured reporting:
1. JSON report with test results grouped by category:
   - Boundary values
   - Empty inputs
   - Invalid combinations
   - Error code accuracy
   
2. HTML report showing:
   - Test category
   - Test name and description
   - CLI command
   - Expected behavior (error/success)
   - Actual result
   - Pass/fail verdict
   - Error code validation
   
3. Human verification for ambiguous cases:
   - Error messages clarity
   - Error handling appropriateness
```

#### 3.2 **Error Code Tests Enhancement**
**File:** `tests/util/test_error_codes.py`

**Enhancement Plan:**
```python
# Add tabular reports:
1. HTML table showing:
   - Error scenario
   - Expected error code
   - Actual error code
   - Error message text
   - Pass/fail
   
2. Coverage report:
   - List all error codes from error.h
   - Mark which are tested
   - Identify untested codes
```

#### 3.3 **Security Test Enhancement**
**File:** `tests/util/test_security_exploits.py`

**Enhancement Plan:**
```python
# Security-focused reporting:
1. HTML report with severity levels:
   🔴 CRITICAL: Exploits that succeeded (should fail)
   🟡 WARNING: Unexpected behavior
   🟢 PASS: Exploit properly rejected
   
2. Vulnerability tracking:
   - Exploit type (injection, traversal, overflow)
   - Test vector
   - Actual behavior
   - Mitigation status
   
3. Human review required for:
   - Edge cases in security boundaries
   - Novel attack vectors
```

---

### Phase 4: Master Orchestrator Unification (Week 4)

#### 4.1 **Unify Master Test Runners**

**Current State:**
- `run_master_tests.py` - Main orchestrator
- `run_comprehensive_tests.py` - Secondary orchestrator
- `comprehensive_test_verification.py` - Most complete

**Goal:** Single unified orchestrator with pluggable test suites

**New Architecture:**
```python
# tests/unified_test_runner.py (NEW)

class UnifiedTestRunner:
    """Single orchestrator for ALL SlowFrame tests"""
    
    def __init__(self):
        self.suites = {
            'aspect': AspectTestSuite,
            'overlay': TextOverlayTestSuite,
            'formats': FormatValidatorSuite,
            'edge': EdgeCaseTestSuite,
            'errors': ErrorCodeTestSuite,
            'security': SecurityTestSuite,
            'performance': PerformanceTestSuite,
        }
        self.report_generator = TestReportGenerator()
        self.human_verifier = HumanVerifier()
    
    def run_all_suites(self):
        """Run all test suites"""
        
    def run_suite(self, name):
        """Run specific suite"""
        
    def generate_reports(self):
        """Generate unified HTML + JSON reports"""
        
    def verify_interactive(self, suite=None):
        """Interactive human verification"""
```

**Usage:**
```bash
# Run all tests
python3 tests/unified_test_runner.py

# Run specific suite
python3 tests/unified_test_runner.py --suite overlay

# Generate reports without running
python3 tests/unified_test_runner.py --report-only

# Interactive verification
python3 tests/unified_test_runner.py --verify-interactive

# Run and verify in one go
python3 tests/unified_test_runner.py --suite overlay --verify-interactive
```

---

## Implementation Checklist

### Week 1: Infrastructure
- [ ] Create `tests/util/test_report_generator.py`
  - [ ] HTML template system
  - [ ] Image embedding and annotation
  - [ ] JSON report generation
  - [ ] CLI command tracking
  
- [ ] Create `tests/util/human_verification_framework.py`
  - [ ] Interactive verification prompts
  - [ ] Automated image preview
  - [ ] Session persistence
  - [ ] Verdict tracking

- [ ] Update `test_metadata.py` to include verification checklists
  - [ ] Add `verification_checklist` field to TestMetadata
  - [ ] Define checklists for each test category

### Week 2: Visual Validation Tests
- [ ] **Enhance `test_text_overlay_comprehensive.py`**
  - [ ] Integrate TestReportGenerator
  - [ ] Add image annotations
  - [ ] Generate HTML report with embedded images
  - [ ] Add interactive verification mode
  - [ ] Define verification checklists
  
- [ ] **Enhance `format_validator.py`**
  - [ ] Add HTML report generation
  - [ ] Embed sample images for each format
  - [ ] Add metadata display
  - [ ] Visual verification checklists

- [ ] **Enhance `test_aspect_comprehensive.py`**
  - [ ] Add HTML report (currently only text)
  - [ ] Annotate output images
  - [ ] Add padding/crop visualization helpers
  - [ ] Human verification workflow

### Week 3: Robustness Tests
- [ ] **Enhance `test_edge_cases.py`**
  - [ ] JSON structured reports
  - [ ] HTML categorized reports
  - [ ] Test coverage tracking
  
- [ ] **Enhance `test_error_codes.py`**
  - [ ] HTML tabular reports
  - [ ] Error code coverage report
  - [ ] Untested code identification
  
- [ ] **Enhance `test_security_exploits.py`**
  - [ ] Security-focused HTML reports
  - [ ] Severity level indicators
  - [ ] Vulnerability tracking dashboard

- [ ] **Enhance `test_performance_stress.py`**
  - [ ] Benchmark reporting
  - [ ] Performance graphs
  - [ ] Regression detection

### Week 4: Unification
- [ ] Create `tests/unified_test_runner.py`
  - [ ] Integrate all test suites
  - [ ] Unified reporting
  - [ ] Interactive verification mode
  - [ ] Suite selection
  
- [ ] Deprecate/merge old orchestrators
  - [ ] Mark `run_master_tests.py` as deprecated
  - [ ] Mark `run_comprehensive_tests.py` as deprecated
  - [ ] Migrate functionality to unified runner

- [ ] Documentation
  - [ ] Update `docs/TEST_QUICK_START.md`
  - [ ] Create `docs/TEST_HUMAN_VERIFICATION.md`
  - [ ] Update all test-related documentation

---

## Human Verification Requirements by Test Type

### 🔴 **CRITICAL - Always Requires Human Verification**
1. **Text Overlay Tests** - Visual appearance, readability, styling
2. **Format Validation** - Visual quality, color accuracy
3. **Security Tests** - Edge cases, novel attack vectors

### 🟡 **RECOMMENDED - Human Spot-Check**
1. **Aspect Ratio Tests** - Visual quality assessment
2. **Edge Case Tests** - Error message clarity
3. **Performance Tests** - Real-world performance validation

### 🟢 **OPTIONAL - Automated Sufficient**
1. **Error Code Tests** - Exact code matching
2. **Integration Tests** - Functional correctness
3. **File I/O Tests** - File existence and permissions

---

## Report Output Structure

### Unified HTML Report Layout
```
SlowFrame Test Report - [Date]
├── Executive Summary
│   ├── Total Tests: X/Y passed
│   ├── By Suite breakdown
│   └── Critical issues (if any)
│
├── Test Suites
│   ├── Aspect Ratio Tests (24 tests)
│   │   ├── T001_center_m1_wide
│   │   │   ├── Input Image (embedded)
│   │   │   ├── Output Image (embedded, annotated)
│   │   │   ├── CLI Command
│   │   │   ├── Automated Checks: ✅
│   │   │   └── Human Verification: □ Pending / ✅ Approved / ❌ Failed
│   │   └── ...
│   │
│   ├── Text Overlay Tests (50+ tests)
│   │   ├── Visual comparison grid
│   │   ├── Checklist for each test
│   │   └── Notes field
│   │
│   ├── Format Validation (6 formats)
│   ├── Edge Cases (50+ tests)
│   ├── Error Codes (30+ tests)
│   └── Security Tests (40+ tests)
│
└── Verification Session
    ├── Date/Time
    ├── Verifier name (optional)
    └── Notes and issues
```

### JSON Report Structure
```json
{
  "report_date": "2026-02-14T16:30:00",
  "summary": {
    "total_tests": 200,
    "passed": 198,
    "failed": 2,
    "human_verified": 150,
    "pending_verification": 50
  },
  "suites": {
    "aspect": { ... },
    "overlay": { ... },
    ...
  },
  "tests": [
    {
      "id": "T001",
      "name": "center_m1_wide",
      "suite": "aspect",
      "command": "./bin/slowframe -i ... -o ...",
      "status": "passed",
      "automated_checks": { ... },
      "human_verification": {
        "verified": true,
        "verdict": "approved",
        "notes": "Cropping looks correct",
        "verifier": "human",
        "date": "2026-02-14T16:45:00"
      },
      "files": {
        "input": "tests/images/...",
        "output": "tests/test_outputs/...",
        "annotated": "tests/test_outputs/.../annotated.png"
      }
    }
  ]
}
```

---

## Success Criteria

1. ✅ **Universal HTML Reports:** All test suites generate HTML reports with embedded images
2. ✅ **Image Annotations:** All visual tests have annotated output images (test ID, command, metadata)
3. ✅ **Human Verification Workflow:** Interactive verification mode for visual tests
4. ✅ **Unified Orchestrator:** Single entry point for running all tests
5. ✅ **Verification Tracking:** Persistent storage of human verification verdicts
6. ✅ **Side-by-Side Comparisons:** Visual tests show input/output/expected comparisons
7. ✅ **Checklists:** Each test type has defined verification checklist
8. ✅ **Complete Documentation:** Updated docs explaining verification workflow

---

## Migration Strategy

### Backward Compatibility
- Keep existing test files functional
- Add new reporting as optional `--html-report` flag initially
- Gradually deprecate old runners

### User Communication
```bash
# Old way (still works)
python3 tests/test_aspect_comprehensive.py

# New way (enhanced reporting)
python3 tests/test_aspect_comprehensive.py --html-report

# Future way (unified)
python3 tests/unified_test_runner.py --suite aspect
```

### Deprecation Timeline
- Week 1-2: Add new features with flags
- Week 3: Make new features default
- Week 4: Deprecate old runners (keep as wrappers)

---

## Priority Ranking

1. **🔴 P0 - Text Overlay Enhancement** (Week 2, Day 1-3)
   - Most critical for human validation
   - Current gap is largest here

2. **🔴 P0 - Report Generator Framework** (Week 1, Day 1-4)
   - Needed for all other enhancements
   - Reusable across all suites

3. **🟡 P1 - Aspect & Format Tests** (Week 2, Day 4-5)
   - High value, moderate gaps

4. **🟡 P1 - Unified Orchestrator** (Week 4)
   - Improves usability significantly

5. **🟢 P2 - Robustness Test Reports** (Week 3)
   - Lower urgency, mostly functional now

---

## Next Steps

1. **Review and approval** of this plan
2. **Create Week 1 infrastructure** (`test_report_generator.py`, `human_verification_framework.py`)
3. **Implement P0 text overlay enhancement** as proof of concept
4. **Iterate based on feedback**
5. **Roll out to all test suites**

**Estimated Total Effort:** 4 weeks (1 developer, full-time)  
**Can be parallelized:** Infrastructure + individual test enhancements can proceed simultaneously
