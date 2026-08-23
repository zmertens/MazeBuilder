@echo off
setlocal

pushd "%~dp0.."
powershell -NoProfile -Command "$t = Measure-Command { & '.\amazing.exe' '-j resource_paths.json' | Out-Null };"
set "exitcode=%errorlevel%"
popd

exit /b %exitcode%

