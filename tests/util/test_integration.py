#!/usr/bin/env python3
"""
Integration Testing for PiSSTVpp2
Tests feature combinations and real-world usage scenarios
"""

import subprocess
import os
import sys
import tempfile
from pathlib import Path
from typing import List, Tuple
from dataclasses import dataclass
import time

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

@dataclass
class IntegrationTest:
    """Represents an integration test case"""
    name: str
    image: str
    args: List[str]
    description: str = ""
    verify_output: bool = True

class IntegrationTestSuite:
    """Comprehensive integration testing suite"""
    
    # Configuration
    PROTOCOLS = ['m1', 'm2', 's1', 's2', 'sdx', 'r36', 'r72']
    FORMATS = ['wav', 'aiff', 'ogg']
    ASPECTS = ['center', 'pad', 'stretch']
    SAMPLE_RATES = [8000, 11025, 22050, 44100, 48000]
    
    def __init__(self, executable_path=None,
                 images_dir=None):
        if executable_path is None:
            script_dir = Path(__file__).parent.parent.parent
            executable_path = str(script_dir / "bin" / "pisstvpp2")
        if images_dir is None:
            script_dir = Path(__file__).parent.parent
            images_dir = str(script_dir / "images")
        self.exe = executable_path
        self.images_dir = images_dir
        self.passed = 0
        self.failed = 0
        self.skipped = 0
        self.test_results = []
        self.temp_dir = tempfile.mkdtemp(prefix="pisstvpp2_test_")
        
        if not os.path.exists(self.exe):
            raise FileNotFoundError(f"Executable not found: {self.exe}")
        if not os.path.exists(self.images_dir):
            raise FileNotFoundError(f"Images directory not found: {self.images_dir}")
        
        # Discover available test images
        self.test_images = self._discover_images()
    
    def _discover_images(self) -> dict:
        """Discover and categorize test images"""
        images = {}
        for filename in os.listdir(self.images_dir):
            filepath = os.path.join(self.images_dir, filename)
            if os.path.isfile(filepath):
                # Extract format and dimensions from filename
                parts = filename.rsplit('.', 1)
                if len(parts) == 2:
                    name, ext = parts
                    if ext.lower() in ['jpg', 'jpeg', 'png', 'bmp', 'gif', 'ppm']:
                        if ext.lower() not in images:
                            images[ext.lower()] = []
                        images[ext.lower()].append({
                            'filename': filename,
                            'path': filepath,
                            'name': name
                        })
        return images
    
    def _run_command(self, args: List[str]) -> Tuple[int, str, str]:
        """Execute command and return exit code, stdout, stderr"""
        cmd = [self.exe] + args
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=60
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "TIMEOUT"
        except Exception as e:
            return -1, "", str(e)
    
    def _log_result(self, test: IntegrationTest, passed: bool, message: str = ""):
        """Log test result"""
        status = "PASS" if passed else "FAIL"
        color = Colors.GREEN if passed else Colors.RED
        
        print(f"{color}[{status}]{Colors.RESET} {test.name}")
        if test.description:
            print(f"     {Colors.CYAN}→ {test.description}{Colors.RESET}")
        if message:
            print(f"     {message}")
        
        self.test_results.append({
            'name': test.name,
            'status': status,
            'message': message
        })
        
        if passed:
            self.passed += 1
        else:
            self.failed += 1
    
    # =========================================================================
    # TEST GROUPS
    # =========================================================================
    
    def test_protocol_format_combinations(self):
        """Test all protocol and format combinations"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Protocol & Format Combinations ==={Colors.RESET}")
        
        # Pick a good test image
        test_image = next(iter(self.test_images.values()))[0]['path'] if self.test_images else None
        if not test_image:
            print(f"{Colors.YELLOW}Skipping - no test images available{Colors.RESET}")
            return
        
        test_count = 0
        for protocol in self.PROTOCOLS:
            for format_type in self.FORMATS:
                output_file = os.path.join(self.temp_dir, f"test_{protocol}_{format_type}.{format_type}")
                args = ["-i", test_image, "-p", protocol, "-f", format_type, "-o", output_file]
                
                returncode, stdout, stderr = self._run_command(args)
                passed = returncode == 0
                
                test = IntegrationTest(
                    f"Protocol {protocol} with {format_type} format",
                    test_image,
                    args,
                    f"Encoding to {format_type}"
                )
                
                self._log_result(test, passed)
                test_count += 1
                
                # Verify output file creation
                if passed and os.path.exists(output_file):
                    file_size = os.path.getsize(output_file)
                    print(f"     Output: {output_file.split('/')[-1]} ({file_size} bytes)")
        
        print(f"     Tested {test_count} protocol/format combinations")
    
    def test_image_format_variety(self):
        """Test encoding with different image formats"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Image Format Support ==={Colors.RESET}")
        
        formats_tested = {}
        for ext, images in self.test_images.items():
            if images:
                test_image = images[0]
                output_file = os.path.join(self.temp_dir, f"test_format_{ext}.wav")
                args = ["-i", test_image['path'], "-f", "wav", "-o", output_file]
                
                returncode, stdout, stderr = self._run_command(args)
                passed = returncode == 0
                
                test = IntegrationTest(
                    f"Encoding from {ext.upper()} image",
                    test_image['path'],
                    args,
                    f"File: {test_image['filename']}"
                )
                
                self._log_result(test, passed)
                formats_tested[ext] = {'passed': passed, 'images': len(images)}
        
        print(f"\n     Image formats tested: {', '.join(formats_tested.keys())}")
        for ext, info in formats_tested.items():
            print(f"     • {ext.upper()}: {info['images']} images, {'✓' if info['passed'] else '✗'}")
    
    def test_image_resolution_handling(self):
        """Test encoding with different image resolutions"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Image Resolution Handling ==={Colors.RESET}")
        
        resolution_tests = []
        for ext, images in self.test_images.items():
            for img in images[:3]:  # Test up to 3 per format
                # Extract resolution if available in filename
                output_file = os.path.join(self.temp_dir, f"test_res_{img['name']}.wav")
                args = ["-i", img['path'], "-f", "wav", "-o", output_file]
                
                returncode, stdout, stderr = self._run_command(args)
                passed = returncode == 0
                
                test = IntegrationTest(
                    f"Resolution: {img['filename']}",
                    img['path'],
                    args,
                    "Testing various image dimensions"
                )
                
                self._log_result(test, passed)
                resolution_tests.append({'name': img['filename'], 'passed': passed})
        
        passed_count = sum(1 for t in resolution_tests if t['passed'])
        print(f"\n     {passed_count}/{len(resolution_tests)} resolution tests passed")
    
    def test_cw_signature_variations(self):
        """Test CW signature with various callsigns and parameters"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== CW Signature Variations ==={Colors.RESET}")
        
        test_image = next(iter(self.test_images.values()))[0]['path'] if self.test_images else None
        if not test_image:
            print(f"{Colors.YELLOW}Skipping - no test images{Colors.RESET}")
            return
        
        cw_tests = [
            ("N0CALL", 15, 800, "Standard callsign with defaults"),
            ("W5CALL", 20, 700, "Different callsign, custom WPM"),
            ("K1ABC", 10, 600, "Short callsign, low WPM"),
            ("VE3XYZ", 40, 1000, "Long callsign, high WPM, high tone"),
            ("ZL2/N0CALL", 25, 900, "Multi-part callsign with slash"),
        ]
        
        for callsign, wpm, tone, desc in cw_tests:
            output_file = os.path.join(self.temp_dir, f"test_cw_{callsign.replace('/', '_')}.wav")
            args = ["-i", test_image, "-C", callsign, "-W", str(wpm), "-T", str(tone), "-o", output_file]
            
            returncode, stdout, stderr = self._run_command(args)
            passed = returncode == 0
            
            test = IntegrationTest(
                f"CW: {callsign} @ {wpm} WPM, {tone} Hz",
                test_image,
                args,
                desc
            )
            
            self._log_result(test, passed)
    
    def test_aspect_ratio_handling(self):
        """Test aspect ratio correction with different image sizes"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Aspect Ratio Handling ==={Colors.RESET}")
        
        # Use different resolution images to test aspect handling
        test_count = 0
        for ext, images in self.test_images.items():
            if images:
                test_image = images[0]['path']
                for aspect in self.ASPECTS:
                    output_file = os.path.join(self.temp_dir, f"test_aspect_{aspect}_{ext}.wav")
                    args = ["-i", test_image, "-a", aspect, "-o", output_file]
                    
                    returncode, stdout, stderr = self._run_command(args)
                    passed = returncode == 0
                    
                    test = IntegrationTest(
                        f"Aspect '{aspect}' with {ext.upper()} image",
                        test_image,
                        args,
                        f"Testing {aspect} aspect correction"
                    )
                    
                    self._log_result(test, passed)
                    test_count += 1
        
        print(f"     Tested {test_count} aspect/format combinations")
    
    def test_sample_rate_variations(self):
        """Test encoding with different sample rates"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Sample Rate Variations ==={Colors.RESET}")
        
        test_image = next(iter(self.test_images.values()))[0]['path'] if self.test_images else None
        if not test_image:
            print(f"{Colors.YELLOW}Skipping - no test images{Colors.RESET}")
            return
        
        for rate in self.SAMPLE_RATES:
            output_file = os.path.join(self.temp_dir, f"test_rate_{rate}.wav")
            args = ["-i", test_image, "-r", str(rate), "-o", output_file]
            
            returncode, stdout, stderr = self._run_command(args)
            passed = returncode == 0
            
            test = IntegrationTest(
                f"Sample rate: {rate} Hz",
                test_image,
                args,
                "Testing audio quality variations"
            )
            
            self._log_result(test, passed)
            
            # Check file size for quality assessment
            if passed and os.path.exists(output_file):
                file_size = os.path.getsize(output_file)
                print(f"     {rate} Hz: {file_size} bytes")
    
    def test_output_file_naming(self):
        """Test output filename generation and validation"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Output File Naming ==={Colors.RESET}")
        
        test_image = next(iter(self.test_images.values()))[0]['path'] if self.test_images else None
        if not test_image:
            print(f"{Colors.YELLOW}Skipping - no test images{Colors.RESET}")
            return
        
        naming_tests = [
            ([], "Auto-generated from input filename"),
            (["-o", "custom_output.wav"], "Explicit output filename"),
            (["-f", "aiff"], "Auto-generated with AIFF extension"),
            (["-f", "ogg"], "Auto-generated with OGG extension"),
        ]
        
        for args_extra, desc in naming_tests:
            args = ["-i", test_image] + args_extra
            
            returncode, stdout, stderr = self._run_command(args)
            passed = returncode == 0
            
            test = IntegrationTest(
                f"Output naming: {desc}",
                test_image,
                args,
                desc
            )
            
            self._log_result(test, passed)
    
    def test_verbose_mode_variations(self):
        """Test verbose and debugging options"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Verbose & Debug Options ==={Colors.RESET}")
        
        test_image = next(iter(self.test_images.values()))[0]['path'] if self.test_images else None
        if not test_image:
            print(f"{Colors.YELLOW}Skipping - no test images{Colors.RESET}")
            return
        
        verbose_tests = [
            (["-v"], "Verbose output only"),
            (["-Z"], "Timestamps (implies verbose)"),
            (["-K"], "Keep intermediate files"),
            (["-v", "-K"], "Verbose + keep intermediates"),
        ]
        
        for args_extra, desc in verbose_tests:
            output_file = os.path.join(self.temp_dir, f"test_verbose_{desc.replace(' ', '_')}.wav")
            args = ["-i", test_image, "-o", output_file] + args_extra
            
            returncode, stdout, stderr = self._run_command(args)
            passed = returncode == 0
            
            test = IntegrationTest(
                f"Option: {desc}",
                test_image,
                args,
                desc
            )
            
            self._log_result(test, passed)
    
    def test_multi_image_processing(self):
        """Test processing multiple images in sequence"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Multi-Image Processing ==={Colors.RESET}")
        
        # Collect different format images
        images_to_test = []
        for ext, imgs in self.test_images.items():
            if imgs:
                images_to_test.append(imgs[0])
                if len(images_to_test) >= 3:
                    break
        
        if not images_to_test:
            print(f"{Colors.YELLOW}Skipping - insufficient test images{Colors.RESET}")
            return
        
        for img in images_to_test:
            output_file = os.path.join(self.temp_dir, f"multi_{img['name']}.wav")
            args = ["-i", img['path'], "-o", output_file]
            
            returncode, stdout, stderr = self._run_command(args)
            passed = returncode == 0
            
            test = IntegrationTest(
                f"Sequential: {img['filename']}",
                img['path'],
                args,
                "Processing multiple images"
            )
            
            self._log_result(test, passed)
        
        print(f"     Processed {len(images_to_test)} images in sequence")
    
    def run_all_tests(self):
        """Execute all integration tests"""
        print(f"\n{Colors.BOLD}{Colors.CYAN}╔═══════════════════════════════════════════════════════════════╗{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}║  PiSSTVpp2 - Integration Testing Suite                       ║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}║  Testing real-world feature combinations and workflows       ║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}╚═══════════════════════════════════════════════════════════════╝{Colors.RESET}")
        
        print(f"\n{Colors.CYAN}Test Images Found: {sum(len(v) for v in self.test_images.values())} images{Colors.RESET}")
        for ext, imgs in self.test_images.items():
            print(f"  • {ext.upper()}: {len(imgs)} image(s)")
        
        start_time = time.time()
        
        try:
            self.test_protocol_format_combinations()
            self.test_image_format_variety()
            self.test_image_resolution_handling()
            self.test_cw_signature_variations()
            self.test_aspect_ratio_handling()
            self.test_sample_rate_variations()
            self.test_output_file_naming()
            self.test_verbose_mode_variations()
            self.test_multi_image_processing()
        finally:
            # Cleanup
            import shutil
            try:
                shutil.rmtree(self.temp_dir)
            except:
                pass
        
        total_time = time.time() - start_time
        self._print_summary(total_time)
    
    def _print_summary(self, total_time):
        """Print comprehensive test summary"""
        total = self.passed + self.failed + self.skipped
        
        print(f"\n{Colors.BOLD}{Colors.BLUE}═══════════════════════════════════════════════════════════════{Colors.RESET}")
        print(f"{Colors.BOLD}Integration Test Summary:{Colors.RESET}")
        print(f"  {Colors.GREEN}Passed:  {self.passed}{Colors.RESET}")
        print(f"  {Colors.RED}Failed:  {self.failed}{Colors.RESET}")
        print(f"  {Colors.YELLOW}Skipped: {self.skipped}{Colors.RESET}")
        print(f"  {Colors.BOLD}Total:   {total}{Colors.RESET}")
        print(f"  {Colors.CYAN}Time:    {total_time:.2f}s{Colors.RESET}")
        print(f"{Colors.BLUE}═══════════════════════════════════════════════════════════════{Colors.RESET}")
        
        if self.failed == 0:
            print(f"{Colors.GREEN}{Colors.BOLD}✓ All integration tests passed!{Colors.RESET}")
        else:
            print(f"{Colors.RED}{Colors.BOLD}✗ {self.failed} test(s) failed{Colors.RESET}")

if __name__ == "__main__":
    exe_path = sys.argv[1] if len(sys.argv) > 1 else None
    images_path = sys.argv[2] if len(sys.argv) > 2 else None
    if exe_path is None:
        script_dir = Path(__file__).parent.parent.parent
        exe_path = str(script_dir / "bin" / "pisstvpp2")
    if images_path is None:
        script_dir = Path(__file__).parent.parent
        images_path = str(script_dir / "images")
    
    try:
        suite = IntegrationTestSuite(exe_path, images_path)
        suite.run_all_tests()
        sys.exit(0 if suite.failed == 0 else 1)
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.RESET}")
        sys.exit(2)
