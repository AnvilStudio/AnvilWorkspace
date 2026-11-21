#pragma once

#include <Asset/AssetTypes/Texture.h>

namespace anv
{
    class MetalTexture final : public Texture
    {
    public:
        explicit MetalTexture(const std::filesystem::path& path);
        explicit MetalTexture(Deserialized& deserialized);
        MetalTexture(const unsigned char* rgbaPixels, int width, int height, const std::string& name);
        ~MetalTexture() override;

        bool IsGPUReady() const override;
        void* GetNativeHandle() const override;
        void* GetImGuiTextureID() override;

    private:
        void Upload();
        void Destroy();

    private:
        void* m_Texture = nullptr;
    };
}
