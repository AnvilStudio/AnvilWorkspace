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
            --"./src/**.mm",
            "./src/**.h",
            "./include/**.h",

            "./vendor/imgui/imgui.cpp",
            "./vendor/imgui/imgui_demo.cpp",
            "./vendor/imgui/imgui_draw.cpp",
            "./vendor/imgui/imgui_tables.cpp",
            "./vendor/imgui/imgui_widgets.cpp",
            "./vendor/imgui/misc/cpp/imgui_stdlib.cpp"
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
            --"%{VULKAN_SDK}"
        }

        libdirs 
        {
            "vendor/GLFW/".. outdir .."/GLFW",
            --"%{VULKAN_LIB}",   
        }

        filter "system:windows"
            removefiles
            {
                "./src/Render/Platform/Metal/**",
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
                "GLFW",
                "vulkan-1",
                "shaderc_combinedd"
            }

        filter "system:macosx"
            architecture "arm64"

            local pythonIncludes = os.outputof("python3-config --includes")
            local pythonLinkFlags = os.outputof("python3-config --embed --ldflags")

            if pythonIncludes ~= nil and pythonIncludes ~= "" and
               pythonLinkFlags ~= nil and pythonLinkFlags ~= "" then
                defines
                {
                    "PLATFORM_APPLE",
                    "PLATFORM_MACOS",
                    "ANV_ENABLE_PYTHON"
                }

                buildoptions
                {
                    "-fobjc-arc",
                    pythonIncludes
                }

                linkoptions
                {
                    pythonLinkFlags
                }
            else
                print("Warning: python3-config was not found; Python scripting will be disabled.")
                defines
                {
                    "PLATFORM_APPLE",
                    "PLATFORM_MACOS"
                }

                buildoptions
                {
                    "-fobjc-arc"
                }
            end

            files
            {
                "./src/**.mm",

                -- ImGui + GLFW
                "./vendor/imgui/backends/imgui_impl_glfw.h",
                "./vendor/imgui/backends/imgui_impl_glfw.cpp",
                -- ImGui + Metal
                "./vendor/imgui/backends/imgui_impl_metal.h",
                "./vendor/imgui/backends/imgui_impl_metal.mm"
            }
            removefiles
            {
                "./src/Render/Platform/Vulkan/**"
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
