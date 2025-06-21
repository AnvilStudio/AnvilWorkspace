project "Forge"
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
        "%{ROOTDIR}AnvilEngine/include"
    }

    libdirs 
    {
        "%{ROOTDIR}AnvilEngine/" .. outdir .. "/AnvilEngine"
    }

    links
    {
        "AnvilEngine"
    }

filter "system:macosx"
    libdirs
    {
        "%{ROOTDIR}AnvilEngine/vendor/GLFW/" .. outdir .. "/GLFW",
        "%{VULKAN_LIB}"
    }

    links
    {
        "GLFW",
        "vulkan",
        "shaderc_combined",

        "Cocoa.framework",
        "QuartzCore.framework",
        "IOKit.framework",
        "Metal.framework",      -- Required by MoltenVK
        "CoreFoundation.framework"
    }

    postbuildcommands {
        "install_name_tool -add_rpath " .. "\"%{VULKAN_LIB}\"" .. " %{cfg.targetdir}/%{cfg.buildtarget.name}"
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
                    linkoptions { "/SUBSYSTEM:WINDOWS" }
                    buildoptions { "/MP" } -- windows only

            filter { "system:macosx" }
                linkoptions { "-Wl,-rpath,@loader_path" } -- macOS rpath for shared libraries