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
    -- startproject "Forge"
    startproject "AnvRuntime"

    configurations
    {
        "DebugG",
        "Debug",
        "Release"
    }

    local rendererSuffix = ""
    if os.host() == "macosx" then
        rendererSuffix = "-" .. MACOS_RENDERER
    end

    outdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}" .. rendererSuffix
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

    group "Runtime Player"
        include "AnvRuntime/AnvRuntime.lua"
    group ""

    filter "system:windows"
        VULKAN_SDK = os.getenv("VULKAN_SDK") .. "/include"
        VULKAN_LIB = os.getenv("VULKAN_SDK") .. "/Lib"

    filter "system:macosx"
        architecture "arm64"

    filter {}
