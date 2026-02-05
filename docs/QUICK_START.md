# PiSSTVpp2 Quick Start Guide

**Get started with PiSSTVpp2 in 5 minutes**

---

## Installation (3 minutes)

### Step 1: Install Dependencies

**Debian/Ubuntu/Raspberry Pi OS:**
```bash
sudo apt-get update
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev
```

**macOS (with Homebrew):**
```bash
brew install vips libogg libvorbis
```

### Step 2: Download and Build

```bash
# Clone or download repository
cd /path/to/PiSSTVpp2

# Build
make clean && make all

# Verify
./bin/pisstvpp2 -h
```

✅ **Success!** You should see the help message.

---

## First Transmission (2 minutes)

### Prepare Your Image

Any image format works (PNG, JPEG, GIF, BMP), but for best results:
- **Aspect ratio**: 4:3 (e.g., 800x600, 1024x768)
- **Size**: Any size (will be scaled automatically)
- **Content**: High contrast works best

### Encode Your First SSTV Signal

```bash
# Basic encoding (Martin 1 mode, WAV output)
./bin/pisstvpp2 -i your-photo.jpg -o first-transmission.wav
```

**What happens:**
1. Image is loaded and scaled to 320×256
2. Aspect ratio corrected (center-crop by default)
3. VIS header added (identifies mode to receivers)
4. Image encoded line-by-line to audio tones
5. WAV file created: `first-transmission.wav`

### Play It Back

```bash
# Linux/Raspberry Pi
aplay first-transmission.wav

# macOS
afplay first-transmission.wav
```

You should hear:
1. Leader tone (1900 Hz, ~300ms)
2. VIS code (clicking sounds, ~640ms)
3. Image data (varying tones, ~114 seconds for Martin 1)

---

## Understanding Modes

### Quick Mode Guide

| Mode | Speed | Quality | When to Use |
|------|-------|---------|-------------|
| **r36** | 36s | Good | Quick contacts, testing |
| **m2** | 58s | Good | Faster Martin mode |
| **s2** | 71s | Very Good | **Best balance** ⭐ |
| **m1** | 114s | Very Good | **Most compatible** ⭐ |
| **r72** | 72s | Excellent | High quality, different colors |
| **s1** | 110s | Excellent | High quality RGB |
| **sdx** | 269s | Best | Maximum quality (slow) |

### Try Different Modes

```bash
# Fast transmission (Robot 36, 36 seconds)
./bin/pisstvpp2 -i photo.jpg -p r36 -o quick.wav

# Best balance (Scottie 2, 71 seconds)
./bin/pisstvpp2 -i photo.jpg -p s2 -o balanced.wav

# Highest quality (Robot 72, 72 seconds)
./bin/pisstvpp2 -i photo.jpg -p r72 -o hq.wav
```

---

## Common Tasks

### Add Your Callsign (CW Identification)

```bash
# Add CW signature with your callsign
./bin/pisstvpp2 -i photo.jpg -C "N0CALL" -o transmission.wav
```

**What you'll hear:**
- SSTV image transmission
- Then: "SSTV DE N0CALL" in Morse code

**Customize CW:**
```bash
# Faster CW (25 WPM)
./bin/pisstvpp2 -i photo.jpg -C "W1AW" -W 25 -o output.wav

# Different tone (1000 Hz)
./bin/pisstvpp2 -i photo.jpg -C "K0ABC" -T 1000 -o output.wav
```

### Handle Different Image Shapes

```bash
# Portrait photo (will add black bars on sides)
./bin/pisstvpp2 -i portrait.jpg -a pad -o output.wav

# Square photo (will crop to 4:3)
./bin/pisstvpp2 -i square.jpg -a center -o output.wav

# Any photo (will stretch to fit - may distort)
./bin/pisstvpp2 -i any.jpg -a stretch -o output.wav
```

**Aspect modes explained:**
- **center**: Crop to 4:3 from center (default, no distortion)
- **pad**: Add black bars to preserve aspect (no distortion, bars visible)
- **stretch**: Force to 4:3 (may distort image)

### Change Audio Format

```bash
# OGG format (smaller files, good for storage)
./bin/pisstvpp2 -i photo.jpg -f ogg -o transmission.ogg

# AIFF format (macOS/Apple compatible)
./bin/pisstvpp2 -i photo.jpg -f aiff -o transmission.aiff

# WAV format (default, most compatible)
./bin/pisstvpp2 -i photo.jpg -f wav -o transmission.wav
```

