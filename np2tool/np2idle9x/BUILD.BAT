@echo off

echo Building...
nmake /f MAKEFILE vxd
if errorlevel 1 goto error
nmake /f MAKEFILE ctl
if errorlevel 1 goto error
nmake /f MAKEFILE tray
if errorlevel 1 goto error

echo.
echo Build complete
goto end

:error
echo.
echo build failed.

:end
