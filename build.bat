@echo off



if not defined DevEnvDir (
    call "D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)



IF NOT EXIST build mkdir build


pushd build
cl -Zi -FC ..\code\win32_app.cpp user32.lib Gdi32.lib Winmm.lib OpenGL32.lib ..\code\win32_icon\resource.res
popd

