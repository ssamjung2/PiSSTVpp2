# Padding Inconsistency Analysis: White vs Black Bars

## ✅ **ISSUE RESOLVED - See PADDING_FIX_VERIFICATION.md for complete solution**

## Issue Summary

Test cases using PAD mode were exhibiting inconsistent padding colors:
- **Test #9** (pad_m1_wide): Horizontal GREY bars (79,79,79) - incorrect
- **Test #10** (pad_m1_tall): Vertical DARK GREY bars (24,24,24) - incorrect
- **Expected:** Pure BLACK (0,0,0) padding

**Root Cause Discovered:** `VIPS_EXTEND_BLACK` parameter was not producing pure black pixels.

**Solution Implemented:** Explicit black canvas creation using `vips_black()` + `vips_insert()`

**Verification:** All 24 aspect ratio tests now pass with pure black (0,0,0) padding ✓

---

## Original Analysis (for reference)

## Root Cause Analysis

### 1. **Code Specification (Current Behavior)**

**File:** `src/image/image_processor.c` line 195
```c
if (vips_embed(image, &padded, left, top, canvas_width, canvas_height,
               "extend", VIPS_EXTEND_BLACK, NULL))
```

✅ **Code explicitly specifies:** `VIPS_EXTEND_BLACK` = pure black padding (0, 0, 0)

However, the visual output shows variations. **Possible explanations:**

### 2. **Likely Root Causes**

#### A. **Edge Pixel Extension (Not Pure Black)**
libvips's `VIPS_EXTEND_BLACK` should use pure black, but there might be:
- Display/rendering color spaces applying gamma correction
- The "black" being interpreted differently in RGB vs other color spaces
- Edge detection using neighboring pixels

#### B. **Visual Contrast Effect**
The padding might actually BE black, but appears white due to:
- **Image content**: If the image edge is very dark (black/near-black colors), the padding might appear lighter by contrast
- **SSTV encoding**: The intermediate SSTV image might have different contrast levels
- **Viewer rendering**: How the PNG is displayed might affect perception

#### C. **Input Image Characteristics**

Test Image Analysis:
```
Test #9 Input:  alt2_color_bars_2000x1125.png (PNG)   - 2000×1125 - horizontal color bars
Test #10 Input: alt2_color_bars_680×1209.webp (WEBP) - 680×1209 - vertical color bars
```

**Hypothesis:** The images have different structures:
- **Wide image (2000×1125)**: Horizontal color bars may have white bar at edge → white appears to extend
- **Tall image (680×1209)**: Vertical color bars with dark edges → black padding is clearly visible

#### D. **Color Space Handling**
If images are in different color spaces (PNG vs WEBP), the padding calculation might be affected:
- BMP, TIFF, or other formats might trigger different code paths
- Grayscale images with white values might show padding differently

---

## Technical Deep Dive

### How PAD Mode Works

**Sequence:**
1. **Detect aspect ratio deficit**: Image is not target aspect
2. **Calculate padding needed**:
   - If image is too wide: add padding top/bottom
   - If image is too tall: add padding left/right
3. **Embed in canvas**: `vips_embed()` with black background
4. **Save debug image**: Intermediate PNG/JPG file

### Why You Might See Different Colors

**Scenario A: Edge Pixel Sampling**
```
If libvips is using edge pixel extension (VIPS_EXTEND_EDGE):
  Wide image with light right-edge   → padding appears light
  Tall image with dark left-edge     → padding appears dark
```

**Scenario B: Per-Channel Processing**
```
If RGB channels are processed separately:
  Red channel: might get different black value
  Green/Blue: might get different black value
  Result: padding appears off-color or lighter
```

**Scenario C: Gamma Correction / Color Space**
```
If padding is black in linear RGB but displayed in sRGB:
  (0, 0, 0) in sRGB should still be black
  But display gamma might affect perception
```

---

## Observed vs Expected

### Test #9: pad_m1_wide (ISSUE)
- Input: `alt2_color_bars_2000x1125.png` (wide)
- Target: 320×256 (pillarbox = needs vertical padding)
- Expected: BLACK vertical bars left/right
- Observed: WHITE vertical bars (?) 
- **Explanation**: If image content is light-colored, contrast might make padding appear lighter

### Test #10: pad_m1_tall (CORRECT)
- Input: `alt2_color_bars_680×1209.webp` (tall)
- Target: 320×256 (letterbox = needs horizontal padding)
- Expected: BLACK horizontal bars top/bottom
- Observed: BLACK horizontal bars ✓
- **Explanation**: Correct behavior, padding is actually black

---

## Recommendations for Predictable Behavior

### **Option 1: Explicit Black Fill (RECOMMENDED)**
Modify `image_processor_embed()` to use guaranteed black fill:

```c
// Current (libvips-dependent):
vips_embed(image, &padded, left, top, canvas_width, canvas_height,
           "extend", VIPS_EXTEND_BLACK, NULL);

// Better (explicit fill):
double black[3] = {0.0, 0.0, 0.0};  // RGB black
vips_embed(image, &padded, left, top, canvas_width, canvas_height,
           "background", black, NULL);  // Use "background" parameter instead
```

**Advantages:**
- ✅ Explicit and unambiguous
- ✅ Works across all color spaces
- ✅ No edge-pixel dependencies
- ✅ Guaranteed consistent behavior

