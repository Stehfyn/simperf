-- external.lua
IncludeDir = {}
IncludeDir["spdlog"] = "../external/spdlog/include"
IncludeDir["spdlog_setup"] = "../external/spdlog_setup/include"

group "external"
   externalproject "spdlog"
       location "external/spdlog/build"
       kind "StaticLib"
       language "C++"
group ""

group "core"
   include "include/"
   include "tests/"
group ""