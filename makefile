
# Portable Makefile for PiSSTVpp (Raspberry Pi, Linux, macOS)
# Usage: make [all|debug|clean]

PKG_CONFIG ?= pkg-config
CC ?= gcc

# Platform detection
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)


# Common flags
CFLAGS_COMMON = -O3 -ffast-math -funroll-loops -Wall -Wextra -Wpedantic -std=c11 -D_DEFAULT_SOURCE

# Required dependency: libvips (check first)
HAVE_VIPS = $(shell $(PKG_CONFIG) --exists vips glib-2.0 gobject-2.0 && echo 1 || echo 0)

ifeq ($(HAVE_VIPS),0)
    $(error libvips-dev not found. Install with: sudo apt install libvips-dev (Linux) or brew install vips (macOS))
endif

# Use pkg-config to get all necessary cflags and libs for vips, glib-2.0, and gobject-2.0
CFLAGS_PKG   = $(shell $(PKG_CONFIG) --cflags vips glib-2.0 gobject-2.0)
LDFLAGS_PKG  = $(shell $(PKG_CONFIG) --libs vips glib-2.0 gobject-2.0)

# Optional codec support (OGG Vorbis)
HAVE_OGG_SUPPORT  = $(shell $(PKG_CONFIG) --exists vorbis vorbisenc ogg && echo 1 || echo 0)

ifeq ($(HAVE_OGG_SUPPORT),1)
    CFLAGS_COMMON += -DHAVE_OGG_SUPPORT
    CFLAGS_PKG    += $(shell $(PKG_CONFIG) --cflags vorbis vorbisenc ogg)
    LDFLAGS_PKG   += $(shell $(PKG_CONFIG) --libs vorbis vorbisenc ogg)
endif

# MMSSTV library detection (runtime feature, build-time reporting)
# Detection order: MMSSTV_LIB_PATH env var -> pkg-config -> standard paths
MMSSTV_LIB_DETECTED = 0
MMSSTV_LIB_PATH_FOUND =

# 1. Check environment variable
ifdef MMSSTV_LIB_PATH
    ifneq ($(wildcard $(MMSSTV_LIB_PATH)),)
        MMSSTV_LIB_DETECTED = 1
        MMSSTV_LIB_PATH_FOUND = $(MMSSTV_LIB_PATH)
    endif
endif

# 2. Try pkg-config if not found
ifeq ($(MMSSTV_LIB_DETECTED),0)
    MMSSTV_PKG_CHECK = $(shell $(PKG_CONFIG) --exists mmsstv-portable 2>/dev/null && echo 1 || echo 0)
    ifeq ($(MMSSTV_PKG_CHECK),1)
        MMSSTV_LIB_DETECTED = 1
        MMSSTV_LIB_PATH_FOUND = $(shell $(PKG_CONFIG) --variable=libdir mmsstv-portable)/libsstv_encoder.so
    endif
endif

# 3. Search standard library paths if not found
ifeq ($(MMSSTV_LIB_DETECTED),0)
    # macOS paths
    ifeq ($(UNAME_S),Darwin)
        MMSSTV_SEARCH_PATHS = /usr/local/lib/libsstv_encoder.dylib \
                              /opt/homebrew/lib/libsstv_encoder.dylib \
                              $(HOME)/lib/libsstv_encoder.dylib
    else
        # Linux paths
        MMSSTV_SEARCH_PATHS = /usr/lib/libsstv_encoder.so \
                              /usr/local/lib/libsstv_encoder.so \
                              /opt/mmsstv/lib/libsstv_encoder.so \
                              $(HOME)/lib/libsstv_encoder.so
    endif
    
    MMSSTV_LIB_PATH_FOUND = $(firstword $(wildcard $(MMSSTV_SEARCH_PATHS)))
    ifneq ($(MMSSTV_LIB_PATH_FOUND),)
        MMSSTV_LIB_DETECTED = 1
    endif
endif

# Raspberry Pi/ARM optimizations
ifeq ($(UNAME_M),armv7l)
    CFLAGS_PI = -march=armv7-a -mfpu=neon -mfloat-abi=hard
