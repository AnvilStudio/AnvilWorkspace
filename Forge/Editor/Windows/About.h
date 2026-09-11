#pragma once
#include <Anvil.h>

using namespace anv;

class About
{
public:
    void Draw();
    void SetOpen(bool open);

private:
    bool m_DoDraw;
};