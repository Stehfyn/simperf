project "tests"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir "bin/%{cfg.buildcfg}"
    staticruntime "on"

    files 
    {
        "premake5.lua",
        "**.h",
        "**.hpp",
        "**.cpp"
    }

    includedirs
    {
        ".",
        "%{IncludeDir.spdlog}",
    }

    links
    {
        "simperf",
    }

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        symbols "Off"