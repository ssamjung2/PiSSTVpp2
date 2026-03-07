# SSTV Noise Generator - User Guide

## Overview

The SSTV Noise Generator (`tests/util/generate_sstv_noise.py`) creates realistic RF interference patterns for testing the SlowFrame stitcher's robustness against real-world ham radio QRM (interference).

**Key Feature**: Noise affects **entire scan lines** horizontally, simulating how SSTV transmission works (line-by-line from top to bottom).

## How SSTV Rendering Works

SSTV encodes images as continuous audio by scanning horizontally across the image, line by line:
1. Transmitter sends scan line 0 (all pixels left-to-right)
2. Sends scan line 1, line 2, etc.
3. RF interference during transmission corrupts the audio
4. Receiver decodes all affected pixels in that scan line

Therefore, interference manifests as **full-width horizontal bands** of noise, not isolated small regions.

## Noise Presets

### `clean`
**Description**: Clean signal, minimal noise  
**Use Case**: Baseline reference for successful stitching  
**Characteristics**:
- Minimal white noise (1%)
- No hash line corruption
- No bursts or interference tones

```bash
python3 generate_sstv_noise.py --input tile.png --output clean.png --preset clean
```

---

### `light_qrm`
**Description**: Light RF interference - occasional corrupted scan line bands  
**Use Case**: Testing recovery on weak signals  
**Characteristics**:
- 5% of scan lines affected
- 1-2 isolated corruption bands
- Minor impulse noise events
- Occasional RF bursts

**Example Output**: Few horizontal bands of static in otherwise clean image

```bash
python3 generate_sstv_noise.py --input tile.png --output light.png --preset light_qrm
```

---

### `medium_qrm`
**Description**: Moderate RF interference - noticeable corruption  
**Use Case**: Testing typical weak-signal conditions  
**Characteristics**:
- 10% of scan lines affected
- 2-3 corruption bands scattered through image
- Ionospheric fading (amplitude modulation)
- Narrow-band tonal interference
- 1-2 burst events

**Example Output**: Multiple scattered bands of noise, image partially degraded

```bash
python3 generate_sstv_noise.py --input tile.png --output medium.png --preset medium_qrm
```

---

### `heavy_qrm`
**Description**: Heavy RF interference - severe degradation  
**Use Case**: Testing near-unrecoverable signals  
**Characteristics**:
- 20% of scan lines affected
- 3-4 corruption bands
- Ionospheric fading with faster modulation
- 3 separate interference tones
- 2 burst events

**Example Output**: Image significantly obscured with heavy noise bands

```bash
python3 generate_sstv_noise.py --input tile.png --output heavy.png --preset heavy_qrm
```

---

### `extreme_qrm`
**Description**: Extreme RF interference - near-unrecoverable  
**Use Case**: Stress testing stitcher robustness  
**Characteristics**:
- 35% of scan lines corrupted
- 4-5 corruption bands
- Heavy ionospheric fading
- 4 interference frequencies

**Example Output**: Image barely visible through heavy RF noise

```bash
python3 generate_sstv_noise.py --input tile.png --output extreme.png --preset extreme_qrm
```

---

### `fading_only`
**Description**: Ionospheric fading without QRM bursts  
**Use Case**: Testing amplitude modulation handling  
**Characteristics**:
- Minimal white noise
- Fast amplitude modulation (fading flutter)
- No RF bursts or hash line corruption
- Simulates F-layer multi-path propagation

**Example Output**: Image gradually fades darker then brighter as it renders

```bash
python3 generate_sstv_noise.py --input tile.png --output fading.png --preset fading_only
```

---

### `tonal_interference`
**Description**: Narrow-band birdie interference  
**Use Case**: Testing response to specific interference frequencies  
**Characteristics**:
- 3% white noise
- 10% scan line hash corruption
- 3 tonal interference frequencies
- Low impulse noise

**Example Output**: Sinusoidal patterns across image from interference tones

```bash
python3 generate_sstv_noise.py --input tile.png --output tonal.png --preset tonal_interference
```

---

### `burst_noise`
**Description**: Intermittent QRM burst events  
**Use Case**: Testing response to transient interference  
**Characteristics**:
- 5% white noise
- 15% scan line corruption in 4 separate burst regions
- Realistic RF burst pattern
- Localized heavy noise bands

**Example Output**: Several regions with heavy static separated by clean areas