### Adjust Quality (Sample Rate)

```bash
# Lower quality, smaller file (11025 Hz)
./bin/pisstvpp2 -i photo.jpg -r 11025 -o small.wav

# Standard quality (22050 Hz, default)
./bin/pisstvpp2 -i photo.jpg -r 22050 -o standard.wav

# High quality (44100 Hz, CD quality)
./bin/pisstvpp2 -i photo.jpg -r 44100 -o high.wav

# Maximum quality (48000 Hz)
./bin/pisstvpp2 -i photo.jpg -r 48000 -o max.wav
```

**Recommendation**: Use 22050 Hz (default) unless you need smaller files (11025) or maximum quality (44100+).

---

## Complete Example

Let's create a high-quality transmission with all the features:

```bash
./bin/pisstvpp2 \
  -i my-photo.jpg \
  -o contest-transmission.wav \
  -p s2 \
  -f wav \
  -r 44100 \
  -a center \
  -C "N0CALL" \
  -W 18 \
  -T 850 \
  -v
```

**This command:**
- Loads `my-photo.jpg`
- Outputs to `contest-transmission.wav`
- Uses Scottie 2 mode (good quality/speed)
- WAV format
- 44.1 kHz sample rate (high quality)
- Center-crop aspect correction
- Adds CW signature "N0CALL"
- CW speed: 18 WPM
- CW tone: 850 Hz
- Verbose output (shows progress)

---

## Troubleshooting

### "Cannot find image file"
```bash
# Use absolute path
./bin/pisstvpp2 -i /full/path/to/photo.jpg -o output.wav

# Or navigate to image directory
cd /path/to/images
/path/to/PiSSTVpp2/bin/pisstvpp2 -i photo.jpg -o output.wav
```

### "OGG format not supported"
```bash
# OGG libraries not installed, use WAV instead
./bin/pisstvpp2 -i photo.jpg -f wav -o output.wav

# Or install OGG support
sudo apt-get install libogg-dev libvorbis-dev
make clean && make all
```

### Output file is huge
```bash
# Use lower sample rate
./bin/pisstvpp2 -i photo.jpg -r 11025 -o output.wav

# Or use OGG compression
./bin/pisstvpp2 -i photo.jpg -f ogg -o output.ogg
```

### Image looks slanted/distorted when decoded
```bash
# Check image aspect ratio (should be 4:3)
# Use pad mode to avoid cropping
./bin/pisstvpp2 -i photo.jpg -a pad -o output.wav

# Try different modes (Robot modes use 4:3 aspect on 320x240)
./bin/pisstvpp2 -i photo.jpg -p r72 -o output.wav
```

---

## Next Steps

Now that you've created your first SSTV transmission:

1. **Decode it**: Use QSSTV, MMSSTV, or another SSTV decoder to verify your signal
2. **Experiment**: Try different modes and settings
3. **Transmit**: Connect to your radio and send it over the air!
4. **Learn more**: Read the [User Guide](USER_GUIDE.md) for advanced features

---

## Quick Reference

### Most Common Commands

```bash
# Standard transmission
./bin/pisstvpp2 -i photo.jpg -p s2 -o output.wav

# With callsign
./bin/pisstvpp2 -i photo.jpg -p s2 -C "N0CALL" -o output.wav

# High quality
./bin/pisstvpp2 -i photo.jpg -p r72 -r 44100 -o output.wav

# Quick transmission
./bin/pisstvpp2 -i photo.jpg -p r36 -o output.wav
```

### Mode Cheat Sheet

- `m1` = Martin 1 (114s, most compatible)
- `s2` = Scottie 2 (71s, good balance)
- `r36` = Robot 36 (36s, fast)
- `r72` = Robot 72 (72s, high quality)
- `sdx` = Scottie DX (269s, best quality)

---

## Getting Help

- **Help command**: `./bin/pisstvpp2 -h`
- **Full documentation**: [User Guide](USER_GUIDE.md)
- **Mode details**: [Mode Reference](MODE_REFERENCE.md)
- **Issues**: File a [GitHub Issue](https://github.com/yourusername/PiSSTVpp2/issues)

---

**73!** 📡

*You're now ready to encode SSTV transmissions with PiSSTVpp2!*
