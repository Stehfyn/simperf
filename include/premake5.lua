project "simperf"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    targetdir "bin/%{cfg.buildcfg}"
    staticruntime "on"

    files 
    {
        "premake5.lua",
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
        "spdlog",
    }

    defines 
    {
        "SIMPERF_LIB"
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