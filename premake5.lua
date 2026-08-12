-- Optional override for the macOS parallel build action.
newoption {
    trigger = "jobs",
    value = "COUNT",
    description = "Number of parallel macOS build jobs"
}

-- Renderer selection for macOS builds. Metal remains the default so the
-- existing feature-macos workflow is unchanged unless Vulkan is requested.
newoption {
    trigger = "macos-renderer",
    value = "RENDERER",
    description = "Graphics API to use for macOS builds",
    allowed = {
        { "metal", "Metal" },
        { "vulkan", "Vulkan via MoltenVK" }
    }
}

MACOS_RENDERER = _OPTIONS["macos-renderer"] or "metal"

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

    -- Resolve the LunarG Vulkan SDK before project scripts are included so
    -- Anvil.lua and Forge.lua can consume these paths during generation.
    if os.host() == "macosx" and MACOS_RENDERER == "vulkan" then
        local vulkanRoot = os.getenv("VULKAN_SDK")
        if vulkanRoot == nil or vulkanRoot == "" then
            error("--macos-renderer=vulkan requires VULKAN_SDK to point at the LunarG Vulkan SDK.")
        end

        VULKAN_SDK = path.join(vulkanRoot, "include")
        VULKAN_LIB = path.join(vulkanRoot, "lib")
        print("macOS renderer: Vulkan (MoltenVK)")
        print("Vulkan include directory: ", VULKAN_SDK)
        print("Vulkan library directory: ", VULKAN_LIB)
    elseif os.host() == "macosx" then
        print("macOS renderer: Metal")
    end

    group "Dependencies"
        include "AnvilEngine/vendor/GLFW/glfw.lua"
        include "AnvilEngine/vendor/Box2d/Box2D.lua"
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
