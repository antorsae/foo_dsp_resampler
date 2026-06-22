@echo off
setlocal

rem set _component=foo_dsp_x

pushd "%~d0%~p0"

for %%i in ("%cd%") do set _component=%%~ni%%~xi

if not exist "License\License.txt" (
  echo Valid license directory is missing.
  goto :error
)
if not exist "Release\%_component%.dll" (
  echo Release\%_component%.dll is missing.
  goto :error
)
if not exist "x64\Release\%_component%.dll" (
  echo x64\Release\%_component%.dll is missing.
  goto :error
)

if exist "%_component%.dll" del/q/f "%_component%.dll">nul: 2>&1
if exist "x64\%_component%.dll" del/q/f "x64\%_component%.dll">nul: 2>&1
if exist "%_component%.zip" del/q/f "%_component%.zip">nul: 2>&1

copy/y "Release\%_component%.dll" ".\">nul: 2>&1
copy/y "x64\Release\%_component%.dll" "x64\">nul: 2>&1
7za a -tzip -mx=9 "%_component%.zip" "%_component%.dll" "x64\%_component%.dll" "License\*"
if errorlevel 1 (
  echo Failed to create component archive "%_component%.zip"
  goto :error
)

if exist "%_component%.dll" del/q/f "%_component%.dll">nul: 2>&1
if exist "x64\%_component%.dll" del/q/f "x64\%_component%.dll">nul: 2>&1

advzip -z -4 "%_component%.zip"
if errorlevel 1 (
  echo Failed to optimize component zip "%_component%.zip"
  goto :error
)

if exist "%_component%.fb2k-component" del/q/f "%_component%.fb2k-component">nul: 2>&1
ren "%_component%.zip" "%_component%.fb2k-component"
if errorlevel 1 (
  echo Failed to rename %_component%.zip to %_component%.fb2k-component
  goto :error
)

touch -c -m -x -r "Release\%_component%.dll" "%_component%.fb2k-component" >nul: 2>&1

goto :end

:error
pause

:end
popd
endlocal
