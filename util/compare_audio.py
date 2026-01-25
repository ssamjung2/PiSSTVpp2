#!/usr/bin/env python3
import wave
import struct
import math

def analyze_file(filename, label):
    print(f"\n{'='*60}")
    print(f"Analysis: {label}")
    print(f"{'='*60}")
    
    with wave.open(filename, 'rb') as f:
        framerate = f.getframerate()
        frames = f.readframes(f.getnframes())
        audio = struct.unpack('<' + 'H' * (len(frames) // 2), frames)
    
    # Convert to signed for analysis
    audio_signed = [s - 32768 for s in audio]
    
    print(f"File: {filename}")
    print(f"Sample rate: {framerate} Hz")
    print(f"Total samples: {len(audio)}")
    print(f"Duration: {len(audio) / framerate:.2f} seconds")
    print(f"\nAmplitude (unsigned 0-65535):")
    print(f"  Min: {min(audio)}")
    print(f"  Max: {max(audio)}")
    print(f"  Mean: {sum(audio) / len(audio):.1f}")
    print(f"\nAmplitude (signed -32768 to 32767):")
    print(f"  Min: {min(audio_signed)}")
    print(f"  Max: {max(audio_signed)}")
    print(f"  Mean: {sum(audio_signed) / len(audio_signed):.1f}")
    
    # Calculate RMS
    rms = math.sqrt(sum(s**2 for s in audio_signed) / len(audio_signed))
    print(f"  RMS: {rms:.1f}")
    
    # Check first 100 samples (should be silence at 32768)
    first_100 = audio[:100]
    print(f"\nFirst 100 samples (silence):")
    print(f"  Mean: {sum(first_100) / len(first_100):.1f} (should be 32768)")
    
    # Check a section with CW tone (around 0.5 seconds)
    tone_idx_start = int(0.5 * framerate)
    tone_idx_end = int(1.0 * framerate)
    tone_section = audio_signed[tone_idx_start:tone_idx_end]
    tone_rms = math.sqrt(sum(s**2 for s in tone_section) / len(tone_section))
    print(f"\nCW tone section (0.5-1.0 sec):")
    print(f"  RMS: {tone_rms:.1f} (should be ~{rms:.0f})")
    
    # Check transition at tone start
    print(f"\nEnvelope smoothness (CW start around 0.5s):")
    for offset_ms in [0, 5, 10, 20]:
        idx = tone_idx_start + int(offset_ms * framerate / 1000)
        if idx < len(audio):
            value = audio_signed[idx]
            print(f"  At +{offset_ms}ms: {value}")

# Analyze both files
if True:
    try:
        analyze_file('test_cw_fixed.wav', 'FIXED VERSION (with DC bias correction)')
    except Exception as e:
        print(f"Error reading test_cw_fixed.wav: {e}")
    
    try:
        analyze_file('test_cw_hann.wav', 'OLD VERSION (before DC bias fix)')
    except Exception as e:
        print(f"Error reading test_cw_hann.wav: {e}")
