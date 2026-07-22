#pragma once

#include <Asset/AssetTypes/Texture.h>

namespace anv
{
    class VulkanTexture final : public Texture
    {
    public:
        explicit VulkanTexture(const std::filesystem::path& path);
        explicit VulkanTexture(Deserialized& deserialized);
        ~VulkanTexture() override;

        bool IsGPUReady() const override;
        void* GetNativeHandle() const override;

    private:
        void Upload();
        void Destroy();

    private:
        void* m_Image = nullptr;
    };
}
