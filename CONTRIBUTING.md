# Contributing to PiSSTVpp2

**Thank you for your interest in contributing to PiSSTVpp2!**

We welcome contributions of all kinds: bug reports, feature requests, documentation improvements, and code contributions.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [How to Contribute](#how-to-contribute)
3. [Development Workflow](#development-workflow)
4. [Coding Standards](#coding-standards)
5. [Testing Requirements](#testing-requirements)
6. [Documentation](#documentation)
7. [Submitting Changes](#submitting-changes)

---

## Getting Started

### Prerequisites

Before contributing, ensure you have:

1. **Development Environment**
   - C11-compatible compiler (GCC 4.9+ or Clang 3.4+)
   - GNU Make
   - Git

2. **Dependencies**
   - libvips 8.0+ (required)
   - libogg + libvorbis (optional, for OGG support)

3. **Understanding of Project**
   - Read [README.md](README.md)
   - Review [Architecture Documentation](docs/ARCHITECTURE.md)
   - Browse existing code

### Setting Up Development Environment

```bash
# Clone repository
git clone https://github.com/yourusername/PiSSTVpp2.git
cd PiSSTVpp2

# Install dependencies (Debian/Ubuntu example)
sudo apt-get update
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev

# Build
make clean && make all

# Run tests
cd tests
./test_suite.sh
```

**Verify setup:**
- All 55 tests should pass
- No compilation warnings
- Binary runs without errors

---

## How to Contribute

### Reporting Bugs

**Before submitting a bug report:**
1. Check existing issues to avoid duplicates
2. Verify bug exists in latest version
3. Test with minimal example

**Bug report should include:**
- OS and version
- PiSSTVpp2 version
- Steps to reproduce
- Expected behavior
- Actual behavior
- Error messages (if any)
- Relevant files (images, audio, logs)

**Example bug report:**
```
**Environment:**
- OS: Raspberry Pi OS 11 (Bullseye)
- PiSSTVpp2: v2.0
- libvips: 8.12.2

**Steps to Reproduce:**
1. Run: ./bin/pisstvpp2 -i test.jpg -o output.wav -p r72
2. Observe error message

**Expected:** WAV file created successfully

**Actual:** Error: "Failed to encode image"

**Error Log:**
[ERROR] Line 123: Invalid pixel value
...
```

### Suggesting Features

**Feature requests should include:**
- Clear description of feature
- Use case / motivation
- Example usage
- Potential implementation approach (if known)

**Example feature request:**
```
**Feature:** Add support for FAX480 mode

**Use Case:** FAX480 is used in weather satellite reception

**Example Usage:**
./bin/pisstvpp2 -i weather.jpg -p fax480 -o output.wav

**Implementation Notes:**
- VIS code: TBD
- Resolution: 512×480
- Similar to Robot modes
```

### Improving Documentation

Documentation improvements are always welcome!

**Types of documentation:**
- User guides
- Code comments
- Examples
- README updates
- Architecture notes

**Documentation standards:**
- Clear, concise language
- Working code examples
- Proper Markdown formatting
- Cross-references to related docs

---

## Development Workflow

### 1. Fork and Branch

```bash
# Fork repository on GitHub
# Clone your fork
git clone https://github.com/yourusername/PiSSTVpp2.git
cd PiSSTVpp2

# Add upstream remote
git remote add upstream https://github.com/original/PiSSTVpp2.git

# Create feature branch
git checkout -b feature/your-feature-name
```

**Branch naming:**
- `feature/feature-name` - New features
- `fix/bug-description` - Bug fixes
- `docs/topic` - Documentation
- `test/test-name` - Test improvements
- `refactor/module-name` - Code refactoring

### 2. Make Changes

**Development cycle:**
1. Make code changes
2. Build: `make clean && make all`
3. Test: `cd tests && ./test_suite.sh`
4. Fix any issues
5. Repeat

**Incremental development:**
- Make small, focused changes
- Commit frequently
- Test after each change
- Keep commits atomic

### 3. Test Your Changes

**Required testing:**
```bash
# Run full test suite
cd tests
./test_suite.sh

# All 55 tests must pass
# Fix any failures before proceeding
```

**Add new tests for:**
- New features
- Bug fixes (regression tests)
- Edge cases

**Test location:**
- Add to `tests/test_suite.sh`
- Or create new test file
- Document test purpose

### 4. Update Documentation

**Documentation to update:**
- README.md (if user-facing change)
- docs/USER_GUIDE.md (new features)
- docs/ARCHITECTURE.md (code structure changes)
- Code comments (inline documentation)

**Check documentation:**
- Accuracy
- Completeness
- Examples work
- Links valid

### 5. Commit Changes

**Commit message format:**
```
Short summary (50 chars or less)

Detailed explanation of changes:
- What was changed
- Why it was changed
- Any side effects

Fixes #123 (if closing an issue)
```

**Example commit:**
```
Add FAX480 mode support

Implements FAX480 SSTV mode with 512×480 resolution:
- Add mode definition to sstv_modes table
- VIS code 100
- Timing parameters from spec
- Tests for FAX480 encoding

Closes #45
```

**Good commit practices:**
- One logical change per commit
- Clear, descriptive messages
- Reference issues when applicable
- Include context for future readers

---

## Coding Standards

### C Code Style

**General:**
- C11 standard
- POSIX-compatible where possible
- No compiler-specific extensions (unless guarded)

**Formatting:**
```c
// Indentation: 4 spaces (no tabs)
int function_name(int arg1, int arg2)
{
    if (condition) {
        // Code here
    }
    
    for (int i = 0; i < count; i++) {
        // Loop body
    }
    
    return result;
}
```

**Naming:**
```c
// Functions: lowercase_with_underscores
int load_image(const char *filename);

// Types: lowercase_with_underscores_t
typedef struct {
    int width;
    int height;
} image_size_t;

// Constants: UPPERCASE_WITH_UNDERSCORES
#define MAX_BUFFER_SIZE 1024

// Global variables: g_prefix (avoid if possible)
static int g_initialized = 0;

// Local variables: lowercase_with_underscores
int sample_rate = 22050;
```

**Header Guards:**
```c
#ifndef PISSTVPP2_MODULE_H
#define PISSTVPP2_MODULE_H

// Header contents

#endif // PISSTVPP2_MODULE_H
```

**Comments:**
```c
/**
 * Load image from file and prepare for encoding.
 * 
 * @param filename Path to image file
 * @param width Target width in pixels
 * @param height Target height in pixels
 * @return Image object or NULL on error
 */
VipsImage* load_image(const char *filename, int width, int height);

// Single-line comments for brief explanations
int count = 0;  // Number of samples processed
```

**Error Handling:**
```c
// Check all return values
VipsImage *image;
if (vips_image_new_from_file(filename, &image, NULL) != 0) {
    fprintf(stderr, "Failed to load image: %s\n", filename);
    return NULL;
}

// Free resources on error paths
int result = process_image(image);
if (result != 0) {
    g_object_unref(image);  // Clean up
    return result;
}
```

**Memory Management:**
```c
// Always free allocated memory
char *buffer = malloc(size);
if (buffer == NULL) {
    return -1;
}
// ... use buffer ...
free(buffer);

// Use g_object_unref for GObject types (libvips)
VipsImage *image = ...;
// ... use image ...
g_object_unref(image);
```

### Code Organization

**File Structure:**
```c
// 1. Header guard
#ifndef PISSTVPP2_MODULE_H
#define PISSTVPP2_MODULE_H

// 2. Includes
#include <stdio.h>
#include <stdlib.h>
#include "other_module.h"

// 3. Constants
#define MAX_SIZE 1024

// 4. Type definitions
typedef struct {
    int field;
} type_t;

// 5. Function declarations
int function(void);

// 6. End header guard
#endif
```

**Source File:**
```c
// 1. Include own header first
#include "module.h"

// 2. System includes
#include <stdio.h>

// 3. Local includes
#include "other_module.h"

// 4. File-scope constants
static const int DEFAULT_VALUE = 100;

// 5. File-scope functions
static int helper_function(void)
{
    // ...
}

// 6. Public functions
int public_function(void)
{
    // ...
}
```

### Best Practices

**Defensive Programming:**
```c
// Validate inputs
int process_buffer(const uint8_t *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return -1;
    }
    // ...
}

// Check bounds
for (int i = 0; i < count && i < MAX_COUNT; i++) {
    // Safe access
}
```

**Const Correctness:**
```c
// Use const for read-only parameters
int calculate(const int *values, size_t count);

// Use const for string literals
const char *get_mode_name(void);
```

**Avoid Magic Numbers:**
```c
// Bad
if (value == 44) { ... }

// Good
#define MARTIN1_VIS_CODE 44
if (value == MARTIN1_VIS_CODE) { ... }
```

---

## Testing Requirements

### Test Coverage

**All contributions must include tests:**
- New features → Feature tests
- Bug fixes → Regression tests
- Refactoring → Existing tests must pass

### Writing Tests

**Test structure:**
```bash
# tests/test_suite.sh

test_your_feature() {
    echo "Testing your feature..."
    
    # Setup
    local input="tests/images/test.jpg"
    local output="/tmp/test_output.wav"
    
    # Execute
    ./bin/pisstvpp2 -i "$input" -o "$output" -p newmode
    
    # Verify
    if [ $? -ne 0 ]; then
        echo "FAIL: Command failed"
        return 1
    fi
    
    if [ ! -f "$output" ]; then
        echo "FAIL: Output file not created"
        return 1
    fi
    
    # Cleanup
    rm -f "$output"
    
    echo "PASS"
    return 0
}

# Add to test list
TESTS+=("test_your_feature")
```

**Test best practices:**
- Test one thing per test
- Use descriptive test names
- Clean up temporary files
- Check exit codes
- Verify output files
- Test edge cases

### Running Tests

```bash
# Run all tests
cd tests
./test_suite.sh

# Run specific test
./test_suite.sh test_your_feature

# Verbose output
./test_suite.sh -v
```

**Expected results:**
```
Running PiSSTVpp2 Test Suite...
[1/56] test_your_feature... PASS
[2/56] test_martin1... PASS
...
[56/56] test_edge_case... PASS

Results: 56/56 tests passed (100%)
```

### Test Validation

**Python test utilities:**
```bash
# Verify audio file
python3 util/validate_fix.py output.wav

# Analyze CW
python3 util/analyze_cw.py output.wav

# Compare audio
python3 util/compare_audio.py expected.wav actual.wav
```

---

## Documentation

### Code Documentation

**Function comments:**
```c
/**
 * Brief description of function.
 * 
 * Detailed explanation of what the function does,
 * including any important behavior or side effects.
 * 
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 * 
 * @note Any important notes or warnings
 * @see Related functions or documentation
 */
int function_name(int param1, char *param2);
```

**Inline comments:**
```c
// Explain why, not what
int sync_freq = 1200;  // SSTV standard sync frequency

// Complex logic needs explanation
// Calculate phase increment to avoid accumulation error
// over long transmissions (multiple minutes)
double phase_inc = 2.0 * M_PI * freq / sample_rate;
```

### User Documentation

**Update when:**
- Adding user-facing features
- Changing command-line options
- Fixing user-visible bugs
- Improving performance

**Files to update:**
- README.md (overview)
- docs/USER_GUIDE.md (detailed usage)
- docs/QUICK_START.md (if affects basics)

### Developer Documentation

**Update when:**
- Changing architecture
- Adding modules
- Modifying APIs
- Refactoring code

**Files to update:**
- docs/ARCHITECTURE.md
- Code comments
- Header documentation

---

## Submitting Changes

### Pull Request Process

**1. Prepare for PR:**
```bash
# Ensure branch up to date
git fetch upstream
git rebase upstream/main

# Run tests
make clean && make all
cd tests && ./test_suite.sh

# All tests must pass
```

**2. Create Pull Request:**

**PR Title:**
```
Add FAX480 mode support
```

**PR Description:**
```markdown
## Summary
Implements FAX480 SSTV mode for weather satellite images.

## Changes
- Add FAX480 mode definition (VIS code 100)
- Implement 512×480 resolution support
- Add timing parameters from specification
- Update user guide with FAX480 usage
- Add test cases for FAX480 encoding

## Testing
- All existing tests pass (55/55)
- New tests added for FAX480
- Tested on Raspberry Pi 4 and Ubuntu 22.04

## Documentation
- Updated USER_GUIDE.md with FAX480 info
- Updated MODE_REFERENCE.md with specs
- Added code comments

Closes #45
```

**3. PR Checklist:**

- [ ] Code follows project style
- [ ] All tests pass (55/55 or more)
- [ ] New tests added for changes
- [ ] Documentation updated
- [ ] Commit messages clear
- [ ] No merge conflicts
- [ ] Tested on at least one platform
- [ ] Backwards compatible (or documented)

**4. Review Process:**

- Wait for maintainer review
- Address feedback promptly
- Make requested changes
- Update PR with new commits
- Engage in discussion constructively

**5. After Merge:**

```bash
# Update local repository
git checkout main
git pull upstream main

# Delete feature branch
git branch -d feature/your-feature

# Celebrate! 🎉
```

---

## Code Review

### What We Look For

**Code Quality:**
- ✅ Follows coding standards
- ✅ Properly formatted
- ✅ No compiler warnings
- ✅ Memory-safe (no leaks, bounds checked)
- ✅ Error handling complete

**Functionality:**
- ✅ Works as intended
- ✅ Handles edge cases
- ✅ No regressions
- ✅ Performance acceptable

**Testing:**
- ✅ All tests pass
- ✅ New tests added
- ✅ Edge cases tested
- ✅ Platform compatibility

**Documentation:**
- ✅ User docs updated
- ✅ Code comments clear
- ✅ Examples work
- ✅ Architecture docs current

### Giving Feedback

**As a reviewer:**
- Be constructive
- Explain reasoning
- Suggest improvements
- Acknowledge good work

**Example feedback:**
```
Great work on the FAX480 implementation!

A few suggestions:

1. Line 123: Consider using a constant for the VIS code
   instead of magic number 100.

2. Would it make sense to add a test for invalid
   image dimensions?

3. Minor: Typo in comment on line 456 ("recieve" → "receive")

Overall this looks good. Nice job handling the different
resolution requirements!
```

### Receiving Feedback

**As a contributor:**
- Stay open to suggestions
- Ask questions if unclear
- Make requested changes
- Thank reviewers

**Example response:**
```
Thanks for the review!

1. Good point about the magic number. I'll add:
   #define FAX480_VIS_CODE 100

2. Added test_fax480_invalid_dimensions() to test suite.

3. Fixed typo, thanks for catching that!

Updated the PR with these changes.
```

---

## Development Tips

### Building Efficiently

```bash
# Incremental build (only changed files)
make

# Full clean build
make clean && make all

# Build with debug symbols
make CFLAGS="-std=c11 -Wall -Wextra -g -O0" clean all

# Parallel build (faster)
make -j$(nproc)
```

### Debugging

```bash
# Run with gdb
gdb ./bin/pisstvpp2
(gdb) run -i test.jpg -o output.wav
(gdb) bt  # Backtrace on crash

# Valgrind for memory issues
valgrind --leak-check=full ./bin/pisstvpp2 -i test.jpg -o output.wav

# Verbose logging
./bin/pisstvpp2 -i test.jpg -o output.wav -v
```

### Testing Specific Scenarios

```bash
# Test one mode
./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/test.wav -p r72 -v

# Test with different images
for img in tests/images/*.jpg; do
    ./bin/pisstvpp2 -i "$img" -o /tmp/out.wav -p s2
done

# Test all modes
for mode in m1 m2 s1 s2 sdx r36 r72; do
    ./bin/pisstvpp2 -i tests/images/test.jpg -o /tmp/$mode.wav -p $mode
done
```

---

## Getting Help

### Resources

**Documentation:**
- [Architecture](docs/ARCHITECTURE.md) - Code structure
- [Build Guide](docs/BUILD.md) - Build instructions
- [User Guide](docs/USER_GUIDE.md) - Usage reference

**Community:**
- GitHub Issues - Ask questions
- GitHub Discussions - General discussion
- Code Comments - Implementation details

### Common Questions

**Q: How do I add a new SSTV mode?**

A: See [Architecture - Extension Points](docs/ARCHITECTURE.md#adding-new-sstv-modes)

**Q: Tests are failing, what do I do?**

A: Check [Test Suite README](docs/TEST_SUITE_README.md) and ensure clean build

**Q: How do I test on Raspberry Pi?**

A: Build on Pi directly (cross-compilation is complex)

**Q: Can I add Windows support?**

A: WSL works; native Windows needs significant porting effort

---

## License

By contributing to PiSSTVpp2, you agree that your contributions will be licensed under the GNU General Public License v3.0.

See [LICENSE](LICENSE.md) for full license text.

---

## Recognition

Contributors are recognized in:
- Git commit history
- Release notes
- README.md credits section (for significant contributions)

Thank you for contributing to PiSSTVpp2! Your efforts help make amateur radio and SSTV more accessible to everyone.

**73!** 📡

---

*This contributing guide is a living document. Suggestions for improvement are welcome!*
