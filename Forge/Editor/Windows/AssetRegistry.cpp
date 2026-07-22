#include "AssetRegistry.h"

#include <algorithm>
#include <cctype>

using namespace anv;

namespace
{
    std::string ToLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character));
            });

        return value;
    }

    bool ContainsInsensitive(
        const std::string& value,
        const std::string& search)
    {
        if (search.empty())
            return true;

        return ToLower(value).find(ToLower(search))
            != std::string::npos;
    }

    const char* RegistryStatus(
        const AssetRegistryEntry& entry)
    {
        if (entry.resource.string() == "")
            return "No Resource";

        if (!entry.resourceExists)
            return "Missing Resource";

        if (!entry.hasMetadata)
            return "No Metadata";

        if (!entry.metadataExists)
            return "Missing Metadata";

        if (!entry.indexedByResource)
            return "Not Indexed";

        return "OK";
    }

    bool RegistryHasProblem(
        const AssetRegistryEntry& entry)
    {
        return !entry.hasResource ||
               !entry.resourceExists ||
               !entry.hasMetadata ||
               !entry.metadataExists ||
               !entry.indexedByResource;
    }

    const char* ResourceIndexStatus(
        const ResourceIndexEntry& entry)
    {
        if (!entry.resolvesToAsset)
            return "Broken UUID";

        if (!entry.resourceExists)
            return "Missing Resource";

        return "OK";
    }

    bool ResourceIndexHasProblem(
        const ResourceIndexEntry& entry)
    {
        return !entry.resolvesToAsset ||
               !entry.resourceExists;
    }

    void CopyText(const std::string& value)
    {
        ImGui::SetClipboardText(value.c_str());
    }
}

