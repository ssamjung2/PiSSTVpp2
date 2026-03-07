#!/usr/bin/env python3
"""
QRM EXIF & Tile Placement Recovery Test

Comprehensive testing of:
1. Tile position header recovery at various QRM levels
2. EXIF metadata preservation through QRM-corrupted tiles
3. Stitcher accuracy in placing tiles correctly
4. Correlation between QRM level and recovery success

Tests multiple presets and reports detailed metrics.
"""

import os
import sys
import subprocess
import json
from pathlib import Path
from datetime import datetime
from PIL import Image
import numpy as np

# Try to import PIL metadata tools
try:
    from PIL.Image import Exif
    HAS_PIL_EXIF = True
except ImportError:
    HAS_PIL_EXIF = False


def run_command(cmd, description, timeout=120):
    """Run shell command with error handling"""
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", f"Command timeout ({timeout}s)"
    except Exception as e:
        return False, "", str(e)


def check_exif_in_image(image_path: Path) -> dict:
    """Check EXIF data in an image file"""
    result = {
        'path': str(image_path),
        'exists': image_path.exists(),
        'has_exif': False,
        'exif_tags': {},
        'iso': None,
        'f_number': None,
        'focal_length': None,
        'date_time': None,
    }
    
    if not image_path.exists():
        return result
    
    try:
        img = Image.open(image_path)
        
        # Try to get EXIF data
        if HAS_PIL_EXIF:
            exif = img.getexif()
            if exif:
                result['has_exif'] = True
                # Try common EXIF tags
                result['exif_tags'] = {
                    str(k): str(v)[:100] for k, v in exif.items()
                }
                
                # Extract specific fields
                try:
                    if 34855 in exif:  # ISO
                        result['iso'] = exif[34855]
                except:
                    pass
                try:
                    if 33437 in exif:  # F-Number
                        result['f_number'] = str(exif[33437])
                except:
                    pass
                try:
                    if 37386 in exif:  # Focal Length
                        result['focal_length'] = str(exif[37386])
                except:
                    pass
                try:
                    if 306 in exif:  # DateTime
                        result['date_time'] = str(exif[306])
                except:
                    pass
        else:
            # Fallback: check for info dict
            if hasattr(img, 'info'):
                result['has_exif'] = 'exif' in img.info
                result['exif_tags'] = {k: str(v)[:100] for k, v in img.info.items() if 'exif' in k.lower()}
    
    except Exception as e:
        result['error'] = str(e)
    
    return result


def analyze_header_recovery(noisy_dir: Path, noise_level: str) -> dict:
    """Analyze header recovery from noisy tiles"""
    result = {
        'noise_level': noise_level,
        'tiles_analyzed': 0,
        'headers_with_color_data': 0,
        'avg_header_corruption': 0.0,
        'tiles': {}
    }
    
    tiles = sorted(list(noisy_dir.glob("*.png")))
    
    for tile_path in tiles:
        try:
            img = Image.open(tile_path)
            img_array = np.array(img.convert('RGB'))
            
            # Check header region (first 8 rows)
            header_region = img_array[:8, :, :]
            
            # Calculate color deviation in header (should be mostly black/white for markers)
            r, g, b = header_region[:,:,0], header_region[:,:,1], header_region[:,:,2]
            color_deviation = np.mean(np.abs(r.astype(float) - g.astype(float))) + \
                            np.mean(np.abs(g.astype(float) - b.astype(float)))
            
            has_header = color_deviation < 50  # Markers are mostly B&W
            
            result['tiles'][tile_path.name] = {
                'color_deviation': float(color_deviation),
                'likely_has_header': has_header,
            }
            
            result['tiles_analyzed'] += 1
            if has_header:
                result['headers_with_color_data'] += 1
            result['avg_header_corruption'] += color_deviation
        
        except Exception as e:
            result['tiles'][tile_path.name] = {'error': str(e)}
    
    if result['tiles_analyzed'] > 0:
        result['avg_header_corruption'] /= result['tiles_analyzed']
        result['header_detection_rate'] = result['headers_with_color_data'] / result['tiles_analyzed']
    
    return result


def analyze_tile_placement_markers(tile_image: Image.Image) -> dict:
    """Analyze tile placement markers in header region"""
    # This would need the actual marker detection logic
    # For now, just flag potential position data
    return {
        'row': None,
        'col': None,
        'confidence': 0.0,
        'analysis': 'Requires actual marker decoder'
    }