else
    CFLAGS_PI =
endif

# macOS: Homebrew may install vips/glib in /opt/homebrew
ifeq ($(UNAME_S),Darwin)
    GLIB_VER := $(shell ls -d /opt/homebrew/Cellar/glib/[0-9]* 2>/dev/null | tail -1)
    CFLAGS_PKG += -I/opt/homebrew/include
    ifneq ($(GLIB_VER),)
        CFLAGS_PKG += -I$(GLIB_VER)/include/glib-2.0 -I$(GLIB_VER)/lib/glib-2.0/include
        LDFLAGS_PKG += -L$(GLIB_VER)/lib
    endif
    LDFLAGS_PKG += -L/opt/homebrew/lib
endif

# Directory structure
SRC_DIR = src
INC_DIR = src/include
UTIL_DIR = $(SRC_DIR)/util
IMG_DIR = $(SRC_DIR)/image
SSTV_DIR = $(SRC_DIR)/sstv
MMSSTV_DIR = $(SRC_DIR)/mmsstv
BIN_DIR = bin
TEST_DIR = tests

# Final flags (user can override CFLAGS/LDFLAGS if needed)
CFLAGS ?= $(CFLAGS_COMMON) $(CFLAGS_PKG) $(CFLAGS_PI) -I$(SRC_DIR) -I$(INC_DIR) -I$(UTIL_DIR) -I$(IMG_DIR) -I$(SSTV_DIR) -I$(MMSSTV_DIR)
LDFLAGS ?= $(LDFLAGS_PKG) -lm -ldl

# Target configuration
TARGET = $(BIN_DIR)/slowframe
TARGET_LIBGD = $(BIN_DIR)/slowframe_libgd
TARGET_SAN = $(BIN_DIR)/slowframe_san
TARGET_VIPS_TEST = $(BIN_DIR)/vips_test
SRC_FILES = $(SRC_DIR)/slowframe.c $(SRC_DIR)/slowframe_image.c $(SRC_DIR)/slowframe_sstv.c \
            $(SRC_DIR)/slowframe_audio_encoder.c $(SRC_DIR)/audio_encoder_wav.c \
            $(SRC_DIR)/audio_encoder_aiff.c $(SRC_DIR)/audio_encoder_ogg.c \
            $(SRC_DIR)/slowframe_config.c $(SRC_DIR)/slowframe_context.c $(SRC_DIR)/overlay_spec.c \
            $(IMG_DIR)/image_loader.c $(IMG_DIR)/image_processor.c $(IMG_DIR)/image_aspect.c \
            $(SSTV_DIR)/mode_registry.c $(SSTV_DIR)/modes_martin.c \
            $(SSTV_DIR)/modes_scottie.c $(SSTV_DIR)/modes_robot.c \
            $(MMSSTV_DIR)/mmsstv_loader.c $(MMSSTV_DIR)/mmsstv_adapter.c \
            $(UTIL_DIR)/error.c
OBJ_FILES = $(BIN_DIR)/slowframe.o $(BIN_DIR)/slowframe_image.o $(BIN_DIR)/slowframe_sstv.o \
            $(BIN_DIR)/slowframe_audio_encoder.o $(BIN_DIR)/audio_encoder_wav.o \
            $(BIN_DIR)/audio_encoder_aiff.o $(BIN_DIR)/audio_encoder_ogg.o \
            $(BIN_DIR)/slowframe_config.o $(BIN_DIR)/slowframe_context.o $(BIN_DIR)/overlay_spec.o \
            $(BIN_DIR)/image_loader.o $(BIN_DIR)/image_processor.o $(BIN_DIR)/image_aspect.o \
            $(BIN_DIR)/mode_registry.o $(BIN_DIR)/modes_martin.o \
            $(BIN_DIR)/modes_scottie.o $(BIN_DIR)/modes_robot.o \
            $(BIN_DIR)/mmsstv_loader.o $(BIN_DIR)/mmsstv_adapter.o \
            $(BIN_DIR)/error.o

