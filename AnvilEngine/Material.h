#pragma once
#include "Core/Reference.h"
#include <Asset/AssetTypes/Texture.h>
#include <glm/glm.hpp>

namespace anv
{
class Material :
    public RefCounter
{
public:
    virtual void SetColor(const glm::vec4& color) = 0;
    virtual const glm::vec4& GetColor() const = 0;

    virtual void SetTexture(Ref<Texture> texture) = 0;
    virtual Ref<Texture> GetTexture() const = 0;

protected:
    _shared<Context> m_Context;

    glm::vec4 m_Color;
    Ref<Texture> m_Texture;
};
}

