import os
import subprocess

def log_error(file_path, err_msg):
    log_dir = os.path.join(os.path.dirname(__file__), 'logs')
    os.makedirs(log_dir, exist_ok=True)
    log_file = os.path.join(log_dir, 'error.log')
    
    with open(log_file, 'a', encoding='utf-8') as f:
        f.write(f"Failed File: {file_path} | Error: {err_msg}\n")

def convert_audio(music_dir, bitrate, remove_original):
    if not os.path.isdir(music_dir):
        return

    valid_exts = ('.mp3', '.wav', '.flac', '.m4a', '.aac', '.ogg')

    for root, dirs, files in os.walk(music_dir):
        for file in files:
            if not file.lower().endswith(valid_exts):
                continue
            
            if file.lower().endswith('.ogg'):
                continue

            src_path = os.path.join(root, file)
            base_name = os.path.splitext(file)[0]
            dest_path = os.path.join(root, base_name + '.ogg')

            cmd = [
                'ffmpeg', '-y', '-i', src_path,
                '-b:a', bitrate, dest_path
            ]

            try:
                result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                if result.returncode != 0:
                    log_error(src_path, result.stderr.strip())
                    continue

                if remove_original and os.path.exists(dest_path):
                    os.remove(src_path)

            except Exception as e:
                log_error(src_path, str(e))