# MMSSTV Mode Reference

**Extended SSTV modes planned for PiSSTVpp2 v2.1 with MMSSTV library integration**

> **Note**: This document describes features planned for v2.1. PiSSTVpp2 v2.0 supports only the 7 legacy modes (Martin 1/2, Scottie 1/2/DX, Robot 36/72). See [MODE_REFERENCE.md](MODE_REFERENCE.md) for v2.0 modes.

---

## Legend

- **Protocol**: CLI argument for `-p` option (planned)
- **VIS**: Vertical Interval Signal code (transmitted in header)
- **Resolution**: Image dimensions in pixels
- **Duration**: Approximate transmission time
- **Backend**: L = Legacy, M = MMSSTV library (planned for v2.1)

---

## Martin Family (RGB Sequential)

| Protocol | VIS | Mode Name | Resolution | Duration | Backend |
|----------|-----|-----------|------------|----------|---------|
| m1       | 44  | Martin 1  | 320x256    | 114s     | L       |
| m2       | 40  | Martin 2  | 320x256    | 58s      | L       |
| m3       | 104 | Martin 3  | 256x256    | 57s      | M       |
| m4       | 108 | Martin 4  | 256x256    | 29s      | M       |

**Characteristics:**
- RGB sequential scan (R→G→B per line)
- 4:3 aspect ratio
- Excellent color reproduction
- Compatible with most SSTV software

---

## Scottie Family (RGB Sequential)

| Protocol | VIS | Mode Name  | Resolution | Duration | Backend |
|----------|-----|------------|------------|----------|---------|
| s1       | 60  | Scottie 1  | 320x256    | 110s     | L       |
| s2       | 56  | Scottie 2  | 320x256    | 71s      | L       |
| s3       | 52  | Scottie 3  | 256x128    | 55s      | M       |
| s4       | 48  | Scottie 4  | 256x128    | 36s      | M       |
| sdx      | 76  | Scottie DX | 320x256    | 269s     | L       |

**Characteristics:**
- RGB sequential with different timing than Martin
- Better sync pulse design than Martin
- Widely supported
- DX mode for highest quality RGB

---

## Robot Family (YUV 4:2:0 / 4:2:2)

| Protocol | VIS | Mode Name       | Resolution | Duration | Color   | Backend |
|----------|-----|-----------------|------------|----------|---------|---------|
| r12      | 4   | Robot 12 Color  | 160x120    | 12s      | 4:2:0   | M       |
| r24      | 12  | Robot 24 Color  | 320x240    | 24s      | 4:2:2   | M       |
| r36      | 8   | Robot 36 Color  | 320x240    | 36s      | 4:2:0   | L       |
| r72      | 12  | Robot 72 Color  | 320x240    | 72s      | 4:2:2   | L       |

**Characteristics:**
- YUV color encoding (more efficient than RGB)
- 4:2:0 = Chroma subsampled 2x horizontal and vertical
- 4:2:2 = Chroma subsampled 2x horizontal only (better quality)
- Fast transmission times
- Unique calibration header (not VIS)

---

## PD Family (YCrCb with Sync)

| Protocol | VIS | Mode Name | Resolution | Duration | Backend |
|----------|-----|-----------|------------|----------|---------|
| pd50     | 93  | PD 50     | 320x256    | 50s      | M       |
| pd90     | 99  | PD 90     | 320x256    | 90s      | M       |
| pd120    | 95  | PD 120    | 640x496    | 120s     | M       |
| pd160    | 98  | PD 160    | 512x400    | 160s     | M       |
| pd180    | 96  | PD 180    | 640x496    | 180s     | M       |
| pd240    | 97  | PD 240    | 640x496    | 240s     | M       |
| pd290    | 94  | PD 290    | 800x616    | 290s     | M       |

**Characteristics:**
- YCrCb color space (broadcast standard)
- Horizontal sync pulses in each line
- Higher resolutions available (up to 800x616)
- Popular for high-quality transmissions
- Self-synchronizing (resistant to timing errors)

---

## Wraase Family (SC2)

| Protocol | VIS | Mode Name       | Resolution | Duration | Backend |
|----------|-----|-----------------|------------|----------|---------|
| sc2_60   | 56  | Wraase SC2-60   | 320x256    | 60s      | M       |
| sc2_120  | 55  | Wraase SC2-120  | 320x256    | 120s     | M       |

**Characteristics:**
- YCrCb encoding
- Designed for improved sync
- Two speed variants (60s and 120s)

---

## AVT Family

| Protocol | VIS | Mode Name | Resolution | Duration | Backend |
|----------|-----|-----------|------------|----------|---------|
| avt24    | 68  | AVT 24    | 125x120    | 24s      | M       |
| avt90    | 69  | AVT 90    | 250x240    | 90s      | M       |
| avt94    | 70  | AVT 94    | 160x120    | 94s      | M       |

**Characteristics:**
- Developed by AVT (Amateur Video Terminal)
- Compact resolutions for quick transmissions
- Good for low-bandwidth conditions

---

## Pasokon Family (High Resolution)

