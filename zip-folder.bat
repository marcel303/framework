rem zip script by 'cam029' @https://superuser.com/questions/201371/create-zip-folder-from-the-command-line-windows
@echo off
setlocal

rem First parameter - path to dir to be zipped
rem Second parameter- zip file name
set sourceDir=%1
set zipFile=%2

rem Create PowerShell script
rem echo Write-Output 'Custom PowerShell profile in effect!'    > %~dp0TempZipScript.ps1
echo Add-Type -A System.IO.Compression.FileSystem           >> %~dp0TempZipScript.ps1
echo [IO.Compression.ZipFile]::CreateFromDirectory('%sourceDir%','%~dp0%zipFile%') >> %~dp0TempZipScript.ps1

rem Execute script with flag "-ExecutionPolicy Bypass" to get around ExecutionPolicy
PowerShell.exe -ExecutionPolicy Bypass -Command "& '%~dp0TempZipScript.ps1'"
del %~dp0TempZipScript.ps1
endlocal