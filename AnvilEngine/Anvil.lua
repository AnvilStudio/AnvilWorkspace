project "AnvilEngine"
    location "Anvil"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    warnings "off"
    staticruntime "off"

    targetdir("bin/" .. outdir .. "/%{prj.name}")
    objdir("bin-int/" .. outdir .. "/%{prj.name}")

    location(".")

    files
    {
        "./src/**.cpp",
        "./src/**.h",
        "./include/**.h",

        "./vendor/imgui/imgui.cpp",
        "./vendor/imgui/imgui_demo.cpp",
        "./vendor/imgui/imgui_draw.cpp",
        "./vendor/imgui/imgui_tables.cpp",
        "./vendor/imgui/imgui_widgets.cpp",
        "./vendor/imgui/misc/cpp/imgui_stdlib.cpp",

        "./vendor/ImGuizmo/src/ImGuizmo.cpp",
        "./vendor/ImGuiTextEdit/TextEditor.cpp"
    }

    includedirs
    {
        "./src/",
        "./vendor",
        "./vendor/glm",
        "./vendor/GLFW/include",
        "./vendor/Box2d/include",
        "./vendor/tomlplusplus/include",
        "./vendor/entt/single_include",
        "./vendor/imgui/",
        "./vendor/ImGuizmo/src"
    }

    links
    {
        "Box2D"
    }

    filter "system:windows"
        removefiles { "./src/Render/Platform/Metal/**" }

        includedirs { "%{VULKAN_SDK}" }
        libdirs { "%{VULKAN_LIB}" }

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

            linkoptions { pythonLinkFlags }
        else
            print("Warning: python3-config was not found; Python scripting will be disabled.")
            defines
            {
                "PLATFORM_APPLE",
                "PLATFORM_MACOS"
            }

            buildoptions { "-fobjc-arc" }
        end

        files
        {
            "./src/**.mm",
            "./vendor/imgui/backends/imgui_impl_glfw.h",
            "./vendor/imgui/backends/imgui_impl_glfw.cpp",
            "./vendor/imgui/backends/imgui_impl_metal.h",
            "./vendor/imgui/backends/imgui_impl_metal.mm"
        }

        removefiles { "./src/Render/Platform/Vulkan/**" }

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
        defines { "DEBUG", "DEBUG_G" }
        symbols "on"

    filter { "system:windows", "configurations:DebugG" }
        linkoptions { "/SUBSYSTEM:CONSOLE" }
        buildoptions { "/MP" }

    filter { "system:macosx", "configurations:DebugG" }
        linkoptions { "-Wl,-rpath,@loader_path" }

    filter "configurations:Debug"
        defines "DEBUG"
        symbols "on"

    filter { "system:windows", "configurations:Debug" }
        linkoptions { "/SUBSYSTEM:CONSOLE" }
        buildoptions { "/MP" }

    filter { "system:macosx", "configurations:Debug" }
        linkoptions { "-Wl,-rpath,@loader_path" }

    filter "configurations:Release"
        defines "RELEASE"
        optimize "on"

    filter { "system:windows", "configurations:Release" }
        buildoptions { "/MP" }

    filter { "system:macosx", "configurations:Release" }
        linkoptions { "-Wl,-rpath,@loader_path" }

    filter {}
