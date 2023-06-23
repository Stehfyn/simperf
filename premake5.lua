-- premake5.lua
include "external/premake/premake_customization/solution_items.lua"

workspace "simperf"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "simperf"

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    solution_items {"premake5.lua", "external.lua", "tests/Build-Solution.bat", "tests/Clean-Solution.bat"}
    
include "external.lua"
