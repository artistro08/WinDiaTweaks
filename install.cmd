@echo off
rem WinDiaTweaks installer: builds the add-on on this PC, adds a "Dia (Tweaks)" shortcut to the
rem Start menu, and opens Dia with the tweaks. Nothing in Dia's install folder is changed.
setlocal
cd /d "%~dp0"
powershell -NoProfile -Command "if (-not (Get-AppxPackage TheBrowserCompany.Dia)) { exit 1 }" || (echo Dia isn't installed. Get it from https://www.diabrowser.com first. & exit /b 1)

call build.cmd || (echo. & echo Build failed. See "Install" in README.md for what it needs. & exit /b 1)

set "LNK=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Dia (Tweaks).lnk"
powershell -NoProfile -Command "$s=(New-Object -ComObject WScript.Shell).CreateShortcut($env:LNK); $s.TargetPath='%~dp0bin\diatweaks.exe'; $s.WorkingDirectory='%~dp0bin'; $s.Description='Open Dia with WinDiaTweaks'; $s.WindowStyle=7; $s.Save()"
echo Added "Dia (Tweaks)" to the Start menu.

bin\diatweaks.exe restart
echo.
echo Done. Open Dia from "Dia (Tweaks)" from now on, then Settings ^> Patches.
