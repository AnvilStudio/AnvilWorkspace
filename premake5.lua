workspace "AnvilWorkspace"

    architecture "x64"
    startproject "Forge" 
    

    configurations 
    {
        "DebugG", -- graphics debugging
        "Debug",
        "Release"
    }

    VULKAN_SDK = os.getenv("VULKAN_SDK")
    print("Vulkan SDK: ", VULKAN_SDK)
    outdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    ROOTDIR = os.getcwd() .. "/"
    print("Root Directory: ", ROOTDIR)

    group "Dependencies"
        include "AnvilEngine/vendor/GLFW/glfw.lua"
    group ""

    group "Engine"
        include "AnvilEngine/Anvil.lua"
    group ""
 
    group "Editor"
        include "Forge/Forge.lua"
    group ""