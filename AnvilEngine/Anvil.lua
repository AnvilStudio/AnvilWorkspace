project "AnvilEngine"
        location "Anvil"
        kind "StaticLib"
        language "C++"
        cppdialect "C++20"
        warnings "off"
        staticruntime "off"

        targetdir("bin/" .. outdir .. "/%{prj.name}")
        objdir( "bin-int/" .. outdir .. "/%{prj.name}")

        location(".")

        defines
        {

        }

        files
        {
            "./src/**.cpp",
            "./src/**.mm",
            "./src/**.h",
            "./include/**.h",
        }

        includedirs
        {
            "./src/",
            "./vendor",
            "./vendor/glm",
            "./vendor/GLFW/include",
            "./vendor/tomlplusplus/include",
            "./vendor/entt/single_include",
            "./vendor/imgui/",
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
            defines
            {
                "PLATFORM_APPLE",
                "PLATFORM_MACOS",
                --"ANV_RENDER_API_METAL"
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
                    --linkoptions { "/SUBSYSTEM:WINDOWS" }
                    buildoptions { "/MP" } -- windows only

            filter { "system:macosx" }
                linkoptions { "-Wl,-rpath,@loader_path" } -- macOS rpath for shared libraries