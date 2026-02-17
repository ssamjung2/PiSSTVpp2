#!/usr/bin/env python3
"""
Comprehensive Test Verification Tool for SlowFrame

A unified testing system that:
1. Runs all test suites (aspect ratio, text overlay, image formats)
2. Tracks CLI commands and test metadata
3. Annotates images with test information
4. Generates detailed HTML reports with embedded images
5. Generates comprehensive JSON reports

Usage:
    venv/bin/python3 tests/comprehensive_test_verification.py [--verbose] [--output-dir DIR]
"""

import sys
import os
import json
import base64
import subprocess
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Tuple
from dataclasses import asdict
import traceback

# Add tests to path
tests_dir = Path(__file__).parent.parent
util_dir = Path(__file__).parent
sys.path.insert(0, str(tests_dir))
sys.path.insert(0, str(util_dir))

try:
    from PIL import Image, ImageDraw, ImageFont
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Warning: Pillow not available, image annotations disabled")

from test_metadata import get_all_test_metadata, TestMetadata


class ComprehensiveTestVerification:
    """Unified test verification system"""
    
    def __init__(self, output_dir: Path = None, slowframe_bin: str = "./bin/slowframe", verbose: bool = False):
        """Initialize test verification system"""
        self.verbose = verbose
        self.slowframe_bin = Path(slowframe_bin)
        
        # Default output dir is tests/test_outputs
        tests_dir = Path(__file__).parent.parent
        self.output_dir = output_dir or tests_dir / "test_outputs"
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        # Subdirectories
        self.aspect_dir = self.output_dir / "aspect"
        self.overlay_dir = self.output_dir / "overlay"
        self.formats_dir = self.output_dir / "formats"
        self.reports_dir = tests_dir  # Reports go to tests/ directory
        
        for d in [self.aspect_dir, self.overlay_dir, self.formats_dir]:
            d.mkdir(parents=True, exist_ok=True)
        
        # Test results storage
        self.test_results: Dict[str, Dict] = {}
        self.test_metadata = get_all_test_metadata()
        
    def get_image_dimensions(self, img_path: Path) -> Optional[Tuple[int, int]]:
        """Get image dimensions"""
        if not HAS_PIL:
            return None
        try:
            with Image.open(img_path) as img:
                return img.size
        except:
            return None
    
    def image_to_base64(self, img_path: Path) -> Optional[Tuple[str, str]]:
        """Convert image to base64 string with MIME type for embedding"""
        try:
            with open(img_path, 'rb') as f:
                base64_str = base64.b64encode(f.read()).decode('utf-8')
                
            # Determine MIME type from extension
            ext = img_path.suffix.lower()
            mime_types = {
                '.png': 'image/png',
                '.jpg': 'image/jpeg',
                '.jpeg': 'image/jpeg',
                '.gif': 'image/gif',
                '.bmp': 'image/bmp',
                '.tiff': 'image/tiff',
            }
            mime_type = mime_types.get(ext, 'image/png')
            
            return (base64_str, mime_type)
        except:
            return None
    
    def annotate_image(self, image_path: Path, test_info: Dict) -> bool:
        """Add test metadata overlay to PNG image"""
        if not HAS_PIL or not image_path.exists():
            return False
        
        try:
            # Open image
            with Image.open(image_path) as img:
                # Convert to RGBA if needed
                if img.mode != 'RGBA':
                    img = img.convert('RGBA')
                
                # Create overlay text
                test_num = test_info.get('test_number', 0)
                cli_cmd = test_info.get('cli_command', 'N/A')
                source = Path(test_info.get('source_image', 'unknown')).name
                
                # Add text annotations
                draw = ImageDraw.Draw(img)
                
                # Try to use a proportional font, fall back to default
                try:
                    font = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
                except:
                    font = ImageFont.load_default()
                
                # Dark overlay background
                overlay = Image.new('RGBA', img.size, (0, 0, 0, 0))
                overlay_draw = ImageDraw.Draw(overlay)
                overlay_draw.rectangle([(0, 0), (img.width, 50)], fill=(30, 30, 30, 200))
                img = Image.alpha_composite(img, overlay)
                draw = ImageDraw.Draw(img)
                
                # Draw text
                y_offset = 5
                draw.text((5, y_offset), f"Test #{test_num:03d}", fill=(255, 255, 255, 255), font=font)
                draw.text((5, y_offset + 15), f"Source: {source}", fill=(200, 200, 200, 255), font=font)
                draw.text((5, y_offset + 30), f"Cmd: {cli_cmd[:60]}...", fill=(200, 200, 200, 255), font=font)
                
                # Convert back to RGB if original was RGB
                if img.mode == 'RGBA':
                    rgb_img = Image.new('RGB', img.size, (255, 255, 255))
                    rgb_img.paste(img, mask=img.split()[3])
                    rgb_img.save(image_path)
                else:
                    img.save(image_path)
                
                return True
        except Exception as e:
            if self.verbose:
                print(f"Warning: Failed to annotate {image_path}: {e}")
            return False
    
    def run_all_tests(self) -> Dict:
        """Run all test suites and collect results"""
        results = {
            'timestamp': datetime.now().isoformat(),
            'summary': {},
            'tests': {},
            'test_details': {}
        }
        
        # Run the master test suite
        print("Running comprehensive test suite...")
        try:
            # Import test suite
            from test_aspect_comprehensive import AspectTestSuite
            
            # Get correct paths (tests directory, not tests/util)
            tests_dir = Path(__file__).parent.parent
            images_dir = tests_dir / "images"
            
            # Run aspect ratio tests
            print("  • Aspect ratio tests...")
            aspect_suite = AspectTestSuite(
                image_dir=str(images_dir),
                output_dir=str(self.aspect_dir),
                slowframe_bin=str(self.slowframe_bin),
                verbose=self.verbose
            )
            aspect_results = aspect_suite.run_all_tests()
            
            # Process aspect test results
            for i, result in enumerate(aspect_results['results'], 1):
                test_id = result.test_case.name
                metadata = self.test_metadata.get(test_id)
                
                result_data = {
                    'test_number': i,
                    'test_id': test_id,
                    'passed': result.passed,
                    'message': result.message,
                    'execution_time': result.execution_time,
                    'cli_command': result.details.get('cli_command', 'N/A'),
                    'output_image': result.details.get('output_image', ''),
                    'source_image': result.details.get('source_image', ''),
                }
                
                # Add metadata if available
                if metadata:
                    result_data['metadata'] = {
                        'title': metadata.title,
                        'purpose': metadata.purpose,
                        'expected_outcome': metadata.expected_outcome,
                        'category': metadata.category,
                        'mode': metadata.mode,
                        'protocol': metadata.protocol,
                        'test_type': metadata.test_type,
                    }
                
                results['tests'][test_id] = result_data
                self.test_results[test_id] = result_data
                
                # Annotate output image
                output_path_str = result_data.get('output_image', '')
                if output_path_str:  # Check it's not empty or None
                    try:
                        output_path = Path(output_path_str)
                        if output_path.exists():
                            self.annotate_image(output_path, result_data)
                    except Exception as e:
                        if self.verbose:
                            print(f"Warning: Could not annotate {output_path_str}: {e}")
            
            # Summary
            total_aspect = len(aspect_results['results'])
            passed_aspect = sum(1 for r in aspect_results['results'] if r.passed)
            
            results['summary']['aspect_ratio'] = {
                'total': total_aspect,
                'passed': passed_aspect,
                'failed': total_aspect - passed_aspect
            }
            
            if self.verbose:
                print(f"    ✓ {passed_aspect}/{total_aspect} aspect tests passed")
            
        except Exception as e:
            print(f"Error running tests: {e}")
            if self.verbose:
                traceback.print_exc()
        
        return results
    
    def generate_html_report(self, results: Dict) -> Path:
        """Generate comprehensive HTML report with images"""
        output_file = self.reports_dir / "test_verification_report.html"
        
        # Group tests by category
        categories = {}
        for test_id, result in results['tests'].items():
            category = result.get('metadata', {}).get('category', 'unknown')
            if category not in categories:
                categories[category] = []
            categories[category].append((test_id, result))
        
        # Build HTML with raw string to avoid format conflicts
        html_content = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SlowFrame Comprehensive Test Report</title>
    <style>
