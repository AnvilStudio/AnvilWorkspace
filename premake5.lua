-- Optional override for the macOS parallel build action.
newoption {
    trigger = "jobs",
    value = "COUNT",
    description = "Number of parallel macOS build jobs"
}

local function get_macos_job_count()
    local requestedJobs = tonumber(_OPTIONS["jobs"])
    if requestedJobs ~= nil and requestedJobs > 0 then
        return math.floor(requestedJobs)
    end

    local detectedJobs = tonumber(os.outputof("sysctl -n hw.logicalcpu"))
    if detectedJobs ~= nil and detectedJobs > 0 then
        return math.floor(detectedJobs)
    end

    return 1
end

newaction {
    trigger = "build-macos",
    description = "Build Anvil on macOS using parallel compilation",
    execute = function()
        if os.host() ~= "macosx" then
            error("The build-macos action can only run on macOS.")
        end

        local jobs = get_macos_job_count()
        local command = nil

        if os.isfile("Makefile") then
            command = string.format("make -j%d", jobs)
        elseif os.isdir("AnvilWorkspace.xcworkspace") then
            command = string.format(
                "xcodebuild -workspace AnvilWorkspace.xcworkspace -scheme Forge -parallelizeTargets -jobs %d build",
                jobs)
        elseif os.isdir("AnvilWorkspace.xcodeproj") then
            command = string.format(
                "xcodebuild -project AnvilWorkspace.xcodeproj -scheme Forge -parallelizeTargets -jobs %d build",
                jobs)
        else
            error("No generated Makefile or Xcode project was found. Generate one before building.")
        end

        print(string.format("Building with %d parallel jobs...", jobs))
        local result = os.execute(command)
        if result ~= 0 then
            error("macOS build failed.")
        end
    end
}

newaction {
    trigger = "clean",
    description = "Clean all generated project files and binaries",
    execute = function()
        print("Cleaning project files...")
        os.rmdir("bin")
        os.rmdir("bin-int")
        os.rmdir("build")
        os.remove("Makefile")
        os.remove("*.make")
        os.remove("*.sln")
        os.remove("*.vcxproj")
        os.remove("*.vcxproj.*")
        os.remove("*.xcodeproj")
        os.remove("*.xcworkspace")
        print("Clean complete.")
    end
}

workspace "AnvilWorkspace"
    architecture "x64"
    startproject "Forge"

    configurations
    {
        "DebugG",
        "Debug",
        "Release"
    }

    outdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    ROOTDIR = os.getcwd() .. "/"
    print("Root Directory: ", ROOTDIR)

    group "Dependencies"
        include "AnvilEngine/vendor/GLFW/glfw.lua"
        include "AnvilEngine/vendor/Box2D.lua"
    group ""

    group "Engine"
        include "AnvilEngine/Anvil.lua"
    group ""

    group "Editor"
        include "Forge/Forge.lua"
    group ""

    filter "system:windows"
        VULKAN_SDK = os.getenv("VULKAN_SDK") .. "/include"
        VULKAN_LIB = os.getenv("VULKAN_SDK") .. "/Lib"

    filter "system:macosx"
        architecture "arm64"

    filter {}
