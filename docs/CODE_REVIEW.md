# PiSSTVpp Code Review & Linting Analysis

## Executive Summary
The code is functional but has several C linting warnings and engineering practice concerns that should be addressed. The most critical issues are prototypes missing `void` parameters and potential buffer overflow risks.

---

## CRITICAL ISSUES

### 1. **Function Prototypes Missing `void` Parameter (Compiler Warnings)**
**Severity:** HIGH  
**Current State:** 4 warnings generated
```c
// WRONG (Lines 355, 517, 614, 710)
void addvisheader() {
void buildaudio_r36 () {
void addvistrailer () {
void writefile_wav () {
```

**Fix Required:**
```c
// CORRECT
void addvisheader(void) {
void buildaudio_r36(void) {
void addvistrailer(void) {
void writefile_wav(void) {
```

**Why:** In ANSI C, an empty parameter list is deprecated. Using `void` explicitly is the modern standard (C99+) and prevents accidental implicit function declarations.

---

### 2. **Missing Argument Validation in main()**
**Severity:** HIGH  
**Issue:** No check if argc is sufficient before accessing argv

```c
// Line 143: Direct access without bounds checking
strncpy( inputfile , argv[optind] , strlen( argv[optind] ) ) ;

// RISK: If argc <= optind, this accesses uninitialized memory
```

**Fix:**
```c
if (optind >= argc) {
    fprintf(stderr, "Error: Input file required\n");
    return 1;
}
strncpy(inputfile, argv[optind], sizeof(inputfile) - 1);
inputfile[sizeof(inputfile) - 1] = '\0';
```

---

### 3. **Potential Buffer Overflow in strcat()**
**Severity:** HIGH  
**Lines:** 159-163

```c
strncpy( outputfile, inputfile , strlen( inputfile ) ) ;

#ifdef AUDIO_AIFF    
    strcat( outputfile , ".aiff" ) ;  // UNSAFE!
#endif
```

**Problem:** No length checking before `strcat()`. If `inputfile` is 250+ chars, appending `.aiff` will overflow the 255-byte buffer.

**Fix:**
```c
size_t len = strlen(inputfile);
if (len + 6 > sizeof(outputfile) - 1) {
    fprintf(stderr, "Error: filename too long\n");
    return 1;
}
strcat(outputfile, ".aiff");
```

Or better, use `strncat()`:
```c
strncat(outputfile, ".aiff", sizeof(outputfile) - strlen(outputfile) - 1);
```

---

### 4. **Off-by-One Error in Array Access**
**Severity:** MEDIUM  
**Lines:** 292 (writefile_aiff), 766 (writefile_wav)

```c
for ( i=0 ; i<=g_samples ; i++ )  // WRONG: should be <, not <=
{
    fputc( ( g_audio[i] & 0xff00 ) >> 8 , g_outfp ) ;
    fputc( ( g_audio[i] & 0x00ff )      , g_outfp ) ;
}
```

**Problem:** Array `g_audio[MAXSAMPLES]` has indices 0 to MAXSAMPLES-1. Loop reads index `g_samples`, which is out of bounds.

**Fix:**
```c
for ( i=0 ; i<g_samples ; i++ )  // Change <= to <
```

---

## IMPORTANT ISSUES

### 5. **Unused Variables**
**Severity:** MEDIUM

- `g_imgfp` and `g_outfp` are global but only used locally (lines 169, 171)
- `FILE *` pointers should not be global for this use case

**Recommendation:** Make them local to `main()`:
```c
int main(int argc, char *argv[]) {
    FILE *g_imgfp = fopen(inputfile, "r");
    FILE *g_outfp = fopen(outputfile, "w");
    // ...
}
```

---

### 6. **Inconsistent Variable Declarations**
**Severity:** MEDIUM

Lines 123-125 declare variables in the middle of code (not at function start), which violates C89 standard. While C99+ allows this, it's inconsistent with the rest of the codebase.

