#include "UI_Button.h"
#include <cmath>
#include <algorithm>

namespace Skin {
namespace Button {

bool UI_Button::s_DiagramSkinGlobal = false;

static float Clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

static float EaseOutBack(float value) {
    value = Clamp01(value);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(value - 1.0f, 3.0f) + c1 * std::pow(value - 1.0f, 2.0f);
}

static Color WithAlpha(Color color, float alpha) {
    color.a = (unsigned char)(Clamp01(alpha) * 255.0f);
    return color;
}

UI_Button::UI_Button(float x, float y, float width, float height, const std::string& btnLabel, const std::string& btnIndex, int btnIndexNum)
    : rect{ x, y, width, height }, label(btnLabel), indexText(btnIndex), index(btnIndexNum), hoverProgress(0.0f), selectProgress(0.0f), isExpanded(false), selectedOptionIndex(s_DiagramSkinGlobal ? 1 : 0) {
    
    options.push_back({ "기본", !s_DiagramSkinGlobal });
    options.push_back({ "도형", s_DiagramSkinGlobal });
}

void UI_Button::Update(float dt, bool isSelected, bool isHovered, Vector2 mousePos, bool mousePressed) {
    float targetHover = isHovered ? 1.0f : 0.0f;
    hoverProgress += (targetHover - hoverProgress) * std::min(1.0f, dt * 12.0f);

    float targetSelect = isSelected ? 1.0f : 0.0f;
    selectProgress += (targetSelect - selectProgress) * std::min(1.0f, dt * 15.0f);

    if (isExpanded && mousePressed) {
        float uiScale = (float)GetScreenHeight() / 720.0f;
        float optBoxH = 36.0f * uiScale;
        float startY = rect.y + rect.height + 6.0f;

        for (size_t i = 0; i < options.size(); ++i) {
            Rectangle optRect = { rect.x, startY + i * (optBoxH + 4.0f), rect.width, optBoxH };
            if (CheckCollisionPointRec(mousePos, optRect)) {
                for (size_t j = 0; j < options.size(); ++j) {
                    options[j].isChecked = (j == i);
                }
                selectedOptionIndex = (int)i;
                s_DiagramSkinGlobal = (selectedOptionIndex == 1);
                break;
            }
        }
    }
}

void UI_Button::Draw(Font font, float openAmount, float time) const {
    float uiScale = (float)GetScreenHeight() / 720.0f;
    float itemOpen = EaseOutBack(openAmount);
    float appear = Clamp01(openAmount * 1.3f - index * 0.055f);

    if (appear <= 0.01f) return;

    Color baseColor = { 135, 206, 235, 255 };
    Color secondColor = { 255, 182, 193, 255 };
    Color thirdColor = { 221, 160, 221, 255 };

    if (index == 0) {
        baseColor = { 255, 182, 193, 255 };
        secondColor = { 135, 206, 235, 255 };
        thirdColor = { 152, 251, 152, 255 };
    } else if (index == 1) {
        baseColor = { 253, 238, 154, 255 };
        secondColor = { 152, 251, 152, 255 };
        thirdColor = { 221, 160, 221, 255 };
    } else if (index == 2) {
        baseColor = { 152, 251, 152, 255 };
        secondColor = { 255, 182, 193, 255 };
        thirdColor = { 135, 206, 235, 255 };
    } else if (index == 3) {
        baseColor = { 221, 160, 221, 255 };
        secondColor = { 253, 238, 154, 255 };
        thirdColor = { 135, 206, 235, 255 };
    }

    Rectangle box = {
        rect.x,
        rect.y + std::sin(time * 1.5f + index * 0.7f) * 1.2f * uiScale,
        rect.width,
        rect.height
    };

    Color panel = selectProgress > 0.5f ? Color{ 255, 255, 255, (unsigned char)(245.0f * appear) }
                                        : WithAlpha(baseColor, 0.65f * appear);

    DrawRectangleRounded(box, 0.5f, 16, panel);
    DrawRectangleRoundedLines(box, 0.5f, 16, WithAlpha(secondColor, appear * 0.85f));

    Rectangle innerBox = { box.x + 4, box.y + 4, box.width - 8, box.height - 8 };
    DrawRectangleRoundedLines(innerBox, 0.5f, 16, WithAlpha(thirdColor, appear * 0.4f));

    DrawTextEx(font, indexText.c_str(), { box.x + 14.0f * uiScale, box.y + 12.0f * uiScale }, 10.0f * uiScale, 1.0f * uiScale, WithAlpha(thirdColor, appear * 0.95f));

    float labelFontSize = (20.0f + selectProgress * 3.0f) * uiScale;
    Vector2 labelSize = MeasureTextEx(font, label.c_str(), labelFontSize, 1.0f * uiScale);
    Color textColor = selectProgress > 0.5f ? Color{ 60, 50, 60, (unsigned char)(255.0f * appear) } : Color{ 255, 255, 255, (unsigned char)(245.0f * appear) };
    DrawTextEx(font, label.c_str(), { box.x + (box.width - labelSize.x) * 0.5f, box.y + (box.height - labelSize.y) * 0.5f + 2.0f * uiScale }, labelFontSize, 1.0f * uiScale, textColor);

    if (isExpanded) {
        float optBoxH = 36.0f * uiScale;
        float startY = box.y + box.height + 6.0f;

        for (size_t i = 0; i < options.size(); ++i) {
            Rectangle optRect = { box.x, startY + i * (optBoxH + 4.0f), box.width, optBoxH };
            DrawRectangleRounded(optRect, 0.3f, 8, Color{ 20, 24, 32, 235 });
            DrawRectangleRoundedLines(optRect, 0.3f, 8, WithAlpha(baseColor, 0.8f));

            float checkSize = 18.0f * uiScale;
            Rectangle checkRect = { optRect.x + 12.0f * uiScale, optRect.y + (optRect.height - checkSize) * 0.5f, checkSize, checkSize };
            DrawRectangleRounded(checkRect, 0.2f, 4, Color{ 40, 45, 55, 255 });
            DrawRectangleRoundedLines(checkRect, 0.2f, 4, Color{ 200, 200, 200, 255 });

            if (options[i].isChecked) {
                Rectangle innerCheck = { checkRect.x + 4.0f * uiScale, checkRect.y + 4.0f * uiScale, checkSize - 8.0f * uiScale, checkSize - 8.0f * uiScale };
                DrawRectangleRounded(innerCheck, 0.2f, 4, baseColor);
            }

            Vector2 optTextSize = MeasureTextEx(font, options[i].name.c_str(), 16.0f * uiScale, 1.0f * uiScale);
            DrawTextEx(font, options[i].name.c_str(), { checkRect.x + checkRect.width + 12.0f * uiScale, optRect.y + (optRect.height - optTextSize.y) * 0.5f }, 16.0f * uiScale, 1.0f * uiScale, Color{ 240, 240, 240, 255 });
        }
    }
}

bool UI_Button::CheckCollision(Vector2 point) const {
    if (CheckCollisionPointRec(point, rect)) return true;

    if (isExpanded) {
        float uiScale = (float)GetScreenHeight() / 720.0f;
        float optBoxH = 36.0f * uiScale;
        float startY = rect.y + rect.height + 6.0f;
        for (size_t i = 0; i < options.size(); ++i) {
            Rectangle optRect = { rect.x, startY + i * (optBoxH + 4.0f), rect.width, optBoxH };
            if (CheckCollisionPointRec(point, optRect)) return true;
        }
    }
    return false;
}

void UI_Button::SetPosition(float x, float y) {
    rect.x = x;
    rect.y = y;
}

}
}