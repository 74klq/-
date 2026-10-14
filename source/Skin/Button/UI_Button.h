#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "raylib.h"
#include <string>
#include <vector>

namespace Skin {
namespace Button {

struct SkinOption {
    std::string name;
    bool isChecked;
};

class UI_Button {
private:
    Rectangle rect;
    std::string label;
    std::string indexText;
    int index;
    float hoverProgress;
    float selectProgress;
    
    bool isExpanded;
    std::vector<SkinOption> options;
    int selectedOptionIndex;

    static bool s_DiagramSkinGlobal;

public:
    UI_Button(float x, float y, float width, float height, const std::string& btnLabel, const std::string& btnIndex, int btnIndexNum);
    ~UI_Button() = default;

    void Update(float dt, bool isSelected, bool isHovered, Vector2 mousePos, bool mousePressed);
    void Draw(Font font, float openAmount, float time) const;
    bool CheckCollision(Vector2 point) const;
    void SetPosition(float x, float y);
    
    bool IsExpanded() const { return isExpanded; }
    void ToggleExpand() { isExpanded = !isExpanded; }
    int GetSelectedOption() const { return selectedOptionIndex; }

    static bool IsDiagramActive() { return s_DiagramSkinGlobal; }
};

}
}

#endif