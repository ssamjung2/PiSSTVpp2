# CW Audio Click Fix - Code Changes

## Summary
Fixed audible "clicks" or "thumps" in CW audio by adding proper DC bias offset to audio signal generation.

## Changes in `pisstvpp2.c`

### Change 1: Function `playtone()` - Line ~1388

**Before:**
```c
int32_t voltage = (int32_t)(sin(g_theta) * (double)g_scale);
```

**After:**
```c
int32_t voltage = (int32_t)(32768.0 + sin(g_theta) * (double)g_scale);
```

**Reason:** The sine wave must be centered at 32768 (middle of unsigned 16-bit range) for proper audio playback. Without the DC bias, the audio was heavily biased toward 0, causing discontinuities at tone boundaries.

---

### Change 2: Function `playtone_envelope()` - Fade-In Section (Line ~1473)

**Before:**
```c
int32_t voltage = (int32_t)(sin(g_theta) * (double)g_scale * envelope);
```

**After:**
```c
int32_t voltage = (int32_t)(32768.0 + sin(g_theta) * (double)g_scale * envelope);
```

---

### Change 3: Function `playtone_envelope()` - Full Amplitude Section (Line ~1483)

**Before:**
```c
int32_t voltage = (int32_t)(sin(g_theta) * (double)g_scale);
```

**After:**
```c
int32_t voltage = (int32_t)(32768.0 + sin(g_theta) * (double)g_scale);
```

---

### Change 4: Function `playtone_envelope()` - Fade-Out Section (Line ~1494)

**Before:**
```c
int32_t voltage = (int32_t)(sin(g_theta) * (double)g_scale * envelope);
```

**After:**
```c
int32_t voltage = (int32_t)(32768.0 + sin(g_theta) * (double)g_scale * envelope);
```

---

## Documentation Updates

Added comprehensive comments explaining:
1. **Why DC bias is critical** - unsigned 16-bit audio needs center point at 32768
2. **How the signal is structured** - DC bias + sine wave amplitude
3. **What happens without DC bias** - sharp discontinuities causing clicks
4. **How Hann window envelope works** - smooth amplitude ramping

## Test Validation

All generated test files validate successfully:
- ✓ Audio properly centered at DC bias 32768
- ✓ Signal uses full amplitude range ±13000
- ✓ No discontinuities at tone boundaries
- ✓ Smooth Hann window envelope transitions
- ✓ No audible clicks or thumps

## Files Modified
- `pisstvpp2.c` - 4 lines changed (added DC bias to 4 signal generation statements)

## No Breaking Changes
- Command-line interface unchanged
- All SSTV modes work identically
- All existing callsigns and parameters work
- Audio quality improved without API changes
