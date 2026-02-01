#pragma once
#include "../Input.h"

namespace anv
{
    class GLFWInputSystem : public InputSystem
    {
    public:
        explicit GLFWInputSystem(_shared<Window> _win);

    protected:
        bool is_key_pressed(int _code) override;
        bool is_mouse_button_pressed(int _button) override;
        std::pair<float, float> get_mouse_pos() override;
    };
}
