project "Box2D"
    kind "StaticLib"
    language "C"
    cdialect "C17"
    staticruntime "off"
    warnings "off"

    targetdir ("bin/" .. outdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outdir .. "/%{prj.name}")

    files
    {
        "include/box2d/**.h",
        "src/**.h",
        "src/**.c"
    }

    includedirs
    {
        "include",
        "src"
    }

    filter "system:windows"
        systemversion "latest"
        defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "system:linux"
        pic "On"

    filter "system:macosx"
        pic "On"
        architecture "arm64"

    filter "configurations:DebugG"
        symbols "On"

    filter "configurations:Debug"
        symbols "On"

    filter "configurations:Release"
        optimize "Speed"

    filter {}
