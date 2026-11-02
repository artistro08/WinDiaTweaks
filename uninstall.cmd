@echo off
rem Removes the "Dia (Tweaks)" shortcut and reopens Dia without tweaks. Then delete this folder.
setlocal
cd /d "%~dp0"
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Dia (Tweaks).lnk" 2>nul
taskkill /IM Dia.exe /F >nul 2>nul
timeout /t 2 /nobreak >nul
start "" shell:AppsFolder\TheBrowserCompany.Dia_ttt1ap7aakyb4!Dia
echo Dia reopened without tweaks. You can now delete this folder.