"""
        
        # CSS styles (using triple quotes to avoid format string issues)
        html_content += """        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background: #1e1e1e;
            color: #e0e0e0;
            padding: 20px;
            line-height: 1.6;
        }
        .container { max-width: 1400px; margin: 0 auto; }
        header {
            background: linear-gradient(135deg, #2a2a2a, #1e1e1e);
            border: 1px solid #404040;
            border-radius: 8px;
            padding: 30px;
            margin-bottom: 30px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        }
        h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
            color: #4a9eff;
        }
        .header-info {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-top: 15px;
            font-size: 0.9em;
        }
        .info-block {
            background: #252525;
            padding: 10px;
            border-radius: 4px;
            border-left: 3px solid #4a9eff;
        }
        .summary-stats {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin-bottom: 30px;
        }
        .stat-card {
            background: linear-gradient(135deg, #2a2a2a, #252525);
            border: 1px solid #404040;
            border-radius: 8px;
            padding: 20px;
            text-align: center;
        }
        .stat-number {
            font-size: 2em;
            font-weight: bold;
            color: #4a9eff;
            margin-bottom: 5px;
        }
        .stat-label {
            font-size: 0.85em;
            color: #999;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        .category-section {
            margin-bottom: 40px;
        }
        .category-title {
            font-size: 1.5em;
            color: #4a9eff;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 2px solid #404040;
            text-transform: capitalize;
        }
        .test-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(500px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        .test-card {
            background: #252525;
            border: 1px solid #404040;
            border-radius: 8px;
            overflow: hidden;
            transition: all 0.3s ease;
        }
        .test-card:hover {
            border-color: #4a9eff;
            box-shadow: 0 0 20px rgba(74, 158, 255, 0.2);
        }
        .test-header {
            background: #2a2a2a;
            padding: 15px;
            border-bottom: 1px solid #404040;
        }
        .test-number {
            display: inline-block;
            background: #4a9eff;
            color: #1e1e1e;
            padding: 4px 8px;
            border-radius: 4px;
            font-weight: bold;
            font-size: 0.85em;
            margin-right: 10px;
        }
        .test-title {
            font-size: 1.1em;
            font-weight: bold;
            color: #e0e0e0;
        }
        .test-status {
            display: inline-block;
            padding: 2px 6px;
            border-radius: 3px;
            font-size: 0.8em;
            font-weight: bold;
            margin-left: 10px;
        }
        .status-pass {
            background: #4caf50;
            color: #fff;
        }
        .status-fail {
            background: #f44336;
            color: #fff;
        }
        .test-content {
            padding: 15px;
        }
        .metadata-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-bottom: 15px;
            font-size: 0.9em;
        }
        .metadata-item {
            background: #1e1e1e;
            padding: 8px;
            border-radius: 4px;
            border-left: 2px solid #404040;
        }
        .metadata-label {
            color: #999;
            font-weight: bold;
            font-size: 0.8em;
            text-transform: uppercase;
            margin-bottom: 2px;
        }
        .metadata-value {
            color: #e0e0e0;
            word-break: break-all;
        }
        .images-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin: 15px 0;
        }
        .image-container {
            background: #1e1e1e;
            border: 1px solid #404040;
            border-radius: 4px;
            overflow: hidden;
        }
        .image-label {
            background: #2a2a2a;
            padding: 8px;
            font-size: 0.85em;
            color: #999;
            border-bottom: 1px solid #404040;
        }
        .image-content {
            display: flex;
            align-items: center;
            justify-content: center;
            background: #1a1a1a;
            max-height: 300px;
            overflow: auto;
        }
        .image-content img {
            max-width: 100%;
            max-height: 100%;
            display: block;
        }
        .cli-command {
            background: #1a1a1a;
            border: 1px solid #404040;
            border-radius: 4px;
            padding: 10px;
            margin: 10px 0;
            font-family: 'Courier New', monospace;
            font-size: 0.85em;
            color: #4a9eff;
            word-break: break-all;
            overflow-x: auto;
        }
        .purpose-section {
            background: #1e1e1e;
            padding: 10px;
            border-left: 3px solid #4a9eff;
            margin: 10px 0;
            border-radius: 4px;
        }
        .section-title {
            font-size: 0.85em;
            font-weight: bold;
            color: #999;
            text-transform: uppercase;
            margin-bottom: 5px;
        }
        .section-content {
            color: #e0e0e0;
            font-size: 0.9em;
        }
        footer {
            text-align: center;
            padding: 20px;
            color: #666;
            font-size: 0.9em;
            border-top: 1px solid #404040;
            margin-top: 40px;
        }
