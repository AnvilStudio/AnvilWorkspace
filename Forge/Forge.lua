project "Forge"

    -- NOTE --
    -- Windows: VS2022 automatically sets the working dir to the 
    -- start projects directory. 

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir("bin/" .. outdir .. "/%{prj.name}")
    objdir( "bin-int/" .. outdir .. "/%{prj.name}")

    location(".")

    files
    {
        "**.cpp",
        "**.h"
    }

    includedirs 
    {
        "%{ROOTDIR}AnvilEngine/include",
        "%{ROOTDIR}AnvilEngine/vendor/glm", -- TODO: Remove
    }

    links
    {
        "AnvilEngine"
    }

filter "system:macosx"
    architecture "arm64"

    local pythonLinkFlags = os.outputof("python3-config --embed --ldflags")

    defines
    {
        "PLATFORM_APPLE",
        "PLATFORM_MACOS"
    }

    libdirs
    {
        "%{ROOTDIR}AnvilEngine/bin/" .. outdir .. "/AnvilEngine",
        "%{ROOTDIR}AnvilEngine/vendor/GLFW/bin/" .. outdir .. "/GLFW"
    }

    links
    {
        "GLFW",
        "Cocoa.framework",
        "QuartzCore.framework",
        "Metal.framework",
        "MetalKit.framework",
        "IOKit.framework",
        "CoreVideo.framework",
        "CoreFoundation.framework"
    }

    buildoptions
    {
        "-fobjc-arc"
    }

    if pythonLinkFlags ~= nil and pythonLinkFlags ~= "" then
        linkoptions
        {
            pythonLinkFlags
        }
    end

filter "system:windows"
    libdirs
    {
        "%{ROOTDIR}AnvilEngine/vendor/GLFW/" .. outdir .. "/GLFW",
        "%{VULKAN_LIB}"
    }

    links
    {
        "GLFW",
        "vulkan-1",
        "shaderc_combined"
    }

filter "configurations:DebugG"
    defines {
        "DEBUG",
        "DEBUG_G" -- Graphics
    }

    symbols "on"

    filter { "system:windows" }
        linkoptions { "/SUBSYSTEM:CONSOLE" }
        buildoptions { "/MP" } -- windows only

    filter { "system:macosx" }
        linkoptions { "-Wl,-rpath,@loader_path" } -- macOS rpath for shared libraries


filter "configurations:Debug"
    defines "DEBUG"
    symbols "on"

    filter { "system:windows" }
        linkoptions { "/SUBSYSTEM:CONSOLE" }
        buildoptions { "/MP" } -- windows only

    filter { "system:macosx" }
        linkoptions { "-Wl,-rpath,@loader_path" } -- macOS rpath for shared libraries


filter "configurations:Release"
    defines "RELEASE"
    optimize "on"
    kind "WindowedApp" -- Change to WindowedApp for GUI applications

    filter { "system:windows" }
            --linkoptions { "/SUBSYSTEM:WINDOWS" }
            buildoptions { "/MP" } -- windows only

    filter { "system:macosx" }
        linkoptions { "-Wl,-rpath,@loader_path" } -- macOS rpath for shared libraries
