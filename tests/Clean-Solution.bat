@echo off
setlocal

set root="%~dp0..\"
pushd %root%

rem explicit for now
rem Visual Studio Solution Files
del /q /f "*.sln"

del /q /f /s "include\*.vcxproj"
del /q /f /s "include\*.vcxproj.user"
del /q /f /s "include\*.vcxproj.filters"

del /q /f /s "tests\*.vcxproj"
del /q /f /s "tests\*.vcxproj.user"
del /q /f /s "tests\*.vcxproj.filters"

rmdir /q /s external\spdlog\build\

rmdir /q /s bin\
rmdir /q /s bin-int\

echo %cmdcmdline%|find /i """%~f0""">nul && (
    REM Batchfile was double clicked
    pause
) || (
    REM Batchfile was executed from the console
)

popd
endlocal