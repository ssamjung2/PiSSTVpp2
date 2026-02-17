#!/usr/bin/env python3
"""
Test Report Generator - Shared Reporting Infrastructure for SlowFrame Tests

Provides unified HTML and JSON report generation with:
- Embedded images (base64)
- Image annotation overlay
- Test metadata integration
- CLI command tracking
- Human verification checklists
- Pass/fail visual indicators

Usage:
    from test_report_generator import TestReportGenerator
    
    generator = TestReportGenerator("SlowFrame Aspect Tests")
    generator.add_test_result(
        test_id="T001",
        metadata=test_metadata,
        command="./bin/slowframe -i input.png -o output.wav",
        output_files={"output": "output.png"},
        status="passed"
    )
    generator.generate_html_report("report.html")
    generator.generate_json_report("report.json")
"""

import json
import base64
import os
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass, asdict, field

try:
    from PIL import Image, ImageDraw, ImageFont
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


@dataclass
class TestResult:
    """Individual test result"""
    test_id: str
    name: str
    suite: str
    status: str  # "passed", "failed", "skipped", "pending"
    command: str
    timestamp: str
    execution_time: float = 0.0
    
    # Test metadata
    description: str = ""
    input_image: Optional[str] = None
    expected_behavior: str = ""
    
    # Output files
    output_files: Dict[str, str] = field(default_factory=dict)
    
    # Automated checks
    automated_checks: Dict[str, Any] = field(default_factory=dict)
    
    # Human verification
    verification_checklist: List[str] = field(default_factory=list)
    human_verified: bool = False
    human_verdict: Optional[str] = None  # "approved", "rejected", "pending"
    human_notes: str = ""
    verifier_name: str = ""
    verification_date: Optional[str] = None
    
    # Error information
    error_message: str = ""
    error_code: Optional[int] = None


