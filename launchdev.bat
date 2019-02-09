@echo off

set filesToOpen=%~dp0\code\win32_app.cpp
set exeToOpen=%~dp0\build\win32_app.exe


if not defined DevEnvDir (
    call "D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)


start "" "D:\Program Files\Sublime Text 3\sublime_text.exe" %filesToOpen%

start "" "devenv" %exeToOpen% %filesToOpen%

cd /d %~dp0
start cmd.exe
