#ifndef CURTAIN_VFX_H
#define CURTAIN_VFX_H

#include <raylib.h>
#include <functional>

class CurtainVFX {
public:
    enum State {
        IDLE,
        CLOSING,
        HOLD,
        OPENING
    };

    CurtainVFX();
    void Init();
    void Start(std::function<void()> onMidPoint);
    void Update(float dt);
    void Draw(int screenWidth, int screenHeight);
    bool IsActive() const;

private:
    State currentState;
    float progress;
    float speed;
    std::function<void()> midPointCallback;
};

#endif