```c
// Line 123: Variables declared after other statements
printf( "Constants check:\n" ) ;
// ...
uint32_t starttime = time(NULL) ;  // WRONG: should be at function start
```

**Fix:** Move all variable declarations to the top of `main()`:
```c
int main(int argc, char *argv[]) {
    // All declarations here
    char *protocol;
    int option;
    uint32_t starttime;
    // ... then executable statements
}
```

---

### 7. **No Error Checking on File Operations**
**Severity:** HIGH

```c
g_imgfp = fopen( inputfile , "r" ) ;
g_outfp = fopen( outputfile , "w" ) ;
// NO checks if fopen() returned NULL!

if ( ft == FILETYPE_JPG ) 
{ g_imgp = gdImageCreateFromJpeg( g_imgfp ) ; }  // Could crash
```

**Fix:**
```c
g_imgfp = fopen(inputfile, "r");
if (!g_imgfp) {
    fprintf(stderr, "Error: Cannot open input file '%s'\n", inputfile);
    return 1;
}

g_imgp = gdImageCreateFromJpeg(g_imgfp);
if (!g_imgp) {
    fprintf(stderr, "Error: Failed to load JPEG image\n");
    fclose(g_imgfp);
    return 1;
}
```

---

### 8. **Magic Library Resource Leak Risk**
**Severity:** MEDIUM  
**Lines:** 201-224

```c
m = magic_open( MAGIC_NONE ) ;
if ( m && ( magic_load(m, NULL) == 0 ) ) {
    // ... code ...
}
if ( m ) { magic_close(m) ; }  // Good - but what if magic_load() fails?
```

While there's cleanup, the error handling is minimal. The `magic_load()` return value should be explicitly checked.

---

### 9. **Global State Management**
**Severity:** MEDIUM

Multiple global variables make the code harder to test and maintain:
```c
uint16_t   g_audio[MAXSAMPLES];
uint32_t   g_scale, g_samples;
double     g_twopioverrate, g_uspersample;
double     g_theta, g_fudge;
// ... etc (8 globals)
```

**Recommendation:** Encapsulate in a struct:
```c
typedef struct {
    uint16_t audio[MAXSAMPLES];
    uint32_t scale, samples;
    double twopioverrate, uspersample;
    double theta, fudge;
    uint16_t rate;
    uint8_t protocol;
} SSTV_State;
```

---

## CODE STYLE ISSUES

### 10. **Inconsistent Spacing & Formatting**
**Severity:** LOW

- Line 91: Mixed tabs and spaces (`	int option;`)
- Lines with inconsistent spacing: `(  1  <<  (BITS - 1)  )` vs `g_scale`
- Some functions have spaces before `()`, others don't

**Recommendation:** Use automated formatter (e.g., `clang-format`)

---

### 11. **Missing Function Documentation**
**Severity:** LOW

Some functions lack proper documentation:
- `buildaudio_r36()` - minimal comment
- `toneval_rgb()` and `toneval_yuv()` - no documentation
- VIS code meanings not explained

**Recommendation:** Add proper docstrings:
```c
/*
 * buildaudio_r36 - Generate Robot 36 SSTV audio from image
 *
 * Encodes a color image as Robot 36 SSTV format with 240 lines,
 * each line decoded as Y, R-Y, B-Y components per specification.
 *
 * Uses global g_audio[] buffer and g_* state variables.
 * No parameters, returns nothing.
 */
void buildaudio_r36(void) {
```

---

### 12. **Magic Numbers Without Explanation**
**Severity:** MEDIUM

```c
// Line 337: What is 457.6? Should be documented
buildaudio_m(457.6);

// Line 339: What is 228.8?
buildaudio_m(228.8);

// Line 469: What is 4862?
playtone( 1200 , 4862 ) ;
```

