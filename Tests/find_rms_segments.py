import wave
import numpy as np

with wave.open("Tests/output/test_recording.wav", "rb") as w:
    sr = w.getframerate()
    raw = w.readframes(w.getnframes())
    data = np.frombuffer(raw, dtype=np.int16).reshape(-1, 2)

left = data[:, 0].astype(np.float32) / 32768.0

# Compute RMS in 1ms windows (44 samples)
window = int(sr * 0.001)
rms_vals = []
for i in range(0, len(left) - window, window):
    rms = np.sqrt(np.mean(left[i:i+window]**2))
    rms_vals.append(rms)

rms_vals = np.array(rms_vals)
t_ms = np.arange(len(rms_vals))

# Find active segments (RMS > 0.01) vs quiet segments (RMS <= 0.01)
is_active = rms_vals > 0.01

segments = []
cur_state = is_active[0]
cur_start = 0
for i in range(1, len(is_active)):
    if is_active[i] != cur_state:
        segments.append((cur_state, cur_start, i))
        cur_state = is_active[i]
        cur_start = i
segments.append((cur_state, cur_start, len(is_active)))

print("=== Segments between 2.2s and 3.5s ===")
for state, start_ms, end_ms in segments:
    if end_ms >= 2200 and start_ms <= 3500:
        dur = end_ms - start_ms
        name = "AUDIO " if state else "SILENCE"
        print(f"{name}: {start_ms}ms to {end_ms}ms (dur = {dur} ms)")
