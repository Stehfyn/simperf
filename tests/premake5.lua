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
   
    filter "toolset:msc"
        defines { "SP_TOOLSET_MSC" }

    filter "configurations:Debug"
        defines { "SP_TESTS_DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "SP_TESTS_RELEASE" }
        runtime "Release"
        optimize "On"
        symbols "Off"