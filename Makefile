CXX = g++
CC = gcc
RC = windres
CXXFLAGS = -std=c++17 -Wall -D_DEBUG
CFLAGS = -Wall -D_DEBUG

RAYLIB_DIR = C:/Users/me/Downloads/raylib-6.0_win64_mingw-w64/raylib-6.0_win64_mingw-w64
INCLUDES = -I./inc -I./resource -I"$(RAYLIB_DIR)/include"

LDFLAGS = -L./lib/x64 -L"$(RAYLIB_DIR)/lib" -lraylib -lfmod -lavformat -lavcodec -lswscale -lavutil -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++ -mwindows

TARGET = RA
BIN_DIR = bin

OBJS = $(BIN_DIR)/b_main.o \
       $(BIN_DIR)/main_Menu.o \
       $(BIN_DIR)/background_rotation.o \
       $(BIN_DIR)/Menu_background.o \
       $(BIN_DIR)/cursor_move.o \
       $(BIN_DIR)/slow_screen.o \
       $(BIN_DIR)/curtain_vfx.o \
       $(BIN_DIR)/play_scene.o \
       $(BIN_DIR)/note.o \
       $(BIN_DIR)/editor_scene.o \
       $(BIN_DIR)/chart_editor.o \
       $(BIN_DIR)/auto_chart_generator.o \
       $(BIN_DIR)/chart_save.o \
       $(BIN_DIR)/editor_play.o \
       $(BIN_DIR)/hit_effect.o \
       $(BIN_DIR)/shack.o \
       $(BIN_DIR)/note_down_animation.o \
       $(BIN_DIR)/audio_manager.o \
       $(BIN_DIR)/lobby_bgm.o \
       $(BIN_DIR)/Selection.o \
       $(BIN_DIR)/Load.o \
       $(BIN_DIR)/play_main.o \
       $(BIN_DIR)/Gh_click.o \
       $(BIN_DIR)/Fn_click.o \
       $(BIN_DIR)/Sn_click.o \
       $(BIN_DIR)/Clear.o \
       $(BIN_DIR)/HoldNote.o \
       $(BIN_DIR)/resource.o

all: $(BIN_DIR) $(TARGET)

debug: CXXFLAGS = -std=c++17 -Wall -D_DEBUG
debug: CFLAGS = -Wall -D_DEBUG
debug: clean $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET).exe $(LDFLAGS)

$(BIN_DIR)/resource.o: resource.rc
	$(RC) $< -o $@

$(BIN_DIR)/b_main.o: source/main/b_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/main_Menu.o: source/Menu/main_Menu.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/background_rotation.o: source/Menu/background_rotation.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Menu_background.o: source/Menu/Menu_background.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/cursor_move.o: source/Menu/cursor_move.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/slow_screen.o: source/Menu/slow_screen.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/curtain_vfx.o: source/Menu/curtain_vfx.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/play_scene.o: source/play/play_scene.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/note.o: source/play/note.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/editor_scene.o: source/editing/editor_scene.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/chart_editor.o: source/editing/chart_editor.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/auto_chart_generator.o: source/editing/auto_chart_generator.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/chart_save.o: source/editing/chart_save.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/editor_play.o: source/editing/editor_play.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/hit_effect.o: source/vfx/hit_effect.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/shack.o: source/vfx/shack.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/note_down_animation.o: source/Animation/note_down_animation.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/audio_manager.o: source/AudioManager/audio_manager.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/lobby_bgm.o: source/music_execute/Lobby_Music/lobby_bgm.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Selection.o: source/Menu/Map_Selection/Selection.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Load.o: source/UI/Loading/Load.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/play_main.o: source/UI/G_I/play_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Gh_click.o: source/Note_Exception_Hadling/NEH_Ghost_Note/Gh_click.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Fn_click.o: source/Note_Exception_Hadling/NEH_Fast_Note/Fn_click.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Sn_click.o: source/Note_Exception_Hadling/NEH_Slow_Note/Sn_click.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Clear.o: source/Ending/Clear.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/HoldNote.o: source/Hold_Note/HoldNote.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
	@if exist $(TARGET).exe del $(TARGET).exe