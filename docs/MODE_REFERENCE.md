# PiSSTVpp2 v2.0 SSTV Mode Reference

**Complete reference for all SSTV modes supported in PiSSTVpp2 v2.0**

---

## Overview

PiSSTVpp2 v2.0 supports 7 SSTV modes across three families:
- **Martin family** (2 modes): Classic RGB sequential scan
- **Scottie family** (3 modes): Enhanced RGB sequential scan with better sync
- **Robot family** (2 modes): Modern YUV color-subsampled encoding

---

## Legend

| Column | Meaning |
|--------|---------|
| **Protocol** | CLI argument for `-p` option (e.g., `m1`, `s2`, `r36`) |
| **VIS** | Vertical Interval Signal code (transmitted in header for receiver identification) |
| **Resolution** | Image dimensions in pixels (width × height) |
| **Aspect** | Aspect ratio of the mode |
| **Duration @ 22050 Hz** | Approximate transmission time at default sample rate |
| **Color** | Color encoding scheme (RGB or YUV) |

---

## Martin Family

Martin modes use **RGB sequential scan**: each line is encoded as Green, then Blue, then Red. This provides excellent color reproduction and broad compatibility.

### Martin 1 (M1)

| Property | Value |
|----------|-------|
| **Protocol Code** | `m1` |
| **VIS Code** | 44 |
| **Resolution** | 320 × 256 |
| **Aspect Ratio** | 5:4 |
| **Duration** | ~114 seconds |
| **Color Encoding** | RGB Sequential |
| **Characteristics** | Most widely supported mode; excellent color; reliable sync |

**Transmission Structure:**
- Leader tone: 1900 Hz (~300ms)
- VIS header: 1200/2300 Hz (~640ms)
- Image data: 256 lines × 320 pixels
  - Per line: sync (4.86ms) + porches + green (77.4ms) + separator + blue (77.4ms) + separator + red (77.4ms)
- Trailer tone: 1200 Hz (~300ms)

**Best For:**
- General-purpose SSTV transmissions
- Compatibility with older SSTV equipment
- Archive transmissions where compatibility is paramount
- High-quality color images

**CLI Example:**
```bash
./bin/pisstvpp2 -i photo.jpg -p m1 -o transmission.wav
```

---

### Martin 2 (M2)

| Property | Value |
|----------|-------|
| **Protocol Code** | `m2` |
| **VIS Code** | 40 |
| **Resolution** | 320 × 256 |
| **Aspect Ratio** | 5:4 |
| **Duration** | ~58 seconds |
| **Color Encoding** | RGB Sequential |
| **Characteristics** | Fast Martin mode; half the duration of M1; same resolution |

**Transmission Structure:**
- Identical to Martin 1 but with pixel times halved
- Each pixel encoded in ~0.52ms instead of ~1.04ms
- All other timing remains proportionally faster

**Best For:**
- Quick test transmissions
- Bandwidth-limited links
- When transmission time is critical
- Still-quality images despite faster speed

**CLI Example:**
```bash
./bin/pisstvpp2 -i photo.jpg -p m2 -o fast_transmission.wav
```

---

## Scottie Family

Scottie modes enhance the Martin family design with **improved sync pulse design** and different line structure. Provides better error recovery and slightly different color distribution.

### Scottie 1 (S1)

| Property | Value |
|----------|-------|
| **Protocol Code** | `s1` |
| **VIS Code** | 60 |
| **Resolution** | 320 × 256 |
| **Aspect Ratio** | 5:4 |
| **Duration** | ~110 seconds |
| **Color Encoding** | RGB Sequential |
| **Characteristics** | Improved sync vs Martin 1; slightly different line sync |

**Transmission Structure:**
- Leader tone: 1900 Hz (~300ms)
- VIS header: 1200/2300 Hz (~640ms)
- Image data: 256 lines × 320 pixels
  - Initial sync: 9000 Hz pulse for line synchronization
  - Per line: separator + green (80.5ms) + separator + blue (80.5ms) + **line sync pulse** + separator + red (80.5ms)
