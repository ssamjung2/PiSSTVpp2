#!/usr/bin/env python3
"""
Advanced CW audio analysis - frequency domain, harmonics, distortion
"""
import wave
import struct
import numpy as np
from scipy import signal
import matplotlib.pyplot as plt

def analyze_wav_advanced(filename):
    """Advanced analysis with frequency domain and harmonics"""
    
    # Load WAV file
    with wave.open(filename, 'rb') as wav_file:
        params = wav_file.getparams()
        nchannels, sampwidth, framerate, nframes = params[:4]
        audio_data = wav_file.readframes(nframes)
    
    # Convert to numpy array
    audio_array = np.frombuffer(audio_data, dtype=np.uint16)
    audio_signed = audio_array.astype(np.float32) - 32768
    
    print(f"File: {filename}")
    print(f"Sample rate: {framerate} Hz")
    print(f"Duration: {len(audio_array)/framerate:.2f} seconds")
    print(f"Peak amplitude: {np.max(np.abs(audio_signed)):.1f}")
    print(f"RMS level: {np.sqrt(np.mean(audio_signed**2)):.1f}")
    print()
    
    # Extract CW section (last 30 seconds)
    cw_start = max(0, len(audio_array) - int(framerate * 30))
    cw_audio = audio_signed[cw_start:]
    
    print(f"=== CW SECTION (last 30 seconds) ===")
    print(f"CW RMS: {np.sqrt(np.mean(cw_audio**2)):.1f}")
    print()
    
    # Detect individual tone regions
    silence_threshold = 500
    tone_regions = []
    in_tone = False
    start = 0
    
    for i, sample in enumerate(cw_audio):
        is_tone = np.abs(sample) > silence_threshold
        if is_tone and not in_tone:
            start = i
            in_tone = True
        elif not is_tone and in_tone:
            tone_regions.append((start, i))
            in_tone = False
    
    if in_tone:
        tone_regions.append((start, len(cw_audio)))
    
    print(f"Detected {len(tone_regions)} tone regions")
    print()
    
    # Analyze first 5 tones in detail
    print("=== DETAILED TONE ANALYSIS (First 5 tones) ===")
    print()
    
    fig, axes = plt.subplots(5, 2, figsize=(14, 12))
    
    for tone_idx in range(min(5, len(tone_regions))):
        start, end = tone_regions[tone_idx]
        tone = cw_audio[start:end]
        
        if len(tone) < 100:
            continue
        
        duration_ms = len(tone) * 1000.0 / framerate
        peak = np.max(np.abs(tone))
        rms = np.sqrt(np.mean(tone**2))
        
        print(f"Tone {tone_idx+1}: {duration_ms:.1f}ms | Peak: {peak:.0f} | RMS: {rms:.1f}")
        
        # Apply Hann window for FFT
        window = signal.windows.hann(len(tone))
        windowed = tone * window
        
        # Compute FFT
        fft_vals = np.fft.fft(windowed)
        freqs = np.fft.fftfreq(len(tone), 1.0/framerate)
        magnitude = np.abs(fft_vals) / len(tone)
        
        # Find peaks in frequency domain
        positive_freqs = freqs[:len(freqs)//2]
        positive_mag = magnitude[:len(magnitude)//2]
        
        # Detect peaks (local maxima)
        peaks, properties = signal.find_peaks(positive_mag, height=np.max(positive_mag)*0.05)
        peak_freqs = positive_freqs[peaks]
        peak_mags = positive_mag[peaks]
        
        # Sort by magnitude
        sorted_idx = np.argsort(peak_mags)[::-1]
        top_peaks = sorted_idx[:3]  # Top 3 frequencies
        
        print(f"  Top frequencies:")
        for i, idx in enumerate(top_peaks):
            print(f"    {i+1}. {peak_freqs[idx]:.1f} Hz (magnitude: {peak_mags[idx]:.1f})")
        
        # Waveform plot
        time = np.arange(len(tone)) / framerate
        axes[tone_idx, 0].plot(time[:min(1000, len(tone))], tone[:min(1000, len(tone))], linewidth=0.5)
        axes[tone_idx, 0].set_title(f'Tone {tone_idx+1} Waveform ({duration_ms:.1f}ms)')
        axes[tone_idx, 0].set_ylabel('Amplitude')
        axes[tone_idx, 0].grid(True)
        
        # Frequency spectrum
        axes[tone_idx, 1].semilogy(positive_freqs[:4000], positive_mag[:4000])
        axes[tone_idx, 1].set_title(f'Tone {tone_idx+1} Spectrum')
        axes[tone_idx, 1].set_xlabel('Frequency (Hz)')
        axes[tone_idx, 1].set_ylabel('Magnitude')
        axes[tone_idx, 1].set_xlim([0, 2000])
        axes[tone_idx, 1].grid(True, which='both', alpha=0.3)
        
        # Mark detected peaks
        for idx in top_peaks:
            axes[tone_idx, 1].plot(peak_freqs[idx], peak_mags[idx], 'r*', markersize=12)
        
        print()
    
    plt.tight_layout()
    plt.savefig('/Users/ssamjung/Desktop/WIP/PiSSTVpp2/cw_advanced_analysis.png', dpi=100)
    print("Saved: cw_advanced_analysis.png")
    
    # Analyze overall audio quality
    print()
    print("=== OVERALL AUDIO QUALITY ===")
    
    # Calculate PAPR (Peak-to-Average Power Ratio)
    peak_power = np.max(audio_signed**2)
    avg_power = np.mean(audio_signed**2)
    papr = peak_power / avg_power if avg_power > 0 else 0
    print(f"PAPR (Peak-to-Average Power Ratio): {papr:.2f}")
    
    # Check for clipping (values near 32768 or 0)
    near_max = np.sum(np.abs(audio_signed) > 31000)
    near_min = np.sum(np.abs(audio_signed) < 100)
    print(f"Samples near max amplitude: {near_max} ({100*near_max/len(audio_signed):.2f}%)")
    print(f"Samples near zero: {near_min} ({100*near_min/len(audio_signed):.2f}%)")
    print()
    
    # Crest factor (peak / RMS)
    crest_factor = np.max(np.abs(audio_signed)) / np.sqrt(np.mean(audio_signed**2))
    print(f"Crest Factor (Peak/RMS): {crest_factor:.2f}")
    if crest_factor > 3:
        print("  Warning: High crest factor suggests sharp transients or distortion")
    elif crest_factor < 1.5:
        print("  Warning: Very low crest factor - signal may be over-compressed")
    else:
        print("  Good: Natural sine wave characteristic (1.4-1.5)")

if __name__ == '__main__':
    import sys
    filename = sys.argv[1] if len(sys.argv) > 1 else './tests/test_bars_cw_louder.s1.wav'
    analyze_wav_advanced(filename)
