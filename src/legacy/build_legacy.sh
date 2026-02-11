#!/bin/bash
# 
# Legacy Components Build Verification Script
# Tests building pisstvpp and pifm on current platform
# 
# Usage:
#   ./build_legacy.sh              - Test both components
#   ./build_legacy.sh -c pisstvpp  - Test only pisstvpp
#   ./build_legacy.sh -c pifm      - Test only pifm
#   ./build_legacy.sh -h           - Show help
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
LEGACY_DIR="$PROJECT_ROOT/src/legacy"
BIN_DIR="$PROJECT_ROOT/bin"
DOCS_DIR="$PROJECT_ROOT/docs"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'  # No Color

# Configuration
COMPONENT=${1:-all}
VERBOSE=${VERBOSE:-0}
PLATFORM=$(uname -s)
ARCH=$(uname -m)

# ============================================================================
# Helper Functions
# ============================================================================

print_header() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_section() {
    echo -e "\n${BLUE}▶ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    if [ $VERBOSE -eq 1 ]; then
        echo -e "  $1"
    fi
}

# ============================================================================
# Dependency Checking
# ============================================================================

check_command() {
    if command -v "$1" &> /dev/null; then
        return 0
    else
        return 1
    fi
}

check_library() {
    local lib=$1
    local header=${2:-$lib}
    
    if pkg-config --exists "$lib" 2>/dev/null; then
        return 0
    fi
    return 1
}

check_dependencies() {
    print_section "Checking Dependencies"
    
    local deps_ok=1
    
    # Check basic build tools
    if ! check_command gcc; then
        print_error "gcc not found (required for pisstvpp)"
        deps_ok=0
    else
        print_success "gcc found: $(gcc --version | head -1)"
    fi
    
    if ! check_command g++; then
        print_warning "g++ not found (required for pifm)"
    else
        print_success "g++ found: $(g++ --version | head -1)"
    fi
    
    # Check libgd
    if ! check_library gdlib; then
        print_error "libgd not found"
        echo "  Install: brew install gd (macOS) or sudo apt-get install libgd-dev (Linux)"
        deps_ok=0
    else
        print_success "libgd found"
    fi
    
    # Check libmagic
    if ! check_library libmagic; then
        print_error "libmagic not found"
        echo "  Install: brew install libmagic (macOS) or sudo apt-get install libmagic-dev (Linux)"
        deps_ok=0
    else
        print_success "libmagic found"
    fi
    
    # Check pkg-config
    if ! check_command pkg-config; then
        print_warning "pkg-config not found (will use fallback paths)"
    else
        print_success "pkg-config found"
    fi
    
    return $deps_ok
}

# ============================================================================
# Build Functions
# ============================================================================

build_pisstvpp() {
    print_section "Building pisstvpp"
    
    cd "$LEGACY_DIR" || exit 1
    
    if ! check_library gdlib; then
        print_error "libgd not found - skipping pisstvpp"
        return 1
    fi
    
    if ! check_library libmagic; then
        print_error "libmagic not found - skipping pisstvpp"
        return 1
    fi
    
    print_info "Cleaning previous build..."
    make -f Makefile.legacy clean >/dev/null 2>&1 || true
    
    print_info "Running build..."
    if make -f Makefile.legacy pisstvpp; then
        if [ -f "$BIN_DIR/pisstvpp" ]; then
            print_success "pisstvpp built successfully"
            ls -lh "$BIN_DIR/pisstvpp" | sed 's/^/  /'
            return 0
        else
            print_error "Build succeeded but binary not found"
            return 1
        fi
    else
        print_error "pisstvpp build failed"
        return 1
    fi
}

build_pifm() {
    print_section "Building pifm"
    
    cd "$LEGACY_DIR" || exit 1
    
    if ! check_command g++; then
        print_error "g++ not found - skipping pifm"
        return 1
    fi
    
    print_info "Cleaning previous build..."
    make -f Makefile.legacy clean >/dev/null 2>&1 || true
    
    print_info "Running build..."
    if make -f Makefile.legacy pifm 2>/dev/null; then
        if [ -f "$BIN_DIR/pifm" ]; then
            print_success "pifm built successfully"
            ls -lh "$BIN_DIR/pifm" | sed 's/^/  /'
            return 0
        else
            print_error "Build succeeded but binary not found"
            return 1
        fi
    else
        print_error "pifm build failed"
        print_info "Note: pifm requires Raspberry Pi hardware to run, but should build on other platforms"
        return 1
    fi
}

# ============================================================================
# Binary Testing
# ============================================================================

test_pisstvpp() {
    print_section "Testing pisstvpp"
    
    if [ ! -f "$BIN_DIR/pisstvpp" ]; then
        print_error "pisstvpp binary not found"
        return 1
    fi
    
    # Test help
    print_info "Testing --help flag..."
    if "$BIN_DIR/pisstvpp" -h | grep -q "Usage:"; then
        print_success "Help output works"
    else
        print_error "Help output failed"
        return 1
    fi
    
    # Test with non-existent file
    print_info "Testing error handling..."
    if ! "$BIN_DIR/pisstvpp" /nonexistent/file.jpg 2>/dev/null; then
        print_success "Error handling works (correctly rejected invalid file)"
    else
        print_error "Error handling failed (should reject invalid file)"
        return 1
    fi
    
    return 0
}

