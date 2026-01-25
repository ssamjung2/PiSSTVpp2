#!/usr/bin/env python3
import wave
import struct

with wave.open('test_cw_fixed.wav', 'rb') as f:
    framerate = f.getframerate()
    frames = f.readframes(f.getnframes())
    
    # Unpack as uint16 (little-endian)
    audio = struct.unpack('<' + 'H' * (len(frames) // 2), frames)

print(f"Sample rate: {framerate} Hz")
print(f"Total samples: {len(audio)}")
print(f"\nAudio statistics:")
print(f"Min value: {min(audio)}")
print(f"Max value: {max(audio)}")
print(f"Mean (first 1000): {sum(audio[:1000]) / 1000:.1f} (should be 32768)")

# Check that audio is centered at 32768
avg = sum(audio) / len(audio)
print(f"Overall mean: {avg:.1f} (should be close to 32768)")
