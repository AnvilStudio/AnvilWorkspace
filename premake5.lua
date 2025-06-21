workspace "AnvilWorkspace"
    architecture "x64"
    startproject "Forge"

    configurations 
    {
        "DebugG",    -- graphics debugging
        "Debug",
        "Release"
    }

    -- Paths and Environment Variables

    outdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    ROOTDIR = os.getcwd() .. "/"
    print("Root Directory: ", ROOTDIR)

    -- Dependencies
    group "Dependencies"
        include "AnvilEngine/vendor/GLFW/glfw.lua"
    group ""

    -- Engine
    group "Engine"
        include "AnvilEngine/Anvil.lua"
    group ""

    -- Editor
    group "Editor"
        include "Forge/Forge.lua"
    group ""

    filter "system:windows"
        VULKAN_SDK = os.getenv("VULKAN_SDK")

    --filter "system:linux"
        --VULKAN_SDK = os.getenv("VULKAN_SDK") or "/usr/include/vulkan"

    filter "system:macosx"
        VULKAN_SDK = "/usr/local/include/vulkan"
        VULKAN_LIB = "/usr/local/lib"

    print("Vulkan SDK: ", VULKAN_SDK)
    