import wave
import numpy as np

with wave.open("Tests/output/test_recording.wav", "rb") as w:
    sr = w.getframerate()
    raw = w.readframes(w.getnframes())
    data = np.frombuffer(raw, dtype=np.int16).reshape(-1, 2)

left = data[:, 0].astype(np.float32) / 32768.0

# Print samples around 2.22s to 2.26s
t_start = 2.22
t_end = 2.26
i_start = int(t_start * sr)
i_end = int(t_end * sr)

sub = left[i_start:i_end]
print(f"Inspecting from t={t_start}s to {t_end}s (samples {i_start} to {i_end}):")

# Let's see the envelope / peak every 1ms
ms_chunk = int(sr * 0.001)
for idx in range(0, len(sub), ms_chunk):
    t_curr = t_start + idx / sr
    chunk = sub[idx:idx+ms_chunk]
    pk = np.max(np.abs(chunk))
    rms = np.sqrt(np.mean(chunk**2))
    # print when something interesting happens
    if pk > 0.01 or (idx > 0 and np.max(np.abs(sub[idx-ms_chunk:idx])) > 0.01):
        print(f"t={t_curr*1000:.1f}ms: Peak={pk:.5f}, RMS={rms:.5f}")