- Trailer tone: 1200 Hz (~300ms)

**Best For:**
- Higher-quality reception in noisy conditions
- Professional SSTV transmissions
- When sync reliability is important
- Compatibility with modern receivers

**CLI Example:**
```bash
./bin/pisstvpp2 -i photo.jpg -p s1 -o transmission.wav
```

---

### Scottie 2 (S2)

| Property | Value |
|----------|-------|
| **Protocol Code** | `s2` |
| **VIS Code** | 56 |
| **Resolution** | 320 × 256 |
| **Aspect Ratio** | 5:4 |
| **Duration** | ~71 seconds |
| **Color Encoding** | RGB Sequential |
| **Characteristics** | Fast Scottie mode; balanced speed and quality; **recommended for general use** |

**Transmission Structure:**
- Similar to Scottie 1 but with pixel times and line sync adjusted for ~71 second transmission
- Better sync than Martin 2 despite similar speed

**Best For:**
- **Default choice for most situations** ⭐
- Best balance of transmission time and compatibility
- Daily SSTV operations
- When unsure which mode to use

**CLI Example:**
```bash
./bin/pisstvpp2 -i photo.jpg -p s2 -o transmission.wav
```

---

### Scottie DX (SDX)

| Property | Value |
|----------|-------|
| **Protocol Code** | `sdx` |
| **VIS Code** | 76 |
| **Resolution** | 320 × 256 |
| **Aspect Ratio** | 5:4 |
| **Duration** | ~269 seconds |
| **Color Encoding** | RGB Sequential |
| **Characteristics** | Slowest mode; highest quality; excellent color detail; longest transmission |

**Transmission Structure:**
- Extended pixel time vs Scottie 1 for ultra-fine detail
- Each pixel encoded in nearly 2ms
- Same line structure as Scottie 1 but with extended timing

**Best For:**
- Archival-quality transmissions
- Images with fine detail or text
- Transmission when there's no time pressure
- Situations where longest transmission time is acceptable
- Maximum compatible quality

**CLI Example:**
```bash
./bin/pisstvpp2 -i photo.jpg -p sdx -o highest_quality.wav
```

---

## Robot Family

Robot modes use **YUV color encoding with chroma subsampling**, a more efficient approach. Faster transmissions while maintaining good color fidelity.

### Robot 36 Color (R36)

| Property | Value |
|----------|-------|
| **Protocol Code** | `r36` |
| **VIS Code** | 8 |
| **Resolution** | 320 × 240 |
| **Aspect Ratio** | 4:3 |
| **Duration** | ~36 seconds |
| **Color Encoding** | YUV 4:2:0 (chroma subsampled) |
| **Characteristics** | Fast transmission; good color; efficient encoding |

**Transmission Structure:**
- Leader tone: 1900 Hz (~300ms)
- VIS header: 1200/2300 Hz (~640ms)
- Image data: 240 lines (processed as 120 pairs)
  - Y (luminance) component: full resolution
  - Cr/Cb (chroma) components: subsampled 2×2
  - Per line pair: sync + Y line 1 + separator + Y line 2 + chroma porch + Cr (averaged) + Cb (averaged)
- Trailer tone: 1200 Hz (~300ms)

**Color Subsampling Details:**
- **4:2:0 subsampling** = Chroma sampled at 1/4 the Y resolution
- Horizontal: Every 2 pixels share one Cr/Cb value
- Vertical: Every 2 lines share one Cr/Cb value
- Result: Excellent compression; 2× horizontal/vertical color reduction
- Perceived quality: Still very good for standard displays

**Best For:**
- Quick transmissions (~36 seconds)
- Testing and short hops
- When speed is priority over maximum detail
- Modern SSTV receivers (handles YUV well)
- Bandwidth-constrained scenarios

**CLI Example:**
```bash
./bin/pisstvpp2 -i photo.jpg -p r36 -o quick.wav
```

---

### Robot 72 Color (R72)

