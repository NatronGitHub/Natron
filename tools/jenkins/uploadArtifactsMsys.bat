
set MSYSTEM=MINGW64
if "%BITS%"=="32" (
set MSYSTEM=MINGW32
)
set MSYS_ROOT=G:\msys64

cd %MSYS_ROOT%\home\ci\natron-support\buildmaster


%MSYS_ROOT%\usr\bin\bash.exe --login -c "cd ~/natron-support/buildmaster;./uploadArtifactsMain.sh"
