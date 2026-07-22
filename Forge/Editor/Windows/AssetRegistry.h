#pragma once

#include <Anvil.h>

#include <array>
#include <string>

class AssetRegistryPanel
{
public:
    AssetRegistryPanel() = default;

    void Draw(bool* open = nullptr);

private:
    void DrawAssetTable(
        const std::vector<anv::AssetRegistryEntry>& assets);

    void DrawResourceIndexTable(
        const std::vector<anv::ResourceIndexEntry>& entries);

    bool MatchesSearch(
        const anv::AssetRegistryEntry& entry) const;

    bool MatchesSearch(
        const anv::ResourceIndexEntry& entry) const;

    static void DrawPathContextMenu(
        const char* popupId,
        const std::filesystem::path& path);

private:
    std::array<char, 256> m_Search{};
    bool m_ProblemsOnly = false;
};