
project "AnvRuntime"

    -- NOTE:
    -- Windows: VS2022 automatically sets the working directory
    -- to the startup project's directory.

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"

    targetdir("bin/" .. outdir .. "/%{prj.name}")
    objdir("bin-int/" .. outdir .. "/%{prj.name}")

    location(".")

    files
    {
        "**.cpp",
        "**.h"
    }

    includedirs
    {
        "%{ROOTDIR}AnvilEngine/include",
        "%{ROOTDIR}AnvilEngine/vendor/glm"
    }

    links
    {
        "AnvilEngine",
        "Box2D"
    }

    -- Runtime resources
    postbuildcommands
    {
        '{MKDIR} "%{cfg.targetdir}/Anvil"',
        '{COPYDIR} "%{ROOTDIR}AnvilEngine/Resources" "%{cfg.targetdir}/Anvil"'
    }


-- ============================================================
-- macOS
-- ============================================================

filter "system:macosx"

    architecture "arm64"

    defines
    {
        "PLATFORM_APPLE",
        "PLATFORM_MACOS"
    }

    libdirs
    {
        "%{ROOTDIR}AnvilEngine/bin/" .. outdir .. "/AnvilEngine",
        "%{ROOTDIR}AnvilEngine/vendor/GLFW/bin/" .. outdir .. "/GLFW",
        "%{ROOTDIR}AnvilEngine/vendor/Box2d/bin/" .. outdir .. "/Box2D"
    }

    links
    {
        "GLFW",
        "Cocoa.framework",
        "QuartzCore.framework",
        "IOKit.framework",
        "CoreVideo.framework",
        "CoreFoundation.framework"
    }

    buildoptions
    {
        "-fobjc-arc"
    }

    -- Locate and link the embedded Python interpreter.
    local pythonIncludes = os.outputof("python3-config --includes")
    local pythonLinkFlags = os.outputof("python3-config --embed --ldflags")

    if not pythonIncludes or not pythonIncludes:match("%S") then
        error("Failed to retrieve Python include flags on macOS")
    end

    if not pythonLinkFlags or not pythonLinkFlags:match("%S") then
        error("Failed to retrieve Python embedding linker flags on macOS")
    end

    -- Keep compiler and linker options separate.
    -- Preserve complete output so paired framework arguments work.
    buildoptions
    {
        pythonIncludes
    }

    linkoptions
    {
        pythonLinkFlags
    }


-- ============================================================
-- macOS Rendering Backend
-- ============================================================

if MACOS_RENDERER == "vulkan" then

    filter "system:macosx"

        defines
        {
            "ANV_RENDERER_VULKAN",
            "PLATFORM_APPLE_VK"
        }

        includedirs
        {
            "%{VULKAN_SDK}"
        }

        libdirs
        {
            "%{VULKAN_LIB}"
        }

        links
        {
            "vulkan",
            "shaderc_combined"
        }

        -- Allow dyld to locate the Vulkan loader.
        linkoptions
        {
            "-Wl,-rpath," .. VULKAN_LIB
        }

else

    filter "system:macosx"

        defines
        {
            "ANV_RENDERER_METAL"
        }

        links
        {
            "Metal.framework",
            "MetalKit.framework"
        }

end


-- ============================================================
-- Windows
-- ============================================================

filter "system:windows"

    architecture "x86_64"

    defines
    {
        "PLATFORM_WIN64",
        "PLATFORM_WINDOWS",
        "ANV_RENDERER_VULKAN"
    }

    local pythonVersion = PYTHON_VERSION or "313"
    local pythonDir = "%{ROOTDIR}AnvilEngine/Resources/Python"

    includedirs
    {
        pythonDir .. "/include"
    }

    libdirs
    {
        "%{ROOTDIR}AnvilEngine/vendor/GLFW/" .. outdir .. "/GLFW",
        "%{VULKAN_LIB}",
        pythonDir .. "/libs"
    }

    links
    {
        "GLFW",
        "vulkan-1",
        "shaderc_combined",
        "python" .. pythonVersion
    }

    buildoptions
    {
        "/MP"
    }


-- ============================================================
-- DebugG Configuration
-- ============================================================

filter "configurations:DebugG"

    defines
    {
        "DEBUG",
        "DEBUG_G"
    }

    symbols "on"
    optimize "off"

filter { "system:windows", "configurations:DebugG" }

    kind "ConsoleApp"

    linkoptions
    {
        "/SUBSYSTEM:CONSOLE"
    }

filter { "system:macosx", "configurations:DebugG" }

    linkoptions
    {
        "-Wl,-rpath,@loader_path"
    }


-- ============================================================
-- Debug Configuration
-- ============================================================

filter "configurations:Debug"

    defines
    {
        "DEBUG"
    }

    symbols "on"
    optimize "off"

filter { "system:windows", "configurations:Debug" }

    kind "ConsoleApp"

    linkoptions
    {
        "/SUBSYSTEM:CONSOLE"
    }

filter { "system:macosx", "configurations:Debug" }

    linkoptions
    {
        "-Wl,-rpath,@loader_path"
    }


-- ============================================================
-- Release Configuration
-- ============================================================

filter "configurations:Release"

    defines
    {
        "RELEASE"
    }

    optimize "on"
    symbols "off"

filter { "system:windows", "configurations:Release" }

    kind "WindowedApp"

    -- Requires WinMain/wWinMain entry point.
    linkoptions
    {
        "/SUBSYSTEM:WINDOWS"
    }

filter { "system:macosx", "configurations:Release" }

    kind "WindowedApp"

    linkoptions
    {
        "-Wl,-rpath,@loader_path"
    }


-- ============================================================
-- Reset Premake filters
-- ============================================================

filter {}