test_pifm() {
    print_section "Testing pifm"
    
    if [ ! -f "$BIN_DIR/pifm" ]; then
        print_error "pifm binary not found"
        return 1
    fi
    
    # Test help
    print_info "Testing --help flag..."
    if "$BIN_DIR/pifm" -h 2>&1 | grep -q "Usage:"; then
        print_success "Help output works"
    else
        print_error "Help output failed"
        return 1
    fi
    
    # Note about hardware-specific testing
    case $PLATFORM in
        Linux)
            if [ "$ARCH" == "armv7l" ] || [ "$ARCH" == "armv6l" ] || [ "$ARCH" == "aarch64" ]; then
                print_info "RPi hardware detected - full functionality available"
            else
                print_warning "pifm built but hardware testing not possible (not on RPi)"
            fi
            ;;
        Darwin)
            print_warning "pifm built but not functional on macOS (RPi specific)"
            ;;
    esac
    
    return 0
}

# ============================================================================
# Platform Detection
# ============================================================================

detect_platform() {
    print_section "Platform Information"
    echo "  OS: $PLATFORM"
    echo "  Architecture: $ARCH"
    
    case $PLATFORM in
        Linux)
            if [ -f /os-release ]; then
                . /os-release
                echo "  Distro: $PRETTY_NAME"
            fi
            
            if [ -f /proc/device-tree/model ]; then
                echo "  Device: $(cat /proc/device-tree/model)"
            fi
            ;;
        Darwin)
            echo "  macOS Version: $(sw_vers -productVersion)"
            if check_command arch; then
                echo "  Native Arch: $(arch)"
            fi
            ;;
    esac
}

# ============================================================================
# Comprehensive Testing
# ============================================================================

test_all() {
    local pisstvpp_ok=0
    local pifm_ok=0
    
    if ! check_dependencies; then
        print_warning "Some dependencies missing - build may fail"
    fi
    
    detect_platform
    
    case "$COMPONENT" in
        all|pisstvpp)
            if build_pisstvpp; then
                if test_pisstvpp; then
                    pisstvpp_ok=1
                fi
            fi
            ;;
    esac
    
    case "$COMPONENT" in
        all|pifm)
            if build_pifm; then
                if test_pifm; then
                    pifm_ok=1
                fi
            fi
            ;;
    esac
    
    # Summary
    print_header "Build Summary"
    
    case "$COMPONENT" in
        pisstvpp)
            if [ $pisstvpp_ok -eq 1 ]; then
                print_success "pisstvpp build and test passed"
                return 0
            else
                print_error "pisstvpp build or test failed"
                return 1
            fi
            ;;
        pifm)
            if [ $pifm_ok -eq 1 ]; then
                print_success "pifm build and test passed"
                return 0
            else
                print_error "pifm build or test failed"
                return 1
            fi
            ;;
        all)
            echo "  pisstvpp: $([ $pisstvpp_ok -eq 1 ] && echo 'PASS' || echo 'FAIL')"
            echo "  pifm:     $([ $pifm_ok -eq 1 ] && echo 'PASS' || echo 'FAIL')"
            echo ""
            if [ $pisstvpp_ok -eq 1 ] || [ $pifm_ok -eq 1 ]; then
                print_success "At least one component built successfully"
                return 0
            else
                print_error "All components failed to build"
                return 1
            fi
            ;;
    esac
}

# ============================================================================
# Help
# ============================================================================

show_help() {
    cat << 'EOF'
Legacy Components Build Verification Script

Usage:
    ./build_legacy.sh               Build and test all available components
    ./build_legacy.sh -c pisstvpp   Build and test pisstvpp only
    ./build_legacy.sh -c pifm       Build and test pifm only
    ./build_legacy.sh -v            Verbose output
    ./build_legacy.sh -h            Show this help

Environment Variables:
    VERBOSE=1                       Enable verbose output
    
Examples:
    ./build_legacy.sh
    ./build_legacy.sh -c pisstvpp
    ./build_legacy.sh -v -c pifm
    VERBOSE=1 ./build_legacy.sh

Components:
    - pisstvpp: Image to SSTV audio converter (all platforms)
    - pifm:     Raspberry Pi FM transmitter (RPi only)

See docs/LEGACY_BUILD.md for detailed build instructions.

EOF
}

# ============================================================================
# Main
# ============================================================================

main() {
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                return 0
                ;;
            -c|--component)
                COMPONENT="$2"
                shift 2
                ;;
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            all|pisstvpp|pifm)
                COMPONENT="$1"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                return 1
                ;;
        esac
    done
    
    print_header "Legacy Components Build Verification"
    echo "Build Date: $(date)"
    echo "Project Root: $PROJECT_ROOT"
    echo ""
    
    test_all
}

# Run main function
main "$@"
