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

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")
   
    filter "toolset:msc"
        defines { "SP_TOOLSET_MSC" }

    filter "configurations:Debug"
        defines { "SP_DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "SP_RELEASE" }
        runtime "Release"
        optimize "On"
        symbols "Off"