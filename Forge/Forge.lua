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
        "AnvilEngine",
        "Box2D"
    }

    postbuildcommands
    {
        '{MKDIR} "%{cfg.targetdir}/Anvil"',
        '{COPYDIR} "%{wks.location}/AnvilEngine/Resources" "%{cfg.targetdir}/Anvil"'
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
        "%{ROOTDIR}AnvilEngine/vendor/GLFW/bin/" .. outdir .. "/GLFW",
        "%{ROOTDIR}AnvilEngine/vendor/Box2d/bin/" .. outdir .. "/Box2D",
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

    if MACOS_RENDERER == "vulkan" then
        defines
        {
            "ANV_RENDERER_VULKAN",
            "PLATFORM_APPLE_VK"
        }
        includedirs { "%{VULKAN_SDK}" }
        libdirs { "%{VULKAN_LIB}" }

        links
        {
            "vulkan",
            "shaderc_combined"
        }

        -- The LunarG Vulkan loader is installed with an @rpath install name.
        -- Add the selected SDK's lib directory so dyld can resolve
        -- libvulkan.1.dylib when Forge is launched outside the SDK shell.
        linkoptions
        {
            "-Wl,-rpath," .. VULKAN_LIB
        }

        -- TODO: Needs to get working.
        postbuildcommands
        {
            "export VK_DRIVER_FILES=\"$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json\"" -- we use MoltenVK
        }
    else
        defines { "ANV_RENDERER_METAL" }

        links
        {
            "Metal.framework",
            "MetalKit.framework"
        }

        buildoptions { "-fobjc-arc" }
    end

    if pythonLinkFlags ~= nil and pythonLinkFlags ~= "" then
        linkoptions
        {
            pythonLinkFlags
        }
    end


filter "system:windows"
        -- Helper function to strip trailing newlines safely from terminal output
        local function strip_newline(str)
            if str == nil then return "" end
            return string.gsub(str, "[\r\n]+$", "")
        end

        -- Retrieve paths via Python sysconfig
        local rawIncludes = os.outputof("python -c \"import sysconfig; print(sysconfig.get_path('include'))\"")
        -- On Windows, Python libraries live in the standard installation scheme prefix / 'libs'
        local rawLibDir   = os.outputof("python -c \"import sysconfig; print(sysconfig.get_config_var('installed_base') + '\\\\libs')\"")
        -- Querying the exact library name dynamically matching the python version (e.g., python312)
        local rawPyVersion = os.outputof("python -c \"import sys; print(f'python{sys.version_info.major}{sys.version_info.minor}')\"")

        local pythonIncludes = strip_newline(rawIncludes)
        local pythonLibDir   = strip_newline(rawLibDir)
        local pythonLibName  = strip_newline(rawPyVersion)

        print("Python Include Dir: " .. pythonIncludes)
        print("Python Lib Dir: " .. pythonLibDir)
        print("Python Link Target: " .. pythonLibName)
    
    libdirs
    {
        "%{ROOTDIR}AnvilEngine/vendor/GLFW/" .. outdir .. "/GLFW",
        "%{VULKAN_LIB}",
        pythonLibDir
    }

    links
    {
        "GLFW",
        "vulkan-1",
        "shaderc_combined",
        pythonLibName
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
