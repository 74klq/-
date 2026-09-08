#include "editor_scene.h"
#include "chart_editor.h"

static ChartEditor chartEditor;

EditorScene::EditorScene() {
}

EditorScene::~EditorScene() {
    Release();
}

void EditorScene::Init() {
    chartEditor.Init();
}

void EditorScene::HandleInput() {
    chartEditor.HandleInput();
}

void EditorScene::Render() {
    chartEditor.Render();
}

void EditorScene::Release() {
    chartEditor.Release();
}