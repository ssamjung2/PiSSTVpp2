#!/usr/bin/env python3
"""
Performance and Stress Testing for PiSSTVpp2
Tests application performance under various load conditions
"""

import subprocess
import os
import sys
import tempfile
import time
from pathlib import Path
from typing import Tuple, List
import threading
import multiprocessing
from datetime import datetime

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

class PerformanceTestSuite:
    """Performance and stress testing suite"""
    
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
        self.temp_dir = tempfile.mkdtemp(prefix="pisstvpp2_perf_")
        self.results = []
        
        # Collect all available test images
        self.test_images = self._discover_images()
        
        if not os.path.exists(self.exe):
            raise FileNotFoundError(f"Executable not found: {self.exe}")
    
    def _discover_images(self) -> List[str]:
        """Discover all available test images"""
        images = []
        if os.path.exists(self.images_dir):
            for filename in os.listdir(self.images_dir):
                filepath = os.path.join(self.images_dir, filename)
                if os.path.isfile(filepath) and filename.lower().endswith(
                    ('.png', '.jpg', '.jpeg', '.bmp', '.gif', '.ppm')):
                    images.append(filepath)
        return images
    
    def _run_command(self, args) -> Tuple[int, float, str]:
        """Execute command and return exit code, execution time, and stderr"""
        cmd = [self.exe] + args
        try:
            start = time.time()
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=120
            )
            elapsed = time.time() - start
            return result.returncode, elapsed, result.stderr
        except subprocess.TimeoutExpired:
            return -1, 120.0, "TIMEOUT"
        except Exception as e:
            return -1, 0.0, str(e)
    
    def _log_result(self, test_name: str, passed: bool, message: str = ""):
        """Log test result"""
        status = "PASS" if passed else "FAIL"
        color = Colors.GREEN if passed else Colors.RED
        
        print(f"{color}[{status}]{Colors.RESET} {test_name}")
        if message:
            print(f"     {message}")
        
        if passed:
            self.passed += 1
        else:
            self.failed += 1
        
        self.results.append({
            'name': test_name,
            'status': status,
            'message': message
        })
    
    # =========================================================================
    # THROUGHPUT TESTS
    # =========================================================================
    
    def test_single_image_performance(self):
        """Test encoding speed for single image"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Single Image Performance ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        test_image = self.test_images[0]
        output_file = os.path.join(self.temp_dir, "perf_single.wav")
        
        returncode, elapsed, stderr = self._run_command([
            "-i", test_image,
            "-o", output_file
        ])
        
        passed = returncode == 0 and elapsed < 30  # Should complete in < 30 seconds
        self._log_result("Single image encoding", passed, 
                        message=f"Time: {elapsed:.2f}s, Size: {os.path.getsize(output_file) if os.path.exists(output_file) else 0} bytes")
    
    def test_various_format_performance(self):
        """Test encoding speed with different formats"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Format Performance Comparison ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        test_image = self.test_images[0]
        formats = ['wav', 'aiff', 'ogg']
        times = {}
        
        for fmt in formats:
            output_file = os.path.join(self.temp_dir, f"perf_format_{fmt}.{fmt}")
            returncode, elapsed, _ = self._run_command([
                "-i", test_image,
                "-f", fmt,
                "-o", output_file
            ])
            
            passed = returncode == 0
            self._log_result(f"Format: {fmt}", passed, 
                           message=f"Time: {elapsed:.2f}s")
            times[fmt] = elapsed
        
        # Print comparison
        if times:
            fastest = min(times, key=times.get)
            print(f"     Fastest: {fastest} ({times[fastest]:.2f}s)")
    
    def test_protocol_encoding_speed(self):
        """Test encoding speed for different protocols"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Protocol Encoding Speed ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        test_image = self.test_images[0]
        protocols = ['m1', 'm2', 's1', 's2', 'sdx']
        times = {}
        
        for protocol in protocols:
            output_file = os.path.join(self.temp_dir, f"perf_proto_{protocol}.wav")
            returncode, elapsed, _ = self._run_command([
                "-i", test_image,
                "-p", protocol,
                "-o", output_file
            ])
            
            passed = returncode == 0 and elapsed < 60
            self._log_result(f"Protocol: {protocol}", passed, 
                           message=f"Time: {elapsed:.2f}s")
            times[protocol] = elapsed
        
        if times:
            fastest = min(times, key=times.get)
            slowest = max(times, key=times.get)
            print(f"     Range: {fastest} ({times[fastest]:.2f}s) - {slowest} ({times[slowest]:.2f}s)")
    
    # =========================================================================
    # BATCH PROCESSING TESTS
    # =========================================================================
    
    def test_sequential_batch_processing(self):
        """Test sequential encoding of multiple images"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Batch Processing ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        # Process up to 5 images sequentially
        images_to_process = self.test_images[:5]
        start_time = time.time()
        success_count = 0
        
        for idx, image_path in enumerate(images_to_process, 1):
            output_file = os.path.join(self.temp_dir, f"batch_{idx}.wav")
            returncode, _, _ = self._run_command([
                "-i", image_path,
                "-o", output_file
            ])
            
            if returncode == 0:
                success_count += 1
        
        total_time = time.time() - start_time
        passed = success_count == len(images_to_process)
        
        self._log_result("Sequential batch processing", passed,
                        message=f"Processed {success_count}/{len(images_to_process)} in {total_time:.2f}s")
    
    def test_rapid_consecutive_calls(self):
        """Test rapid consecutive encoding operations"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Rapid Consecutive Operations ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        test_image = self.test_images[0]
        iterations = 10
        start_time = time.time()
        success_count = 0
        
        for i in range(iterations):
            output_file = os.path.join(self.temp_dir, f"rapid_{i}.wav")
            returncode, _, _ = self._run_command([
                "-i", test_image,
                "-o", output_file
            ])
            
            if returncode == 0:
                success_count += 1
        
        total_time = time.time() - start_time
        average_time = total_time / iterations if iterations > 0 else 0
        
        passed = success_count == iterations
        self._log_result("Rapid consecutive calls", passed,
                        message=f"Completed {success_count}/{iterations} in {total_time:.2f}s ({average_time:.2f}s avg)")
    
    # =========================================================================
    # GENUINE MEMORY & RESOURCE STRESS TESTS
    # =========================================================================
    
    def test_large_image_processing(self):
        """Test processing of large resolution images"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Genuine Resource Stress Testing ==={Colors.RESET}")
        
        # Find largest image and use it for stress
        if not self.test_images:
            self._log_result("Large image processing", False, message="No test images")
            return
        
        largest_image = max(self.test_images, key=lambda x: os.path.getsize(x))
        file_size = os.path.getsize(largest_image)
        
        # Process the large image multiple times to stress resources
        output_file = os.path.join(self.temp_dir, "stress_large.wav")
        returncode, elapsed, stderr = self._run_command([
            "-i", largest_image,
            "-o", output_file
        ])
        
        if returncode == 0 and os.path.exists(output_file):
            output_size = os.path.getsize(output_file)
            passed = elapsed < 60 and output_size > 0
            self._log_result("Large image processing", passed,
                           message=f"Input: {file_size} bytes, Output: {output_size} bytes, Time: {elapsed:.2f}s")
        else:
            self._log_result("Large image processing", False, message=f"Processing failed (exit: {returncode})")
    
    def test_dedicated_stress_images(self):
        """Test processing of dedicated stress test images (9600x5400 resolution)"""
        images_dir = self.images_dir
        stress_images = [
            os.path.join(images_dir, "alt3_test_panel_9600x5400.jpg"),
            os.path.join(images_dir, "alt3_test_panel_9600x5400_large.jpg")
        ]
        
        for image_path in stress_images:
            if os.path.exists(image_path):
                file_size = os.path.getsize(image_path)
                filename = os.path.basename(image_path)
                
                output_file = os.path.join(self.temp_dir, f"stress_{filename.rsplit('.', 1)[0]}.wav")
                returncode, elapsed, _ = self._run_command([
                    "-i", image_path,
                    "-o", output_file
                ])
                
                if returncode == 0 and os.path.exists(output_file):
                    output_size = os.path.getsize(output_file)
                    passed = elapsed < 120 and output_size > 0
                    file_size_mb = file_size / 1024 / 1024
                    output_size_mb = output_size / 1024 / 1024
                    self._log_result(f"Dedicated stress image: {filename}", passed,
                                   message=f"Input: {file_size_mb:.1f}MB, Output: {output_size_mb:.1f}MB, Time: {elapsed:.2f}s")
                else:
                    self._log_result(f"Dedicated stress image: {filename}", False, 
                                   message=f"Processing failed (exit: {returncode})")
    
    def test_max_resolution_protocol_formats(self):
        """Stress test: encode maximum resolution image with all protocols and formats"""
        images_dir = self.images_dir
        max_res_image = os.path.join(images_dir, "alt3_test_panel_9600x5400_large.jpg")
        
        if not os.path.exists(max_res_image):
            self._log_result("Max resolution protocol matrix", False, message="Stress image not found")
            return
        
        protocols = ['m1', 'm2', 's1', 's2', 'sdx']  # Limit for time
        formats = ['wav', 'aiff', 'ogg']
        total_output_size = 0
        success_count = 0
        times = []
        
        start_time = time.time()
        
        for protocol in protocols:
            for fmt in formats:
                output_file = os.path.join(self.temp_dir, f"maxres_{protocol}_{fmt}.{fmt}")
                returncode, elapsed, _ = self._run_command([
                    "-i", max_res_image,
                    "-p", protocol,
                    "-f", fmt,
                    "-o", output_file
                ])
                
                if returncode == 0 and os.path.exists(output_file):
                    success_count += 1
                    times.append(elapsed)
                    total_output_size += os.path.getsize(output_file)
        
        total_time = time.time() - start_time
        matrix_total = len(protocols) * len(formats)
        avg_time = sum(times) / len(times) if times else 0
        
        passed = success_count == matrix_total
        output_mb = total_output_size / 1024 / 1024
        self._log_result("Max resolution protocol/format matrix", passed,
                        message=f"{matrix_total} combinations in {total_time:.2f}s (avg {avg_time:.2f}s), {output_mb:.1f}MB output")
    
    def test_all_format_stress(self):
        """Stress test: process same image in all formats back-to-back"""
        if not self.test_images:
            return
        
        test_image = self.test_images[0]
        formats = ['wav', 'aiff', 'ogg']
        total_output_size = 0
        success_count = 0
        times = []
        
        start_stress = time.time()
        
        for i in range(3):  # Run all formats 3 times each
            for fmt in formats:
                output_file = os.path.join(self.temp_dir, f"stress_fmt_{i}_{fmt}.{fmt}")
                returncode, elapsed, _ = self._run_command([
                    "-i", test_image,
                    "-f", fmt,
                    "-o", output_file
                ])
                
                if returncode == 0 and os.path.exists(output_file):
                    success_count += 1
                    times.append(elapsed)
                    total_output_size += os.path.getsize(output_file)
        
        total_stress_time = time.time() - start_stress
        avg_time = sum(times) / len(times) if times else 0
        
        passed = success_count == 9  # 3 formats × 3 iterations
        self._log_result("All-format stress test", passed,
                        message=f"9 encodings in {total_stress_time:.2f}s (avg {avg_time:.2f}s), {total_output_size} bytes total")
    
    def test_all_protocol_stress(self):
        """Stress test: process same image with all protocols"""
        if not self.test_images:
            return
        
        test_image = self.test_images[0]
        protocols = ['m1', 'm2', 's1', 's2', 'sdx', 'r36', 'r72']
        total_output_size = 0
        success_count = 0
        times = []
        
        start_stress = time.time()
        
        for protocol in protocols:
            output_file = os.path.join(self.temp_dir, f"stress_proto_{protocol}.wav")
            returncode, elapsed, _ = self._run_command([
                "-i", test_image,
                "-p", protocol,
                "-o", output_file
            ])
            
            if returncode == 0 and os.path.exists(output_file):
                success_count += 1
                times.append(elapsed)
                total_output_size += os.path.getsize(output_file)
        
        total_stress_time = time.time() - start_stress
        avg_time = sum(times) / len(times) if times else 0
        
        passed = success_count == len(protocols)
        self._log_result("All-protocol stress test", passed,
                        message=f"{len(protocols)} protocols in {total_stress_time:.2f}s (avg {avg_time:.2f}s), {total_output_size} bytes total")
    
    def test_protocol_format_matrix_stress(self):
        """Stress test: process with all protocol/format combinations"""
        if not self.test_images:
            return
        
        test_image = self.test_images[0]
        protocols = ['m1', 'm2', 's1', 's2', 'sdx']  # Reduced set for manageability
        formats = ['wav', 'aiff', 'ogg']
        total_output_size = 0
        success_count = 0
        times = []
        
        start_stress = time.time()
        matrix_size = len(protocols) * len(formats)
        
        for protocol in protocols:
            for fmt in formats:
                output_file = os.path.join(self.temp_dir, f"stress_matrix_{protocol}_{fmt}.{fmt}")
                returncode, elapsed, _ = self._run_command([
                    "-i", test_image,
                    "-p", protocol,
                    "-f", fmt,
                    "-o", output_file
                ])
                
                if returncode == 0 and os.path.exists(output_file):
                    success_count += 1
                    times.append(elapsed)
                    total_output_size += os.path.getsize(output_file)
        
        total_stress_time = time.time() - start_stress
        avg_time = sum(times) / len(times) if times else 0
        
        passed = success_count == matrix_size
        self._log_result("Protocol/Format matrix stress", passed,
                        message=f"{matrix_size} combinations in {total_stress_time:.2f}s (avg {avg_time:.3f}s), {total_output_size / 1024 / 1024:.1f}MB total")
    
    def test_sustained_load_stress(self):
        """Stress test: sustained load with multiple images"""
        if not self.test_images or len(self.test_images) < 3:
            return
        
        # Use subset of images for sustained processing
        test_set = self.test_images[:5]
        iterations = 3  # Process each image 3 times
        total_output_size = 0
        success_count = 0
        times = []
        
        start_stress = time.time()
        
        for round_num in range(iterations):
            for img_idx, image_path in enumerate(test_set):
                output_file = os.path.join(self.temp_dir, f"stress_sustain_r{round_num}_i{img_idx}.wav")
                returncode, elapsed, _ = self._run_command([
                    "-i", image_path,
                    "-o", output_file
                ])
                
                if returncode == 0 and os.path.exists(output_file):
                    success_count += 1
                    times.append(elapsed)
                    total_output_size += os.path.getsize(output_file)
        
        total_stress_time = time.time() - start_stress
        total_encodings = len(test_set) * iterations
        avg_time = sum(times) / len(times) if times else 0
        throughput = total_encodings / total_stress_time if total_stress_time > 0 else 0
        
        passed = success_count == total_encodings
        self._log_result("Sustained load stress", passed,
                        message=f"{total_encodings} encodings in {total_stress_time:.2f}s ({throughput:.2f} enc/s), {total_output_size / 1024 / 1024:.1f}MB output")
    
    def test_extended_parameters_stress(self):
        """Stress test: encoding with extended CW and output options"""
        if not self.test_images:
            return
        
        test_image = self.test_images[0]
        extended_configs = [
            {"wpm": 10, "tone": 600},   # Slow CW, low tone
            {"wpm": 50, "tone": 1500},  # Fast CW, high tone
            {"wpm": 25, "tone": 800},   # Normal parameters
            {"wpm": 40, "tone": 1000},  # Extended parameters
        ]
        
        success_count = 0
        times = []
        total_output_size = 0
        
        start_stress = time.time()
        
        for idx, config in enumerate(extended_configs):
            output_file = os.path.join(self.temp_dir, f"stress_ext_{idx}.wav")
            returncode, elapsed, _ = self._run_command([
                "-i", test_image,
                "-C", f"N0STRESS/{idx}",
                "-W", str(config["wpm"]),
                "-T", str(config["tone"]),
                "-Z",  # Timestamps for extra logging
                "-o", output_file
            ])
            
            if returncode == 0 and os.path.exists(output_file):
                success_count += 1
                times.append(elapsed)
                total_output_size += os.path.getsize(output_file)
        
        total_stress_time = time.time() - start_stress
        avg_time = sum(times) / len(times) if times else 0
        
        passed = success_count == len(extended_configs)
        self._log_result("Extended parameters stress", passed,
                        message=f"{len(extended_configs)} configs in {total_stress_time:.2f}s (avg {avg_time:.2f}s), {total_output_size} bytes")
    
    # =========================================================================
    # EMPTY STATE AND EDGE CASE PERFORMANCE
    # =========================================================================
    
    def test_repeated_same_image(self):
        """Test encoding the same image multiple times"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Repeated Operations ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        test_image = self.test_images[0]
        iterations = 5
        times = []
        success_count = 0
        
        for i in range(iterations):
            output_file = os.path.join(self.temp_dir, f"repeat_{i}.wav")
            returncode, elapsed, _ = self._run_command([
                "-i", test_image,
                "-o", output_file
            ])
            
            if returncode == 0:
                success_count += 1
                times.append(elapsed)
        
        if times:
            avg_time = sum(times) / len(times)
            min_time = min(times)
            max_time = max(times)
            variance = max_time - min_time
            
            passed = success_count == iterations
            self._log_result("Repeated same image", passed,
                           message=f"Avg: {avg_time:.2f}s, Range: {min_time:.2f}s-{max_time:.2f}s")
        else:
            self._log_result("Repeated same image", False, message="No successful runs")
    
    def test_different_image_sequence(self):
        """Test encoding sequence of different images"""
        if not self.test_images or len(self.test_images) < 3:
            print(f"{Colors.YELLOW}Insufficient test images{Colors.RESET}")
            return
        
        test_sequence = self.test_images[:3]
        times = []
        success_count = 0
        
        for idx, image_path in enumerate(test_sequence):
            output_file = os.path.join(self.temp_dir, f"seq_{idx}.wav")
            returncode, elapsed, _ = self._run_command([
                "-i", image_path,
                "-o", output_file
            ])
            
            if returncode == 0:
                success_count += 1
                times.append(elapsed)
        
        if times:
            total_time = sum(times)
            avg_time = total_time / len(times)
            
            passed = success_count == len(test_sequence)
            self._log_result("Different image sequence", passed,
                           message=f"Total: {total_time:.2f}s, Avg: {avg_time:.2f}s per image")
        else:
            self._log_result("Different image sequence", False, message="No successful runs")
    
    # =========================================================================
    # STABILITY TESTS
    # =========================================================================
    
    def test_stability_without_crashes(self):
        """Test that application remains stable without crashes"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Stability Testing ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        test_image = self.test_images[0]
        crash_count = 0
        success_count = 0
        iterations = 20
        
        for i in range(iterations):
            output_file = os.path.join(self.temp_dir, f"stability_{i}.wav")
            returncode, _, stderr = self._run_command([
                "-i", test_image,
                "-o", output_file
            ])
            
            if returncode == 0:
                success_count += 1
            elif "segmentation" in stderr.lower() or "core dump" in stderr.lower():
                crash_count += 1
        
        passed = crash_count == 0 and success_count >= iterations - 2  # Allow 2 failures
        self._log_result("Crash-free stability", passed,
                        message=f"Crashes: {crash_count}, Success: {success_count}/{iterations}")
    
    def test_memory_leak_indicators(self):
        """Test for memory leak indicators (indirect measurement)"""
        if not self.test_images:
            return
        
        test_image = self.test_images[0]
        iterations = 10
        times = []
        
        # If there's a memory leak, execution time may increase
        for i in range(iterations):
            output_file = os.path.join(self.temp_dir, f"mtest_{i}.wav")
            returncode, elapsed, _ = self._run_command([
                "-i", test_image,
                "-o", output_file
            ])
            
            if returncode == 0:
                times.append(elapsed)
        
        if len(times) >= 5:
            # Check if later runs are significantly slower (potential memory leak)
            early_avg = sum(times[:3]) / 3
            late_avg = sum(times[-3:]) / 3
            increase_percent = ((late_avg - early_avg) / early_avg * 100) if early_avg > 0 else 0
            
            # Allow up to 20% increase due to system variance
            passed = increase_percent < 20
            self._log_result("Memory leak indicators", passed,
                           message=f"Early avg: {early_avg:.2f}s, Late avg: {late_avg:.2f}s ({increase_percent:.1f}% change)")
        else:
            self._log_result("Memory leak indicators", False, message="Insufficient data")
    
    # =========================================================================
    # OUTPUT QUALITY TESTS
    # =========================================================================
    
    def test_output_file_consistency(self):
        """Test that output files are consistent"""
        print(f"\n{Colors.BOLD}{Colors.BLUE}=== Output Quality Verification ==={Colors.RESET}")
        
        if not self.test_images:
            print(f"{Colors.YELLOW}No test images available{Colors.RESET}")
            return
        
        test_image = self.test_images[0]
        
        # Encode same image twice
        output_file1 = os.path.join(self.temp_dir, "consistency1.wav")
        output_file2 = os.path.join(self.temp_dir, "consistency2.wav")
        
        returncode1, _, _ = self._run_command(["-i", test_image, "-o", output_file1])
        returncode2, _, _ = self._run_command(["-i", test_image, "-o", output_file2])
        
        if returncode1 == 0 and returncode2 == 0:
            size1 = os.path.getsize(output_file1)
            size2 = os.path.getsize(output_file2)
            
            # Check if files are identical
            with open(output_file1, 'rb') as f1, open(output_file2, 'rb') as f2:
                content1 = f1.read()
                content2 = f2.read()
                identical = content1 == content2
            
            passed = size1 == size2 and identical
            self._log_result("Output consistency", passed,
                           message=f"Run 1: {size1} bytes, Run 2: {size2} bytes, Identical: {identical}")
        else:
            self._log_result("Output consistency", False, message="Encoding failed")
    
    def test_output_validity(self):
        """Test that output files are valid audio files"""
        if not self.test_images:
            return
        
        test_image = self.test_images[0]
        
        for fmt in ['wav', 'aiff', 'ogg']:
            output_file = os.path.join(self.temp_dir, f"valid_{fmt}.{fmt}")
            returncode, _, _ = self._run_command([
                "-i", test_image,
                "-f", fmt,
                "-o", output_file
            ])
            
            if returncode == 0 and os.path.exists(output_file):
                file_size = os.path.getsize(output_file)
                
                # Check file headers
                with open(output_file, 'rb') as f:
                    header = f.read(4)
                
                valid_header = False
                if fmt == 'wav' and header.startswith(b'RIFF'):
                    valid_header = True
                elif fmt == 'aiff' and header.startswith(b'FORM'):
                    valid_header = True
                elif fmt == 'ogg' and header.startswith(b'OggS'):
                    valid_header = True
                
                passed = valid_header and file_size > 1000
                self._log_result(f"Output validity ({fmt})", passed,
                               message=f"Header valid: {valid_header}, Size: {file_size} bytes")
            else:
                self._log_result(f"Output validity ({fmt})", False, message="Encoding failed")
    
    # =========================================================================
    # MAIN TEST RUNNER
    # =========================================================================
    
    def run_all_tests(self):
        """Execute all performance tests"""
        print(f"\n{Colors.BOLD}{Colors.CYAN}╔════════════════════════════════════════════════════════╗{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}║  PiSSTVpp2 - Performance & Stress Testing             ║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}║  Measuring throughput, stability, and resource usage   ║{Colors.RESET}")
        print(f"{Colors.BOLD}{Colors.CYAN}╚════════════════════════════════════════════════════════╝{Colors.RESET}")
        
        print(f"\n{Colors.CYAN}Test Images Available: {len(self.test_images)}{Colors.RESET}")
        
        start_time = time.time()
        
        try:
            # Performance benchmarks
            self.test_single_image_performance()
            self.test_various_format_performance()
            self.test_protocol_encoding_speed()
            
            # Batch processing
            self.test_sequential_batch_processing()
            self.test_rapid_consecutive_calls()
            
            # Genuine resource stress tests
            self.test_large_image_processing()
            self.test_dedicated_stress_images()
            self.test_max_resolution_protocol_formats()
            self.test_all_format_stress()
            self.test_all_protocol_stress()
            self.test_protocol_format_matrix_stress()
            self.test_sustained_load_stress()
            self.test_extended_parameters_stress()
            
            # Repeated operations
            self.test_repeated_same_image()
            self.test_different_image_sequence()
            
            # Stability and quality
            self.test_stability_without_crashes()
            self.test_memory_leak_indicators()
            self.test_output_file_consistency()
            self.test_output_validity()
        finally:
            import shutil
            try:
                shutil.rmtree(self.temp_dir)
            except:
                pass
        
        total_time = time.time() - start_time
        self._print_summary(total_time)
    
    def _print_summary(self, total_time):
        """Print test summary"""
        total = self.passed + self.failed
        
        print(f"\n{Colors.BOLD}{Colors.BLUE}══════════════════════════════════════════════════════{Colors.RESET}")
        print(f"{Colors.BOLD}Performance Test Summary:{Colors.RESET}")
        print(f"  {Colors.GREEN}Passed:  {self.passed}{Colors.RESET}")
        print(f"  {Colors.RED}Failed:  {self.failed}{Colors.RESET}")
        print(f"  {Colors.BOLD}Total:   {total}{Colors.RESET}")
        print(f"  {Colors.CYAN}Time:    {total_time:.2f}s{Colors.RESET}")
        print(f"{Colors.BLUE}══════════════════════════════════════════════════════{Colors.RESET}")
        
        if self.failed == 0:
            print(f"{Colors.GREEN}{Colors.BOLD}✓ All performance tests passed!{Colors.RESET}")
        else:
            print(f"{Colors.RED}{Colors.BOLD}✗ {self.failed} test(s) failed{Colors.RESET}")

if __name__ == "__main__":
    exe = None
    if len(sys.argv) > 1:
        exe = sys.argv[1]
    else:
        script_dir = Path(__file__).parent.parent.parent
        exe = str(script_dir / "bin" / "pisstvpp2")
    imgs = None
    if len(sys.argv) > 2:
        imgs = sys.argv[2]
    else:
        script_dir = Path(__file__).parent.parent
        imgs = str(script_dir / "images")
    
    try:
        suite = PerformanceTestSuite(exe, imgs)
        suite.run_all_tests()
        sys.exit(0 if suite.failed == 0 else 1)
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.RESET}")
        sys.exit(2)
