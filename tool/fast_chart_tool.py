import tkinter as tk
from tkinter import filedialog, messagebox
import json
import os
import librosa
import numpy as np

def generate_chart():
    file_path = filedialog.askopenfilename(
        title="ogg인 음악 파일 고르셈 (개발자들만 쓸수있는 프로그램)",
        filetypes=[("OGG Audio Files", "*.ogg"), ("All Audio Files", "*.mp3 *.wav *.ogg")]
    )
    if not file_path:
        return
        
    try:
        y, sr = librosa.load(file_path, sr=22050)
        
        onset_frames = librosa.onset.onset_detect(y=y, sr=sr, backtrack=True)
        onset_times = librosa.frames_to_time(onset_frames, sr=sr)
        
        NOTE_SPEED = 200.0
        min_delta = 0.15
        
        notes = []
        last_time = -1.0
        lane_counter = 0
        
        for t in onset_times:
            if t - last_time >= min_delta:
                posX = t * NOTE_SPEED
                lane = lane_counter % 4
                notes.append({"lane": lane, "posX": round(posX, 2)})
                last_time = t
                lane_counter += 1
        
        filename_only = os.path.basename(file_path)
        music_path = f"music/{filename_only}"
        
        output_dir = "map_data"
        os.makedirs(output_dir, exist_ok=True)
        
        output_filename = os.path.splitext(filename_only)[0] + "_auto.json"
        output_full_path = os.path.join(output_dir, output_filename)
        
        chart_data = {
            "musicPath": music_path,
            "noteCount": len(notes),
            "notes": notes
        }
        
        with open(output_full_path, "w", encoding="utf-8") as f:
            json.dump(chart_data, f, indent=2)
            
        messagebox.showinfo("됬음", f"채보가 만들어졌다\n\n저장 위치: {output_full_path}\n총 노트 수: {len(notes)}개")
        
    except Exception as e:
        messagebox.showerror("버그", f"채보 생성 중 문제가 발생했습니다:\n{str(e)}")

root = tk.Tk()
root.title("The Line 채보 생성기")
root.geometry("360x180")
root.resizable(False, False)

label = tk.Label(root, text="OGG 파일을 선택하면\nThe Line채보 JSON이 생성됩니다.", font=("맑은 고딕", 10), pady=15)
label.pack()

btn = tk.Button(root, text="OGG 파일 불러오기, 변환", command=generate_chart, font=("맑은 고딕", 11, "bold"), bg="#2b2b2b", fg="white", width=26, height=2)
btn.pack(pady=5)

root.mainloop()