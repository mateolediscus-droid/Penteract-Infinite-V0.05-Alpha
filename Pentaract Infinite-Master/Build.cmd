@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\build-release.ps1" %*
if errorlevel 1 (
  echo Build failed. See the message above.
  pause
  exit /b 1
)
