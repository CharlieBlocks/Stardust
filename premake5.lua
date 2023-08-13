
binDir = "Build/bin/%{cfg.architecture}-%{cfg.buildcfg}"
intDir = "Build/bin-int/%{cfg.architecture}-%{cfg.buildcfg}"

workspace "Stardust"
    architecture "x64"
    startproject "Tests"

    configurations
    {
        "Debug",
        "Release"
    }

    project "Stardust"
        location "Stardust"

        language "C"
        cdialect "c17"

        kind "StaticLib"
        staticruntime "on"

        targetdir (bindDir)
        objdir (intDir)

        files
        {
            "Stardust/src/**.c",
            "Stardust/src/**.h"   
        }

        includedirs
        {
            "./src"
        }

        buildoptions
        {
            "/sdl",
            "/permissive"
        }

        filter "system:windows"
            systemversion "latest"

            defines
            {
                "_STARDUST_WIN32"
            }

        filter "configurations:Debug"
            runtime "Debug"
            symbols "on"

        filter "configurations:Release"
            runtime "Release"
            optimize "on"

    project "Tests"
        location "Tests"

        language "C"
        cdialect "c17"

        kind "ConsoleApp"

        targetdir (binDir)
        objdir (intdir)

        files
        {
            "Tests/main.c"
            --"Tests/**.c",
            --"Tests/**.h"
        }

        links
        {
            "Stardust"
        }

        includedirs
        {
            "Stardust/src"
        }

        buildoptions
        {
            "/sdl",
            "/permissive"
        }

        filter "configurations:Debug"
            runtime "Debug"
            symbols "on"

        filter "configurations:Release"
            runtime "Release"
            optimize "on"
