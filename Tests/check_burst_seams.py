import wave
import numpy as np

with wave.open("Tests/output/test_recording.wav", "rb") as w:
    sr = w.getframerate()
    raw = w.readframes(w.getnframes())
    data = np.frombuffer(raw, dtype=np.int16).reshape(-1, 2)

left = data[:, 0].astype(np.float32) / 32768.0

# Find bursts around t=2.35s
start_idx = int(2.30 * sr)
end_idx = int(2.45 * sr)
chunk = left[start_idx:end_idx]

# Find transitions from quiet to loud and loud to quiet
threshold = 0.015
is_loud = np.abs(chunk) > threshold

transitions_up = []
transitions_down = []
for i in range(1, len(is_loud)):
    if not is_loud[i-1] and is_loud[i]:
        transitions_up.append(i)
    elif is_loud[i-1] and not is_loud[i]:
        transitions_down.append(i)

print(f"Found {len(transitions_up)} bursts.")
for idx_up, idx_down in zip(transitions_up[:5], transitions_down[:5]):
    t_up = (start_idx + idx_up) / sr
    t_down = (start_idx + idx_down) / sr
    dur_ms = (idx_down - idx_up) * 1000.0 / sr
    print(f"\n--- Burst at t={t_up:.4f}s (dur = {dur_ms:.2f} ms, {idx_down - idx_up} samples) ---")
    
    # Check start of burst: first 5 samples
    print("  First 5 samples of burst:")
    for s in range(idx_up, min(idx_up + 5, len(chunk))):
        print(f"    sample[{(start_idx+s)/sr:.5f}s] = {chunk[s]:.5f}")
        
    # Check end of burst: last 5 samples
    print("  Last 5 samples of burst:")
    for s in range(max(idx_up, idx_down - 5), idx_down):
        print(f"    sample[{(start_idx+s)/sr:.5f}s] = {chunk[s]:.5f}")
        
    # Check first 5 samples of silence after burst
    print("  First 5 samples of silence after burst:")
    for s in range(idx_down, min(idx_down + 5, len(chunk))):
        print(f"    sample[{(start_idx+s)/sr:.5f}s] = {chunk[s]:.5f}")
