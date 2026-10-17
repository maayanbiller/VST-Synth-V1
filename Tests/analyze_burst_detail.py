import wave
import numpy as np

with wave.open("Tests/output/test_recording.wav", "rb") as w:
    sr = w.getframerate()
    n_frames = w.getnframes()
    raw = w.readframes(n_frames)
    data = np.frombuffer(raw, dtype=np.int16).reshape(-1, 2)

left = data[:, 0].astype(np.float32) / 32768.0

# Look at 5 bursts between t=2.3s and 2.5s
# Print the exact samples of one burst
start_idx = int(sr * 2.28)
end_idx = int(sr * 2.33)
chunk = left[start_idx:end_idx]

print(f"Sampling chunk length: {len(chunk)} samples ({len(chunk)/sr*1000:.1f} ms)")

# Check if there are exact zero samples
zero_count = np.sum(chunk == 0.0)
near_zero_count = np.sum(np.abs(chunk) < 1e-4)
print(f"Total samples: {len(chunk)}, Exact zeros: {zero_count}, Near zeros: {near_zero_count}")

# Print continuous runs of zeros or near-zeros
is_silent = np.abs(chunk) < 0.005
silent_runs = []
cur_run = 0
for s in is_silent:
    if s:
        cur_run += 1
    else:
        if cur_run > 0:
            silent_runs.append(cur_run)
            cur_run = 0
if cur_run > 0:
    silent_runs.append(cur_run)

print("Long silent run lengths (in samples):", [r for r in silent_runs if r > 100])
print("Corresponding durations (in ms):", [f"{r*1000/sr:.2f} ms" for r in silent_runs if r > 100])

# Inspect what happens right at the start of a burst: is it a smooth attack or a sudden jump?
# Find transition from silent to active
for i in range(1, len(chunk)):
    if is_silent[i-1] and not is_silent[i]:
        # Print 10 samples around this transition
        t_trans = (start_idx + i) / sr
        print(f"\nTransition from silence to burst at t={t_trans:.4f}s (sample {i}):")
        for j in range(max(0, i-3), min(len(chunk), i+8)):
            print(f"  sample[{j}]: {chunk[j]:.6f}")
        break
