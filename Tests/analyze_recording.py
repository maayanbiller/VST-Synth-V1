import wave
import numpy as np

with wave.open("Tests/output/test_recording.wav", "rb") as w:
    n_ch = w.getnchannels()
    sr = w.getframerate()
    n_frames = w.getnframes()
    raw = w.readframes(n_frames)
    data = np.frombuffer(raw, dtype=np.int16).reshape(-1, n_ch)

print(f"Channels: {n_ch}, SampleRate: {sr}, Total frames: {n_frames} ({n_frames/sr:.2f} s)")

left = data[:, 0].astype(np.float32) / 32768.0

# Print RMS in 100ms chunks around t=2.0s to 4.0s
chunk = int(sr * 0.05)
print("=== RMS per 50ms chunk from 2.0s to 3.5s ===")
for i in range(int(sr * 2.0), int(sr * 3.5), chunk):
    t = i / sr
    rms = np.sqrt(np.mean(left[i:i+chunk]**2))
    peak = np.max(np.abs(left[i:i+chunk]))
    print(f"t={t:.3f}s: RMS={rms:.5f}, Peak={peak:.5f}")

# Inspect the exact wave around 2.25s
idx_start = int(sr * 2.24)
idx_end = int(sr * 2.45)
sub = left[idx_start:idx_end]

# Find non-zero regions
threshold = 0.01
is_active = np.abs(sub) > threshold

bursts = []
in_burst = False
start_b = 0
for i, active in enumerate(is_active):
    if active and not in_burst:
        in_burst = True
        start_b = i
    elif not active and in_burst:
        in_burst = False
        bursts.append((start_b, i))

print(f"\nDetected {len(bursts)} bursts between 2.24s and 2.45s:")
for b_start, b_end in bursts[:10]:
    t0 = (idx_start + b_start) / sr
    t1 = (idx_start + b_end) / sr
    dur_ms = (b_end - b_start) * 1000.0 / sr
    print(f"Burst: {t0:.4f}s to {t1:.4f}s (duration {dur_ms:.2f} ms)")
    if len(bursts) > 1 and bursts.index((b_start, b_end)) > 0:
        prev_end = bursts[bursts.index((b_start, b_end)) - 1][1]
        gap_ms = (b_start - prev_end) * 1000.0 / sr
        print(f"   Gap from previous burst: {gap_ms:.2f} ms")