```bash
python3 generate_sstv_noise.py --input tile.png --output bursts.png --preset burst_noise
```

---

### `header_corruption`
**Description**: Noise concentrated on tile header region  
**Use Case**: Testing header recovery mechanisms  
**Characteristics**:
- Heavy focus on first 10 scan lines (header region)
- 3x noise boost in header
- 25% hash line corruption
- 5 impulse events

**Example Output**: Heavy noise in first ~10 lines, cleaner below

```bash
python3 generate_sstv_noise.py --input tile.png --output header.png --preset header_corruption
```

---

## Command Line Usage

### Single File Processing

```bash
# Use preset
python3 generate_sstv_noise.py --input tile.png --output noisy.png --preset light_qrm

# Custom parameters
python3 generate_sstv_noise.py --input tile.png --output noisy.png \
  --white-noise 0.10 --hash-lines 0.15 --impulses 3 --noise-level 0.2

# With reproducible seed
python3 generate_sstv_noise.py --input tile.png --output noisy.png \
  --preset medium_qrm --seed 42
```

### Batch Processing

```bash
# Process entire directory
python3 generate_sstv_noise.py --dir tiles/ --output-dir noisy_tiles/ --preset heavy_qrm

# Verbose output
python3 generate_sstv_noise.py --dir tiles/ --output-dir noisy/ --preset light_qrm --verbose
```

### List Available Presets

```bash
python3 generate_sstv_noise.py --list-presets
```

## Custom Noise Configuration

Fine-tune noise parameters for specific scenarios:

```bash
python3 generate_sstv_noise.py --input tile.png --output custom.png \
  --noise-level 0.20 \
  --white-noise 0.12 \
  --hash-lines 0.18 \
  --hash-width 2 \
  --impulses 4 \
  --impulse-size 3 \
  --fading \
  --fading-frequency 0.7 \
  --bursts 2 \
  --burst-width 3 \
  --header-boost 1.5
```

### Parameter Reference

| Parameter | Range | Description |
|-----------|-------|-------------|
| `--noise-level` | 0.0-1.0 | Overall amplitude of all noise types |
| `--white-noise` | 0.0-1.0 | Intensity of Gaussian white noise bands |
| `--hash-lines` | 0.0-1.0 | Proportion of scan lines to corrupt |
| `--hash-width` | 1-20 | Height of corrupted bands in pixels |
| `--impulses` | 0-20 | Number of RF burst events |
| `--impulse-size` | 1-10 | Height of each burst band |
| `--fading` | boolean | Enable ionospheric fading |
| `--fading-frequency` | 0.1-3.0 | Modulation frequency (cycles per image) |
| `--bursts` | 0-10 | Number of separate burst regions |
| `--burst-width` | 1-10 | Height of each burst band |
| `--header-boost` | 1.0-3.0 | Noise amplification in header region |

## Integration with Robustness Testing

The stitcher robustness test suite uses the noise generator to automatically create test data:

```bash
# Run robustness tests with fresh noise generation
python3 test_stitcher_robustness.py --regen-noise

# Test specific presets
python3 test_stitcher_robustness.py --preset light_qrm medium_qrm heavy_qrm

# Save detailed JSON report
python3 test_stitcher_robustness.py --report results.json
```

## Real-World Map

| Preset | Real Condition |
|--------|---|
| `clean` | Clear HF band, excellent propagation |
| `light_qrm` | Single weak interferer, otherwise clean |
| `medium_qrm` | Typical marginal conditions (S3-S5 signal) |
| `heavy_qrm` | Crowded band with multiple interferers (S1-S3 signal) |
| `extreme_qrm` | Emergency/contest conditions, severe QRM |
| `fading_only` | Long-distance propagation, auroral conditions |
| `tonal_interference` | PLC line noise, switching supply harmonics |
| `burst_noise` | CW/FSK burst interference, solar noise |

## Performance Notes

- **File Size**: Noisy tiles typically 10-15% larger than clean (more entropy)
- **Generation Time**: ~100-300ms per 320×240 tile
- **Memory**: ~5MB per tile during processing
- **Deterministic**: Use `--seed` for reproducible results

## Limitations & Future Work

- Tonal interference approximated as sinusoidal modulation (not full frequency analysis)
- No simulation of image rotation/aspect artifacts
- Phase coherence between scan lines not simulated
- No multipath/ghost image simulation