void AssetRegistryPanel::Draw(bool* open)   
{
    if (!ImGui::Begin("Asset Registry", open))
    {
        ImGui::End();
        return;
    }

    auto assetManager =
        App::GetInstance()->GetAssetManager();

    if (!assetManager)
    {
        ImGui::TextDisabled(
            "Asset manager is unavailable.");

        ImGui::End();
        return;
    }

    ImGui::SetNextItemWidth(300.0f);

    ImGui::InputTextWithHint(
        "##AssetRegistrySearch",
        "Search name, type, UUID, or path...",
        m_Search.data(),
        m_Search.size());

    ImGui::SameLine();

    ImGui::Checkbox(
        "Problems only",
        &m_ProblemsOnly);

    ImGui::Separator();

    const auto assets =
        assetManager->GetRegistrySnapshot();

    const auto resources =
        assetManager->GetResourceIndexSnapshot();

    if (ImGui::BeginTabBar("AssetRegistryTabs"))
    {
        if (ImGui::BeginTabItem("Assets"))
        {
            ImGui::Text(
                "Registered assets: %zu",
                assets.size());

            DrawAssetTable(assets);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Resource Index"))
        {
            ImGui::Text(
                "Indexed resources: %zu",
                resources.size());

            DrawResourceIndexTable(resources);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void AssetRegistryPanel::DrawAssetTable(
    const std::vector<AssetRegistryEntry>& assets)
{
    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit;

    if (!ImGui::BeginTable(
            "AssetRegistryTable",
            7,
            flags,
            ImVec2(0.0f, 0.0f)))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);

    ImGui::TableSetupColumn(
        "Status",
        ImGuiTableColumnFlags_WidthFixed,
        125.0f);

    ImGui::TableSetupColumn(
        "Name",
        ImGuiTableColumnFlags_WidthFixed,
        180.0f);

    ImGui::TableSetupColumn(
        "Type",
        ImGuiTableColumnFlags_WidthFixed,
        120.0f);

    ImGui::TableSetupColumn(
        "UUID",
        ImGuiTableColumnFlags_WidthFixed,
        310.0f);

    ImGui::TableSetupColumn(
        "Resource",
        ImGuiTableColumnFlags_WidthFixed,
        440.0f);

    ImGui::TableSetupColumn(
        "Metadata",
        ImGuiTableColumnFlags_WidthFixed,
        440.0f);

    ImGui::TableSetupColumn(
        "Indexed",
        ImGuiTableColumnFlags_WidthFixed,
        75.0f);

    ImGui::TableHeadersRow();

    for (const auto& entry : assets)
    {
        const bool hasProblem =
            RegistryHasProblem(entry);

        if (m_ProblemsOnly && !hasProblem)
            continue;

        if (!MatchesSearch(entry))
            continue;

        ImGui::PushID(entry.uuid.uuid.c_str());

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);

        if (hasProblem)
            ImGui::TextUnformatted(
                RegistryStatus(entry));
        else
            ImGui::TextUnformatted("OK");

        ImGui::TableSetColumnIndex(1);

        ImGui::TextUnformatted(
            entry.name.empty()
                ? "<unnamed>"
                : entry.name.c_str());

        if (ImGui::BeginPopupContextItem(
                "AssetNameContext"))
        {
            if (ImGui::MenuItem("Copy Name"))
                CopyText(entry.name);

            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(2);

        ImGui::TextUnformatted(
            entry.type.empty()
                ? "<unknown>"
                : entry.type.c_str());

        ImGui::TableSetColumnIndex(3);

        ImGui::TextUnformatted(
            entry.uuid.uuid.empty()
                ? "<none>"
                : entry.uuid.uuid.c_str());

        if (ImGui::BeginPopupContextItem(
                "AssetUuidContext"))
        {
            if (ImGui::MenuItem("Copy UUID"))
                CopyText(entry.uuid.uuid);

            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(4);

        const std::string resource =
            entry.resource.empty()
                ? "<none>"
                : entry.resource.string();

        ImGui::TextUnformatted(resource.c_str());

        DrawPathContextMenu(
            "AssetResourceContext",
            entry.resource);

        ImGui::TableSetColumnIndex(5);

        const std::string metadata =
            entry.metadata.empty()
                ? "<none>"
                : entry.metadata.string();

        ImGui::TextUnformatted(metadata.c_str());

        DrawPathContextMenu(
            "AssetMetadataContext",
            entry.metadata);

        ImGui::TableSetColumnIndex(6);

        ImGui::TextUnformatted(
            entry.indexedByResource
                ? "Yes"
                : "No");

        ImGui::PopID();
    }

    ImGui::EndTable();
}

void AssetRegistryPanel::DrawResourceIndexTable(
    const std::vector<ResourceIndexEntry>& entries)
{
    constexpr ImGuiTableFlags flags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_Reorderable |
        ImGuiTableFlags_Hideable |
        ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingFixedFit;

    if (!ImGui::BeginTable(
            "AssetResourceIndexTable",
            6,
            flags,
            ImVec2(0.0f, 0.0f)))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);

    ImGui::TableSetupColumn(
        "Status",
        ImGuiTableColumnFlags_WidthFixed,
        120.0f);

    ImGui::TableSetupColumn(
        "Resource",
        ImGuiTableColumnFlags_WidthFixed,
        520.0f);

    ImGui::TableSetupColumn(
        "UUID",
        ImGuiTableColumnFlags_WidthFixed,
        310.0f);

    ImGui::TableSetupColumn(
        "Asset",
        ImGuiTableColumnFlags_WidthFixed,
        180.0f);

    ImGui::TableSetupColumn(
        "Type",
        ImGuiTableColumnFlags_WidthFixed,
        130.0f);

    ImGui::TableSetupColumn(
        "Resolved",
        ImGuiTableColumnFlags_WidthFixed,
        80.0f);

    ImGui::TableHeadersRow();

    for (const auto& entry : entries)
    {
        const bool hasProblem =
            ResourceIndexHasProblem(entry);

        if (m_ProblemsOnly && !hasProblem)
            continue;

        if (!MatchesSearch(entry))
            continue;

        ImGui::PushID(
            entry.normalizedResource.c_str());

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);

        ImGui::TextUnformatted(
            ResourceIndexStatus(entry));

        ImGui::TableSetColumnIndex(1);

        ImGui::TextUnformatted(
            entry.normalizedResource.empty()
                ? "<none>"
                : entry.normalizedResource.c_str());

        if (ImGui::BeginPopupContextItem(
                "IndexedResourceContext"))
        {
            if (ImGui::MenuItem(
                    "Copy Resource Path"))
            {
                CopyText(entry.normalizedResource);
            }

            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(2);

        ImGui::TextUnformatted(
            entry.uuid.uuid.empty()
                ? "<none>"
                : entry.uuid.uuid.c_str());

        if (ImGui::BeginPopupContextItem(
                "IndexedUuidContext"))
        {
            if (ImGui::MenuItem("Copy UUID"))
                CopyText(entry.uuid.uuid);

            ImGui::EndPopup();
        }

        ImGui::TableSetColumnIndex(3);

        ImGui::TextUnformatted(
            entry.assetName.empty()
                ? "<unresolved>"
                : entry.assetName.c_str());

        ImGui::TableSetColumnIndex(4);

        ImGui::TextUnformatted(
            entry.assetType.empty()
                ? "<unknown>"
                : entry.assetType.c_str());

        ImGui::TableSetColumnIndex(5);

        ImGui::TextUnformatted(
            entry.resolvesToAsset
                ? "Yes"
                : "No");

        ImGui::PopID();
    }

    ImGui::EndTable();
}

bool AssetRegistryPanel::MatchesSearch(
    const AssetRegistryEntry& entry) const
{
    const std::string search(m_Search.data());

    if (search.empty())
        return true;

    return
        ContainsInsensitive(entry.name, search) ||
        ContainsInsensitive(entry.type, search) ||
        ContainsInsensitive(entry.uuid.uuid, search) ||
        ContainsInsensitive(
            entry.resource.string(),
            search) ||
        ContainsInsensitive(
            entry.metadata.string(),
            search);
}

bool AssetRegistryPanel::MatchesSearch(
    const ResourceIndexEntry& entry) const
{
    const std::string search(m_Search.data());

    if (search.empty())
        return true;

    return
        ContainsInsensitive(
            entry.normalizedResource,
            search) ||
        ContainsInsensitive(
            entry.uuid.uuid,
            search) ||
        ContainsInsensitive(
            entry.assetName,
            search) ||
        ContainsInsensitive(
            entry.assetType,
            search);
}

void AssetRegistryPanel::DrawPathContextMenu(
    const char* popupId,
    const std::filesystem::path& path)
{
    if (!ImGui::BeginPopupContextItem(popupId))
        return;

    if (path.empty())
    {
        ImGui::TextDisabled("No path available");
        ImGui::EndPopup();
        return;
    }

    const std::string value =
        path.string();

    if (ImGui::MenuItem("Copy Path"))
        CopyText(value);

    ImGui::EndPopup();
}