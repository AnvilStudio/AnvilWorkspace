#pragma once
#include <random>
#include <sstream>
#include <iomanip>

namespace anv
{
namespace uuid
{
    struct EntityUUID
    {
        // probably needs to be a uint32_t
        std::string uuid = "";
    };

    struct AssetUUID 
    {
        // probably needs to be a uint32_t
        std::string uuid = "";
    };

    inline EntityUUID uuid_GenEntID()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        // Generate 10-5-5-5-12 format UUID
        ss << std::setw(10) << dis(gen) << '-';
        ss << std::setw(5) << (dis(gen) & 0xFFFF) << '-';
        ss << std::setw(5) << ((dis(gen) & 0x0FFF) | 0x4000) << '-'; // Version 4 UUID
        ss << std::setw(5) << ((dis(gen) & 0x3FFF) | 0x8000) << '-'; // Variant 1 UUID
        ss << std::setw(12) << ((static_cast<uint64_t>(dis(gen)) << 32) | dis(gen));

        return EntityUUID{ ss.str() };
    }

    inline AssetUUID uuid_GenAssetID()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);

        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        // Generate 8-4-4-4-12 format UUID
        ss << std::setw(8) << dis(gen) << '-';
        ss << std::setw(4) << (dis(gen) & 0xFFFF) << '-';
        ss << std::setw(4) << ((dis(gen) & 0x0FFF) | 0x4000) << '-'; // Version 4 UUID
        ss << std::setw(4) << ((dis(gen) & 0x3FFF) | 0x8000) << '-'; // Variant 1 UUID
        ss << std::setw(12) << ((static_cast<uint64_t>(dis(gen)) << 32) | dis(gen));

        return AssetUUID{ ss.str() };
    }
}
}