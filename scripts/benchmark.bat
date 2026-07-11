@echo off
setlocal

pushd "%~dp0.."
powershell -NoProfile -Command "$t = Measure-Command { & '.\build-msvc-tests\tests\Release\mazebuildertests.exe' '[lots of applies]' | Out-Null }; Write-Output ('ElapsedMs=' + [math]::Round($t.TotalMilliseconds, 2))"
set "exitcode=%errorlevel%"
popd

exit /b %exitcode%

