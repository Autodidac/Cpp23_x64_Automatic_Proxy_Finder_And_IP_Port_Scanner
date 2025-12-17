@echo off
setlocal

:: Configuration settings
set BUILD_DIR=build-msvc
set GENERATOR="Visual Studio 17 2022"
set PLATFORM=x64
set CONFIG=Release

:: Remove existing build directory
echo Removing build directory: %BUILD_DIR%
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)

:: Configure project with CMake
echo Configuring project...
cmake -S . -B %BUILD_DIR% -G %GENERATOR% -A %PLATFORM%

if %errorlevel% neq 0 (
    echo Error in CMake configuration
    exit /b %errorlevel%
)

:: Build project
echo Building %CONFIG% configuration...
cmake --build %BUILD_DIR% --config %CONFIG%

if %errorlevel% neq 0 (
    echo Error in build process
    exit /b %errorlevel%
)

echo Build completed successfully
endlocal