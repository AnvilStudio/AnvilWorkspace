#pragma once

#include "../Asset.h"

namespace anv
{
    class Texture : public Asset
    {
    public:
        ~Texture() override;

        static Ref<Texture> Create(const std::filesystem::path& path);
        static Ref<Texture> Create(Deserialized& deserialized);

        int Width() const { return m_Width; }
        int Height() const { return m_Height; }
        int Channels() const { return m_Channels; }
        unsigned char* Data() const { return m_Data; }

        virtual bool IsGPUReady() const = 0;
        virtual void* GetNativeHandle() const = 0;

    protected:
        explicit Texture(const std::filesystem::path& path);
        explicit Texture(Deserialized& deserialized);
        explicit Texture(const std::string& internalName);

        void Load();
        void Unload();
        void OnSave(Serializer& serializer) override;

    protected:
        int m_Width = 0;
        int m_Height = 0;
        int m_Channels = 0;
        unsigned char* m_Data = nullptr;
    };
}
