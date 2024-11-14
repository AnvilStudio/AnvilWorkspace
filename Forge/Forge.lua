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

filter "configurations:DebugG"
    defines "DEBUG"
    symbols "on"
    linkoptions { "/SUBSYSTEM:CONSOLE" }
    kind "ConsoleApp"

filter "configurations:Debug"
    defines "DEBUG"
    symbols "on"
    linkoptions { "/SUBSYSTEM:CONSOLE" }
    kind "ConsoleApp"

filter "configurations:Release"
    defines "RELEASE"
    optimize "on"
    linkoptions { "/SUBSYSTEM:WINDOWS" }
    kind "WindowedApp"