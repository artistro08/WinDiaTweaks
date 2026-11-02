@echo off
rem WinDiaTweaks installer: builds the add-on on this PC, adds a "Dia (Tweaks)" shortcut to the
rem Start menu, and opens Dia with the tweaks. Nothing in Dia's install folder is changed.
setlocal
rem Full path: build.cmd narrows PATH, and some PCs lack PowerShell's folder in it.
set "PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
cd /d "%~dp0"
%PS% -NoProfile -Command "if (-not (Get-AppxPackage TheBrowserCompany.Dia)) { exit 1 }" || (echo Dia isn't installed. Get it from https://www.diabrowser.com first. & exit /b 1)

call "%~dp0build.cmd" || (echo. & echo Build failed. See "Install" in README.md for what it needs. & exit /b 1)

set "LNK=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Dia (Tweaks).lnk"
%PS% -NoProfile -Command "$s=(New-Object -ComObject WScript.Shell).CreateShortcut($env:LNK); $s.TargetPath='%~dp0bin\diatweaks.exe'; $s.WorkingDirectory='%~dp0bin'; $s.Description='Open Dia with WinDiaTweaks'; $s.WindowStyle=7; $s.Save()"
echo Added "Dia (Tweaks)" to the Start menu.

"%~dp0bin\diatweaks.exe" restart
echo.
echo Done. Open Dia from "Dia (Tweaks)" from now on, then Settings ^> Patches.

rem Optional: star the repo. Only with a yes; uses the GitHub CLI if it's signed in, else opens the page.
echo.
%SystemRoot%\System32\choice.exe /c yn /n /m "Star WinDiaTweaks on GitHub? [y/n] "
if errorlevel 2 exit /b 0
where gh >nul 2>nul || goto :starpage
gh auth status >nul 2>nul || goto :starpage
gh api -X PUT user/starred/zioder/WinDiaTweaks --silent >nul 2>nul || goto :starpage
echo Starred. Thank you!
exit /b 0
:starpage
start "" https://github.com/zioder/WinDiaTweaks
echo Opened the repo page: click the Star button at the top right. Thank you!
exit /b 0
