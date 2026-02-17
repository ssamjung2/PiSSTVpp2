#!/usr/bin/env python3
"""
Human Verification Framework - Interactive Test Verification for SlowFrame

Provides standardized human verification workflow:
- Interactive terminal prompts with checklists
- Automated image preview (macOS/Linux)
- Session persistence and tracking
- Verdict recording with notes
- Resume interrupted sessions

Usage:
    from human_verification_framework import HumanVerifier
    
    verifier = HumanVerifier()
    verdict = verifier.verify_test(
        test_id="T001",
        metadata=test_metadata,
        images={"output": "output.png"},
        checklist=["Text rendered", "Correct color"]
    )
    verifier.save_session()
"""

import json
import sys
import os
import subprocess
import platform
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Any
from dataclasses import dataclass, asdict


@dataclass
class VerificationResult:
    """Result of a human verification"""
    test_id: str
    verdict: str  # "approved", "rejected", "skipped", "defer"
    verifier_name: str
    timestamp: str
    notes: str = ""
    checklist_results: Dict[str, bool] = None
    
    def __post_init__(self):
        if self.checklist_results is None:
            self.checklist_results = {}


class HumanVerifier:
    """Interactive human verification system"""
    
    def __init__(self, 
                 session_file: Optional[str] = None,
                 auto_preview: bool = True,
                 verifier_name: str = ""):
        """Initialize human verifier
        
        Args:
            session_file: Path to session persistence file
            auto_preview: Automatically open images for preview
            verifier_name: Name of person verifying (optional)
        """
        if session_file is None:
            session_file = "tests/verification_session.json"
        
        self.session_file = Path(session_file)
        self.auto_preview = auto_preview
        self.verifier_name = verifier_name or self._get_default_verifier_name()
        
        self.results: Dict[str, VerificationResult] = {}
        self.session_start = datetime.now()
        self.current_test_index = 0
        
        # Load existing session if available
        self._load_session()
    
    def _get_default_verifier_name(self) -> str:
        """Get default verifier name from environment"""
        return os.environ.get('USER', 'unknown')
    
    def _load_session(self):
        """Load existing verification session"""
        if self.session_file.exists():
            try:
                with open(self.session_file, 'r') as f:
                    data = json.load(f)
                
                # Restore results
                for test_id, result_data in data.get('results', {}).items():
                    self.results[test_id] = VerificationResult(**result_data)
                
                self.current_test_index = data.get('current_test_index', 0)
                
                print(f"📂 Loaded previous session: {len(self.results)} tests verified")
                print(f"   Session file: {self.session_file}")
                
            except Exception as e:
                print(f"Warning: Could not load session: {e}")
    
    def save_session(self):
        """Save verification session to file"""
        self.session_file.parent.mkdir(parents=True, exist_ok=True)
        
        data = {
            "session_start": self.session_start.isoformat(),
            "last_updated": datetime.now().isoformat(),
            "verifier_name": self.verifier_name,
            "current_test_index": self.current_test_index,
            "results": {
                test_id: asdict(result) 
                for test_id, result in self.results.items()
            }
        }
        
        with open(self.session_file, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"💾 Session saved: {self.session_file}")
    
    def get_verification(self, test_id: str) -> Optional[VerificationResult]:
        """Get existing verification result for a test
        
        Args:
            test_id: Test identifier
            
        Returns:
            VerificationResult if exists, None otherwise
        """
        return self.results.get(test_id)
    
    def verify_test(self,
                   test_id: str,
                   test_name: str,
                   metadata: Optional[Any] = None,
                   images: Optional[Dict[str, str]] = None,
                   checklist: Optional[List[str]] = None,
                   command: str = "",
                   auto_approve: bool = False) -> VerificationResult:
        """Verify a single test interactively
        
        Args:
            test_id: Test identifier (e.g., "T001")
            test_name: Human-readable test name
            metadata: Test metadata object
            images: Dict of label -> image path
            checklist: List of verification items
            command: CLI command used
            auto_approve: Skip interaction if test passed automated checks
            
        Returns:
            VerificationResult
        """
        # Check if already verified
        if test_id in self.results:
            print(f"\n✓ {test_id} already verified: {self.results[test_id].verdict}")
            return self.results[test_id]
        
        print("\n" + "=" * 70)
        print(f"🔍 VERIFICATION: {test_id} - {test_name}")
        print("=" * 70)
        
        # Show metadata
        if metadata:
            if hasattr(metadata, 'description'):
                print(f"\n📝 Description: {metadata.description}")
            if hasattr(metadata, 'expected_output'):
                print(f"✓  Expected: {metadata.expected_output}")
        
        # Show command
        if command:
            print(f"\n💻 Command: {command}")
        
        # Preview images
        if images and self.auto_preview:
            print(f"\n🖼️  Opening {len(images)} image(s) for review...")
            for label, path in images.items():
                if path and os.path.exists(path):
                    self._open_image(path, label)
        
        # Show checklist
        checklist_results = {}
        if checklist:
            print("\n📋 Verification Checklist:")
            for i, item in enumerate(checklist, 1):
                print(f"   {i}. □ {item}")
        
        # Auto-approve if requested
        if auto_approve:
            print("\n✅ Auto-approved (passed automated checks)")
            result = VerificationResult(
                test_id=test_id,
                verdict="approved",
                verifier_name="automated",
                timestamp=datetime.now().isoformat(),
                notes="Auto-approved based on automated checks"
            )
            self.results[test_id] = result
            return result
        
        # Interactive verification
        print("\n" + "-" * 70)
        
        # Get verdict
        while True:
            verdict = input("Verdict [a=approve, r=reject, s=skip, d=defer, q=quit]: ").strip().lower()
            
            if verdict == 'q':
                print("\n⏸  Verification paused. Run again to resume.")
                self.save_session()
                sys.exit(0)
            
            if verdict in ['a', 'r', 's', 'd']:
                break
            
            print("Invalid input. Please enter a, r, s, d, or q.")
        
        verdict_map = {
            'a': 'approved',
            'r': 'rejected',
            's': 'skipped',
            'd': 'defer'
        }
        
        verdict_full = verdict_map[verdict]
        
        # Get notes if rejected or deferred
        notes = ""
        if verdict in ['r', 'd']:
            notes = input("Notes (optional): ").strip()
        
        # Create result
        result = VerificationResult(
            test_id=test_id,
            verdict=verdict_full,
            verifier_name=self.verifier_name,
            timestamp=datetime.now().isoformat(),
            notes=notes,
            checklist_results=checklist_results
        )
        
        self.results[test_id] = result
        
        # Auto-save after each verification
        self.save_session()
        
        return result
    
    def batch_verify(self,
                    tests: List[Dict[str, Any]],
                    start_index: Optional[int] = None) -> Dict[str, VerificationResult]:
        """Verify multiple tests interactively
        
        Args:
            tests: List of test dictionaries with keys:
                   test_id, test_name, metadata, images, checklist, command
            start_index: Index to start from (resume support)
            
        Returns:
            Dict of test_id -> VerificationResult
        """
        if start_index is None:
            start_index = self.current_test_index
        
        total = len(tests)
        
        print(f"\n{'='*70}")
        print(f"🎯 BATCH VERIFICATION: {total} tests")
        print(f"   Starting at index: {start_index}")
        print(f"   Already verified: {len(self.results)}")
        print(f"{'='*70}")
        
        for i in range(start_index, total):
            test = tests[i]
            self.current_test_index = i
            
            print(f"\n[{i+1}/{total}] Processing {test.get('test_id', f'Test {i+1}')}")
            
            result = self.verify_test(
                test_id=test.get('test_id', f"T{i+1:03d}"),
                test_name=test.get('test_name', f"Test {i+1}"),
                metadata=test.get('metadata'),
                images=test.get('images', {}),
                checklist=test.get('checklist', []),
                command=test.get('command', ""),
                auto_approve=test.get('auto_approve', False)
            )
        
        self.current_test_index = total
        self.save_session()
        
        return self.results
    
    def _open_image(self, image_path: str, label: str = ""):
        """Open image for preview using platform-appropriate viewer
        
        Args:
            image_path: Path to image file
            label: Label for the image
        """
        if not os.path.exists(image_path):
            print(f"   ⚠️  Image not found: {image_path}")
            return
        
        system = platform.system()
        
        try:
            if system == "Darwin":  # macOS
                subprocess.run(["open", image_path], check=False)
            elif system == "Linux":
                subprocess.run(["xdg-open", image_path], check=False)
            elif system == "Windows":
                os.startfile(image_path)
            else:
                print(f"   ℹ️  Manual review required: {image_path}")
                
        except Exception as e:
            print(f"   ⚠️  Could not open image: {e}")
            print(f"      Path: {image_path}")
    
    def get_summary(self) -> Dict[str, Any]:
        """Get verification summary statistics
        
        Returns:
            Dict with summary statistics
        """
        total = len(self.results)
        approved = sum(1 for r in self.results.values() if r.verdict == "approved")
        rejected = sum(1 for r in self.results.values() if r.verdict == "rejected")
        skipped = sum(1 for r in self.results.values() if r.verdict == "skipped")
        deferred = sum(1 for r in self.results.values() if r.verdict == "defer")
        
        return {
            "total_verified": total,
            "approved": approved,
            "rejected": rejected,
            "skipped": skipped,
            "deferred": deferred,
            "verifier": self.verifier_name,
            "session_start": self.session_start.isoformat()
        }
    
    def print_summary(self):
        """Print verification summary"""
        summary = self.get_summary()
        
        print("\n" + "=" * 70)
        print("📊 VERIFICATION SUMMARY")
        print("=" * 70)
        print(f"Total Verified:  {summary['total_verified']}")
        print(f"  ✅ Approved:   {summary['approved']}")
        print(f"  ❌ Rejected:   {summary['rejected']}")
        print(f"  ⊘  Skipped:    {summary['skipped']}")
        print(f"  ⏳ Deferred:   {summary['deferred']}")
        print(f"\nVerifier: {summary['verifier']}")
        print(f"Session Started: {summary['session_start']}")
        print("=" * 70)
    
    def export_results(self, output_file: str):
        """Export verification results to JSON
        
        Args:
            output_file: Path to output JSON file
        """
        data = {
            "summary": self.get_summary(),
            "results": {
                test_id: asdict(result)
                for test_id, result in self.results.items()
            }
        }
        
        with open(output_file, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"📄 Results exported: {output_file}")


def main():
    """Example/test usage"""
    verifier = HumanVerifier()
    
    # Example test verification
    result = verifier.verify_test(
        test_id="T001",
        test_name="Example Test",
        images={"output": "output.png"},
        checklist=[
            "Image dimensions correct",
            "Colors look accurate",
            "No visual artifacts"
        ],
        command="./bin/slowframe -i input.png -o output.png"
    )
    
    print(f"\nResult: {result.verdict}")
    
    verifier.print_summary()
    verifier.save_session()


if __name__ == "__main__":
    main()
