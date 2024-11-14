workspace "AnvilWorkspace"

    architecture "x64"
    startproject "Forge" 
    

    configurations 
    {
        "DebugG",
        "Debug",
        "Release"
    }

    VULKAN_SDK = os.getenv("VULKAN_SDK")
    print("Vulkan SDK: ", VULKAN_SDK)
    outdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    ROOTDIR = os.getcwd() .. "/"
    print("Root Directory: ", root_dir)
--    vendor_dir = root_dir .. "Anvil/include/"
--    print("Vendor Directory: ", vendor_dir)

    group "Dependencies"
        include "AnvilEngine/vendor/GLFW/glfw.lua"
    group ""

    group "Engine"
        include "AnvilEngine/Anvil.lua"
    group ""

    group "Editor"
        include "Forge/Forge.lua"
    group ""