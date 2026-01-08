@echo off
setlocal
cd /d "%~dp0.."

echo [1/3] Configuring project (Debug)...
cmake -S . -B build -DBUILD_TESTS=ON
if %errorlevel% neq 0 exit /b %errorlevel%

echo [2/3] Building project...
cmake --build build --config Debug -j
if %errorlevel% neq 0 exit /b %errorlevel%

echo [3/3] Moving executables to root...
if exist "Debug\r-type_client.exe" (
    copy /Y "Debug\r-type_client.exe" ".\r-type_client.exe"
    echo Moved r-type_client.exe to root.
) else (
    echo WARNING: Debug\r-type_client.exe not found!
)

if exist "Debug\r-type_server.exe" (
    copy /Y "Debug\r-type_server.exe" ".\r-type_server.exe"
    echo Moved r-type_server.exe to root.
) else (
    echo WARNING: Debug\r-type_server.exe not found!
)

echo Build and move complete!
endlocal