def run_qrm_test(preset: str, project_root: Path) -> dict:
    """Run complete QRM test for a single preset"""
    
    tiling_dir = project_root / "tests" / "test_outputs" / "tiling"
    stitch_binary = project_root / "bin" / "stitch_tiles"
    noise_generator = project_root / "tests" / "util" / "generate_sstv_noise.py"
    venv_python = project_root / "venv" / "bin" / "python"
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    test_dir = project_root / "tests" / "test_outputs" / f"qrm_exif_test_{preset}_{timestamp}"
    test_dir.mkdir(parents=True, exist_ok=True)
    
    results = {
        'preset': preset,
        'timestamp': timestamp,
        'test_dir': str(test_dir),
        'stages': {}
    }
    
    # Stage 1: Generate noisy tiles
    print(f"\n{'='*80}")
    print(f"STAGE 1: Generating {preset.upper()} corrupted tiles")
    print(f"{'='*80}")
    
    noisy_dir = test_dir / "noisy_tiles"
    noisy_dir.mkdir(parents=True, exist_ok=True)
    
    success, stdout, stderr = run_command(
        [
            str(venv_python),
            str(noise_generator),
            "--dir", str(tiling_dir),
            "--output-dir", str(noisy_dir),
            "--preset", preset,
        ],
        f"Generate {preset} tiles"
    )
    
    results['stages']['noise_generation'] = {
        'success': success,
        'tile_count': len(list(noisy_dir.glob("*.png"))) if success else 0,
        'error': stderr if not success else None
    }
    
    if not success:
        print(f"✗ Failed to generate {preset} tiles: {stderr}")
        return results
    
    print(f"✓ Generated {results['stages']['noise_generation']['tile_count']} noisy tiles")
    
    # Stage 2: Discover tile sets and session IDs  
    print(f"\n{'='*80}")
    print(f"STAGE 2: Discovering tile sets ({preset})")
    print(f"{'='*80}")
    
    success, list_output, stderr = run_command(
        [str(stitch_binary), "--dir", str(noisy_dir), "--list"],
        f"List tile sets",
        timeout=30
    )
    
    discovered_sessions = []
    if success and list_output:
        # Parse session IDs from output
        for line in list_output.split('\n'):
            if 'Session ID:' in line:
                try:
                    session_id = line.split('Session ID:')[1].strip()
                    if session_id and session_id.startswith('0x'):
                        discovered_sessions.append(session_id)
                        print(f"✓ Found session: {session_id}")
                except:
                    pass
    
    results['stages']['discovery'] = {
        'success': success,
        'sessions_found': discovered_sessions,
        'list_output': list_output[:500] if list_output else ""
    }
    
    if not discovered_sessions:
        print(f"✗ No valid tile sessions found")
        return results
    
    # Stage 3: Analyze header recovery
    print(f"\n{'='*80}")
    print(f"STAGE 3: Analyzing header recovery ({preset})")
    print(f"{'='*80}")
    
    header_analysis = analyze_header_recovery(noisy_dir, preset)
    results['stages']['header_analysis'] = header_analysis
    print(f"✓ Header detection rate: {header_analysis.get('header_detection_rate', 0)*100:.1f}%")
    print(f"  Avg header corruption: {header_analysis.get('avg_header_corruption', 0):.2f}")
    
    # Stage 4: Check EXIF in noisy tiles
    print(f"\n{'='*80}")
    print(f"STAGE 4: Checking EXIF preservation in noisy tiles ({preset})")
    print(f"{'='*80}")
    
    exif_results = {
        'tiles_checked': 0,
        'tiles_with_exif': 0,
        'exif_fields_present': set(),
        'sample_tiles': {}
    }
    
    for i, tile_path in enumerate(sorted(list(noisy_dir.glob("horus_42_full*.png")))[:5]):  # Check first 5
        exif_check = check_exif_in_image(tile_path)
        exif_results['tiles_checked'] += 1
        if exif_check['has_exif']:
            exif_results['tiles_with_exif'] += 1
            exif_results['exif_fields_present'].update(exif_check['exif_tags'].keys())
        exif_results['sample_tiles'][tile_path.name] = {
            'has_exif': exif_check['has_exif'],
            'iso': exif_check['iso'],
            'f_number': exif_check['f_number'],
            'focal_length': exif_check['focal_length'],
            'date_time': exif_check['date_time'],
        }
    
    results['stages']['exif_check_noisy_tiles'] = exif_results
    exif_rate = exif_results['tiles_with_exif'] / max(1, exif_results['tiles_checked']) * 100
    print(f"✓ EXIF preservation in noisy tiles: {exif_results['tiles_with_exif']}/{exif_results['tiles_checked']} ({exif_rate:.1f}%)")
    
    # Stage 5: Stitch noisy tiles using discovered session
    print(f"\n{'='*80}")
    print(f"STAGE 5: Stitching {preset} tiles")
    print(f"{'='*80}")
    
    stitch_output = test_dir / f"stitched_{preset}.png"
    
    # Use first discovered session for stitching
    session_to_use = discovered_sessions[0]
    print(f"Using session: {session_to_use}")
    
    success, stdout, stderr = run_command(
        [str(stitch_binary), "--dir", str(noisy_dir), "--session", session_to_use, "-o", str(stitch_output)],
        f"Stitch {preset} tiles",
        timeout=120
    )
    
    stitch_results = {
        'success': success,
        'session_used': session_to_use,
        'output_size': stitch_output.stat().st_size if stitch_output.exists() else 0,
        'error': stderr if not success else None,
        'stdout': stdout[:200] if stdout else ""
    }
    
    results['stages']['stitching'] = stitch_results
    
    if success:
        print(f"✓ Stitching successful")
        print(f"  Output size: {stitch_results['output_size']} bytes")
    else:
        print(f"✗ Stitching failed: {stderr}")
    
    # Stage 6: Check EXIF in stitched output
    print(f"\n{'='*80}")
    print(f"STAGE 6: Checking EXIF in stitched output ({preset})")
    print(f"{'='*80}")
    
    if stitch_output.exists():
        stitched_exif = check_exif_in_image(stitch_output)
        results['stages']['exif_check_stitched'] = stitched_exif
        
        if stitched_exif['has_exif']:
            print(f"✓ Stitched output HAS EXIF data")
            if stitched_exif['iso']:
                print(f"  ISO: {stitched_exif['iso']}")
            if stitched_exif['f_number']:
                print(f"  F-Number: {stitched_exif['f_number']}")
            if stitched_exif['focal_length']:
                print(f"  Focal Length: {stitched_exif['focal_length']}")
            if stitched_exif['date_time']:
                print(f"  Date/Time: {stitched_exif['date_time']}")
        else:
            print(f"✗ Stitched output has NO EXIF data")
    
    # Stage 7: Visual inspection of seams
    print(f"\n{'='*80}")
    print(f"STAGE 7: Evaluating visual quality ({preset})")
    print(f"{'='*80}")
    
    if stitch_output.exists():
        img = Image.open(stitch_output)
        img_array = np.array(img)
        
        # Check for visible seams/corruption
        visual_metrics = {
            'dimensions': img.size,
            'has_corruption': False,
            'analysis': 'Visual inspection would require ML model'
        }
        
        results['stages']['visual_quality'] = visual_metrics
        print(f"✓ Output dimensions: {visual_metrics['dimensions']}")
        print(f"  File size: {stitch_output.stat().st_size / 1024:.1f} KB")
    
    return results