| Property | Value |
|----------|-------|
| **Protocol Code** | `r72` |
| **VIS Code** | 12 |
| **Resolution** | 320 × 240 |
| **Aspect Ratio** | 4:3 |
| **Duration** | ~72 seconds |
| **Color Encoding** | YUV 4:2:2 (chroma subsampled) |
| **Characteristics** | High-quality fast mode; better color than R36; twice the duration |

**Transmission Structure:**
- Similar to Robot 36 but with extended pixel timing
- Each pixel takes approximately 2× the time of R36
- Better color fidelity than R36 due to less aggressive chroma subsampling

**Color Subsampling Details:**
- **4:2:2 subsampling** = Chroma subsampled horizontally only
- Horizontal: Every 2 pixels share one Cr/Cb value
- Vertical: Full resolution (no vertical subsampling)
- Result: Better color detail vs R36; still efficient
- Perceived quality: Better than R36, competitive with RGB modes

**Best For:**
- Good-quality fast transmissions (~72 seconds)
- High-quality work with reasonable speed
- Color-critical images with YUV advantages
- Default for "good quality, fast" requirement
- When R36 is too fast but RGB modes are too slow

**CLI Example:**
```bash
./bin/pisstvpp2 -i photo.jpg -p r72 -o high_quality.wav
```

---

## Mode Comparison Table

Quick reference for all 7 modes:

| Mode | VIS | Time | Res | Aspect | Quality | Speed | Best For |
|------|-----|------|-----|--------|---------|-------|----------|
| **m1** | 44 | 114s | 320×256 | 5:4 | Excellent | Slow | Most compatible, archival |
| **m2** | 40 | 58s | 320×256 | 5:4 | Good | Fast | Quick Martin tests |
| **s1** | 60 | 110s | 320×256 | 5:4 | Excellent | Slow | Professional, noisy conditions |
| **s2** | 56 | 71s | 320×256 | 5:4 | Very Good | Medium | ⭐ **Default choice** |
| **sdx** | 76 | 269s | 320×256 | 5:4 | Best | Very Slow | Archival, fine detail |
| **r36** | 8 | 36s | 320×240 | 4:3 | Good | Very Fast | Quick transmissions |
| **r72** | 12 | 72s | 320×240 | 4:3 | Excellent | Medium | Good balance for YUV |

---

## Mode Selection Guide

### By Transmission Time

**Ultra-Fast (< 45 seconds)**
- **r36** (36s) - Robot 36: Fast YUV mode for quick contacts

**Fast (45-75 seconds)**
- **m2** (58s) - Martin 2: Fast RGB mode
- **s2** (71s) - Scottie 2: **Recommended** ⭐ balanced option
- **r72** (72s) - Robot 72: High-quality YUV

**Slow (100+ seconds)**
- **s1** (110s) - Scottie 1: Professional quality
- **m1** (114s) - Martin 1: Most compatible
- **sdx** (269s) - Scottie DX: Highest quality

### By Quality Level

**Good Quality**
- r36, m2: Quick transmissions with acceptable quality

**Very Good Quality**
- s2, r72: Excellent balance

**Excellent Quality**
- m1, s1: High-quality RGB modes

**Highest Quality**
- sdx: Maximum detail, slowest transmission

### By Use Case

**General Use / Default**
```bash
./bin/pisstvpp2 -i photo.jpg -p s2              # Scottie 2 - best general choice
```

**Fastest Possible**
```bash
./bin/pisstvpp2 -i photo.jpg -p r36             # Robot 36 - 36 seconds
```

**Best Compatibility**
```bash
./bin/pisstvpp2 -i photo.jpg -p m1              # Martin 1 - most receivers
```

**Highest Quality**
```bash
./bin/pisstvpp2 -i photo.jpg -p sdx             # Scottie DX - best detail
```

**Professional / Noisy Channels**
```bash
./bin/pisstvpp2 -i photo.jpg -p s1              # Scottie 1 - better sync
```

