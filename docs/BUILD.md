# PiSSTVpp2 Build Guide

**Complete instructions for building PiSSTVpp2 from source**

---

## Table of Contents

1. [Quick Build](#quick-build)
2. [Platform-Specific Instructions](#platform-specific-instructions)
3. [Dependency Details](#dependency-details)
4. [Build Options](#build-options)
5. [Troubleshooting](#troubleshooting)
6. [Advanced Build Topics](#advanced-build-topics)

---

## Quick Build

### For the Impatient

```bash
# Install dependencies (Debian/Ubuntu/Raspberry Pi)
sudo apt-get update
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev

# Build
cd /path/to/PiSSTVpp2
make clean && make all

# Test
./bin/pisstvpp2 -h
```

Done! Skip to [usage](USER_GUIDE.md) if everything worked.

---

## Platform-Specific Instructions

### Debian / Ubuntu / Raspberry Pi OS

**Supported Versions:**
- Debian 10+ (Buster and newer)
- Ubuntu 18.04+ (Bionic and newer)
- Raspberry Pi OS (any recent version)

#### Step 1: Update Package Lists

```bash
sudo apt-get update
```

#### Step 2: Install Build Tools

```bash
sudo apt-get install build-essential
```

**Includes:**
- gcc (C compiler)
- make (build automation)
- libc6-dev (C standard library headers)

#### Step 3: Install Required Dependencies

```bash
sudo apt-get install libvips-dev
```

**libvips** is required for image processing.

#### Step 4: Install Optional Dependencies

```bash
sudo apt-get install libogg-dev libvorbis-dev
```

**OGG support** is optional but recommended for compressed audio output.

**Note:** If you skip this step, PiSSTVpp2 will build without OGG support (WAV and AIFF only).

#### Step 5: Build

```bash
cd /path/to/PiSSTVpp2
make clean
make all
```

**Expected Output:**
```
gcc -std=c11 -Wall -Wextra -O2 ... -c src/pisstvpp2.c -o src/pisstvpp2.o
gcc -std=c11 -Wall -Wextra -O2 ... -c src/pisstvpp2_image.c -o src/pisstvpp2_image.o
...
gcc src/pisstvpp2.o ... -o bin/pisstvpp2 ...
```

#### Step 6: Verify

```bash
./bin/pisstvpp2 -h
```

Should display help message.

#### Step 7: Optional System-Wide Install

```bash
sudo cp bin/pisstvpp2 /usr/local/bin/
```

Now you can run `pisstvpp2` from anywhere.

---

### Fedora / RHEL / CentOS

**Supported Versions:**
- Fedora 30+
- RHEL 8+
- CentOS 8+

#### Step 1: Install Build Tools

```bash
sudo dnf groupinstall "Development Tools"
```

Or minimal:
```bash
sudo dnf install gcc make
```

#### Step 2: Install Required Dependencies

```bash
sudo dnf install vips-devel
```

#### Step 3: Install Optional Dependencies

```bash
sudo dnf install libogg-devel libvorbis-devel
```

#### Step 4: Build

```bash
cd /path/to/PiSSTVpp2
make clean && make all
```

#### Step 5: Verify

```bash
./bin/pisstvpp2 -h
```

---

### macOS

**Supported Versions:**
- macOS 10.12+ (Sierra and newer)
- Works on Intel and Apple Silicon (M1/M2)

#### Option A: Using Homebrew (Recommended)

**Step 1: Install Homebrew** (if not already installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**Step 2: Install Xcode Command Line Tools**

```bash
xcode-select --install
```

**Step 3: Install Dependencies**

```bash
brew install vips libogg libvorbis
```

**Step 4: Build**

```bash
cd /path/to/PiSSTVpp2
make clean && make all
```

**Step 5: Verify**

```bash
./bin/pisstvpp2 -h
```

#### Option B: Using MacPorts

**Step 1: Install MacPorts** (if not already installed)

Download from [macports.org](https://www.macports.org/)

**Step 2: Install Dependencies**

```bash
sudo port install vips libogg libvorbis
```

**Step 3: Build**

```bash
cd /path/to/PiSSTVpp2
make clean && make all
```

#### macOS Notes

**Compiler:**
- macOS uses Clang instead of GCC
- Clang is compatible (supports C11)
- Makefile works with both

**Library Paths:**
- Homebrew: `/opt/homebrew` (Apple Silicon) or `/usr/local` (Intel)
- MacPorts: `/opt/local`
- `pkg-config` handles paths automatically

**Apple Silicon:**
- Native ARM64 builds work
- Rosetta not required

---

### Raspberry Pi Specific

**All Raspberry Pi Models Supported:**
- Pi Zero / Zero W
- Pi 1 Model B/B+
- Pi 2 Model B
- Pi 3 Model B/B+
- Pi 4 Model B
- Pi 400
- Pi 5

#### Pi Zero / Pi 1 (Single Core)

**Build Time:**
- ~30-60 seconds (slower CPU)

**Memory:**
- 512 MB RAM sufficient
- No swap needed for build

**Instructions:**
Same as Debian/Ubuntu above.

**Performance:**
- Encoding: 3-8 seconds per image
- Perfectly usable

#### Pi 2 / Pi 3

**Build Time:**
- ~10-20 seconds

**Performance:**
- Encoding: 1-3 seconds per image
- Excellent performance

**64-bit OS:**
Pi 3 can run 64-bit Raspberry Pi OS. Both 32-bit and 64-bit work fine.

#### Pi 4 / Pi 5

**Build Time:**
- ~5-10 seconds (Pi 4)
- ~3-5 seconds (Pi 5)

**Performance:**
- Encoding: <1 second per image
- Desktop-class performance

**8GB Models:**
Excess RAM doesn't help (PiSSTVpp2 uses <20 MB), but useful for multitasking.

#### Raspberry Pi OS Lite

If using headless/lite OS:

```bash
# Ensure pkg-config installed
sudo apt-get install pkg-config

# Then proceed with normal build
```

---

### Other UNIX-like Systems

#### FreeBSD

```bash
# Install dependencies
pkg install vips libogg libvorbis

# Build
cd /path/to/PiSSTVpp2
gmake clean && gmake all
```

Note: Use `gmake` (GNU Make) instead of `make`.

#### OpenBSD

```bash
# Install dependencies
pkg_add vips libogg libvorbis

# Build
cd /path/to/PiSSTVpp2
make clean && make all
```

#### Solaris / illumos

Likely works with appropriate dependencies, but untested.

---

### Windows

**Option A: WSL (Windows Subsystem for Linux) - Recommended**

1. Enable WSL:
   ```powershell
   wsl --install
   ```

2. Install Ubuntu from Microsoft Store

3. Inside WSL, follow [Debian/Ubuntu instructions](#debian--ubuntu--raspberry-pi-os)

**Option B: Cygwin**

1. Install Cygwin with packages:
   - gcc-core
   - make
   - libvips-devel
   - libogg-devel
   - libvorbis-devel

2. Build in Cygwin terminal:
   ```bash
   cd /path/to/PiSSTVpp2
   make clean && make all
   ```

**Option C: MinGW**

Possible but not tested. Similar to Cygwin approach.

**Note:** Native Windows (MSVC) not currently supported.

---

## Dependency Details

### libvips (Required)

**What it is:**
Fast image processing library

**Why required:**
- Load images (JPEG, PNG, GIF, etc.)
- Color space conversion
- Resize/crop operations
- Aspect ratio correction

**Version:**
- Minimum: 8.0
- Recommended: 8.10+
- Latest: Any version works

**Install:**
```bash
# Debian/Ubuntu
sudo apt-get install libvips-dev

# Fedora
sudo dnf install vips-devel

# macOS
brew install vips
```

**Check installed version:**
```bash
pkg-config --modversion vips
```

**Manual build** (if not in package manager):
```bash
wget https://github.com/libvips/libvips/releases/download/v8.14.2/vips-8.14.2.tar.gz
tar xzf vips-8.14.2.tar.gz
cd vips-8.14.2
./configure
make
sudo make install
```

### libogg + libvorbis (Optional)

**What they are:**
- libogg: OGG container format
- libvorbis: Vorbis audio codec

**Why optional:**
- Only needed for OGG output format
- WAV and AIFF work without it

**Benefits:**
- Smaller file sizes (60-80% reduction)
- Good quality lossy compression

**Install:**
```bash
# Debian/Ubuntu
sudo apt-get install libogg-dev libvorbis-dev

# Fedora
sudo dnf install libogg-devel libvorbis-devel

# macOS
brew install libogg libvorbis
```

**Check if installed:**
```bash
pkg-config --exists vorbisenc && echo "OGG support available"
```

**Building without OGG:**
If OGG libraries not installed, build proceeds without error:
```bash
make clean && make all
# Works fine, just no -f ogg option
```

**Verify OGG support in binary:**
```bash
./bin/pisstvpp2 -h | grep -i format
# Should list: wav, aiff, ogg (if compiled in)
```

---

## Build Options

### Build Flags

#### Standard Build (Optimized)

```bash
make clean && make all
```

**Default flags:**
- `-std=c11`: C11 standard
- `-Wall -Wextra`: All warnings enabled
- `-O2`: Optimization level 2 (fast)

#### Debug Build

```bash
make CFLAGS="-std=c11 -Wall -Wextra -g -O0" clean all
```

**Debug flags:**
- `-g`: Include debug symbols
- `-O0`: No optimization (easier debugging)

**Use with:**
```bash
gdb ./bin/pisstvpp2
```

#### Maximum Optimization

```bash
make CFLAGS="-std=c11 -Wall -Wextra -O3 -march=native" clean all
```

**Flags:**
- `-O3`: Maximum optimization
- `-march=native`: Optimize for current CPU

**Warning:** May not be portable to other CPUs.

#### Static Linking

```bash
make LDFLAGS="-static" clean all
```

**Creates:**
- Fully static binary (no shared library dependencies)
- Larger binary size
- Portable to systems without libraries installed

**Note:** May not work on all platforms (macOS doesn't support static linking easily).

### Conditional Features

#### Force OGG Support Off

```bash
make CFLAGS="-std=c11 -Wall -Wextra -O2" clean all
# (don't define -DHAVE_OGG)
```

Or edit Makefile to remove OGG detection.

#### Custom Installation Prefix

```bash
make clean all
sudo make install PREFIX=/opt/pisstvpp2
```

(Requires adding install target to Makefile)

---

## Troubleshooting

### Common Build Errors

#### "vips/vips.h: No such file or directory"

**Problem:** libvips not installed or not found

**Solution:**
```bash
# Install libvips
sudo apt-get install libvips-dev

# Or check pkg-config
pkg-config --cflags vips
```

If pkg-config fails:
```bash
# Ensure pkg-config installed
sudo apt-get install pkg-config
```

#### "vorbis/vorbisenc.h: No such file or directory"

**Problem:** OGG libraries not installed

**Solution A:** Install OGG support
```bash
sudo apt-get install libogg-dev libvorbis-dev
make clean && make all
```

**Solution B:** Build without OGG
```bash
# Remove OGG source file from Makefile
# Edit Makefile: remove audio_encoder_ogg.c from SRCS
make clean && make all
```

#### "cannot find -lvips"

**Problem:** libvips library not in linker path

**Solution:**
```bash
# Check library location
pkg-config --libs vips

# If not found, ensure libvips installed
sudo apt-get install libvips-dev

# Or set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

#### "make: command not found"

**Problem:** GNU Make not installed

**Solution:**
```bash
# Debian/Ubuntu
sudo apt-get install make

# macOS
xcode-select --install
```

#### "gcc: command not found"

**Problem:** C compiler not installed

**Solution:**
```bash
# Debian/Ubuntu
sudo apt-get install build-essential

# macOS
xcode-select --install

# Fedora
sudo dnf groupinstall "Development Tools"
```

#### Warnings during build

**Common warnings:**
```
warning: unused parameter 'argc' [-Wunused-parameter]
```

**Solution:** Warnings are generally harmless. Build succeeds despite them.

To suppress warnings:
```bash
make CFLAGS="-std=c11 -O2" clean all
# (remove -Wall -Wextra)
```

#### Link errors on macOS

**Problem:** Architecture mismatch (x86_64 vs arm64)

**Solution:**
```bash
# Check architecture
uname -m
# arm64 = Apple Silicon, x86_64 = Intel

# Ensure Homebrew for correct architecture
# Apple Silicon: /opt/homebrew/bin/brew
# Intel: /usr/local/bin/brew

# Reinstall dependencies
brew reinstall vips libogg libvorbis
```

### Runtime Errors

#### "error while loading shared libraries: libvips.so.42"

**Problem:** libvips library not found at runtime

**Solution:**
```bash
# Add library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Or update cache
sudo ldconfig
```

**Permanent fix:**
```bash
# Add to ~/.bashrc or ~/.profile
echo 'export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

#### "VIPS-WARNING: unable to load library 'gobject-2.0'"

**Problem:** GLib dependency missing

**Solution:**
```bash
# Debian/Ubuntu
sudo apt-get install libglib2.0-dev

# Fedora
sudo dnf install glib2-devel
```

#### Segmentation fault on startup

**Problem:** Library version mismatch

**Solution:**
```bash
# Rebuild with verbose linking
make clean
make V=1 all

# Check linked libraries
ldd bin/pisstvpp2

# Reinstall dependencies
sudo apt-get install --reinstall libvips-dev
make clean && make all
```

---

## Advanced Build Topics

### Cross-Compilation

#### For Raspberry Pi (from x86_64 Linux)

**Step 1: Install cross-compiler**

```bash
sudo apt-get install gcc-arm-linux-gnueabihf
```

**Step 2: Cross-compile dependencies**

This is complex. Easier to build on Pi directly or use qemu.

**Step 3: Cross-compile PiSSTVpp2**

```bash
make CC=arm-linux-gnueabihf-gcc clean all
```

**Note:** Cross-compiling dependencies (libvips, etc.) is difficult. Recommend native compilation on target.

#### Using Qemu (Emulation)

```bash
# Install qemu
sudo apt-get install qemu-user-static

# Run ARM binary on x86
qemu-arm-static bin/pisstvpp2 -h
```

### Custom Makefile Modifications

#### Add Install Target

Edit `Makefile`, add:

```makefile
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 bin/pisstvpp2 $(DESTDIR)$(BINDIR)/

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/pisstvpp2

.PHONY: install uninstall
```

**Usage:**
```bash
make clean all
sudo make install

# Custom prefix
sudo make install PREFIX=/opt/local
```

#### Add Debug Target

```makefile
debug: CFLAGS = -std=c11 -Wall -Wextra -g -O0 -DDEBUG
debug: clean all

.PHONY: debug
```

**Usage:**
```bash
make debug
gdb ./bin/pisstvpp2
```

#### Add Test Target

```makefile
test: all
	cd tests && ./test_suite.sh

.PHONY: test
```

**Usage:**
```bash
make test
```

### Building from Git

```bash
# Clone repository
git clone https://github.com/yourusername/PiSSTVpp2.git
cd PiSSTVpp2

# Checkout specific version
git checkout v2.0

# Build
make clean && make all
```

### Building Specific Components Only

**Build only object files:**
```bash
make src/pisstvpp2.o src/pisstvpp2_image.o
```

**Build specific module:**
```bash
make src/pisstvpp2_sstv.o
```

**Clean specific files:**
```bash
rm src/pisstvpp2.o
make src/pisstvpp2.o
```

### Parallel Build

```bash
# Use multiple CPU cores
make -j$(nproc) all

# Or specify core count
make -j4 all
```

**Note:** PiSSTVpp2 builds fast anyway (<10 seconds), parallel build marginal benefit.

### Verbose Build Output

```bash
# See exact commands
make V=1 all

# Or
make --debug all
```

---

## Verification

### Post-Build Checks

#### 1. Binary Exists

```bash
ls -lh bin/pisstvpp2
# Should show executable file, ~100-200 KB
```

#### 2. Help Runs

```bash
./bin/pisstvpp2 -h
# Should display help text without errors
```

#### 3. Version Info

```bash
./bin/pisstvpp2 --version
# Should show version number
```

#### 4. Dependencies Linked

```bash
ldd bin/pisstvpp2
# Should show libvips, libogg, libvorbis (if compiled in)
```

**Expected output:**
```
linux-vdso.so.1 (0x00007ffc...)
libvips.so.42 => /usr/lib/x86_64-linux-gnu/libvips.so.42
libvorbisenc.so.2 => /usr/lib/x86_64-linux-gnu/libvorbisenc.so.2
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6
...
```

#### 5. Quick Test

```bash
# Create test image
convert -size 320x256 xc:blue test_blue.png

# Encode
./bin/pisstvpp2 -i test_blue.png -o test.wav

# Verify output
ls -lh test.wav
file test.wav
# Should show: RIFF (little-endian) data, WAVE audio
```

#### 6. Run Test Suite

```bash
cd tests
./test_suite.sh
# Should show: 55/55 tests passed
```

---

## Build Performance

### Build Times

**Raspberry Pi Zero:**
- Clean build: ~45 seconds

**Raspberry Pi 4:**
- Clean build: ~8 seconds

**Modern x86_64 Laptop:**
- Clean build: ~2 seconds

**Apple M1:**
- Clean build: ~1 second

### Optimization Impact

**Binary Size:**
- Debug (-g -O0): ~450 KB
- Standard (-O2): ~200 KB
- Optimized (-O3): ~180 KB
- Stripped: ~120 KB

**Runtime Performance:**
- `-O0` (debug): 3× slower encoding
- `-O2` (standard): Baseline
- `-O3` (max): ~5-10% faster

**Recommendation:** Use `-O2` (default). `-O3` minimal benefit for typical use.

---

## Getting Help

### Build Issues

1. **Check this guide** for common problems
2. **Read error messages** carefully (often self-explanatory)
3. **Verify dependencies** installed correctly
4. **Try clean build**: `make clean && make all`
5. **Check version compatibility** (OS, libraries)

### Reporting Bugs

If build fails:

1. **Gather information:**
   ```bash
   uname -a
   gcc --version
   pkg-config --modversion vips
   make clean && make V=1 all 2>&1 | tee build.log
   ```

2. **File issue** with:
   - OS and version
   - Compiler version
   - Library versions
   - Complete build log
   - Steps to reproduce

### Resources

- **User Guide**: [docs/USER_GUIDE.md](USER_GUIDE.md)
- **Architecture**: [docs/ARCHITECTURE.md](ARCHITECTURE.md)
- **Testing**: [docs/TEST_SUITE_README.md](TEST_SUITE_README.md)

---

## Summary

**Quick Build:**
```bash
# Dependencies
sudo apt-get install build-essential libvips-dev libogg-dev libvorbis-dev

# Build
cd PiSSTVpp2
make clean && make all

# Test
./bin/pisstvpp2 -i test.jpg -o test.wav
```

**That's it!** 🎉

---

**73!** 📡

*End of Build Guide*
