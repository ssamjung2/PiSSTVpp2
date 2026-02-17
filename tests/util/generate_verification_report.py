#!/usr/bin/env python3
"""
Generate Human Verification Report for SlowFrame Tests

Creates an interactive HTML report showing:
- Test case intent and parameters
- Input/output images side-by-side
- Expected vs actual behavior
- Manual verification checklist
- Notes field for documenting issues

Usage:
    venv/bin/python3 tests/generate_verification_report.py
    venv/bin/python3 tests/generate_verification_report.py --suite aspect
    venv/bin/python3 tests/generate_verification_report.py --output reports/verification.html
"""

import sys
import json
import subprocess
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional
import base64

# Test case descriptions and expected behaviors
TEST_INTENT = {
    # Aspect Ratio Tests
    "center_m1_wide": {
        "intent": "CENTER mode with wide source image (wider than target)",
        "input": "2000x1125 (16:9) color bars with embedded black regions",
        "target": "320x256 (5:4) Martin M1",
        "expected": "Image cropped from sides to match 5:4 aspect, then scaled. No padding added. Source black bars preserved proportionally.",
        "verify": [
            "No black padding on edges (beyond source content)",
            "Vertical color bars centered and cropped from sides",
            "Dimensions exactly 320x256",
            "Center portion of source visible"
        ]
    },
    "center_m1_tall": {
        "intent": "CENTER mode with tall source image (taller than target)",
        "input": "680x1209 (9:16) color bars",
        "target": "320x256 (5:4) Martin M1",
        "expected": "Image cropped from top/bottom to match 5:4 aspect, then scaled. No padding.",
        "verify": [
            "No black padding on edges",
            "Center portion of source visible",
            "Dimensions exactly 320x256",
            "Cropped equally from top and bottom"
        ]
    },
    "pad_m1_wide": {
        "intent": "PAD mode with wide source (letterbox test)",
        "input": "2000x1125 (16:9) color bars",
        "target": "320x256 (5:4) Martin M1",
        "expected": "Source scaled to fit width (320px), black bars added top/bottom to reach 256px height",
        "verify": [
            "Black letterbox bars on top and bottom",
            "Full width of source visible (320px)",
            "Bars symmetrical (equal top and bottom)",
            "No distortion of source aspect ratio",
            "Total dimensions 320x256"
        ]
    },
    "pad_m1_tall": {
        "intent": "PAD mode with tall source (pillarbox test)",
        "input": "680x1209 (9:16) color bars",
        "target": "320x256 (5:4) Martin M1",
        "expected": "Source scaled to fit height (256px), black bars added left/right to reach 320px width",
        "verify": [
            "Black pillarbox bars on left and right",
            "Full height of source visible (256px)",
            "Bars symmetrical (equal left and right)",
            "No distortion of source aspect ratio",
            "Total dimensions 320x256"
        ]
    },
    "stretch_m1_wide": {
        "intent": "STRETCH mode with wide source (distortion test)",
        "input": "2000x1125 (16:9) color bars",
        "target": "320x256 (5:4) Martin M1",
        "expected": "Non-uniform scaling to exactly 320x256. Aspect ratio changed, image appears compressed horizontally.",
        "verify": [
            "No black padding added (source black content preserved)",
            "Dimensions exactly 320x256",
            "Circles appear stretched/elliptical if present",
            "Full source content visible but distorted",
            "Vertical bars appear narrower than source"
        ]
    },
    "stretch_m1_tall": {
        "intent": "STRETCH mode with tall source",
        "input": "680x1209 (9:16) color bars",
        "target": "320x256 (5:4) Martin M1",
        "expected": "Non-uniform scaling. Image appears stretched horizontally (wider than source aspect).",
        "verify": [
            "No padding added",
            "Dimensions exactly 320x256",
            "Image appears wider/compressed vertically",
            "All source content visible"
        ]
    },
    
    # Text Overlay Tests
    "overlay_basic": {
        "intent": "Basic text overlay with default styling",
        "input": "320x256 color bars",
        "params": "-T 'Test:Hello SSTV'",
        "expected": "Blue text on white background in bottom-left corner",
        "verify": [
            "Text readable: 'Test' and 'Hello SSTV'",
            "Blue text color (SSTV standard)",
            "White background bar behind text",
            "Positioned at bottom-left",
            "Text does not obscure critical image content"
        ]
    },
    "overlay_positioning": {
        "intent": "Text overlay positioning test",
        "input": "320x256 color bars",
        "params": "-T 'pos:tr|Test:TOP-RIGHT'",
        "expected": "Text positioned in top-right corner",
        "verify": [
            "Text in top-right corner",
            "Proper spacing from edges",
            "Background bar present",
            "Text fully visible (not clipped)"
        ]
    },
    "overlay_multi_line": {
        "intent": "Multi-line text overlay",
        "input": "320x256 color bars",
        "params": "-T 'Test:Line1' -T 'Test:Line2|pos:bc'",
        "expected": "Multiple text overlays at different positions",
        "verify": [
            "Multiple text elements visible",
            "Each at specified position",
            "No overlap between text elements",
            "All text readable"
        ]
    },
    "overlay_background_bar": {
        "intent": "Full-width background bar for signal protection",
        "input": "320x256 color bars",
        "params": "-T 'bgbar:true|bgbarw:full|Test:PROTECTED'",
        "expected": "Full-width background bar across image",
        "verify": [
            "Background bar spans full width",
            "Bar height matches text + padding",
            "Text centered on bar",
            "Bar color correct (default or specified)",
            "Enhances text visibility"
        ]
    },
    
    # Image Format Tests
    "format_png": {
        "intent": "PNG format support verification",
        "input": "PNG with transparency",
        "expected": "PNG loaded correctly, transparency converted to opaque background",
        "verify": [
            "Image loads without errors",
            "Transparent areas converted to black/white",
            "Colors preserved accurately",
            "No artifacts from format conversion"
        ]
    },
    "format_jpeg": {
        "intent": "JPEG format support",
        "input": "Standard JPEG image",
        "expected": "JPEG loaded and processed correctly",
        "verify": [
            "Image loads without errors",
            "Colors accurate despite JPEG compression",
            "No excessive blocking artifacts",
            "Aspect ratio preserved"
        ]
    },
    "format_gif": {
        "intent": "GIF format support (animated GIF uses first frame)",
        "input": "GIF image (potentially animated)",
        "expected": "First frame loaded if animated, static GIF processed normally",
        "verify": [
            "Image loads successfully",
            "Only first frame used for animated GIFs",
            "Palette-based colors converted to RGB",
            "No dithering artifacts"
        ]
    },
}