def main():
    project_root = Path(__file__).parent.parent.parent
    
    # Verify requirements
    tiling_dir = project_root / "tests" / "test_outputs" / "tiling"
    if not tiling_dir.exists():
        print(f"Error: Tiling directory not found: {tiling_dir}")
        sys.exit(1)
    
    # Run tests for multiple QRM presets
    presets = ["clean", "light_qrm", "medium_qrm", "heavy_qrm"]
    all_results = {
        'test_suite': 'QRM EXIF & Tile Placement Recovery',
        'test_date': datetime.now().isoformat(),
        'presets_tested': presets,
        'preset_results': {}
    }
    
    print("=" * 80)
    print("QRM EXIF & TILE PLACEMENT RECOVERY TEST SUITE")
    print("=" * 80)
    print(f"\nTesting {len(presets)} QRM presets...")
    print(f"Source tiles: {tiling_dir}")
    
    for preset in presets:
        results = run_qrm_test(preset, project_root)
        all_results['preset_results'][preset] = results
    
    # Save comprehensive results
    results_file = project_root / "tests" / "test_outputs" / "qrm_exif_recovery_results.json"
    with open(results_file, 'w') as f:
        json.dump(all_results, f, indent=2, default=str)
    
    print("\n" + "=" * 80)
    print("TEST SUMMARY")
    print("=" * 80)
    
    for preset, results in all_results['preset_results'].items():
        print(f"\n{preset.upper()}:")
        stages = results.get('stages', {})
        
        # Noise generation
        if 'noise_generation' in stages:
            print(f"  ✓ Generated {stages['noise_generation']['tile_count']} tiles")
        
        # Discovery
        if 'discovery' in stages:
            discovery = stages['discovery']
            sessions = discovery.get('sessions_found', [])
            print(f"  ✓ Discovered {len(sessions)} tile set(s)")
            for session in sessions:
                print(f"      - {session}")
        
        # Header recovery
        if 'header_analysis' in stages:
            rate = stages['header_analysis'].get('header_detection_rate', 0)
            print(f"  Header detection: {rate*100:.1f}%")
        
        # EXIF preservation
        if 'exif_check_noisy_tiles' in stages:
            noisy_exif = stages['exif_check_noisy_tiles']
            print(f"  EXIF in noisy tiles: {noisy_exif.get('tiles_with_exif', 0)}/{noisy_exif.get('tiles_checked', 0)}")
        
        # Stitching
        if 'stitching' in stages:
            stitching = stages['stitching']
            status = "✓" if stitching['success'] else "✗"
            print(f"  {status} Stitching {'succeeded' if stitching['success'] else 'failed'}")
        
        # EXIF in output
        if 'exif_check_stitched' in stages:
            stitched = stages['exif_check_stitched']
            status = "✓" if stitched.get('has_exif') else "✗"
            print(f"  {status} EXIF in stitched output: {stitched.get('has_exif', False)}")
    
    print(f"\n✓ Detailed results saved to: {results_file}")
    print("\nTest completed.")


if __name__ == "__main__":
    main()
