import wave
import numpy as np

with wave.open("Tests/output/test_recording.wav", "rb") as w:
    sr = w.getframerate()
    n_frames = w.getnframes()
    raw = w.readframes(n_frames)
    data = np.frombuffer(raw, dtype=np.int16).reshape(-1, 2)

left = data[:, 0].astype(np.float32) / 32768.0

# Detect frequencies of notes before 2.25s and after 2.25s
def get_pitch(segment, sr):
    # autocorrelation
    segment = segment - np.mean(segment)
    corr = np.correlate(segment, segment, mode='full')
    corr = corr[len(corr)//2:]
    # find first peak after zero crossing
    d = np.diff(corr)
    start = np.where(d > 0)[0]
    if len(start) == 0:
        return 0
    peak = start[0] + np.argmax(corr[start[0]:min(len(corr), start[0] + int(sr/50))])
    if peak > 0:
        return sr / peak
    return 0

print("=== Analyzing Pitches in test_recording.wav ===")
# Before t=2.25s
for t in [1.2, 1.4, 1.6, 1.8, 2.0]:
    idx = int(t * sr)
    seg = left[idx:idx + 2048]
    pitch = get_pitch(seg, sr)
    print(f"t={t:.2f}s: pitch={pitch:.1f} Hz (MIDI Note ~ {69 + 12*np.log2(pitch/440.0):.1f})")

# After t=2.25s: check active bursts
print("\n=== Analyzing Pitches in bursts after 2.25s ===")
for t in [2.282, 2.315, 2.348, 2.381, 2.50, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00]:
    idx = int(t * sr)
    seg = left[idx:idx + 512]
    # find where seg is loud
    loud = np.where(np.abs(seg) > 0.02)[0]
    if len(loud) > 50:
        sub = seg[loud[0]:loud[0]+256]
        pitch = get_pitch(sub, sr)
        midi = 69 + 12*np.log2(pitch/440.0) if pitch > 0 else 0
        print(f"t={t:.3f}s: pitch={pitch:.1f} Hz (MIDI Note ~ {midi:.1f})")
    else:
        print(f"t={t:.3f}s: in silent gap")
