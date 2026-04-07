@echo off
echo ================================================
echo  Shattered Arcana - Motion Reference Library
echo  Windows Installer
echo ================================================
echo.

python --version >/dev/null 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Install from https://python.org
    pause
    exit /b 1
)

set INSTALL_DIR=%USERPROFILE%\motion_capture
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

echo Creating Python environment...
if not exist "%INSTALL_DIR%\.venv" python -m venv "%INSTALL_DIR%\.venv"
call "%INSTALL_DIR%\.venv\Scripts\activate.bat"

echo Installing dependencies...
pip install --quiet playwright playwright-stealth
playwright install chromium

copy /y "%~dp0yt_capture_win.py" "%INSTALL_DIR%\motion_capture.py" >/dev/null

:: Create shortcut
(
echo @echo off
echo call "%INSTALL_DIR%\.venv\Scripts\activate.bat"
echo python "%INSTALL_DIR%\motion_capture.py" %%*
) > "%INSTALL_DIR%\mc.bat"

echo.
echo ================================================
echo  Installed to: %INSTALL_DIR%
echo ================================================
echo.
echo USAGE:
echo.
echo   cd %INSTALL_DIR%
echo.
echo   mc                          Full auto-build (searches + captures + uploads)
echo   mc --dry-run                Show what would be captured
echo   mc --search-only            Search only, no captures
echo   mc --category knight_sword  Build specific category only
echo   mc --capture "URL"          Capture specific video
echo   mc --upload                 Upload all to dev server
echo   mc --list                   Show library contents
echo   mc --search "query"         Search YouTube
echo.
pause