**Recommendation:** Use named constants:
```c
#define MARTIN1_PIXELTIME 457.6   // microseconds per pixel in Martin 1
#define MARTIN2_PIXELTIME 228.8   // microseconds per pixel in Martin 2
#define MARTIN_SYNC_DUR   4862    // microseconds, sync pulse duration
```

---

## COMPILER & LINT WARNINGS

### Current Warnings (gcc -Wall -Wextra -Wpedantic):
```
pisstvpp2.c:355:18: warning: function declaration without prototype [-Wstrict-prototypes]
pisstvpp2.c:517:21: warning: function declaration without prototype [-Wstrict-prototypes]
pisstvpp2.c:614:20: warning: function declaration without prototype [-Wstrict-prototypes]
pisstvpp2.c:710:20: warning: function declaration without prototype [-Wstrict-prototypes]
```

**All 4 warnings** need to be fixed by adding `void` to function declarations.

---

## POTENTIAL RUNTIME ISSUES

### 13. **Integer Casting and Precision Loss**
**Severity:** LOW  
**Lines:** 453-461 (buildaudio_r36)

```c
y1[x] = 16.0 + (0.003906 * ((65.738 * (float)r1) + ...));
```

Mixing float calculations with uint8_t array storage may cause precision issues. Consider:
```c
y1[x] = (uint8_t)(16.0 + (0.003906 * ((65.738 * (float)r1) + ...)));
```

---

### 14. **Uninitialized Variable Risk**
**Severity:** MEDIUM  
**Line:** 114

```c
char *protocol;  // Declared but not initialized!
```

If command line doesn't provide `-p` option, `protocol` is uninitialized when used on line 129:
```c
if ( strcmp(protocol, "m1") == 0 )  // UNDEFINED BEHAVIOR if -p not provided
```

**Fix:**
```c
char *protocol = "m1";  // Default
```

---

## SECURITY CONCERNS

### 15. **No Input Validation for Sample Rate**
**Severity:** MEDIUM  
**Line:** 100

```c
g_rate = (atoi(optarg));  // No range checking!
```

User could set `g_rate = 0`, `g_rate = 999999`, etc., causing:
- Division by zero in calculations
- Buffer overflow with MAXSAMPLES
- Audio file corruption

**Fix:**
```c
g_rate = atoi(optarg);
if (g_rate < 8000 || g_rate > 48000) {
    fprintf(stderr, "Error: Sample rate must be 8000-48000 Hz\n");
    return 1;
}
```

---

## RECOMMENDATIONS SUMMARY

### Priority 1 (Fix Immediately):
1. ✅ Add `void` to function declarations (lines 355, 517, 614, 710)
2. ✅ Fix off-by-one in loops: `i<=g_samples` → `i<g_samples`
3. ✅ Add null checks for `fopen()` and `gdImage*()`
4. ✅ Add buffer overflow protection in filename handling
5. ✅ Initialize `protocol` variable with default

### Priority 2 (Important):
6. Validate command-line arguments (argc, sample rate)
7. Move global `FILE*` pointers to local scope
8. Consolidate variable declarations at function start
9. Add descriptive names/constants for magic numbers
10. Improve error messages with `fprintf(stderr, ...)`

### Priority 3 (Best Practices):
11. Add function documentation comments
12. Use consistent code formatting
13. Consider refactoring globals into a state struct
14. Add build warnings to CI/CD if applicable

---

## Build Recommendations

Add to compile command for stricter checking:
```bash
gcc -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wshadow \
    -fstack-protector-strong -O3 -ffast-math -funroll-loops \
    -I/opt/homebrew/include pisstvpp2.c -o pisstvpp2 \
    -L/opt/homebrew/lib -lm -lgd -lmagic
```

This catches more potential issues at compile time.

---

## Testing Recommendations

1. **Fuzz testing:** Test with various malformed input files
2. **Boundary testing:** Test with minimum/maximum valid inputs
3. **Resource testing:** Test with very large images
4. **Error path testing:** Provide non-existent files, unreadable files, etc.

