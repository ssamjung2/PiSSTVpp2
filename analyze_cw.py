#!/usr/bin/env python3
"""
Analyze CW audio for clipping behavior and signal quality
"""
import wave
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal

def analyze_wav(filename):
    """Analyze WAV file for clipping and signal quality"""
    
    # Load WAV file
    with wave.open(filename, 'rb') as wav_file:
        params = wav_file.getparams()
        nchannels, sampwidth, framerate, nframes = params[:4]
        audio_data = wav_file.readframes(nframes)
    
    # Convert to numpy array (16-bit unsigned)
    audio_array = np.frombuffer(audio_data, dtype=np.uint16)
    
    print(f"File: {filename}")
    print(f"Sample rate: {framerate} Hz")
    print(f"Total samples: {nframes}")
    print(f"Duration: {nframes/framerate:.2f} seconds")
    print(f"Channels: {nchannels}")
    print()
    
    # Convert to signed for easier analysis
    audio_signed = audio_array.astype(np.int32) - 32768
    
    print(f"Min value: {audio_signed.min()} ({audio_array.min()})")
    print(f"Max value: {audio_signed.max()} ({audio_array.max()})")
    print(f"RMS level: {np.sqrt(np.mean(audio_signed**2)):.1f}")
    print()
    
    # Detect clipping
    clip_threshold = 32000  # Values above this in unsigned (0-65535) are suspicious
    clipped_high = np.sum(audio_array >= clip_threshold)
    clipped_low = np.sum(audio_array <= 768)  # Below ~0.01 in signed
    
    print(f"Samples >= {clip_threshold} (potential clipping): {clipped_high} ({100*clipped_high/len(audio_array):.3f}%)")
    print(f"Samples <= 768 (near zero): {clipped_low} ({100*clipped_low/len(audio_array):.3f}%)")
    print()
    
    # Analyze CW section (roughly last 30 seconds, where K0TEST is sent)
    cw_start = int(framerate * (nframes/framerate - 30))  # Last 30 seconds
    cw_audio = audio_signed[cw_start:]
    
    print(f"=== CW SECTION (last 30 seconds) ===")
    print(f"CW section start sample: {cw_start}")
    print(f"CW samples: {len(cw_audio)}")
    print(f"CW min: {cw_audio.min()}")
    print(f"CW max: {cw_audio.max()}")
    print(f"CW RMS: {np.sqrt(np.mean(cw_audio**2)):.1f}")
    print()
    
    # Detect tone regions (non-silence)
    silence_threshold = 500  # Below this is silence
    tone_mask = np.abs(cw_audio) > silence_threshold
    tone_regions = []
    in_tone = False
    start = 0
    
    for i, is_tone in enumerate(tone_mask):
        if is_tone and not in_tone:
            start = i
            in_tone = True
        elif not is_tone and in_tone:
            tone_regions.append((start, i))
            in_tone = False
    
    if in_tone:
        tone_regions.append((start, len(tone_mask)))
    
    print(f"Detected {len(tone_regions)} tone regions (dits/dashes)")
    print()
    
    # Analyze each tone
    tone_peaks = []
    for idx, (start, end) in enumerate(tone_regions[:20]):  # First 20 tones
        tone = cw_audio[start:end]
        peak = np.max(np.abs(tone))
        rms = np.sqrt(np.mean(tone**2))
        tone_peaks.append(peak)
        crest_factor = peak / rms if rms > 0 else 0
        
        duration_ms = (end - start) * 1000.0 / framerate
        print(f"Tone {idx+1}: {duration_ms:6.1f}ms | Peak: {peak:5d} | RMS: {rms:6.1f} | Crest Factor: {crest_factor:.2f}")
    
    print()
    print(f"Tone peak average: {np.mean(tone_peaks):.1f}")
    print(f"Tone peak std dev: {np.std(tone_peaks):.1f}")
    print()
    
    # Plot waveform
    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    
    # Full audio
    time_full = np.arange(len(audio_signed)) / framerate
    axes[0].plot(time_full, audio_signed, linewidth=0.5)
    axes[0].set_title('Full SSTV + CW Audio')
    axes[0].set_ylabel('Amplitude (signed)')
    axes[0].grid(True)
    axes[0].set_xlim([0, len(audio_signed)/framerate])
    
    # CW section
    time_cw = np.arange(len(cw_audio)) / framerate + cw_start/framerate
    axes[1].plot(time_cw, cw_audio, linewidth=0.5)
    axes[1].set_title('CW Section (Last 30 seconds)')
    axes[1].set_ylabel('Amplitude (signed)')
    axes[1].grid(True)
    axes[1].axhline(y=32000, color='r', linestyle='--', alpha=0.5, label='Clip threshold')
    axes[1].axhline(y=-32000, color='r', linestyle='--', alpha=0.5)
    axes[1].legend()
    
    # Zoomed CW (first 5 dits/dashes)
    if len(tone_regions) > 0:
        zoom_end = min(tone_regions[4][1] if len(tone_regions) > 4 else tone_regions[-1][1], 
                       cw_start + int(2 * framerate))  # 2 seconds max
        zoom_audio = audio_signed[cw_start:zoom_end]
        time_zoom = np.arange(len(zoom_audio)) / framerate
        axes[2].plot(time_zoom, zoom_audio, linewidth=1)
        axes[2].set_title('CW Zoomed (First ~2 seconds)')
        axes[2].set_xlabel('Time (seconds, relative to CW start)')
        axes[2].set_ylabel('Amplitude (signed)')
        axes[2].grid(True)
        axes[2].axhline(y=32000, color='r', linestyle='--', alpha=0.5, label='Clip threshold')
        axes[2].axhline(y=-32000, color='r', linestyle='--', alpha=0.5)
        axes[2].legend()
    
    plt.tight_layout()
    plt.savefig('/Users/ssamjung/Desktop/WIP/PiSSTVpp2/cw_analysis.png', dpi=100)
    print("Saved: cw_analysis.png")
    plt.show()

if __name__ == '__main__':
    import sys
    filename = sys.argv[1] if len(sys.argv) > 1 else './tests/test_bars_cw_smooth.m1.wav'
    analyze_wav(filename)