class TestReportGenerator:
    """Generates comprehensive HTML and JSON test reports"""
    
    def __init__(self, report_title: str = "SlowFrame Test Report"):
        """Initialize report generator
        
        Args:
            report_title: Title for the report
        """
        self.title = report_title
        self.tests: List[TestResult] = []
        self.generation_date = datetime.now().isoformat()
        self.summary = {
            "total": 0,
            "passed": 0,
            "failed": 0,
            "skipped": 0,
            "pending": 0,
            "human_verified": 0,
            "verification_pending": 0
        }
        
    def add_test_result(self,
                       test_id: str,
                       name: str,
                       suite: str,
                       status: str,
                       command: str,
                       metadata: Optional[Any] = None,
                       output_files: Optional[Dict[str, str]] = None,
                       automated_checks: Optional[Dict[str, Any]] = None,
                       execution_time: float = 0.0,
                       error_message: str = "",
                       error_code: Optional[int] = None) -> TestResult:
        """Add a test result to the report
        
        Args:
            test_id: Unique test identifier (e.g., "T001")
            name: Test name
            suite: Test suite name
            status: Test status ("passed", "failed", "skipped", "pending")
            command: CLI command used
            metadata: Test metadata object (optional)
            output_files: Dict of file type -> path
            automated_checks: Dict of check results
            execution_time: Test execution time in seconds
            error_message: Error message if failed
            error_code: Error code if failed
            
        Returns:
            TestResult object
        """
        result = TestResult(
            test_id=test_id,
            name=name,
            suite=suite,
            status=status,
            command=command,
            timestamp=datetime.now().isoformat(),
            execution_time=execution_time,
            output_files=output_files or {},
            automated_checks=automated_checks or {},
            error_message=error_message,
            error_code=error_code
        )
        
        # Extract metadata if provided
        if metadata:
            if hasattr(metadata, 'description'):
                result.description = metadata.description
            if hasattr(metadata, 'input_image'):
                result.input_image = metadata.input_image
            if hasattr(metadata, 'expected_output'):
                result.expected_behavior = metadata.expected_output
            if hasattr(metadata, 'verification_checklist'):
                result.verification_checklist = metadata.verification_checklist
        
        self.tests.append(result)
        self._update_summary(result)
        
        return result
    
    def _update_summary(self, result: TestResult):
        """Update summary statistics"""
        self.summary["total"] += 1
        self.summary[result.status] = self.summary.get(result.status, 0) + 1
        
        if result.human_verified:
            self.summary["human_verified"] += 1
        elif result.verification_checklist:
            self.summary["verification_pending"] += 1
    
    def annotate_image(self,
                      image_path: str,
                      output_path: str,
                      test_id: str,
                      annotations: Dict[str, str],
                      font_size: int = 14) -> bool:
        """Annotate an image with test information
        
        Args:
            image_path: Path to source image
            output_path: Path to save annotated image
            test_id: Test identifier
            annotations: Dict of label -> value to annotate
            font_size: Font size for annotations
            
        Returns:
            True if successful, False otherwise
        """
        if not HAS_PIL:
            return False
        
        try:
            img = Image.open(image_path)
            draw = ImageDraw.Draw(img)
            
            # Try to load a font, fall back to default
            try:
                font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", font_size)
            except:
                font = ImageFont.load_default()
            
            # Add test ID banner at top
            banner_height = 30
            banner = Image.new('RGB', (img.width, banner_height), color=(40, 40, 40))
            banner_draw = ImageDraw.Draw(banner)
            banner_draw.text((10, 8), f"Test: {test_id}", fill=(255, 255, 255), font=font)
            
            # Combine banner with image
            combined = Image.new('RGB', (img.width, img.height + banner_height))
            combined.paste(banner, (0, 0))
            combined.paste(img, (0, banner_height))
            
            # Add annotations at bottom
            if annotations:
                draw = ImageDraw.Draw(combined)
                y_offset = img.height + banner_height - 20 * len(annotations) - 10
                
                for label, value in annotations.items():
                    text = f"{label}: {value}"
                    # Add semi-transparent background
                    bbox = draw.textbbox((10, y_offset), text, font=font)
                    draw.rectangle(bbox, fill=(0, 0, 0, 128))
                    draw.text((10, y_offset), text, fill=(255, 255, 255), font=font)
                    y_offset += 20
            
            combined.save(output_path)
            return True
            
        except Exception as e:
            print(f"Warning: Could not annotate image: {e}")
            return False
    
    def image_to_base64(self, image_path: str) -> Tuple[str, str]:
        """Convert image to base64 string with MIME type
        
        Args:
            image_path: Path to image file
            
        Returns:
            Tuple of (base64_string, mime_type)
        """
        if not os.path.exists(image_path):
            return "", ""
        
        # Detect MIME type from extension
        ext = os.path.splitext(image_path)[1].lower()
        mime_types = {
            '.png': 'image/png',
            '.jpg': 'image/jpeg',
            '.jpeg': 'image/jpeg',
            '.gif': 'image/gif',
            '.webp': 'image/webp',
            '.bmp': 'image/bmp',
            '.tiff': 'image/tiff',
            '.tif': 'image/tiff'
        }
        mime_type = mime_types.get(ext, 'image/png')
        
        try:
            with open(image_path, 'rb') as f:
                encoded = base64.b64encode(f.read()).decode('utf-8')
            return encoded, mime_type
        except Exception as e:
            print(f"Warning: Could not encode image {image_path}: {e}")
            return "", mime_type
    
    def generate_html_report(self, output_file: str):
        """Generate comprehensive HTML report
        
        Args:
            output_file: Path to output HTML file
        """
        html = self._generate_html_content()
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html)
        
        print(f"HTML report generated: {output_file}")
    
    def generate_json_report(self, output_file: str):
        """Generate JSON report
        
        Args:
            output_file: Path to output JSON file
        """
        report = {
            "report_title": self.title,
            "generation_date": self.generation_date,
            "summary": self.summary,
            "tests": [asdict(test) for test in self.tests]
        }
        
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, indent=2)
        
        print(f"JSON report generated: {output_file}")
        print(f"  Total: {self.summary['total']}, Passed: {self.summary['passed']}, "
              f"Failed: {self.summary['failed']}")
    
    def _generate_html_content(self) -> str:
        """Generate HTML content"""
        
        # Generate test sections
        test_sections = []
        for test in self.tests:
            test_sections.append(self._generate_test_section(test))
        
        # Status icon mapper
        status_icons = {
            "passed": "✅",
            "failed": "❌",
            "skipped": "⊘",
            "pending": "⏳"
        }
        
        html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{self.title}</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            background: #f5f5f5;
            padding: 20px;
        }}
        .container {{
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            overflow: hidden;
        }}
        header {{
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
        }}
        header h1 {{
            font-size: 28px;
            margin-bottom: 10px;
        }}
        header .meta {{
            opacity: 0.9;
            font-size: 14px;
        }}
        .summary {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 20px;
            padding: 30px;
            background: #f9f9f9;
            border-bottom: 1px solid #e0e0e0;
        }}
        .summary-card {{
            background: white;
            padding: 20px;
            border-radius: 6px;
            text-align: center;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
        }}
        .summary-card .number {{
            font-size: 32px;
            font-weight: bold;
            margin-bottom: 5px;
        }}
        .summary-card .label {{
            color: #666;
            font-size: 13px;
            text-transform: uppercase;
        }}
        .summary-card.passed .number {{ color: #22c55e; }}
        .summary-card.failed .number {{ color: #ef4444; }}
        .summary-card.total .number {{ color: #3b82f6; }}
        .summary-card.pending .number {{ color: #f59e0b; }}
        
        .test-section {{
            padding: 30px;
            border-bottom: 1px solid #e0e0e0;
        }}
        .test-section:last-child {{
            border-bottom: none;
        }}
        .test-header {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }}
        .test-title {{
            font-size: 20px;
            font-weight: 600;
        }}
        .test-status {{
            padding: 6px 12px;
            border-radius: 20px;
            font-size: 14px;
            font-weight: 500;
        }}
        .test-status.passed {{ background: #dcfce7; color: #166534; }}
        .test-status.failed {{ background: #fee2e2; color: #991b1b; }}
        .test-status.skipped {{ background: #f3f4f6; color: #6b7280; }}
        .test-status.pending {{ background: #fef3c7; color: #92400e; }}
        
        .test-metadata {{
            background: #f9fafb;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
            font-size: 14px;
        }}
        .test-metadata .label {{
            font-weight: 600;
            color: #4b5563;
        }}
        
        .command-block {{
            background: #1e293b;
            color: #e2e8f0;
            padding: 15px;
            border-radius: 6px;
            font-family: 'Monaco', 'Courier New', monospace;
            font-size: 12px;
            overflow-x: auto;
            margin-bottom: 20px;
        }}
        
        .images-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }}
        .image-container {{
            text-align: center;
        }}
        .image-container img {{
            max-width: 100%;
            border: 1px solid #e5e7eb;
            border-radius: 6px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
        }}
        .image-label {{
            margin-top: 10px;
            font-size: 13px;
            font-weight: 600;
            color: #6b7280;
        }}
        
        .checklist {{
            background: #fffbeb;
            padding: 15px;
            border-radius: 6px;
            border-left: 4px solid #f59e0b;
            margin-bottom: 20px;
        }}
        .checklist-title {{
            font-weight: 600;
            margin-bottom: 10px;
            color: #92400e;
        }}
        .checklist-item {{
            padding: 5px 0;
            font-size: 14px;
        }}
        .checklist-item::before {{
            content: "□ ";
            font-weight: bold;
            margin-right: 8px;
        }}
        
        .verification-status {{
            background: #dbeafe;
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
        }}
        .verification-status.approved {{
            background: #dcfce7;
            border-left: 4px solid #22c55e;
        }}
        .verification-status.rejected {{
            background: #fee2e2;
            border-left: 4px solid #ef4444;
        }}
        
        .automated-checks {{
            margin-bottom: 20px;
        }}
        .check-item {{
            padding: 8px 12px;
            margin: 5px 0;
            border-radius: 4px;
            font-size: 14px;
        }}
        .check-item.passed {{
            background: #dcfce7;
            color: #166534;
        }}
        .check-item.failed {{
            background: #fee2e2;
            color: #991b1b;
        }}
        
        .error-message {{
            background: #fee2e2;
            color: #991b1b;
            padding: 15px;
            border-radius: 6px;
            border-left: 4px solid #ef4444;
            margin-bottom: 20px;
            font-family: monospace;
            font-size: 13px;
        }}
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>{self.title}</h1>
            <div class="meta">
                Generated: {datetime.fromisoformat(self.generation_date).strftime('%B %d, %Y at %I:%M %p')}
            </div>
        </header>
        
        <div class="summary">
            <div class="summary-card total">
                <div class="number">{self.summary['total']}</div>
                <div class="label">Total Tests</div>
            </div>
            <div class="summary-card passed">
                <div class="number">{self.summary.get('passed', 0)}</div>
                <div class="label">Passed</div>
            </div>
            <div class="summary-card failed">
                <div class="number">{self.summary.get('failed', 0)}</div>
                <div class="label">Failed</div>
            </div>
            <div class="summary-card pending">
                <div class="number">{self.summary.get('verification_pending', 0)}</div>
                <div class="label">Verification Pending</div>
            </div>
        </div>
        
        {''.join(test_sections)}
    </div>
</body>
</html>"""
        
        return html
    
    def _generate_test_section(self, test: TestResult) -> str:
        """Generate HTML section for a single test"""
        
        status_icon = {"passed": "✅", "failed": "❌", "skipped": "⊘", "pending": "⏳"}
        icon = status_icon.get(test.status, "❓")
        
        # Generate metadata section
        metadata_html = ""
        if test.description or test.expected_behavior:
            metadata_html = f"""
            <div class="test-metadata">
                {f'<div><span class="label">Description:</span> {test.description}</div>' if test.description else ''}
                {f'<div><span class="label">Expected:</span> {test.expected_behavior}</div>' if test.expected_behavior else ''}
                {f'<div><span class="label">Execution Time:</span> {test.execution_time:.3f}s</div>' if test.execution_time else ''}
            </div>
            """
        
        # Generate images section
        images_html = ""
        if test.output_files:
            image_items = []
            for label, path in test.output_files.items():
                if path and os.path.exists(path):
                    b64, mime = self.image_to_base64(path)
                    if b64:
                        image_items.append(f"""
                        <div class="image-container">
                            <img src="data:{mime};base64,{b64}" alt="{label}">
                            <div class="image-label">{label}</div>
                        </div>
                        """)
            
            if image_items:
                images_html = f'<div class="images-grid">{"".join(image_items)}</div>'
        
        # Generate checklist
        checklist_html = ""
        if test.verification_checklist:
            items = "".join([f'<div class="checklist-item">{item}</div>' 
                           for item in test.verification_checklist])
            checklist_html = f"""
            <div class="checklist">
                <div class="checklist-title">Human Verification Checklist:</div>
                {items}
            </div>
            """
        
        # Generate verification status
        verification_html = ""
        if test.human_verified:
            status_class = "approved" if test.human_verdict == "approved" else "rejected"
            verification_html = f"""
            <div class="verification-status {status_class}">
                <strong>Human Verification:</strong> {test.human_verdict.upper()}
                {f'<div>Verifier: {test.verifier_name}</div>' if test.verifier_name else ''}
                {f'<div>Date: {test.verification_date}</div>' if test.verification_date else ''}
                {f'<div>Notes: {test.human_notes}</div>' if test.human_notes else ''}
            </div>
            """
        
        # Generate automated checks
        checks_html = ""
        if test.automated_checks:
            check_items = []
            for check_name, result in test.automated_checks.items():
                status_class = "passed" if result else "failed"
                icon = "✓" if result else "✗"
                check_items.append(f'<div class="check-item {status_class}">{icon} {check_name}</div>')
            
            checks_html = f"""
            <div class="automated-checks">
                <strong>Automated Checks:</strong>
                {"".join(check_items)}
            </div>
            """
        
        # Generate error message
        error_html = ""
        if test.error_message:
            error_html = f"""
            <div class="error-message">
                <strong>Error:</strong><br>
                {test.error_message}
                {f'<div>Exit Code: {test.error_code}</div>' if test.error_code else ''}
            </div>
            """
        
        return f"""
        <div class="test-section">
            <div class="test-header">
                <div class="test-title">{icon} {test.test_id}: {test.name}</div>
                <div class="test-status {test.status}">{test.status.upper()}</div>
            </div>
            
            {metadata_html}
            
            <div class="command-block">
                $ {test.command}
            </div>
            
            {error_html}
            {checks_html}
            {images_html}
            {checklist_html}
            {verification_html}
        </div>
        """


if __name__ == "__main__":
    # Example usage
    generator = TestReportGenerator("Example Test Report")
    
    generator.add_test_result(
        test_id="T001",
        name="example_test",
        suite="aspect",
        status="passed",
        command="./bin/slowframe -i input.png -o output.wav",
        execution_time=0.15
    )
    
    generator.generate_html_report("example_report.html")
    generator.generate_json_report("example_report.json")
    
    print("\nExample reports generated!")
