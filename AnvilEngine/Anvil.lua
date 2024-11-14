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

        print("Install Dir: ", INSTALL_DIR)

        files
        {
            "src/**.cpp",
            "src/**.h",
            "include/**.h"
        }

        includedirs
        {
            "src/",
            "vendor/GLFW/include",
            "%{VULKAN_SDK}/include"
        }

        libdirs 
        {
            "vendor/GLFW/".. outdir .."/GLFW",
            "%{VULKAN_SDK}/Lib",
        }

        links
        {
            "GLFW",
            "vulkan-1",
        }


        buildoptions { "/MP" }

        filter "configurations:DebugG"
            defines {
                "DEBUG",
                "DEBUG_G" -- Graphics
            }
            symbols "on"
            linkoptions { "/SUBSYSTEM:CONSOLE" }


        filter "configurations:Debug"
            defines "DEBUG"
            symbols "on"
            linkoptions { "/SUBSYSTEM:CONSOLE" }

        filter "configurations:Release"
            defines "RELEASE"
            optimize "on"
            linkoptions { "/SUBSYSTEM:WINDOWS" }