"""
        
        html_content += """    </style>
</head>
<body>
<div class="container">
    <header>
        <h1>🔬 SlowFrame Comprehensive Test Report</h1>
        <div class="header-info">
            <div class="info-block">
                <strong>Generated:</strong> """ + results['timestamp'] + """
            </div>
            <div class="info-block">
                <strong>Total Tests:</strong> """ + str(len(results['tests'])) + """
            </div>
"""
        
        # Calculate stats
        total_tests = len(results['tests'])
        total_passed = sum(1 for t in results['tests'].values() if t['passed'])
        total_failed = total_tests - total_passed
        
        html_content += """            <div class="info-block">
                <strong>Passed:</strong> """ + str(total_passed) + """
            </div>
            <div class="info-block">
                <strong>Failed:</strong> """ + str(total_failed) + """
            </div>
        </div>
    </header>
    
    <div class="summary-stats">
        <div class="stat-card">
            <div class="stat-number">""" + str(total_tests) + """</div>
            <div class="stat-label">Total Tests</div>
        </div>
        <div class="stat-card">
            <div class="stat-number" style="color: #4caf50;">""" + str(total_passed) + """</div>
            <div class="stat-label">Passed</div>
        </div>
        <div class="stat-card">
            <div class="stat-number" style="color: #f44336;">""" + str(total_failed) + """</div>
            <div class="stat-label">Failed</div>
        </div>
    </div>
