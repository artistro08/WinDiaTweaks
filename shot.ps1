param(
    [string]$Out = "$PSScriptRoot\bin\shot.png",
    [string]$Process = "Dia",
    [switch]$FullScreen,
    [switch]$Maximize
)
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
    [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
    [DllImport("dwmapi.dll")] public static extern int DwmGetWindowAttribute(IntPtr h, int attr, out RECT r, int size);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@
[Win]::SetProcessDPIAware() | Out-Null

Add-Type -AssemblyName UIAutomationClient
$__A=[System.Windows.Automation.AutomationElement]; $__p=(Get-Process Dia).Id
$__w=$__A::RootElement.FindAll([System.Windows.Automation.TreeScope]::Children,[System.Windows.Automation.Condition]::TrueCondition) | ? { $_.Current.ProcessId -in $__p -and $_.Current.Name -ne 'Settings' } | select -First 1
$h = [IntPtr]$__w.Current.NativeWindowHandle
if ($Maximize) { [Win]::ShowWindow($h, 3) | Out-Null; Start-Sleep -Milliseconds 500 }

# Windows only lets the foreground process hand focus over, so borrow its input queue for a moment.
$fg = [Win]::GetForegroundWindow()
[uint32]$fgPid = 0
$fgThread = [Win]::GetWindowThreadProcessId($fg, [ref]$fgPid)
$me = [Win]::GetCurrentThreadId()
[Win]::AttachThreadInput($me, $fgThread, $true) | Out-Null
[Win]::BringWindowToTop($h) | Out-Null
[Win]::SetForegroundWindow($h) | Out-Null
[Win]::AttachThreadInput($me, $fgThread, $false) | Out-Null
Start-Sleep -Milliseconds 800

if ($FullScreen) {
    $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $x = $b.X; $y = $b.Y; $w = $b.Width; $ht = $b.Height
} else {
    $r = New-Object Win+RECT
    if ([Win]::DwmGetWindowAttribute($h, 9, [ref]$r, 16) -ne 0) { throw "DwmGetWindowAttribute failed" }
    $x = $r.Left; $y = $r.Top; $w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
}

$bmp = New-Object System.Drawing.Bitmap $w, $ht
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($x, $y, 0, 0, $bmp.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
Write-Output "$Out ${w}x${ht} at $x,$y"
