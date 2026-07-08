-- cleaning gen project files
newaction {
    trigger     = "clean",
    description = "Clean all generated project files and binaries",
    execute     = function()
        print("Cleaning project files...")
        os.rmdir("bin")
        os.rmdir("bin-int")
        os.rmdir("build")

        -- Remove generated project files
        os.remove("Makefile")
        os.remove("*.make")
        os.remove("*.sln")
        os.remove("*.vcxproj")
        os.remove("*.vcxproj.*")
        os.remove("*.xcodeproj")
        os.remove("*.xcworkspace")
        print("Clean complete.")
    end
}


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
        architecture "arm64"
        VULKAN_SDK = "/usr/local/include/vulkan"
        VULKAN_LIB = "/usr/local/lib"

    filter "system:windows"
        VULKAN_SDK = os.getenv("VULKAN_SDK") .. "/include"
        VULKAN_LIB = os.getenv("VULKAN_SDK") .. "/Lib"

    print("Vulkan SDK: ", VULKAN_SDK)
    