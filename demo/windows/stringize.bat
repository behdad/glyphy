@echo off
echo.You MUST have sed.exe in your PATH
for /r ..\.. %%F in (*.glsl) do call :process_file %%~nxF %%F
exit /b
goto :skip

:process_file
rem echo Processing file %1 ...
setlocal
set filename=%1
set pathname=%2
set dot_to_dash=%pathname:.=-%
set destination=%dot_to_dash%.h
set dot_to_underscore=%filename:.=_%
set decl=%dot_to_underscore:-=_%
rem echo pathname %pathname%
rem echo destination %destination%
rem echo decl %decl%
echo.static const char* %decl% = > %destination% 
sed -f sed_enquote_lines.txt "%pathname%" >> "%destination%" 
echo.; >> %destination% 
echo.stringized: "%pathname%" to "%destination%" 
endlocal
exit /b

:skip
