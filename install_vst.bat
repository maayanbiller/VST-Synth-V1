@echo off
echo Installing VST Synth V1...
if exist "C:\Users\maaya\projects\Synth\VST-Synth-V1\build\VSTSynthV1_artefacts\Release\VST3\VST Synth V1.vst3" (
    xcopy "C:\Users\maaya\projects\Synth\VST-Synth-V1\build\VSTSynthV1_artefacts\Release\VST3\VST Synth V1.vst3" "C:\Program Files\Common Files\VST3\VST Synth V1.vst3" /E /I /Y
) else (
    xcopy "C:\Users\maaya\projects\Synth\VST-Synth-V1\build\VSTSynthV1_artefacts\Debug\VST3\VST Synth V1.vst3" "C:\Program Files\Common Files\VST3\VST Synth V1.vst3" /E /I /Y
)
if %ERRORLEVEL% equ 0 (
    echo Successfully installed!
) else (
    echo Error copying files.
)
pause
