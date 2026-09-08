import tkinter as tk
from tkinter import filedialog, messagebox
import json
import os
import librosa
import numpy as np

def generate_chart():
    file_path = filedialog.askopenfilename(
        title="뉴에이지 피아노 OGG 파일 선택",
        filetypes=[("OGG Audio Files", "*.ogg"), ("All Audio Files", "*.mp3 *.wav *.ogg")]
    )
    if not file_path:
        return
        
    try:
        y, sr = librosa.load(file_path, sr=22050)
        y_harmonic, _ = librosa.effects.hpss(y)
        
        tempo, beat_frames = librosa.beat.beat_track(y=y_harmonic, sr=sr)
        bpm = float(tempo[0]) if isinstance(tempo, np.ndarray) else float(tempo)
        beat_times = librosa.frames_to_time(beat_frames, sr=sr)
        
        offset = beat_times[0] if len(beat_times) > 0 else 0.0
        grid_size = (60.0 / bpm) / 2.0
        
        onset_frames = librosa.onset.onset_detect(y=y_harmonic, sr=sr, backtrack=True, delta=0.04, wait=2)
        onset_times = librosa.frames_to_time(onset_frames, sr=sr)
        
        NOTE_SPEED = 200.0
        stft = np.abs(librosa.stft(y_harmonic))
        freqs = librosa.fft_frequencies(sr=sr)
        
        grouped_notes = {}
        
        for frame, t in zip(onset_frames, onset_times):
            if t < offset:
                continue
            
            steps = round((t - offset) / grid_size)
            quantized_t = offset + steps * grid_size
            if quantized_t < 0:
                quantized_t = 0.0
                
            if frame < stft.shape[1]:
                spectrum = stft[:, frame]
                dominant_freq = freqs[np.argmax(spectrum)]
                
                if dominant_freq < 350:
                    lane = 0
                elif dominant_freq < 800:
                    lane = 1
                elif dominant_freq < 1500:
                    lane = 2
                else:
                    lane = 3
            else:
                lane = 0
                
            time_key = round(quantized_t, 3)
            if time_key not in grouped_notes:
                grouped_notes[time_key] = set()
            grouped_notes[time_key].add(lane)
        
        notes = []
        for t_key in sorted(grouped_notes.keys()):
            lanes = list(grouped_notes[t_key])[:2]
            posX = round(t_key * NOTE_SPEED, 2)
            for lane in lanes:
                notes.append({"lane": lane, "posX": posX})
        
        filename_only = os.path.basename(file_path)
        music_path = f"music/{filename_only}"
        
        output_dir = "map_data"
        os.makedirs(output_dir, exist_ok=True)
        
        output_filename = os.path.splitext(filename_only)[0] + "_strict_quantized.json"
        output_full_path = os.path.join(output_dir, output_filename)
        
        chart_data = {
            "musicPath": music_path,
            "noteCount": len(notes),
            "notes": notes
        }
        
        with open(output_full_path, "w", encoding="utf-8") as f:
            json.dump(chart_data, f, indent=2)
            
        messagebox.showinfo("완료", f"채보 생성 완료\n\nBPM: {round(bpm, 1)} | 오프셋: {round(offset, 3)}초\n저장 위치: {output_full_path}\n총 노트 수: {len(notes)}개")
        
    except Exception as e:
        messagebox.showerror("오류", f"채보 생성 중 문제가 발생했습니다:\n{str(e)}")

root = tk.Tk()
root.title("뉴에이지 피아노 채보 생성기")
root.geometry("360x180")
root.resizable(False, False)

label = tk.Label(root, text="BPM 및 오프셋 기반 채보 생성 툴", font=("맑은 고딕", 10), pady=15)
label.pack()

btn = tk.Button(root, text="자동화 채보", command=generate_chart, font=("맑은 고딕", 11, "bold"), bg="#2b2b2b", fg="white", width=26, height=2)
btn.pack(pady=5)

root.mainloop()
