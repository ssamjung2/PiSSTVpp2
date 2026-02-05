#!/usr/bin/env python3
"""
Comprehensive test suite for PiSSTVpp SSTV encoder
Tests various protocols, audio formats, sample rates, aspect ratios, and CW signatures
"""

import os
import sys
import subprocess
import json
import tempfile
import shutil
from pathlib import Path
from datetime import datetime
import struct
import wave

class TestSuite:
    """Main test suite orchestrator for PiSSTVpp"""
    
    def __init__(self, executable_path="../bin/pisstvpp2", verbose=False):
        """Initialize test suite
        
        Args:
            executable_path: Path to pisstvpp2 executable
            verbose: Enable verbose output
        """
        self.exe = executable_path
        self.verbose = verbose
        self.test_dir = Path("test_outputs")
        self.test_results = []
        self.passed = 0
        self.failed = 0
        self.skipped = 0
        
        # Create test output directory
        self.test_dir.mkdir(exist_ok=True)
        
        # Verify executable exists
        if not os.path.exists(self.exe):
            raise FileNotFoundError(f"Executable not found: {self.exe}")
        
        # Discover available test images
        self.test_images = self._discover_test_images()
        if not self.test_images:
            raise FileNotFoundError("No test images found in images directory")
    
    def _discover_test_images(self):
        """Find all test images in the images directory"""
        image_extensions = ['.jpg', '.jpeg', '.png', '.bmp', '.gif', '.tiff']
        images = {}
        images_dir = Path("images")
        
        # Create images directory if it doesn't exist
        if not images_dir.exists():
            images_dir.mkdir(parents=True, exist_ok=True)
        
        for ext in image_extensions:
            for img_file in images_dir.glob(f"*{ext}"):
                if img_file.name.startswith("test"):
                    # Determine dimensions from filename or defaults
                    if "320x240" in img_file.name:
                        images[str(img_file)] = (320, 240)
                    elif "320x256" in img_file.name:
                        images[str(img_file)] = (320, 256)
                    elif "400x300" in img_file.name:
                        images[str(img_file)] = (400, 300)
                    elif "300x400" in img_file.name:
                        images[str(img_file)] = (300, 400)
                    else:
                        images[str(img_file)] = None  # Unknown dimensions
        
        return images
    
    def _run_command(self, args):
        """Execute pisstvpp2 with given arguments
        
        Returns:
            (returncode, stdout, stderr)
        """
        cmd = [self.exe] + args
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=300  # 5 minute timeout
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "TIMEOUT: Command exceeded 5 minutes"
        except Exception as e:
            return -1, "", f"ERROR: {str(e)}"
    
    def _test_output_file(self, output_file):
        """Validate output file properties
        
        Returns:
            (is_valid, file_info_dict)
        """
        info = {
            'exists': False,
            'size': 0,
            'format': None,
            'sample_rate': 0,
            'channels': 0,
            'duration_seconds': 0.0,
            'errors': []
        }
        
        if not os.path.exists(output_file):
            info['errors'].append("Output file not created")
            return False, info
        
        info['exists'] = True
        info['size'] = os.path.getsize(output_file)
        
        if info['size'] == 0:
            info['errors'].append("Output file is empty")
            return False, info
        
        # Try to read WAV header
        try:
            with wave.open(output_file, 'rb') as wav:
                info['channels'] = wav.getnchannels()
                info['sample_rate'] = wav.getframerate()
                frames = wav.getnframes()
                info['duration_seconds'] = frames / info['sample_rate']
                info['format'] = 'WAV'
                
                # Validate expected properties
                if info['channels'] != 1:
                    info['errors'].append(f"Expected 1 channel, got {info['channels']}")
                    return False, info
                if info['sample_rate'] not in [8000, 11025, 22050, 32000, 44100, 48000]:
                    info['errors'].append(f"Unexpected sample rate: {info['sample_rate']}")
        except wave.Error:
            # Try AIFF format
            if self._validate_aiff(output_file, info):
                return len(info['errors']) == 0, info
            # Try OGG format
            if self._validate_ogg(output_file, info):
                return len(info['errors']) == 0, info
            info['errors'].append("File is not valid WAV, AIFF, or OGG")
            return False, info
        except Exception as e:
            info['errors'].append(f"Error reading file: {str(e)}")
            return False, info
        
        return len(info['errors']) == 0, info
    
    def _validate_aiff(self, filename, info):
        """Validate AIFF file format"""
        try:
            with open(filename, 'rb') as f:
                # AIFF header: FORM, size, AIFF
                form = f.read(4)
                if form != b'FORM':
                    return False
                f.read(4)  # size
                aiff = f.read(4)
                if aiff != b'AIFF':
                    return False
                
                info['format'] = 'AIFF'
                info['errors'] = []
                return True
        except:
            return False

    def _validate_ogg(self, filename, info):
        """Validate OGG Vorbis file format (basic header check)"""
        try:
            with open(filename, 'rb') as f:
                header = f.read(4)
                if header != b'OggS':
                    return False

                # Look for Vorbis identification header within first 64KB
                f.seek(0)
                data = f.read(65536)
                if b'vorbis' not in data:
                    return False

                info['format'] = 'OGG'
                info['errors'] = []
                return True
        except:
            return False
    
    def _assert_success(self, test_name, returncode, stderr):
        """Check command returned success"""
        if returncode != 0:
            self.test_results.append({
                'test': test_name,
                'status': 'FAILED',
                'reason': f"Command failed with code {returncode}\n{stderr}"
            })
            self.failed += 1
            return False
        return True
    
    def _assert_output(self, test_name, output_file):
        """Check output file is valid"""
        is_valid, info = self._test_output_file(output_file)
        if not is_valid:
            self.test_results.append({
                'test': test_name,
                'status': 'FAILED',
                'reason': f"Output validation failed: {info['errors']}"
            })
            self.failed += 1
            return False, info
        return True, info
    
    def _log_test(self, test_name, status, details=""):
        """Log test result"""
        self.test_results.append({
            'test': test_name,
            'status': status,
            'details': details
        })
        
        if status == 'PASSED':
            self.passed += 1
        elif status == 'FAILED':
            self.failed += 1
        elif status == 'SKIPPED':
            self.skipped += 1
    
    # =========================================================================
    # TEST GROUPS
    # =========================================================================
    
    def test_help_and_info(self):
        """Test help and version information"""
        print("\n" + "="*70)
        print("TEST GROUP: Help and Information")
        print("="*70)
        
        # Test -h flag
        ret, stdout, stderr = self._run_command(["-h"])
        if "usage" in stdout.lower() or "options" in stdout.lower():
            self._log_test("Help output (-h)", "PASSED")
        else:
            self._log_test("Help output (-h)", "FAILED", "Help text not found")
    
    def test_missing_input(self):
        """Test error handling for missing input file"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Missing Arguments")
        print("="*70)
        
        # Missing -i argument
        ret, stdout, stderr = self._run_command([])
        if ret != 0 and ("required" in stderr.lower() or "input" in stderr.lower()):
            self._log_test("Missing input file (-i)", "PASSED", 
                          f"Correctly rejected: {stderr.split(chr(10))[0]}")
        else:
            self._log_test("Missing input file (-i)", "FAILED", 
                          "Should reject missing input")
    
    def test_nonexistent_input(self):
        """Test error handling for non-existent input file"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Invalid Input")
        print("="*70)
        
        ret, stdout, stderr = self._run_command(["-i", "nonexistent_file.jpg"])
        if ret != 0:
            self._log_test("Non-existent input file", "PASSED",
                          f"Correctly rejected: {stderr.split(chr(10))[0]}")
        else:
            self._log_test("Non-existent input file", "FAILED",
                          "Should reject non-existent file")
    
    def test_protocols(self):
        """Test all supported SSTV protocols"""
        print("\n" + "="*70)
        print("TEST GROUP: SSTV Protocols")
        print("="*70)
        
        protocols = {
            'm1': {'code': 44, 'name': 'Martin 1', 'time_sec': '~114'},
            'm2': {'code': 40, 'name': 'Martin 2', 'time_sec': '~58'},
            's1': {'code': 60, 'name': 'Scottie 1', 'time_sec': '~110'},
            's2': {'code': 56, 'name': 'Scottie 2', 'time_sec': '~71'},
            'sdx': {'code': 76, 'name': 'Scottie DX', 'time_sec': '~269'},
            'r36': {'code': 8, 'name': 'Robot 36', 'time_sec': '~36'},
            'r72': {'code': 12, 'name': 'Robot 72', 'time_sec': '~72'},
        }
        
        # Use first available test image
        test_img = list(self.test_images.keys())[0]
        
        for proto_code, proto_info in protocols.items():
            output_file = self.test_dir / f"test_protocol_{proto_code}.wav"
            
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-p", proto_code,
                "-o", str(output_file),
                "-a", "center",
                "-v"                
            ])
            
            if not self._assert_success(f"Protocol {proto_code} ({proto_info['name']})", ret, stderr):
                continue
            
            is_valid, info = self._assert_output(
                f"Protocol {proto_code} ({proto_info['name']})", 
                str(output_file)
            )
            
            if is_valid:
                self._log_test(
                    f"Protocol {proto_code} ({proto_info['name']})",
                    "PASSED",
                    f"Output: {info['duration_seconds']:.1f}s @ {info['sample_rate']} Hz, {info['size']} bytes"
                )
    
    def test_audio_formats(self):
        """Test WAV, AIFF, and OGG output formats"""
        print("\n" + "="*70)
        print("TEST GROUP: Audio Output Formats")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        for fmt in ['wav', 'aiff', 'ogg']:
            output_file = self.test_dir / f"test_format_{fmt}.{fmt}"
            
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-f", fmt,
                "-o", str(output_file),
                "-a", "center",
                "-v"
            ])
            
            if ret != 0:
                if fmt == 'ogg' and "not compiled" in stderr.lower():
                    self._log_test(
                        f"Output format {fmt.upper()}",
                        "SKIPPED",
                        "OGG support not compiled in"
                    )
                    continue
                if not self._assert_success(f"Output format {fmt.upper()}", ret, stderr):
                    continue
            
            is_valid, info = self._assert_output(
                f"Output format {fmt.upper()}", 
                str(output_file)
            )
            
            if is_valid:
                self._log_test(
                    f"Output format {fmt.upper()}",
                    "PASSED",
                    f"Format: {info['format']}, Size: {info['size']} bytes"
                )
    
    def test_sample_rates(self):
        """Test various audio sample rates"""
        print("\n" + "="*70)
        print("TEST GROUP: Audio Sample Rates")
        print("="*70)
        
        sample_rates = [8000, 11025, 22050, 32000, 44100, 48000]
        test_img = list(self.test_images.keys())[0]
        
        for rate in sample_rates:
            output_file = self.test_dir / f"test_rate_{rate}.wav"
            
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-r", str(rate),
                "-o", str(output_file),
                "-a", "center",
                "-v"
            ])
            
            if not self._assert_success(f"Sample rate {rate} Hz", ret, stderr):
                continue
            
            is_valid, info = self._assert_output(
                f"Sample rate {rate} Hz", 
                str(output_file)
            )
            
            if is_valid:
                if info['sample_rate'] == rate:
                    self._log_test(
                        f"Sample rate {rate} Hz",
                        "PASSED",
                        f"Verified {info['sample_rate']} Hz, {info['duration_seconds']:.1f}s"
                    )
                else:
                    self._log_test(
                        f"Sample rate {rate} Hz",
                        "FAILED",
                        f"Expected {rate}, got {info['sample_rate']}"
                    )
    
    def test_invalid_sample_rates(self):
        """Test rejection of invalid sample rates"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Invalid Sample Rates")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        invalid_rates = [7999, 4000, 48001, 96000, 0, -1]
        
        for rate in invalid_rates:
            output_file = self.test_dir / f"test_invalid_rate_{rate}.wav"
            
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-r", str(rate),
                "-o", str(output_file),
                "-a", "center",
                "-v"
            ])
            
            if ret != 0:
                self._log_test(
                    f"Invalid sample rate {rate} Hz (rejection)",
                    "PASSED",
                    "Correctly rejected"
                )
            else:
                self._log_test(
                    f"Invalid sample rate {rate} Hz (rejection)",
                    "FAILED",
                    "Should reject out-of-range rate"
                )
    
    def test_aspect_modes(self):
        """Test aspect ratio correction modes"""
        print("\n" + "="*70)
        print("TEST GROUP: Aspect Ratio Modes")
        print("="*70)
        
        aspect_modes = ['center', 'pad', 'stretch']
        test_img = list(self.test_images.keys())[0]
        
        for mode in aspect_modes:
            output_file = self.test_dir / f"test_aspect_{mode.replace(':', '_')}.wav"
            
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-a", mode,
                "-o", str(output_file),
                "-v"
            ])
            
            if not self._assert_success(f"Aspect mode {mode}", ret, stderr):
                continue
            
            is_valid, info = self._assert_output(
                f"Aspect mode {mode}", 
                str(output_file)
            )
            
            if is_valid:
                self._log_test(
                    f"Aspect ratio mode '{mode}'",
                    "PASSED",
                    f"Output: {info['duration_seconds']:.1f}s"
                )
    
    def test_invalid_aspect_mode(self):
        """Test rejection of invalid aspect mode"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Invalid Aspect Modes")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-a", "invalid_mode"
        ])
        
        if ret != 0 and ("aspect" in stderr.lower() or "must be" in stderr.lower()):
            self._log_test("Invalid aspect mode rejection", "PASSED", "Correctly rejected")
        else:
            self._log_test("Invalid aspect mode rejection", "FAILED", "Should reject invalid mode")
    
    def test_cw_signatures(self):
        """Test CW callsign signature generation"""
        print("\n" + "="*70)
        print("TEST GROUP: CW Signatures")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        cw_tests = [
            {'call': 'K4ABC', 'wpm': 15, 'tone': 800, 'name': 'Standard K4ABC'},
            {'call': 'N0CALL', 'wpm': 10, 'tone': 600, 'name': 'Slow N0CALL 10 WPM'},
            {'call': 'W5ZZZ', 'wpm': 25, 'tone': 1000, 'name': 'Fast W5ZZZ 25 WPM'},
            {'call': 'K5ABC/MM', 'wpm': 15, 'tone': 1200, 'name': 'Maritime with slash'},
        ]
        
        for test_case in cw_tests:
            output_file = self.test_dir / f"test_cw_{test_case['call'].replace('/', '_')}.wav"
            
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-C", test_case['call'],
                "-W", str(test_case['wpm']),
                "-T", str(test_case['tone']),
                "-o", str(output_file),
                "-v"
            ])
            
            if not self._assert_success(f"CW {test_case['name']}", ret, stderr):
                continue
            
            is_valid, info = self._assert_output(
                f"CW {test_case['name']}", 
                str(output_file)
            )
            
            if is_valid:
                self._log_test(
                    f"CW Signature: {test_case['name']}",
                    "PASSED",
                    f"Duration: {info['duration_seconds']:.1f}s"
                )
    
    def test_cw_without_callsign(self):
        """Test error handling for CW options without callsign"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - CW Validation")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        # Test -W without -C
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-W", "20"
        ])
        
        if ret != 0 and ("-C" in stderr or "callsign" in stderr.lower()):
            self._log_test("CW -W without -C (error check)", "PASSED", "Correctly rejected")
        else:
            self._log_test("CW -W without -C (error check)", "FAILED", "Should require -C with -W")
        
        # Test -T without -C
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-T", "1000"
        ])
        
        if ret != 0 and ("-C" in stderr or "callsign" in stderr.lower()):
            self._log_test("CW -T without -C (error check)", "PASSED", "Correctly rejected")
        else:
            self._log_test("CW -T without -C (error check)", "FAILED", "Should require -C with -T")
    
    def test_invalid_cw_parameters(self):
        """Test rejection of invalid CW parameters"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Invalid CW Parameters")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        # Invalid WPM (out of range)
        for wpm in [0, 51, -1, 100]:
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-C", "TEST",
                "-W", str(wpm),
                "-a", "center",
                "-v"
            ])
            if ret != 0:
                self._log_test(f"Invalid WPM {wpm} (rejection)", "PASSED")
            else:
                self._log_test(f"Invalid WPM {wpm} (rejection)", "FAILED")
        
        # Invalid tone frequency (out of range)
        for tone in [300, 2100, 399, 0, -1]:
            ret, stdout, stderr = self._run_command([
                "-i", test_img,
                "-C", "TEST",
                "-T", str(tone),
                "-a", "center",
                "-v"
            ])
            if ret != 0:
                self._log_test(f"Invalid tone {tone} Hz (rejection)", "PASSED")
            else:
                self._log_test(f"Invalid tone {tone} Hz (rejection)", "FAILED")
    
    def test_long_callsign(self):
        """Test rejection of oversized callsign"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Callsign Validation")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        long_call = "A" * 50  # Too long
        
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-C", long_call
        ])
        
        if ret != 0 and ("too long" in stderr.lower() or "max" in stderr.lower()):
            self._log_test("Oversized callsign rejection", "PASSED", "Correctly rejected")
        else:
            self._log_test("Oversized callsign rejection", "FAILED", "Should reject long callsign")
    
    def test_invalid_format(self):
        """Test rejection of invalid output format"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Invalid Formats")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-f", "mp3"
        ])
        
        if ret != 0 and ("format" in stderr.lower() or "wav" in stderr.lower()):
            self._log_test("Invalid format rejection", "PASSED", "Correctly rejected mp3")
        else:
            self._log_test("Invalid format rejection", "FAILED", "Should reject invalid format")
    
    def test_invalid_protocol(self):
        """Test rejection of invalid protocol"""
        print("\n" + "="*70)
        print("TEST GROUP: Error Handling - Invalid Protocols")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-p", "invalid"
        ])
        
        if ret != 0 and ("protocol" in stderr.lower() or "unrecognized" in stderr.lower()):
            self._log_test("Invalid protocol rejection", "PASSED", "Correctly rejected")
        else:
            self._log_test("Invalid protocol rejection", "FAILED", "Should reject invalid protocol")
    
    def test_combined_options(self):
        """Test combinations of multiple options"""
        print("\n" + "="*70)
        print("TEST GROUP: Combined Options")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        test_cases = [
            {
                'name': 'Robot 72 at 44100 Hz with CW',
                'args': ['-p', 'r72', '-r', '44100', '-f', 'wav', '-C', 'W5ZZZ', '-W', '20']
            },
            {
                'name': 'Martin 2 with AIFF and custom aspect',
                'args': ['-p', 'm2', '-f', 'aiff', '-a', 'stretch', '-r', '48000']
            },
            {
                'name': 'Scottie DX with center mode and verbose',
                'args': ['-p', 'sdx', '-a', 'center', '-v']
            },
        ]
        
        for i, test_case in enumerate(test_cases, 1):
            output_file = self.test_dir / f"test_combined_{i}.wav"
            
            args = ['-i', test_img, '-o', str(output_file)] + test_case['args']
            ret, stdout, stderr = self._run_command(args)
            
            if not self._assert_success(f"Combined: {test_case['name']}", ret, stderr):
                continue
            
            is_valid, info = self._assert_output(
                f"Combined: {test_case['name']}", 
                str(output_file)
            )
            
            if is_valid:
                self._log_test(
                    f"Combined: {test_case['name']}",
                    "PASSED",
                    f"Duration: {info['duration_seconds']:.1f}s @ {info['sample_rate']} Hz"
                )
    
    def test_multiple_images(self):
        """Test encoding with different test images"""
        print("\n" + "="*70)
        print("TEST GROUP: Multiple Test Images")
        print("="*70)
        
        # Limit to first 3 images to save time
        for idx, img_name in enumerate(list(self.test_images.keys())[:3]):
            output_file = self.test_dir / f"test_img_{idx}_{Path(img_name).stem}.wav"
            
            ret, stdout, stderr = self._run_command([
                "-i", img_name,
                "-o", str(output_file)
            ])
            
            if not self._assert_success(f"Image: {img_name}", ret, stderr):
                continue
            
            is_valid, info = self._assert_output(
                f"Image: {img_name}", 
                str(output_file)
            )
            
            if is_valid:
                dims_str = f"{self.test_images[img_name]}" if self.test_images[img_name] else "unknown dims"
                self._log_test(
                    f"Test image: {img_name}",
                    "PASSED",
                    f"Dims: {dims_str}, Output: {info['duration_seconds']:.1f}s"
                )
    
    def test_output_file_naming(self):
        """Test output file naming and automatic extension addition"""
        print("\n" + "="*70)
        print("TEST GROUP: Output File Naming")
        print("="*70)
        
        test_img = list(self.test_images.keys())[0]
        
        # Test automatic .wav extension
        output_base = str(self.test_dir / "test_auto_ext")
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-o", output_base,
            "-f", "wav"
        ])
        
        if os.path.exists(output_base + ".wav"):
            self._log_test("Auto .wav extension", "PASSED")
        else:
            self._log_test("Auto .wav extension", "FAILED", "File not found with .wav ext")
        
        # Test explicit .aiff extension
        output_aiff = str(self.test_dir / "test_explicit.aiff")
        ret, stdout, stderr = self._run_command([
            "-i", test_img,
            "-o", output_aiff,
            "-f", "aiff"
        ])
        
        if os.path.exists(output_aiff):
            self._log_test("Explicit .aiff filename", "PASSED")
        else:
            self._log_test("Explicit .aiff filename", "FAILED", "File not created")
    
    # =========================================================================
    # RUN ALL TESTS
    # =========================================================================
    
    def run_all_tests(self):
        """Execute all test groups"""
        print(f"\n{'='*70}")
        print(f"PiSSTVpp Comprehensive Test Suite")
        print(f"Start time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"Executable: {self.exe}")
        print(f"Available test images: {len(self.test_images)}")
        print(f"{'='*70}")
        
        test_methods = [
            self.test_help_and_info,
            self.test_missing_input,
            self.test_nonexistent_input,
            self.test_protocols,
            self.test_audio_formats,
            self.test_sample_rates,
            self.test_invalid_sample_rates,
            self.test_aspect_modes,
            self.test_invalid_aspect_mode,
            self.test_cw_signatures,
            self.test_cw_without_callsign,
            self.test_invalid_cw_parameters,
            self.test_long_callsign,
            self.test_invalid_format,
            self.test_invalid_protocol,
            self.test_combined_options,
            self.test_multiple_images,
            self.test_output_file_naming,
        ]
        
        for test_method in test_methods:
            try:
                test_method()
            except Exception as e:
                print(f"\nERROR in {test_method.__name__}: {str(e)}")
                self.failed += 1
        
        self.print_summary()
    
    def print_summary(self):
        """Print test execution summary"""
        print(f"\n{'='*70}")
        print(f"Test Summary")
        print(f"{'='*70}")
        print(f"Total tests: {self.passed + self.failed + self.skipped}")
        print(f"✓ PASSED: {self.passed}")
        print(f"✗ FAILED: {self.failed}")
        print(f"⊘ SKIPPED: {self.skipped}")
        print(f"{'='*70}\n")
        
        # Save results to JSON
        results_file = self.test_dir / f"test_results_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(results_file, 'w') as f:
            json.dump({
                'summary': {
                    'passed': self.passed,
                    'failed': self.failed,
                    'skipped': self.skipped,
                    'total': self.passed + self.failed + self.skipped,
                    'timestamp': datetime.now().isoformat()
                },
                'results': self.test_results
            }, f, indent=2)
        
        print(f"Results saved to: {results_file}\n")
        
        if self.failed > 0:
            sys.exit(1)

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="PiSSTVpp Test Suite")
    parser.add_argument("--exe", default="./pisstvpp2", help="Path to pisstvpp2 executable")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output")
    parser.add_argument("--keep-outputs", action="store_true", help="Keep test output files")
    
    args = parser.parse_args()
    
    try:
        suite = TestSuite(executable_path=args.exe, verbose=args.verbose)
        suite.run_all_tests()
    except FileNotFoundError as e:
        print(f"FATAL: {str(e)}")
        sys.exit(1)