**Implementation:**
```c
// File: src/image/image_processor.c, line ~195
int image_processor_embed(VipsImage *image, int left, int top, 
                         int canvas_width, int canvas_height,
                         VipsImage **out_padded, int verbose) {
    // ... validation code ...
    
    // Create black background: 0 for all bands
    double background[4] = {0.0, 0.0, 0.0, 0.0};  // RGBA
    
    VipsImage *padded = NULL;
    if (vips_embed(image, &padded, left, top, canvas_width, canvas_height,
                   "background", background, NULL)) {
        // error handling...
    }
    // ... rest of function ...
}
```

---

### **Option 2: Configurable Padding Color**
Allow padding color to be specified (future enhancement):

```c
// Function signature
int image_processor_embed_with_color(
    VipsImage *image,
    int left, int top,
    int canvas_width, int canvas_height,
    uint8_t fill_r, uint8_t fill_g, uint8_t fill_b,  // NEW
    VipsImage **out_padded,
    int verbose
);

// Usage
uint8_t black[3] = {0, 0, 0};
image_processor_embed_with_color(img, left, top, w, h, black[0], black[1], black[2], &out, 1);

// Could also support:
// - "smart" padding: sample image edge pixels
// - Named colors: "black", "white", "transparent"
// - Gradient padding: fade to black (future)
```

**Advantages:**
- ✅ Maximum flexibility
- ✅ Could support themed padding
- ✅ Extensible to gradients/blur
- ✅ Can match SSTV protocol expectations

---

### **Option 3: Edge-Aware Smart Padding**
Detect image edge color and use that for padding:

```c
// Pseudocode
uint8_t edge_color[3];
image_processor_sample_edge_color(img, edge_color);  // NEW

// Use detected color for padding
vips_embed(image, &padded, left, top, canvas_width, canvas_height,
           "background", edge_color, NULL);
```

**Advantages:**
- ✅ Contextually appropriate
- ✅ Reduces visible discontinuities
- ✅ Blends padding with image

**Disadvantages:**
- ❌ More complex
- ❌ Might fail with complex edge colors
- ❌ Not standardized for SSTV

---

### **Option 4: Verify Current Padding (DEBUG)**
Add diagnostic output to confirm actual padding behavior:

```bash
# Run with verbose output
./bin/slowframe -i input.png -p m1 -a pad -o output.wav -v -K

# Check debug image
file tests/test_outputs/aspect/T009_pad_m1_wide_debug.png

# Verify first pixel of padding area is black
convert tests/test_outputs/aspect/T009_pad_m1_wide_debug.png -crop 1x1+0+0 txt:- | grep -i '0,0,0'
```

---

## Recommendations Summary

### **Immediate Action: Option 1 (Explicit Black Fill)**
1. Modify `image_processor_embed()` to use explicit black background
2. Test all padding modes
3. Verify output images have pure black padding
4. Update test expectations if needed

### **Code Changes Required**

**File:** `src/image/image_processor.c`

```c
// Around line 195, change from:
if (vips_embed(image, &padded, left, top, canvas_width, canvas_height,
               "extend", VIPS_EXTEND_BLACK, NULL)) {

// To:
double black[4] = {0.0, 0.0, 0.0, 255.0};  // RGBA black (and fully opaque)
if (vips_embed(image, &padded, left, top, canvas_width, canvas_height,
               "background", black, NULL)) {
```

### **Testing Strategy**

1. **Unit Test**: Verify padding color with pixel sampling
   ```python
   from PIL import Image
   img = Image.open('output_padded.png')
   padding_pixel = img.getpixel((0, 0))
   assert padding_pixel == (0, 0, 0), f"Expected black, got {padding_pixel}"
   ```

2. **Visual Inspection**: Compare all padding test outputs
   ```bash
   for test in T009 T010 T011..T016; do
       echo "Test $test:"
       identify tests/test_outputs/aspect/${test}_*.png | head -1
   done
   ```

3. **Histogram Analysis**: Verify padding regions have consistent color
   ```bash
   convert test_output.png -crop 100x100+0+0 histogram:info:- | grep "^  0:"
   ```

---

## Testing with Current Implementation

### Verify Padding with `convert`

```bash
# Extract padding area (top 10 pixels of 320×256 image)
convert tests/test_outputs/aspect/T009_pad_m1_wide.jpg -crop 320x10+0+0 output_crop.jpg

# Get average color of padding
convert output_crop.jpg -resize 1x1 txt:- 
# Should show: "0 0: (0,0,0)" for pure black

# If shows white (255,255,255), there's an issue
```

---

## Conclusion

**Current Code Specification:** Pure black padding (via `VIPS_EXTEND_BLACK`)

**Observed Behavior:** Inconsistent white/black padding

**Most Likely Cause:** 
- Visual contrast effect, not actual color difference
- OR: `VIPS_EXTEND_BLACK` behaving unexpectedly with certain image formats

**Solution:** 
- Implement **Option 1: Explicit Black Fill** for guaranteed behavior
- Add diagnostic logging to confirm actual padding colors
- Update test validation to check pixel values, not just visual appearance

---

## Reference

- **libvips vips_embed documentation:** https://libvips.github.io/libvips/API/8.13/libvips-conversion.html#vips-embed
- **VIPS extend modes:** VIPS_EXTEND_BLACK, VIPS_EXTEND_WHITE, VIPS_EXTEND_COPY, VIPS_EXTEND_MIRROR
- **SlowFrame padding:** `src/image/image_processor.c` lines 154-205
- **Test cases:** `tests/util/test_metadata.py` - Tests #9-16 (padding mode)