| Protocol | VIS | Mode Name  | Resolution | Duration | Backend |
|----------|-----|------------|------------|----------|---------|
| p3       | 113 | Pasokon P3 | 640x496    | 203s     | M       |
| p5       | 114 | Pasokon P5 | 640x496    | 305s     | M       |
| p7       | 115 | Pasokon P7 | 640x496    | 406s     | M       |

**Characteristics:**
- Developed for Japanese amateur radio
- High resolution (640x496)
- Three quality/speed trade-offs
- Excellent detail preservation

---

## FAX Mode (Monochrome)

| Protocol | VIS | Mode Name | Resolution | Duration | Backend |
|----------|-----|-----------|------------|----------|---------|
| fax480   | 0   | FAX 480   | 512x480    | 240s     | M       |

**Characteristics:**
- Monochrome only (no color)
- High vertical resolution
- Used for document/text transmission
- 5-second FAX tone header (not VIS)

---

## ML Family (High Resolution Color)

| Protocol | VIS | Mode Name | Resolution | Duration | Backend |
|----------|-----|-----------|------------|----------|---------|
| ml180    | 120 | ML 180    | 640x496    | 180s     | M       |
| ml240    | 121 | ML 240    | 640x496    | 240s     | M       |
| ml320    | 122 | ML 320    | 640x496    | 320s     | M       |

**Characteristics:**
- Modern high-resolution modes
- 640x496 color images
- Three speed/quality variants
- Excellent for detailed images

---

## Mode Selection Guide

### Quick Transmission (< 30s)
- **r12** (12s, 160x120) - Fastest color mode
- **m4** (29s, 256x256) - Fast RGB mode

### Standard Quality (30s - 90s)
- **r36** (36s, 320x240) - Good balance, legacy
- **pd50** (50s, 320x256) - Good sync, modern
- **s2** (71s, 320x256) - Classic RGB mode
- **r72** (72s, 320x240) - High quality YUV

### High Quality (90s - 180s)
- **s1** (110s, 320x256) - Classic high-quality RGB
- **m1** (114s, 320x256) - Very popular mode
- **pd120** (120s, 640x496) - High resolution
- **pd180** (180s, 640x496) - Excellent quality
- **ml180** (180s, 640x496) - Modern high-res

### Highest Quality (> 180s)
- **pd240** (240s, 640x496) - Professional quality
- **sdx** (269s, 320x256) - Best RGB quality
- **pd290** (290s, 800x616) - Highest resolution PD
- **ml320** (320s, 640x496) - Highest quality ML
- **p7** (406s, 640x496) - Pasokon maximum quality

---

## CLI Examples

```bash
# Fast transmission (Robot 12, 12 seconds)
./bin/pisstvpp2 -i photo.jpg -p r12 -o output.wav

# Standard quality (PD 120, 2 minutes)
./bin/pisstvpp2 -i photo.jpg -p pd120 -o output.wav

# Highest quality (PD 290, 4.8 minutes)
./bin/pisstvpp2 -i photo.jpg -p pd290 -o output.ogg -r 44100

# Classic mode (Martin 1, most compatible)
./bin/pisstvpp2 -i photo.jpg -p m1 -o output.wav

# List all available modes
./bin/pisstvpp2 --list-modes

# Show mode details
./bin/pisstvpp2 --mode-info pd120
```

---

## Compatibility Notes

### Universal Compatibility (Most Software)
- **m1**, **m2** - Martin modes
- **s1**, **s2** - Scottie modes
- **r36** - Robot 36

### Wide Compatibility (Modern Software)
- **pd120**, **pd180**, **pd240** - PD modes
- **r72** - Robot 72
- **sdx** - Scottie DX

### Specialized Software Required
- **ML modes** - Requires MMSSTV or compatible
- **Pasokon modes** - Requires MMSSTV or compatible
- **AVT modes** - Less common, check decoder support

---

## Technical Specifications

### Frequency Ranges

**RGB Modes (Martin, Scottie):**
- Black: 1500 Hz
- White: 2300 Hz
- Sync: 1200 Hz

**YUV Modes (Robot):**
- Y (Luma) range: 1500-2300 Hz
- U/V (Chroma) range: 1500-2300 Hz
- Sync: 1200 Hz

**PD Modes:**
- Black: 1500 Hz
- White: 2300 Hz
- Sync: 1200 Hz
- Porch: 1900 Hz

### Sample Rate Recommendations

| Mode Family | Min Rate | Recommended | High Quality |
|-------------|----------|-------------|--------------|
| Martin      | 11025 Hz | 22050 Hz    | 44100 Hz     |
| Scottie     | 11025 Hz | 22050 Hz    | 44100 Hz     |
| Robot       | 11025 Hz | 22050 Hz    | 44100 Hz     |
| PD          | 22050 Hz | 44100 Hz    | 48000 Hz     |
| ML          | 22050 Hz | 44100 Hz    | 48000 Hz     |

---

## See Also

- [MMSSTV Integration Guide](MMSSTV_INTEGRATION.md) - Integration architecture
- [Test Suite README](TEST_SUITE_README.md) - Testing all modes
- [Quick Reference](QUICK_REFERENCE.md) - General usage guide

---

*Mode reference prepared: January 29, 2026*  
*Total modes: 7 legacy + 25 MMSSTV = 32 modes*  
*Extensible to 100+ modes with mmsstv-portable library*
