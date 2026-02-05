
# Portable Makefile for PiSSTVpp (Raspberry Pi, Linux, macOS)
# Usage: make [all|debug|clean]

PKG_CONFIG ?= pkg-config
CC ?= gcc

# Platform detection
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)


# Common flags
CFLAGS_COMMON = -O3 -ffast-math -funroll-loops -Wall -Wextra -Wpedantic -std=c11
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
BIN_DIR = bin
TEST_DIR = tests
UTIL_DIR = util

# Final flags (user can override CFLAGS/LDFLAGS if needed)
CFLAGS ?= $(CFLAGS_COMMON) $(CFLAGS_PKG) $(CFLAGS_PI) -I$(SRC_DIR) -I$(INC_DIR)
LDFLAGS ?= $(LDFLAGS_PKG) -lm

# Target configuration
TARGET = $(BIN_DIR)/pisstvpp2
TARGET_LIBGD = $(BIN_DIR)/pisstvpp2_libgd
TARGET_SAN = $(BIN_DIR)/pisstvpp2_san
TARGET_VIPS_TEST = $(BIN_DIR)/vips_test
SRC_FILES = $(SRC_DIR)/pisstvpp2.c $(SRC_DIR)/pisstvpp2_image.c $(SRC_DIR)/pisstvpp2_sstv.c \
            $(SRC_DIR)/pisstvpp2_audio_encoder.c $(SRC_DIR)/audio_encoder_wav.c \
            $(SRC_DIR)/audio_encoder_aiff.c $(SRC_DIR)/audio_encoder_ogg.c
OBJ_FILES = $(BIN_DIR)/pisstvpp2.o $(BIN_DIR)/pisstvpp2_image.o $(BIN_DIR)/pisstvpp2_sstv.o \
            $(BIN_DIR)/pisstvpp2_audio_encoder.o $(BIN_DIR)/audio_encoder_wav.o \
            $(BIN_DIR)/audio_encoder_aiff.o $(BIN_DIR)/audio_encoder_ogg.o

all: $(TARGET)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

debug: CFLAGS += -g -O0
debug: clean $(TARGET)

# Test targets
test: $(TARGET) test-python

test-python: $(TARGET)
	cd $(TEST_DIR) && python3 test_suite.py

test-bash: $(TARGET)
	chmod +x $(TEST_DIR)/test_suite_bash.sh
	cd $(TEST_DIR) && ./test_suite_bash.sh

test-quick: $(TARGET)
	cd $(TEST_DIR) && ./test_suite_bash.sh

test-full: $(TARGET)
	cd $(TEST_DIR) && python3 test_suite.py --verbose

test-clean:
	rm -rf $(TEST_DIR)/test_outputs/

# CI/CD test target
test-ci: $(TARGET)
	cd $(TEST_DIR) && python3 test_suite.py
	@if [ $$? -eq 0 ]; then echo "All tests passed!"; else echo "Some tests failed!"; exit 1; fi

clean:
	rm -f $(BIN_DIR)/pisstvpp2* $(BIN_DIR)/*.o $(BIN_DIR)/*.dSYM
	rm -rf $(BIN_DIR)/*.dSYM

.PHONY: all debug test test-python test-bash test-quick test-full test-clean test-ci clean