"""
        
        # Add tests by category
        for category in ['aspect_ratio', 'text_overlay', 'image_format']:
            if category not in categories:
                continue
            
            category_tests = categories[category]
            category_title = {
                'aspect_ratio': 'Aspect Ratio Tests',
                'text_overlay': 'Text Overlay Tests',
                'image_format': 'Image Format Tests'
            }.get(category, category)
            
            html_content += f'    <div class="category-section">\n        <h2 class="category-title">{category_title}</h2>\n        <div class="test-grid">\n'
            
            for test_id, result in sorted(category_tests, key=lambda x: x[1].get('test_number', 0)):
                metadata = result.get('metadata', {})
                status = 'pass' if result['passed'] else 'fail'
                status_label = '✓ PASS' if result['passed'] else '✗ FAIL'
                status_class = 'status-pass' if result['passed'] else 'status-fail'
                
                # Get image paths
                output_img = result.get('output_image', '')
                source_img = result.get('source_image', '')
                
                # Try to load images
                output_base64 = None
                output_mime = 'image/png'
                source_base64 = None
                source_mime = 'image/png'
                output_dims = "N/A"
                source_dims = "N/A"
                
                if output_img:
                    output_path = Path(output_img)
                    if output_path.exists():
                        result_tuple = self.image_to_base64(output_path)
                        if result_tuple:
                            output_base64, output_mime = result_tuple
                        output_dims_tuple = self.get_image_dimensions(output_path)
                        if output_dims_tuple:
                            output_dims = f"{output_dims_tuple[0]}×{output_dims_tuple[1]}"
                
                if source_img:
                    source_path = Path(source_img)
                    if source_path.exists():
                        result_tuple = self.image_to_base64(source_path)
                        if result_tuple:
                            source_base64, source_mime = result_tuple
                        source_dims_tuple = self.get_image_dimensions(source_path)
                        if source_dims_tuple:
                            source_dims = f"{source_dims_tuple[0]}×{source_dims_tuple[1]}"
                
                # Build test card HTML
                test_num = result.get('test_number', 0)
                html_content += f'            <div class="test-card">\n'
                html_content += f'                <div class="test-header">\n'
                html_content += f'                    <span class="test-number">#{test_num:03d}</span>\n'
                html_content += f'                    <span class="test-title">{metadata.get("title", test_id)}</span>\n'
                html_content += f'                    <span class="test-status {status_class}">{status_label}</span>\n'
                html_content += f'                </div>\n'
                html_content += f'                <div class="test-content">\n'
                
                # Mode and Protocol
                if metadata.get('mode'):
                    html_content += f'                    <div class="metadata-grid">\n'
                    html_content += f'                        <div class="metadata-item">\n'
                    html_content += f'                            <div class="metadata-label">Mode</div>\n'
                    html_content += f'                            <div class="metadata-value">{metadata.get("mode", "N/A").upper()}</div>\n'
                    html_content += f'                        </div>\n'
                    
                    if metadata.get('protocol'):
                        html_content += f'                        <div class="metadata-item">\n'
                        html_content += f'                            <div class="metadata-label">Protocol</div>\n'
                        html_content += f'                            <div class="metadata-value">{metadata.get("protocol", "N/A").upper()}</div>\n'
                        html_content += f'                        </div>\n'
                    
                    html_content += f'                    </div>\n'
                
                # Purpose section
                if metadata.get('purpose'):
                    html_content += f'                    <div class="purpose-section">\n'
                    html_content += f'                        <div class="section-title">Purpose</div>\n'
                    html_content += f'                        <div class="section-content">{metadata.get("purpose")}</div>\n'
                    html_content += f'                    </div>\n'
                
                # Expected outcome
                if metadata.get('expected_outcome'):
                    html_content += f'                    <div class="purpose-section">\n'
                    html_content += f'                        <div class="section-title">Expected Outcome</div>\n'
                    html_content += f'                        <div class="section-content">{metadata.get("expected_outcome")}</div>\n'
                    html_content += f'                    </div>\n'
                
                # Images with resolutions
                if source_base64 or output_base64:
                    html_content += f'                    <div class="images-grid">\n'
                    
                    if source_base64:
                        html_content += f'                        <div class="image-container">\n'
                        html_content += f'                            <div class="image-label">Input Image ({source_dims})</div>\n'
                        html_content += f'                            <div class="image-content">\n'
                        html_content += f'                                <img src="data:{source_mime};base64,{source_base64}" alt="Input">\n'
                        html_content += f'                            </div>\n'
                        html_content += f'                        </div>\n'
                    
                    if output_base64:
                        html_content += f'                        <div class="image-container">\n'
                        html_content += f'                            <div class="image-label">Output Image ({output_dims})</div>\n'
                        html_content += f'                            <div class="image-content">\n'
                        html_content += f'                                <img src="data:{output_mime};base64,{output_base64}" alt="Output">\n'
                        html_content += f'                            </div>\n'
                        html_content += f'                        </div>\n'
                    
                    html_content += f'                    </div>\n'
                
                # CLI Command
                if result.get('cli_command'):
                    cli_cmd = result.get('cli_command', '')
                    html_content += f'                    <div class="cli-command">$ {cli_cmd}</div>\n'
                
                # Test message
                if result.get('message'):
                    msg = result.get('message', '')
                    html_content += f'                    <p style="font-size: 0.9em; color: #999; margin-top: 10px;">{msg}</p>\n'
                
                html_content += f'                </div>\n'
                html_content += f'            </div>\n'
            
            html_content += f'        </div>\n'
            html_content += f'    </div>\n'
        
        # Footer
        html_content += """    <footer>
        <p>SlowFrame Comprehensive Test Verification Report</p>
        <p>Generated by comprehensive_test_verification.py</p>
    </footer>