# ANSI color codes for output
COLOR_RESET   = \033[0m
COLOR_BOLD    = \033[1m
COLOR_GREEN   = \033[32m
COLOR_YELLOW  = \033[33m
COLOR_CYAN    = \033[36m
COLOR_RED     = \033[31m

# Check mark symbols
CHECK_OK   = ✓
CHECK_NONE = ✗

all: build-info $(TARGET) build-success

build-info:
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - Build Configuration$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "Platform:    $(UNAME_S) ($(UNAME_M))"
	@echo "Compiler:    $(CC)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "Dependencies:"
ifeq ($(HAVE_VIPS),1)
	@echo "  $(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) libvips      - Required (image processing)"
else
	@echo "  $(COLOR_RED)$(CHECK_NONE)$(COLOR_RESET) libvips      - NOT FOUND (install libvips-dev)"
endif
ifeq ($(HAVE_OGG_SUPPORT),1)
	@echo "  $(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) OGG/Vorbis   - Enabled (compressed audio)"
else
	@echo "  $(COLOR_YELLOW)$(CHECK_NONE)$(COLOR_RESET) OGG/Vorbis   - Not found (WAV/AIFF only)"
endif
ifeq ($(MMSSTV_LIB_DETECTED),1)
	@echo "  $(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) MMSSTV lib   - $(MMSSTV_LIB_PATH_FOUND)"
else
	@echo "  $(COLOR_YELLOW)$(CHECK_NONE)$(COLOR_RESET) MMSSTV lib   - Not found (runtime detection will try)"
endif
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "Features:"
	@echo "  • Native modes:   7 (always available)"
ifeq ($(MMSSTV_LIB_DETECTED),1)
	@echo "  • MMSSTV modes:   43 (library detected - will load at runtime)"
else
	@echo "  • MMSSTV modes:   0-43 (depends on runtime detection)"
endif
ifeq ($(HAVE_OGG_SUPPORT),1)
	@echo "  • Audio formats:  WAV, AIFF, OGG"
else
	@echo "  • Audio formats:  WAV, AIFF"
endif
	@echo "  • Text overlay:   Enabled"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)[BUILD]$(COLOR_RESET) Compiling source files..."
	@echo ""

build-success:
	@echo ""
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)$(COLOR_BOLD)[BUILD] $(CHECK_OK) Build successful!$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@filesize=$$(stat -f%z "$(TARGET)" 2>/dev/null || stat -c%s "$(TARGET)" 2>/dev/null); \
	filesizekb=$$(($$filesize / 1024)); \
	echo "Binary: $(TARGET) ($${filesizekb} KB)"
	@echo "Run:    ./bin/slowframe -h"
ifeq ($(MMSSTV_LIB_DETECTED),0)
	@echo ""
	@echo "$(COLOR_YELLOW)Note:$(COLOR_RESET) MMSSTV library not detected at build time."
	@echo "      SlowFrame will attempt runtime detection."
	@echo "      Set MMSSTV_LIB_PATH to help detection."
endif
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo ""

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(SSTV_DIR)/%.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(MMSSTV_DIR)/%.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(UTIL_DIR)/%.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(IMG_DIR)/%.c
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	@echo "  LINK  $@"
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

debug: CFLAGS += -g -O0 -DDEBUG
debug: build-info-debug $(TARGET) build-success-debug

build-info-debug:
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - DEBUG BUILD$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_YELLOW)[DEBUG]$(COLOR_RESET) Building with debug symbols (-g -O0)"
	@echo ""

build-success-debug:
	@echo ""
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)$(COLOR_BOLD)[DEBUG] $(CHECK_OK) Debug build successful!$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@filesize=$$(stat -f%z "$(TARGET)" 2>/dev/null || stat -c%s "$(TARGET)" 2>/dev/null); \
	filesizekb=$$(($$filesize / 1024)); \
	echo "Binary: $(TARGET) ($${filesizekb} KB with debug symbols)"
	@echo "Debug:  gdb ./bin/slowframe"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo ""

# Verbose build (show full compiler commands)
verbose: MAKEFLAGS =
verbose: all

# Help target - show available build targets
help:
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - Build System Help$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo ""
	@echo "$(COLOR_BOLD)Build Targets:$(COLOR_RESET)"
	@echo "  $(COLOR_GREEN)make [all]$(COLOR_RESET)      - Build SlowFrame (default, optimized)"
	@echo "  $(COLOR_GREEN)make debug$(COLOR_RESET)      - Build with debug symbols (-g -O0)"
	@echo "  $(COLOR_GREEN)make sanitize$(COLOR_RESET)   - Build with AddressSanitizer & UBSan"
	@echo "  $(COLOR_GREEN)make verbose$(COLOR_RESET)    - Build with full compiler output"
	@echo "  $(COLOR_GREEN)make clean$(COLOR_RESET)      - Remove all build artifacts"
	@echo ""
	@echo "$(COLOR_BOLD)Analysis Targets:$(COLOR_RESET)"
	@echo "  $(COLOR_GREEN)make analyze$(COLOR_RESET)    - Run static analysis (clang analyzer)"
	@echo "  $(COLOR_GREEN)make coverage$(COLOR_RESET)   - Build with code coverage support"
	@echo ""
	@echo "$(COLOR_BOLD)Test Targets:$(COLOR_RESET)"
	@echo "  $(COLOR_GREEN)make test$(COLOR_RESET)       - Run Python test suite (default)"
	@echo "  $(COLOR_GREEN)make test-quick$(COLOR_RESET) - Run quick bash tests"
	@echo "  $(COLOR_GREEN)make test-full$(COLOR_RESET)  - Run full test suite (verbose)"
	@echo "  $(COLOR_GREEN)make test-ci$(COLOR_RESET)    - Run tests for CI/CD"
	@echo "  $(COLOR_GREEN)make test-clean$(COLOR_RESET) - Clean test outputs"
	@echo ""
	@echo "$(COLOR_BOLD)Installation:$(COLOR_RESET)"
	@echo "  $(COLOR_GREEN)make install$(COLOR_RESET)    - Install to system (requires sudo)"
	@echo "  $(COLOR_GREEN)make uninstall$(COLOR_RESET)  - Remove from system (requires sudo)"
	@echo ""
	@echo "$(COLOR_BOLD)Environment Variables:$(COLOR_RESET)"
	@echo "  MMSSTV_LIB_PATH     - Path to libsstv_encoder library"
	@echo "  CC                  - C compiler (default: gcc)"
	@echo "  CFLAGS              - Additional compiler flags"
	@echo "  LDFLAGS             - Additional linker flags"
	@echo "  PREFIX              - Installation prefix (default: /usr/local)"
	@echo ""
	@echo "$(COLOR_BOLD)Examples:$(COLOR_RESET)"
	@echo "  # Clean rebuild from scratch"
	@echo "  make clean && make all"
	@echo ""
	@echo "  # Build with MMSSTV library at custom location"
	@echo "  MMSSTV_LIB_PATH=/usr/local/lib/libsstv.so make"
	@echo ""
	@echo "  # Test for memory errors with sanitizers"
	@echo "  make sanitize && ./bin/slowframe -i test.png -o out.wav"
	@echo ""
	@echo "  # Install to custom prefix directory"
	@echo "  sudo make install PREFIX=/usr/local"
	@echo ""
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"

# Sanitize build - AddressSanitizer and UndefinedBehaviorSanitizer
sanitize: CFLAGS += -g -O1 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls
sanitize: LDFLAGS += -fsanitize=address -fsanitize=undefined
sanitize: clean
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - SANITIZE BUILD$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_YELLOW)[SANITIZE]$(COLOR_RESET) Building with memory sanitizers..."
	@echo "  • AddressSanitizer: Detects memory errors"
	@echo "  • UBSanitizer: Detects undefined behavior"
	@echo ""
	@$(MAKE) $(TARGET)
	@echo ""
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)$(COLOR_BOLD)[SANITIZE] $(CHECK_OK) Sanitized build complete!$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "Run tests with: ./bin/slowframe -i test.png -o out.wav"
	@echo "Memory errors will be reported at runtime"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"