**YUV Preference (Modern**
```bash
./bin/pisstvpp2 -i photo.jpg -p r72             # Robot 72 - YUV 4:2:2
```

---

## VIS Code Reference

The VIS (Vertical Interval Signal) code identifies the SSTV mode to receivers:

| VIS Code | Mode | Protocol |
|----------|------|----------|
| 8 | Robot 36 Color | r36 |
| 12 | Robot 72 Color | r72 |
| 40 | Martin 2 | m2 |
| 44 | Martin 1 | m1 |
| 56 | Scottie 2 | s2 |
| 60 | Scottie 1 | s1 |
| 76 | Scottie DX | sdx |

---

## Technical Details

### Image Scaling

All modes scale input images to the required resolution:
- **Martin modes** (320×256): 5:4 aspect ratio
- **Scottie modes** (320×256): 5:4 aspect ratio  
- **Robot modes** (320×240): 4:3 aspect ratio

Aspect ratio correction options (via `-a` flag):
- **center** (default): Crop to target aspect ratio from center
- **pad**: Add black bars to preserve aspect ratio
- **stretch**: Force to target aspect ratio (may distort)

### Frequency Ranges

- **Sync pulses**: 1200 Hz (standard)
- **Porch/Separator**: 1500 Hz
- **Image data**: 1200-2300 Hz (RGB) or 1500-2300 Hz (YUV)
- **Bright pixels**: Higher frequencies
- **Dark pixels**: Lower frequencies

### Audio Sample Rates

PiSSTVpp2 supports sample rates from 8,000 to 48,000 Hz:
```bash
./bin/pisstvpp2 -i photo.jpg -r 44100          # High fidelity (44.1 kHz)
./bin/pisstvpp2 -i photo.jpg -r 22050          # Default (22.05 kHz)
./bin/pisstvpp2 -i photo.jpg -r 8000           # Low bandwidth (8 kHz)
```

---

## Architecture Notes

### RGB Sequential Modes (Martin, Scottie)

All three color channels (R, G, B) encoded separately for each line:
- **Encoding order**: Green → Blue → Red (traditional)
- **Inherent aspect ratio**: 5:4 (320×256 pixels)
- **Color fidelity**: Excellent (no subsampling)
- **Compatibility**: Very high (widely supported)
- **Characteristics**: Sensitive to timing errors; benefits from good sync

### YUV Chroma-Subsampled Modes (Robot)

Luminance (Y) encoded at full resolution; chroma (Cr/Cb) subsampled:
- **Inherent aspect ratio**: 4:3 (320×240 pixels)
- **Encoding**: Luma at full res; chroma at reduced resolution
- **Color fidelity**: Good (minimal artifacts)
- **Efficiency**: Better (fewer samples needed)
- **Timing**: More forgiving; data-driven sync

---

## CW Identification

All modes support optional CW (Morse code) identification appended after transmission:

```bash
./bin/pisstvpp2 -i photo.jpg -p m1 -C "N0CALL" -o transmission.wav
```

This appends: "SSTV DE N0CALL" in Morse code at the end of the audio transmission.

**CW Customization:**
- `-W <wpm>`: Speed in words-per-minute (1-50, default: 15)
- `-T <freq>`: Tone frequency in Hz (400-2000, default: 800)

---

## Version Information

- **PiSSTVpp2 Version**: 2.0.0
- **SSTV Modes**: 7 (Martin: 2, Scottie: 3, Robot: 2)
- **Audio Formats**: WAV, AIFF, OGG Vorbis
- **Sample Rates**: 8,000 - 48,000 Hz
- **Image Formats**: PNG, JPEG, GIF, BMP, TIFF, WebP (via libvips)

---

## See Also

- [User Guide](USER_GUIDE.md) - Complete feature reference
- [Quick Start](QUICK_START.md) - Get started in 5 minutes
- [BUILD.md](BUILD.md) - Installation and build instructions
- [ARCHITECTURE.md](ARCHITECTURE.md) - Technical design details
