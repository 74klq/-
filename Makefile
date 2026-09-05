CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall
CFLAGS = -Wall

RAYLIB_DIR = C:/Users/me/Downloads/raylib-6.0_win64_mingw-w64/raylib-6.0_win64_mingw-w64
INCLUDES = -I./inc -I./resource -I"$(RAYLIB_DIR)/include"
LDFLAGS = -L./lib/x64 -L"$(RAYLIB_DIR)/lib" -lraylib -lfmod -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++ -mwindows

TARGET = The_Line
BIN_DIR = bin

OBJS = $(BIN_DIR)/b_main.o $(BIN_DIR)/main_Menu.o $(BIN_DIR)/background_rotation.o $(BIN_DIR)/play_scene.o $(BIN_DIR)/note.o $(BIN_DIR)/editor_scene.o $(BIN_DIR)/chart_editor.o $(BIN_DIR)/chart_save.o $(BIN_DIR)/editor_play.o $(BIN_DIR)/hit_effect.o $(BIN_DIR)/audio_manager.o $(BIN_DIR)/Selection.o

all: $(BIN_DIR) $(TARGET)

debug: CXXFLAGS = -std=c++17 -Wall -D_DEBUG
debug: CFLAGS = -Wall -D_DEBUG
debug: clean $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET).exe $(LDFLAGS)

$(BIN_DIR)/b_main.o: source/main/b_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/main_Menu.o: source/Menu/main_Menu.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/background_rotation.o: source/Menu/background_rotation.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/play_scene.o: source/play/play_scene.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/note.o: source/play/note.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/editor_scene.o: source/editing/editor_scene.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/chart_editor.o: source/editing/chart_editor.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/chart_save.o: source/editing/chart_save.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/editor_play.o: source/editing/editor_play.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/hit_effect.o: source/vfx/hit_effect.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/audio_manager.o: source/AudioManager/audio_manager.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/Selection.o: source/Menu/Map_Selection/Selection.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
	@if exist $(TARGET).exe del $(TARGET).exe