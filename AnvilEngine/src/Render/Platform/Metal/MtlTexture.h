#pragma once

#include <Asset/AssetTypes/Texture.h>

namespace anv
{
    class MetalTexture final : public Texture
    {
    public:
        explicit MetalTexture(const std::filesystem::path& path);
        explicit MetalTexture(Deserialized& deserialized);
        ~MetalTexture() override;

        bool IsGPUReady() const override;
        void* GetNativeHandle() const override;

    private:
        void Upload();
        void Destroy();

    private:
        void* m_Texture = nullptr;
    };
}