class VerificationReportGenerator:
    """Generate interactive HTML verification report"""
    
    def __init__(self, output_dir: Path = Path("verification_reports")):
        self.output_dir = output_dir
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.test_results = []
        
    def add_test_result(self, test_name: str, test_data: dict):
        """Add test result for inclusion in report"""
        self.test_results.append({
            'name': test_name,
            'data': test_data
        })
    
    def image_to_base64(self, image_path: Path) -> str:
        """Convert image to base64 for embedding in HTML"""
        if not image_path.exists():
            return ""
        try:
            with open(image_path, 'rb') as f:
                return base64.b64encode(f.read()).decode('utf-8')
        except Exception as e:
            print(f"Warning: Could not encode {image_path}: {e}")
            return ""
    
    def generate_html(self, output_file: Path, manifest: dict = None):
        """Generate interactive HTML verification report"""
        
        # Use manifest data if provided, otherwise use collected test results
        if not manifest:
            manifest = {"tests": {}, "version": "2.1.0"}
        
        html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SlowFrame Test Verification Report</title>
    <style>
        * {{ box-sizing: border-box; }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background: #f5f5f5;
            color: #333;
        }}
        .header {{
            background: white;
            padding: 30px;
            border-radius: 8px;
            margin-bottom: 30px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        h1 {{
            margin: 0 0 10px 0;
            color: #2c3e50;
        }}
        .subtitle {{
            color: #7f8c8d;
            margin: 5px 0;
        }}
        .test-card {{
            background: white;
            padding: 25px;
            margin-bottom: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            page-break-inside: avoid;
        }}
        .test-header {{
            display: flex;
            justify-content: space-between;
            align-items: start;
            margin-bottom: 20px;
            padding-bottom: 15px;
            border-bottom: 2px solid #ecf0f1;
        }}
        .test-title {{
            font-size: 1.3em;
            font-weight: 600;
            color: #2c3e50;
            margin: 0;
        }}
        .test-status {{
            padding: 6px 16px;
            border-radius: 20px;
            font-weight: 600;
            font-size: 0.9em;
        }}
        .status-pass {{ background: #d4edda; color: #155724; }}
        .status-fail {{ background: #f8d7da; color: #721c24; }}
        .status-review {{ background: #fff3cd; color: #856404; }}
        
        .test-intent {{
            background: #f8f9fa;
            padding: 15px;
            border-left: 4px solid #3498db;
            margin: 15px 0;
            border-radius: 4px;
        }}
        .intent-label {{
            font-weight: 600;
            color: #2c3e50;
            margin-bottom: 8px;
        }}
        .intent-text {{
            color: #555;
            line-height: 1.6;
        }}
        
        .params {{
            background: #f8f9fa;
            padding: 12px;
            border-radius: 4px;
            font-family: 'Monaco', 'Courier New', monospace;
            font-size: 0.9em;
            margin: 15px 0;
            overflow-x: auto;
        }}
        .param-label {{
            font-weight: 600;
            color: #2c3e50;
            margin-bottom: 5px;
        }}
        
        .verification-checklist {{
            margin: 20px 0;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 4px;
        }}
        .checklist-title {{
            font-weight: 600;
            margin-bottom: 12px;
            color: #2c3e50;
        }}
        .checklist-item {{
            padding: 8px 0;
            display: flex;
            align-items: center;
        }}
        .checklist-item input {{
            margin-right: 10px;
        }}
        
        .image-comparison {{
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin: 20px 0;
        }}
        .image-panel {{
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            overflow: hidden;
            background: #fafafa;
        }}
        .image-panel-header {{
            background: #ecf0f1;
            padding: 12px;
            font-weight: 600;
            color: #2c3e50;
            border-bottom: 2px solid #bdc3c7;
        }}
        .image-container {{
            padding: 15px;
            text-align: center;
            background: #fff;
        }}
        .image-container img {{
            max-width: 100%;
            height: auto;
            border: 1px solid #ddd;
            border-radius: 4px;
            image-rendering: pixelated;
        }}
        .image-info {{
            margin-top: 10px;
            font-size: 0.9em;
            color: #7f8c8d;
        }}
        
        .notes-section {{
            margin-top: 25px;
            padding-top: 20px;
            border-top: 2px solid #ecf0f1;
        }}
        .notes-title {{
            font-weight: 600;
            margin-bottom: 10px;
            color: #2c3e50;
        }}
        .notes-input {{
            width: 100%;
            min-height: 100px;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 4px;
            font-family: inherit;
            font-size: 0.95em;
            resize: vertical;
        }}
        .notes-input:focus {{
            outline: none;
            border-color: #3498db;
        }}
        
        .verdict {{
            display: flex;
            gap: 15px;
            margin-top: 15px;
        }}
        .verdict-btn {{
            flex: 1;
            padding: 12px;
            border: 2px solid;
            border-radius: 6px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
        }}
        .verdict-pass {{
            background: #d4edda;
            border-color: #28a745;
            color: #155724;
        }}
        .verdict-pass:hover {{
            background: #28a745;
            color: white;
        }}
        .verdict-fail {{
            background: #f8d7da;
            border-color: #dc3545;
            color: #721c24;
        }}
        .verdict-fail:hover {{
            background: #dc3545;
            color: white;
        }}
        
        .summary {{
            background: white;
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }}
        .summary-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-top: 15px;
        }}
        .summary-stat {{
            text-align: center;
            padding: 15px;
            background: #f8f9fa;
            border-radius: 6px;
        }}
        .stat-value {{
            font-size: 2em;
            font-weight: bold;
            color: #3498db;
        }}
        .stat-label {{
            color: #7f8c8d;
            margin-top: 5px;
        }}
        
        @media print {{
            body {{ background: white; }}
            .test-card {{ page-break-inside: avoid; }}
        }}
        
        @media (max-width: 768px) {{
            .image-comparison {{
                grid-template-columns: 1fr;
            }}
        }}
    </style>
    <script>
        function saveVerification(testName) {{
            const notes = document.getElementById('notes-' + testName).value;
            const verdict = document.querySelector('input[name="verdict-' + testName + '"]:checked')?.value;
            
            // Save to localStorage
            const key = 'verification-' + testName;
            localStorage.setItem(key, JSON.stringify({{
                notes: notes,
                verdict: verdict,
                timestamp: new Date().toISOString()
            }}));
            
            alert('Verification saved for ' + testName);
        }}
        
        function loadVerification(testName) {{
            const key = 'verification-' + testName;
            const data = localStorage.getItem(key);
            if (data) {{
                const parsed = JSON.parse(data);
                document.getElementById('notes-' + testName).value = parsed.notes || '';
                if (parsed.verdict) {{
                    const radio = document.querySelector('input[name="verdict-' + testName + '"][value="' + parsed.verdict + '"]');
                    if (radio) radio.checked = true;
                }}
            }}
        }}
        
        function exportVerifications() {{
            const results = {{}};
            document.querySelectorAll('.test-card').forEach(card => {{
                const testName = card.dataset.testName;
                const key = 'verification-' + testName;
                const data = localStorage.getItem(key);
                if (data) {{
                    results[testName] = JSON.parse(data);
                }}
            }});
            
            const blob = new Blob([JSON.stringify(results, null, 2)], {{type: 'application/json'}});
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = 'slowframe_verification_results.json';
            a.click();
        }}
        
        window.onload = function() {{
            document.querySelectorAll('.test-card').forEach(card => {{
                loadVerification(card.dataset.testName);
            }});
        }};
    </script>
</head>
<body>
    <div class="header">
        <h1>🔍 SlowFrame Test Verification Report</h1>
        <div class="subtitle">Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</div>
        <div class="subtitle">Version: 2.1.0 | Human Review Required</div>
    </div>
    
    <div class="summary">
        <h2>📊 Test Summary</h2>
        <div class="summary-grid">
            <div class="summary-stat">
                <div class="stat-value" id="total-tests">0</div>
                <div class="stat-label">Total Tests</div>
            </div>
            <div class="summary-stat">
                <div class="stat-value" id="reviewed-tests">0</div>
                <div class="stat-label">Reviewed</div>
            </div>
            <div class="summary-stat">
                <div class="stat-value" id="pending-tests">0</div>
                <div class="stat-label">Pending</div>
            </div>
        </div>
        <button onclick="exportVerifications()" style="margin-top: 20px; padding: 10px 20px; background: #3498db; color: white; border: none; border-radius: 6px; font-weight: 600; cursor: pointer;">
            💾 Export Verification Results
        </button>
    </div>
"""
        
        # Generate test cards
        test_count = 0
        
        # Use manifest data if available
        if manifest and "tests" in manifest:
            test_items = manifest["tests"].items()
        else:
            test_items = [(t['name'], t['data']) for t in self.test_results]
        
        for test_name, test_data in test_items:
            test_count += 1
            
            intent_info = TEST_INTENT.get(test_name, {
                "intent": "Test case documentation pending",
                "expected": "See test parameters",
                "verify": []
            })
            
            input_img = test_data.get('input_image') or test_data.get('input')
            output_img = test_data.get('output_image') or test_data.get('output')
            
            html += f"""
    <div class="test-card" data-test-name="{test_name}">
        <div class="test-header">
            <h2 class="test-title">#{test_count}: {test_name.replace('_', ' ').title()}</h2>
            <span class="test-status status-review">⏳ NEEDS REVIEW</span>
        </div>
        
        <div class="test-intent">
            <div class="intent-label">🎯 Test Intent:</div>
            <div class="intent-text">{intent_info.get('intent', 'N/A')}</div>
        </div>
        
        <div class="params">
            <div class="param-label">📋 Test Parameters:</div>
            <div>Input: {intent_info.get('input', 'N/A')}</div>
            <div>Target: {intent_info.get('target', test_data.get('target', 'N/A'))}</div>
"""
            
            if 'mode' in test_data:
                html += f"            <div>Mode: {test_data.get('mode', 'N/A')}</div>\n"
            if 'protocol' in test_data:
                html += f"            <div>Protocol: {test_data.get('protocol', 'N/A')}</div>\n"
            if 'aspect' in test_data:
                html += f"            <div>Aspect: {test_data.get('aspect', 'N/A')}</div>\n"
            
            html += """        </div>
        
        <div class="test-intent">
            <div class="intent-label">✅ Expected Behavior:</div>
            <div class="intent-text">{0}</div>
        </div>
""".format(intent_info.get('expected', 'N/A'))
            
            if intent_info.get('verify'):
                html += """
        <div class="verification-checklist">
            <div class="checklist-title">🔍 Verification Checklist:</div>
"""
                for i, item in enumerate(intent_info['verify']):
                    html += f"""
            <div class="checklist-item">
                <input type="checkbox" id="check-{test_name}-{i}">
                <label for="check-{test_name}-{i}">{item}</label>
            </div>
"""
                html += """
        </div>
"""
            
            # Add image comparison if available
            if input_img or output_img:
                html += """
        <div class="image-comparison">
"""
                if input_img and Path(input_img).exists():
                    input_b64 = self.image_to_base64(Path(input_img))
                    html += f"""
            <div class="image-panel">
                <div class="image-panel-header">📥 Input Image</div>
                <div class="image-container">
"""
                    if input_b64:
                        html += f"""
                    <img src="data:image/png;base64,{input_b64}" alt="Input">
"""
                    else:
                        html += f"""
                    <p style="color: #999;">Could not load image</p>
"""
                    html += f"""
                    <div class="image-info">{Path(input_img).name}</div>
                </div>
            </div>
"""
                
                if output_img and Path(output_img).exists():
                    output_b64 = self.image_to_base64(Path(output_img))
                    html += f"""
            <div class="image-panel">
                <div class="image-panel-header">📤 Output Image</div>
                <div class="image-container">
"""
                    if output_b64:
                        html += f"""
                    <img src="data:image/png;base64,{output_b64}" alt="Output">
"""
                    else:
                        html += f"""
                    <p style="color: #999;">Could not load image</p>
"""
                    html += f"""
                    <div class="image-info">{Path(output_img).name}</div>
                </div>
            </div>
"""
                html += """
        </div>
"""
            
            html += f"""
        <div class="notes-section">
            <div class="notes-title">📝 Verification Notes:</div>
            <textarea class="notes-input" id="notes-{test_name}" placeholder="Document your findings here...&#10;- What looks correct?&#10;- Any issues found?&#10;- Specific observations?"></textarea>
            
            <div class="verdict">
                <label class="verdict-btn verdict-pass">
                    <input type="radio" name="verdict-{test_name}" value="pass" style="margin-right: 8px;">
                    ✅ PASS - Verified Correct
                </label>
                <label class="verdict-btn verdict-fail">
                    <input type="radio" name="verdict-{test_name}" value="fail" style="margin-right: 8px;">
                    ❌ FAIL - Issues Found
                </label>
            </div>
            
            <button onclick="saveVerification('{test_name}')" style="width: 100%; margin-top: 15px; padding: 10px; background: #3498db; color: white; border: none; border-radius: 6px; font-weight: 600; cursor: pointer;">
                💾 Save Verification
            </button>
        </div>
    </div>
"""
        
        html += f"""
    <script>
        document.getElementById('total-tests').textContent = {test_count};
    </script>
</body>
</html>
"""
        
        with open(output_file, 'w') as f:
            f.write(html)
        
        print(f"✅ Verification report generated: {output_file}")
        print(f"   {test_count} tests included")
        print(f"\n📖 To review:")
        print(f"   open {output_file}")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Generate human verification report")
    parser.add_argument('--output', '-o', default='verification_reports/verification_report.html',
                       help='Output HTML file path')
    parser.add_argument('--manifest', '-m', default='test_manifest.json',
                       help='Test manifest JSON file')
    parser.add_argument('--suite', '-s', choices=['aspect', 'overlay', 'formats', 'all'],
                       default='all', help='Test suite to include')
    
    args = parser.parse_args()
    
    # Load manifest
    manifest_file = Path(args.manifest)
    if not manifest_file.exists():
        print(f"❌ Manifest file not found: {manifest_file}")
        print(f"   Run: python3 generate_test_manifest.py first")
        sys.exit(1)
    
    with open(manifest_file) as f:
        manifest = json.load(f)
    
    generator = VerificationReportGenerator()
    
    # Filter tests by suite
    suite_map = {
        'aspect': 'aspect_ratio',
        'overlay': 'text_overlay',
        'formats': 'image_format',
    }
    
    test_count = 0
    for test_name, test_data in manifest["tests"].items():
        category = test_data.get("category")
        
        # Skip if not matching suite filter
        if args.suite != 'all':
            target_category = suite_map.get(args.suite)
            if category != target_category:
                continue
        
        # Only add tests that have output images
        if test_data.get("output_exists"):
            test_count += 1
            generator.add_test_result(test_name, {
                'input': test_data.get('input_image'),
                'output': test_data.get('output_image'),
                'category': category,
                **{k: v for k, v in test_data.items() if k not in ['input_image', 'output_image', 'category', 'input_exists', 'output_exists']}
            })
    
    if test_count == 0:
        print(f"❌ No tests found in manifest matching suite: {args.suite}")
        sys.exit(1)
    
    # Generate report - need to update the generator to use manifest data
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    generator.generate_html(output_path, manifest)
    print(f"✅ Verification report generated: {output_path}")
    print(f"   {test_count} tests included")


if __name__ == '__main__':
    main()
