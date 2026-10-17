@echo off
:: Check for Administrator privileges
net session >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Requesting Administrator privileges to install to Program Files...
    powershell -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

echo Installing VST Synth V1...

:: Source directory
set "SRC=%~dp0build\VSTSynthV1_artefacts\Release\VST3\VST Synth V1.vst3"
if not exist "%SRC%" (
    set "SRC=%~dp0build\VSTSynthV1_artefacts\Debug\VST3\VST Synth V1.vst3"
)

if not exist "%SRC%" (
    echo Error: Build artefacts not found at "%SRC%". Please build the project first.
    pause
    exit /b 1
)

:: Install to System VST3 folder
echo Copying to "C:\Program Files\Common Files\VST3\VST Synth V1.vst3"...
xcopy "%SRC%" "C:\Program Files\Common Files\VST3\VST Synth V1.vst3" /E /I /Y /Q >nul

:: Install to User VST3 folder
echo Copying to "%LOCALAPPDATA%\Programs\Common\VST3\VST Synth V1.vst3"...
xcopy "%SRC%" "%LOCALAPPDATA%\Programs\Common\VST3\VST Synth V1.vst3" /E /I /Y /Q >nul

if %ERRORLEVEL% equ 0 (
    echo ========================================================
    echo VST Synth V1 installed successfully to all VST3 folders!
    echo ========================================================
) else (
    echo Error copying files.
)

timeout /t 3
