@echo off



if not defined DevEnvDir (
    call "D:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)



set lib1= -LIBPATH:"..\lib\ffmpeg-4.1-win64-dev\lib"
set LinkerFlags= -link %lib1% user32.lib Gdi32.lib Winmm.lib OpenGL32.lib ..\code\win32_icon\resource.res

set include1= -I"..\lib\ffmpeg-4.1-win64-dev\include"
set CompilerFlags= -nologo -FC -Zi -WX %include1%



set build_folder=build
IF NOT EXIST %build_folder% mkdir %build_folder%

set lib_folder=lib
copy %lib_folder%\ffmpeg.exe %build_folder%
xcopy /s /y /q %lib_folder%\ffmpeg-4.1-win64-shared\bin\*.dll %build_folder%

pushd %build_folder%
cl %CompilerFlags% ..\code\win32_app.cpp %LinkerFlags%
popd



set release_folder=release
IF NOT EXIST %release_folder% mkdir %release_folder%

copy %build_folder%\win32_app.exe %release_folder%\archive-stacks.exe
copy %lib_folder%\ffmpeg.exe %release_folder%
xcopy /s /y /q %lib_folder%\ffmpeg-4.1-win64-shared\bin\*.dll %release_folder%

