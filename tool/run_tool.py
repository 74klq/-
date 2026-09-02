import configparser
import os

CONFIG_PATH = os.path.join(os.path.dirname(__file__), 'config.ini')

def main():
    config = configparser.ConfigParser()
    if not os.path.exists(CONFIG_PATH):
        return

    config.read(CONFIG_PATH, encoding='utf-8')
    
    music_dir = config.get('PATHS', 'MUSIC_DIR', fallback='../music')
    bitrate = config.get('AUDIO_SETTINGS', 'BITRATE', fallback='192k')
    remove_original = config.getboolean('AUDIO_SETTINGS', 'REMOVE_ORIGINAL', fallback=False)

    base_dir = os.path.dirname(__file__)
    target_dir = os.path.normpath(os.path.join(base_dir, music_dir))

    from converter import convert_audio
    convert_audio(target_dir, bitrate, remove_original)

if __name__ == '__main__':
    main()