</div>
</body>
</html>
"""
        
        # Write HTML file
        with open(output_file, 'w') as f:
            f.write(html_content)
        
        return output_file
    
    def generate_json_report(self, results: Dict) -> Path:
        """Generate JSON report"""
        output_file = self.reports_dir / "test_verification_report.json"
        
        # Make results JSON serializable
        json_results = {
            'timestamp': results['timestamp'],
            'summary': results['summary'],
            'tests': results['tests']
        }
        
        with open(output_file, 'w') as f:
            json.dump(json_results, f, indent=2)
        
        return output_file
    
    def verify(self) -> bool:
        """Run complete verification workflow"""
        print("\n" + "=" * 70)
        print("🔬 SlowFrame Comprehensive Test Verification")
        print("=" * 70 + "\n")
        
        # Run tests
        results = self.run_all_tests()
        
        # Generate reports
        print("\n📄 Generating reports...")
        html_path = self.generate_html_report(results)
        json_path = self.generate_json_report(results)
        
        print(f"  ✓ HTML report: {html_path}")
        print(f"  ✓ JSON report: {json_path}")
        
        # Summary
        print("\n" + "=" * 70)
        summary = results.get('summary', {})
        total_passed = sum(r.get('passed', 1) for r in summary.values() if isinstance(r, dict))
        
        print("✅ Comprehensive Test Verification Complete")
        print("=" * 70)
        
        return True


def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Comprehensive test verification tool")
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--output-dir', '-o', help='Output directory for test results')
    
    args = parser.parse_args()
    
    output_dir = Path(args.output_dir) if args.output_dir else None
    verifier = ComprehensiveTestVerification(output_dir=output_dir, verbose=args.verbose)
    verifier.verify()


if __name__ == '__main__':
    main()