# Static analysis using clang analyzer
analyze:
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - STATIC ANALYSIS$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@if command -v scan-build >/dev/null 2>&1; then \
		echo "$(COLOR_YELLOW)[ANALYZE]$(COLOR_RESET) Running clang static analyzer..."; \
		echo ""; \
		scan-build -o analysis-reports --status-bugs make clean all; \
		echo ""; \
		echo "$(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) Analysis complete"; \
		echo "Reports: analysis-reports/"; \
	else \
		echo "$(COLOR_RED)$(CHECK_NONE)$(COLOR_RESET) scan-build not found"; \
		echo "Install: brew install llvm  (macOS)"; \
		echo "         apt-get install clang-tools  (Linux)"; \
	fi
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"

# Code coverage build
coverage: CFLAGS += -g -O0 --coverage -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += --coverage
coverage: clean $(TARGET)
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - COVERAGE BUILD$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) Built with code coverage instrumentation"
	@echo ""
	@echo "Next steps:"
	@echo "  1. Run: make test"
	@echo "  2. Generate report: gcov src/*.c"
	@echo "  3. View: lcov --capture --directory . --output-file coverage.info"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"

# Installation (requires sudo for system directories)
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

install: $(TARGET)
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - INSTALLATION$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "Installing to: $(PREFIX)"
	@mkdir -p $(BINDIR)
	@install -m 0755 $(TARGET) $(BINDIR)/slowframe
	@echo "$(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) Binary installed: $(BINDIR)/slowframe"
	@if [ -f docs/slowframe.1 ]; then \
		mkdir -p $(MANDIR); \
		install -m 0644 docs/slowframe.1 $(MANDIR)/slowframe.1; \
		echo "$(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) Manual installed: $(MANDIR)/slowframe.1"; \
	fi
	@echo ""
	@echo "$(COLOR_GREEN)$(COLOR_BOLD)Installation complete!$(COLOR_RESET)"
	@echo "Run: slowframe -h"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"

