project "AnvilEngine"
        location "Anvil"
        kind "StaticLib"
        language "C++"
        cppdialect "C++20"
        warnings "off"

        targetdir("bin/" .. outdir .. "/%{prj.name}")
        objdir( "bin-int/" .. outdir .. "/%{prj.name}")

        location(".")

        defines
        {

        }

        files
        {
            "./src/**.cpp",
            "./src/**.h",
            "./include/**.h"
        }

        includedirs
        {
            "./src/",
            "./vendor/GLFW/include",
            "./vendor/glm",
            "%{VULKAN_SDK}"
        }

        libdirs 
        {
            "vendor/GLFW/".. outdir .."/GLFW",
            "%{VULKAN_LIB}",
        }

        filter "system:windows"
            links
            {
                "GLFW",
                "vulkan-1",
                "shaderc_combinedd"
            }

        filter "system:macosx"
            links
            {
                "GLFW",
                "vulkan",
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
        
            filter { "system:windows" }
                    linkoptions { "/SUBSYSTEM:WINDOWS" }
                    buildoptions { "/MP" } -- windows only

            filter { "system:macosx" }
                linkoptions { "-Wl,-rpath,@loader_path" } -- macOS rpath for shared libraries