# Uninstallation
uninstall:
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@echo "$(COLOR_BOLD)  SlowFrame v2.1.0 - UNINSTALLATION$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"
	@rm -f $(BINDIR)/slowframe
	@echo "$(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) Removed: $(BINDIR)/slowframe"
	@rm -f $(MANDIR)/slowframe.1
	@echo "$(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) Removed: $(MANDIR)/slowframe.1"
	@echo ""
	@echo "$(COLOR_GREEN)$(COLOR_BOLD)Uninstallation complete!$(COLOR_RESET)"
	@echo "$(COLOR_CYAN)═══════════════════════════════════════════════════$(COLOR_RESET)"

# Test targets
test: $(TARGET) test-python

test-python: $(TARGET)
	cd $(TEST_DIR)/util && python3 test_suite.py

test-bash: $(TARGET)
	chmod +x $(TEST_DIR)/util/test_suite_bash.sh
	cd $(TEST_DIR)/util && ./test_suite_bash.sh

test-quick: $(TARGET)
	cd $(TEST_DIR)/util && ./test_suite_bash.sh

test-full: $(TARGET)
	cd $(TEST_DIR)/util && python3 test_suite.py --verbose

test-clean:
	rm -rf $(TEST_DIR)/test_outputs/

# CI/CD test target
test-ci: $(TARGET)
	cd $(TEST_DIR)/util && python3 test_suite.py
	@if [ $$? -eq 0 ]; then echo "All tests passed!"; else echo "Some tests failed!"; exit 1; fi

clean:
	@echo "$(COLOR_YELLOW)[CLEAN]$(COLOR_RESET) Removing build artifacts..."
	@rm -f $(BIN_DIR)/slowframe* $(BIN_DIR)/*.o
	@rm -rf $(BIN_DIR)/*.dSYM
	@echo "$(COLOR_GREEN)$(CHECK_OK)$(COLOR_RESET) Clean complete"

.PHONY: all build-info build-success build-info-debug build-success-debug debug verbose \
        help sanitize analyze coverage install uninstall \
        test test-python test-bash test-quick test-full test-clean test-ci clean
